/*
 * procs/unixvme/vme_primitiv.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: vme_primitiv.c,v 1.13 2011/04/06 20:30:35 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/unixvme/vme.h"
#include "../../lowlevel/devices.h"
#include "../../trigger/trigger.h"

extern ems_u32* outptr;
extern int wirbrauchen;

#define get_device(class, crate) \
    (struct vme_dev*)get_gendevice((class), (crate))

RCS_REGISTER(cvsid, "procs/unixvme")

/*****************************************************************************/
static plerrcode
test_vmeparm(ems_u32* p, int n)
{
    plerrcode pres;
    if (p[0]!=n)
        return(plErr_ArgNum);
    if ((pres=find_odevice(modul_vme, p[1], 0))!=plOK)
        return pres;
    wirbrauchen = 1;
    return plOK;
}
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: crate
 * p[2]: min read length
 * p[3]: min write length
 */
plerrcode proc_vme_mindmalen(ems_u32* p)
{
    int rlen, wlen;
    struct vme_dev* dev=get_device(modul_vme, p[1]);

    rlen=p[2];
    wlen=p[3];
    if (dev->mindmalen(dev, &rlen, &wlen)) {
	return plErr_System;
    }
    *outptr++=rlen;
    *outptr++=wlen;
    return plOK;
}

plerrcode test_proc_vme_mindmalen(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_vme, p[1], 0))!=plOK)
        return pres;

    wirbrauchen=2;
    return plOK;
}

char name_proc_vme_mindmalen[] = "vme_mindmalen";
int ver_proc_vme_mindmalen = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: crate
 * p[2]: min read length
 * p[3]: min write length
 */
plerrcode proc_vme_minpipelen(ems_u32* p)
{
    int rlen, wlen;
    struct vme_dev* dev=get_device(modul_vme, p[1]);

    rlen=p[2];
    wlen=p[3];
    if (dev->minpipelen(dev, &rlen, &wlen)) {
	return plErr_System;
    }
    *outptr++=rlen;
    *outptr++=wlen;
    return plOK;
}

plerrcode test_proc_vme_minpipelen(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_vme, p[1], 0))!=plOK)
        return pres;

    wirbrauchen=2;
    return plOK;
}

char name_proc_vme_minpipelen[] = "vme_minpipelen";
int ver_proc_vme_minpipelen = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: crate
 * p[2]: absolute address
 */
