/*
 * emstcl_ved.cc
 * 
 * created 06.09.95
 * 13.01.1999 PW: set_error_code added
 * 16.07.1999 PW: for changed findstring adapted
 * 07.Jan.2001 PW: formers.add in E_ved::E_ved fixed
 */

#include "config.h"
#include "cxxcompat.hxx"
#include "emstcl_ved.hxx"
#include "emstcl_is.hxx"
#include <cstdlib>
#include <cerrno>
#include <errors.hxx>
#include <ems_errors.hxx>
#include <ved_errors.hxx>
#include "findstring.hxx"
#include "tcl_cxx.hxx"
#include <versions.hxx>

VERSION("Nov 16 2004", __FILE__, __DATE__, __TIME__,
"$ZEL: emstcl_ved.cc,v 1.25 2010/06/20 23:30:06 wuestner Exp $")
#define XVERSION

E_veds eveds;

/*****************************************************************************/
E_ved::formerent::formerent(void)
:name(0), form(0)
{}
/*****************************************************************************/
E_ved::formerent::formerent(const char* c, former f)
:name(c), form(f)
{}
/*****************************************************************************/
E_ved::formerents::formerents(void)
:formers(0), num_(0), size(0)
{}
/*****************************************************************************/
E_ved::formerents::~formerents(void)
{
delete[] formers;
}
/*****************************************************************************/
void E_ved::formerents::add(E_ved::formerent ent)
{
if (num_==size)
  {
  size+=20;
  formerent* help=new formerent[size];
  for (int i=0; i<num_; i++) help[i]=formers[i];
  delete[] formers;
  formers=help;
  }
formers[num_++]=ent;
}
/*****************************************************************************/
int E_ved::formerents::lookup(former f) const
{
int i=0;
while (i<num_ && formers[i].form!=f) i++;
return i==num_?-1:i;
}
/*****************************************************************************/
int E_ved::formerents::lookup(const char* c) const
{
int i=0;
while (i<num_ && formers[i].name!=c) i++;
return i==num_?-1:i;
}
/*****************************************************************************/

E_ved::formerents E_ved::formers;

E_ved::E_ved(Tcl_Interp* interp, const char* name, C_VED::VED_prior prior)
:C_VED(name, prior), interp(interp), tclcommand(0), destructed(0),
confbacks(0), numconfbacks(0), maxconfbacks(0), ccomm("")
{
//cerr << "E_ved::E_ved(); this=" << (void*)this << endl;
if (!formers.num())
  {
  formers.add(formerent("dummy",        &E_ved::f_dummy));
  formers.add(formerent("command",      &E_ved::f_command));     // Prozedurlisten
  formers.add(formerent("void",         &E_ved::f_void));        // nur Fehlerbehandlung
  formers.add(formerent("lamstatus",    &E_ved::f_lamstatus));   // LAM-Status und -Attribute
  formers.add(formerent("lamupload",    &E_ved::f_lamupload));   // Domain LAM
  formers.add(formerent("createis",     &E_ved::f_createis));    //
  formers.add(formerent("identify",     &E_ved::f_identify));    // ident ved
  formers.add(formerent("integer",      &E_ved::f_integer));     // one single integer
  formers.add(formerent("integerlist",  &E_ved::f_integerlist)); // list of integers; erstes Wort ist Anzahl
  formers.add(formerent("rawintegerlist", &E_ved::f_rawintegerlist)); // list of integers
  formers.add(formerent("intfmt",       &E_ved::f_intfmt));      // integers, arbitrary format
  formers.add(formerent("dostatus",     &E_ved::f_dostatus));    // dataoutstatus
  formers.add(formerent("ioaddr",       &E_ved::f_ioaddr));      // upload dataout or datain
  formers.add(formerent("trigger",      &E_ved::f_trigger));     // upload trigger
  formers.add(formerent("readvar",      &E_ved::f_readvar));     // read variable
  formers.add(formerent("readoutlist",  &E_ved::f_readoutlist)); // upload readoutlist
  formers.add(formerent("domevent",     &E_ved::f_domevent));    // upload event
  //formers.add(formerent("printconf",    &E_ved::f_printconf));   // print confirmation (formatiert))
  formers.add(formerent("printconf_raw", &E_ved::f_printconf_raw));// print confirmation (raw))
  formers.add(formerent("printconf_text", &E_ved::f_printconf_text));// print confirmation (raw))
  formers.add(formerent("printconf_tabular", &E_ved::f_printconf_tabular));// print confirmation (raw))
  formers.add(formerent("stringlist",   &E_ved::f_stringlist));  // list of strings; erstes Wort ist Anzahl
  formers.add(formerent("strings",      &E_ved::f_stringlist));  // list of strings; erstes Wort ist Anzahl
  formers.add(formerent("string",       &E_ved::f_string));      // string
  formers.add(formerent("confirmation", &E_ved::f_confirmation));// save Confirmation als Objekt
  /* wozu soll das gut sein???
  formers.add(formerent(0, 0));
  */
  }
eveds.add(this);
}

