/*
 * dataout/cluster/tapecontrol.c
 * 
 * created 20.12.1998 PW
 */
static char *rcsid="$ZEL: tapecontrol_osf.c,v 1.4 2004/06/18 23:29:17 wuestner Exp $";

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/devgetinfo.h>
#include <io/common/iotypes.h>
#include <io/common/devdriver.h>
#include <io/cam/cam.h>      /* CAM defines from the CAM document */
#include <io/cam/dec_cam.h>  /* CAM defines for Digital CAM source files */
#include <io/cam/uagt.h>     /* CAM defines for the UAgt driver */
#include "scsi_all.h" /* CAM defines for ALL SCSI devices */
#include "handlercom.h"
#include <xdrstring.h>
#include "tapecontrol.h"

static int bus, target, lun;
static int p;

typedef unsigned char cam_buf[255];

/*****************************************************************************/
static int cam_in(unsigned char* cdb, int cdb_size, unsigned char* data,
    int datasize, int timeout)
{
CCB_SCSIIO ccb;
UAGT_CAM_CCB ua_ccb;
int i; unsigned char* b; unsigned char sbuf[36];

bzero((caddr_t)&ccb,sizeof(CCB_SCSIIO));        /* Clear the CCB structure */
ccb.cam_ch.my_addr = (struct ccb_header *)&ccb; /* "Its" address */
ccb.cam_ch.cam_ccb_len = sizeof(CCB_SCSIIO);    /* a SCSI I/O CCB */
ccb.cam_ch.cam_func_code = XPT_SCSI_IO;         /* the opcode */
ccb.cam_ch.cam_path_id = bus;                   /* selected bus */
ccb.cam_ch.cam_target_id = target;              /* selected target */
ccb.cam_ch.cam_target_lun = lun;                /* selected lun */

ccb.cam_sense_ptr = sbuf;                       /* Autosense data */
ccb.cam_sense_len = 36;                         /* size of Autosense data */

ccb.cam_timeout = timeout;
ccb.cam_cdb_len = cdb_size;                     /* how many bytes for command */

b=ccb.cam_cdb_io.cam_cdb_bytes;
for (i=0; i<cdb_size; i++)
  {
  printf("cdb[%d]=%02x\n", i, cdb[i]);
  b[i]=cdb[i];
  }

bzero((caddr_t)&ua_ccb, sizeof(UAGT_CAM_CCB));  /* Clear the ua_ccb structure */

/* Set up the fields for the User Agent Ioctl call. */
ua_ccb.uagt_ccb = (CCB_HEADER *)&ccb;    /* where the CCB is */
ua_ccb.uagt_ccblen = sizeof(CCB_SCSIIO); /* size of it */
                                         /* how many bytes to pull in */
ua_ccb.uagt_snsbuf = sbuf;               /* Autosense data */
ua_ccb.uagt_snslen = 36;                 /* size of Autosense data */
ua_ccb.uagt_cdb = (CDB_UN *)0;           /* CDB is in the CCB */
ua_ccb.uagt_cdblen = 0;                  /* CDB is in the CCB */
ua_ccb.uagt_flags =0;

ccb.cam_ch.cam_flags = CAM_DIR_IN;

/* Set up the rest of the CCB for the command. */
ccb.cam_data_ptr = data;                  /* where the data goes */
ccb.cam_dxfer_len = datasize;             /* how much data */

ua_ccb.uagt_buffer = data;                /* where the data goes */
ua_ccb.uagt_buflen = datasize;            /* how much data */
if (ioctl(p, UAGT_CAM_IO, (caddr_t)&ua_ccb) < 0 )
  {
  printf("ioctl( UAGT_CAM_IO): %s\n", strerror(errno));
  return -1;
  }
if (ccb.cam_ch.cam_status != CAM_REQ_CMP)
  {
  printf("fehler in cam_in\n");
  printf("senselen=%d\n", ua_ccb.uagt_snslen);
  for (i=0; i<ua_ccb.uagt_snslen; i++)
    printf("sense[%d]=%02x\n", i, (unsigned int)sbuf[i]);
  printf("ccb.cam_ccb_len   =%02x\n", ccb.cam_ch.cam_ccb_len);
  printf("ccb.cam_func_code =%02x\n", ccb.cam_ch.cam_func_code);
  printf("ccb.cam_status    =%02x\n", ccb.cam_ch.cam_status);
  printf("ccb.cam_path_id   =%02x\n", ccb.cam_ch.cam_path_id);
  printf("ccb.cam_target_id =%02x\n", ccb.cam_ch.cam_target_id);
  printf("ccb.cam_target_lun=%02x\n", ccb.cam_ch.cam_target_lun);
  printf("ccb.cam_flags     =%02x\n", ccb.cam_ch.cam_flags);
  for (i=0; i<255; i++)
    printf("data[%d]=%02x %3d %c\n", i, data[i], data[i],
        (data[i]>=' ')&(data[i]<128)?data[i]:' ');
  return -1;
  }
else
  return 0;
}
/*****************************************************************************/
int init_scsi_commands(int path)
{
int res;
device_info_t devinfo;

bus=-1; target=-1; lun=-1; p=-1;
bzero((caddr_t)&devinfo, sizeof(device_info_t));

res=ioctl(path, DEVGETINFO, (caddr_t)&devinfo);
if (res<0)
  {
  printf("ioctl(DEVGETINFO): %s\n", strerror(errno));
  return 0;
  }

bus=devinfo.v1.businfo.bus.scsi.bus_num;
target=devinfo.v1.businfo.bus.scsi.tgt_id;
lun=devinfo.v1.businfo.bus.scsi.lun;
printf("bus=%d, target=%d, lun=%d\n", bus, target, lun);

if ((p=open("/dev/cam", O_RDWR, 0))<0)
  {
  printf("open /dev/cam : %s\n", strerror(errno));
  return 0;
  }
return 1;
}
/*****************************************************************************/
int done_scsi_commands()
{
close(p);
return 0;
}
/*****************************************************************************/
void make_tapename(int path)
{
int res;
ALL_INQ_CDB cdb;
unsigned char data[255];

printf("make_tapename called\n");
cdb.opcode=0x12;
cdb.evpd=0;
cdb.lun=lun;
cdb.page=0;
cdb.alloc_len=255;
cdb.control=0;

res=cam_in((unsigned char*)&cdb, 6, data, 255, 10);
if (res<0)
  {
  printf("error in tapename()\n");
  goto fehler;
  }
strncpy(tapename, data+16, 16);
tapename[16]=0;
return;

fehler:
tapename[0]=0;
}
/*****************************************************************************/
static void do_Inquire(int** in, int** out)
{
int res;
ALL_INQ_CDB cdb;
unsigned char data[255];

cdb.opcode=0x12;
cdb.evpd=0;
cdb.lun=lun;
cdb.page=0;
cdb.alloc_len=255;
cdb.control=0;

res=cam_in((unsigned char*)&cdb, 6, data, 255, 10);
if (res<0)
  {
  printf("error in do_Inquire()\n");
  *(*out)++=1;
  return;
  }
*(*out)++=0;
*(*out)++=2;
*out=outnstring(*out, data+16, 16);
*out=outnstring(*out, data+8, 8);
}
/*****************************************************************************/
static void do_Load(int** in, int** out)
{
typedef struct {
  u_char opcode;     /* 0x1b */
  u_char immed   :1,
                 :4,
         lun     :3;
  u_char         :8;
  u_char         :8;
  u_char load    :1,
         reten   :1,
         eot     :1,
                 :5;
  u_char link    :1,
         flag    :1,
         res     :4,
                 :2;
} load_cdb;
load_cdb cdb;
int res;

cdb.opcode=0x1b;
cdb.immed=1;
cdb.lun=lun;
cdb.load=!!*(*in)++;
cdb.reten=0;
cdb.eot=0;
cdb.link=0;
cdb.flag=0;
cdb.res=0;
res=cam_in((unsigned char*)&cdb, 6, (unsigned char*)0, 0, 120);
if (res<0) printf("error in do_Load()\n");
*(*out)++=0;
}
/*****************************************************************************/
static void do_Prevent(int** in, int** out)
{
}
/*****************************************************************************/
static void do_Erase(int** in, int** out)
{
}
/*****************************************************************************/
static void do_Locate(int** in, int** out)
{
}
/*****************************************************************************/
static void do_Space(int** in, int** out)
{
}
/*****************************************************************************/
static void do_Rewind(int** in, int** out)
{
}
/*****************************************************************************/
static void do_ReadPos(int** in, int** out)
{
}
/*****************************************************************************/
static void do_ClearLog(int** in, int** out)
{
}
/*****************************************************************************/
int status_DLT4000(int path, int* m)
{
return 0;
}

int status_EXABYTE(int path, int* m)
{
return 0;
}

int scsi_control_tape(int path, int* in, int* out)
{
int code=*in++;
int* help=out;
switch (code)
  {
  case hndlcode_none: break;
  case hndlcode_Inquire:
    do_Inquire(&in, &out);
    break;
  case hndlcode_Load:
    do_Load(&in, &out);
    break;
  case hndlcode_Prevent:
    do_Prevent(&in, &out);
    break;
  case hndlcode_Erase:
    do_Erase(&in, &out);
    break;
  case hndlcode_Locate:
    do_Locate(&in, &out);
    break;
  case hndlcode_Space:
    do_Space(&in, &out);
    break;
  case hndlcode_Rewind:
    do_Rewind(&in, &out);
    break;
  case hndlcode_ReadPos:
    do_ReadPos(&in, &out);
    break;
  case hndlcode_ClearLog:
    do_ClearLog(&in, &out);
    break;
  case hndlcode_Filemark: break;
    do_Filemark(&in, &out);
    break;
  default: printf("hndlcode (A): %d; not decoded\n", code);
  }
return out-help;
}
/*****************************************************************************/
/*****************************************************************************/