plerrcode proc_vmeread1(ems_u32* p)
{
    int res;
    ems_u8 help;
    struct vme_dev* dev=get_device(modul_vme, p[1]);

    res=dev->read_a32d8(dev, p[2], &help);
    if (res==1) {
	*outptr++ = (int)help;
	return(plOK);
    } else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_vmeread1(ems_u32* p)
{
    return test_vmeparm(p, 2);
}

char name_proc_vmeread1[] = "vmeread1";
int ver_proc_vmeread1 = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: crate
 * p[2]: absolute address
 */
plerrcode proc_vmeread2(ems_u32* p)
{
    int res;
    ems_u16 help;
    struct vme_dev* dev=get_device(modul_vme, p[1]);

    res=dev->read_a32d16(dev, p[2], &help);
    if (res==2) {
	*outptr++ = (int)help;
	return(plOK);
    } else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_vmeread2(ems_u32* p)
{
    return test_vmeparm(p, 2);
}

char name_proc_vmeread2[] = "vmeread2";
int ver_proc_vmeread2 = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: crate
 * p[2]: absolute address
 */
plerrcode proc_vme24read1(ems_u32* p)
{
    int res;
    ems_u8 help;
    struct vme_dev* dev=get_device(modul_vme, p[1]);

    res=dev->read_a24d8(dev, p[2], &help);
    if (res==1) {
	*outptr++ = (int)help;
	return(plOK);
    } else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_vme24read1(ems_u32* p)
{
    return test_vmeparm(p, 2);
}

char name_proc_vme24read1[] = "vme24read1";
int ver_proc_vme24read1 = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: crate
 * p[2]: absolute address
 */
plerrcode proc_vme24read2(ems_u32* p)
{
    int res;
    ems_u16 help;
    struct vme_dev* dev=get_device(modul_vme, p[1]);

    res=dev->read_a24d16(dev, p[2], &help);
    if (res==2) {
	*outptr++ = (int)help;
	return(plOK);
    } else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_vme24read2(ems_u32* p)
{
    return test_vmeparm(p, 2);
}

char name_proc_vme24read2[] = "vme24read2";
int ver_proc_vme24read2 = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: crate
 * p[2]: absolute address
 * p[3]: data
 */
plerrcode proc_vmewrite1(ems_u32* p)
{
    int res;
    struct vme_dev* dev=get_device(modul_vme, p[1]);

    res=dev->write_a32d8(dev, p[2], (ems_u8)p[3]);
    if (res==1) {
	return(plOK);
    } else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_vmewrite1(ems_u32* p)
{
    return test_vmeparm(p, 3);
}

char name_proc_vmewrite1[] = "vmewrite1";
int ver_proc_vmewrite1 = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: crate
 * p[2]: absolute address
 * p[3]: data
 */
plerrcode proc_vmewrite2(ems_u32* p)
{
    int res;
    struct vme_dev* dev=get_device(modul_vme, p[1]);

    res=dev->write_a32d16(dev, p[2], (ems_u16)p[3]);
    if (res==2)
	return(plOK);
    else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_vmewrite2(ems_u32* p)
{
    return test_vmeparm(p, 3);
}

char name_proc_vmewrite2[] = "vmewrite2";
int ver_proc_vmewrite2 = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: crate
 * p[2]: absolute address
 * p[3]: data
 */
plerrcode proc_vme24write1(ems_u32* p)
{
    int res;
    struct vme_dev* dev=get_device(modul_vme, p[1]);

    res=dev->write_a24d8(dev, p[2], (ems_u8)p[3]);
    if (res==1)
	return(plOK);
    else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_vme24write1(ems_u32* p)
{
    return test_vmeparm(p, 3);
}

char name_proc_vme24write1[] = "vme24write1";
int ver_proc_vme24write1 = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: crate
 * p[2]: absolute address
 * p[3]: data
 */
plerrcode proc_vme24write2(ems_u32* p)
{
    int res;
    struct vme_dev* dev=get_device(modul_vme, p[1]);

    res=dev->write_a24d16(dev, p[2], (ems_u16)p[3]);
    if (res==2)
	return(plOK);
    else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_vme24write2(ems_u32* p)
{
    return test_vmeparm(p, 3);
}

char name_proc_vme24write2[] = "vme24write2";
int ver_proc_vme24write2 = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: crate
 * p[2]: absolute address
 */
plerrcode proc_vmeread4(ems_u32* p)
{
    int res;
    struct vme_dev* dev=get_device(modul_vme, p[1]);

    res=dev->read_a32d32(dev, p[2], outptr);
    if (res==4) {
	outptr++;
	return(plOK);
    } else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_vmeread4(ems_u32* p)
{
    return test_vmeparm(p, 2);
}

char name_proc_vmeread4[] = "vmeread4";
int ver_proc_vmeread4 = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: crate
 * p[2]: absolute address
 */
plerrcode proc_vme24read4(ems_u32* p)
{
    int res;
    struct vme_dev* dev=get_device(modul_vme, p[1]);

    res=dev->read_a24d32(dev, p[2], outptr);
    if (res==4) {
	outptr++;
	return(plOK);
    } else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_vme24read4(ems_u32* p)
{
    return test_vmeparm(p, 2);
}

char name_proc_vme24read4[] = "vme24read4";
int ver_proc_vme24read4 = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: crate
 * p[2]: absolute address
 * p[3]: data
 */
plerrcode proc_vmewrite4(ems_u32* p)
{
    int res;
    struct vme_dev* dev=get_device(modul_vme, p[1]);

    res=dev->write_a32d32(dev, p[2], p[3]);
    if (res==4)
	return(plOK);
    else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_vmewrite4(ems_u32* p)
{
    return test_vmeparm(p, 3);
}

char name_proc_vmewrite4[] = "vmewrite4";
int ver_proc_vmewrite4 = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: crate
 * p[2]: absolute address
 * p[3]: data
 */
plerrcode proc_vme24write4(ems_u32* p)
{
    int res;
    struct vme_dev* dev=get_device(modul_vme, p[1]);

    res=dev->write_a24d32(dev, p[2], p[3]);
    if (res==4)
	return(plOK);
    else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_vme24write4(ems_u32* p)
{
    return test_vmeparm(p, 3);
}

char name_proc_vme24write4[] = "vme24write4";
int ver_proc_vme24write4 = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==5
 * p[1]: crate
 * p[2]: absolute address
 * p[3]: address modifier
 * p[4]: datasize
 * p[5]: number of words
 */
plerrcode proc_vmeread(ems_u32* p)
{
    int res;
    struct vme_dev* dev=get_device(modul_vme, p[1]);

    res=dev->read(dev, p[2], p[3], outptr, p[4]*p[5], p[4], (ems_u32**)&outptr);
#if 0
    printf("proc_vmeread: res=%d event %d\n", res, trigger.eventcnt);
#endif
    if (res>=0) {
        return plOK;
    } else {
        *outptr++=-res;
        return plErr_System;
    }
}

plerrcode test_proc_vmeread(ems_u32* p)
{
    plerrcode pres;
    if (p[0] != 5)
        return(plErr_ArgNum);
    if ((pres=find_odevice(modul_vme, p[1], 0))!=plOK)
        return pres;
    wirbrauchen = (p[4]*p[5]+3)/4;
    if (!wirbrauchen) wirbrauchen=1;
    return(plOK);
}

char name_proc_vmeread[] = "vmeread";
int ver_proc_vmeread = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==5
 * p[1]: crate
 * p[2]: absolute address
 * p[3]: address modifier
 * p[4]: datasize
 * p[5]: number of words
 */
plerrcode proc_vmereadfifo(ems_u32* p)
{
    int res;
    struct vme_dev* dev=get_device(modul_vme, p[1]);

    res=dev->read_a32_fifo(dev, p[2], outptr, p[4]*p[5], p[4],
            (ems_u32**)&outptr);
    if (res>=0) {
        return plOK;
    } else {
        *outptr++=-res;
        return plErr_System;
    }
}

plerrcode test_proc_vmereadfifo(ems_u32* p)
{
    plerrcode pres;
    if (p[0] != 5)
        return(plErr_ArgNum);
    if ((pres=find_odevice(modul_vme, p[1], 0))!=plOK)
        return pres;

    if (p[3]!=0xb) {
        printf("idiotic code: AM has to be 0xb!\n");
        return plErr_ArgRange;
    }

    wirbrauchen = (p[4]*p[5]+3)/4;
    if (!wirbrauchen) wirbrauchen=1;
    return(plOK);
}

char name_proc_vmereadfifo[] = "vmereadfifo";
int ver_proc_vmereadfifo = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==4
 * p[1]: crate
 * p[2]: absolute address
 * p[3]: datasize
 * p[4]: num
 */
plerrcode proc_vme24read(ems_u32* p)
{
    int res;
    struct vme_dev* dev=get_device(modul_vme, p[1]);

    res=dev->read_a24(dev, p[2], outptr, p[3]*p[4], p[3], (ems_u32**)&outptr);
    if (res>=0) {
        return plOK;
    } else {
        *outptr++=-res;
        return plErr_System;
    }
}

plerrcode test_proc_vme24read(ems_u32* p)
{
    plerrcode pres;
    if (p[0] != 4)
        return(plErr_ArgNum);
    if ((pres=find_odevice(modul_vme, p[1], 0))!=plOK)
        return pres;
    wirbrauchen = (p[3]*p[4]+3)/4;
    if (!wirbrauchen) wirbrauchen=1;
    return(plOK);
}

char name_proc_vme24read[] = "vme24read";
int ver_proc_vme24read = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==5+num_data
 * p[1]: crate
 * p[2]: absolute address
 * p[3]: address modifier
 * p[4]: datasize
 * p[5]: num
 * p[6...]: data
 */
plerrcode proc_vmewrite(ems_u32* p)
{
    int res;
    struct vme_dev* dev=get_device(modul_vme, p[1]);

    res=dev->write(dev, p[2], p[3], p+6, p[4]*p[5], p[4]);
    if (res>=0) {
        return plOK;
    } else {
        *outptr++=-res;
        return plErr_System;
    }
}

plerrcode test_proc_vmewrite(ems_u32* p)
{
    plerrcode pres;
    if (p[0] < 5)
        return(plErr_ArgNum);
    if (p[0]!=5+(p[4]*p[5]+3)/4)
        return(plErr_ArgNum);
    if ((pres=find_odevice(modul_vme, p[1], 0))!=plOK)
        return pres;
    wirbrauchen = 1;
    return(plOK);
}

char name_proc_vmewrite[] = "vmewrite";
int ver_proc_vmewrite = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==4
 * p[1]: crate
 * p[2]: absolute address
 * p[3]: datasize
 * p[4]: num
 * p[5]: data
 */
plerrcode proc_vme24write(ems_u32* p)
{
    int res;
    struct vme_dev* dev=get_device(modul_vme, p[1]);

    res=dev->write_a24(dev, p[2], p+5, p[3]*p[4], p[3]);
    if (res>=0) {
        return plOK;
    } else {
        *outptr++=-res;
        return plErr_System;
    }
}

plerrcode test_proc_vme24write(ems_u32* p)
{
    plerrcode pres;
    if (p[0] < 4)
        return(plErr_ArgNum);
    if (p[0]!=4+(p[3]*p[4]+3)/4)
        return(plErr_ArgNum);
    if ((pres=find_odevice(modul_vme, p[1], 0))!=plOK)
        return pres;
    wirbrauchen = 1;
    return(plOK);
}

char name_proc_vme24write[] = "vme24write";
int ver_proc_vme24write = 1;
/*****************************************************************************/
/*****************************************************************************/
