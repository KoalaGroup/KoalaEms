/*
 * commu_cdb.cc
 * 
 * created: 17.07.95
 * 14.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <errors.hxx>
#include <commu_log.hxx>

#include <exp_control_params.c>
#include <exp_control_structure.c>
#include <process_control_structure.c>
#include "versions.hxx"

VERSION("Jun 05 1996", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_cdb.cc,v 2.19 2004/11/26 15:14:15 wuestner Exp $")
#define XVERSION

typedef struct exp_control_structure ec;
typedef struct process_control_structure pc;

extern C_log elog, nlog, dlog;

ec *p_ec;
pc *p_pc;

extern "C"
  {
  int shm_ec_struc(ec**);
  int shm_pc_struc(pc**);
  int process_reg(pc*, int, const char*, int*);
  int process_can(pc*, int, int);
  int get_ved_param(ec*, const char*, int*, char*, int*, char*);
  }

#include <commu_cdb.hxx>

//const int PROC_TYPE=3;
//#define PROC_BASENAME "Communication"

const int C_cdb::proc_type=3;
const String C_cdb::proc_basename="Communication";

/*****************************************************************************/

C_cdb::C_cdb()
{
int res;

shm_ec_struc(&p_ec);   /*  get access to exp_control_structure */
shm_pc_struc(&p_pc);   /*  get access to process_control_structure */

/*
 * elog << time << "process_reg(" << (void*)p_pc << ", " << proc_type << ", "
 *     << (void*)&proc_num << ")" << flush;
 * cerr << "process_reg(" << (void*)p_pc << ", " << proc_type << ", "
 *     << (void*)&proc_num << ")" << flush;
 */

res=process_reg(p_pc, proc_type, proc_basename, &proc_num);
if (res!=0)
  {
  delete this;
  throw new C_program_error("error in process_reg");
  }
}

/*****************************************************************************/

C_cdb::~C_cdb()
{
/*
 * elog << time << "process_can(" << (void*)p_pc << ", " << proc_type << ", "
 *     << proc_num << ")" << flush;
 * cerr << "process_can(" << (void*)p_pc << ", " << proc_type << ", "
 *     << proc_num << ")" << flush;
 */

process_can(p_pc, proc_type, proc_num);
}

/*****************************************************************************/

char* C_cdb::listname()
{
ostrstream ss;
ss << "<COSY>" << ends;
return(ss.str());
}

/*****************************************************************************/

C_VED_addr* C_cdb::getVED(const String& ved_name)
{
int adr_type, res, port;
char host[256], path[1024];
char *ptr;

if (ved_name=="")
  throw new C_program_error("C_cdb::getVED: ved_name is empty");
res=get_ved_param(p_ec, ved_name, &adr_type, host, &port, path);
if (res!=0) return (C_VED_addr*)0; // VED not found

if (adr_type!=1)
  {
  cerr << "get_ved_param returns " << adr_type << " as adr_type." << endl;
  throw new C_program_error("get_ved_param returns wrong type");
  }
else
  {
  if ((ptr=strchr(host, ' '))!=NULL) *ptr=0;
  if ((ptr=strchr(path, ' '))!=NULL) *ptr=0;
  if (path[0]==0)
    {
    if (port==0)
      {
      C_VED_addr_unix* addr;
      addr=new C_VED_addr_unix(host);
      return(addr);
      }
    else
      {
      C_VED_addr_inet* addr;
      addr=new C_VED_addr_inet(host, port);
      return(addr);
      }
    }
  else
    {
    C_VED_addr_inet_path* addr;
    C_strlist* list;
    list=new C_strlist(1);
    list->set(0, path);
    addr=new C_VED_addr_inet_path(host, port, list);
    return(addr);
    }
  }
}

/*****************************************************************************/

C_strlist* C_cdb::getVEDlist()
{
int c, i;
C_strlist *strlist;
char ca;

for (i=0, c=0; i<max_ved; i++)
  {
  ca=p_ec->hc.ved[i].name[0];
  if ((ca!=0) && (ca!=' ')) c++;
  }
strlist=new C_strlist(c);
for (i=0, c=0; i<max_ved; i++)
// dass ungueltige Strings aus Leerzeichen bestehen, muss wohl an Fortran liegen
  {
  ca=p_ec->hc.ved[i].name[0];
  if ((ca!=0) && (ca!=' ')) strlist->set(c++, p_ec->hc.ved[i].name);
  }
return(strlist);
}

/*****************************************************************************/
/*****************************************************************************/