/*****************************************************************************/

E_ved::~E_ved()
{
//cerr << "E_ved::~E_ved(); this=" << (void*)this << endl;
destructed=1;
if (tclcommand)
  {
  //cerr << "  call Tcl_DeleteCommand("
  //    << Tcl_GetCommandName(interp, tclcommand) << ")" << endl;
  Tcl_DeleteCommand(interp, Tcl_GetCommandName(interp, tclcommand));
  }
delete[] confbacks;
eveds.remove(this);
}

/*****************************************************************************/
void E_ved::create_tcl_proc()
{
    STRING rootname=STRING("ved_")+name();
    STRING newname=rootname;
    int idx=0;
    Tcl_CmdInfo info;

    while (Tcl_GetCommandInfo(interp, (char*)newname.c_str(), &info)!=0) {
        idx++;
        OSTRINGSTREAM ss;
        ss << rootname << '#' << idx;
        newname=ss.str();
    }
    origprocname=newname;

    tclcommand=Tcl_CreateCommand(interp, (char*)newname.c_str(),
    E_ved_Ems_VedCommand, ClientData(this), (Tcl_CmdDeleteProc*)Ems_VedDelete);
    Tcl_SetResult(interp, (char*)newname.c_str(), TCL_VOLATILE);
}
/*****************************************************************************/
int E_ved_Ems_VedCommand(ClientData clientdata, Tcl_Interp* interp, int argc,
    const char* argv[])
{
int res=((E_ved*)clientdata)->VedCommand(argc, argv);
return res;
}
/*****************************************************************************/

void E_ved::Ems_VedDelete(ClientData clientdata)
{
//cerr << "E_ved::Ems_VedDelete(" << (void*)clientdata << ")" << endl;
((E_ved*)clientdata)->deletecommand();
((E_ved*)clientdata)->tclcommand=0;
if (!((E_ved*)clientdata)->destructed)
  {
  //cerr << "  call delete " << (void*)clientdata << endl;
  delete (E_ved*)clientdata;
  }
}

/*****************************************************************************/

void E_ved::deletecommand()
{
if (ccomm.length()!=0)
  {
  string::size_type x;
#ifdef NO_ANSI_CXX
  while ((x=ccomm.index(STRING("%n")))>=0)
      ccomm=ccomm(0, x)+name()+ccomm(x+2, ccomm.length()-x-2);
#else
  while ((x=ccomm.find("%n"))!=string::npos)
      ccomm=ccomm.substr(0, x)+name()+ccomm.substr(x+2, string::npos);
#endif
#ifdef NO_ANSI_CXX
  while ((x=ccomm.index(STRING("%c")))>=0)
      ccomm=ccomm(0, x)+tclprocname()+ccomm(x+2, ccomm.length()-x-2);
#else
  while ((x=ccomm.find("%c"))!=string::npos)
      ccomm=ccomm.substr(0, x)+tclprocname()+ccomm.substr(x+2, string::npos);
#endif
  if (Tcl_GlobalEval(interp, (char*)ccomm.c_str())!=TCL_OK)
      Tcl_BackgroundError(interp);
  }
}

/*****************************************************************************/

STRING E_ved::tclprocname() const
{
STRING s;
s=Tcl_GetCommandName(interp, tclcommand);
return s;
}

/*****************************************************************************/

const char* E_ved::commnames[]={
  "caplist",
  "close",
  "closecommand",
  "command",
  "command1",
  "confmode",
  "datain",
  "dataout",
  "event",
  "flush",
  "identify",
  "initiate",
  "is",
  "lam",
  "lamproclist",
  "modullist",
  "name",
  "namelist",
  "openis",
  "pending",
  "readout",
  "reset",
  "status",
  "trigger",
  "typedunsol",
  "unsol",
  "var",
  "version_separator",
  0};

/*****************************************************************************/

