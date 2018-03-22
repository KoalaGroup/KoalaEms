/*
 * procs/hlral/hlraltest.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: hlraltest.c,v 1.5 2017/10/09 21:25:37 wuestner Exp $";

#include <sys/types.h>
#include <cdefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../../lowlevel/hlral/hlralreg.h"
#include "../../lowlevel/hlral/hlral_int.h"
#include "../../lowlevel/devices.h"
#include "../procs.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/hlral")

/*****************************************************************************/
/*
 * p[0]: number of args (==3)
 * p[1]: board
 * p[2]: number of bytes to be written
 * p[3]: number of bytes to be read
 */
plerrcode
proc_HLRALtest_io(ems_u32* p)
{
    struct hlral* dev;
    ems_u8* buffer;
    int max, res, i;

    dev=get_pcihl_device(p[1]);
    max=p[2]>p[3]?p[2]:p[3];
    buffer=malloc(max);
    if (!buffer) return plErr_System;

    for (i=0; i<max; i++) buffer[i]=i;
    res=pcihotlink_write(dev, buffer, p[2]);
    if (res!=p[2]) {
        printf("HLRALtest_io: write failed, res=%d errno=%d\n", res, errno);
        return plErr_System;
    }
    for (i=0; i<max; i++) buffer[i]=0xa5;
    res=pcihotlink_read(dev, buffer, p[3]);
    if (res!=p[3]) {
        if (res<0) {
            printf("HLRALtest_io: read failed, res=%d errno=%d\n", res, errno);
            return plErr_System;
        } else {
            printf("HLRALtest_io: read: res=%d\n", res);
        }
    }
    for (i=0; i<res; i++) {
        if (buffer[i]!=(i&0xff)) printf("[%d]: %02x=>%02x\n", i, i&0xff, buffer[i]);
    }
    *outptr+=res;
    return plOK;
}

plerrcode
test_proc_HLRALtest_io(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3) return (plErr_ArgNum);
    if ((pres=find_odevice(modul_pcihl, p[1], 0))!=plOK)
        return pres;
    return plOK;
}

char name_proc_HLRALtest_io[] = "HLRALtest_io";

int ver_proc_HLRALtest_io = 1;
/*****************************************************************************/
plerrcode
proc_HLRALtest_dma(ems_u32* p)
{
    return plOK;
}

plerrcode
test_proc_HLRALtest_dma(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3) return (plErr_ArgNum);
    if ((pres=find_odevice(modul_pcihl, p[1], 0))!=plOK)
        return pres;
    return plOK;
}

char name_proc_HLRALtest_dma[] = "HLRALtest_dma";

int ver_proc_HLRALtest_dma = 1;
/*****************************************************************************/
/*
 * p[0]: argcount>=1
 * p[1]: board
 * p[2]: no of bytes to be read
 *
 * outptr[0]: number of bytes read
 * outptr[1..]: data
 */
plerrcode
proc_HLRALread(ems_u32* p)
{
    struct hlral* dev=get_pcihl_device(p[1]);
    int res;

    res=pcihotlink_read(dev, (ems_u8*)(outptr+1), p[2]);
    *outptr++=res;
    if (res>0)
        outptr+=(res+3)/4;
    return plOK;
}

plerrcode
test_proc_HLRALread(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_pcihl, p[1], 0))!=plOK)
        return pres;
    wirbrauchen = (p[2]+3)/4+1;
    return plOK;
}

char name_proc_HLRALread[] = "HLRALread";

int ver_proc_HLRALread = 1;
/*****************************************************************************/
/*
 * p[0]: argcount>=1
 * p[1]: board
 * p[2]: no of bytes to be written
 * p[3..]: data
 */
plerrcode
proc_HLRALwrite(ems_u32* p)
{
    struct hlral* dev=get_pcihl_device(p[1]);
    int res;

    res=pcihotlink_write(dev, (ems_u8*)(p+3), p[2]);
    *outptr++=res;

    return plOK;
}

plerrcode
test_proc_HLRALwrite(ems_u32* p)
{
    plerrcode pres;

    if (p[0]<2) 
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_pcihl, p[1], 0))!=plOK)
        return pres;
    wirbrauchen = 1;
    return plOK;
}

char name_proc_HLRALwrite[] = "HLRALwrite";

int ver_proc_HLRALwrite = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: board
 */
plerrcode
proc_HLRALstart(ems_u32* p)
{
    struct hlral* dev=get_pcihl_device(p[1]);

    if (pcihotlink_start(dev)<0)
        return plErr_System;
    else
        return plOK;
}

plerrcode
test_proc_HLRALstart(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_pcihl, p[1], 0))!=plOK)
        return pres;
    wirbrauchen = 0;
    return plOK;
}

char name_proc_HLRALstart[] = "HLRALstart";

int ver_proc_HLRALstart = 1;
/*****************************************************************************/
/*****************************************************************************/
