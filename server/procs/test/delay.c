/*
 * procs/test/delay.c
 * created 1994-11-04
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: delay.c,v 1.10 2016/06/30 21:53:33 wuestner Exp $";

#include <errorcodes.h>
#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "../procprops.h"
#include "../procs.h"
#include "../../lowlevel/oscompat/oscompat.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/test")

/*****************************************************************************/
/*
Delay(sec) wartet sec Sekunden, bevor es mit plOK antwortet
*/
plerrcode proc_Delay(ems_u32* p)
{
    int d;
    D(D_USER|D_TIME, printf("Delay: sleep %d seconds.\n", p[1]);)
    d=p[1];
    tsleep(d*100);

    return(plOK);
}

plerrcode test_proc_Delay(ems_u32* p)
{
if (p[0]!=1) return(plErr_ArgNum);
if ((unsigned int)p[1]>100) return(plErr_ArgRange);
return(plOK);
}
#ifdef PROCPROPS
static procprop Delay_prop={0, 0, "<delay>", 0};

procprop* prop_proc_Delay(void)
{
return(&Delay_prop);
}
#endif
char name_proc_Delay[]="Delay";
int ver_proc_Delay=1;

/*****************************************************************************/
/*
 * p[0]: # of arguments
 * p[1]: milliseconds to wait
 * mDelay(msec) waits msec milliseconds before returning
 */
plerrcode proc_mDelay(ems_u32* p)
{
    struct timeval to;

    to.tv_sec=p[1]/1000;
    to.tv_usec=(p[1]%1000)*1000;

    select(0, 0, 0, 0, &to); /* may be interrupted */

    return plOK;
}

plerrcode test_proc_mDelay(ems_u32* p)
{
    if (p[0]!=1)
        return plErr_ArgNum;
    return plOK;
}
char name_proc_mDelay[]="mDelay";
int ver_proc_mDelay=1;

/*****************************************************************************/
/*****************************************************************************/
