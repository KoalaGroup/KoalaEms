/*
 * procs/unixvme/caen/v462.c
 * created 10.Oct.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: v462.c,v 1.4 2011/04/06 20:30:35 wuestner Exp $";

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
extern int wirbrauchen;
extern int *memberlist;

RCS_REGISTER(cvsid, "procs/unixvme/caen")

/*
 * V462 Dual Gate Generator
 * A24D16
 * reserves 256 Byte
 */
/*
 * 0x00: Status/Start
 * 0x02: MSB Ch0
 * 0x04: LSB Ch0
 * 0x06: MSB Ch1
 * 0x08: LSB Ch1
 * 0x0A..0xF8: unused
 * 0xFA: 0xFAF5 (const)
 * 0xFC: <15:10>Manufacturer(==0x2) <9:0>ModuleType(==0xa)
 * 0xFE: <15:12>Version <11:0>Serial No.
 */
/*
 * Status/Start:
 * <0..8> unused
 * <9>    Start 0     (write only)
 * <10>   Start 1     (write only)
 * <11>   Gate 0      (read only)
 * <12>   Gate 1      (read only)
 * <13>   VME/Local 0 (read only)
 * <14>   VME/Local 1 (read only)
 * <15>   Error       (read only)
 */
/*****************************************************************************/
/*
 * p[0]: argcount=(2*memberlist[0])
 * p[1]: gate length for channel 0 module 1
 * p[2]: gate length for channel 1 module 1
 * p[3]: gate length for channel 0 module 2
 * p[4]: gate length for channel 1 module 2
 * ...
 */
plerrcode proc_v462init(ems_u32* p)
{
    int i, res;

    p++; /* skip argcount */
    for (i=memberlist[0]; i>0; i--) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 base=module->address.vme.addr;

        res=dev->write_a24d16(dev, base+0x2, (*p>>16)&0xffff);
        if (res!=2) return plErr_System;
        res=dev->write_a24d16(dev, base+0x4, *p&0xffff);
        if (res!=2) return plErr_System;
        p++;

        res=dev->write_a24d16(dev, base+0x6, (*p>>16)&0xffff);
        if (res!=2) return plErr_System;
        res=dev->write_a24d16(dev, base+0x8, *p&0xffff);
        if (res!=2) return plErr_System;
        p++;
    }
    return plOK;
}

plerrcode test_proc_v462init(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V462, 0};

    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;
    if (p[0]!=memberlist[0]*2) return plErr_ArgNum;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v462init[] = "v462init";
int ver_proc_v462init = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v462status(ems_u32* p)
{
    int i, res;

    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u16 v;

        res=dev->read_a24d16(dev, module->address.vme.addr, &v);
        if (res!=2) return plErr_System;
        *outptr++=v;
    }
    return plOK;
}

plerrcode test_proc_v462status(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V462, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=memberlist[0]*2;
    return plOK;
}

char name_proc_v462status[] = "v462status";
int ver_proc_v462status = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==memberlist[0]
 * p[1]: start module 1 (0: start none 1: start ch0 2: start ch1 3: start both)
 * p[2]: start module 2
 */
plerrcode proc_v462trigger(ems_u32* p)
{
    int i, res;

    p++; /* skip argcount */
    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;

        if (*p) {
            res=dev->write_a24d16(dev, module->address.vme.addr, (*p&3)<<9);
            if (res!=2) return plErr_System;
        }
        p++;
    }
    return plOK;
}

plerrcode test_proc_v462trigger(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V462, 0};

    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;
    if (p[0]!=memberlist[0]) return plErr_ArgNum;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v462trigger[] = "v462trigger";
int ver_proc_v462trigger = 1;
/*****************************************************************************/
/*****************************************************************************/
