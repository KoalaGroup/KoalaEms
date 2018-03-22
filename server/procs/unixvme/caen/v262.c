/*
 * procs/unixvme/caen/v262.c
 * created 11.Sep.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: v262.c,v 1.6 2017/10/20 23:20:52 wuestner Exp $";

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
 * V262 multipurpose i/o register
 * A24D16
 * reserves 256 Byte
 */
/*
 * 0x04: write ECL level register
 * 0x06: write NIM level register
 * 0x08: write NIM pulse register
 * 0x0A: read NIM level register
 * 0xFA: 0xFAF5 (const)
 * 0xFC: <15:10>Manufacturer(==0x2) <9:0>ModuleType(==0x1)
 * 0xFE: <15:12>Version <11:0>Serial No.
 */

/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v262clear(__attribute__((unused)) ems_u32* p)
{
    int i, res;

    for (i=memberlist[0]; i>0; i--) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;

        /* clear all outputs */
        res=dev->write_a24d16(dev, module->address.vme.addr+0x04, 0);
        if (res!=2) return plErr_System;
        res=dev->write_a24d16(dev, module->address.vme.addr+0x06, 0);
        if (res!=2) return plErr_System;
        res=dev->write_a24d16(dev, module->address.vme.addr+0x08, 0);
        if (res!=2) return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_v262clear(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V262, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v262clear[] = "v262clear";
int ver_proc_v262clear = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v262read(__attribute__((unused)) ems_u32* p)
{
    unsigned int i;
    int res;

    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u16 data;

        /* read data */
        res=dev->read_a24d16(dev, module->address.vme.addr+0x0a, &data);
        if (res!=2) return plErr_System;
        *outptr++=data&0xf;
    }
    return plOK;
}

plerrcode test_proc_v262read(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V262, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=memberlist[0];
    return plOK;
}

char name_proc_v262read[] = "v262read";
int ver_proc_v262read = 1;
/*****************************************************************************/
/*****************************************************************************/
