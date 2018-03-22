/*
 * procs/unixvme/caen/v560.c
 * created 11.Sep.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: v560.c,v 1.6 2017/10/20 23:20:52 wuestner Exp $";

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
 * V560 16-channel 32-bit scaler (100 MHz)
 * A32D16 (registers) A32D32 (counters)
 * reserves 256 Byte
 */
/*
 * 0x04: interrupt vector
 * 0x06: interrupt level and veto
 * 0x08: enable VME IRQ
 * 0x0A: disable VME IRQ
 * 0x0C: clear VME IRQ
 * 0x0E: request register
 * 0x10: counter 0
 * 0x14..0x48: counter 1..14
 * 0x4C: counter 15
 * 0x50: scale clear 
 * 0x52: veto set 
 * 0x54: veto reset
 * 0x56: scale increase
 * 0x58: scale status register
 * 0xFA: 0xFAF5 (const)
 * 0xFC: <15:10>Manufacturer(==0x2) <9:0>ModuleType(==0x18)
 * 0xFE: <15:12>Version <11:0>Serial No.
 */

/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v560clear(__attribute__((unused)) ems_u32* p)
{
    int i, res;

    for (i=memberlist[0]; i>0; i--) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;

        /* clear all */
        res=dev->write_a32d16(dev, module->address.vme.addr+0x50, 0);
        if (res!=2) return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_v560clear(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V560, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v560clear[] = "v560clear";
int ver_proc_v560clear = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v560read(__attribute__((unused)) ems_u32* p)
{
    int j, res;
    unsigned int i;

    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr+0x10;

        /* read all 16 channels */
        for (j=0; j<16; j++, addr+=4) {
            res=dev->read_a32d32(dev, addr, outptr++);
            if (res!=4) return plErr_System;
        }
    }
    return plOK;
}

plerrcode test_proc_v560read(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V560, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=memberlist[0]*16;
    return plOK;
}

char name_proc_v560read[] = "v560read";
int ver_proc_v560read = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v560status(__attribute__((unused)) ems_u32* p)
{
    unsigned int i;
    int res;

    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u16 data;

        /* read status register */
        res=dev->read_a32d16(dev, module->address.vme.addr+0x58, &data);
        if (res!=2) return plErr_System;
        *outptr++=data&0xff;
    }
    return plOK;
}

plerrcode test_proc_v560status(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V560, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=memberlist[0];
    return plOK;
}

char name_proc_v560status[] = "v560status";
int ver_proc_v560status = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v560incr(__attribute__((unused)) ems_u32* p)
{
    unsigned int i;
    int res;

    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;

        /* touch increment register */
        res=dev->write_a32d16(dev, module->address.vme.addr+0x56, 0);
        if (res!=2) return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_v560incr(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V560, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v560incr[] = "v560incr";
int ver_proc_v560incr = 1;
/*****************************************************************************/
/*****************************************************************************/
