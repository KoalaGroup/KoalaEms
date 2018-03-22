/*
 * procs/general/struct/IfThenElse.c
 * created before 28.09.93
 */

static const char* cvsid __attribute__((unused))=
    "$ZEL: IfThenElse.c,v 1.11 2017/10/09 21:25:37 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "../../../objects/var/variables.h"
#include <errorcodes.h>
#include "../../procs.h"
#include "../../procprops.h"
#include "../../proclist.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/general/struct")

/*****************************************************************************/
/*
IfThenElse(var,{proclist1},{proclist2})
  fuehrt proclist1 aus, wenn der Wert von var (muss Laenge 1 haben) !=0 ist,
  sonst proclist2
*/
plerrcode proc_IfThenElse(ems_u32* p)
{
    register int i;

    T(proc_IfThenElse)
    p++;
    if (var_list[*p++].var.val)
        scan_proclist(p);
    else {
        i= *(p++);
        while (i--) p+= *(p+1)+2;
        scan_proclist(p);
    }
    return(plOK);
}

plerrcode test_proc_IfThenElse(ems_u32* p)
{
    ems_u32* help;
    ssize_t limit1, limit2;
    int i;

    T(test_proc_IfThenElse)
    if (*p<3) return(plErr_ArgNum);
    if (p[1]>MAX_VAR) return(plErr_IllVar);
    if (!var_list[p[1]].len) return(plErr_NoVar);
    if (var_list[p[1]].len!=1) return(plErr_IllVarSize);
    if(test_proclist(p+2, *p-1, &limit1)) {
        *outptr++=1;
        return(plErr_RecursiveCall);
    }
    i=p[2];
    help=p+3;
    while (i--) help+= *(help+1)+2;
    if (test_proclist(help,*p-(help-p)+1,&limit2)) {
        *outptr++=2;
        return(plErr_RecursiveCall);
    }
    if ((limit1>=0)&&(limit1>=0))
        wirbrauchen=(limit1>limit2?limit1:limit2);
    return(plOK);
}
#ifdef PROCPROPS
static procprop IfThenElse_prop={1, -1, 0, 0};

procprop* prop_proc_IfThenElse()
{
    return(&IfThenElse_prop);
}
#endif
char name_proc_IfThenElse[]="IfThenElse";
int ver_proc_IfThenElse=1;

/*****************************************************************************/
/*****************************************************************************/
