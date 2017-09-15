/*
 * ems/server/procs/hv/iseghv/iseghv.c
 * created 24.11.2005
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: iseghv.c,v 1.10 2011/04/06 20:30:33 wuestner Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../procprops.h"
#include "../../../lowlevel/canbus/can.h"
#include "../../../lowlevel/devices.h"
#include "../../../objects/domain/dom_ml.h"

extern ems_u32* outptr;
extern int wirbrauchen;

#define get_device(bus) \
    (struct canbus_dev*)get_gendevice(modul_can, (bus))

RCS_REGISTER(cvsid, "procs/hv/iseghv")

/*****************************************************************************/
/*
 * p[0]: argnum
 * p[1]: bus
 * p[2]: data_id|(ext<<8)
 * p[3...]: data bytes 1..7
 */
plerrcode proc_iseghv_nmt(ems_u32* p)
{
    struct canbus_dev* dev=get_device(p[1]);
    struct can_msg msg;
    /*int direction=0*/;
    int data_id=p[2]&0xff;
    int ext=(p[2]>>8)&1;
    int i;
    plerrcode pres;

    msg.id=4|ext<<1;
    msg.msgtype=0;
    msg.len=p[0]-1;

    msg.data[0]=data_id;
    for (i=3; i<=p[0]; i++)
        msg.data[i-2]=p[i];

    pres=dev->write(dev, &msg);

    return pres;
}

plerrcode test_proc_iseghv_nmt(ems_u32* p)
{
    plerrcode pres;

    if (p[0]<2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_can, p[1], 0))!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_iseghv_nmt[]="iseghv_nmt";
int ver_proc_iseghv_nmt=1;
/*****************************************************************************/
/*
 * p[0]: argnum
 * p[1]: bus
 * p[2]: module_id ( 0 to 63)
 * p[3]: data_id|(ext<<8)
 * p[4...]: data bytes 1..7
 */
plerrcode proc_iseghv_write(ems_u32* p)
{
    struct canbus_dev* dev=get_device(p[1]);
    struct can_msg msg;
    int module_id=p[2]&0x3f;
    /*int direction=0*/;
    int data_id=p[3]&0xff;
    int ext=(p[3]>>8)&1;
    int i;
    plerrcode pres;

    msg.id=1<<9|module_id<<3|ext<<1;
    msg.msgtype=0;
    msg.len=p[0]-2;

    msg.data[0]=data_id;
    for (i=4; i<=p[0]; i++)
        msg.data[i-3]=p[i];

    pres=dev->write(dev, &msg);

    return pres;
}

plerrcode test_proc_iseghv_write(ems_u32* p)
{
    plerrcode pres;

    if (p[0]<3)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_can, p[1], 0))!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_iseghv_write[]="iseghv_write";
int ver_proc_iseghv_write=1;
/*****************************************************************************/
/*
 * p[0]: argnum
 * p[1]: bus
 * p[2]: module_id ( 0 to 63)
 * p[3]: data_id|(ext<<8)
 */
/*
 * returns:
 *   number of data bytes
 *   up to 7 data bytes (all other things are known from the request)
 */
plerrcode proc_iseghv_read(ems_u32* p)
{
    struct canbus_dev* dev=get_device(p[1]);
    struct can_msg wmsg;
    struct can_rd_msg rmsg;
    int module_id=p[2]&0x3f;
    /*int direction=1;*/
    int data_id=p[3]&0xff;
    int ext=(p[3]>>8)&1;
    int i;
    plerrcode pres;

    wmsg.id=1<<9|module_id<<3|ext<<1|1;
    wmsg.msgtype=0;
    wmsg.len=1;

    wmsg.data[0]=data_id;

    rmsg.msg.id=wmsg.id&~1; /* same ID with direction bit reset */

    pres=dev->write_read(dev, &wmsg, &rmsg, 100);
    if (pres!=plOK)
        return pres;

    *outptr++=rmsg.msg.len-1;
    for (i=1; i<rmsg.msg.len; i++)
        *outptr++=rmsg.msg.data[i];
    return plOK;
}

plerrcode test_proc_iseghv_read(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_can, p[1], 0))!=plOK)
        return pres;

    wirbrauchen=8;
    return plOK;
}

