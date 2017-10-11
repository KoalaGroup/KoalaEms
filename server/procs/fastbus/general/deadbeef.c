/*
 * procs/fastbus/general/deadbeef.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: deadbeef.c,v 1.9 2015/04/06 21:33:31 wuestner Exp $";

#include <sconf.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <assert.h>

#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../procs.h"
#include "../../procprops.h"
#ifdef OBJ_VAR
#include "../../../objects/var/variables.h"
#endif
#include "../../../objects/domain/dom_ml.h"
#include "../../../trigger/trigger.h"

extern ems_u32* outptr;
extern int *memberlist;
extern int wirbrauchen;

static void
dump(ems_u32* pp, unsigned int wc, ems_u32 pa, int ev)
{
    int i;
    printf("pa=%d ev=%d wc=%d count=%d\n", pa, ev, wc, *pp++);
    for (i=0; i<wc; i++) printf("  %08x\n", *pp++);
}

RCS_REGISTER(cvsid, "procs/fastbus/general")

/******************************************************************************/
static void
test_beef(ems_u32* pp, unsigned int wc, ems_u32 pa, int ev)
{
    int i;
    ems_u32 v, *p=pp+1;
    for (i=wc; i; i--) {
        v=*p++;
        if ((v==0xa5a5a5a5) || (v==0x5a5a5a5a)) {
            dump(pp, wc, pa, ev); return;
        }
    }
}
/*****************************************************************************/
/*
DeadBeef
p[0] : No. of parameters (==3)
p[1] : module
p[2] : secondary address
p[3] : count
outptr[0] : SS-code
outptr[1] : count
outptr[2] : values
*/

plerrcode proc_DeadBeef(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct fastbus_dev* dev=module->address.fastbus.dev;
    ems_u32 pa=module->address.fastbus.pa;
    int res;

    memset(outptr+2, 0x5a, p[3]<<2);
    res=dev->FRDB(dev, pa, p[2], p[3], outptr+2, outptr+0, "proc_DeadBeef");
    if (res>=0) {
        if (*outptr) {
            assert(res>0); /* res is at least 1 if ss is !=0 */
            res--; /* FRDB not stopped by limit counter */
        }
        outptr[1]=res;
        test_beef(outptr+1, (unsigned int)res, pa, trigger.eventcnt);
        outptr+=2+res;
        return plOK;
    } else {
        *outptr++=res;
        return plErr_System;
    }
}

plerrcode test_proc_DeadBeef(ems_u32* p)
{
    if (p[0]!=3) return(plErr_ArgNum);
    if (!valid_module(p[1], modul_fastbus)) return plErr_ArgRange;
    wirbrauchen=2+p[3];
    return plOK;
}
#ifdef PROCPROPS
static procprop DeadBeef_prop={1, -1, 0, 0};

procprop* prop_proc_DeadBeef()
{
    return(&DeadBeef_prop);
}
#endif
char name_proc_DeadBeef[]="DeadBeef";
int ver_proc_DeadBeef=1;

/*****************************************************************************/
/*
RandBeef
p[0] : No. of parameters (==3)
p[1] : module
p[2] : secondary address
p[3] : count
outptr[0] : SS-code
outptr[1] : start
outptr[2] : count
outptr[3] : values
*/

plerrcode proc_RandBeef(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct fastbus_dev* dev=module->address.fastbus.dev;
    ems_u32 pa=module->address.fastbus.pa;
    ems_u32 count, start;
    int res;

    count=random()&p[3]; if (!count) count=1;
    start=random()&(2048-count);
    memset(outptr+3, 0x5a, count<<2);
    outptr[1]=start;
    res=dev->FRDB(dev, pa, start, count, outptr+3, outptr+0, "proc_RandBeef");
    if (res>=0) {
        if (*outptr) res--; /* FRDB not stopped by limit counter */
        outptr[2]=res;
        outptr+=3+res;
        return plOK;
    } else {
        *outptr++=res;
        return plErr_System;
    }
}

plerrcode test_proc_RandBeef(ems_u32* p)
{
    struct timeval tv;
    if (p[0]!=3) return(plErr_ArgNum);
    if (!valid_module(p[1], modul_fastbus)) return plErr_ArgRange;
    gettimeofday(&tv, 0);
    srandom((unsigned int)tv.tv_usec);
    wirbrauchen=2+p[3];
    return plOK;
}
#ifdef PROCPROPS
static procprop RandBeef_prop={1, -1, 0, 0};

procprop* prop_proc_RandBeef()
{
    return(&RandBeef_prop);
}
#endif
char name_proc_RandBeef[]="RandBeef";
int ver_proc_RandBeef=1;

/*****************************************************************************/
/*****************************************************************************/
