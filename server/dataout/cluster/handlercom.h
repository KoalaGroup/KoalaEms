/*
 * dataout/cluster/handlercom.h
 * 
 * created      28.04.97 PW
 * 22.07.1998 PW: hndl?_unload and hndl?_reopen added
 * 20.09.1998 PW: struct hndlr_msg introduced (istead of int[])
 * 05.02.1999 PW: hndle_init_scsi added
 * 24.04.1999 PW: hndl?_reopen removed (filemark has to be used instead)
 */
/* $ZEL: handlercom.h,v 1.4 2002/09/28 17:40:55 wuestner Exp $*/

#ifndef _handlercom_h_
#define _handlercom_h_

typedef enum {
  hndlr_none,
  hndlr_exit,
  hndlr_status,
  hndlr_write,
  hndlr_linkdata,
  hndlr_filemark,
  hndlr_wind,
  hndlr_rewind,
  hndlr_seod,
  hndlr_unload,
  hndlr_control,
  hndlr_debug,
  hndlr_version
  } hndlr_request;
typedef enum {
  hndlc_none,
  hndlc_error,
  hndlc_exit,
  hndlc_pid,
  hndlc_status,
  hndlc_write,
  hndlc_linkdata,
  hndlc_filemark,
  hndlc_wind,
  hndlc_rewind,
  hndlc_seod,
  hndlc_unload,
  hndlc_control,
  hndlc_debug,
  hndlc_version
  } hndlr_confirm;
typedef enum {
  hndle_none,
  hndle_exec,
  hndle_args,
  hndle_open,
  hndle_linkdata,
  hndle_readstatus,
  hndle_write,
  hndle_protocol,
  hndle_wind,
  hndle_rewind,
  hndle_seod,
  hndle_unload,
  hndle_control,
  hndle_init_scsi,
  hndle_status,
  hndle_version
  } hndlr_errorkeys;
typedef enum {
  hndlcode_none,
  hndlcode_Rewind,
  hndlcode_Erase,
  hndlcode_Write,
  hndlcode_WriteFileMarks,
  hndlcode_Space,
  hndlcode_Inquiry,
  hndlcode_ModeSelect,
  hndlcode_Load,
  hndlcode_Locate,
  hndlcode_LogSelect,
  hndlcode_LogSense,
  hndlcode_ModeSense,
  hndlcode_Prevent_Allow,
  hndlcode_ReadPos
  } hndlr_controlcodes;
typedef enum {
  hndlbuf_shm,
  hndlbuf_file
  } hndlr_buffertypes;

typedef struct
  {
  int size;
  int pid;
  int xid;
  union
    {
    hndlr_request req;
    hndlr_confirm conf;
    } code;
  int data[100]; /* willkuerlich, reicht aber im Moment */
    /* data[0]: [errorcode|pid|...] */
    /* data[1]: [errno|...] */
  } hndlr_msg;

#endif
