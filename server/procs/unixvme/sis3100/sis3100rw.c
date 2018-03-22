/*
 * procs/unixvme/sis3100/sis3100rw.c
 * created 11.Jan.2006 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100rw.c,v 1.10 2017/10/09 21:09:24 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../lowlevel/unixvme/vme.h"
#include "../../../lowlevel/devices.h"

#ifdef DMALLOC
#include dmalloc.h
#endif

extern ems_u32* outptr;

#define get_vmedevice(crate) \
    (struct vme_dev*)get_gendevice(modul_vme, (crate))
#define ofs(what, elem) ((int)&(((what *)0)->elem))

RCS_REGISTER(cvsid, "procs/unixvme/sis3100")

/*****************************************************************************/
char name_proc_sis3100_read[] = "sis3100_read";
int ver_proc_sis3100_read = 1;
/*
 * p[0]: argcount==3
 * p[1]: crate ID
 * p[2]: domain (0: PCI card, 1: VME card)
 * p[3]: address
 */
plerrcode proc_sis3100_read(ems_u32* p)
{
    struct vme_dev* dev=get_vmedevice(p[1]);

    if (dev->read_controller(dev, p[2], p[3], outptr)<0)
        return plErr_System;
    else
        outptr++;

    return plOK;
}

plerrcode test_proc_sis3100_read(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if (p[2]>1)
        return plErr_ArgRange;

    pres=find_vme_odevice(p[1], (struct vme_dev**)0);

    wirbrauchen=1;
    return pres;
}
/*****************************************************************************/
char name_proc_sis3100_write[] = "sis3100_write";
int ver_proc_sis3100_write = 1;
/*
 * p[0]: argcount==4
 * p[1]: crate ID
 * p[2]: domain (0: PCI card, 1: VME card)
 * p[3]: address
 * p[4]: value
 */
plerrcode proc_sis3100_write(ems_u32* p)
{
    struct vme_dev* dev=get_vmedevice(p[1]);

    if (dev->write_controller(dev, p[2], p[3], p[4])<0)
        return plErr_System;

    return plOK;
}

plerrcode test_proc_sis3100_write(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=4)
        return plErr_ArgNum;
    if (p[2]>1)
        return plErr_ArgRange;

printf("sis3100_write: p[0]=%d p[1]=%d p[2]=%d\n",
        p[0], p[1], p[2]);

    pres=find_vme_odevice(p[1], (struct vme_dev**)0);

    wirbrauchen=0;
    return pres;
}
/*****************************************************************************/
char name_proc_sis3100_ack[] = "sis3100_ack";
int ver_proc_sis3100_ack = 1;
/*
 * p[0]: argcount==2
 * p[1]: crate ID
 * p[2]: level
 */
plerrcode proc_sis3100_ack(ems_u32* p)
{
    struct vme_dev* dev=get_vmedevice(p[1]);
    ems_u32 vector, error;

    if (dev->irq_ack(dev, p[2], &vector, &error)<0)
        return plErr_System;

    *outptr++=error;
    *outptr++=vector;
    return plOK;
}

plerrcode test_proc_sis3100_ack(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if (p[2]>0xff)
        return plErr_ArgRange;

    pres=find_vme_odevice(p[1], (struct vme_dev**)0);

    wirbrauchen=2;
    return pres;
}
/*****************************************************************************/
char name_proc_sis3100_front_io[] = "sis3100_front_io";
int ver_proc_sis3100_front_io = 1;
/*
 * p[0]: argcount==3 or 4
 * p[1]: crate ID
 * p[2]: domain (0: PCI card, 1: VME card)
 * p[3]: static: 0; latched: 1
 * [p[4]: value]
 */
plerrcode proc_sis3100_front_io(ems_u32* p)
{
    struct vme_dev* dev=get_vmedevice(p[1]);
    ems_u32 val;

    val=p[0]>3?p[4]:0;
    if (dev->front_io(dev, p[2], p[3], &val)<0)
        return plErr_System;

    *outptr++=val;
    return plOK;
}

plerrcode test_proc_sis3100_front_io(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3 && p[0]!=4)
        return plErr_ArgNum;
    if (p[2]>1)
        return plErr_ArgRange;
    if (p[3]>1)
        return plErr_ArgRange;

    pres=find_vme_odevice(p[1], (struct vme_dev**)0);

    wirbrauchen=1;
    return pres;
}
/*****************************************************************************/
char name_proc_sis3100_front_setup[] = "sis3100_front_setup";
int ver_proc_sis3100_front_setup = 1;
/*
 * p[0]: argcount==2 or 3
 * p[1]: crate ID
 * p[2]: domain (0: PCI card, 1: VME card)
 * [p[3]: value]
 */
plerrcode proc_sis3100_front_setup(ems_u32* p)
{
    struct vme_dev* dev=get_vmedevice(p[1]);
    ems_u32 val;

    val=p[0]>2?p[3]:0;
    if (dev->front_setup(dev, p[2], &val)<0)
        return plErr_System;

    *outptr++=val;
    return plOK;
}

plerrcode test_proc_sis3100_front_setup(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2 && p[0]!=3)
        return plErr_ArgNum;
    if (p[2]>1)
        return plErr_ArgRange;

    pres=find_vme_odevice(p[1], (struct vme_dev**)0);

    wirbrauchen=1;
    return pres;
}
/*****************************************************************************/
/*****************************************************************************/
