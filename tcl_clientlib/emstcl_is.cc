/*
 * tcl_clientlib/emstcl_is.cc
 * 
 * created 06.09.95
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <cstdlib>
#include "emstcl_is.hxx"
#include <proc_is.hxx>
#include "findstring.hxx"
#include "tcl_cxx.hxx"
#include <versions.hxx>

VERSION("2009-Feb-25", __FILE__, __DATE__, __TIME__,
"$ZEL: emstcl_is.cc,v 1.15 2010/02/03 00:15:50 wuestner Exp $")
#define XVERSION


/*****************************************************************************/

E_is::E_is(Tcl_Interp* interp, E_ved* ved, int idx, openmode mode,
    int id)
:C_instr_system(ved, idx, mode, id), destructed(0), eved(ved), interp(interp),
    ccomm(""), dcomm(""), destroy(0)
{}

/*****************************************************************************/

E_is::E_is(Tcl_Interp* interp, E_ved* ved, int idx, openmode mode)
:C_instr_system(ved, idx, mode), destructed(0), eved(ved), interp(interp),
    ccomm(""), dcomm(""), destroy(0)
{}

/*****************************************************************************/

E_is::~E_is()
{
destructed=1;
if (tclcommand)
    Tcl_DeleteCommand(interp, Tcl_GetCommandName(interp, tclcommand));
}

/*****************************************************************************/
void E_is::create_tcl_proc()
{
    STRING pn=eved->origtclprocname();
    STRING pn1=pn.substr(3, pn.length()-3);
    OSTRINGSTREAM ss;
    ss << "is" << pn1 << '_' << idx;
    STRING rootname=ss.str();
    STRING newname=rootname;
    int i=0;
    Tcl_CmdInfo info;
    while (Tcl_GetCommandInfo(interp, (char*)newname.c_str(), &info)!=0) {
        i++;
        OSTRINGSTREAM ss;
        ss << rootname << '#' << idx;
        newname=ss.str();
    }
    origprocname=newname;
    tclcommand=Tcl_CreateCommand(interp, (char*)newname.c_str(), E_is_Ems_IsCommand,
    ClientData(this), E_is_Ems_IsDelete);
    //Tcl_SetResult(interp, newname, TCL_VOLATILE);
}
/*****************************************************************************/

const char* E_is::tclprocname() const
{
return Tcl_GetCommandName(interp, tclcommand);
}

/*****************************************************************************/

int E_is_Ems_IsCommand(ClientData clientdata, Tcl_Interp* interp, int argc,
    const char* argv[])
{
int res=((E_is*)clientdata)->IsCommand(argc, argv);
return res;
}

/*****************************************************************************/

void E_is_Ems_IsDelete(ClientData clientdata)
{
//cout<<"E_is::Ems_IsDelete; destructed="<<((E_is*)clientdata)->destructed<<endl;
((E_is*)clientdata)->delcommand();
((E_is*)clientdata)->tclcommand=0;
if (!((E_is*)clientdata)->destructed)
  {
  //cerr << "  call delete " << (void*)clientdata << endl;
  delete (E_is*)clientdata;
  }
}

/*****************************************************************************/

void E_is::deletecommand(STRING& comm)
{
//cout<<"E_is::deletecommand("<<comm<<")"<<endl;
if (comm.length()!=0)
  {
  string::size_type x;
#ifdef NO_ANSI_CXX
  while ((x=comm.index(STRING("%n")))>=0)
      comm=comm(0, x)+eved->name()+comm(x+2, comm.length()-x-2);
#else
  while ((x=comm.find(STRING("%n")))!=string::npos)
      comm=comm.substr(0, x)+eved->name()+comm.substr(x+2, string::npos);
#endif
#ifdef NO_ANSI_CXX
  while ((x=comm.index(STRING("%v")))>=0)
      comm=comm(0, x)+eved->tclprocname()+comm(x+2, comm.length()-x-2);
#else
  while ((x=comm.find(STRING("%v")))!=string::npos)
      comm=comm.substr(0, x)+eved->tclprocname()+comm.substr(x+2, string::npos);
#endif
#ifdef NO_ANSI_CXX
  while ((x=comm.index(STRING("%i")))>=0)
#else
  while ((x=comm.find(STRING("%i")))!=string::npos)
#endif
    {
    OSTRINGSTREAM ss;
#ifdef NO_ANSI_CXX
    ss << comm(0, x) << idx << comm(x+2, comm.length()-x-2) << ends;
#else
    ss << comm.substr(0, x) << idx << comm.substr(x+2, string::npos);
#endif
    comm=ss.str();
    }
#ifdef NO_ANSI_CXX
  while ((x=comm.index(STRING("%c")))>=0)
      comm=comm(0, x)+tclprocname()+comm(x+2, comm.length()-x-2);
#else
  while ((x=comm.find(STRING("%c")))!=string::npos)
      comm=comm.substr(0, x)+tclprocname()+comm.substr(x+2, string::npos);
#endif
  if (Tcl_GlobalEval(interp, (char*)comm.c_str())!=TCL_OK)
    {
    cerr << interp->result << endl;
    }
  }
}

/*****************************************************************************/

void E_is::delcommand()
{
//cout<<"E_is::delcommand"<<endl;
deletecommand(ccomm);
if (destroy) deletecommand(dcomm);
}

/*****************************************************************************/
int E_is::IsCommand(int argc, const char* argv[])
{
// <IS> close|delete|... ...
//
    int res;
#if 0
    fprintf(stderr, "E_is:");
    for (int i=0; i<argc; i++) {
        fprintf(stderr, " %s", argv[i]);
    }
    fprintf(stderr, "\n");
#endif
    if (argc<2) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("argument required", -1));
        res=TCL_ERROR;
    } else {
        static const char* names[]={
            "close",
            "delete",
            "closecommand",
            "deletecommand",
            "id",
            "idx",
            "caplist",
            "command",
            "command1",
            "memberlist",
            "readoutlist",
            "status",
            "enable",
            "ved",
            0
        };
        switch (findstring(interp, argv[1], names)) {
            case 0: res=e_close(argc, argv); break;
            case 1: res=e_delete(argc, argv); break;
            case 2: res=e_closecommand(argc, argv); break;
            case 3: res=e_deletecommand(argc, argv); break;
            case 4: res=e_id(argc, argv); break;
            case 5: res=e_idx(argc, argv); break;
            case 6: res=e_caplist(argc, argv); break;
            case 7: res=e_command(argc, argv); break;
            case 8: res=e_command1(argc, argv); break;
            case 9: res=e_memberlist(argc, argv); break;
            case 10: res=e_readoutlist(argc, argv); break;
            case 11: res=e_status(argc, argv); break;
            case 12: res=e_enable(argc, argv); break;
            case 13: res=e_ved(argc, argv); break;
            default: res=TCL_ERROR; break;
        }
    }
    return res;
}
/*****************************************************************************/
/*****************************************************************************/
