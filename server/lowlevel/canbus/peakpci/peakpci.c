/*
 * lowlevel/canbus/peakpci/peakpci.c
 * 
 * PEAK CANbus interface
 * 
 * Autor: Sebastian Detert
 * 
 * created Feb. 2006
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: peakpci.c,v 1.3 2011/04/06 20:30:22 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <emsctypes.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../main/scheduler.h"
#include <../../../commu/commu.h>
#include "../canbus_init.h"
#include "pcan.h"

#define CAN_BAUD_1M     0x0014  //   1 MBit/s
#define CAN_BAUD_500K   0x001C  // 500 kBit/s
#define CAN_BAUD_250K   0x011C  // 250 kBit/s
#define CAN_BAUD_125K   0x031C  // 125 kBit/s
#define CAN_BAUD_100K   0x432F  // 100 kBit/s
#define CAN_BAUD_50K    0x472F  //  50 kBit/s
#define CAN_BAUD_20K    0x532F  //  20 kBit/s
#define CAN_BAUD_10K    0x672F  //  10 kBit/s
#define CAN_BAUD_5K     0x7F7F  //   5 kBit/s

#define USE_BAUD CAN_BAUD_125K

struct can_peakpci_info {
    int p;
    struct seltaskdescr* st;
};

RCS_REGISTER(cvsid, "lowlevel/canbus/peakpci")

/*****************************************************************************/
static void
tvdiff(struct timeval *a, struct timeval *b, struct timeval *d)
{
    d->tv_usec=a->tv_usec-b->tv_usec;
    d->tv_sec=a->tv_sec-b->tv_sec;
    if (d->tv_usec<0) {
        d->tv_usec+=1000000;
        d->tv_sec--;
    }
}
/*****************************************************************************/
#if 0
static void
printtime(const char* text, struct timeval *t)
{
    printf("%s: T=%10ld.%06ld\n", text, t->tv_sec, t->tv_usec);
}
#endif
/*****************************************************************************/
static void
dump_can_message(struct canbus_dev* dev, const char* text, const char* caller,
        TPCANMsg *msg)
{
    if (dev->do_debug) {
        int i;

        printf("%s (%s): id=0x%x type=0x%x len=%d\n", text, caller,
            msg->ID, msg->MSGTYPE, msg->LEN);
        for (i=0; i<msg->LEN; i++) {
            printf("%02x ", msg->DATA[i]);
        }
        printf("\n");
    }
}
/*****************************************************************************/
static void
dump_can_message_(struct canbus_dev* dev, const char* text, const char* caller,
        struct can_msg *msg)
{
    if (dev->do_debug) {
        int i;

        printf("%s (%s): id=0x%x type=0x%x len=%d\n", text, caller,
            msg->id, msg->msgtype, msg->len);
        for (i=0; i<msg->len; i++) {
            printf("%02x ", msg->data[i]);
        }
        printf("\n");
    }
}
/*****************************************************************************/
static void
peakpci_unexpected_message(struct canbus_dev* dev, TPCANRdMsg *buf,
        int dump, const char* caller)
{
    if (dump)
        dump_can_message(dev, "unexpected_message", caller, &buf->Msg);
    if (dev->send_unsol) {
        ems_u32 data[12];
        int i;

        data[0]=1; /*HeartBeat_canbus*/
        data[1]=buf->Msg.ID;
        data[2]=buf->Msg.MSGTYPE;
        data[3]=buf->Msg.LEN;
        for (i=0; i<=buf->Msg.LEN; i++)
            data[4+i]=buf->Msg.DATA[i];
        if (send_unsolicited(Unsol_HeartBeat, data, 4+buf->Msg.LEN)<0)
            dev->send_unsol=0;
    }
}
/*****************************************************************************/
static plerrcode
peakpci_read_(struct canbus_dev* dev, struct can_rd_msg *msg,
    struct timeval *stop)
{
    struct can_peakpci_info* info=(struct can_peakpci_info*)dev->info;
    TPCANRdMsg buf;
    struct timeval now, diff;
    int id=msg->msg.id;

    gettimeofday(&now, 0);
    tvdiff(stop, &now, &diff);
    /* execute loop at least one time even if timeout==0 and
       stop-now is negative */
    if (diff.tv_sec<0) {
        diff.tv_sec=0;
        diff.tv_usec=0;
    }

