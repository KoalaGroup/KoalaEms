/*
 * $ZEL: canbus.h,v 1.2 2006/08/04 19:09:54 wuestner Exp $
 * lowlevel/can/canbus.h
 * 
 * created: 2006-04-20 PW
 * changed: 2006-05-02 SD
 */

#ifndef _canbus_h_
#define _canbus_h_

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <emsctypes.h>
#include "../devices.h"
#include "canbus_msg.h"

/* maximum device number */
#ifndef MAX_CAN_DEVICES
#define MAX_CAN_DEVICES 64
#endif

struct can_device {
    int found; 
};

enum cantypes {can_none, can_peakpci, can_can331};

#if 0
struct can_message { /* similar to pcan.h::TPCANMsg */
  uint32_t id;              /* 11/29 bit code             */
  uint8_t  msgtype;         /* bits of MSGTYPE_*          */
  uint8_t  len;             /* count of data bytes (0..8) */
  uint8_t  data[8];         /* data bytes, up to 8        */
};

struct can_rd_message { /* similar to pcan.h::TPCANRdMsg */
    struct can_message message;
    uint32_t dwTime;
};
#endif

struct can_dev {
/* this first element matches struct generic_dev */
    struct dev_generic generic;

    int (*CAN_Read)(struct can_dev*, TPCANRdMsg* msg);
    int (*CAN_Search_Dev)(struct can_dev*);
    int (*CAN_ReadWrite_Short)(struct can_dev*, int data_dir, int id,
        int data_id, int len, int *buf, TPCANRdMsg*msg);
    int (*CAN_ReadWrite)(struct can_dev*, int data_dir, int ext_instr, int nmt,
        int id, int active, int data_id, int len, int *buf,
        TPCANRdMsg*msg);
    int (*CAN_ReadWrite_ID)(struct can_dev*, int identifier, int data_id,
        int len, int *buf, TPCANRdMsg*msg);
    int (*CAN_Found_Dev)(struct can_dev*, int *anz, int **devv);
    int (*CAN_Send)(struct can_dev*, int data_dir, int ext_instr, int nmt,
        int id, int active, int data_id, int len, int *buf);
    int (*CAN_Send_ID)(struct can_dev*, int identifier, int data_id, int len,
        int *buf);
    int (*CAN_Send_Short)(struct can_dev*, int data_dir, int id, int data_id,
        int len, int *buf);
    void (*CAN_ClearBuf)(struct can_dev*);
    
    
/*
 * DUMMY must be the last 'function'; it is used to check whether all other
 * function pointers are initialised.
 */
    int (*DUMMY)(void);

    struct can_device found_devices[MAX_CAN_DEVICES];

    enum cantypes cantype;
    void* info;
};

/*plerrcode can_write(int ID, ems_u32 *buff, int len);*/

#endif