int E_ved::exec_comm(int id, int argc, const char* argv[])
{
int res;
switch (id)
  {
  case  0: res=e_caplist(argc, argv); break;
  case  1: res=e_close(argc, argv); break;
  case  2: res=e_closecommand(argc, argv); break;
  case  3: res=e_command(argc, argv); break;
  case  4: res=e_command1(argc, argv); break;
  case  5: res=e_confmode(argc, argv); break;
  case  6: res=e_datain(argc, argv); break;
  case  7: res=e_dataout(argc, argv); break;
  case  8: res=e_event(argc, argv); break;
  case  9: res=e_flush(argc, argv); break;
  case 10: res=e_identify(argc, argv); break;
  case 11: res=e_initiate(argc, argv); break;
  case 12: res=e_is(argc, argv); break;
  case 13: res=e_lam(argc, argv); break;
  case 14: res=e_lamproclist(argc, argv); break;
  case 15: res=e_modullist(argc, argv); break;
  case 16: res=e_name(argc, argv); break;
  case 17: res=e_namelist(argc, argv); break;
  case 18: res=e_openis(argc, argv); break;
  case 19: res=e_pending(argc, argv); break;
  case 20: res=e_readout(argc, argv); break;
  case 21: res=e_reset(argc, argv); break;
  case 22: res=e_status(argc, argv); break;
  case 23: res=e_trigger(argc, argv); break;
  case 24: res=e_typedunsol(argc, argv); break;
  case 25: res=e_unsol(argc, argv); break;
  case 26: res=e_var(argc, argv); break;
  case 27: res=e_version_separator(argc, argv); break;
  default:
    cerr << "Program error in E_ved::exec_comm: id=" << id << endl;
    res=TCL_ERROR;
    break;
  }
return res;
}
/*****************************************************************************/
int E_ved::VedCommand(int argc, const char* argv[])
{
    int res;
#if 0
    fprintf(stderr, "E_ved:");
    for (int i=0; i<argc; i++) {
        fprintf(stderr, " %s", argv[i]);
    }
    fprintf(stderr, "\n");
#endif
    if (argc<2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("argument required", -1));
        res=TCL_ERROR;
    } else {
        int idx;
        idx=findstring(interp, argv[1], commnames);
        if (idx==-1)
            res=TCL_ERROR;
        else
            res=exec_comm(idx, argc, argv);
    }
    return res;
}
/*****************************************************************************/
const char* E_ved::formername(former f) const
{
int i=formers.lookup(f);
return i<0?0:formers.str(i);
}
/*****************************************************************************/

void E_ved::installconfback(confbackentry* entry)
{
if (numconfbacks==maxconfbacks)
  {
  confbackentry** help=new confbackentry*[numconfbacks+10];
  for (int i=0; i<numconfbacks; i++) help[i]=confbacks[i];
  delete[] confbacks;
  confbacks=help;
  maxconfbacks=numconfbacks+10;
  }
confbacks[numconfbacks++]=entry;
}

/*****************************************************************************/

E_ved::confbackentry* E_ved::extractconfback(int xid)
{
int i=0;
while ((i<numconfbacks) && (confbacks[i]->xid!=xid)) i++;
if (i==numconfbacks) return 0;
confbackentry* entry=confbacks[i];
for (int j=i; j<numconfbacks-1; j++) confbacks[j]=confbacks[j+1];
numconfbacks--;
return entry;
}

/*****************************************************************************/

E_ved::confbackentry::~confbackentry()
{
delete[] formargs;
if (ok_command)
  {
  Tcl_DStringFree(ok_command);
  delete ok_command;
  }
if (err_command)
  {
  Tcl_DStringFree(err_command);
  delete err_command;
  }
}

/*****************************************************************************/

void E_ved::confbackentry::set_formargs(int idx, const STRING& arg)
{
if (numformargs<=idx)
  {
  int i;
  STRING* help=new STRING[idx+1];
  for (i=0; i<numformargs; i++) help[i]=formargs[i];
  for (i=numformargs; i<idx; i++) help[i]="";
  delete[] formargs;
  formargs=help;
  numformargs=idx+1;
  }
formargs[idx]=arg;
}

/*****************************************************************************/

static STRING* newstring(int n)
{
STRING* xxx;
xxx=new STRING[n];
return xxx;
}

