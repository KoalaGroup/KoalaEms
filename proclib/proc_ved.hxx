/*
 * proclib/proc_ved.hxx
 * 
 * created: 15.08.94 PW
 *
 * $ZEL: proc_ved.hxx,v 2.29 2016/05/02 15:28:43 wuestner Exp $
 */

#ifndef _proc_ved_hxx_
#define _proc_ved_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <sys/time.h>
#include "smartptr_t.hxx"
#include "proc_modes.hxx"
#include "proc_conf.hxx"
#include "proc_modullist.hxx"
#include "proc_memberlist.hxx"
#include "proc_isstatus.hxx"
#include "proc_iotype.hxx"
#include "proc_namelist.hxx"
#include "proc_ioaddr.hxx"
#include "proc_dataio.hxx"
#include "proc_readoutstatus.hxx"
#include "proc_plist.hxx"
#include <ems_err.h>
#include <nocopy.hxx>
#include <objecttypes.h>
#include <outbuf.hxx>
#include <inbuf.hxx>

class C_instr_system;

using namespace std;

/*****************************************************************************/

class C_add_trans;
class C_capability_list;
class C_proclist;

class C_VED : public nocopy {
    friend class C_proclist;
    friend class C_instr_system;
  public:
    typedef int VED_prior;
    static const VED_prior DEF_prior, NO_prior, CC_prior, CCM_prior, SER_prior,
         EB_prior;
    C_VED(const string& name, VED_prior prior=NO_prior);
    virtual ~C_VED();

  private:
    string ved_name;
    ems_u32 ved;
    T_smartptr<C_namelist> namelist;
    T_smartptr<C_namelist> namelist_dom;
    T_smartptr<C_namelist> namelist_pi;
//     P_namelist namelist;
//     P_namelist namelist_obj;
//     P_namelist namelist_pi;
//    C_namelist* namelist;
//    C_namelist* namelist_obj;
//    C_namelist* namelist_pi;
    T_smartptr<C_capability_list> capab_proc_list;
    T_smartptr<C_capability_list> capab_trig_list;
    T_smartptr<C_proclist> lamlist;
    T_smartptr<C_proclist> triglist;
    C_instr_system** islist;
    int num_is;
    void add_is(C_instr_system*);
    void delete_is(C_instr_system*);
    int loadcaplist(Capabtyp typ, C_capability_list** capab_list);

  protected:
    conf_mode confmode_;
    int last_xid_;
    C_instr_system* is0_;
    void make_lamlist(void);
    void make_triglist(void);

