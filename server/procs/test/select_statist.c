/*
 * procs/test/select_statist.c
 * OS9
 * 04.11.94
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: select_statist.c,v 1.8 2017/10/20 23:20:52 wuestner Exp $";

#include <math.h>
#include <errorcodes.h>
#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "../procprops.h"
#include "../procs.h"
#include "../../main/scheduler.h"
/*#include <xdrfloat.h>*/
#include <xdrstring.h>

#ifdef SELECT_STATIST
/*
 * extern void sched_select_task_get_statist(seltaskdescr* descr, float* enabled,
 *     float* disabled, int clear);
 */
#endif

RCS_REGISTER(cvsid, "procs/test")

/*****************************************************************************/
#ifdef SELECT_STATIST
static void putfloat(double v)
{
    if (v==0) {
        *outptr++=1;
        *outptr++=0;
    } else {
        double ilog;
        int scale, i;
        modf(log10(v), &ilog);
        scale=8-(int)ilog;
        for (i=0; i<scale; i++) v*=10.;
        *outptr++=scale;
        *outptr++=(ems_i32)v;
    }
}
#endif
/*****************************************************************************/

plerrcode proc_SelStat(__attribute__((unused)) ems_u32* p)
{
#ifdef SELECT_STATIST
    struct seltaskdescr* h;
    ems_u32* help=outptr++;
    int num=1;

{
    double runtime, idle;
    sched_select_get_statist(&runtime, &idle, (int)p[1]);
    outptr=outstring(outptr, "select");
    putfloat(runtime);
    putfloat(idle);
}

    h=get_first_select_task();
    while (h) {
        double enabled, disabled;
        outptr=outstring(outptr, h->name);
        sched_select_task_get_statist(h, &enabled, &disabled, (int)p[1]);
        putfloat(enabled);
        putfloat(disabled);
        num++;
        h=get_next_select_task(h);
    }
    *help=num;
#endif
    return plOK;
}

plerrcode test_proc_SelStat(__attribute__((unused)) ems_u32* p)
{
#ifdef SELECT_STATIST
    if (p[0]!=1) return plErr_ArgNum;
    if ((p[1]!=0) && (p[1]!=1)) return plErr_ArgRange;
#else
    return plErr_NoSuchProc;
#endif
    return plOK;
}
#ifdef PROCPROPS
static procprop SelStat_prop={0, 1, "void", 0};

procprop* prop_proc_SelStat(void)
{
    return(&SelStat_prop);
}
#endif
char name_proc_SelStat[]="SelectStat";
int ver_proc_SelStat=1;

/*****************************************************************************/
/*****************************************************************************/