char name_proc_iseghv_read[]="iseghv_read";
int ver_proc_iseghv_read=1;
/*****************************************************************************/
/*
 * p[0]: argnum
 * p[1]: bus
 * p[2]: module_id ( 0 to 63)
 * p[3]: data byte0
 */
#define NREAD 128 /* maximum number of frames to be received */
plerrcode proc_iseghv_logon(ems_u32* p)
{
    struct canbus_dev* dev=get_device(p[1]);
    struct can_msg wmsg;
    struct can_rd_msg rmsg[NREAD];
    int nread=NREAD, i;
    plerrcode pres;

    wmsg.id=1<<9|p[2]<<3|0<<2|0<<1|0;
    wmsg.msgtype=0;
    wmsg.len=2;
    wmsg.data[0]=0xd8; /* log-on/log-off */
    wmsg.data[1]=p[3]; /* connect or disconnect */

    for (i=0; i<NREAD; i++) {
        rmsg[i].msg.id=-1;
    }

    pres=dev->write_read_bulk(dev, 1, &wmsg, &nread, rmsg, 2000);
    if (pres==plErr_Timeout)
        pres=plOK;
    if (pres!=plOK)
        return pres;

    for (i=0; i<nread; i++) {
        struct can_msg *msg=&rmsg[i].msg;
        if (((msg->id&0x607)==0x201)&&(msg->len==3)&&(msg->data[0]==0xd8)) {
            int idx=(msg->id>>3)&0x3f;
            if ((idx==p[2]) && (msg->data[0]==0xd8)) {
                *outptr++=msg->len-1;
                for (i=1; i<msg->len; i++)
                    *outptr++=msg->data[i];
                return plOK;
            }
        }
    }

    return plErr_Timeout;
}

plerrcode test_proc_iseghv_logon(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_can, p[1], 0))!=plOK)
        return pres;

    wirbrauchen=2;
    return plOK;
}

char name_proc_iseghv_logon[]="iseghv_logon";
int ver_proc_iseghv_logon=1;
/*****************************************************************************/
/*
 * parameter
 * p[0]: argnum==1
 * p[1]: bus
 */
#define NREAD 128 /* maximum number of frames to be received */
plerrcode proc_iseghv_enumerate(ems_u32* p) {
    struct canbus_dev* dev=get_device(p[1]);
    struct can_msg wmsg[64];
    struct can_rd_msg rmsg[NREAD];
    int nread=NREAD, ndev, i;
    plerrcode pres;
    struct device {
        int class;
        int status;
    } devices[64];

    for (i=0; i<64; i++) {
        wmsg[i].id=1<<9|i<<3|0<<2|0<<1|0;
        wmsg[i].msgtype=0;
        wmsg[i].len=2;
        wmsg[i].data[0]=0xd8; /* log-on/log-off */
        wmsg[i].data[1]=0;    /* disconnect */
    }
    for (i=0; i<NREAD; i++) {
        rmsg[i].msg.id=-1;
    }

    pres=dev->write_read_bulk(dev, 64, wmsg, &nread, rmsg, 2000);
    if (pres==plErr_Timeout)
        pres=plOK;
    if (pres!=plOK)
        return pres;

    for (i=0; i<64; i++)
        devices[i].class=-1;

    for (i=0; i<nread; i++) {
        struct can_msg *msg=&rmsg[i].msg;
        if (((msg->id&0x607)==0x201)&&(msg->len==3)&&(msg->data[0]==0xd8)) {
            int idx=(msg->id>>3)&0x3f;
            devices[idx].status=msg->data[1];
            devices[idx].class=msg->data[2];
        }
    }

    ndev=0;
    for (i=0; i<64; i++) {
        if (devices[i].class!=-1)
            ndev++;
    }
    *outptr++=ndev;
    for (i=0; i<64; i++) {
        if (devices[i].class!=-1) {
            *outptr++=i;
            *outptr++=devices[i].class;
            *outptr++=devices[i].status;
        }
    }
    return plOK;
}

plerrcode test_proc_iseghv_enumerate(ems_u32* p){
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_can, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=193;
    return plOK;
}

char name_proc_iseghv_enumerate[]="iseghv_enumerate";
int ver_proc_iseghv_enumerate=1;
/*****************************************************************************/
/*****************************************************************************/
