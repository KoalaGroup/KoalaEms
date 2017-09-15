/*
 * procs/unixvme/caen/v729a.c
 * created 11.Sep.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: v729a.c,v 1.6 2011/04/06 20:30:35 wuestner Exp $";

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
 * V729a 4-channel 12-bit 40 MHz ADC
 * A32D16 (registers) A32D32 (output buffers)
 * reserves 65536 Byte
 */
/*
 * 0x00: output buffer ch. 0/1 (D32/BLT32)
 * 0x04: output buffer ch. 2/3 (D32/BLT32)
 * 0x08: sample register
 * 0x0A: accepted event counter
 * 0x0C: rejected event counter
 * 0x0E: control/status
 * 0x10: FIFO setting register
 * 0x12: update FIFO set register
 * 0x14: reset
 * 0x16: stop
 * 0x18: DAC ofset ch.0 +
 * 0x1A: DAC ofset ch.0 -
 * 0x1C: DAC ofset ch.1 +
 * 0x1E: DAC ofset ch.1 -
 * 0x20: DAC ofset ch.2 +
 * 0x22: DAC ofset ch.2 -
 * 0x24: DAC ofset ch.3 +
 * 0x26: DAC ofset ch.3 -
 * 0xFA: 0xFAF5 (const)
 * 0xFC: <15:10>Manufacturer(==0x2) <9:0>ModuleType(==0x48)
 * 0xFE: <15:12>Version <11:0>Serial No.
 */

