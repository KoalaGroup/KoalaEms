/* $ZEL: scsicompat.h,v 1.4 2002/09/28 17:41:02 wuestner Exp $ */
#ifndef _scsicompat_h_
#define _scsicompat_h_

#include <sys/types.h>

#undef OS_OK

#ifdef SCSICOMPAT_bsd
#ifdef OS_OK
#error A only one system for scsicompat allowed
#endif
#include "bsd/scsicompat_bsd.h"
#define OS_OK
#endif

#ifdef SCSICOMPAT_osf
#ifdef OS_OK
#error B only one system for scsicompat allowed
#endif
#include "osf/scsicompat_osf.h"
#define OS_OK
#endif

#ifdef SCSICOMPAT_linux
#ifdef OS_OK
#error C only one system for scsicompat allowed
#endif
#include "linux/scsicompat_linux.h"
#define OS_OK
#endif

#ifdef SCSICOMPAT_lynx
#ifdef OS_OK
#error D only one system for scsicompat allowed
#endif
#include "lynx/scsicompat_lynx.h"
#endif

#ifndef OS_OK
#error unsupported OS for scsicompat.h
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ 
#endif

typedef struct
  {
  int error_code;
  int sense_key;
  int add_sense_code;
  int add_sense_code_qual;
  int code_data;
  int field_ptr;
  int valid;
  int infobytes;
  int bpv;
  int bitptr;
  int filemark;
  int eom;
  } scsi_error_info;

typedef struct
  {
  int filemarks;
  int records;
  int sw_position;
  } tape_sanity;

typedef struct
  {
  int ignore_medium_changed;
  } scsi_policy;

typedef struct 
  {
  char vendorname[9];
  char devicename[17];
  char revision[5];
  int device_type;
  } scsi_inquiry_data;

typedef struct raw_scsi_var
  {
  raw_scsi_info x;
  scsi_error_info scsi_error;
  scsi_inquiry_data inquiry_data;
  int (*stat_proc)(struct raw_scsi_var* device, char* name, int* m);
  tape_sanity sanity;
  scsi_policy policy;
  } raw_scsi_var;

typedef struct
  {
  int bop;  /* begin of partition */
  int eop;  /* end of partition */
  int bpu;  /* block position unknown */
  int part; /* partition */
  int fbl;  /* first block location */
  int lbl;  /* last block location */
  int blib; /* number of blocks in buffer */
  int byib; /* number of bytes in buffer */
  } scsi_position_data;

raw_scsi_var* scsicompat_init(char* filename, int pid);
int scsicompat_init_(char* filename, raw_scsi_var* var, int pid);
int scsicompat_done(raw_scsi_var* var);
int scsicompat_done_(raw_scsi_var* var);

/*01*/ int scsi_rewind(raw_scsi_var* var, int immediate);
/*03*/ int scsi_request_sense(raw_scsi_var* var, unsigned char** obuf,
    int* len);
/*05*/ int scsi_read_block_limits(raw_scsi_var* var, unsigned char** obuf,
    int* len);
/*0a*/ int scsi_write(raw_scsi_var* var, int num, int* data);
/*10*/ int scsi_write_filemark(raw_scsi_var* var, int num, int immediate);
/*11*/ int scsi_space(raw_scsi_var* var, int code, int count);
/*12*/ int scsi_inquiry(raw_scsi_var* var, unsigned char** obuf, int* len);
/*15*/ int scsi_mode_select(raw_scsi_var* var, int density);
/*16*/ int scsi_reserve_unit(raw_scsi_var* var, int thirt_party);
/*17*/ int scsi_release_unit(raw_scsi_var* var, int thirt_party);
/*19*/ int scsi_erase(raw_scsi_var* var, int immediate, int extend);
/*1a/5a*/int scsi_mode_sense(raw_scsi_var* var, int page_control,
    int page_code, unsigned char** obuf, int* len);
/*1b*/ int scsi_load_unload(raw_scsi_var* var, int load, int immediate);
/*1e*/ int scsi_prevent_allow(raw_scsi_var* var, int prevent);
/*2b*/ int scsi_locate(raw_scsi_var* var, int position, int immediate);
/*34*/ int scsi_read_position(raw_scsi_var* var, scsi_position_data* pos);
/*4c*/ int scsi_log_select(raw_scsi_var* var);
/*4d*/ int scsi_log_sense(raw_scsi_var* var, int page_code,
    unsigned char** obuf, int* len);

int inquire_devicedata(raw_scsi_var* device);
int set_position(raw_scsi_var* var, char* caller);
int check_position(raw_scsi_var* var, char* caller);

int interpret_error(raw_scsi_var* var);

int status_DLT(raw_scsi_var* device, char* name, int* m);
int status_EXABYTE(raw_scsi_var* device, char* name, int* m);
int status_ELIANT(raw_scsi_var* device, char* name, int* m);
int status_UNKNOWN(raw_scsi_var* device, char* name, int* m);
int status_ULTRIUM(raw_scsi_var* device, char* name, int* m);

void scsi_dump_buffer(unsigned char* buf, int size);
int scsireq_buff_decode(unsigned char *buff, size_t len, char *fmt, ...);
void scsi_interpret_error(raw_scsi_var* var);
void scsi_decode_error(raw_scsi_var* var, int direction,
    unsigned char *cdb, int cdblen,
    unsigned char *sense, int senselen,
    unsigned char *data, int datalen);

#endif
