/*
 * procs/camac/discriminator/C808.c
 * created 2007-04-04 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: c808.c,v 1.3 2015/04/06 21:33:24 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/camac/camac.h"
#include "../../procs.h"
#include "../../procprops.h"
#include "../../../objects/domain/dom_ml.h"

RCS_REGISTER(cvsid, "procs/camac/discriminator")

extern ems_u32* outptr;
extern int* memberlist;
extern int wirbrauchen;


/*
 * CAEN C808 16-channel constant fraction Discriminator
 */
/*
 * after power up all register contents are undefined!
 * X and Q are generated for all valid commands
 *
 * F(16) A(0..15): write discriminator threshold to channel 0..15 (8 bit)
 * F(17) A(0)    : write channel enable pattern (16 bit)
 * F(18) A(0)    : write output width for channels 0..7  (8 bit)
 * F(18) A(1)    : write output width for channels 8..15 (8 bit)
 * F(19) A(0)    : write dead time for channels 0..7  (8 bit)
 * F(19) A(1)    : write dead time for channels 8..15 (8 bit)
 * F(20) A(0)    : write majority threshold (8 bit)
 * F(25) A(0)    : generate test pulse
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
        printf("C808 write CAMAC(N=%d A=%d F=%d D=%d: system error\n", N, A, F, D);
        return plErr_System;
    }
    if (qx&X) {
        if (!dev->CAMACgotX(stat)) {
            printf("C808 write CAMAC(N=%d A=%d F=%d D=%d: no X\n", N, A, F, D);
            return plErr_HW;
        }
    }
    if (qx&Q) {
        if (!dev->CAMACgotQ(stat)) {
            printf("C808 write CAMAC(N=%d A=%d F=%d D=%d: no Q\n", N, A, F, D);
            return plErr_HW;
        }
    }
    return plOK;
}
/*****************************************************************************/
/*
 * p[0]: No. of arguments
 * p[1]: module (index in memberlist or modullist)
 * sets all 
 */
plerrcode
proc_C808init(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;
    plerrcode pres;
    int i;

    /* set all thresholds to maximum */
    for (i=0; i<16; i++) {
        if ((pres=write_with_check(dev, N, i, 16, 255, X|Q))!=plOK)
            return pres;
    }

    /* disable all channels */
    if ((pres=write_with_check(dev, N, 0, 17, 0, X|Q))!=plOK)
        return pres;

    /* set output width to maximum */
    if ((pres=write_with_check(dev, N, 0, 18, 255, X|Q))!=plOK)
        return pres;
    if ((pres=write_with_check(dev, N, 1, 18, 255, X|Q))!=plOK)
        return pres;

    /* set dead time to minimum */
    if ((pres=write_with_check(dev, N, 0, 19, 0, X|Q))!=plOK)
        return pres;
    if ((pres=write_with_check(dev, N, 1, 19, 0, X|Q))!=plOK)
        return pres;

    /* set majority threshold to maximum */
    if ((pres=write_with_check(dev, N, 0, 20, 255, X|Q))!=plOK)
        return pres;

    return plOK;
}

plerrcode test_proc_C808init(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=1)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=CAEN_DISC_C808)
        return plErr_BadModTyp;
    wirbrauchen=0;
    return plOK;
}

char name_proc_C808init[]="C808init";
int ver_proc_C808init=1;
/*****************************************************************************/
/*
 * p[0]: No. of arguments
 * p[1]: module (index in memberlist or modullist)
 * p[2]: channel (0..15 or -1 for all
 * p[3]: threshold
 */
plerrcode
proc_C808threshold(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;
    ems_i32* ip=(ems_i32*)p;
    int i;

    if (ip[2]<0) {
        for (i=0; i<16; i++) {
            camadr_t addr=dev->CAMACaddr(N, i, 16);
            if (dev->CAMACwrite(dev, &addr, p[3])<0) {
                printf("C808threshold: CAMACwrite slot %d failed\n", N);
                return plErr_System;
            }
        }
    } else {
        camadr_t addr=dev->CAMACaddr(N, p[2], 16);
        if (dev->CAMACwrite(dev, &addr, p[3])<0) {
            printf("C808threshold: CAMACwrite slot %d failed\n", N);
            return plErr_System;
        }
    }
    return plOK;
}

plerrcode test_proc_C808threshold(ems_u32* p)
{
    ml_entry* module;
    ems_i32* ip=(ems_i32*)p;

    if (p[0]!=3)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac))
        return plErr_ArgRange;
    if (ip[2]>15 && ip[2]<-1)
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=CAEN_DISC_C808)
        return plErr_BadModTyp;
    wirbrauchen=0;
    return plOK;
}

char name_proc_C808threshold[]="C808threshold";
int ver_proc_C808threshold=1;
/*****************************************************************************/
/*
 * p[0]: No. of arguments
 * p[1]: module (index in memberlist or modullist)
 * p[2]: bitmask (channels to be enabled)
 */
