/*
 * readargs.hxx
 * 
 * created 26.10.94 PW
 * 
 * $ZEL: readargs.hxx,v 2.20 2010/06/20 22:45:17 wuestner Exp $
 */

#ifndef _readargs_hxx_
#define _readargs_hxx_

#include "config.h"
#include <iostream>

/*****************************************************************************/

class C_option;

class C_readargs
  {
  public:
    C_readargs(int argc, char* argv[]);
    virtual ~C_readargs();
    enum hint {demands, required, exclusive, implicit};
    enum argtype {argument, environment};
    void addoption(const char* name, const char* flag, bool def,
        const char* descr=0, const char* =0);
    void addoption(const char* name, const char* flag, int def,
        const char* descr=0, const char* =0);
    void addoption(const char* name, const char* flag, ems_u64 def,
        const char* descr=0, const char* =0);
    void addoption(const char* name, const char* flag, double def,
        const char* descr=0, const char* =0);
    void addoption(const char* name, const char* flag, char def,
        const char* descr=0, const char* =0);
    void addoption(const char* name, const char* flag, const char* def,
        const char* descr=0, const char* =0);
    void addoption(const char* name, int pos, bool def,
        const char* descr=0, const char* =0);
    void addoption(const char* name, int pos, int def,
        const char* descr=0, const char* =0);
    void addoption(const char* name, int pos, ems_u64 def,
        const char* descr=0, const char* =0);
    void addoption(const char* name, int pos, double def,
        const char* descr=0, const char* =0);
    void addoption(const char* name, int pos, char def,
        const char* descr=0, const char* =0);
    void addoption(const char* name, int pos, const char* def,
        const char* descr=0, const char* =0);
    void hints(hint hint_, ...);
    int  use_env(const char* optname, const char* envname);
    int  multiplicity(const char* name) const;
    int  multiplicity(const char* name, int);
    int  interpret(bool autohelp=true);
    typedef void(*insetfunc)(std::ostream&);
    void helpinset(int, insetfunc);
    void printhelp() const;
    bool getboolopt(const char* name, unsigned int=0) const;
    int  getintopt(const char* name, unsigned int=0) const;
    ems_u64 getu64opt(const char* name, unsigned int=0) const;
    double getrealopt(const char* name, unsigned int=0) const;
    char getcharopt(const char* name, unsigned int=0) const;
    const char* getstringopt(const char* name, unsigned int=0) const;
    int  isdefault(const char* name) const;
    int  isenviron(const char* name) const;
    int  isargument(const char* name) const;
    int  numnames() const;
    const char* getnames(int i) const;
    const char* progname() const {return argv[0];}
    const char* baseprogname() const {return baseprogname_;}
    void addname(const char* name);
  protected:
    int argc;
    char **argv;
    char* baseprogname_;
    C_option** optlist;
    int optlistsize;
    C_option** poptlist;
    int poptlistsize;
    C_option** requiredlist;
    int requiredlistsize;
    C_option*** exclusivelists;
    int* exclusivelistsizes;
    int numexclusivelists;
    C_option*** implicitlists;
    int* implicitlistsizes;
    int numimplicitlists;
    C_option*** demandlists;
    int* demandlistsizes;
    int numdemandlists;
    int parse_args();
    int check_environment();
    int check_implicit_options();
    int check_required_options();
    int check_exclusive_options();
    int check_demand_options();
    int addopt(const char* name, const char* flag);
    int addopt(const char* name, int pos);
    C_option* findentry(const char* name) const;
    int testflag(const char* flag) const;
    int testpos(int pos) const;
    C_option* findflag(const char* flag, int n) const;
    C_option* findpos(int pos) const;
    char **names;
    int nnames;
    int interpreted;
    int numhelpinsets;
    insetfunc* helpinsets;
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