/*****************************************************************************/
//
// E_ved::confbackbox::init wird von allen Ems-Tcl-Prozeduren aufgerufen,
// die eine Confirmation erwarten, diese aber nicht sofort erhalten koennen,
// da confmode auf asynchron gesetzt ist. confbackbox enthaelt Informationen,
// wie ein geeigneter Callback aufgerufen werden soll.
//   Die Syntax fuer jedes der betroffen Tcl-Kommandos ist folgende:
//
// command [argumente ...] : [former [argument]|] ok_callback ? err_callback
//
// ok_callback und err_callback sind beliebige Tcl-Prozeduren, former ist der
// Name einer der vordefinierten Formatierungsfunktionen.
//   Wenn die empfangene Confirmation keine Fehlermeldung enthaelt, wird former
// aufgerufen, sein Output wird als Argumentliste an ok_callback weitergegeben.
// Wenn former nicht angegeben ist, wird der Former verwendet, der exakt den
// Output erzeugt, den die urspruengliche Ems-Tcl-Prozedur im synchronen Modus
// erzeugt haette.
//   Wenn die empfangene Confirmation auf einen Fehler hinweist, wird sie in
// ein Tcl-Confirmation-Objekt konvertiert und an err_callback uebergeben.
//
// Wenn die Ems-Tcl-Prozedur wie im synchronen Modus aufgerufen wird, also
// former, ok_callback und err_callback nicht angegeben sind, wird im
// Erfolgsfall der Default-Former und die Tcl-Prozedur Ems_async_ok, im
// Fehlerfall die Tcl-Prozedur Ems_async_error aufgerufen. Ems_async_ok und
// Ems_async_error muessen vorher von der Applikation definiert worden sein.
//

int E_ved::confbackbox::init(Tcl_Interp* interp, const E_ved* ved, int& argc,
    const char* argv[], former def_form)
{
if (entry!=0) cerr << "error in E_ved::confbackbox::init: entry!=0" << endl;
entry=new confbackentry;

// Indizes der Separatoren (':','|','?')
int colon_idx=0, pipe_idx=0, question_idx=0; 
entry->numformargs=0;

int i=1; // argv[0] ist Prozedurname, kann also kein Trennzeichen sein
while ((i<argc) && ((argv[i][0]!=':') || (argv[i][1]!=0))) i++;
if (i<argc)
  {
  colon_idx=i;
  i++; // Doppelpunkt ueberspringen
  while ((i<argc) && ((argv[i][0]!='|') || (argv[i][1]!=0))) i++;
  if (i<argc)
    {
    pipe_idx=i;
    i++; // Pipesymbol ueberspringen
    }
  else
    i=colon_idx+1; // da war kein Pipesymbol
  while ((i<argc) && ((argv[i][0]!='?') || (argv[i][1]!=0))) i++;
  if (i<argc) 
    {
    question_idx=i;
    }
  }

if (colon_idx==0) // kein Callback definiert
  {
  entry->form=def_form;
  entry->former_default=1;
  entry->former_implicit=1;
  //entry->ok_command="";
  //entry->err_command="";
  //print();
  return TCL_OK;
  }

if (question_idx==0) goto error; // schlechter Stil, aber effizient
int ok_idx;
if (pipe_idx!=0) // former angegeben;
  {
  if ((pipe_idx-colon_idx)==1) goto error; // nur Separatoren, nichts dazwischen
  int formidx=findstring(interp, argv[colon_idx+1], formers);
  if (formidx>=0)
    entry->form=formers[formidx].form;
  else
    {
    Tcl_AppendResult(interp, "\ndefault former is " ,
        ved->formername(def_form), (char*)0);
    return TCL_ERROR;
    }

  entry->former_default=def_form==entry->form;
  entry->former_implicit=0;
  entry->numformargs=pipe_idx-colon_idx+2;
//  entry->formargs=new STRING[entry->numformargs];
  entry->formargs=newstring(entry->numformargs);
  for (i=0; i<entry->numformargs; i++) entry->formargs[i]=argv[colon_idx+2+i];
  ok_idx=pipe_idx+1;
  }
else
  {
  entry->form=def_form;
  entry->former_default=1;
  entry->former_implicit=1;
  ok_idx=colon_idx+1;
  }

entry->ok_command=new Tcl_DString;
Tcl_DStringInit(entry->ok_command);
for (i=ok_idx; i<question_idx; i++)
    Tcl_DStringAppendElement(entry->ok_command, argv[i]);

entry->err_command=new Tcl_DString;
Tcl_DStringInit(entry->err_command);
for (i=question_idx+1; i<argc; i++)
    Tcl_DStringAppendElement(entry->err_command, argv[i]);

argc=colon_idx;

//print();
return TCL_OK;

error:
Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong args for callback; must be "
    "\"':' [former [arguments ...]'|'] ok_callback '?' err_callback\"", -1));
return TCL_ERROR;
}

/*****************************************************************************/
//
// E_ved::confbackbox::finit wird von allen Ems-Tcl-Prozeduren aufgerufen,
// die dem Anwender die Moeglichkeit geben wollen, eine Confirmation nicht
// auf herkoemmliche Art zu formatieren, sondern eine Formatierungsprozedur
// explizit anzugeben.
//   Die Syntax der betroffen Tcl-Kommandos ist dann folgende:
//
// command [argumente ...] [| former [argument ...]]
//

