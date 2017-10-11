/*
 * procs/camac/discriminator/ccf8200.c
 * created 2007-08-25 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: ccf8200.c,v 1.5 2015/04/06 21:33:26 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/camac/camac.h"
#include "../../procs.h"
#include "../../procprops.h"
#include "../../../objects/domain/dom_ml.h"

extern ems_u32* outptr;
extern int* memberlist;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/camac/discriminator")

/*
 * EG&G CCF8200 8-channel CF Discriminator
 */
/*
 * F(16) A(0..7) : write discriminator threshold to channel 0..7 (8 bit)
 * F(0)  A(0..7) : read discriminator threshold
 * F(17) A(0)    : write "A" output width (8 bit?)
 * F(1)  A(0)    : read  "A" output width
 * F(17) A(1)    : write "B" output width (8 bit?)
 * F(1)  A(1)    : read  "B" output width
 * F(17) A(2)    : write channel inhibit mask (8 bit)
 * F(1)  A(2)    : read channel inhibit mask
 * F(25) A(0)    : generate test pulse
 * F(9)          : clear module
 */

enum QX {Q=1, X=2};
/*****************************************************************************/
static plerrcode
write_with_check(struct camac_dev* dev, int N, int A, int F, int D, enum QX qx)
{
    camadr_t addr;
    ems_u32 stat;
    int res;

    addr=dev->CAMACaddr(N, 0, 17);
    res=dev->CAMACwrite(dev, &addr, D);
    res|=dev->CAMACstatus(dev, &stat);
    if (res) {
        printf("CCF8200 write CAMAC(N=%d A=%d F=%d D=%d: system error\n", N, A, F, D);
        return plErr_System;
    }
    if (qx&X) {
        if (!dev->CAMACgotX(stat)) {
            printf("CCF8200 write CAMAC(N=%d A=%d F=%d D=%d: no X\n", N, A, F, D);
            return plErr_HW;
        }
    }
    if (qx&Q) {
        if (!dev->CAMACgotQ(stat)) {
            printf("CCF8200 write CAMAC(N=%d A=%d F=%d D=%d: no Q\n", N, A, F, D);
            return plErr_HW;
        }
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
cntl_with_check(struct camac_dev* dev, int N, int A, int F, enum QX qx)
{
    camadr_t addr;
    ems_u32 stat;
    int res;

    addr=dev->CAMACaddr(N, 0, 17);
    res=dev->CAMACcntl(dev, &addr, &stat);
    if (res) {
        printf("CCF8200 cntl CAMAC(N=%d A=%d F=%d: system error\n", N, A, F);
        return plErr_System;
    }
    if (qx&X) {
        if (!dev->CAMACgotX(stat)) {
            printf("CCF8200 cntl CAMAC(N=%d A=%d F=%d: no X\n", N, A, F);
            return plErr_HW;
        }
    }
    if (qx&Q) {
        if (!dev->CAMACgotQ(stat)) {
            printf("CCF8200 cntl CAMAC(N=%d A=%d F=%d: no Q\n", N, A, F);
            return plErr_HW;
        }
    }
    return plOK;
}
/*****************************************************************************/
/*
 * p[0]: No. of arguments
 * p[1]: module (index in memberlist or modullist)
 * sets all registers to a default value
 */
plerrcode
proc_CCF8200init(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;
    plerrcode pres;
    int i;

    /* clear module */
    if ((pres=cntl_with_check(dev, N, 0, 9, X|Q))!=plOK)
        return pres;

    /* set all thresholds to maximum */
    for (i=0; i<8; i++) {
        if ((pres=write_with_check(dev, N, i, 16, 255, X|Q))!=plOK)
            return pres;
    }

    /* set "A" output width to maximum */
    if ((pres=write_with_check(dev, N, 0, 17, 255, X|Q))!=plOK)
        return pres;

    /* set "B" output width to maximum */
    if ((pres=write_with_check(dev, N, 1, 17, 255, X|Q))!=plOK)
        return pres;

    /* disable all channels */
    if ((pres=write_with_check(dev, N, 2, 17, 0xff, X|Q))!=plOK)
        return pres;

    return plOK;
}

plerrcode test_proc_CCF8200init(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=1)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=ORTEC_DISC_CCF8200)
        return plErr_BadModTyp;
    wirbrauchen=0;
    return plOK;
}