plerrcode
proc_C808enable(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;

    camadr_t addr=dev->CAMACaddr(N, 0, 17);
    if (dev->CAMACwrite(dev, &addr, p[2])<0) {
        printf("C808enable: CAMACwrite slot %d failed\n", N);
        return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_C808enable(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=2)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=CAEN_DISC_C808)
        return plErr_BadModTyp;
    wirbrauchen=0;
    return plOK;
}

char name_proc_C808enable[]="C808enable";
int ver_proc_C808enable=1;
/*****************************************************************************/
/*
 * p[0]: No. of arguments
 * p[1]: module (index in memberlist or modullist)
 * p[2]: 1: channel 0..7, 2: channel 8..15, 3: all channels
 * p[3]: width
 */
plerrcode
proc_C808pulsewidth(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;

    if (p[2]&1) {
        camadr_t addr=dev->CAMACaddr(N, 0, 18);
        if (dev->CAMACwrite(dev, &addr, p[3])<0) {
            printf("C808pulsewidth: CAMACwrite slot %d failed\n", N);
            return plErr_System;
        }
    }
    if (p[2]&2) {
        camadr_t addr=dev->CAMACaddr(N, 1, 18);
        if (dev->CAMACwrite(dev, &addr, p[3])<0) {
            printf("C808pulsewidth: CAMACwrite slot %d failed\n", N);
            return plErr_System;
        }
    }

    return plOK;
}

plerrcode test_proc_C808pulsewidth(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=3)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac))
        return plErr_ArgRange;
    if (p[2]&~3)
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=CAEN_DISC_C808)
        return plErr_BadModTyp;
    wirbrauchen=0;
    return plOK;
}

char name_proc_C808pulsewidth[]="C808pulsewidth";
int ver_proc_C808pulsewidth=1;
/*****************************************************************************/
/*
 * p[0]: No. of arguments
 * p[1]: module (index in memberlist or modullist)
 * p[2]: 1: channel 0..7, 2: channel 8..15, 3: all channels
 * p[3]: width
 */
plerrcode
proc_C808deadtime(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;

    if (p[2]&1) {
        camadr_t addr=dev->CAMACaddr(N, 0, 19);
        if (dev->CAMACwrite(dev, &addr, p[3])<0) {
            printf("C808deadtime: CAMACwrite slot %d failed\n", N);
            return plErr_System;
        }
    }
    if (p[2]&2) {
        camadr_t addr=dev->CAMACaddr(N, 1, 19);
        if (dev->CAMACwrite(dev, &addr, p[3])<0) {
            printf("C808deadtime: CAMACwrite slot %d failed\n", N);
            return plErr_System;
        }
    }

    return plOK;
}

plerrcode test_proc_C808deadtime(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=3)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac))
        return plErr_ArgRange;
    if (p[2]&~3)
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=CAEN_DISC_C808)
        return plErr_BadModTyp;
    wirbrauchen=0;
    return plOK;
}

char name_proc_C808deadtime[]="C808deadtime";
int ver_proc_C808deadtime=1;
/*****************************************************************************/
/*
 * p[0]: No. of arguments
 * p[1]: module (index in memberlist or modullist)
 * p[2]: majority threshold (raw register value (0..255))
 */
plerrcode
proc_C808majthr(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;
    camadr_t addr;

    addr=dev->CAMACaddr(N, 0, 20);
    if (dev->CAMACwrite(dev, &addr, p[2])<0) {
        printf("C808majthr: CAMACwrite slot %d failed\n", N);
        return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_C808majthr(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=2)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=CAEN_DISC_C808)
        return plErr_BadModTyp;
    wirbrauchen=0;
    return plOK;
}

char name_proc_C808majthr[]="C808majthr";
int ver_proc_C808majthr=1;
/*****************************************************************************/
/*
 * p[0]: No. of arguments
 * p[1]: module (index in memberlist or modullist)
 */
plerrcode
proc_C808testpulse(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;
    ems_u32 stat;
    camadr_t addr;

    addr=dev->CAMACaddr(N, 0, 25);
    if (dev->CAMACcntl(dev, &addr, &stat)<0) {
        printf("C808testpulse: CAMACcntl slot %d failed\n", N);
        return plErr_System;
    }
    if (stat!=0xc0000000) {
        printf("C808testpulse: CAMACcntl slot %d: stat=0x%x\n", N, stat);
        return plErr_HW;
    }
    return plOK;
}

plerrcode test_proc_C808testpulse(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=1)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=CAEN_DISC_C808)
        return plErr_BadModTyp;
    wirbrauchen=0;
    return plOK;
}

char name_proc_C808testpulse[]="C808testpulse";
int ver_proc_C808testpulse=1;
/*****************************************************************************/
/*****************************************************************************/
