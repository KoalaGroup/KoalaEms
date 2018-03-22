/*
 * procs/general/struct/VarLoop.c
 * created before 28.09.93
 */

static const char* cvsid __attribute__((unused))=
    "$ZEL: VarLoop.c,v 1.11 2017/10/09 21:25:37 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "../../../objects/var/variables.h"
#include <errorcodes.h>
#include "../../procs.h"
#include "../../procprops.h"
#include "../../proclist.h"

RCS_REGISTER(cvsid, "procs/general/struct")

/*****************************************************************************/
/*
VarLoop(var,{proclist})
  fuehrt proclist dem Wert von var (muss Laenge 1 haben)
  entsprechend oft aus
*/
plerrcode proc_VarLoop(ems_u32* p)
{
register int i;

T(proc_VarLoop)
i= var_list[*++p].var.val;
p++;
while (i--) scan_proclist(p);
return(plOK);
}

plerrcode proc_While(ems_u32* p)
{
  register ems_u32 *i;

  i= &var_list[*++p].var.val;
  p++;
  while(*i)scan_proclist(p);
  return(plOK);
}

plerrcode test_proc_VarLoop(ems_u32* p)
{
ssize_t limit;
T(test_proc_VarLoop)
if (*p<2) return(plErr_ArgNum);
if (p[1]>MAX_VAR) return(Err_IllVar);
if (!var_list[p[1]].len) return(Err_NoVar);
if (var_list[p[1]].len!=1) return(plErr_IllVarSize);
if(test_proclist(p+2,*p-1,&limit))return(plErr_RecursiveCall);
if(limit>=0)wirbrauchen=var_list[p[1]].var.val*limit;
return(plOK);
}

plerrcode test_proc_While(ems_u32* p)
{
  ssize_t limit;
  if(*p<2)return(plErr_ArgNum);
  if(p[1]>MAX_VAR)return(Err_IllVar);
  if(!var_list[p[1]].len)return(Err_NoVar);
  if(var_list[p[1]].len!=1)return(plErr_IllVarSize);
  if(test_proclist(p+2,*p-1,&limit))return(plErr_RecursiveCall);
  return(plOK);
}
#ifdef PROCPROPS
static procprop VarLoop_prop={1, -1, 0, 0};
static procprop While_prop={1, -1, 0, 0};

procprop* prop_proc_VarLoop()
{
return(&VarLoop_prop);
}

procprop* prop_proc_While()
{
return(&While_prop);
}
#endif
char name_proc_VarLoop[]="VarLoop";
char name_proc_While[]="While";
int ver_proc_VarLoop=1;
int ver_proc_While=1;

/*****************************************************************************/
/*****************************************************************************/