/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v729reset(ems_u32* p)
{
    int i, res;

    for (i=memberlist[0]; i>0; i--) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;

        /* reset */
        res=dev->write_a32d16(dev, module->address.vme.addr+0x14, 0);
        if (res!=2) return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_v729reset(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V729A, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v729reset[] = "v729reset";
int ver_proc_v729reset = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v729stop(ems_u32* p)
{
    int i, res;

    for (i=memberlist[0]; i>0; i--) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;

        /* reset */
        res=dev->write_a32d16(dev, module->address.vme.addr+0x16, 1);
        if (res!=2) return plErr_System;
        res=dev->write_a32d16(dev, module->address.vme.addr+0x16, 0);
        if (res!=2) return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_v729stop(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V729A, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v729stop[] = "v729stop";
int ver_proc_v729stop = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v729acqmode(ems_u32* p)
{
    int i, res;

    for (i=memberlist[0]; i>0; i--) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;

        /* set DAQ mode */
        res=dev->write_a32d16(dev, module->address.vme.addr+0x0E, 0);
        if (res!=2) return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_v729acqmode(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V729A, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v729acqmode[] = "v729acqmode";
int ver_proc_v729acqmode = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==9
 * p[1]: module
 * p[2]: DAC ofset ch.0 +
 * p[3]: DAC ofset ch.0 -
 * p[4]: DAC ofset ch.1 +
 * p[5]: DAC ofset ch.1 -
 * p[6]: DAC ofset ch.2 +
 * p[7]: DAC ofset ch.2 -
 * p[8]: DAC ofset ch.3 +
 * p[9]: DAC ofset ch.3 -
 */
plerrcode proc_v729offsets(ems_u32* p)
{
    int i, res;
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;
    ems_u32 addr=module->address.vme.addr+0x18;
    
    for (i=0; i<8; i++, addr+=2) {
        res=dev->write_a32d16(dev, addr, p[i+2]);
        if (res!=2) return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_v729offsets(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V729A, 0};

    if (p[0]!=9) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;
    if ((unsigned int)p[1]>memberlist[0]) return plErr_ArgRange;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v729offsets[] = "v729offsets";
int ver_proc_v729offsets = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v729read(ems_u32* p)
{
    int i, res;

    *outptr++=memberlist[0];
    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 base=module->address.vme.addr;
        ems_u32 ob01=base;
        ems_u32 ob23=base+4;
        ems_u16 data, samples, nbytes;
        ems_u32* help;

#if 0
        /* accepted event counter */
        res=dev->read_a32d16(dev, base+0x0a, &data);
        if (res!=2) return plErr_System;
        if (data!=1) {
            printf("v729read: accepted event counter=%d\n", data);
            return plErr_Other;
        }
#endif

        /* rejected event counter */
        res=dev->read_a32d16(dev, base+0x0c, &data);
        if (res!=2) return plErr_System;
        if (data) {
            printf("v729read: rejected event counter=%d\n", data);
            return plErr_Other;
        }
        /* sample register */
        res=dev->read_a32d16(dev, base+0x08, &samples);
        if (res!=2) return plErr_System;

        nbytes=(samples+1)*4;

        /* read output buffers */
        *outptr++=samples+1;
        help=outptr;
        res=dev->read_a32_fifo(dev, ob01, outptr, nbytes, 4, (ems_u32**)&outptr);
        if (res!=nbytes) {
            printf("v729read: read_a32_fifo(ob01, %d): res=%d\n", nbytes, res);
            return plErr_System;
        }
        if ((help[0]&0xe0000000)!=0xe0000000) {
            printf("v729read: read_a32_fifo(ob01): first word is not a header "
                    "(0x%08x)\n", help[0]);
            return plErr_Other;
        }
        if ((help[samples-1]&0x60000000)!=0x60000000) {
            printf("v729read: read_a32_fifo(ob01): last word not valid "
                    "(0x%08x)\n", help[samples-1]);
            return plErr_Other;
        }
        if (help[samples]&0x20000000) {
            printf("v729read: read_a32_fifo(ob01): too many valid words "
                    "(0x%08x)\n", help[samples]);
            return plErr_Other;
        }

        *outptr++=samples+1;
        help=outptr;
        res=dev->read_a32_fifo(dev, ob23, outptr, nbytes, 4, (ems_u32**)&outptr);
        if (res!=nbytes) {
            printf("v729read: read_a32_fifo(ob23, %d): res=%d\n", nbytes, res);
            return plErr_System;
        }
        if ((help[0]&0xc0000000)!=0xc0000000) {
            printf("v729read: read_a32_fifo(ob23): first word is not a header "
                    "(0x%08x)\n", help[0]);
            return plErr_Other;
        }
        if ((help[samples-1]&0x40000000)!=0x40000000) {
            printf("v729read: read_a32_fifo(ob23): last word not valid "
                    "(0x%08x)\n", help[samples-1]);
            return plErr_Other;
        }
        if (help[samples]&0x40000000) {
            printf("v729read: read_a32_fifo(ob23): too many valid words "
                    "(0x%08x)\n", help[samples]);
            return plErr_Other;
        }

    }
    return plOK;
}

plerrcode test_proc_v729read(ems_u32* p)
{
    plerrcode pres;
    ems_u32 mtypes[]={CAEN_V729A, 0};
    ems_u16 data;
    int i, res;

    if (p[0]) return plErr_ArgNum;
    if ((pres=test_proc_vme(memberlist, mtypes))!=plOK) return pres;

    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 base=module->address.vme.addr;

        res=dev->read_a32d16(dev, base+0x0e, &data);
        if (res!=2) return plErr_System;
        if (data&0x30) {
            printf("v729read: not in acquisition mode\n");
            return plErr_HWTestFailed;
        }
    }

    wirbrauchen=memberlist[0]*(4096+1)*2;
    return plOK;
}

char name_proc_v729read[] = "v729read";
int ver_proc_v729read = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v729status(ems_u32* p)
{
    int i, res;

    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u16 data;

        /* read status register */
        res=dev->read_a32d16(dev, module->address.vme.addr+0x0e, &data);
        if (res!=2) return plErr_System;
        *outptr++=data&0x3f;
    }
    return plOK;
}

plerrcode test_proc_v729status(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V729A, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=memberlist[0];
    return plOK;
}

char name_proc_v729status[] = "v729status";
int ver_proc_v729status = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: number of samples
 */
plerrcode proc_v729samples(ems_u32* p)
{
    int i, res;

    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;

        /* write sample register */
        res=dev->write_a32d16(dev, module->address.vme.addr+0x08, p[1]);
        if (res!=2) return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_v729samples(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V729A, 0};

    if (p[0]!=1) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v729samples[] = "v729samples";
int ver_proc_v729samples = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: number of samples
 */
plerrcode proc_v729clb(ems_u32* p)
{
    int i, res, cbl=p[1];
    int obae=0, obaf=1;
    
    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 set=module->address.vme.addr+0x10;
        ems_u32 update=module->address.vme.addr+0x12;
        ems_u16 data[4];

        data[0]=(obae&0xff)<<8;
        data[1]=obae&0xf00;
        data[2]=((obaf&0xff)<<8)|(cbl&0xff);
        data[3]=(obaf&0xf00)|((cbl&0xf00)>>8);

        for (i=0; i<4; i++) {
            res=dev->write_a32d16(dev, set, data[i]);
            if (res!=2) return plErr_System;
            res=dev->write_a32d16(dev, update, 0);
            if (res!=2) return plErr_System;
        }
    }
    return plOK;
}

plerrcode test_proc_v729clb(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_V729A, 0};

    if (p[0]!=1) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_v729clb[] = "v729clb";
int ver_proc_v729clb = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v729pread(ems_u32* p)
{
    int i, res;

    *outptr++=memberlist[0];
    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 base=module->address.vme.addr;
        ems_u32 ob01=base;
        ems_u32 ob23=base+4;
        ems_u16 data, samples, nbytes;
        ems_u32* help;

#if 0
        /* accepted event counter */
        res=dev->read_a32d16(dev, base+0x0a, &data);
        if (res!=2) return plErr_System;
        if (data!=1) {
            printf("v729pread: accepted event counter=%d\n", data);
            return plErr_Other;
        }
#endif

        /* rejected event counter */
        res=dev->read_a32d16(dev, base+0x0c, &data);
        if (res!=2) return plErr_System;
        if (data) {
            printf("v729pread: rejected event counter=%d\n", data);
            return plErr_Other;
        }
        /* sample register */
        res=dev->read_a32d16(dev, base+0x08, &samples);
        if (res!=2) return plErr_System;

        nbytes=(samples+1)*4;

        /* read output buffers */
        *outptr++=samples+1;
        help=outptr;
        res=dev->read_a32_fifo(dev, ob01, outptr, nbytes, 4, (ems_u32**)&outptr);
        if (res!=nbytes) {
            printf("v729pread: read_a32_fifo(ob01, %d): res=%d\n", nbytes, res);
            return plErr_System;
        }
        if ((help[0]&0xe0000000)!=0xe0000000) {
            printf("v729pread: read_a32_fifo(ob01): first word is not a header "
                    "(0x%08x)\n", help[0]);
            return plErr_Other;
        }
        if ((help[samples-1]&0x60000000)!=0x60000000) {
            printf("v729pread: read_a32_fifo(ob01): last word not valid "
                    "(0x%08x)\n", help[samples-1]);
            return plErr_Other;
        }
        if (help[samples]&0x20000000) {
            printf("v729pread: read_a32_fifo(ob01): too many valid words "
                    "(0x%08x)\n", help[samples]);
            return plErr_Other;
        }
//        *outptr++=samples+1;
        help=outptr;
        res=dev->read_a32_fifo(dev, ob23, outptr, nbytes, 4, (ems_u32**)&outptr);
        if (res!=nbytes) {
            printf("v729pread: read_a32_fifo(ob23, %d): res=%d\n", nbytes, res);
            return plErr_System;
        }
        outptr=help;
/*	
        if ((help[0]&0xc0000000)!=0xc0000000) {
            printf("v729pread: read_a32_fifo(ob23): first word is not a header "
                    "(0x%08x)\n", help[0]);
            return plErr_Other;
        }
        if ((help[samples-1]&0x40000000)!=0x40000000) {
            printf("v729pread: read_a32_fifo(ob23): last word not valid "
                    "(0x%08x)\n", help[samples-1]);
            return plErr_Other;
        }
        if (help[samples]&0x40000000) {
            printf("v729pread: read_a32_fifo(ob23): too many valid words "
                    "(0x%08x)\n", help[samples]);
            return plErr_Other;
        }
*/
    }
    return plOK;
}

plerrcode test_proc_v729pread(ems_u32* p)
{
    plerrcode pres;
    ems_u32 mtypes[]={CAEN_V729A, 0};
    ems_u16 data;
    int i, res;

    if (p[0]) return plErr_ArgNum;
    if ((pres=test_proc_vme(memberlist, mtypes))!=plOK) return pres;

    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 base=module->address.vme.addr;

        res=dev->read_a32d16(dev, base+0x0e, &data);
        if (res!=2) return plErr_System;
        if (data&0x30) {
            printf("v729pread: not in acquisition mode\n");
            return plErr_HWTestFailed;
        }
    }

    wirbrauchen=memberlist[0]*(4096+1)*2;
    return plOK;
}

char name_proc_v729pread[] = "v729pread";
int ver_proc_v729pread = 1;

/************************************************************/
plerrcode proc_v729prread(ems_u32* p)
{
    int i, res;

    *outptr++=memberlist[0];
    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 base=module->address.vme.addr;
        ems_u32 ob01=base;
//        ems_u32 ob23=base+4;
        ems_u16 data, samples, nbytes;
        ems_u32* help;

#if 0
        /* accepted event counter */
        res=dev->read_a32d16(dev, base+0x0a, &data);
        if (res!=2) return plErr_System;
        if (data!=1) {
            printf("v729prread: accepted event counter=%d\n", data);
            return plErr_Other;
        }
#endif

        /* rejected event counter */
        res=dev->read_a32d16(dev, base+0x0c, &data);
        if (res!=2) return plErr_System;
        if (data) {
            printf("v729prread: rejected event counter=%d\n", data);
            return plErr_Other;
        }
        /* sample register */
        res=dev->read_a32d16(dev, base+0x08, &samples);
        if (res!=2) return plErr_System;

        nbytes=(samples+1)*4;

        /* read output buffers */
        *outptr++=samples+1;
        help=outptr;
        res=dev->read_a32_fifo(dev, ob01, outptr, nbytes, 4, (ems_u32**)&outptr);
        if (res!=nbytes) {
            printf("v729prread: read_a32_fifo(ob01, %d): res=%d\n", nbytes, res);
            return plErr_System;
        }
        if ((help[0]&0xe0000000)!=0xe0000000) {
            printf("v729prread: read_a32_fifo(ob01): first word is not a header "
                    "(0x%08x)\n", help[0]);
            return plErr_Other;
        }
        if ((help[samples-1]&0x60000000)!=0x60000000) {
            printf("v729prread: read_a32_fifo(ob01): last word not valid "
                    "(0x%08x)\n", help[samples-1]);
            return plErr_Other;
        }
        if (help[samples]&0x20000000) {
            printf("v729prread: read_a32_fifo(ob01): too many valid words "
                    "(0x%08x)\n", help[samples]);
            return plErr_Other;
        }
/*        *outptr++=samples+1;
        help=outptr;
        res=dev->read_a32_fifo(dev, ob23, outptr, nbytes, 4, (ems_u32**)&outptr);
        if (res!=nbytes) {
            printf("v729prread: read_a32_fifo(ob23, %d): res=%d\n", nbytes, res);
            return plErr_System;
        }
        outptr=help;

        if ((help[0]&0xc0000000)!=0xc0000000) {
            printf("v729prread: read_a32_fifo(ob23): first word is not a header "
                    "(0x%08x)\n", help[0]);
            return plErr_Other;
        }
        if ((help[samples-1]&0x40000000)!=0x40000000) {
            printf("v729prread: read_a32_fifo(ob23): last word not valid "
                    "(0x%08x)\n", help[samples-1]);
            return plErr_Other;
        }
        if (help[samples]&0x40000000) {
            printf("v729prread: read_a32_fifo(ob23): too many valid words "
                    "(0x%08x)\n", help[samples]);
            return plErr_Other;
        }
*/
    }
    return plOK;
}

plerrcode test_proc_v729prread(ems_u32* p)
{
    plerrcode pres;
    ems_u32 mtypes[]={CAEN_V729A, 0};
    ems_u16 data;
    int i, res;

    if (p[0]) return plErr_ArgNum;
    if ((pres=test_proc_vme(memberlist, mtypes))!=plOK) return pres;

    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 base=module->address.vme.addr;

        res=dev->read_a32d16(dev, base+0x0e, &data);
        if (res!=2) return plErr_System;
        if (data&0x30) {
            printf("v729prread: not in acquisition mode\n");
            return plErr_HWTestFailed;
        }
    }

    wirbrauchen=memberlist[0]*(4096+1)*2;
    return plOK;
}

char name_proc_v729prread[] = "v729prread";
int ver_proc_v729prread = 1;
/*****************************************************************************/
/*****************************************************************************/
