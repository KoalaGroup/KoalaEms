/*
 * procs/unixvme/caen/v785.c
 * created 23.Sep.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: v785.c,v 1.5 2011/04/06 20:30:35 wuestner Exp $";

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
 * V785 32-channel peak sensing ADC
 * A32D16 (registers) A32D32/BLT32 (data buffer)
 * reserves 65536 Byte
 */
/*
 * 0x00..0x7FF: output buffer
 * 0x1000: firmware revision
 * 0x1002: geo address
 * 0x1004: MCST/CBLT address
 * 0x1006: bit set 1
 * 0x1008: bit clear 1
 * 0x100A: interrupt level
 * 0x100C: interrupt vector
 * 0x100E: status register 1
 * 0x1010: control register 1
 * 0x1012: ADER high
 * 0x1014: ADER low
 * 0x1016: single shot reset
 * 0x101A: MCST/CBLT ctrl
 * 0x1020: event trigger register
 * 0x1022: status register 2
 * 0x1024: event counter_l
 * 0x1026: event counter_h
 * 0x1028: increment event
 * 0x102A: increment offset
 * 0x102C: load test register
 * 0x102E: fat clear window
 * 0x1032: bit set 2
 * 0x1034: bit clear 2
 * 0x1036: memory write test address
 * 0x1038: memory test word high
 * 0x103A: memory test word low
 * 0x103C: crate select
 * 0x103E: test write event
 * 0x1040: event counter reset
 * 0x1064: memory test read address
 * 0x1068: sw comm
 * 0x106A: slide constant
 * 0x1070: AAD
 * 0x1072: BAD
 * 0x1080..0x10BF: thresholds
 * 0x8000..ffff: ROM 
 */

