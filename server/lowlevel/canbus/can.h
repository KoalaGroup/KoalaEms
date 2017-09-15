/*
 * $ZEL: can.h,v 1.2 2007/08/07 10:59:04 wuestner Exp $
 * lowlevel/canbus/can.h
 * 
 * created: 2006-04-20 PW
 * changed: 2006-05-02 SD
 */

#ifndef _can_h_
#define _can_h_

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <emsctypes.h>
#include "../devices.h"
#include "canbus_msg.h"

enum cantypes {can_none, can_peakpci, can_can331};

struct canbus_dev {
/* this first element matches struct generic_dev */
    struct dev_generic generic;

    plerrcode (*read)(struct canbus_dev*, struct can_rd_msg* msg,
        int timeout/*ms*/);
    plerrcode (*write)(struct canbus_dev*, struct can_msg *msg);
    plerrcode (*write_read)(struct canbus_dev*, struct can_msg *wmsg,
        struct can_rd_msg *rmsg, int timeout/*ms*/);
    plerrcode (*write_read_bulk)(struct canbus_dev*,
        int num_write, struct can_msg *wmsg,
        int* num_read, struct can_rd_msg *rmsg,
        int timeout/*ms*/);
    int (*unsol)(struct canbus_dev*, int onoff);
    int (*debug)(struct canbus_dev*, int onoff);

/*
 * DUMMY must be the last 'function'; it is used to check whether all other
 * function pointers are initialised.
 */
    int (*DUMMY)(void);

    enum cantypes cantype;
    int send_unsol;
    int do_debug;
    char* pathname;
    void* info;
};

#endif
