/*
 * commu_local_server_l.hxx.m4
 * 
 * $ZEL: commu_local_server_l.hxx.m4,v 2.8 2009/08/21 22:02:28 wuestner Exp $
 * 
 * created 05.02.95 PW
 */

#ifndef _commu_local_server_l_hxx_
#define _commu_local_server_l_hxx_

#include <commu_local_server.hxx>

/*****************************************************************************/

class C_local_server_l: public C_local_server
  {
  public:
    C_local_server_l(const char*, en_policies);
    virtual ~C_local_server_l();

    typedef struct
      {
      plerrcode (C_local_server_l::*proc)(const ems_u32*);
      plerrcode (C_local_server_l::*test_proc)(const ems_u32*);
      const char* name_proc;
      int* ver_proc;
      } listproc;

    typedef struct
      {
      procprop* (C_local_server_l::*prop_proc)();
      } listprop;

  protected:
    C_VED_addr_builtin addr;

    define(`proc',`plerrcode proc_$1(const ems_u32* ptr);
    plerrcode test_proc_$1(const ems_u32* ptr);
    procprop* prop_proc_$1();
    static const char name_proc_$1[];
    static int ver_proc_$1;')
    include(SRC/commu_procs_l.inc)

    static int NrOfProcs;
    static listproc Proc[];
    static listprop Prop[];

    virtual errcode Nothing(const ems_u32* ptr, int num);
    virtual errcode Initiate(const ems_u32* ptr, int num);
    virtual errcode Conclude(const ems_u32* ptr, int num);
    virtual errcode Identify(const ems_u32* ptr, int num);
    virtual errcode GetNameList(const ems_u32* ptr, int num);
    virtual errcode GetCapabilityList(const ems_u32* ptr, int num);
    virtual errcode GetProcProperties(const ems_u32* ptr, int num);
    virtual errcode DoCommand(const ems_u32* ptr, int num);
    virtual errcode TestCommand(const ems_u32* ptr, int num);
    errcode test_proclist(const ems_u32* ptr, int num);
    errcode scan_proclist(const ems_u32* ptr);

  public:
    //void logstring(const char*);
    void logstring(const STRING&);
    virtual const C_VED_addr& get_addr() const {return(addr);}
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