/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v785convert(ems_u32* p)
{
    int i, res;

    for (i=memberlist[0]; i>0; i--) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;

        /* write conversion pulse */
        res=dev->write_a32d16(dev, module->address.vme.addr+0x1068, 0);
        if (res!=2) return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_v785convert(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V785, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v785convert[] = "v785convert";
int ver_proc_v785convert = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v785reset(ems_u32* p)
{
    int i, res;

    for (i=memberlist[0]; i>0; i--) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;

        /* reset module */
        res=dev->write_a32d16(dev, addr+0x1006, 0x80);
        if (res!=2) return plErr_System;
        res=dev->write_a32d16(dev, addr+0x1008, 0x80);
        if (res!=2) return plErr_System;
        /* set some empty prog bit */
        res=dev->write_a32d16(dev, module->address.vme.addr+0x1032, 0x1000);
        if (res!=2) return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_v785reset(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V785, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v785reset[] = "v785reset";
int ver_proc_v785reset = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: bit set register 2
 */
/*
 *    0: mem test
 *    2: offline
 *    4: clear data
 *    8: over range prog
 *   10: low threshold prog
 *   40: test acq
 *   80: slide enable
 *  800: auto increment
 * 1000: empty prog
 * 2000: slide-sub enable
 * 4000: all trig
 */
plerrcode proc_v785bitset2(ems_u32* p)
{
    int i, res;

    for (i=memberlist[0]; i>0; i--) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;

        /* write bit set register 2 */
        res=dev->write_a32d16(dev, module->address.vme.addr+0x1032, p[1]);
        if (res!=2) return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_v785bitset2(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V785, 0};

    if ((unsigned)p[0]>1) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v785bitset2[] = "v785bitset2";
int ver_proc_v785bitset2 = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: bit res register 2
 */
plerrcode proc_v785bitres2(ems_u32* p)
{
    int i, res;

    for (i=memberlist[0]; i>0; i--) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;

        /* write bit res register 2 */
        res=dev->write_a32d16(dev, module->address.vme.addr+0x1034, p[1]);
        if (res!=2) return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_v785bitres2(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V785, 0};

    if ((unsigned)p[0]>1) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v785bitres2[] = "v785bitres2";
int ver_proc_v785bitres2 = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v785read(ems_u32* p)
{
    int i, res;

    *outptr++=memberlist[0];
    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;
        ems_u32 data, *help;
        int code, count, j;

        help=outptr++;

        /* read header */
        res=dev->read_a32d32(dev, addr, &data);
        if (res!=4) return plErr_System;
        code=(data>>24)&7;
        if (code==6) { /* buffer empty */
            /*
            *help=0;
            return plOK;
            */
            printf("v785read: buffer empty\n");
            *outptr++=0;
            *help=1;
            return plErr_HW;
        }
        if (code!=2) { /* no header */
            printf("v785read: not a header: 0x%08x\n", data);
            *outptr++=1;
            *outptr++=data;
            *help=2;
            return plErr_HW;
        }
        *outptr++=data;
        count=(data>>8)&0x3f;

        /* read data */
        for (j=0; j<count; j++) {
            res=dev->read_a32d32(dev, addr, &data);
            if (res!=4) return plErr_System;
            if (data&0x3c00000) {
                printf("v785read: not a data word: 0x%08x\n", data);
                *outptr++=2;
                *outptr++=data;
                *help=outptr-help-1;
                return plErr_HW;
            }
            *outptr++=data;
        }

        /* read EOB */
        res=dev->read_a32d32(dev, addr, &data);
        if (res!=4) return plErr_System;
        code=(data>>24)&7;
        if (code!=4) { /* no EOB */
            printf("v785read: not a EOB: 0x%08x\n", data);
            *outptr++=3;
            *outptr++=data;
            *help=outptr-help-1;
            return plErr_HW;
        }
        *outptr++=data;

        /* read invalid last word */
        res=dev->read_a32d32(dev, addr, &data);
        if (res!=4) return plErr_System;
        code=(data>>24)&7;
        if (code!=0x6) { /* no EOB */
            printf("v785read: not a invalid word: 0x%08x\n", data);
            *outptr++=4;
            *outptr++=data;
            *help=outptr-help-1;
            return plErr_HW;
        }
        *help=outptr-help-1;
    }

    return plOK;
}

plerrcode test_proc_v785read(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V785, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=memberlist[0]*34*32;
    return plOK;
}

char name_proc_v785read[] = "v785read";
int ver_proc_v785read = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v785readblock(ems_u32* p)
{
    int i, res;

    *outptr++=memberlist[0];
    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr;
        ems_u32 data, *help;
        int code, count;

        help=outptr++;

        /* read header */
        res=dev->read_a32d32(dev, addr, &data);
        if (res!=4) return plErr_System;
        code=(data>>24)&7;
        if (code==6) { /* buffer empty */
            /*
            *help=0;
            return plOK;
            */
            printf("v785read: buffer empty\n");
            *outptr++=0;
            *help=1;
            return plErr_HW;
        }
        if (code!=2) { /* no header */
            printf("v785read: not a header: 0x%08x\n", data);
            *outptr++=1;
            *outptr++=data;
            *help=2;
            return plErr_HW;
        }
        *outptr++=data;
        count=(data>>8)&0x3f;

        /* read data, EOB and invalid last word */
        res=dev->read_a32_fifo(dev, addr, outptr, (count+2)*4, 4, 0);
        if (res!=(count+2)*4) {
            printf("v785readblock: read_a32_fifo(%d): res=%d\n",
                    (count+2)*4, res);
            return plErr_System;
        }
        outptr+=count+1;
        code=(outptr[-1]>>24)&7;
        if (code!=4) { /* no EOB */
            printf("v785read: not a EOB: 0x%08x\n", data);
            *outptr++=3;
            *outptr++=data;
            *help=outptr-help-1;
            return plErr_HW;
        }
        code=(outptr[0]>>24)&7;
        if (code!=0x6) { /* no EOB */
            printf("v785read: not a invalid word: 0x%08x\n", data);
            *outptr++=4;
            *outptr++=data;
            *help=outptr-help-1;
            return plErr_HW;
        }
        *help=outptr-help-1;
    }

    return plOK;
}

plerrcode test_proc_v785readblock(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V785, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=memberlist[0]*34*32;
    return plOK;
}

char name_proc_v785readblock[] = "v785read";
int ver_proc_v785readblock = 2;
/*****************************************************************************/
/*
 * p[0]: argcount==memberlist[0]*32
 * p[1..32]: thresholds for module 1
 * p[n*32+1..(n+1)*32]: thresholds for module n
 * ...
 */
plerrcode proc_v785threshold(ems_u32* p)
{
    int i, res;

    p++;
    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 addr=module->address.vme.addr+0x1080;
        int j;

        /* write threshold values */
        for (j=32; j; j--, addr+=2) {
            res=dev->write_a32d16(dev, addr, *p++);
            if (res!=2) return plErr_System;
        }
    }
    return plOK;
}

plerrcode test_proc_v785threshold(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V785, 0};

    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;
    if (p[0]!=memberlist[0]*32) return plErr_ArgNum;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v785threshold[] = "v785threshold";
int ver_proc_v785threshold = 1;
/*****************************************************************************/
/*****************************************************************************/
