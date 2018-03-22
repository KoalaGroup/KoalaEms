/*
 * procs/unixvme/caen/v830.c
 * created 10.Oct.2002 p.Kulessa
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: v830.c,v 1.6 2017/10/20 23:20:52 wuestner Exp $";

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
 * V830 32-channel 32-bit scaler (250 MHz)
 * A32D16 (registers) A32D32 (counters)
 * reserves 256 Byte
 */
/*
 * 0x0000-0x0FFC : MEB
 * 0x1000: counter 0
 * 0x1004..0x1078: counter 1..31
 * 0x107c: counter 32
 * ...
 * ...
 * 0x1108 Control register
 * ...
 * ...
 * 0x110e Status register
 * ...
 * ...
 * 0x1120 Software reset register
 * 0x1122 Software clear register
 * ...
 * ...
 * 0x4000 Configuration ROM
 * 0x1132 Firmware 
 */

/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v830clear(__attribute__((unused)) ems_u32* p)
{
    int i, res;

    for (i=memberlist[0]; i>0; i--) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;

        /* clear all */
        res=dev->write_a32d16(dev, module->address.vme.addr+0x1122, 0);
        if (res!=2) return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_v830clear(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V830, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v830clear[] = "v830clear";
int ver_proc_v830clear = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v830reset(__attribute__((unused)) ems_u32* p)
{
    int i, res;

    for (i=memberlist[0]; i>0; i--) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;

        /* clear all */
        res=dev->write_a32d16(dev, module->address.vme.addr+0x1120, 0);
        if (res!=2) return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_v830reset(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V830, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v830reset[] = "v830reset";
int ver_proc_v830reset = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v830read(__attribute__((unused)) ems_u32* p)
{
    unsigned int i, j;
    int res;

    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr+0x1000;

        /* read all 32 channels */
        for (j=0; j<32; j++, addr+=4) {
            res=dev->read_a32d32(dev, addr, outptr++);
            if (res!=4) return plErr_System;
        }
    }
    return plOK;
}

plerrcode test_proc_v830read(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V830, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=memberlist[0]*32;
    return plOK;
}

char name_proc_v830read[] = "v830read";
int ver_proc_v830read = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v830status(__attribute__((unused)) ems_u32* p)
{
    unsigned int i;
    int res;

    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u16 data;

        /* read status register */
        res=dev->read_a32d16(dev, module->address.vme.addr+0x110e, &data);
        if (res!=2) return plErr_System;
        *outptr++=data&0xff;
    }
    return plOK;
}

plerrcode test_proc_v830status(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V830, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=memberlist[0];
    return plOK;
}

char name_proc_v830status[] = "v830status";
int ver_proc_v830status = 1;
/*****************************************************************************/
/*****************************************************************************/
