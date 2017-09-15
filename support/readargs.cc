/*
 * readargs.cc
 * 
 * created 26.10.94 PW
 * 05.06.1998 PW: adapted for STD_STRICT_ANSI
 * 30.Jul.2004 PW: ems_u64 added
 */

#include "config.h"
#include <iostream>
#include <sstream>
#include <iomanip>

#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include "versions.hxx"

#include <cstdarg>
#include "readargs.hxx"

VERSION("2008-Nov-15", __FILE__, __DATE__, __TIME__,
"$ZEL: readargs.cc,v 2.32 2010/06/20 22:46:18 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

class C_option;

class C_option_manip
  {
  private:
    typedef ostream& (*option_manip) (ostream&, const C_option&);
    option_manip f;
    const C_option& option;
  public:
    C_option_manip(option_manip, const C_option&);
    friend ostream& operator <<(ostream&, const C_option_manip&);
  };

#ifdef getchar
#undef getchar
#endif

class C_option
  {
  friend class C_readargs;
  friend ostream& flag(ostream&, const C_option&);
  friend ostream& flagname(ostream&, const C_option&);
  friend ostream& defval(ostream&, const C_option&);
  friend ostream& typname(ostream&, const C_option&);
  friend ostream& value(ostream&, const C_option&);
  friend ostream& envvar(ostream&, const C_option&);
  protected:
    C_option(const char*, const char*, const char*, const char*);
    C_option(const char*, int, const char*, const char*);
    virtual ~C_option();
    char* name;
    char* flag;
    int pos;
    char* descr;
    char* fname;
    char** envlist;
    unsigned int envlistsize;
             int multi_soll;
    unsigned int multi_ist;
    virtual int convert(const char* s, C_readargs::argtype)=0;
    int isarg;
    int isenv;
    int isdefault() const;
    int isenviron() const;
    int isargument() const;
    virtual int multiplicity(int)=0;
    void printflag(ostream&) const;
    virtual void printflagname(ostream&) const=0;
    virtual void printtypname(ostream&) const=0;
    virtual void printdefval(ostream&) const =0;
    virtual void printvalue(ostream&) const =0;
    void printenvvar(ostream&) const;
    int flaglen() const;
    int fnamelen() const;
    void add_env(const char*);
    void use_env();
  };

C_option_manip flag(const C_option&);
C_option_manip flag(const C_option*);
C_option_manip flagname(const C_option&);
C_option_manip flagname(const C_option*);
C_option_manip typname(const C_option&);
C_option_manip typname(const C_option*);
C_option_manip defval(const C_option&);
C_option_manip defval(const C_option*);
C_option_manip value(const C_option&);
C_option_manip value(const C_option*);
C_option_manip envvar(const C_option&);
C_option_manip envvar(const C_option*);

class C_booloption: public C_option
  {
  friend class C_readargs;
  protected:
    C_booloption(const char*, const char*, bool, const char*, const char*);
    C_booloption(const char*, int, bool, const char*, const char*);
    virtual ~C_booloption();
    virtual int convert(const char* s, C_readargs::argtype);
    bool def;
    bool* vals;
    virtual int multiplicity(int);
    virtual void printflagname(ostream&) const;
    virtual void printtypname(ostream&) const;
    virtual void printdefval(ostream&) const;
    virtual void printvalue(ostream&) const;
  };
class C_intoption: public C_option
  {
  friend class C_readargs;
  protected:
    C_intoption(const char*, const char*, int, const char*, const char*);
    C_intoption(const char*, int, int, const char*, const char*);
    virtual ~C_intoption();
    virtual int convert(const char* s, C_readargs::argtype);
    int def;
    int* vals;
    virtual int multiplicity(int);
    virtual void printflagname(ostream&) const;
    virtual void printtypname(ostream&) const;
    virtual void printdefval(ostream&) const;
    virtual void printvalue(ostream&) const;
  };
class C_u64option: public C_option {
    friend class C_readargs;
    protected:
        C_u64option(const char*, const char*, ems_u64, const char*, const char*);
        C_u64option(const char*, int, ems_u64, const char*, const char*);
        virtual ~C_u64option();
        virtual int convert(const char* s, C_readargs::argtype);
        ems_u64 def;
        ems_u64* vals;
        virtual int multiplicity(int);
        virtual void printflagname(ostream&) const;
        virtual void printtypname(ostream&) const;
        virtual void printdefval(ostream&) const;
        virtual void printvalue(ostream&) const;
};
class C_realoption: public C_option {
  friend class C_readargs;
  protected:
    C_realoption(const char*, const char*, double, const char*, const char*);
    C_realoption(const char*, int, double, const char*, const char*);
    virtual ~C_realoption();
    virtual int convert(const char* s, C_readargs::argtype);
    double def;
    double* vals;
    virtual int multiplicity(int);
    virtual void printflagname(ostream&) const;
    virtual void printtypname(ostream&) const;
    virtual void printdefval(ostream&) const;
    virtual void printvalue(ostream&) const;
};
class C_charoption: public C_option
  {
  friend class C_readargs;
  protected:
    C_charoption(const char*, const char*, char, const char*, const char*);
    C_charoption(const char*, int, char, const char*, const char*);
    virtual ~C_charoption();
    virtual int convert(const char* s, C_readargs::argtype);
    char def;
    char* vals;
    virtual int multiplicity(int);
    virtual void printflagname(ostream&) const;
    virtual void printtypname(ostream&) const;
    virtual void printdefval(ostream&) const;
    virtual void printvalue(ostream&) const;
  };
class C_stringoption: public C_option
  {
  friend class C_readargs;
  protected:
    C_stringoption(const char*, const char*, const char*, const char*,
        const char*);
    C_stringoption(const char*, int, const char*, const char*,
        const char*);
    virtual ~C_stringoption();
    virtual int convert(const char* s, C_readargs::argtype);
    char* def;
    char** vals;
    virtual int multiplicity(int);
    virtual void printflagname(ostream&) const;
    virtual void printtypname(ostream&) const;
    virtual void printdefval(ostream&) const;
    virtual void printvalue(ostream&) const;
  };

/*****************************************************************************/

C_readargs::C_readargs(int argc, char* argv[])
:argc(argc), argv(argv),
    optlist(0), optlistsize(0),
    poptlist(0), poptlistsize(0),
    requiredlist(0), requiredlistsize(0),
    exclusivelists(0), exclusivelistsizes(0), numexclusivelists(0),
    implicitlists(0), implicitlistsizes(0), numimplicitlists(0),
    demandlists(0), demandlistsizes(0), numdemandlists(0),
    names(0), nnames(0), interpreted(0), numhelpinsets(0),
    helpinsets(0)
{
baseprogname_=strrchr(argv[0], '/');
if (baseprogname_==0) baseprogname_=argv[0];
}

/*****************************************************************************/

C_readargs::~C_readargs()
{
int i;

if (optlist) for (i=0; i<optlistsize; i++) delete optlist[i];
delete[] optlist;
if (names) for (i=0; i<nnames; i++) delete[] names[i];
delete[] names;

delete requiredlist;
for (i=0; i<numexclusivelists; i++) delete[] exclusivelists[i];
delete[] exclusivelistsizes;
delete[] exclusivelists;
for (i=0; i<numimplicitlists; i++) delete[] implicitlists[i];
delete[] implicitlistsizes;
delete[] implicitlists;
for (i=0; i<numdemandlists; i++) delete[] demandlists[i];
delete[] demandlistsizes;
delete[] demandlists;
}

/*****************************************************************************/
C_option*
C_readargs::findentry(const char* name) const
{
    for (int i=0; i<optlistsize; i++)
        if (strcmp(name, optlist[i]->name)==0)
            return optlist[i];
    for (int i=0; i<poptlistsize; i++)
        if (strcmp(name, poptlist[i]->name)==0)
            return poptlist[i];
    return 0;
}
/*****************************************************************************/

int C_readargs::testflag(const char* flag) const
{
int i;

for (i=0; i<optlistsize; i++)
    if (strcmp(flag, optlist[i]->flag)==0) return(-1);
return(0);
}

/*****************************************************************************/

int C_readargs::testpos(int pos) const
{
int i;

for (i=0; i<poptlistsize; i++)
    if (pos==poptlist[i]->pos) return(-1);
return(0);
}

/*****************************************************************************/

C_option* C_readargs::findflag(const char* flag, int n) const
{
int i, num;
C_option* opt=0;

// flag exakt?
for (num=0, i=0; i<optlistsize; i++)
  {
  if (strcmp(flag, optlist[i]->flag)==0) return(optlist[i]);
  }

// flag abgekuerzt?
for (num=0, i=0; i<optlistsize; i++)
  {
  if (strncmp(flag, optlist[i]->flag, n)==0) {opt=optlist[i]; num++;}
  }
// trotzdem eindeutig?
if (num==1)
  return(opt);
else if (num>1)
  return(reinterpret_cast<C_option*>(-1));
else
  return(0);
}

/*****************************************************************************/

C_option* C_readargs::findpos(int pos) const
{
int i;

for (i=0; i<poptlistsize; i++) if (pos==poptlist[i]->pos) return(poptlist[i]);
return(0);
}

/*****************************************************************************/

int C_readargs::addopt(const char* name, const char* flag)
{
C_option** help;
int i;

if (findentry(name)!=0)
  {
  cerr << "C_readargs::addopt: duplicate option \"" << name << "\"." << endl;
  return(-1);
  }
if (testflag(flag)!=0)
  {
  cerr << "C_readargs::addopt: duplicate flag \"" << flag << "\"." << endl;
  return(-1);
  }
help=new C_option*[optlistsize+1];
for (i=0; i<optlistsize; i++) help[i]=optlist[i];
delete[] optlist;
optlist=help;
return(0);
}

/*****************************************************************************/

int C_readargs::addopt(const char* name, int pos)
{
C_option** help;
int i;

if (findentry(name)!=0)
  {
  cerr << "C_readargs::addopt: duplicate option \"" << name << "\"." << endl;
  return(-1);
  }
if (testpos(pos)!=0)
  {
  cerr << "C_readargs::addopt(\"" << name << "\"): duplicate position \""
      << pos << "\"." << endl;
  return(-1);
  }
help=new C_option*[poptlistsize+1];
for (i=0; i<poptlistsize; i++) help[i]=poptlist[i];
delete[] poptlist;
poptlist=help;
return(0);
}

/*****************************************************************************/

void C_readargs::addoption(const char* name, const char* flag, bool def,
    const char* descr, const char* fname)
{
if (addopt(name, flag)!=0) return;
optlist[optlistsize++]=new C_booloption(name, flag, def, descr, fname);
}

/*****************************************************************************/

void C_readargs::addoption(const char* name, const char* flag, int def,
    const char* descr, const char* fname)
{
if (addopt(name, flag)!=0) return;
optlist[optlistsize++]=new C_intoption(name, flag, def, descr, fname);
}

/*****************************************************************************/

void C_readargs::addoption(const char* name, const char* flag, ems_u64 def,
    const char* descr, const char* fname)
{
if (addopt(name, flag)!=0) return;
optlist[optlistsize++]=new C_u64option(name, flag, def, descr, fname);
}

/*****************************************************************************/
void C_readargs::addoption(const char* name, const char* flag, double def,
    const char* descr, const char* fname)
{
    if (addopt(name, flag)!=0)
        return;
    optlist[optlistsize++]=new C_realoption(name, flag, def, descr, fname);
}
/*****************************************************************************/

void C_readargs::addoption(const char* name, const char* flag, char def,
    const char* descr, const char* fname)
{
if (addopt(name, flag)!=0) return;
optlist[optlistsize++]=new C_charoption(name, flag, def, descr, fname);
}

/*****************************************************************************/

void C_readargs::addoption(const char* name, const char* flag, const char* def,
    const char* descr, const char* fname)
{
if (addopt(name, flag)!=0) return;
optlist[optlistsize++]=new C_stringoption(name, flag, def, descr, fname);
}

/*****************************************************************************/

void C_readargs::addoption(const char* name, int pos, bool def,
    const char* descr, const char* fname)
{
if (addopt(name, pos)!=0) return;
poptlist[poptlistsize++]=new C_booloption(name, pos, def, descr, fname);
}

/*****************************************************************************/

void C_readargs::addoption(const char* name, int pos, int def,
    const char* descr, const char* fname)
{
if (addopt(name, pos)!=0) return;
poptlist[poptlistsize++]=new C_intoption(name, pos, def, descr, fname);
}

/*****************************************************************************/
void C_readargs::addoption(const char* name, int pos, double def,
    const char* descr, const char* fname)
{
    if (addopt(name, pos)!=0)
        return;
    poptlist[poptlistsize++]=new C_realoption(name, pos, def, descr, fname);
}
/*****************************************************************************/

void C_readargs::addoption(const char* name, int pos, char def,
    const char* descr, const char* fname)
{
if (addopt(name, pos)!=0) return;
poptlist[poptlistsize++]=new C_charoption(name, pos, def, descr, fname);
}

/*****************************************************************************/

void C_readargs::addoption(const char* name, int pos, const char* def,
    const char* descr, const char* fname)
{
if (addopt(name, pos)!=0) return;
poptlist[poptlistsize++]=new C_stringoption(name, pos, def, descr, fname);
}

/*****************************************************************************/
int C_readargs::use_env(const char* optname, const char* envname)
{
    //const char* envval;
    C_option* option;

    //if ((envval=getenv(envname))==0) return(0);

    if ((option=findentry(optname))==0) {
        cerr << "C_readargs::use_env: no Option \"" << optname << "\"." << endl;
        return -1;
    }
    //return(option->convert(envval, environment));
    option->add_env(envname);
    return 0;
}
/*****************************************************************************/
int C_readargs::parse_args()
{
    int argidx=1, fehler=0;
    int onlynames=0;

    while (!fehler && (argidx<argc)) {
        char* s;
        s=argv[argidx];
        argidx++;
        if (!onlynames && (strcmp(s, "--")==0)) {
            onlynames=1;
        } else if (!onlynames && ((s[0]=='-') || (s[0]=='+'))) {
            char boolarg=s[0];
            while ((s[0]=='-') || (s[0]=='+')) s++;
            char* p1=strchr(s, '=');
            char* p2=strchr(s, ' ');
            char* p; char c='$';
            if ((p1!=0) && (p2!=0))
                p=p1<p2?p1:p2;
            else if (p1!=0)
                p=p1;
            else if (p2!=0)
                p=p2;
            else
                p=0;
            if (p!=0) {
                c=*p;
                *p=0;
            }
            C_option* opt=findflag(s, strlen(s));
            if (opt==0) {
                cerr << "unknown option \"" << s << "\"." << endl;
                fehler=-1;
            } else if (opt==reinterpret_cast<C_option*>(-1)) {
                cerr << "ambigious option \"" << s << "\"." << endl;
                fehler=-1;
            } else {
                if (p!=0) {
                    fehler=opt->convert(p+1, argument);
                } else if ((argidx<argc)&&((dynamic_cast<C_booloption*>(opt)==0) ||
                          ((argv[argidx][0]!='-')&&(argv[argidx][0]!='+')))) {
                    fehler=opt->convert(argv[argidx], argument);
                    argidx++;
                } else if (dynamic_cast<C_booloption*>(opt)!=0) {
                    char x[2]={boolarg, 0};
                    fehler=opt->convert(x, argument);
                } else {
                    cerr << "argument for flag \""<<flag(opt)<<"\" missing"<<endl;
                    fehler=-1;
                }
            }
            if (p!=0) *p=c;
        } else { // onlynames || s[0]!='-'
            C_option* opt;
            if ((opt=findpos(nnames))!=0)
                fehler=opt->convert(s, argument);
            addname(s);
        }
    }  
    return fehler;
}
/*****************************************************************************/

int C_readargs::check_environment()
{
int i;
for (i=0; i<optlistsize; i++)
  if (!optlist[i]->isargument()) optlist[i]->use_env();
for (i=0; i<poptlistsize; i++)
  if (!poptlist[i]->isargument()) poptlist[i]->use_env();
return 0;
}

/*****************************************************************************/
int C_readargs::check_implicit_options()
{
    int changed;
    do {
        changed=0;
        for (int i=0; i<numimplicitlists; i++) {
            int size;
            size=implicitlistsizes[i];
            if ((size>1) && implicitlists[i][0]->isargument()) {
                for (int j=1; j<size; j++) {
                    C_booloption* b_opt=dynamic_cast<C_booloption*>(implicitlists[i][j]);
                    if (!b_opt->vals[0]) { // XXX warum?
                        b_opt->convert(0, C_readargs::argument);
                        changed=1;
                    }
                }
            }
        }
    } while (changed);
    return 0;
}
/*****************************************************************************/

int C_readargs::check_required_options()
{
int i, fehler=0;
for (i=0; i<requiredlistsize; i++)
  if (requiredlist[i]->isdefault())
    {
    cerr << flag(requiredlist[i]) << " required." << endl;
    fehler=-1;
    }
return fehler;
}

/*****************************************************************************/

int C_readargs::check_exclusive_options()
{
int i, fehler=0;
for (i=0; i<numexclusivelists; i++)
  {
  int j, size;
  int count=0;
  size=exclusivelistsizes[i];
  for (j=0; j<size; j++)
      if (exclusivelists[i][j]->isargument()) count++;
  if (count>1)
    {
    for (j=0; j<size; j++)
      {
      cerr << flag(exclusivelists[i][j]);
      if (j+2<size)
        cerr << ", ";
      else if (j+1<size)
        cerr << " and ";
      }
    cerr << " are exclusive." << endl;
    fehler=-1;
    }
  }
return fehler;
}

/*****************************************************************************/
int C_readargs::check_demand_options()
{
    int i, fehler=0;
    for (i=0; i<numdemandlists; i++) {
        int j, size;

        size=demandlistsizes[i];
        if ((size>1) && !demandlists[i][0]->isdefault()) {
            for (j=1; j<size; j++) {
                if (demandlists[i][j]->isdefault()) {
                    cerr<<flag(demandlists[i][0])<<" requires "
                            <<flag(demandlists[i][j])<<endl;
                    fehler=-1;
                }
            }
        }
    }
    return fehler;
}
/*****************************************************************************/
int
C_readargs::interpret(bool autohelp)
{
    int fehler;

    if (autohelp) {
        if (findentry("help")==0)
            addoption("help", "help", false, "this text");
    }

    interpreted=1;

    fehler=parse_args();

    if (fehler==0) fehler=check_environment();

    if (fehler==0) fehler=check_implicit_options();

    if (fehler==0) fehler=check_required_options();

    if (fehler==0) fehler=check_exclusive_options();

    if (fehler==0) fehler=check_demand_options();

    if (autohelp && ((fehler==-1) || getboolopt("help"))) {
        printhelp();
        fehler=-1;
    }
    return(fehler);
}
/*****************************************************************************/
bool
C_readargs::getboolopt(const char* name, unsigned int idx) const
{
    C_option* opt_;
    C_booloption* opt;

    if ((opt_=findentry(name))==0) {
        cerr<<"C_readargs::getboolopt: \""<<name<<"\" not found."<<endl;
        return false;
    }

    if ((opt=dynamic_cast<C_booloption*>(opt_))==0) {
        cerr<<"C_readargs::getboolopt: \""<<name<<"\" is not boolean."<<endl;
        return false;
    }

    if ((idx==0) && opt->isdefault()) {
        return opt->def;
    } else if (idx<opt->multi_ist) {
        return opt->vals[idx];
    } else {
        cerr<<"C_readargs::getboolopt: \""<<name<<"\": no value for index "
            << idx << endl;
        return false;
    }
}

/*****************************************************************************/
int
C_readargs::getintopt(const char* name, unsigned int idx) const
{
    C_option* opt_;
    C_intoption* opt;

    if ((opt_=findentry(name))==0) {
        cerr<<"C_readargs::getintopt: \""<<name<<"\" not found."<<endl;
        return false;
    }

    if ((opt=dynamic_cast<C_intoption*>(opt_))==0) {
        cerr<<"C_readargs::getintopt: \""<<name<<"\" is not integer."<<endl;
        return false;
    }

    if ((idx==0) && opt->isdefault()) {
        return opt->def;
    } else if (idx<opt->multi_ist) {
        return opt->vals[idx];
    } else {
        cerr<<"C_readargs::getintopt: \""<<name<<"\": no value for index "
            << idx << endl;
        return false;
    }
}
/*****************************************************************************/
ems_u64
C_readargs::getu64opt(const char* name, unsigned int idx) const
{
    C_option* opt_;
    C_u64option* opt;

    if ((opt_=findentry(name))==0) {
        cerr<<"C_readargs::getu64opt: \""<<name<<"\" not found."<<endl;
        return false;
    }

    if ((opt=dynamic_cast<C_u64option*>(opt_))==0) {
        cerr<<"C_readargs::getu64opt: \""<<name<<"\" is not u64."<<endl;
        return false;
    }

    if ((idx==0) && opt->isdefault()) {
        return opt->def;
    } else if (idx<opt->multi_ist) {
        return opt->vals[idx];
    } else {
        cerr<<"C_readargs::getu64opt: \""<<name<<"\": no value for index "
            << idx << endl;
        return false;
    }
}
/*****************************************************************************/
double
C_readargs::getrealopt(const char* name, unsigned int idx) const
{
    C_option* opt_;
    C_realoption* opt;

    if ((opt_=findentry(name))==0) {
        cerr<<"C_readargs::getrealopt: \""<<name<<"\" not found."<<endl;
        return false;
    }

    if ((opt=dynamic_cast<C_realoption*>(opt_))==0) {
        cerr<<"C_readargs::getrealopt: \""<<name<<"\" is not real."<<endl;
        return false;
    }

    if ((idx==0) && opt->isdefault()) {
        return opt->def;
    } else if (idx<opt->multi_ist) {
        return opt->vals[idx];
    } else {
        cerr<<"C_readargs::getrealopt: \""<<name<<"\": no value for index "
            << idx << endl;
        return false;
    }
}
/*****************************************************************************/
char
C_readargs::getcharopt(const char* name, unsigned int idx) const
{
    C_option* opt_;
    C_charoption* opt;

    if ((opt_=findentry(name))==0) {
        cerr<<"C_readargs::getcharopt: \""<<name<<"\" not found."<<endl;
        return false;
    }

    if ((opt=dynamic_cast<C_charoption*>(opt_))==0) {
        cerr<<"C_readargs::getcharopt: \""<<name<<"\" is not character."<<endl;
        return false;
    }

    if ((idx==0) && opt->isdefault()) {
        return opt->def;
    } else if (idx<opt->multi_ist) {
        return opt->vals[idx];
    } else {
        cerr<<"C_readargs::getcharopt: \""<<name<<"\": no value for index "
            << idx << endl;
        return false;
    }
}
/*****************************************************************************/
const char*
C_readargs::getstringopt(const char* name, unsigned int idx) const
{
    C_option* opt_;
    C_stringoption* opt;

    if ((opt_=findentry(name))==0) {
        cerr<<"C_readargs::getstringopt: \""<<name<<"\" not found."<<endl;
        return false;
    }

    if ((opt=dynamic_cast<C_stringoption*>(opt_))==0) {
        cerr<<"C_readargs::getstringopt: \""<<name<<"\" is not string."<<endl;
        return false;
    }

    if ((idx==0) && opt->isdefault()) {
        return opt->def;
    } else if (idx<opt->multi_ist) {
        return opt->vals[idx];
    } else {
        cerr<<"C_readargs::getstringopt: \""<<name<<"\": no value for index "
            << idx << endl;
        return false;
    }
}
/*****************************************************************************/

int C_readargs::isdefault(const char* name) const
{
C_option* opt;

if ((opt=findentry(name))==0)
  {
  cerr << "C_readargs::isdefault: \"" << name << "\" not found." << endl;
  return(1);
  }
return opt->isdefault();
}

/*****************************************************************************/

int C_readargs::isenviron(const char* name) const
{
C_option* opt;

if ((opt=findentry(name))==0)
  {
  cerr << "C_readargs::isenviron: \"" << name << "\" not found." << endl;
  return(1);
  }
return opt->isenviron();
}

/*****************************************************************************/

int C_readargs::isargument(const char* name) const
{
C_option* opt;

if ((opt=findentry(name))==0)
  {
  cerr << "C_readargs::isargument: \"" << name << "\" not found." << endl;
  return(1);
  }
return(opt->isargument());
}

/*****************************************************************************/

int C_readargs::multiplicity(const char* name, int mult)
{
C_option* opt;

if (interpreted)
  {
  cerr << "C_readargs::multiplicity: \"" << name
      << "\": cannot change multiplicity during " << endl
      << "or after interpretation." << endl;
  return -1;
  }

if ((opt=findentry(name))==0)
  {
  cerr << "C_readargs::multiplicity: \"" << name << "\" not found." << endl;
  return -1;
  }

return(opt->multiplicity(mult));
}

/*****************************************************************************/

int C_readargs::multiplicity(const char* name) const
{
C_option* opt;

if ((opt=findentry(name))==0)
  {
  cerr << "C_readargs::multiplicity: \"" << name << "\" not found." << endl;
  return(-1);
  }

if (interpreted)
  return(opt->multi_ist);
else
  return(opt->multi_soll);
}
/*****************************************************************************/

int C_readargs::numnames() const
{
return(nnames);
}

/*****************************************************************************/

const char* C_readargs::getnames(int i) const
{
if ((i>=0) && (i<nnames))
  return(names[i]);
else
  return(0);
}

/*****************************************************************************/

void C_readargs::addname(const char* name)
{
char** help;
int i;

help=new char*[nnames+1];
for (i=0; i<nnames; i++) help[i]=names[i];
delete[] names;
names=help;
names[nnames]=new char[strlen(name)+1];
strcpy(names[nnames], name);
nnames++;
}

/*****************************************************************************/
void C_readargs::hints(hint hint_, ...)
{
int count;

va_list vl;
C_option*** liste=0;
int* zaehler=0;

va_start(vl, hint_);
count=0;
while (va_arg(vl, const char*)!=0) count++;
va_end(vl);

switch (hint_)
  {
  case required:
    liste=&requiredlist;
    zaehler=&requiredlistsize;
    break;
  case exclusive:
    {
    if (count<2)
      {
      cerr << "C_readargs::hints(exclusive...): count<2" << endl;
      return;
      }
    int i;
    C_option*** help=new C_option**[numexclusivelists+1];
    for (i=0; i<numexclusivelists; i++) help[i]=exclusivelists[i];
    delete[] exclusivelists;
    exclusivelists=help;
    int* ihelp=new int[numexclusivelists+1];
    for (i=0; i<numexclusivelists; i++) ihelp[i]=exclusivelistsizes[i];
    delete[] exclusivelistsizes;
    exclusivelistsizes=ihelp;
    exclusivelists[numexclusivelists]=0;
    exclusivelistsizes[numexclusivelists]=0;
    liste=exclusivelists+numexclusivelists;
    zaehler=exclusivelistsizes+numexclusivelists;
    numexclusivelists++;
    }
    break;
  case implicit:
    {
    if (count<2)
      {
      cerr << "C_readargs::hints(implicit...): count<2" << endl;
      return;
      }
    int i;
    C_option*** help=new C_option**[numimplicitlists+1];
    if (!help) return;
    for (i=0; i<numimplicitlists; i++) help[i]=implicitlists[i];
    delete[] implicitlists;
    implicitlists=help;
    int* ihelp=new int[numimplicitlists+1];
    for (i=0; i<numimplicitlists; i++) ihelp[i]=implicitlistsizes[i];
    delete[] implicitlistsizes;
    implicitlistsizes=ihelp;
    implicitlists[numimplicitlists]=0;
    implicitlistsizes[numimplicitlists]=0;
    liste=implicitlists+numimplicitlists;
    zaehler=implicitlistsizes+numimplicitlists;
    numimplicitlists++;
    }
    break;
  case demands:
    {
    if (count<2)
      {
      cerr << "C_readargs::hints(demands...): count<2" << endl;
      return;
      }
    int i;
    C_option*** help=new C_option**[numdemandlists+1];
    if (!help) return;
    for (i=0; i<numdemandlists; i++) help[i]=demandlists[i];
    delete[] demandlists;
    demandlists=help;
    int* ihelp=new int[numdemandlists+1];
    for (i=0; i<numdemandlists; i++) ihelp[i]=demandlistsizes[i];
    delete[] demandlistsizes;
    demandlistsizes=ihelp;
    demandlists[numdemandlists]=0;
    demandlistsizes[numdemandlists]=0;
    liste=demandlists+numdemandlists;
    zaehler=demandlistsizes+numdemandlists;
    numdemandlists++;
    }
    break;
  }

{
const char* name;
int i;
C_option* opt;

C_option** help=new C_option*[*zaehler+count];
for (i=0; i<*zaehler; i++) help[i]=(*liste)[i];
delete[] *liste;
*liste=help;

va_start(vl, hint_);
for (i=0; i<count; i++)
  {
  name=va_arg(vl, const char*);
  if ((opt=findentry(name))==0)
    {
    cerr << "C_readargs::hints(): \"" << name << "\" not found." << endl;
    va_end(vl);
    return;
    }
  if ((hint_==implicit) && (i>0) && (dynamic_cast<C_booloption*>(opt)==0))
    {
    cerr << "C_readargs::hints(): \"" << name << "\" is not boolean." << endl;
    va_end(vl);
    return;
    }
  (*liste)[(*zaehler)++]=opt;
  }

va_end(vl);
}
}

/*****************************************************************************/

void C_readargs::helpinset(int pos, insetfunc func)
{
if (pos>=numhelpinsets)
  {
  insetfunc* help=new insetfunc[pos+1];
  int i;
  for (i=0; i<numhelpinsets; i++) help[i]=helpinsets[i];
  for (; i<=pos; i++) help[i]=0;
  delete[] helpinsets;
  helpinsets=help;
  numhelpinsets=pos+1;
  }
helpinsets[pos]=func;
}

/*****************************************************************************/

void C_readargs::printhelp() const
{
int i;
C_option* opt;

if ((numhelpinsets>0) && (helpinsets[0]!=0)) helpinsets[0](cerr);
cerr << endl << "usage:" << endl << "  " << progname();
if ((numhelpinsets>1) && (helpinsets[1]!=0)) helpinsets[1](cerr);
for (i=0; i<optlistsize; i++)
  {
  if (dynamic_cast<C_booloption*>(optlist[i])!=0)
    cerr << " -" << optlist[i]->flag;
  else
    cerr << " -" << optlist[i]->flag << " <" << flagname(optlist[i]) << ">";
  }
if ((numhelpinsets>2) && (helpinsets[2]!=0)) helpinsets[2](cerr);
i=0;
while ((opt=findpos(i++))!=0)
  {
  cerr << " <" << flagname(opt) << ">";
  }
cerr << endl << endl;
if ((numhelpinsets>3) && (helpinsets[3]!=0)) helpinsets[3](cerr);
int size=0, msize=0, rest;

for (i=0; i<optlistsize; i++)
  if (optlist[i]->descr)
    {
    // "-_X(bool)__:_"
    size=optlist[i]->flaglen();
    if (size>msize) msize=size;
    }
i=0;
while ((opt=findpos(i++))!=0)
  if (opt->descr)
    {
    // "__X:_(bool)__:_"
    // "__(bool)__:_"           wird ignoriert, da zu kurz
    if (opt->fname) size=opt->fnamelen();
    if (size>msize) msize=size;
    }
if (msize>30) msize=30;

for (i=0; i<optlistsize; i++)
  {
  if (optlist[i]->descr)
    {
    int j;
    rest=msize-optlist[i]->flaglen();
    if (rest<0) rest=0;
    cerr << "  -" << optlist[i]->flag << ' ' << typname(optlist[i]);
    for (j=0; j<rest; j++) cerr << ' ';
    cerr << ": " << optlist[i]->descr << envvar(optlist[i]) << endl;
    }
  }
if ((numhelpinsets>4) && (helpinsets[4]!=0)) helpinsets[4](cerr);
i=0;
while ((opt=findpos(i++))!=0)
  {
  if (opt->descr)
    {
    int j;
    if (opt->fname)
      {
      rest=msize-strlen(opt->fname);
      cerr << "  " << opt->fname << ": ";
      }
    else
      {
      rest=msize+2;
      cerr << "  ";
      }
    if (rest<0) rest=0;
    cerr << typname(opt);
    for (j=0; j<rest; j++) cerr << ' ';
    cerr << ": " << opt->descr << envvar(opt) << endl;
    }
  }

if ((numhelpinsets>5) && (helpinsets[5]!=0)) helpinsets[5](cerr);
cerr << endl << "values (default ==> actual):" << endl << endl;
if ((numhelpinsets>6) && (helpinsets[6]!=0)) helpinsets[6](cerr);

for (i=0; i<optlistsize; i++)
  {
  int j;
  rest=msize-optlist[i]->flaglen();
  if (rest<0) rest=0;
  cerr << "  -" << optlist[i]->flag;
  for (j=0; j<rest; j++) cerr << ' ';
  cerr << " : " << defval(optlist[i]);
  if (!optlist[i]->isdefault()) cerr << " ==> " << value(optlist[i]);
  cerr << endl;
  }
if ((numhelpinsets>7) && (helpinsets[7]!=0)) helpinsets[7](cerr);
i=0;
while ((opt=findpos(i++))!=0)
  {
  int j;
  if (opt->fname)
    {
    rest=msize-strlen(opt->fname);
    cerr << "   " << opt->fname;
    }
  else
    {
    rest=msize;
    cerr << "   ";
    }
  if (rest<0) rest=0;
  for (j=0; j<rest; j++) cerr << ' ';
  cerr << " : " << defval(opt);
  if (!opt->isdefault()) cerr << " ==> " << value(opt);
  cerr << endl;
  }

cerr << endl;
if ((numhelpinsets>8) && (helpinsets[8]!=0)) helpinsets[8](cerr);
}

/*****************************************************************************/

C_option_manip::C_option_manip(option_manip f, const C_option& copt)
:f(f), option(copt)
{}

/*****************************************************************************/

ostream& operator <<(ostream& os, const C_option_manip& manip)
{
return(manip.f(os, manip.option));
}

/*****************************************************************************/

ostream& flag(ostream& os, const C_option& copt)
{
copt.printflag(os);
return(os);
}

/*****************************************************************************/

ostream& flagname(ostream& os, const C_option& copt)
{
copt.printflagname(os);
return(os);
}

/*****************************************************************************/

ostream& typname(ostream& os, const C_option& copt)
{
copt.printtypname(os);
return(os);
}

/*****************************************************************************/

ostream& defval(ostream& os, const C_option& copt)
{
copt.printdefval(os);
return(os);
}

/*****************************************************************************/

ostream& value(ostream& os, const C_option& copt)
{
copt.printvalue(os);
return(os);
}

/*****************************************************************************/

ostream& envvar(ostream& os, const C_option& copt)
{
copt.printenvvar(os);
return(os);
}

/*****************************************************************************/

C_option_manip flag(const C_option& copt)
{
return(C_option_manip(&flag, copt));
}

/*****************************************************************************/

C_option_manip flag(const C_option* copt)
{
return(C_option_manip(&flag, *copt));
}

/*****************************************************************************/

C_option_manip flagname(const C_option& copt)
{
return(C_option_manip(&flagname, copt));
}

/*****************************************************************************/

C_option_manip flagname(const C_option* copt)
{
return(C_option_manip(&flagname, *copt));
}

/*****************************************************************************/

C_option_manip typname(const C_option& copt)
{
return(C_option_manip(&typname, copt));
}

/*****************************************************************************/

C_option_manip typname(const C_option* copt)
{
return(C_option_manip(&typname, *copt));
}

/*****************************************************************************/

C_option_manip defval(const C_option& copt)
{
return(C_option_manip(&defval, copt));
}

/*****************************************************************************/

C_option_manip defval(const C_option* copt)
{
return(C_option_manip(&defval, *copt));
}

/*****************************************************************************/

C_option_manip value(const C_option& copt)
{
return(C_option_manip(&value, copt));
}

/*****************************************************************************/

C_option_manip value(const C_option* copt)
{
return(C_option_manip(&value, *copt));
}

/*****************************************************************************/

C_option_manip envvar(const C_option* copt)
{
return(C_option_manip(&envvar, *copt));
}

/*****************************************************************************/

C_option::C_option(const char* name, const char* flag, const char* descr,
    const char* fname)
:pos(-1), envlist(0), envlistsize(0), multi_soll(1), multi_ist(0),
    isarg(0), isenv(0)
{
C_option::name=new char[strlen(name)+1];
strcpy(C_option::name, name);
C_option::flag=new char[strlen(flag)+1];
strcpy(C_option::flag, flag);
if (descr)
  {
  C_option::descr=new char[strlen(descr)+1];
  strcpy(C_option::descr, descr);
  }
else
  C_option::descr=0;
if (fname)
  {
  C_option::fname=new char[strlen(fname)+1];
  strcpy(C_option::fname, fname);
  }
else
  C_option::fname=0;
}

/*****************************************************************************/

C_option::C_option(const char* name, int pos, const char* descr,
    const char* fname)
:flag(0), pos(pos), envlist(0), envlistsize(0), multi_soll(1), multi_ist(0),
    isarg(0), isenv(0)
{
C_option::name=new char[strlen(name)+1];
strcpy(C_option::name, name);
C_option::pos=pos;
if (descr)
  {
  C_option::descr=new char[strlen(descr)+1];
  strcpy(C_option::descr, descr);
  }
else
  C_option::descr=0;
if (fname)
  {
  C_option::fname=new char[strlen(fname)+1];
  strcpy(C_option::fname, fname);
  }
else
  C_option::fname=0;
}

/*****************************************************************************/

C_option::~C_option()
{
delete[] name;
delete[] flag;
delete[] descr;
delete[] fname;
for (unsigned int i=0; i<envlistsize; i++) delete[] envlist[i];
delete[] envlist;
}

/*****************************************************************************/

int C_option::flaglen(void) const
{
if (flag!=0)
  return(strlen(flag));
else
  return(0);
}

/*****************************************************************************/

int C_option::fnamelen(void) const
{
if (fname!=0)
  return(strlen(fname));
else
  return(0);
}

/*****************************************************************************/

int C_option::isdefault(void) const
{
return(!(isarg||isenv));
}

/*****************************************************************************/

int C_option::isenviron(void) const
{
return(isenv);
}

/*****************************************************************************/

int C_option::isargument(void) const
{
return(isarg);
}

/*****************************************************************************/
void
C_option::add_env(const char* name)
{
    char** help=new char*[envlistsize+1];
    for (unsigned int i=0; i<envlistsize; i++)
        help[i]=envlist[i];
    delete[] envlist;
    envlist=help;
    envlist[envlistsize]=new char[strlen(name)+1];
    strcpy(envlist[envlistsize], name);
    envlistsize++;
}
/*****************************************************************************/
void
C_option::use_env(void)
{
    const char* envval;
    unsigned int i;

    i=0;
    while (!isenviron() && (i<envlistsize)) {
        if ((envval=getenv(envlist[i]))!=0)
            convert(envval, C_readargs::environment);
        i++;
    }
}
/*****************************************************************************/
void
C_option::printenvvar(ostream& os) const
{
    if (envlistsize==0)
        return;
    os << " (env: ";
    for (unsigned int i=0; i+1<envlistsize; i++)
        os << envlist[i] << ", ";
    os << envlist[envlistsize-1] << ')';
}
/*****************************************************************************/

void C_option::printflag(ostream& os) const
{
if (flag)
  os << '-' << flag;
else
  printflagname(os);
return;
}

/*****************************************************************************/

C_booloption::C_booloption(const char* name, const char* flag,
    bool def, const char* descr, const char* fname)
:C_option(name, flag, descr, fname), def(def)
{
vals=new bool[1];
}

/*****************************************************************************/

C_booloption::C_booloption(const char* name, int pos,
    bool def, const char* descr, const char* fname)
:C_option(name, pos, descr, fname), def(def)
{
vals=new bool[1];
}

/*****************************************************************************/

C_booloption::~C_booloption()
{
delete[] vals;
}

/*****************************************************************************/

int C_booloption::multiplicity(int mult)
{
int old_mult;

old_mult=multi_soll;
if (mult<1)
  multi_soll=-1;
else
  {
  delete[] vals;
  vals=new bool[mult];
  multi_soll=mult;
  }
return(old_mult);
}

/*****************************************************************************/

void C_booloption::printflagname(ostream& os) const
{
os << '-' << flag;
return;
}

/*****************************************************************************/

void C_booloption::printtypname(ostream& os) const
{
os << "(bool)  ";
return;
}

/*****************************************************************************/

void C_booloption::printdefval(ostream& os) const
{
os << (def?"True":"False");
return;
}

/*****************************************************************************/
void C_booloption::printvalue(ostream& os) const
{
    for (unsigned int i=0; i<multi_ist; i++) {
        os << (vals[i]?"True":"False");
        if ((i+1)<multi_ist)
            os << ", ";
    }
    return;
}
/*****************************************************************************/

C_intoption::C_intoption(const char* name, const char* flag,
    int def, const char* descr, const char* fname)
:C_option(name, flag, descr, fname), def(def)
{
vals=new int[1];
}

/*****************************************************************************/

C_intoption::C_intoption(const char* name, int pos,
    int def, const char* descr, const char* fname)
:C_option(name, pos, descr, fname), def(def)
{
vals=new int[1];
}

/*****************************************************************************/

C_intoption::~C_intoption()
{
delete[] vals;
}

/*****************************************************************************/
int C_intoption::multiplicity(int mult)
{
    int old_mult;

    old_mult=multi_soll;
    if (mult<1) {
        multi_soll=-1;
    } else {
        delete[] vals;
        vals=new int[mult];
        multi_soll=mult;
    }
    return old_mult;
}
/*****************************************************************************/

void C_intoption::printflagname(ostream& os) const
{
os << (fname?fname:"int");
return;
}

/*****************************************************************************/

void C_intoption::printtypname(ostream& os) const
{
os << "(int)   ";
return;
}

/*****************************************************************************/

void C_intoption::printdefval(ostream& os) const
{
os << def;
return;
}

/*****************************************************************************/
void
C_intoption::printvalue(ostream& os) const
{
    for (unsigned int i=0; i<multi_ist; i++) {
        os << vals[i];
        if ((i+1)<multi_ist)
            os << ", ";
    }
    return;
}
/*****************************************************************************/

C_u64option::C_u64option(const char* name, const char* flag,
    ems_u64 def, const char* descr, const char* fname)
:C_option(name, flag, descr, fname), def(def)
{
vals=new ems_u64[1];
}

/*****************************************************************************/

C_u64option::C_u64option(const char* name, int pos,
    ems_u64 def, const char* descr, const char* fname)
:C_option(name, pos, descr, fname), def(def)
{
vals=new ems_u64[1];
}

/*****************************************************************************/

C_u64option::~C_u64option()
{
delete[] vals;
}

/*****************************************************************************/

int C_u64option::multiplicity(int mult)
{
int old_mult;

old_mult=multi_soll;
if (mult<1)
  multi_soll=-1;
else
  {
  delete[] vals;
  vals=new ems_u64[mult];
  multi_soll=mult;
  }
return(old_mult);
}

/*****************************************************************************/

void C_u64option::printflagname(ostream& os) const
{
os << (fname?fname:"uint64");
return;
}

/*****************************************************************************/
void C_u64option::printtypname(ostream& os) const
{
    os << "(uint64)";
    return;
}
/*****************************************************************************/

void C_u64option::printdefval(ostream& os) const
{
os << def;
return;
}

/*****************************************************************************/
void
C_u64option::printvalue(ostream& os) const
{
    for (unsigned int i=0; i<multi_ist; i++) {
        os << vals[i];
        if ((i+1)<multi_ist)
            os << ", ";
    }
    return;
}
/*****************************************************************************/
C_realoption::C_realoption(const char* name, const char* flag,
    double def, const char* descr, const char* fname)
:C_option(name, flag, descr, fname), def(def)
{
    vals=new double[1];
}
/*****************************************************************************/
C_realoption::C_realoption(const char* name, int pos,
    double def, const char* descr, const char* fname)
:C_option(name, pos, descr, fname), def(def)
{
    vals=new double[1];
}
/*****************************************************************************/
C_realoption::~C_realoption()
{
    delete[] vals;
}
/*****************************************************************************/
int C_realoption::multiplicity(int mult)
{
    int old_mult;

    old_mult=multi_soll;
    if (mult<1) {
        multi_soll=-1;
    } else {
        delete[] vals;
        vals=new double[mult];
        multi_soll=mult;
    }
    return(old_mult);
}
/*****************************************************************************/
void C_realoption::printflagname(ostream& os) const
{
    os << (fname?fname:"real");
    return;
}
/*****************************************************************************/
void C_realoption::printtypname(ostream& os) const
{
    os << "(real)  ";
    return;
}
/*****************************************************************************/
void C_realoption::printdefval(ostream& os) const
{
    os << def;
    return;
}
/*****************************************************************************/
void
C_realoption::printvalue(ostream& os) const
{
    for (unsigned int i=0; i<multi_ist; i++) {
        os << vals[i];
        if ((i+1)<multi_ist)
            os << ", ";
    }
    return;
}
/*****************************************************************************/

C_charoption::C_charoption(const char* name, const char* flag,
    char def, const char* descr, const char* fname)
:C_option(name, flag, descr, fname), def(def)
{
vals=new char[1];
}

/*****************************************************************************/

C_charoption::C_charoption(const char* name, int pos,
    char def, const char* descr, const char* fname)
:C_option(name, pos, descr, fname), def(def)
{
vals=new char[1];
}

/*****************************************************************************/

C_charoption::~C_charoption()
{
delete[] vals;
}

/*****************************************************************************/

int C_charoption::multiplicity(int mult)
{
int old_mult;

old_mult=multi_soll;
if (mult<1)
  multi_soll=-1;
else
  {
  delete[] vals;
  vals=new char[mult];
  multi_soll=mult;
  }
return(old_mult);
}

/*****************************************************************************/

void C_charoption::printflagname(ostream& os) const
{
os << (fname?fname:"char");
return;
}

/*****************************************************************************/

void C_charoption::printtypname(ostream& os) const
{
os << "(char)  ";
return;
}

/*****************************************************************************/

void C_charoption::printdefval(ostream& os) const
{
os << '\'' << def << '\'';
return;
}

/*****************************************************************************/
void
C_charoption::printvalue(ostream& os) const
{
    for (unsigned int i=0; i<multi_ist; i++) {
        os << '\'' << vals[i] << '\'';
        if ((i+1)<multi_ist)
            os << ", ";
    }
    return;
}
/*****************************************************************************/

C_stringoption::C_stringoption(const char* name, const char* flag,
    const char* def, const char* descr, const char* fname)
:C_option(name, flag, descr, fname)
{
C_stringoption::def=new char[strlen(def)+1];
strcpy(C_stringoption::def, def);
vals= new char*[1];
}

/*****************************************************************************/

C_stringoption::C_stringoption(const char* name, int pos,
    const char* def, const char* descr, const char* fname)
:C_option(name, pos, descr, fname)
{
C_stringoption::def=new char[strlen(def)+1];
strcpy(C_stringoption::def, def);
vals= new char*[1];
}

/*****************************************************************************/
C_stringoption::~C_stringoption()
{
    delete[] def;
    for (unsigned int i=0; i<multi_ist; i++)
        delete[] vals[i];
    delete[] vals;
}
/*****************************************************************************/

int C_stringoption::multiplicity(int mult)
{
int old_mult;

old_mult=multi_soll;
if (mult<1)
  multi_soll=-1;
else
  {
  delete[] vals;
  vals=new char*[mult];
  multi_soll=mult;
  }
return(old_mult);
}

/*****************************************************************************/

void C_stringoption::printflagname(ostream& os) const
{
os << (fname?fname:"string");
return;
}

/*****************************************************************************/

void C_stringoption::printtypname(ostream& os) const
{
os << "(string)";
return;
}

/*****************************************************************************/

void C_stringoption::printdefval(ostream& os) const
{
os << '\"' << def << '\"';
return;
}

/*****************************************************************************/
void C_stringoption::printvalue(ostream& os) const
{
    for (unsigned int i=0; i<multi_ist; i++) {
        os << '\"' << vals[i] << '\"';
        if ((i+1)<multi_ist)
            os << ", ";
    }
    return;
}
/*****************************************************************************/
int
C_booloption::convert(const char* s, C_readargs::argtype argt)
{
    if (static_cast<int>(multi_ist)==multi_soll) {
        if (multi_soll==1) {
            multi_ist=0;
        } else {
            cerr << "too many occurencies of flag \"" << ::flag(this) << "\"." << endl;
            return -1;
        }
    }
    if ((multi_soll==-1) && (multi_ist>0)) {
        bool* help=new bool[multi_ist+1];
        for (unsigned int i=0; i<multi_ist; i++)
            help[i]=vals[i];
        delete[] vals;
        vals=help;
    }
// possible values: (NULL), true ,t, yes, y, 1, false, f, no, n, 0 
    if ((s==0)
            || (strcmp(s, "-")==0)
            || (strcasecmp(s, "true")==0)
            || (strcasecmp(s, "t")==0)
            || (strcasecmp(s, "yes")==0)
            || (strcasecmp(s, "y")==0)
            || (strcmp(s, "1")==0)) {
        vals[multi_ist++]=true;
    } else if ((strcmp(s, "+")==0)
            || (strcasecmp(s, "false")==0)
            || (strcasecmp(s, "f")==0)
            || (strcasecmp(s, "no")==0)
            || (strcasecmp(s, "n")==0)
            || (strcmp(s, "0")==0)) {
        vals[multi_ist++]=false;
    } else {
        cerr << "cannot convert \"" << s << "\" to boolean ("<<::flag(this)<<")." << endl;
        return(-1);
    }
    if (argt==C_readargs::argument)
        isarg=1;
    else if (argt==C_readargs::environment)
        isenv=1;
    return 0;
}
/*****************************************************************************/
int
C_intoption::convert(const char* s, C_readargs::argtype argt)
{
    istringstream ss(s);
    char dummy[1024];
    if (static_cast<int>(multi_ist)==multi_soll) {
        if (multi_soll==1) {
            multi_ist=0;
        } else {
            cerr<<"too many occurencies of flag \""<<::flag(this)<<"\"."<<endl;
            return -1;
        }
    }
    if ((multi_soll==-1) && (multi_ist>0)) {
        int* help=new int[multi_ist+1];
        for (unsigned int i=0; i<multi_ist; i++)
            help[i]=vals[i];
        delete[] vals;
        vals=help;
    }
    void* res=ss>>resetiosflags(ios::basefield)>>vals[multi_ist];
    if (!res||(ss>>dummy)) {
        cerr<<"cannot convert \""<<s<<"\" to integer ("<<::flag(this)<<")."<<endl;
        return -1;
    } else {
        multi_ist++;
        if (argt==C_readargs::argument)
            isarg=1;
        else if (argt==C_readargs::environment)
            isenv=1;
        return 0;
    }
}
/*****************************************************************************/
int C_u64option::convert(const char* s, C_readargs::argtype argt)
{
    istringstream ss(s);
    char dummy[1024];
    if (static_cast<int>(multi_ist)==multi_soll) {
        if (multi_soll==1) {
            multi_ist=0;
        } else {
            cerr << "too many occurencies of flag \"" << ::flag(this)
                    << "\"." << endl;
            return -1;
        }
    }
    if ((multi_soll==-1) && (multi_ist>0)) {
        ems_u64* help=new ems_u64[multi_ist+1];
        for (unsigned int i=0; i<multi_ist; i++)
            help[i]=vals[i];
        delete[] vals;
        vals=help;
    }
    if (!(ss >> resetiosflags(ios::basefield) >> vals[multi_ist]) ||
            (ss >> dummy)) {
        cerr << "cannot convert \"" << s << "\" to uint64 ("<<::flag(this)
                <<")." << endl;
        return(-1);
    } else {
        multi_ist++;
        if (argt==C_readargs::argument)
            isarg=1;
        else if (argt==C_readargs::environment)
            isenv=1;
        return(0);
    }
}
/*****************************************************************************/
int
C_realoption::convert(const char* s, C_readargs::argtype argt)
{
    istringstream ss(s);
    char dummy[1024];
    if (static_cast<int>(multi_ist)==multi_soll) {
        if (multi_soll==1) {
            multi_ist=0;
        } else {
            cerr << "too many occurencies of flag \"" << ::flag(this) << "\"." << endl;
            return -1;
        }
    }
    if ((multi_soll==-1) && (multi_ist>0)) {
        double* help=new double[multi_ist+1];
        for (unsigned int i=0; i<multi_ist; i++)
            help[i]=vals[i];
        delete[] vals;
        vals=help;
    }
    if (!(ss >> resetiosflags(ios::basefield) >> vals[multi_ist])
            || (ss >> dummy)) {
        cerr << "cannot convert \"" << s << "\" to double ("<<::flag(this)<<")." << endl;
        return(-1);
    } else {
        multi_ist++;
        if (argt==C_readargs::argument)
            isarg=1;
        else if (argt==C_readargs::environment)
            isenv=1;
        return(0);
    }
}
/*****************************************************************************/
int
C_charoption::convert(const char* s, C_readargs::argtype argt)
{
    int len;

    if (static_cast<int>(multi_ist)==multi_soll) {
        if (multi_soll==1) {
            multi_ist=0;
        } else {
            cerr << "too many occurencies of flag \"" << ::flag(this) << "\"." << endl;
            return(-1);
        }
    }
    len=strlen(s);
    if (len==0) {
        cerr << "C_charoption::convert: empty string ("<<::flag(this)<<")." << endl;
        return(-1);
    } else if (len>1) {
        cerr << "\"" << s << "\" is not a single character ("<<::flag(this)<<")." <<endl;
        return(-1);
    }
    if ((multi_soll==-1) && (multi_ist>0)) {
        char* help=new char[multi_ist+1];
        for (unsigned int i=0; i<multi_ist; i++)
            help[i]=vals[i];
        delete[] vals;
        vals=help;
    }
    if (argt==C_readargs::argument)
        isarg=1;
    else if (argt==C_readargs::environment)
        isenv=1;
    vals[multi_ist++]=s[0];
    return(0);
}
/*****************************************************************************/
int
C_stringoption::convert(const char* s, C_readargs::argtype argt)
{
    if (static_cast<int>(multi_ist)==multi_soll) {
        if (multi_soll==1) {
            multi_ist=0;
        } else {
            cerr << "too many occurencies of flag \"" << ::flag(this) << "\"." << endl;
            return(-1);
        }
    }
    if ((multi_soll==-1) && (multi_ist>0)) {
        char** help=new char*[multi_ist+1];
        for (unsigned int i=0; i<multi_ist; i++)
            help[i]=vals[i];
        delete[] vals;
        vals=help;
    }
    vals[multi_ist]=new char[strlen(s)+1];
    strcpy(vals[multi_ist], s);
    multi_ist++;
    if (argt==C_readargs::argument)
        isarg=1;
    else if (argt==C_readargs::environment)
        isenv=1;
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
