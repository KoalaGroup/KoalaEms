/*
 * common/msg.h
 * 
 * created 12.11.94
 * 02.Mar.2004 msgheader elements changed to ems_u32
 * 
 * $ZEL: msg.h,v 2.6 2004/11/26 14:37:50 wuestner Exp $
 */

#ifndef _msg_h_
#define _msg_h_

#include <requesttypes.h>
#include <intmsgtypes.h>
#include <unsoltypes.h>
#include "emsctypes.h"

union Reqtype {
    Request reqtype;
    IntMsg intmsgtype;
    UnsolMsg unsoltype;
    ems_u32 generic;
};

typedef struct {
    ems_u32 size;
    ems_u32 client;
    ems_u32 ved;
    union Reqtype type;
    ems_u32 flags;
    ems_u32 transid;
} msgheader;

#define headersize (sizeof(msgheader)/sizeof(int))

#define Flag_Confirmation 0x1
#define Flag_Intmsg 0x2
#define Flag_Unsolicited 0x4
#define Flag_NoLog 0x8
#define Flag_IdGlobal 0x20

#define EMS_commu   (ems_u32)-2 /* id of commu-process */
#define EMS_invalid (ems_u32)-1 /* if id invalid */
#define EMS_unknown (ems_u32)-3 /* if id unknown */

#endif
/*****************************************************************************/
/*****************************************************************************/
