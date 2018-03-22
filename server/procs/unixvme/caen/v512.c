/*
 * procs/unixvme/caen/v512.c
 * created 05.Sep.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: v512.c,v 1.6 2017/10/20 23:20:52 wuestner Exp $";

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
 * V512 8 channel 4 fold AND/OR programmable logic unit
 * A24/32D16
 * reserves 256 Byte
 */
/*
 * 0x00: IRQ vector
 * 0x02: IRQ level
 * 0x04: reserved
 * 0x06: IRQ enable
 * 0x08..0x0E: reserved
 * 0x10: function
 * 0x12..0xF8: reserved
 * 0xFA: 0xFAF5 (const)
 * 0xFC: <15:10>Manufacturer(==0x2) <9:0>ModuleType(==0x1A)
 * 0xFE: <15:12>Version <11:0>Serial No.
 */

/*****************************************************************************/
/*
 * p[0]: argcount==number of modules
 * p[1..]: functions
 */
plerrcode proc_v512init(ems_u32* p)
{
    unsigned int i;
    int res;

    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;

        /* disable IRQ */
        res=dev->write_a32d16(dev, addr+0x06, 0);
        if (res!=2) {
            *outptr++=i;
            return plErr_System;
        }
        /* write function code */
        res=dev->write_a32d16(dev, addr+0x10, p[i]);
        if (res!=2) {
            *outptr++=i;
            return plErr_System;
        }
    }
    return plOK;
}

plerrcode test_proc_v512init(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V512, 0};

    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;
    if (p[0]!=memberlist[0]) return plErr_ArgNum;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v512init[] = "v512init";
int ver_proc_v512init = 1;
/*****************************************************************************/
/*****************************************************************************/