char name_proc_CCF8200init[]="CCF8200init";
int ver_proc_CCF8200init=1;
/*****************************************************************************/
/*
 * p[0]: No. of arguments
 * p[1]: module (index in memberlist or modullist)
 * p[2]: channel (0..7 or -1 for all)
 * p[3]: threshold
 */
plerrcode
proc_CCF8200threshold(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;
    ems_i32* ip=(ems_i32*)p;
    int i;

    if (ip[2]<0) {
        for (i=0; i<8; i++) {
            camadr_t addr=dev->CAMACaddr(N, i, 16);
            if (dev->CAMACwrite(dev, &addr, p[3])<0) {
                printf("CCF8200threshold: CAMACwrite slot %d failed\n", N);
                return plErr_System;
            }
        }
    } else {
        camadr_t addr=dev->CAMACaddr(N, p[2], 16);
        if (dev->CAMACwrite(dev, &addr, p[3])<0) {
            printf("CCF8200threshold: CAMACwrite slot %d failed\n", N);
            return plErr_System;
        }
    }
    return plOK;
}

plerrcode test_proc_CCF8200threshold(ems_u32* p)
{
    ml_entry* module;
    ems_i32* ip=(ems_i32*)p;

    if (p[0]!=3)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac))
        return plErr_ArgRange;
    if (ip[2]>7 && ip[2]<-1)
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=ORTEC_DISC_CCF8200)
        return plErr_BadModTyp;
    wirbrauchen=0;
    return plOK;
}

char name_proc_CCF8200threshold[]="CCF8200threshold";
int ver_proc_CCF8200threshold=1;
/*****************************************************************************/
/*
 * p[0]: No. of arguments
 * p[1]: module (index in memberlist or modullist)
 * p[2]: bitmask (channels to be inhibited)
 */
plerrcode
proc_CCF8200inhibit(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;

    camadr_t addr=dev->CAMACaddr(N, 2, 17);
    if (dev->CAMACwrite(dev, &addr, p[2])<0) {
        printf("CCF8200inhibit: CAMACwrite slot %d failed\n", N);
        return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_CCF8200inhibit(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=2)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=ORTEC_DISC_CCF8200)
        return plErr_BadModTyp;
    wirbrauchen=0;
    return plOK;
}

char name_proc_CCF8200inhibit[]="CCF8200inhibit";
int ver_proc_CCF8200inhibit=1;
/*****************************************************************************/
/*
 * p[0]: No. of arguments
 * p[1]: module (index in memberlist or modullist)
 * p[2]: 0: output "A", 1: output "B"
 * p[3]: width
 */
plerrcode
proc_CCF8200pulsewidth(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;

    camadr_t addr=dev->CAMACaddr(N, p[2], 17);
    if (dev->CAMACwrite(dev, &addr, p[3])<0) {
        printf("CCF8200pulsewidth: CAMACwrite slot %d failed\n", N);
        return plErr_System;
    }

    return plOK;
}

plerrcode test_proc_CCF8200pulsewidth(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=3)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac))
        return plErr_ArgRange;
    if (p[2]>1)
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=ORTEC_DISC_CCF8200)
        return plErr_BadModTyp;
    wirbrauchen=0;
    return plOK;
}

char name_proc_CCF8200pulsewidth[]="CCF8200pulsewidth";
int ver_proc_CCF8200pulsewidth=1;
/*****************************************************************************/
/*
 * p[0]: No. of arguments
 * p[1]: module (index in memberlist or modullist)
 */
plerrcode
proc_CCF8200testpulse(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;
    ems_u32 stat;
    camadr_t addr;

    addr=dev->CAMACaddr(N, 0, 25);
    if (dev->CAMACcntl(dev, &addr, &stat)<0) {
        printf("CCF8200testpulse: CAMACcntl slot %d failed\n", N);
        return plErr_System;
    }
    if (stat!=0xc0000000) {
        printf("CCF8200testpulse: CAMACcntl slot %d: stat=0x%x\n", N, stat);
        return plErr_HW;
    }
    return plOK;
}

plerrcode test_proc_CCF8200testpulse(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=1)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=ORTEC_DISC_CCF8200)
        return plErr_BadModTyp;
    wirbrauchen=0;
    return plOK;
}

char name_proc_CCF8200testpulse[]="CCF8200testpulse";
int ver_proc_CCF8200testpulse=1;
/*****************************************************************************/
/*****************************************************************************/
