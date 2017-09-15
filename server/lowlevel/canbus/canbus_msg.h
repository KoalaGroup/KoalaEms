/*
 * $ZEL: canbus_msg.h,v 1.1 2007/03/02 22:52:47 wuestner Exp $
 * lowlevel/canbus/canbus_msg.h
 */

#ifndef _canbus_msg_h_
#define _canbus_msg_h_

#include <stdint.h>

struct can_msg {
    unsigned int id;    /* 11/29 bit message identifier                  */
    int  msgtype;       /* 0x0: standard 0x1: remote 0x2: extended frame */
    int  len;           /* count of data bytes (0..8)                    */
    ems_u8 data[8];     /* data bytes, up to 8                           */
};

struct can_rd_msg {
  struct can_msg msg;   /* the above message   */
  ems_u32 dwTime;       /* a timestamp in msec */
};

#endif
