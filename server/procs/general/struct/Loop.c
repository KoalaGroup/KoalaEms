/*
 * procs/general/struct/Loop.c
 * created before 03.05.93
 */

static const char* cvsid __attribute__((unused))=
    "$ZEL: Loop.c,v 1.11 2017/10/09 21:25:37 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../procprops.h"
#include "../../proclist.h"

RCS_REGISTER(cvsid, "procs/general/struct")

/*****************************************************************************/

/* Loop(anz,{proclist})
fuehrt proclist anz-fach aus */

plerrcode proc_Loop(ems_u32* p)
{
    register int i;

    T(proc_Loop)
    i= *++p;
    p++;
    while(i--) scan_proclist(p);
    return plOK;
}

plerrcode test_proc_Loop(ems_u32* p)
{
    errcode res;
    ssize_t limit;

    T(test_proc_Loop)
    if(*p<2)
        return plErr_ArgNum;
    if ((res=test_proclist(p+2, *p-1, &limit)))
        return plErr_RecursiveCall ;
    if (limit>=0)
        wirbrauchen=p[1]*limit;
    return plOK;
}
#ifdef PROCPROPS
static procprop Loop_prop={1, -1, 0, 0};

procprop* prop_proc_Loop()
{
    return(&Loop_prop);
}
#endif
char name_proc_Loop[]="Loop";
int ver_proc_Loop=1;

/*****************************************************************************/
/*****************************************************************************/
