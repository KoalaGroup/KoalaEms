/*
 * emstcl_ved.hxx
 *
 * $ZEL: emstcl_ved.hxx,v 1.22 2014/07/14 15:13:25 wuestner Exp $
 * created: 06.09.95 PW
 */

#ifndef _emstcl_ved_hxx_
#define _emstcl_ved_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <proc_ved.hxx>
#include <proc_modullist.hxx>
#include <tcl.h>
#include "findstring.hxx"

using namespace std;

/*****************************************************************************/

class E_ved;

class E_confirmation {
  public:
    E_confirmation(Tcl_Interp*, E_ved*, const C_confirmation&);
    ~E_confirmation();
    string tclprocname() const;
    string origtclprocname() const {return origprocname;}
    void create_tcl_proc();
    static string new_E_confirmation(Tcl_Interp*, E_ved*,
        const C_confirmation&);
    int ConfirmationCommand(int, const char*[]);
    int deleted;
  protected:
    Tcl_Interp* interp;
    Tcl_Command tclcommand;
    string origprocname;
    E_ved* ved;
    static const E_confirmation** econfirmations;
    static int num_econfirmations, max_econfirmations;
    C_confirmation* confirmation;
    //static int Ems_ConfirmationCommand(ClientData, Tcl_Interp*, int, char*[]);
    //static void Ems_ConfirmationDelete(ClientData);
    void reg() const;
    void unreg() const;
    int e_delete(int, const char*[]);
    int e_print(int, const char*[]);
    //int e_convert(int, char*[]);
    int e_get(int, const char*[]);
};
extern "C" {
int E_confirmation_Ems_ConfirmationCommand(ClientData, Tcl_Interp*, int,
        const char*[]);
void E_confirmation_Ems_ConfirmationDelete(ClientData);
}
/*****************************************************************************/

