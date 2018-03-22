/*
 * procs/general/struct/vonundzuvar.c
 * created before 03.03.94
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: vonundzuvar.c,v 1.9 2017/10/09 21:25:37 wuestner Exp $";

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

plerrcode proc_ResultInVar(ems_u32* p)
{
    ems_u32* help;
    plerrcode res;
    int len, i;
    help=outptr;
    if ((res=scan_proclist(p+2))) return(plErr_Other);
    i=outptr-help;
    len=var_list[p[1]].len;
    if (len==1) {
        var_list[p[1]].var.val= *help;
    } else {
        register ems_u32 *ptr;
        outptr=help;
        if(i>len)i=len;
        ptr=var_list[p[1]].var.ptr;
        while(i--)*ptr++= *outptr++;
    }
    outptr=help;
    return(plOK);
}

plerrcode test_proc_ResultInVar(ems_u32* p)
{
    if (p[0]<2) return(plErr_ArgNum);
    if (p[1]>MAX_VAR) return(Err_IllVar);
    if (!var_list[p[1]].len) return(Err_NoVar);
    if (test_proclist(p+2,p[0]-1,0)) return(plErr_RecursiveCall);
    wirbrauchen=0; /* ??? - Der Platz wird temporaer gebraucht! */
    return(plOK);
}
#ifdef PROCPROPS
static procprop ResultInVar_prop={0, 0, 0, 0};

procprop* prop_proc_ResultInVar()
{
    return(&ResultInVar_prop);
}
#endif
char name_proc_ResultInVar[]="ResultInVar";
int ver_proc_ResultInVar=1;

/*****************************************************************************/
/*****************************************************************************/