  public:
    string name() const {return ved_name;}
    int  ved_id() const {return ved;}
    C_instr_system* is0() const {return is0_;}
    void ResetVED();
    C_confirmation* UploadEvent();
    C_confirmation* Initiate(int id);
    C_confirmation* Identify(int level);
    C_confirmation* GetVEDStatus(void);
    void CreateVariable(int idx, int size);
    void DeleteVariable(int idx);
    void WriteVariable(int idx, ems_i32 val);
    void WriteVariable(int idx, ems_i32* source, int size);
    int  ReadVariable(int idx);
    void ReadVariable(int idx, ems_i32* dest, int* size);
    void ReadVariable(int idx, ems_i32** dest, int* size);
    int  GetVariableAttributes(int idx);
    //void UploadDataout();
    C_data_io* UploadDataout(int idx);
    void StartReadout();
    void ResetReadout();
    void StopReadout();
    void ResumeReadout();
    InvocStatus GetReadoutStatus(ems_u32* eventcount);
    C_readoutstatus* GetReadoutStatus(int naux, const int* aux, int useaux=1);
    string GetReadoutParams();
    void UploadDataoutAddr(int idx, C_add_trans& addr);
    int  SetUnsol(int);
    void create_is(int idx, int id); // asynchroner Modus
    C_confirmation* GetConf(int xid, const struct timeval *timeout=0);
    C_confirmation* GetConf(const struct timeval *timeout=0);
    C_confirmation* GetNamelist();
    C_confirmation* GetNamelist(Object);
    C_confirmation* GetNamelist(Object, Domain);
    C_confirmation* GetNamelist(Object, Invocation);
    C_confirmation* GetNamelist(int*, int);
    C_confirmation* GetNamelist(int, ...);
    conf_mode confmode() const {return confmode_;}
    conf_mode confmode(conf_mode);
    int last_xid() const {return last_xid_;}
    int procnum(const char*, Capabtyp);
    int numprocs(Capabtyp);
    string procname(int, Capabtyp);
    char version_separator(Capabtyp);
    char version_separator(char, Capabtyp);
    C_confirmation* GetProcProperties(int, int, ems_u32*);
    void downdatain(int idx, int size, const ems_u32* domain);
    void DownloadDatain(int idx, const C_data_io& addr);
    void DownloadDatain(int idx, IOAddr addrtyp, C_add_trans addr);
    void DownloadDatain(int idx, IOAddr addrtyp, const char* name);
    void DownloadDatain(int idx, IOAddr addrtyp,
        const char* name, int space, C_add_trans offset, int option);
    void DownloadDatain(int idx, InOutTyp typ, IOAddr addrtyp,
        C_add_trans addr);
    void DownloadDatain(int idx, InOutTyp typ, IOAddr addrtyp,
        const char* name);
    void DownloadDatain(int idx, InOutTyp typ, IOAddr addrtyp,
        const char* name, int space, C_add_trans offset, int option);
    void DeleteDatain(int idx);
    C_data_io* UploadDatain(int idx);
    void downdataout(int idx, int size, const ems_u32* domain);
    void DownloadDataout(int idx, const C_data_io& addr);
    void DownloadDataout(int idx, InOutTyp typ, int buffersize, int priority,
        IOAddr addrtyp);           // nur fuer addrtyp==Addr_Null brauchbar
    void DownloadDataout(int idx, InOutTyp typ, int buffersize, int priority,
        IOAddr addrtyp, int addr); // nur fuer addrtyp==Addr_Raw brauchbar
    void DownloadDataout(int idx, InOutTyp typ, int buffersize, int priority,
        IOAddr addrtyp, const char* name); // Modul, LocalSocket, Tape, File
    void DownloadDataout(int idx, InOutTyp typ, int buffersize, int priority,
        IOAddr addrtyp, const char* host, int port); // Addr_Socket
    void DeleteDataout(int id);
    C_confirmation* GetDataoutStatus(int idx, int arg=0);
    void WindDataout(int idx, int offset);
    void WriteDataout(int idx, int default_header, const C_buf&);
    void WriteDataout(int idx, int default_header, int size,
            const ems_u32* data);
    void WriteDataout(int idx, int size, const char* data);
    void WriteDataout(int idx, string st);
    void EnableDataout(int idx, int val);
    void DeleteIS(int idx);
    C_isstatus* ISStatus(int);
    int is_num() const {return num_is;}
    C_instr_system* is_entry(int i) {return islist[i];}
    //void DownloadModullist(int modules, int* list);
    void DownloadModullist(const C_modullist&, int version=1);
    C_modullist* UploadModullist(int version);
    void DeleteModullist();
    void CreateLam(int idx, int id, int is, const char *trigproc,
        const int* args, int argnum);
    C_confirmation* UploadLam(int idx);
    void DeleteLam(int idx);
    void clear_lamlist() {if (!!lamlist) lamlist->clear();}
    void StartLam(int idx);
    void ResetLam(int idx);
    void StopLam(int idx);
    void ResumeLam(int idx);
    void lam_add_param(int par) {lamlist->add_par(par);}
    void lam_add_param(int num, int par, ...);
    void lam_add_param(const char* par) {lamlist->add_par(par);}
    void lam_add_proc(const char* proc);
    void lam_add_proc(const char* proc, int arg);
    void lam_add_proc(const char* proc, int num, /*int arg,*/ ...);
    void lam_add_proc(int proc);
    void DownloadLamproc(int idx, int unsolflag);
    C_confirmation* UploadLamproc(int idx);
    void DeleteLamproc(int idx);
    void clear_triglist() {if (!!triglist) triglist->clear();}
    void trig_set_proc(const char* proc);
    void trig_set_proc(int proc);
    void trig_add_param(int par) {triglist->add_par(par);}
    void trig_add_param(int num, int par, ...);
    void trig_add_param(const char* par) {triglist->add_par(par);}
    void DownloadTrigger(int idx);
    void DownloadTrigger(int idx, const char* proc, int argnum, ...);
    void DownloadTrigger(int idx, const char* proc, int* args, int argnum);
    C_confirmation* UploadTrigger(int idx);
    void DeleteTrigger(int idx);
};

#endif

/*****************************************************************************/
/*****************************************************************************/
