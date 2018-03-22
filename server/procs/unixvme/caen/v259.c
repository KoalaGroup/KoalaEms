/*
 * procs/unixvme/caen/v259.c
 * created 05.Sep.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: v259.c,v 1.6 2017/10/20 23:20:52 wuestner Exp $";

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
 * V259 16 Bit strobed multi hit pattern unit
 * A24D16
 * reserves 256 Byte
 */
/*
 * 0x00..0x1E: unused
 * 0x20: Reset all registers
 * 0x22: read and reset pattern register
 * 0x24: read and reset multiplicity register
 * 0x26: read pattern register
 * 0x28: read multiplicity register
 * 0x2A..0xF8: unused
 * 0xFA: 0xFAF5 (const)
 * 0xFC: <15:10>Manufacturer(==0x2) <9:0>ModuleType(==0x4)
 * 0xFE: <15:12>Version <11:0>Serial No.
 */

/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v259init(__attribute__((unused)) ems_u32* p)
{
    int i, res;

    for (i=memberlist[0]; i>0; i--) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;

        /* reset module */
        res=dev->write_a24d16(dev, module->address.vme.addr+0x20, 0);
        if (res!=2) return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_v259init(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V259, CAEN_V259E, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v259init[] = "v259init";
int ver_proc_v259init = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: read multiplicity also
 */
plerrcode proc_v259read(ems_u32* p)
{
    unsigned int i;
    int res;

    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u16 data;

        /* read data */
        res=dev->read_a24d16(dev, module->address.vme.addr+0x22, &data);
        if (res!=2) return plErr_System;
        *outptr++=data;
        if (p[1]) {/* read multiplicity */
            res=dev->read_a24d16(dev, module->address.vme.addr+0x24, &data);
            if (res!=2) return plErr_System;
            *outptr++=data;
        }
    }
    return plOK;
}

plerrcode test_proc_v259read(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V259, CAEN_V259E, 0};

    if (p[0]!=1) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;
    if ((unsigned int)p[1]>1) return plErr_ArgRange;

    wirbrauchen=memberlist[0];
    if (p[1]) wirbrauchen*=2;
    return plOK;
}

char name_proc_v259read[] = "v259read";
int ver_proc_v259read = 1;
/*****************************************************************************/
/*****************************************************************************/
