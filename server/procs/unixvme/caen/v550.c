/*
 * procs/unixvme/caen/v550.c
 * created 27.Jun.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: v550.c,v 1.7 2017/10/20 23:20:52 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
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
 * V550 2-channel C-RAMS
 * A32D16 (registers) A32D32 (FIFOs and pedestal/threshold memory)
 * reserves 64 KByte
 */
/*
 * 0x00: interrupt register
 * 0x02: status register
 * 0x04: number of channels
 * 0x06: module clear
 * 0x08: FIFO channel 0
 * 0x0C: FIFO channel 1
 * 0x10: word count channel 0
 * 0x12: word count channel 1
 * 0x14: test pattern channel 0
 * 0x14: test pattern channel 1
 * 0xFA: 0xFAF5 (const)
 * 0xFC: <15:10>Manufacturer(==0x2) <9:0>ModuleType(==0x34)
 * 0xFE: <15:12>Version <11:0>Serial No.
 * 0x2000..3FFC: pedestal/threshold memory channel 0
 * 0x4000..5FFC: pedestal/threshold memory channel 1
 */

/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v550clear(__attribute__((unused)) ems_u32* p)
{
    int m, res;

    for (m=memberlist[0]; m>0; m--) {
        ml_entry* module=ModulEnt(m);
        struct vme_dev* dev=module->address.vme.dev;

        /* clear all */
        res=dev->write_a32d16(dev, module->address.vme.addr+0x6, 0);
        if (res!=2) return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_v550clear(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V550, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v550clear[] = "v550clear";
int ver_proc_v550clear = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v550read(__attribute__((unused)) ems_u32* p)
{
    unsigned int m;
    int res;
    ems_u16 wordcount;

    for (m=1; m<=memberlist[0]; m++) {
        ml_entry* module=ModulEnt(m);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;

        res=dev->read_a32d16(dev, addr+0x10, &wordcount);
        if (res!=2) return plErr_System;
        wordcount&=0x7ff;
        *outptr++=wordcount;
        printf("wordcount0=0x%x\n", wordcount);
        res=dev->read_a32_fifo(dev, addr+0x8, outptr, wordcount<<2, 4, &outptr);
        if (res<0) {
            printf("v550read: errno=%s\n", strerror(errno));
            return plErr_System;
        }
        if (res!=wordcount<<2) {
            printf("v550read: res=%d (not %d)\n", res, wordcount<<2);
            return plErr_System;
        }

        res=dev->read_a32d16(dev, addr+0x12, &wordcount);
        if (res!=2) return plErr_System;
        wordcount&=0x7ff;
        *outptr++=wordcount;
        res=dev->read_a32_fifo(dev, addr+0xc, outptr, wordcount<<2, 4, &outptr);
        if (res<0) {
            printf("v550read: errno=%s\n", strerror(errno));
            return plErr_System;
        }
        if (res!=wordcount<<2) {
            printf("v550read: res=%d (not %d)\n", res, wordcount<<2);
            return plErr_System;
        }
    }
    return plOK;
}

plerrcode test_proc_v550read(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V550, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=memberlist[0]*4096*2;
    return plOK;
}

char name_proc_v550read[] = "v550read";
int ver_proc_v550read = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v550status(__attribute__((unused)) ems_u32* p)
{
    unsigned int m;
    int res;

    for (m=1; m<=memberlist[0]; m++) {
        ml_entry* module=ModulEnt(m);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u16 data;

        /* read status register */
        res=dev->read_a32d16(dev, module->address.vme.addr+0x2, &data);
        if (res!=2) return plErr_System;
        *outptr++=data&0x3ff;
    }
    return plOK;
}

plerrcode test_proc_v550status(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V550, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=memberlist[0];
    return plOK;
}

char name_proc_v550status[] = "v550status";
int ver_proc_v550status = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v550init(__attribute__((unused)) ems_u32* p)
{
    unsigned int m;

    for (m=1; m<=memberlist[0]; m++) {
        ml_entry* module=ModulEnt(m);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;
        int i;

        /* clear */
        dev->write_a32d16(dev, addr+0x6, 0);

        /* set VME memory owner */
        dev->write_a32d16(dev, addr+0x2, 0x0);

        /* load pedestal memories with 0's */
        for (i=0; i<2048; i++) {
            dev->write_a32d32(dev, addr+0x2000+4*i, 0x0);
        }
        for (i=0; i<2048; i++) {
            dev->write_a32d32(dev, addr+0x4000+4*i, 0x0);
        }

        /* set conversion logic  memory owner +test mode */
        dev->write_a32d16(dev, addr+0x2, 0x3);

        /* set number of channels */
        /* 32 channels for both ADC channels */
        dev->write_a32d16(dev, addr+4, 0x41);
        /* 1 channel for both ADC channels */
        /*dev->write_a32d16(dev, addr+4, 0x0);*/

    }
    return plOK;
}

plerrcode test_proc_v550init(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V550, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v550init[] = "v550init";
int ver_proc_v550init = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v550fill(__attribute__((unused)) ems_u32* p)
{
    unsigned int m;

    for (m=1; m<=memberlist[0]; m++) {
        ml_entry* module=ModulEnt(m);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;
        int i;

        if (m==2) continue;
        /* Testmode, owner=conv. logic */
        dev->write_a32d16(dev, addr+2, 0x3);
        for (i=0; i<5; i++) {
            u_int16_t pattern;
            /* simulate hits */
            pattern=0x1000|(m<<10);
            if (m!=3) dev->write_a32d16(dev, addr+0x14, pattern|(0<<8)|(i)); 
            dev->write_a32d16(dev, addr+0x16, pattern|(1<<8)|(i));
        }
        if (m==4) {
            u_int16_t pattern=0x1111;
            for (i=0; i<100; i++) {
                dev->write_a32d16(dev, addr+0x14, pattern); 
            }
        }
        /* no Testmode, owner=conv. logic */
        dev->write_a32d16(dev, addr+2, 0x2);
    }
    return plOK;
}

plerrcode test_proc_v550fill(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V550, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=memberlist[0]*4096*2;
    return plOK;
}

char name_proc_v550fill[] = "v550fill";
int ver_proc_v550fill = 1;
/*****************************************************************************/
/*****************************************************************************/