int E_ved::confbackbox::finit(Tcl_Interp* interp, const E_ved* ved, int& argc,
    const char* argv[], former def_form)
{
entry=new confbackentry;
int pipe;

pipe=argc-1;
while ((pipe>0) && ((argv[pipe][0]!='|') || (argv[pipe][1]!=0))) pipe--;
if (pipe==0)
  {
  entry->form=def_form;
  entry->former_default=1;
  entry->former_implicit=1;
  }
else
  {
  if (pipe==argc-1) // '|' gefunden, aber kein Former angegeben
        goto error; // schlechter Stil, aber effizient
  int formidx=findstring(interp, argv[pipe+1], formers);
  if (formidx>=0)
    entry->form=formers[formidx].form;
  else
    {
    Tcl_AppendResult(interp, "\ndefault former is " ,
        ved->formername(def_form), (char*)0);
    return TCL_ERROR;
    }
  entry->former_default=def_form==entry->form;
  entry->former_implicit=0;
  entry->numformargs=argc-pipe-2;

  //entry->formargs=new STRING[entry->numformargs];
  // Bug in g++, deshalb etwas umstaendlich
  entry->formargs=newstring(entry->numformargs);
  for (int i=0; i<entry->numformargs; i++)
     entry->formargs[i]=argv[pipe+2+i];
  argc=pipe;
  }
return TCL_OK;

error:
Tcl_SetObjResult(interp,
    Tcl_NewStringObj("wrong args for former; must be \"'|' "
    "[former [arguments ...]\"", -1));
return TCL_ERROR;
}

/*****************************************************************************/

int E_ved::confbackbox::callformer(E_ved* ved, OSTRINGSTREAM& ss,
    const C_confirmation* conf)
{
int err=(ved->*(entry->form))(ss, conf, !entry->former_default,
              entry->numformargs, entry->formargs);
return err;
}

/*****************************************************************************/
#if 0
void E_ved::confbackentry::print() const
{
cerr << "confbackentry " << (void*)this << ":" << endl;
cerr << "  isimplicit  = " <<  former_implicit << endl;
cerr << "  isdefault   = " <<  former_default << endl;
cerr << "  former      = " << *(void**)&form << endl;
cerr << "  formargs    = " << formargs << endl;
cerr << "  ok_command  = " << ok_command << endl;
cerr << "  err_command = " << err_command << endl;
}
#endif
/*****************************************************************************/

E_ved::confbackentry* E_ved::confbackbox::get()
{
confbackentry* help=entry;
entry=0;
return help;
}

/*****************************************************************************/

E_veds::E_veds(void)
:listsize(10), list_idx(0)
{
list=new E_ved*[listsize];
if (list==0) throw new C_unix_error(errno, "E_veds::E_veds");
}

/*****************************************************************************/

E_veds::~E_veds(void)
{
delete[] list;
}

/*****************************************************************************/

void E_veds::add(E_ved* ved)
{
//cerr << "E_veds::add(" << (void*)ved << ")" << endl;
if (list_idx==listsize)
  {
  E_ved** help;
  listsize+=10;
  help=new E_ved*[listsize];
  if (help==0) throw new C_unix_error(errno, "E_veds::add");
  for (int i=0; i<list_idx; i++) help[i]=list[i];
  delete[] list;
  list=help;
  }
list[list_idx++]=ved;
}

/*****************************************************************************/

void E_veds::remove(const E_ved* ved)
{
//cerr << "E_veds::remove(" << (void*)ved << ")" << endl;
int idx;

for (idx=0; (idx<list_idx) && (list[idx]!=ved); idx++);
if (list[idx]==ved)
  {
  for (int i=idx; i<list_idx-1; i++) list[i]=list[i+1];
  list_idx--;
  }
else
  {
  throw new C_program_error("E_veds::remove: ved existiert nicht.");
  }
}

/*****************************************************************************/

E_ved* E_veds::find(int ved) const
{
//cerr << "E_veds::find(" << (void*)ved << ")" << endl;
for (int i=0; i<list_idx; i++)
  {
  if (list[i]->ved_id()==ved) return list[i];
  }
return 0;
}

/*****************************************************************************/

int E_veds::find(const E_ved* ved) const
{
//cerr << "E_veds::find(" << (void*)ved << ")" << endl;
for (int i=0; i<list_idx; i++)
  {
  if (list[i]==ved) return i;
  }
return -1;
}

/*****************************************************************************/
/*****************************************************************************/
