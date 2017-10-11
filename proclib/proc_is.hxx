/*
 * proc_is.hxx
 * 
 * created: 05.09.95
 * 
 * $ZEL: proc_is.hxx,v 2.19 2014/07/14 15:11:54 wuestner Exp $
 * 
 */

#ifndef _proc_is_hxx_
#define _proc_is_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include "proc_ved.hxx"
#include "proc_plist.hxx"
#include "proc_isstatus.hxx"
#include <ems_err.h>
#include "caplib.hxx"
#include <outbuf.hxx>

using namespace std;

/*****************************************************************************/

class C_instr_system: public nocopy
  {
  public:
    typedef enum {create, open, notest} openmode;
    C_instr_system(C_VED* ved, int idx, openmode, int id);
    C_instr_system(C_VED* ved, int idx, openmode);
    virtual ~C_instr_system();

  protected:
    string is_name;
    C_VED* ved;
    int ved_id;
    int idx;
    conf_mode& confmode_;
    int last_xid_;
    C_proclist proclist;
    C_proclist readoutlist;

  public:
    string name() const {return is_name;}
    exec_mode execution_mode(exec_mode mode);
    exec_mode execution_mode() const {return(proclist.execmode);}
    C_proclist* newproclist(){return new C_proclist(__FILE__":65", ved, this,
        ved->capab_proc_list);}
    C_confirmation* command(C_proclist*);
    C_confirmation* command(const char* proc, int argnum, ...);
    C_confirmation* command(int procnum, int argnum, ...);
    C_confirmation* command(const char*, const C_outbuf&);
    C_confirmation* command(const char* proc);
    C_confirmation* command(const char* proc, const char* arg);
    C_confirmation* command(int procnum);
    void add_param(int par) {proclist.add_par(par);}
    //void add_param(float par) {proclist.add_par(par);}
    //void add_param(double par) {proclist.add_par(par);}
    void add_param(int num, int par, ...);
    void add_param(int num, ems_u32* par) {proclist.add_par(num, par);}
    void add_param(const char* par) {proclist.add_par(par);}
    void clear_command() {proclist.clear();}
    C_confirmation* commandlist(int* list, int size);
    C_confirmation* execute();
    int procnum(const char* proc) const
        {return ved->procnum(proc, Capab_listproc);}
    int numprocs() const {return ved->numprocs(Capab_listproc);}
    string procname(int idx) const {return ved->procname(idx, Capab_listproc);}
    int is_idx() const {return(idx);}
    int is_id();
    C_isstatus* ISStatus();
    void DownloadISModullist(int modules, ems_u32* list);
    C_memberlist* UploadISModullist();
    void DeleteISModullist();
    void ro_clear(void) {readoutlist.clear();}
    void ro_add_param(int par) {readoutlist.add_par(par);}
    void ro_add_param(int num, int par, ...);
    //void ro_add_param(float par) {readoutlist.add_par(par);}
    //void ro_add_param(double par) {readoutlist.add_par(par);}
    void ro_add_param(const char* par) {readoutlist.add_par(par);}
    void ro_add_proc(const char* proc);
    void ro_add_proc(const char* proc, int arg);
    void ro_add_proc(const char* proc, int num, ...);
    void ro_add_proc(int proc);
    void DownloadReadoutlist(int priority, int num_trigg, ...);
    void DownloadReadoutlist(int priority, ems_u32 *trigg, int num_trigg);
    void DeleteReadoutlist(int trigg);
    C_confirmation* UploadReadoutlist(int trigg);
    char version_separator() const
        {return ved->version_separator(Capab_listproc);}
    void enable(int);
    int last_xid() const {return last_xid_;}
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
