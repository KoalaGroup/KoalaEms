/*
 * esm/server/procs/caenet/caenet.c
 * created: 2007-Mar-20 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: caenet.c,v 1.7 2011/04/06 20:30:29 wuestner Exp $";

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../lowlevel/caenet/caennet.h"
#include "../procs.h"

extern ems_u32* outptr;
extern int wirbrauchen;

#define get_device(bus) \
    (struct caenet_dev*)get_gendevice(modul_caenet, (bus))

RCS_REGISTER(cvsid, "procs/caenet")

/*****************************************************************************/
/*
 * p[0]: 1+(no. of data)
 * p[1]: bus
 * p[2]: data... each word represents 16 bit
 */
plerrcode proc_caenet_write(ems_u32* p)
{
    struct caenet_dev* dev=get_device(p[1]);
    ems_u8* buf;
    plerrcode pres;
    int i, j;

    dev->buffer(dev, &buf);

    for (i=2, j=0; i<=p[0]; i++) {
        buf[j++]=p[i]&0xff;
        buf[j++]=(p[i]>>8)&0xff;
    }

    pres=dev->write(dev, buf, (p[0]-1)*2);
    return pres;
}

plerrcode test_proc_caenet_write(ems_u32* p)
{
    plerrcode pres;

    if (p[0]<1)
        return plErr_ArgNum;
    if (p[0]-1>CAENET_BLOCKSIZE/2)
        return plErr_ArgRange;
    if ((pres=find_odevice(modul_caenet, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_caenet_write[]="caenet_write";
int ver_proc_caenet_write=2;
/*****************************************************************************/
/*
 * p[0]: 1+(no. of data)
 * p[1]: bus
 * p[2]: data... each word represents 8 bit
 */
plerrcode proc_caenet_writeB(ems_u32* p)
{
    struct caenet_dev* dev=get_device(p[1]);
    ems_u8* buf;
    plerrcode pres;
    int i;

    dev->buffer(dev, &buf);

    for (i=2; i<=p[0]; i++)
        buf[i-2]=p[i];

    pres=dev->write(dev, buf, p[0]-1);
    return pres;
}

plerrcode test_proc_caenet_writeB(ems_u32* p)
{
    plerrcode pres;

    if (p[0]<1)
        return plErr_ArgNum;
    if (p[0]-1>CAENET_BLOCKSIZE)
        return plErr_ArgRange;
    if ((pres=find_odevice(modul_caenet, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_caenet_writeB[]="caenet_writeB";
int ver_proc_caenet_writeB=1;
/*****************************************************************************/
/*
 * p[0]: 1|2
 * p[1]: bus
 * [p[2]: max_words]
 */
plerrcode proc_caenet_read(ems_u32* p)
{
    struct caenet_dev* dev=get_device(p[1]);
    ems_u8* buf;
    int len=p[2], i;
    plerrcode pres;

    dev->buffer(dev, &buf);

    len=p[0]>1?p[2]*2:CAENET_BLOCKSIZE;
    pres=dev->read(dev, buf, &len);
    if (pres==plOK) {
        if (len&1)
            return plErr_Overflow;
        *outptr++=len/2;
        for (i=0; i<len; i+=2) {
            *outptr++=buf[i]+(buf[i+1]<<8);
        }
    }
    return pres;
}

plerrcode test_proc_caenet_read(ems_u32* p)
{
    plerrcode pres;

    if (p[0]<1 || p[0]>2)
        return plErr_ArgNum;
    if (p[0]>1) {
        if (p[2]>CAENET_BLOCKSIZE/2)
            return plErr_ArgRange;
    }
    if ((pres=find_odevice(modul_caenet, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=p[0]>1?p[2]:CAENET_BLOCKSIZE;
    return plOK;
}

char name_proc_caenet_read[]="caenet_read";
int ver_proc_caenet_read=2;
/*****************************************************************************/
/*
 * p[0]: 1|2
 * p[1]: bus
 * [p[2]: max_bytes]
 */
plerrcode proc_caenet_readB(ems_u32* p)
{
    struct caenet_dev* dev=get_device(p[1]);
    ems_u8* buf;
    int len=p[2], i;
    plerrcode pres;

    dev->buffer(dev, &buf);

    len=p[0]>1?p[2]:CAENET_BLOCKSIZE;
    pres=dev->read(dev, buf, &len);
    if (pres==plOK) {
        *outptr++=len;
        for (i=0; i<len; i++)
            *outptr++=buf[i];
    }
    return pres;
}

plerrcode test_proc_caenet_readB(ems_u32* p)
{
    plerrcode pres;

    if (p[0]<1 || p[0]>2)
        return plErr_ArgNum;
    if (p[0]>1) {
        if (p[2]>CAENET_BLOCKSIZE)
            return plErr_ArgRange;
    }
    if ((pres=find_odevice(modul_caenet, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=p[0]>1?p[2]:CAENET_BLOCKSIZE;
    return plOK;
}

char name_proc_caenet_readB[]="caenet_readB";
int ver_proc_caenet_readB=1;
/*****************************************************************************/
/*
 * p[0]: 1+(no. of data)
 * p[1]: bus
 * p[x]: data... each word represents 16 bit
 */
plerrcode proc_caenet_write_read(ems_u32* p)
{
    struct caenet_dev* dev=get_device(p[1]);
    ems_u8* buf;
    plerrcode pres;
    int len, i, j;

    dev->buffer(dev, &buf);

    for (i=2, j=0; i<=p[0]; i++) {
        buf[j++]=p[i]&0xff;
        buf[j++]=(p[i]>>8)&0xff;
    }

    len=CAENET_BLOCKSIZE;
    if ((pres=dev->write_read(dev, buf, (p[0]-1)*2, buf, &len))!=plOK)
        return pres;
    if (len&1)
        return plErr_Overflow;
    *outptr++=len/2;
    for (i=0; i<len; i+=2)
        *outptr++=buf[i]+(buf[i+1]<<8);

    return plOK;
}

plerrcode test_proc_caenet_write_read(ems_u32* p)
{
    plerrcode pres;

    if ((p[0]<2) || (p[0]-1>CAENET_BLOCKSIZE/2))
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_caenet, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=CAENET_BLOCKSIZE;
    return plOK;
}

char name_proc_caenet_write_read[]="caenet_write_read";
int ver_proc_caenet_write_read=2;
/*****************************************************************************/
/*
 * p[0]: 1+(no. of data)
 * p[1]: bus
 * p[x]: data...
 */
plerrcode proc_caenet_write_readB(ems_u32* p)
{
    struct caenet_dev* dev=get_device(p[1]);
    ems_u8* buf;
    plerrcode pres;
    int len, i;

    dev->buffer(dev, &buf);

    for (i=2; i<=p[0]; i++)
        buf[i-2]=p[i];

    len=CAENET_BLOCKSIZE;
    if ((pres=dev->write_read(dev, buf, p[0]-1, buf, &len))!=plOK)
        return pres;
    *outptr++=len;
    for (i=0; i<len; i++)
        *outptr++=buf[i];

    return plOK;
}

plerrcode test_proc_caenet_write_readB(ems_u32* p)
{
    plerrcode pres;

    if ((p[0]<2) || (p[0]-1>CAENET_BLOCKSIZE))
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_caenet, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=CAENET_BLOCKSIZE;
    return plOK;
}

char name_proc_caenet_write_readB[]="caenet_write_readB";
int ver_proc_caenet_write_readB=1;
/*****************************************************************************/
/*
 * p[0]: 1
 * p[1]: bus
 */
plerrcode proc_caenet_echo(ems_u32* p)
{
    struct caenet_dev* dev=get_device(p[1]);
    int num;
    plerrcode pres;


    pres=dev->echo(dev, &num);
    if (pres==plOK)
        *outptr++=num;
    return pres;
}

plerrcode test_proc_caenet_echo(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_caenet, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=1;
    return plOK;
}

char name_proc_caenet_echo[]="caenet_echo";
int ver_proc_caenet_echo=1;
/*****************************************************************************/
/*
 * p[0]: 2
 * p[1]: bus
 * p[2]: timeout
 */
plerrcode proc_caenet_timeout(ems_u32* p)
{
    struct caenet_dev* dev=get_device(p[1]);

    return dev->timeout(dev, p[2]);
}

plerrcode test_proc_caenet_timeout(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_caenet, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_caenet_timeout[]="caenet_timeout";
int ver_proc_caenet_timeout=1;
/*****************************************************************************/
/*
 * p[0]: 2
 * p[1]: bus
 */
plerrcode proc_caenet_led(ems_u32* p)
{
    struct caenet_dev* dev=get_device(p[1]);

    return dev->led(dev);
}

plerrcode test_proc_caenet_led(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_caenet, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_caenet_led[]="caenet_led";
int ver_proc_caenet_led=1;
/*****************************************************************************/
/*
 * p[0]: 2
 * p[1]: bus
 */
plerrcode proc_caenet_reset(ems_u32* p)
{
    struct caenet_dev* dev=get_device(p[1]);

    return dev->reset(dev);
}

plerrcode test_proc_caenet_reset(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_caenet, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_caenet_reset[]="caenet_reset";
int ver_proc_caenet_reset=1;
/*****************************************************************************/
/*****************************************************************************/
