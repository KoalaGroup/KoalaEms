/*
 * procs/unixvme/caen/v556.c
 * created 07.Oct.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: v556.c,v 1.9 2017/10/20 23:20:52 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/unixvme/vme.h"
#include "../../../lowlevel/devices.h"
#include "../vme_verify.h"

extern ems_u32* outptr;
extern unsigned int *memberlist;

RCS_REGISTER(cvsid, "procs/unixvme/caen")

/*
 * V556 8-channel peak sensing ADC
 * A32D16
 * reserves 256 Byte
 */
/*
 * 0x00: IRQ Register
 * 0x02..0xE unused
 * 0x10: Lower Threshold
 * 0x12: Upper Threshold
 * 0x14: Delay
 * 0x16: FF
 * 0x18: Output Buffer
 * 0x1A: Control
 * 0x1C: Reset
 * 0x1E: HF
 * 0x20..0xF8: unused
 * 0xFA: 0xFAF5 (const)
 * 0xFC: <15:10>Manufacturer(==0x2) <9:0>ModuleType(==0x36)
 * 0xFE: <15:12>Version <11:0>Serial No.
 */
/*
 * Control:
 * <0..7> Enable channel n
 * <12>   /FIFO half full
 * <13>   /FIFO full
 * <14>   /FIFO empty
 * <15>   /fast clear selection
 */
/*****************************************************************************/
/*
 * p[0]: argcount==0
 * p[1]: channel mask    module 1
 * p[2]: lower threshold module 1
 * p[3]: upper threshold module 1
 * p[4]: channel mask    module 2
 * p[5]: lower threshold module 2
 * p[6]: upper threshold module 2
 * ...
 */
plerrcode proc_v556init(ems_u32* p)
{
    int i, res;

    p++; /* skip argcount */
    for (i=memberlist[0]; i>0; i--) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 base=module->address.vme.addr;

        /* reset module */
        res=dev->write_a32d16(dev, base+0x1c, 0);
        if (res!=2) return plErr_System;
        /* reset IRQ vector/level */
        res=dev->write_a32d16(dev, base+0x0, 0);
        if (res!=2) return plErr_System;
        /* set FIFO half full mode */
        res=dev->write_a32d16(dev, base+0x1e, 0);
        if (res!=2) return plErr_System;
        /* enable channels (and disable fast clear) */
        res=dev->write_a32d16(dev, base+0x1a, *p++);
        if (res!=2) return plErr_System;
        /* set lower threshold */
        res=dev->write_a32d16(dev, base+0x10, *p++);
        if (res!=2) return plErr_System;
        /* set upper threshold */
        res=dev->write_a32d16(dev, base+0x12, *p++);
        if (res!=2) return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_v556init(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V556, 0};
    unsigned int i, j;

    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;
    if (p[0]!=3*memberlist[0]) return plErr_ArgNum;
    for (i=0, j=1; i<memberlist[0]; i++, j+=3) {
        if (p[j+0]&~0xff) {*outptr++=j+0; return plErr_ArgRange;}
        if (p[j+1]&~0xff) {*outptr++=j+1; return plErr_ArgRange;}
        if (p[j+2]&~0xff) {*outptr++=j+2; return plErr_ArgRange;}
    }
    wirbrauchen=0;
    return plOK;
}

char name_proc_v556init[] = "v556init";
int ver_proc_v556init = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v556read(__attribute__((unused)) ems_u32* p)
{
    int res, n;
    ems_u32* help;
    unsigned int i;

/* Readout can take place at least 13 us after trigger. This is not enforced
   inside this procedure */

    help=outptr++; n=0;
    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 base=module->address.vme.addr;
        ems_u16 v;
        int mult, j;

        /* fifo empty? */
        res=dev->read_a32d16(dev, base+0x1a, &v);
        if (res!=2) return plErr_System;
        if (!(v&0x4000)) continue;
        *outptr++=i-1;
        n++;

        /* read header*/
        res=dev->read_a32d16(dev, base+0x18, &v);
        if (res!=2) return plErr_System;
        if (!(v&0x8000)) {
            printf("v556read: 0x%04x is not a header (member %d)\n", v, i);
            return plErr_HW;
        }
        *outptr++=v;

        mult=(v>>12)&7; /* number of channels; event counter is not checked */
        for (j=0; j<mult; j++) {
            /* fifo still not empty? */
            res=dev->read_a32d16(dev, base+0x1a, &v);
            if (res!=2) return plErr_System;
            if (!(v&0x4000)) {
                printf("v556read: (member %d) mult=%d FIFO empty at word %d\n",
                    i, mult, j);
                return plErr_HW;
            }
            /* read data word */
            res=dev->read_a32d16(dev, base+0x18, &v);
            if (res!=2) return plErr_System;
            if (v&0x8000) {
                printf("v556read: 0x%04x is not a channel data (member %d)\n",
                        v, i);
                return plErr_HW;
            }
            *outptr++=v;
        }
    }
    *help=n;

    return plOK;
}

plerrcode test_proc_v556read(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V556, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=memberlist[0]*10+1;
    return plOK;
}

char name_proc_v556read[] = "v556read";
int ver_proc_v556read = 1;
/*****************************************************************************/
/*****************************************************************************/
