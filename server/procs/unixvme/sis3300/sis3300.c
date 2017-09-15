/*
 * procs/unixvme/sis3300/sis3300.c
 * created 2005-Sep-05 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3300.c,v 1.8 2011/04/06 20:30:35 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
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

RCS_REGISTER(cvsid, "procs/unixvme/sis3100")

/*
 * SIS3300 100 MHz FADC
 * A32D32/BLT32/MBLT64
 * occupies 16777216 Byte of address space
 */
/*
 * 0x0000: Control Register(write), Status Register(read)
 * 0x0004: Module ID and firmware revision
 * 0x0008: Interrupt configuration register
 * 0x000c: Interrupt control register
 * 0x0010: Acquisition control/status register 
 * 0x0014: Extern start delay register
 * 0x0018: Extern stop delay register
 * 0x0020: General reset
 * 0x0030: VME start sampling
 * 0x0034: VME sto sampling
 * 0x00001000: Event time stamp directory bank 1 (0x1000 bytes)
 * 0x00002000: Event time stamp directory bank 1 (0x1000 bytes)
 * 0x00100000: Event configuration register (w/o)
 * 0x00100020: Clock provider register (w/o)
 * 0x00100024: No_Of_Sample register (w/o)
 * Event information ADC group 1 (ADC1 and ADC2)
 * 0x00200000: Event configuration register
 * 0x00200004: Threshold register
 * 0x00200008: Bank 1 address counter
 * 0x0020000c: Bank 2 address counter
 * 0x00200010: Bank 1 event counter
 * 0x00200014: Bank 2 event counter
 * 0x00200020: Clock predivider register
 * 0x00200024: No_Of_Sample register
 * 0x00201000: Event directory bank 1 (0x1000 bytes)
 * 0x00202000: Event directory bank 1 (0x1000 bytes)
 *  Event information ADC group 2 (ADC3 and ADC4)
 * 0x00280000..0x00282000: identical to ADC group 1
 *  Event information ADC group 3 (ADC5 and ADC6)
 * 0x00300000..0x00302000: identical to ADC group 1
 *  Event information ADC group 4 (ADC7 and ADC8)
 * 0x00380000..0x00382000: identical to ADC group 1
 *   (Event information in all four groups is identical (unless the module has a
 *    hardware problem))
 *  Bank 1 memory
 * 0x00400000: Bank 1 memory (ADC1,2) (0x80000 bytes)
 * 0x00480000: Bank 1 memory (ADC3,4) (0x80000 bytes)
 * 0x00500000: Bank 1 memory (ADC5,6) (0x80000 bytes)
 * 0x00580000: Bank 1 memory (ADC7,8) (0x80000 bytes)
 *  Bank 2 memory
 * 0x00600000: Bank 2 memory (ADC1,2) (0x80000 bytes)
 * 0x00680000: Bank 2 memory (ADC3,4) (0x80000 bytes)
 * 0x00700000: Bank 2 memory (ADC5,6) (0x80000 bytes)
 * 0x00780000: Bank 2 memory (ADC7,8) (0x80000 bytes)
 */
/*
 * !!! This code deals only with memory bank 1!!!
 * (I don't have boards stuffed with two banks.) 
 */