class E_ved: virtual public C_VED {
  public:
    E_ved(Tcl_Interp*, const char*, C_VED::VED_prior);
    virtual ~E_ved();
    string tclprocname() const;
    string origtclprocname() const {return origprocname;}
    void create_tcl_proc();
    virtual int VedCommand(int, const char*[]);
    typedef int (E_ved::*former)(ostringstream&, const C_confirmation*, int,
            int, const string*);
    string unsolcomm[NrOfUnsolmsg];
    string defunsolcomm;
    struct confbackentry
      {
      confbackentry():xid(-1), formargs(0), numformargs(0), ok_command(0),
          err_command(0) {}
      confbackentry(const confbackentry&);
      ~confbackentry();
      int xid;
      int former_default;
      int former_implicit;
      former form;
      string* formargs;
      int numformargs;
      Tcl_DString*/*String*/ ok_command;
      Tcl_DString*/*String*/ err_command;
      void set_formargs(int, const string& arg);
      void print() const;
      };
    class confbackbox
      {
      public:
        confbackbox() :entry(0) {}
        confbackbox(const confbackbox&);
        ~confbackbox() {delete entry;}
        int init(Tcl_Interp*, const E_ved*, int&, const char*[], former);
        int finit(Tcl_Interp*, const E_ved*, int&, const char*[], former);
        confbackentry* get();
        void xid(int id) {entry->xid=id;}
        //void needtest(int needtest) {entry->needtest=needtest;}
        void form(former form) {entry->form=form;}
        int callformer(E_ved*, ostringstream&, const C_confirmation*);
        void formargs(int idx, const string& arg) {entry->set_formargs(idx, arg);}
        //void ok_command(string ok_command) {entry->ok_command=ok_command;}
        //void err_command(string err_command) {entry->err_command=err_command;}
        void print() const {entry->print();}
        int numformargs() const {return entry->numformargs;}
        int isdefault() const {return entry->former_default;}
        int isimplicit() const {return entry->former_implicit;}
      protected:
        confbackentry* entry;
      };
    friend class confbackbox;
    class formerent
      {
      public:
        formerent();
        formerent(const char*, former);
        const char* name;
        former form;
      };
    class formerents: public strarr
      {
      public:
        formerents();
        virtual ~formerents();
      protected:
        formerent* formers;
        int num_, size;
      public:
        void add(formerent);
        virtual const char* str(int i) const {return formers[i].name;}
        virtual int num() const {return num_;}
        formerent& operator [] (int i) const {return formers[i];}
        virtual int lookup(const char*) const;
        virtual int lookup(former) const;
      };
  protected:
    Tcl_Interp* interp;
    Tcl_Command tclcommand;
    string origprocname;
    int destructed;
    static const char* commnames[];
    //static int Ems_VedCommand(ClientData, Tcl_Interp*, int, char*[]);
    static void Ems_VedDelete(ClientData);
    confbackentry** confbacks;
    int numconfbacks, maxconfbacks;
    virtual const char* typname() const {return "unknown";}
    virtual int exec_comm(int, int, const char*[]);
    string ccomm;
    int hasconfback(int) const;
    void deletecommand();
    //static void timerdummy(ClientData);
    int e_close(int, const char*[]);
    int e_closecommand(int, const char*[]);
    int e_name(int, const char*[]);
    int e_confmode(int, const char*[]);
    int e_pending(int, const char*[]);
    int e_flush(int, const char*[]);
    int e_reset(int, const char*[]);
    int e_var(int, const char*[]);
    int e_var_create(int, const char*[]);
    int e_var_delete(int, const char*[]);
    int e_var_read(int, const char*[]);
    int e_var_write(int, const char*[]);
    int e_var_size(int, const char*[]);
    int e_identify(int, const char*[]);
    int e_initiate(int, const char*[]);
    int e_status(int, const char*[]);
    int e_namelist(int, const char*[]);
    int e_unsol(int, const char*[]);
    int e_typedunsol(int, const char*[]);
    int e_event(int, const char*[]);
    int e_readout(int, const char*[]);
    int e_ro_start(int, const char*[]);
    int e_ro_stop(int, const char*[]);
    int e_ro_resume(int, const char*[]);
    int e_ro_reset(int, const char*[]);
    int e_ro_status(int, const char*[]);
    int e_datain(int, const char*[]);
    int e_datain_create(int, const char*[]);
    int e_datain_delete(int, const char*[]);
    int e_datain_upload(int, const char*[]);
    int e_dataout(int, const char*[]);
    int e_dataout_create(int, const char*[]);
    int e_dataout_delete(int, const char*[]);
    int e_dataout_status(int, const char*[]);
    int e_dataout_upload(int, const char*[]);
    int e_dataout_wind(int, const char*[]);
    int writedataout(confbackbox&, int, int, char*, int);
    int writedataout(confbackbox&, int, int, C_outbuf&);
    int e_dataout_write(int, const char*[]);
    int e_dataout_writefile(int, const char*[]);
    int e_dataout_enable(int, const char*[]);
    int e_is(int, const char*[]);
    int e_is_list(int, const char*[]);
    int e_is_exists(int, const char*[]);
    int e_is_create(int, const char*[]);
    int e_is_open(int, const char*[]);
    int e_is_delete(int, const char*[]);
    int e_is_id(int, const char*[]);
    int e_openis(int, const char*[]);
    int e_version_separator(int, const char*[]);
    int e_command(int, const char*[]);
    int e_command1(int, const char*[]);
    int e_modullist(int, const char*[]);
    int e_modlist_create(int, const char*[]);
    int e_modlist_upload(int, const char*[]);
    int e_modlist_delete(int, const char*[]);
    int e_lam(int, const char*[]);
    int e_lam_create(int, const char*[]);
    int e_lam_delete(int, const char*[]);
    int e_lam_status(int, const char*[]);
    int e_lam_start(int, const char*[]);
    int e_lam_stop(int, const char*[]);
    int e_lam_resume(int, const char*[]);
    int e_lam_reset(int, const char*[]);
    int e_lamproclist(int, const char*[]);
    int e_lamproclist_setup(int, const char*[]);
    int e_lamproclist_create(int, const char*[]);
    int e_lamproclist_delete(int, const char*[]);
    int e_lamproclist_upload(int, const char*[]);
    int e_trigger(int, const char*[]);
    int e_trig_upload(int, const char*[]);
    int e_trig_delete(int, const char*[]);
    int e_trig_create(int, const char*[]);
    int e_caplist(int, const char*[]);
    static formerents formers;
  public:
    const char* formername(former) const;
    void installconfback(confbackentry*);
    void installconfback(confbackbox& box) {installconfback(box.get());}
    int f_dummy(ostringstream&, const C_confirmation*, int, int, const string*);
    int f_command(ostringstream&, const C_confirmation*, int, int, const string*);
    int f_void(ostringstream&, const C_confirmation*, int, int, const string*);
    virtual int f_lamupload(ostringstream&, const C_confirmation*, int, int, const string*);
    int f_lamstatus(ostringstream&, const C_confirmation*, int, int, const string*);
    int f_dostatus(ostringstream&, const C_confirmation*, int, int, const string*);
    int f_ioaddr(ostringstream&, const C_confirmation*, int, int, const string*);
    int ff_ioaddr(ostringstream&, const C_data_io*);
    int f_identify(ostringstream&, const C_confirmation*, int, int, const string*);
    virtual int f_createis(ostringstream&, const C_confirmation*, int, int, const string*);
    int f_integer(ostringstream&, const C_confirmation*, int, int, const string*);
    int f_integerlist(ostringstream&, const C_confirmation*, int, int, const string*);
    int f_rawintegerlist(ostringstream&, const C_confirmation*, int, int, const string*);
    int f_intfmt(ostringstream&, const C_confirmation*, int, int, const string*);
    int f_domevent(ostringstream&, const C_confirmation*, int, int, const string*);
    int f_modullist(ostringstream&, const C_confirmation*, int, int, const string*);
    void ff_modullist(ostringstream&, const C_modullist*, int version);
    virtual int f_trigger(ostringstream&, const C_confirmation*, int, int, const string*);
    int f_readvar(ostringstream&, const C_confirmation*, int, int, const string*);
    int f_isstatus(ostringstream&, const C_confirmation*, int, int, const string*);
    void ff_isstatus(ostringstream&, const C_isstatus*);
    int f_readoutstatus(ostringstream&, const C_confirmation*, int, int, const string*);
    virtual int f_readoutlist(ostringstream&, const C_confirmation*, int, int, const string*);
    //virtual int f_printconf(ostringstream&, const C_confirmation*, int, int, const string*);
    virtual int f_printconf_raw(ostringstream&, const C_confirmation*, int, int, const string*);
    virtual int f_printconf_text(ostringstream&, const C_confirmation*, int, int, const string*);
    virtual int f_printconf_tabular(ostringstream&, const C_confirmation*, int, int, const string*);
    int f_stringlist(ostringstream&, const C_confirmation*, int, int, const string*);
    int f_string(ostringstream&, const C_confirmation*, int, int, const string*);
    int f_confirmation(ostringstream&, const C_confirmation*, int, int, const string*);
    int confbacknum() const {return numconfbacks;}
    confbackentry* extractconfback(int);
};
extern "C" {
int E_ved_Ems_VedCommand(ClientData, Tcl_Interp*, int, const char*[]);
void E_ved_timerdummy(ClientData);
}
/*****************************************************************************/

class E_veds {
  public:
    E_veds();
    ~E_veds();
  private:
    E_ved** list;
    int listsize;
    int list_idx;
  protected:
    friend class E_ved;
    void add(E_ved* ved);
    void remove(const E_ved* ved);
  public:
    //static void timerdummy(ClientData); replaced by E_veds_timerdummy
    int  count() const {return list_idx;}
    E_ved* operator[] (int idx) const {return list[idx];}
    E_ved* find(int) const;
    int find(const E_ved* ved) const;
    int pending() const;
};

extern E_veds eveds;
void set_error_code(Tcl_Interp*, const C_error*);
extern "C" void E_veds_timerdummy(ClientData);

#endif

/*****************************************************************************/
/*****************************************************************************/
