/******************************************************************************
*                                                                             *
* GetVar.c                                                                    *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* created before 07.09.93                                                     *
* last changed 17.01.94                                                       *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: GetVarRange.c,v 1.6 2011/04/06 20:30:32 wuestner Exp $";

#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../objects/var/variables.h"
#include "../../procs.h"
#include "../../procprops.h"

extern ems_u32* outptr;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/general/vars")

/*****************************************************************************/
/*
 * p[0]: size (3)
 * p[1]: var
 * p[2]: start
 * p[3]: stop
 */
plerrcode proc_GetVarRange(ems_u32* p)
{
int len=var_list[p[1]].len;
if (len==1)
  *outptr++=var_list[p[1]].var.val;
else
  {
  int i;
  ems_u32* v=var_list[p[1]].var.ptr;
  for (i=p[2]; i<p[3]; i++) *outptr++=v[i];
  }
return(plOK);
}

plerrcode test_proc_GetVarRange(ems_u32* p)
{
int len;
if (p[0]!=3) return(plErr_ArgNum);
if (p[1]>MAX_VAR) return(plErr_IllVar);
if ((len=var_list[p[1]].len)==0) return(plErr_NoVar);
if ((unsigned int)p[2]>=len) return(plErr_ArgRange);
if ((unsigned int)p[3]>=len) return(plErr_ArgRange);
if (p[2]>p[3]) return(plErr_ArgRange);
wirbrauchen=p[3]-p[2]+1;
return(plOK);
}
#ifdef PROCPROPS
static procprop GetVarRange_prop={1, -1, "var start stop", ""};

procprop* prop_proc_GetVarRange()
{
return(&GetVarRange_prop);
}
#endif
char name_proc_GetVarRange[]="GetVarRange";
int ver_proc_GetVarRange=1;

/*****************************************************************************/
/*****************************************************************************/