/*****************************************************************************/
static plerrcode
sis3300_rset_bits(ml_entry* module, ems_u32 reg, int nr_bits,
    int set_shift, int res_shift, ems_u32 value)
{
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
    ems_u32 mask, xval;

    mask=0xffffffffUL>>(32-nr_bits);
    value&=mask;
    xval=(((value&mask)<<set_shift)
        | ((~value&mask)<<res_shift));
    if (dev->write_a32d32(dev, addr+reg, xval)!=4) {
        printf("sis3300_rset_bits(0x%08x, x%08x) failed: %s\n",
                addr+reg, xval, strerror(errno));
        return plErr_System;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
sis3300_write(ml_entry* module, ems_u32 reg, ems_u32 value)
{
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;

    if (dev->write_a32d32(dev, addr+reg, value)!=4) {
        printf("sis3300_write(0x%08x, x%08x) failed: %s\n",
                addr+reg, value, strerror(errno));
        return plErr_System;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
sis3300_read(ml_entry* module, ems_u32 reg, ems_u32* value)
{
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;

    if (dev->read_a32d32(dev, addr+reg, value)!=4) {
        printf("sis3300_read(0x%08x) failed: %s\n",
                addr+reg, strerror(errno));
        return plErr_System;
    }
    return plOK;
}
/*****************************************************************************/
/*
 * p[0]: argcount(==1)
 * p[1]: index in memberlist
 */
plerrcode proc_sis3300stat(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
    ems_u32 val;

    dev->read_a32d32(dev, addr+0x0, &val);
    printf("[ 0] status: 0x%08x\n", val);

    dev->read_a32d32(dev, addr+0x8, &val);
    printf("[ 8] irq conf: 0x%08x\n", val);

    dev->read_a32d32(dev, addr+0xc, &val);
    printf("[ c] irq contr: 0x%08x\n", val);

    dev->read_a32d32(dev, addr+0x10, &val);
    printf("[10] acq contr: 0x%08x\n", val);

    dev->read_a32d32(dev, addr+0x200000, &val);
    printf("[200000] event conf: 0x%08x\n", val);

    dev->read_a32d32(dev, addr+0x200008, &val);
    printf("[200008] bank1 addr: 0x%08x\n", val);

    dev->read_a32d32(dev, addr+0x20000c, &val);
    printf("[20000c] bank2 addr: 0x%08x\n", val);

    dev->read_a32d32(dev, addr+0x200010, &val);
    printf("[200010] bank1 event: 0x%08x\n", val);

    dev->read_a32d32(dev, addr+0x200014, &val);
    printf("[200014] bank2 event: 0x%08x\n", val);

    dev->read_a32d32(dev, addr+0x200024, &val);
    printf("[200024] no. of sample: 0x%08x\n", val);

    return plOK;
}

plerrcode test_proc_sis3300stat(ems_u32* p)
{
    ml_entry* module;
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=SIS_3300)
        return plErr_BadModTyp;
    if ((pres=verify_vme_module(module, 0))!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_sis3300stat[] = "sis3300stat";
int ver_proc_sis3300stat = 1;
/*****************************************************************************/
/*
 * p[0]: argcount(==3)
 * p[1]: index in memberlist
 * p[2]: clock source (bit 12..14 of acquisition control register)
 * p[3]: start/stop logic (bit 8..10 of acquisition control register)
 */
plerrcode proc_sis3300init_single(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    /*struct vme_dev* dev=module->address.vme.dev;*/
    /*ems_u32 addr=module->address.vme.addr;*/
    plerrcode pres;

    /* reset module */
    if ((pres=sis3300_write(module, 0x20, 0))!=plOK)
        return pres;

    /* set clock source */
    if ((pres=sis3300_rset_bits(module, 0x10, 3, 12, 12+16, p[2]))!=plOK)
            return pres;

    /* set start/stop logic */
    if ((pres=sis3300_rset_bits(module, 0x10, 3, 8, 8+16, p[3]))!=plOK)
            return pres;

    /* set some other things in acquisition control register ?? */

    /* switch LED on */
    sis3300_write(module, 0x0, 0x1 /*0x10000: off*/);

    return plOK;
}

plerrcode test_proc_sis3300init_single(ems_u32* p)
{
    ml_entry* module;
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=SIS_3300)
        return plErr_BadModTyp;
    if ((pres=verify_vme_module(module, 0))!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_sis3300init_single[] = "sis3300init_single";
int ver_proc_sis3300init_single = 1;
/*****************************************************************************/
/*
 * p[0]: argcount(==3)
 * p[1]: index in memberlist
 * p[2]: bitmask for channels
 * p[3]: number of samples
 */
plerrcode proc_sis3300sample(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
    ems_u32 stat;

    /* arm for sampling (start clock) */
    dev->write_a32d32(dev, addr+0x10, 1);

    /* start sampling */
    dev->write_a32d32(dev, addr+0x30, 0);

    /* wait for automatic end of sampling */
    do {
        dev->read_a32d32(dev, addr+0x0, &stat);
    } while (stat&0x20);

    /* read data */
    if (p[3]>0x80000) p[3]=0x80000;
    dev->read_a32(dev, addr+0x400000, outptr, p[3]*4, 4, &outptr);

    return plOK;
}

plerrcode test_proc_sis3300sample(ems_u32* p)
{
    ml_entry* module;
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=SIS_3300)
        return plErr_BadModTyp;
    if ((pres=verify_vme_module(module, 0))!=plOK)
        return pres;

    wirbrauchen=p[3];
    return plOK;
}

char name_proc_sis3300sample[] = "sis3300sample";
int ver_proc_sis3300sample = 1;
/*****************************************************************************/
/*
 * p[0]: argcount(==4)
 * p[1]: index in memberlist
 * p[2]: irq source (0 if interrupts not used)
 * p[3]: group mask
 * p[4]: no. of samples
 */
plerrcode proc_sis3300read_single(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
    ems_u32 conf, *help;
    int res, i;
    plerrcode pres=plOK;

    /* disable IRQ */
    if (p[2]) {
        if ((pres=sis3300_write(module, 0xc, p[2]<<16))!=plOK)
            return pres;
        if ((pres=sis3300_read(module, 0x8, &conf))!=plOK)
            return pres;
        if ((pres=sis3300_write(module, 0x8, conf&~0x800))!=plOK)
            return pres;
    }

    /* read data */
    help=outptr++;
    *help=0;
    for (i=0; i<4; i++) {
        if ((1<<i)&p[3]) {
            (*help)++;
            *outptr++=p[4];
            res=dev->read_a32(dev, addr+0x400000+0x80000*i, outptr, p[4]*4, 4,
                    &outptr);
            if (res!=p[4]*4) {
                printf("read sis3300: samples=%d, res=%d errno=%s\n",
                    p[4]*4, res*4, strerror(errno));
                return plErr_System;
            }
        }
    }

    /* reenable IRQ */
    if (p[2]) {
        if ((sis3300_write(module, 0xc, p[2]))!=plOK)
            return pres;
        if ((sis3300_write(module, addr+0x8, conf))!=plOK)
            return pres;
    }

    /* reenable sample clock bank 1 */
    if ((pres=sis3300_write(module, 0x10, 1<<0))!=plOK)
            return pres;

    return plOK;
}

plerrcode test_proc_sis3300read_single(ems_u32* p)
{
    ml_entry* module;
    plerrcode pres;

    if (p[0]!=4)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0)) {
        *outptr++=1;
        return plErr_ArgRange;
    }
    module=ModulEnt(p[1]);
    if (module->modultype!=SIS_3300)
        return plErr_BadModTyp;
    if ((pres=verify_vme_module(module, 0))!=plOK)
        return pres;
    if (p[2]&~0xf) {
        *outptr++=2;
        return plErr_ArgRange;
    }
    if (p[3]&~0xf) {
        *outptr++=3;
        return plErr_ArgRange;
    }
    if (p[4]>0x20000) {
        *outptr++=4;
        return plErr_ArgRange;
    }

    wirbrauchen=4*p[4];
    return plOK;
}

char name_proc_sis3300read_single[] = "sis3300read_single";
int ver_proc_sis3300read_single = 1;
/*****************************************************************************/
/*
 * p[0]: argcount(==4)
 * p[1]: index in memberlist
 * p[2]: irq level
 * p[3]: irq vector
 * p[4]: irq source (0 if irq not used)
 */
#define ROAK 0
plerrcode proc_sis3300start_single(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    /*struct vme_dev* dev=module->address.vme.dev;*/
    /*ems_u32 addr=module->address.vme.addr;*/
    plerrcode pres;

    /* set irq source */
    if ((pres=sis3300_rset_bits(module, 0xc, 4, 0, 0+16, p[4]))!=plOK)
            return pres;

    /* set irq vector and level */
    if ((pres=sis3300_write(module, 0x8,
            (p[3]&0xff)|((p[2]&7)<<8)|(1<<11)|(ROAK<<12)))!=plOK)
            return pres;

    /* enable sample clock bank 1 */
    if ((pres=sis3300_write(module, 0x10, 1<<0))!=plOK)
            return pres;

    /* switch on user LED */
    if ((pres=sis3300_write(module, 0x0, 1<<0))!=plOK)
            return pres;

    return plOK;
}

plerrcode test_proc_sis3300start_single(ems_u32* p)
{
    ml_entry* module;
    plerrcode pres;

    if (p[0]!=4)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=SIS_3300)
        return plErr_BadModTyp;
    if ((pres=verify_vme_module(module, 0))!=plOK)
        return pres;
    if (p[4]>3) {
        *outptr++=4;
        return plErr_ArgRange;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_sis3300start_single[] = "sis3300start_single";
int ver_proc_sis3300start_single = 1;
/*****************************************************************************/
/*
 * p[0]: argcount(==2)
 * p[1]: index in memberlist
 */
plerrcode proc_sis3300stop_single(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    /*struct vme_dev* dev=module->address.vme.dev;*/
    /*ems_u32 addr=module->address.vme.addr;*/
    plerrcode pres;

    /* switch off user LED */
    if ((pres=sis3300_write(module, 0x0, 1<<16))!=plOK)
            return pres;

    /* disable sample clocks */
    if ((pres=sis3300_write(module, 0x10, 3<<16))!=plOK)
            return pres;

    /* disable irq */
    if ((pres=sis3300_write(module, 0x8, 0))!=plOK)
            return pres;

    /* reset irq sources */
    if ((pres=sis3300_write(module, 0xc, 0xf<<16))!=plOK)
            return pres;

    return plOK;
}

plerrcode test_proc_sis3300stop_single(ems_u32* p)
{
    ml_entry* module;
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=SIS_3300)
        return plErr_BadModTyp;
    if ((pres=verify_vme_module(module, 0))!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_sis3300stop_single[] = "sis3300stop_single";
int ver_proc_sis3300stop_single = 1;
/*****************************************************************************/
/*
 * p[0]: argcount(==3)
 * p[1]: index in memberlist
 * p[2]: register
 * [p[3]: value]
 */
plerrcode proc_sis3300reg(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    /*struct vme_dev* dev=module->address.vme.dev;*/
    ems_u32 addr=module->address.vme.addr;
    ems_u32 val;
    plerrcode pres;

    if (p[0]>2) {
        if ((pres=sis3300_write(module, addr+p[2], p[3]))!=plOK)
            return pres;
    } else {
        if ((pres=sis3300_read(module, addr+p[2], &val))!=plOK)
            return pres;
        *outptr++=val;
    }
    return plOK;
}

plerrcode test_proc_sis3300reg(ems_u32* p)
{
    ml_entry* module;
    plerrcode pres;

    if ((p[0]!=2) && (p[0]!=3))
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=SIS_3300)
        return plErr_BadModTyp;
    if ((pres=verify_vme_module(module, 0))!=plOK)
        return pres;

    wirbrauchen=p[3];
    return plOK;
}

char name_proc_sis3300reg[] = "sis3300reg";
int ver_proc_sis3300reg = 1;
/*****************************************************************************/
/*
 * p[0]: argcount(==4)
 * p[1]: index in memberlist
 * p[2]: group
 * p[3]: value
 * p[4]: no. of samples
 */
plerrcode proc_sis3300fill_mem(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr;
    int res, i;
    plerrcode pres=plOK;
    ems_u32* buf;

    buf=malloc(p[4]*sizeof(ems_u32));
    if (!buf) {
        printf("proc_sis3300fill_mem: %s\n", strerror(errno));
        return plErr_System;
    }
    for (i=0; i<p[4]; i++)
        buf[i]=p[3];

    res=dev->write_a32(dev, addr+0x400000+0x80000*p[2], buf, p[4]*4, 4);
    if (res!=p[4]*4) {
        printf("proc_sis3300fill_mem: samples=%d, res=%d errno=%s\n",
                p[4]*4, res*4, strerror(errno));
        pres=plErr_System;
    }
    free(buf);

    return pres;
}

plerrcode test_proc_sis3300fill_mem(ems_u32* p)
{
    ml_entry* module;
    plerrcode pres;

    if (p[0]!=4)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0)) {
        *outptr++=1;
        return plErr_ArgRange;
    }
    module=ModulEnt(p[1]);
    if (module->modultype!=SIS_3300)
        return plErr_BadModTyp;
    if ((pres=verify_vme_module(module, 0))!=plOK)
        return pres;
    if (p[2]>3) {
        *outptr++=2;
        return plErr_ArgRange;
    }
    if (p[4]>0x20000) {
        *outptr++=4;
        return plErr_ArgRange;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_sis3300fill_mem[] = "sis3300fill_mem";
int ver_proc_sis3300fill_mem = 1;
/*****************************************************************************/
/*****************************************************************************/
