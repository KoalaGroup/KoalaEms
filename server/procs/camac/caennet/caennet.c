/*
 * procs/camac/caennet/caennet.c
 * 
 * created before 27.07.94
 * 02.Aug.2001 PW: multicrate support
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: caennet.c,v 1.12 2011/04/06 20:30:30 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/camac/camac.h"
#include "../../../lowlevel/oscompat/oscompat.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../procs.h"
#include "../../procprops.h"

extern ems_u32* outptr;
extern int* memberlist;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/camac/caennet")

static void select_target(ml_entry* n, int crate, int modul)
{
    struct camac_dev* dev=n->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(n), 0, 17);
    dev->CAMACwrite(dev, &addr, (crate<<8)+modul);
    tsleep(6);
}

static plerrcode get_status(ml_entry* n)
{
    ems_u32 *help=outptr;
    int res;
    struct camac_dev* dev=n->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(n), 0, 0);

    res=dev->CAMACblreadQstop(dev, &addr, 10, outptr);
    if (res>=0)
        outptr+=res;
    else
        return plErr_System;

    if ((outptr-help)==2) {
        if (*help&0x8000) {
            outptr=help+1;
            return(plErr_HW);
        } else {
            outptr=help;
        }
    } else { /* sollte nicht passieren */
        printf("CAENNET: Status hat %lld Worte\n",
                (unsigned long long)(outptr-help-1));
        return plErr_HW;
    }
    return(plOK);
}

static void do_write(ml_entry* n, int param, int val)
{
    struct camac_dev* dev=n->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(n), param, 16);
    dev->CAMACwrite(dev, &addr, val);
    tsleep(12);
}

#define LOOPS 100

static plerrcode do_read(ml_entry* n, int param, int max)
{
    struct camac_dev* dev;
    camadr_t addr;
    ems_u32 val;
    int cnt, slot, res;
    ems_u32 *help;

    dev=n->address.camac.dev;
    slot=CAMACslot_e(n);

    addr=dev->CAMACaddr(slot, param, 17);
    dev->CAMACwrite(dev, &addr, 0xff00);
    tsleep(12);
    addr=dev->CAMACaddr(slot, 0, 0);
    cnt=LOOPS;
    do {
        dev->CAMACread(dev, &addr, &val);
        tsleep(6);
    } while (!dev->CAMACgotQ(val)&&(--cnt));
    if (!cnt) return(plErr_HW);
    help=outptr;
    *outptr++=val;
    res=dev->CAMACblreadQstop(dev, &addr, max, outptr);
    if (res>=0)
        outptr+=res;
    else
        return plErr_System;
    if (((outptr-help)==2) && (*help&0x8000)) {
        outptr=help+1;
        return(plErr_HW);
    }
    outptr--;
    return(plOK);
}

plerrcode proc_CAENnetWrite(ems_u32* p)
{
    ml_entry* n;
    n=ModulEnt(p[1]);
    select_target(n,p[2],p[3]);
    do_write(n,p[4],p[5]);
    return get_status(n);
}

plerrcode proc_CAENnetRead(ems_u32* p)
{
    ml_entry* n;
    n=ModulEnt(p[1]);
    select_target(n,p[2],p[3]);
    return do_read(n,p[4],600);
}

static plerrcode test_CAENparams(ems_u32* p)
{
    if (!valid_module(p[1], modul_camac, 0)) return(plErr_ArgRange);
    if (ModulEnt(p[2])->modultype!=CAENNET_SLOW) return(plErr_BadModTyp);
    return plOK;
}

plerrcode test_proc_CAENnetWrite(ems_u32* p)
{
    if (*p!=5)return(plErr_ArgNum);
    return test_CAENparams(p);
}

plerrcode test_proc_CAENnetRead(ems_u32* p)
{
    if (*p!=4)return(plErr_ArgNum);
   return test_CAENparams(p);
}

#ifdef PROCPROPS
static procprop all_prop={
    -1,
    -1,
    "keine Ahnung",
    "keine Ahnung"
};

procprop* prop_proc_CAENnetWrite()
{
    return &all_prop;
}

procprop* prop_proc_CAENnetRead()
{
    return &all_prop;
}
#endif

char name_proc_CAENnetWrite[]="CAENnetWrite";
char name_proc_CAENnetRead[]="CAENnetRead";

int ver_proc_CAENnetWrite=1;
int ver_proc_CAENnetRead=1;

/*****************************************************************************/
/*****************************************************************************/
