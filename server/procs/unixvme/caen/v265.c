/*
 * procs/unixvme/caen/v265.c
 * created 07.Oct.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: v265.c,v 1.5 2011/04/06 20:30:35 wuestner Exp $";

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
 * V265 8-channel charge integrating ADC
 * A24D16
 * reserves 256 Byte
 */
/*
 * 0x00: Status/Control
 * 0x02: Clear
 * 0x04: DAC (test logic)
 * 0x06: Gate Generation
 * 0x08: Data
 * 0x0A..0xF8: unused
 * 0xFA: 0xFAF5 (const)
 * 0xFC: <15:10>Manufacturer(==0x2) <9:0>ModuleType(==0x12)
 * 0xFE: <15:12>Version (0:NIM 1:ECL) <11:0>Serial No.
 */
/*
 * Status/Control:
 * <0..7>    IQ vector
 * <8..10>   IRQ level (0: disabled)
 * <11..13>  unused
 * <14> FIFO full
 * <15> RDY  (==FIFO not empty)
 */
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v265init(ems_u32* p)
{
    int i, res;

    for (i=memberlist[0]; i>0; i--) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 base=module->address.vme.addr;

        /* reset module */
        res=dev->write_a24d16(dev, base+0x2, 0);
        if (res!=2) return plErr_System;
        /* reset IRQ vector/level */
        res=dev->write_a24d16(dev, base+0x0, 0);
        if (res!=2) return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_v265init(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V265, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v265init[] = "v265init";
int ver_proc_v265init = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v265read(ems_u32* p)
{
    int i, res;

    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 base=module->address.vme.addr;
        int j;

        /* read data */

        for (j=0; j<16; j++) {
            ems_u16 v;
            do { /* it will take approximately 300 us for the first value :-( */
                res=dev->read_a24d16(dev, base+0x0, &v);
                if (res!=2) return plErr_System;
            } while (!(v&0x8000)); /* fifo empty */
            res=dev->read_a24d16(dev, base+0x8, &v);
            if (res!=2) return plErr_System;
            *outptr++=v;
        }
    }
    return plOK;
}

plerrcode test_proc_v265read(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V265, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=memberlist[0]*16;
    return plOK;
}

char name_proc_v265read[] = "v265read";
int ver_proc_v265read = 1;
/*****************************************************************************/
/*****************************************************************************/
