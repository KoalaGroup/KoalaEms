/*
 * procs/unixvme/sis3400/sis3400.c
 * created 10.Oct.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3400.c,v 1.9 2011/04/06 20:30:35 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../objects/pi/readout.h"
#include "../../../lowlevel/unixvme/vme.h"
#include "../../../lowlevel/devices.h"
#include "../vme_verify.h"

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

#define FIFOSIZE 32768

RCS_REGISTER(cvsid, "procs/unixvme/sis3100")

/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: control input mode (0: front panel 1: bus 2: VME 3: test mode)
 * p[2]: trailing edge (0: disable 1: enable)
 */
plerrcode proc_sis3400init(ems_u32* p)
{
    int i, res;

    for (i=memberlist[0]; i>0; i--) {
        ems_u32 v, id, version;
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 base=module->address.vme.addr;

        /* read module identification */
        res=dev->read_a32d32(dev, base+4, &v);
        if (res<0) return plErr_System;
        id=(v>>16)&0xffff;
        version=(v>>12)&0xf;
        if (id!=0x3400) {
            printf("sis3400init(idx=%d): id=%04x version=%d\n", i, id, version);
            printf("                     read 0x%08x\n", v);
            return plErr_HWTestFailed;
        }
        printf("sis3400init(idx=%d): id=%04x version=%d\n", i, id, version);

        /* reset module */
        res=dev->write_a32d32(dev, base+0x20, 0);
        if (res<0) return plErr_System;

        /* setup input logic */
        {
        ems_u32 mode=p[1];
        res=dev->write_a32d32(dev, base+0x0, ((mode&7)<<4)|((~mode&7)<<12));
        if (res<0) return plErr_System;
        }
        /* enable 200 MHz clock */
        res=dev->write_a32d32(dev, base+0x0, 4);
        if (res<0) return plErr_System;

        /* setup formatter mode */
            /* enable trailing edge: 2 */
            /* single wire mode    : 1 */
        res=dev->write_a32d32(dev, base+0x100, 1|(p[2]?2:0)); 
        if (res<0) return plErr_System;

        /* set formatter addess */
        res=dev->write_a32d32(dev, base+0x104, i-1);
        if (res<0) return plErr_System;

        /* enable input logic */
        res=dev->write_a32d32(dev, base+0x28, 0);
        if (res<0) return plErr_System;

        /* switch LED on (just for fun) */
        res=dev->write_a32d32(dev, base+0x0, 0x1 /*0x100: off*/);
        if (res<0) return plErr_System;
    }
    return plOK;
}

plerrcode test_proc_sis3400init(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={SIS_3400, 0};

    if (p[0]!=2) return plErr_ArgNum;
    if ((unsigned int)p[1]>2) return plErr_ArgRange;
    if ((unsigned int)p[2]>1) return plErr_ArgRange;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_sis3400init[] = "sis3400init";
int ver_proc_sis3400init = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_sis3400clear(ems_u32* p)
{
    int i;

    for (i=memberlist[0]; i>0; i--) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;

        /* clear FIFO */
        dev->write_a32d32(dev, module->address.vme.addr+0x130, 0);
    }
    return plOK;
}

plerrcode test_proc_sis3400clear(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={SIS_3400, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_sis3400clear[] = "sis3400clear";
int ver_proc_sis3400clear = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_sis3400read(ems_u32* p)
{
    int i;

    *outptr++=memberlist[0];
    for (i=memberlist[0]; i>0; i--) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 base=module->address.vme.addr, v, *help;
        int res, max;

        help=outptr++;
        res=dev->read_a32d32(dev, base+0x108, &v);
        if (res<0) return plErr_System;
        if (v&0x1000) {
            printf("sis3400read: FIFO overflow: flags=0x%x\n", v);
            return plErr_Overflow;
        }
        max=event_max/2;
        do {
            int n=max;
            if (n>FIFOSIZE) n=FIFOSIZE;
            res=dev->read_a32_fifo(dev, base+0x20000, outptr, n*4, 4,
                (ems_u32**)&outptr);
            if (res<0) {
                printf("sis3400read: read_a32_fifo failed\n");
                return plErr_System;
            }
            max-=res/4;
        } while ((res==FIFOSIZE) && (max>0));
        *help=(outptr-help)-1;
        res=dev->read_a32d32(dev, base+0x108, &v);
        if (res<0) return plErr_System;
        if (v&0x1000) {
            printf("sis3400read: too many data: flags=0x%x\n", v);
            return plErr_Overflow;
        }
    }
    return plOK;
}

plerrcode test_proc_sis3400read(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={SIS_3400, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=event_max/2;
    return plOK;
}

char name_proc_sis3400read[] = "sis3400read";
int ver_proc_sis3400read = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_sis3400tread(ems_u32* p)
{
    int i;

    printf("sis3400tread:\n");
    for (i=memberlist[0]; i>0; i--) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 base=module->address.vme.addr;
        ems_u32 v;

        dev->read_a32d32(dev, base+0x108, &v);
        while (!(v&1)) {
            dev->read_a32d32(dev, base+0x20000, &v);
            printf("  %08x id %d chan %2d stamp %7d %c\n",
                v,
                (v>>26)&0x1f,
                (v>>20)&0x3f,
                v&0xfffff,
                (v&0x80000000)?'t':' ');
            dev->read_a32d32(dev, base+0x108, &v);
        }
    }
    return plOK;
}

plerrcode test_proc_sis3400tread(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={SIS_3400, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_sis3400tread[] = "sis3400tread";
int ver_proc_sis3400tread = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_sis3400readclock(ems_u32* p)
{
    int i;

    for (i=memberlist[0]; i>0; i--) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 base=module->address.vme.addr;
        ems_u32 v[4];

        dev->write_a32d32(dev, base+0x200, 1);
        dev->read_a32d32(dev, base+0x230, v+0);
        dev->read_a32d32(dev, base+0x234, v+1);
        dev->read_a32d32(dev, base+0x238, v+2);
        dev->read_a32d32(dev, base+0x23c, v+3);
        dev->write_a32d32(dev, base+0x200, 0);
        *outptr++=(v[0]<<24)|(v[1]<<16)|(v[2]<<8)|v[3];
    }
    return plOK;
}

plerrcode test_proc_sis3400readclock(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={SIS_3400, 0};

    if (p[0]) return plErr_ArgNum;
    if ((res=test_proc_vme(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=memberlist[0];
    return plOK;
}

char name_proc_sis3400readclock[] = "sis3400readclock";
int ver_proc_sis3400readclock = 1;
/*****************************************************************************/
/*****************************************************************************/
