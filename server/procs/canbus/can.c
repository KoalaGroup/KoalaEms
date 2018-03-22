/*
 * esm/server/procs/canbus/can.c
 * created 7.1.2006 pk
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: can.c,v 1.6 2017/10/20 23:20:52 wuestner Exp $";

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../lowlevel/canbus/can.h"
#include "../procs.h"

#define get_device(bus) \
    (struct canbus_dev*)get_gendevice(modul_can, (bus))

RCS_REGISTER(cvsid, "procs/canbus")

/*****************************************************************************/
/*
 * p[0]: 3+(no. of data)
 * p[1]: bus
 * p[2]: id
 * p[3]: type 0x0: standard; 0x1: remote; 0x2: extended
 * p[4]: data...
 */
plerrcode proc_canbus_write(ems_u32* p)
{
    struct canbus_dev* dev=get_device(p[1]);
    struct can_msg msg;
    unsigned int i;

    msg.id=p[2];
    msg.msgtype=p[3];
    msg.len=p[0]-3;
    for (i=4; i<=p[0]; i++)
        msg.data[i-4]=p[i];

    return dev->write(dev, &msg);
}

plerrcode test_proc_canbus_write(ems_u32* p)
{
    plerrcode pres;

    if ((p[0]<3) || (p[0]>11))
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_can, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}

char name_proc_canbus_write[]="canbus_write";
int ver_proc_canbus_write=1;
/*****************************************************************************/
/*
 * p[0]: 3
 * p[1]: bus
 * p[2]: id -1: any identifier
 * p[3]: timeout ms
 */
/*
 * returns:
 *   id
 *   msgtype
 *   timestamp
 *   len
 *   data...
 */
plerrcode proc_canbus_read(ems_u32* p)
{
    struct canbus_dev* dev=get_device(p[1]);
    struct can_rd_msg msg;
    plerrcode pres;
    int i;

    msg.msg.id=p[2];

    pres=dev->read(dev, &msg, p[3]);
    if (pres!=plOK)
        return pres;
    *outptr++=msg.msg.len+4;
    *outptr++=msg.msg.id;
    *outptr++=msg.msg.msgtype;
    *outptr++=msg.dwTime;
    *outptr++=msg.msg.len;
    for (i=0; i<msg.msg.len; i++)
        *outptr++=msg.msg.data[i];
    return plOK;
}

plerrcode test_proc_canbus_read(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_can, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=13;
    return plOK;
}

char name_proc_canbus_read[]="canbus_read";
int ver_proc_canbus_read=1;
/*****************************************************************************/
/*
 * p[0]: 4+(no. of data)
 * p[1]: bus
 * p[2]: id
 * p[3]: type 0x0: standard; 0x1: remote; 0x2: extended
 * p[4]: timeout ms
 * p[5]: data...
 */
plerrcode proc_canbus_write_read(ems_u32* p)
{
    struct canbus_dev* dev=get_device(p[1]);
    struct can_rd_msg rmsg;
    struct can_msg wmsg;
    plerrcode pres;
    unsigned int i;
    int ii;

    wmsg.id=p[2];
    wmsg.msgtype=p[3];
    wmsg.len=p[0]-4;
    for (i=5; i<=p[0]; i++)
        wmsg.data[i-5]=p[i];
    rmsg.msg.id=p[2];

    pres=dev->write_read(dev, &wmsg, &rmsg, p[4]);
    if (pres!=plOK)
        return pres;
    *outptr++=rmsg.msg.len+4;
    *outptr++=rmsg.msg.id;
    *outptr++=rmsg.msg.msgtype;
    *outptr++=rmsg.dwTime;
    *outptr++=rmsg.msg.len;
    for (ii=0; ii<rmsg.msg.len; ii++)
        *outptr++=rmsg.msg.data[ii];
    return plOK;
}

plerrcode test_proc_canbus_write_read(ems_u32* p)
{
    plerrcode pres;

    if ((p[0]<4) || (p[0]>12))
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_can, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=13;
    return plOK;
}

char name_proc_canbus_write_read[]="canbus_write_read";
int ver_proc_canbus_write_read=1;
/*****************************************************************************/
/*
 * p[0]: 4+(no. of data)
 * p[1]: bus
 * p[2]: id
 * p[3]: type 0x0: standard; 0x1: remote; 0x2: extended
 * p[4]: timeout ms
 * p[5]: # of expected returns
 * p[6]: return_ids
 * p[7..]: data...
 */
plerrcode proc_canbus_write_read_burst(ems_u32* p)
{
    struct canbus_dev* dev=get_device(p[1]);
    struct can_msg wmsg;
    struct can_rd_msg *rmsg;
    plerrcode pres;
    unsigned i;
    int ii, j, nread;

    rmsg=malloc(p[5]*sizeof(struct can_rd_msg));
    if (!rmsg) {
        printf("proc_canbus_write_read_burst: %s\n", strerror(errno));
        return plErr_NoMem;
    }

    for (i=0; i<p[5]; i++)
        rmsg[i].msg.id=p[6];

    wmsg.id=p[2];
    wmsg.msgtype=p[3];
    wmsg.len=p[0]-6;
    for (i=7; i<=p[0]; i++)
        wmsg.data[i-7]=p[i];

    nread=p[5];
    pres=dev->write_read_bulk(dev, 1, &wmsg, &nread, rmsg, p[4]);
    if (pres==plErr_Timeout)
        pres=plOK;
    if (pres!=plOK)
        return pres;

    *outptr++=nread;
    for (ii=0; ii<nread; ii++) {
        *outptr++=rmsg[ii].msg.len+4;
        *outptr++=rmsg[ii].msg.id;
        *outptr++=rmsg[ii].msg.msgtype;
        *outptr++=rmsg[ii].dwTime;
        *outptr++=rmsg[ii].msg.len;
        for (j=0; j<rmsg[ii].msg.len; j++)
            *outptr++=rmsg[ii].msg.data[j];
    }

    free(rmsg);
    return plOK;
}

plerrcode test_proc_canbus_write_read_burst(ems_u32* p)
{
    plerrcode pres;

    if ((p[0]<6) || (p[0]>14))
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_can, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=p[5]*10;
    return plOK;
}

char name_proc_canbus_write_read_burst[]="canbus_write_read_burst";
int ver_proc_canbus_write_read_burst=1;
/*****************************************************************************/
/*
 * p[0]: 2
 * p[1]: bus
 * p[2]: 0: on 1: off -1: return only old value
 */
plerrcode proc_canbus_unsol(ems_u32* p)
{
    struct canbus_dev* dev=get_device(p[1]);

    *outptr++=dev->unsol(dev, p[2]);
    return plOK;
}

plerrcode test_proc_canbus_unsol(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_can, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=1;
    return plOK;
}

char name_proc_canbus_unsol[]="canbus_unsol";
int ver_proc_canbus_unsol=1;
/*****************************************************************************/
/*
 * p[0]: 2
 * p[1]: bus
 * p[2]: 0: on 1: off -1: return only old value
 */
plerrcode proc_canbus_debug(ems_u32* p)
{
    struct canbus_dev* dev=get_device(p[1]);

    *outptr++=dev->debug(dev, p[2]);
    return plOK;
}

plerrcode test_proc_canbus_debug(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_can, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=1;
    return plOK;
}

char name_proc_canbus_debug[]="canbus_debug";
int ver_proc_canbus_debug=1;
/*****************************************************************************/
/*****************************************************************************/