    while (diff.tv_sec>=0) {
        fd_set readfd;
        int res;

        FD_ZERO(&readfd);
        FD_SET(info->p, &readfd);
        res=select(info->p+1, &readfd, 0, 0, &diff);
        if ((res<0)&&(errno==EINTR))
            res=0;
        if (res<0) {
            printf("peakpci_read: select: %s\n", strerror(errno));
            return plErr_System;
        }
        if (res>0) {
            if (ioctl(info->p, PCAN_READ_MSG, &buf)<0) {
                printf("peakpci_read: %s\n", strerror(errno));
                return plErr_System;
            }
            if ((id>=0)&&(id!=buf.Msg.ID)) {
                peakpci_unexpected_message(dev, &buf, 1, "peakpci_read_");
            } else {
                int i;
                dump_can_message(dev, "  expected_message", "peakpci_read_",
                    &buf.Msg);
                msg->msg.id=buf.Msg.ID;
                msg->msg.msgtype=buf.Msg.MSGTYPE;
                msg->msg.len=buf.Msg.LEN;
                for (i=0; i<buf.Msg.LEN; i++)
                    msg->msg.data[i]=buf.Msg.DATA[i];
                msg->dwTime=buf.dwTime;
                return plOK;
            }
        }
        gettimeofday(&now, 0);
        tvdiff(stop, &now, &diff);
    }
    return plErr_Timeout;
}
/*****************************************************************************/
static plerrcode
peakpci_read(struct canbus_dev* dev, struct can_rd_msg *msg, int timeout)
{
    struct timeval stop;

    gettimeofday(&stop, 0);
    stop.tv_usec+=timeout*1000;
    if (stop.tv_usec>=1000000) {
        stop.tv_usec-=1000000;
        stop.tv_sec++;
    }
    return peakpci_read_(dev, msg, &stop);
}
/*****************************************************************************/
static plerrcode
peakpci_write(struct canbus_dev* dev, struct can_msg *msg)
{
    struct can_peakpci_info* info=(struct can_peakpci_info*)dev->info;
    TPCANMsg buf;
    int i;

    dump_can_message_(dev, "      send message", "peakpci_write", msg);
    buf.ID=msg->id;
    buf.MSGTYPE=msg->msgtype;
    buf.LEN=msg->len;
    for (i=0; i<8; i++)
    buf.DATA[i]=msg->data[i];

    if (ioctl(info->p, PCAN_WRITE_MSG, &buf)<0) {
        printf("peakpci_write: errno=%s\n", strerror(errno));
        return plErr_System;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
peakpci_write_read(struct canbus_dev* dev, struct can_msg *wmsg,
    struct can_rd_msg *rmsg, int timeout/*ms*/)
{
    plerrcode pres;

    pres=peakpci_write(dev, wmsg);
    if (pres!=plOK)
        return pres;

    pres=peakpci_read(dev, rmsg, timeout);
    return pres;
}
/*****************************************************************************/
static plerrcode
peakpci_write_read_bulk(struct canbus_dev* dev,
        int num_write, struct can_msg *wmsg,
        int* num_read, struct can_rd_msg *rmsg,
        int timeout/*ms*/)
{
    struct timeval stop;
    plerrcode pres=plOK;
    int nread, i;

    for (i=0; i<num_write; i++) {
        pres=peakpci_write(dev, wmsg+i);
        if (pres!=plOK) {
            printf("peakpci_write_read_bulk: error with i=%d\n", i);
            return pres;
        }
    }

    gettimeofday(&stop, 0);
    stop.tv_usec+=timeout*1000;
    while (stop.tv_usec>=1000000) {
        stop.tv_usec-=1000000;
        stop.tv_sec++;
    }

    nread=*num_read;
    *num_read=0;
    while (*num_read<nread) {
        pres=peakpci_read_(dev, rmsg+*num_read, &stop);
        if (pres==plOK)
            (*num_read)++;
        else
            break;
    }

    return pres;
}
/*****************************************************************************/
static void
peakpci_message(int p, enum select_types selected, union callbackdata cbdata)
{
    struct generic_dev* gendev=(struct generic_dev*)cbdata.p;
    struct canbus_dev* dev=(struct canbus_dev*)gendev;
    /*struct can_peakpci_info* info=(struct can_peakpci_info*)&dev->info;*/
    TPCANRdMsg buf;

    if (ioctl(p, PCAN_READ_MSG, &buf)<0) {
        printf("peakpci_message: errno=%s\n", strerror(errno));
        return;
    }
    peakpci_unexpected_message(dev, &buf, 0, "peakpci_message");
}
/*****************************************************************************/
static int
peakpci_unsol(struct canbus_dev* dev, int onoff)
{
    int old=dev->send_unsol;
    if (onoff>=0)
        dev->send_unsol=onoff;
    return old;
}
/*****************************************************************************/
static int
peakpci_debug(struct canbus_dev* dev, int onoff)
{
    int old=dev->do_debug;
    if (onoff>=0)
        dev->do_debug=onoff;
    return old;
}
/*****************************************************************************/
static int
peak_can_init(struct canbus_dev* dev) {
    struct can_peakpci_info* info=(struct can_peakpci_info*)dev->info;
    TPCANInit init;

    info->p=open(dev->pathname, O_RDWR/*|O_NONBLOCK*/);
    if (info->p<0) {
        printf("peac_can_init: open \"%s\": %s\n", dev->pathname,
                strerror(errno));
        return -1;
    }
    if (fcntl(info->p, F_SETFD, FD_CLOEXEC)<0) {
        printf("fcntl(peac_can_init \"%s\", FD_CLOEXEC): %s\n", dev->pathname,
                strerror(errno));
        goto error;
    }

    init.wBTR0BTR1    = USE_BAUD; /* combined BTR0 and BTR1 register of the SJA100 */
#if 0
    init.ucCANMsgType = MSGTYPE_STANDARD;  /* 11 bits */
#else
    init.ucCANMsgType = MSGTYPE_EXTENDED;  /* 29 bits */
#endif
    init.ucListenOnly = 0;        /* listen only mode when != 0 */
    
    if (ioctl(info->p, PCAN_INIT, &init)<0) {
        printf("peac_can_init: PCAN_INIT \"%s\": %s\n", dev->pathname,
                strerror(errno));
        goto error;
    }

    return 0;
error:
    close(info->p);
    info->p=-1;
    return -1;
    
}
/*****************************************************************************/
static errcode
can_done_peakpci(struct generic_dev* gdev)
{
    struct canbus_dev* dev=(struct canbus_dev*)gdev;
    struct can_peakpci_info* info=(struct can_peakpci_info*)dev->info;

    if (info->st) {
        sched_delete_select_task(info->st);
    }
    close(info->p);
    if (dev->pathname)
        free(dev->pathname);
    free(info);
    dev->info=0;

    return OK;
}
/*****************************************************************************/
errcode
can_init_peakpci(struct canbus_dev* dev)
{
    struct can_peakpci_info* info;
    union callbackdata cbdata;
    errcode res;

    info=(struct can_peakpci_info*) calloc(1, sizeof(struct can_peakpci_info));
    if (!info) {
        printf("can_init_peakpci: calloc(info): %s\n", strerror(errno));
        return Err_NoMem;
    }
    dev->info=info;

    if (peak_can_init(dev)<0) {
        res=Err_System;
        goto error;
    }

    cbdata.p=dev;
    info->st=sched_insert_select_task(peakpci_message, cbdata,
            "peakpci", info->p, select_read
    #ifdef SELECT_STATIST
            , 1
    #endif
            );
    if (!info->st) {
        printf("can_init_peakpci: cannot install callback for message\n");
        res=Err_System;
        goto error;
    }

    dev->generic.done   =can_done_peakpci;
    dev->read           =peakpci_read;
    dev->write          =peakpci_write;
    dev->write_read     =peakpci_write_read;
    dev->write_read_bulk=peakpci_write_read_bulk;
    dev->unsol          =peakpci_unsol;
    dev->debug          =peakpci_debug;

    dev->send_unsol=0;
    dev->do_debug=0;
    dev->generic.online=1;

    return OK;

error:
    if (info->st) {
        sched_delete_select_task(info->st);
    }
    close(info->p);
    free(info);
    dev->info=0;
    return res;
}
/*****************************************************************************/
/*****************************************************************************/
