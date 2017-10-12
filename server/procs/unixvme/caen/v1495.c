/*
 * procs/unixvme/caen/v1495.c
 * created 2015-02-05 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL$";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <stdlib.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../objects/var/variables.h"
#include "../../../lowlevel/unixvme/vme.h"
#include "../../../lowlevel/devices.h"
#include "../../../trigger/trigger.h"
#include "../../procs.h"
#include "../vme_verify.h"

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

RCS_REGISTER(cvsid, "procs/unixvme/caen")

/*
 * V1495 GENERAL PURPOSE VME BOARD
 * A24/32 D16/32 (registers)
 * reserves 65536 Byte
 */
/* registers
 * 0x0000..0x0FFC: USER FPGA Block transfer
 * 0x1000..0x7ffc: USER FPGA Access
 * 0x8000..0x8006: reserved
 * 0x8008        : Geo Address_Register   (R)
 * 0x800a        : Module Reset           (W)
 * 0x800c        : Firmware revision      (R)
 * 0x800e        : Select VME FPGA Flash
 * 0x8010        : VME FPGA Flash memory
 * 0x8012        : Select USER FPGA Flash
 * 0x8014        : USER FPGA Flash memory
 * 0x8016        : USER FPGA Configuration
 * 0x8018        : Scratch16
 * 0x8020        : Scratch32               (D32 only)
 * 0x8100..0x81fe: Configuration ROM       (R)
 */

/*****************************************************************************/
static plerrcode
test_mod_type(ems_u32 idx)
{
    ems_u32 mtypes[]={CAEN_V1495, 0};
    ml_entry* module;

    if (memberlist) {
        if (idx>memberlist[0])
            return plErr_ArgRange;
        idx=memberlist[idx];
    }
    if (!modullist)
        return plErr_NoDomain;
    if (idx>=modullist->modnum)
        return plErr_BadISModList;

    module=modullist->entry+idx;
    return test_proc_vmemodule(module, mtypes);
}
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: module idx
 */
plerrcode proc_v1495reset(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
    u_int32_t v32;
    u_int16_t v16;
    int res;

    res=dev->write_a32d32(dev, addr+0x800a, 0);
    if (res!=4)
        return plErr_System;

    /* check access to scratch registers */
    res=dev->write_a32d16(dev, addr+0x8018, 0xa5a5);
    if (res!=2)
        return plErr_System;
    res=dev->read_a32d16(dev, addr+0x8018, &v16);
    if (res!=2)
        return plErr_System;
    if (v16!=0xa5a5)
        return plErr_HW;

    res=dev->write_a32d32(dev, addr+0x8020, 0x5a5aa5a5);
    if (res!=4)
        return plErr_System;
    res=dev->read_a32d32(dev, addr+0x8020, &v32);
    if (res!=4)
        return plErr_System;
    if (v32!=0x5a5aa5a5)
        return plErr_HW;

    return plOK;
}

plerrcode test_proc_v1495reset(ems_u32* p)
{
    plerrcode res;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((res=test_mod_type(p[1]))!=plOK)
        return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v1495reset[] = "v1495reset";
int ver_proc_v1495reset = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: module idx
 */
plerrcode proc_v1495ident(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
    u_int32_t v32;
    u_int16_t v16;
    int res;

    res=dev->read_a32d32(dev, addr+0x8008, &v32);
    if (res!=4)
        return plErr_System;
    printf("GEO: %08x\n", v32);

    res=dev->read_a32d32(dev, addr+0x800c, &v32);
    if (res!=4)
        return plErr_System;
    printf("firmware: %08x\n", v32);

    res=dev->read_a32d16(dev, addr+0x8124, &v16);
    if (res!=2)
        return plErr_System;
    printf("oui2: %04x\n", v16);
    res=dev->read_a32d16(dev, addr+0x8128, &v16);
    if (res!=2)
        return plErr_System;
    printf("oui1: %04x\n", v16);
    res=dev->read_a32d16(dev, addr+0x812c, &v16);
    if (res!=2)
        return plErr_System;
    printf("oui0: %04x\n", v16);
    res=dev->read_a32d16(dev, addr+0x8130, &v16);
    if (res!=2)
        return plErr_System;
    printf("version: %04x\n", v16);
    res=dev->read_a32d16(dev, addr+0x8134, &v16);
    if (res!=2)
        return plErr_System;
    printf("board2: %04x\n", v16);
    res=dev->read_a32d16(dev, addr+0x8138, &v16);
    if (res!=2)
        return plErr_System;
    printf("board1: %04x\n", v16);
    res=dev->read_a32d16(dev, addr+0x813c, &v16);
    if (res!=2)
        return plErr_System;
    printf("board0: %04x\n", v16);
    res=dev->read_a32d16(dev, addr+0x8180, &v16);
    if (res!=2)
        return plErr_System;
    printf("ser1: %04x\n", v16);
    res=dev->read_a32d16(dev, addr+0x8184, &v16);
    if (res!=2)
        return plErr_System;
    printf("ser0: %04x\n", v16);

    return plOK;
}

plerrcode test_proc_v1495ident(ems_u32* p)
{
    plerrcode res;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((res=test_mod_type(p[1]))!=plOK)
        return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v1495ident[] = "v1495ident";
int ver_proc_v1495ident = 1;
/*****************************************************************************/
/*****************************************************************************/
