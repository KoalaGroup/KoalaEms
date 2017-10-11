/*
 * tcl_clientlib/emstcl_is.hxx
 * 
 * $ZEL: emstcl_is.hxx,v 1.11 2014/07/14 15:13:25 wuestner Exp $
 * 
 * created 06.09.95
 */

#ifndef _emstcl_is_cc_
#define _emstcl_is_cc_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <tcl.h>
#include <proc_is.hxx>
#include "emstcl_ved.hxx"

using namespace std;

/*****************************************************************************/

class E_is: public C_instr_system {
  public:
    E_is(Tcl_Interp*, E_ved*, int, openmode, int);
    E_is(Tcl_Interp*, E_ved*, int, openmode);
    ~E_is(); // virtuell, wenn weitere IS abgeleitet werden
    const char* tclprocname() const;
    const char* origtclprocname() const {return origprocname.c_str();}
    void create_tcl_proc();
    int IsCommand(int, const char*[]); // virtuell, wenn weitere IS abgeleitet werden
    typedef int (E_is::*isproc)(int, const char*[]);
    void delcommand();
    Tcl_Command tclcommand;
    int destructed;
  protected:
    E_ved* eved;
    Tcl_Interp* interp;
    string origprocname;

    //static int Ems_IsCommand(ClientData, Tcl_Interp*, int, const char*[]);
    //static void Ems_IsDelete(ClientData);
    string ccomm;
    string dcomm;
    int destroy;
    void deletecommand(string&);
    int e_close(int, const char*[]);
    int e_closecommand(int, const char*[]);
    int e_delete(int, const char*[]);
    int e_deletecommand(int, const char*[]);
    int e_id(int, const char*[]);
    int e_idx(int, const char*[]);
    int e_ved(int, const char*[]);
    int e_caplist(int, const char*[]);
    int e_command(int, const char*[]);
    int e_command1(int, const char*[]);
    int e_memberlist(int, const char*[]);
    int e_memlist_upload(int, const char*[]);
    int e_memlist_create(int, const char*[]);
    int e_memlist_delete(int, const char*[]);
    int e_status(int, const char*[]);
    int e_enable(int, const char*[]);
    int e_readoutlist(int, const char*[]);
    int e_rolist_upload(int, const char*[]);
    int e_rolist_create(int, const char*[]);
    int e_rolist_delete(int, const char*[]);
  private:
    int e_rolist_setup(int, const char*[]);
};
extern "C" {
    int E_is_Ems_IsCommand(ClientData, Tcl_Interp*, int, const char*[]);
    void E_is_Ems_IsDelete(ClientData);
}
#endif

/*****************************************************************************/
/*****************************************************************************/
