#ifndef _canbus_msg_h_
#define _canbus_msg_h_

#include <stdint.h>

typedef struct 
{
  uint32_t ID;              // 11/29 bit code
  uint8_t  MSGTYPE;         // bits of MSGTYPE_*
  uint8_t  LEN;             // count of data bytes (0..8)
  uint8_t  DATA[8];         // data bytes, up to 8
} TPCANMsg;              // for PCAN_WRITE_MSG

typedef struct
{
  TPCANMsg Msg;          // the above message
  uint32_t    dwTime;       // a timestamp in msec, read only
} TPCANRdMsg;            // for PCAN_READ_MSG

#endif
