/*
 * emstcl_ved.hxx
 *
 * $ZEL: emstcl_ved.hxx,v 1.21 2010/02/03 00:15:51 wuestner Exp $
 * created: 06.09.95 PW
 */

#ifndef _emstcl_ved_hxx_
#define _emstcl_ved_hxx_

#include "config.h"
#include "cxxcompat.hxx"
#include <proc_ved.hxx>
#include <proc_modullist.hxx>
#include <tcl.h>
#include "findstring.hxx"

/*****************************************************************************/

class E_ved;

class E_confirmation {
  public:
    E_confirmation(Tcl_Interp*, E_ved*, const C_confirmation&);
    ~E_confirmation();
    STRING tclprocname() const;
    STRING origtclprocname() const {return origprocname;}
    void create_tcl_proc();
    static STRING new_E_confirmation(Tcl_Interp*, E_ved*,
        const C_confirmation&);
    int ConfirmationCommand(int, const char*[]);
    int deleted;
  protected:
    Tcl_Interp* interp;
    Tcl_Command tclcommand;
    STRING origprocname;
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
    STRING tclprocname() const;
    STRING origtclprocname() const {return origprocname;}
    void create_tcl_proc();
    virtual int VedCommand(int, const char*[]);
    typedef int (E_ved::*former)(OSTRINGSTREAM&, const C_confirmation*, int,
            int, const STRING*);
    STRING unsolcomm[NrOfUnsolmsg];
    STRING defunsolcomm;
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
      STRING* formargs;
      int numformargs;
      Tcl_DString*/*String*/ ok_command;
      Tcl_DString*/*String*/ err_command;
      void set_formargs(int, const STRING& arg);
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
        int callformer(E_ved*, OSTRINGSTREAM&, const C_confirmation*);
        void formargs(int idx, const STRING& arg) {entry->set_formargs(idx, arg);}
        //void ok_command(STRING ok_command) {entry->ok_command=ok_command;}
        //void err_command(STRING err_command) {entry->err_command=err_command;}
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
    STRING origprocname;
    int destructed;
    static const char* commnames[];
    //static int Ems_VedCommand(ClientData, Tcl_Interp*, int, char*[]);
    static void Ems_VedDelete(ClientData);
    confbackentry** confbacks;
    int numconfbacks, maxconfbacks;
    virtual const char* typname() const {return "unknown";}
    virtual int exec_comm(int, int, const char*[]);
    STRING ccomm;
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
    int f_dummy(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    int f_command(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    int f_void(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    virtual int f_lamupload(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    int f_lamstatus(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    int f_dostatus(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    int f_ioaddr(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    int ff_ioaddr(OSTRINGSTREAM&, const C_data_io*);
    int f_identify(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    virtual int f_createis(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    int f_integer(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    int f_integerlist(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    int f_rawintegerlist(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    int f_intfmt(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    int f_domevent(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    int f_modullist(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    void ff_modullist(OSTRINGSTREAM&, const C_modullist*, int version);
    virtual int f_trigger(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    int f_readvar(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    int f_isstatus(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    void ff_isstatus(OSTRINGSTREAM&, const C_isstatus*);
    int f_readoutstatus(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    virtual int f_readoutlist(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    //virtual int f_printconf(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    virtual int f_printconf_raw(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    virtual int f_printconf_text(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    virtual int f_printconf_tabular(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    int f_stringlist(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    int f_string(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
    int f_confirmation(OSTRINGSTREAM&, const C_confirmation*, int, int, const STRING*);
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
