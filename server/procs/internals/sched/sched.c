static const char* cvsid __attribute__((unused))=
    "$ZEL: sched.c,v 1.6 2017/10/20 23:20:52 wuestner Exp $";

#include <stdlib.h>
#include <errorcodes.h>

#include <xdrstring.h>
#include <rcs_ids.h>
#include "../../../main/scheduler.h"
#include "../../procprops.h"
#include "../../procs.h"

RCS_REGISTER(cvsid, "procs/internals/sched")

plerrcode proc_PrintTasks(__attribute__((unused)) ems_u32* p)
{
  sched_print_tasks();
  return(plOK);
}

plerrcode test_proc_PrintTasks(__attribute__((unused)) ems_u32* p)
{
  return(plOK);
}

char name_proc_PrintTasks[]="PrintTasks";
int ver_proc_PrintTasks=1;

plerrcode proc_AdjustPrio(ems_u32* p)
{
  char *name;
  ems_u32 *help;

  help=xdrstrcdup(&name, p+1);
  sched_adjust_prio(name,help[0],help[1]);
  free(name);
  return(plOK);
}

plerrcode test_proc_AdjustPrio(ems_u32* p)
{
  if((p[0]<3)||(p[0]!=xdrstrlen(p+1)+2))return(plErr_ArgNum);
  return(plOK);
}

#ifdef PROCPROPS
static procprop all_prop={-1, -1, "keine Ahnung", "keine Ahnung"};

procprop* prop_proc_AdjustPrio()
{
return(&all_prop);
}
procprop* prop_proc_PrintTasks()
{
return(&all_prop);
}
#endif

char name_proc_AdjustPrio[]="AdjustPrio";
int ver_proc_AdjustPrio=1;
