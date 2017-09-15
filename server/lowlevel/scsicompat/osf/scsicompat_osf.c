/*
 * lowlevel/scsicompat/osf/scsicompat_osf.c
 * 
 * 06.May.2001 PW: usage of tape_sanity changed
 */
/*
 * static char *rcsid="$ZEL: scsicompat_osf.c,v 1.5 2002/09/28 17:41:05 wuestner Exp $";
 */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
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
#include <io/cam/scsi_all.h>
#include <io/cam/scsi_opcodes.h>
#include <io/cam/scsi_sequential.h>
#include <io/cam/scsi_direct.h>
#include "scsi_private.h"

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include "../scsicompat.h"

#define ALEN 255
static unsigned char buf[ALEN];

#define TRACE
#ifdef TRACE
#define TT(x) {x}
#else
#define TT(x)
#endif

#undef USE_STAT
/*****************************************************************************/
#ifndef USE_STAT
static int get_businfo(char* filename, raw_scsi_var* var, int pid)
{
device_info_t devinfo;
int res;

var->x.path=open(filename, O_RDWR, 0); /* MUST NOT be O_WRONLY on BSD */
if (var->x.path<0)
  {
  printf("scsicompat_init_%d: %s not open: %s\n", pid, filename, strerror(errno));
  return -1;
  }
if (fcntl(var->x.path, F_SETFD, FD_CLOEXEC)<0)
  {
  printf("scsicompat_osf.c: fcntl(campath, FD_CLOEXEC): %s\n", strerror(errno));
  }

bzero((caddr_t)&devinfo, sizeof(device_info_t));
res=ioctl(var->x.path, DEVGETINFO, (caddr_t)&devinfo);
if (res<0)
  {
  printf("ioctl(DEVGETINFO): %s\n", strerror(errno));
  return -1;
  }
var->x.bus=devinfo.v1.businfo.bus.scsi.bus_num;
var->x.target=devinfo.v1.businfo.bus.scsi.tgt_id;
var->x.lun=devinfo.v1.businfo.bus.scsi.lun;
printf("bus=%d, target=%d, lun=%d\n", var->x.bus, var->x.target, var->x.lun);
return 0;
}
#endif
/*****************************************************************************/
#ifdef USE_STAT
/* not usable on TRU64 UNIX 5.0 */
XXX
static int get_businfo(char* filename, raw_scsi_var* var, int pid)
{
int min_num, res;
struct stat buffer;

res=stat(filename, &buffer);
if (res<0)
  {
  printf("scsicompat_init_%d: stat(%s): %s\n", pid, filename, strerror(errno));
  return -1;
  }
if ((buffer.st_mode&S_IFCHR)==0)
  {
  printf("scsicompat_init_%d: %s is not a character special file.\n", pid,
      filename);
  return -1;
  }
min_num=minor(buffer.st_rdev);
var->x.bus=min_num/16384;
var->x.target=(min_num-var->x.bus*16384)/1024;
var->x.lun=0;
var->x.density=min_num%1024;
var->x.path=-1;
printf("bus=%d, target=%d, lun=?, density=%d\n", var->x.bus, var->x.target,
    var->x.density);
return 0;
}
#endif
/*****************************************************************************/
int scsicompat_init_(char* filename, raw_scsi_var* var, int pid)
{
TT(printf("scsicompat_init_\n");)
if (get_businfo(filename, var, pid)<0)
  {
  return -1;
  }

if ((var->x.campath=open("/dev/cam", O_RDWR, 0))<0)
  {
  printf("open /dev/cam : %s\n", strerror(errno));
  return -1;
  }
if (fcntl(var->x.campath, F_SETFD, FD_CLOEXEC)<0)
  {
  printf("scsicompat_osf.c: fcntl(campath, FD_CLOEXEC): %s\n", strerror(errno));
  }
return 0;
}
/*****************************************************************************/
int scsicompat_done_(raw_scsi_var* var)
{
close(var->x.path);
close(var->x.campath);
return 0;
}
/*****************************************************************************/
static int enter_cam(raw_scsi_var* var, int direction,
    unsigned char* cdb, int cdb_size,
    unsigned char* data, int* datasize,
    int timeout)
{
CCB_SCSIIO ccb;
UAGT_CAM_CCB ua_ccb;
int i; unsigned char* b; unsigned char sbuf[36];

bzero((caddr_t)&ccb,sizeof(CCB_SCSIIO));        /* Clear the CCB structure */
ccb.cam_ch.my_addr = (struct ccb_header *)&ccb; /* "Its" address */
ccb.cam_ch.cam_ccb_len = sizeof(CCB_SCSIIO);    /* a SCSI I/O CCB */
ccb.cam_ch.cam_func_code = XPT_SCSI_IO;         /* the opcode */
ccb.cam_ch.cam_path_id = var->x.bus;              /* selected bus */
ccb.cam_ch.cam_target_id = var->x.target;         /* selected target */
ccb.cam_ch.cam_target_lun = var->x.lun;           /* selected lun */

ccb.cam_sense_ptr = sbuf;                       /* Autosense data */
ccb.cam_sense_len = 36;                         /* size of Autosense data */

ccb.cam_timeout = timeout;
ccb.cam_cdb_len = cdb_size;                     /* how many bytes for command */

b=ccb.cam_cdb_io.cam_cdb_bytes;

for (i=0; i<cdb_size; i++) b[i]=cdb[i];

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

/* only CAM_DIR_IN, CAM_DIR_OUT and CAM_DIR_NONE allowed */
ccb.cam_ch.cam_flags = direction;

/* Set up the rest of the CCB for the command. */
ccb.cam_data_ptr = data;                  /* where the data goes */
ccb.cam_dxfer_len = datasize?*datasize:0; /* how much data */

ua_ccb.uagt_buffer = data;                /* where the data goes */
ua_ccb.uagt_buflen = datasize?*datasize:0;/* how much data */

if (ioctl(var->x.campath, UAGT_CAM_IO, (caddr_t)&ua_ccb) < 0 )
  {
  printf("ioctl(UAGt_CAM_IO): %s\n", strerror(errno));
  return -1;
  }
if ((direction==CAM_DIR_IN) && (datasize))
  {
  *datasize=ccb.cam_dxfer_len-ccb.cam_resid;
  }
if ((ccb.cam_ch.cam_status&CAM_STATUS_MASK) != CAM_REQ_CMP)
  {
  char* s;
  printf("error in enter_cam: ");
  switch (ccb.cam_ch.cam_status&CAM_STATUS_MASK)
    {
    case CAM_REQ_INPROG        :s="CCB request is in progress"; break;
    case CAM_REQ_CMP           :s="CCB request completed w/out error"; break;
    case CAM_REQ_ABORTED       :s="CCB request aborted by the host"; break;
    case CAM_UA_ABORT          :s="Unable to Abort CCB request"; break;
    case CAM_REQ_CMP_ERR       :s="CCB request completed with an err"; break;
    case CAM_BUSY              :s="CAM subsystem is busy"; break;
    case CAM_REQ_INVALID       :s="CCB request is invalid"; break;
    case CAM_PATH_INVALID      :s="Path ID supplied is invalid"; break;
    case CAM_DEV_NOT_THERE     :s="SCSI device not installed/there"; break;
    case CAM_UA_TERMIO         :s="Unable to Terminate I/O CCB req"; break;
    case CAM_SEL_TIMEOUT       :s="Target selection timeout"; break;
    case CAM_CMD_TIMEOUT       :s="Command timeout"; break;
    case CAM_MSG_REJECT_REC    :s="Message reject received"; break;
    case CAM_SCSI_BUS_RESET    :s="SCSI bus reset sent/received"; break;
    case CAM_UNCOR_PARITY      :s="Uncorrectable parity err occurred"; break;
    case CAM_AUTOSENSE_FAIL    :s="Autosense: Request sense cmd fail"; break;
    case CAM_NO_HBA            :s="No HBA detected Error"; break;
    case CAM_DATA_RUN_ERR      :s="Data overrun/underrun error"; break;
    case CAM_UNEXP_BUSFREE     :s="Unexpected BUS free"; break;
    case CAM_SEQUENCE_FAIL     :s="Target bus phase sequence failure"; break;
    case CAM_CCB_LEN_ERR       :s="CCB length supplied is inadequate"; break;
    case CAM_PROVIDE_FAIL      :s="Unable to provide requ. capability"; break;
    case CAM_BDR_SENT          :s="A SCSI BDR msg was sent to target"; break;
    case CAM_REQ_TERMIO        :s="CCB request terminated by the host"; break;
    case CAM_HBA_ERR           :s="Unrecoverable host bus adaptor err"; break;
    case CAM_BUS_RESET_DENIED  :s="SCSI bus reset denied"; break;
    case CAM_MUNSA_REJECT      :s="MUNSA rejecting device       "; break;
    case CAM_IDE               :s="Initiator Detected Error Received"; break;
    case CAM_RESRC_UNAVAIL     :s="Resource unavailable"; break;
    case CAM_UNACKED_EVENT     :s="Unacknowledged event by host"; break;
    case CAM_MESSAGE_RECV      :s="Msg received in Host Target Mode"; break;
    case CAM_INVALID_CDB       :s="Invalid CDB recvd in HT Mode"; break;
    case CAM_LUN_INVALID       :s="LUN supplied is invalid"; break;
    case CAM_TID_INVALID       :s="Target ID supplied is invalid"; break;
    case CAM_FUNC_NOTAVAIL     :s="The requ. func is not available"; break;
    case CAM_NO_NEXUS          :s="Nexus is not established"; break;
    case CAM_IID_INVALID       :s="The initiator ID is invalid"; break;
    case CAM_CDB_RECVD         :s="The SCSI CDB has been received"; break;
    case CAM_LUN_ALLREADY_ENAB :s="LUN already enabled"; break;
    case CAM_SCSI_BUSY         :s="SCSI bus busy"; break;
    default: s=0;
    };
  if (s)
    printf("%s\n", s);
  else
    printf("CAM error %d (unknown)\n", ccb.cam_ch.cam_status&CAM_STATUS_MASK);
  if (ccb.cam_ch.cam_flags&CAM_DIR_IN) printf("CAM_DIR_IN\n");
  if (ccb.cam_ch.cam_flags&CAM_DIR_OUT) printf("CAM_DIR_OUT\n");
  if (ccb.cam_ch.cam_flags&CAM_DIR_NONE) printf("CAM_DIR_NONE\n");
  printf("cdb: size      =%d\n", cdb_size);
  scsi_dump_buffer(cdb, cdb_size);
  printf("uagt_snslen    =%d\n", ua_ccb.uagt_snslen);
  printf("cam_sense_len  =%d\n", ccb.cam_sense_len);
  printf("cam_sense_resid=%d\n", (int)ccb.cam_sense_resid);
  printf("cam_ccb_len    =%02x\n", ccb.cam_ch.cam_ccb_len);
  printf("cam_func_code  =%02x\n", ccb.cam_ch.cam_func_code);
  printf("cam_status     =%02x\n", ccb.cam_ch.cam_status);
  printf("cam_path_id    =%02x\n", ccb.cam_ch.cam_path_id);
  printf("cam_target_id  =%02x\n", ccb.cam_ch.cam_target_id);
  printf("cam_target_lun =%02x\n", ccb.cam_ch.cam_target_lun);
  printf("cam_flags      =%02x\n", ccb.cam_ch.cam_flags);
  printf("cam_timeout    =%d\n", ccb.cam_timeout);
  printf("cam_cdb_len    =%d\n", ccb.cam_cdb_len);
  if (ccb.cam_ch.cam_status&CAM_AUTOSNS_VALID)
    {
    scsi_decode_error(var, direction, cdb, cdb_size,
        sbuf, ua_ccb.uagt_snslen-ccb.cam_sense_resid, 0, 0);
    }
  else
    printf("sense data not valid\n");
  /* check if SIM queue is frozen */
  if (ccb.cam_ch.cam_status & CAM_SIM_QFRZN)
    {
    CCB_RELSIM ccb_sim_rel;
    UAGT_CAM_CCB ua_ccb_sim_rel;

    printf("the SIM queue is frozen.\n");
    /* release SIM queue */
    /* Set up the CCB for an XPT_REL_SIMQ request. */
    /* Set up the CAM header for the XPT_REL_SIMQ function. */
    ccb_sim_rel.cam_ch.my_addr        = (struct ccb_header*)&ccb_sim_rel;
                                                       /* "Its" address */
    ccb_sim_rel.cam_ch.cam_ccb_len    = sizeof(CCB_RELSIM);
                                                      /* a SIMQ release  */
    ccb_sim_rel.cam_ch.cam_func_code  = XPT_REL_SIMQ; /* the opcode */
    ccb_sim_rel.cam_ch.cam_path_id    = var->x.bus;   /* selected bus */
    ccb_sim_rel.cam_ch.cam_target_id  = var->x.target;/* selected target */
    ccb_sim_rel.cam_ch.cam_target_lun = var->x.lun;   /* selected lun */
    /* The needed CAM flags are: CAM_DIR_NONE - No data */
    /* will be transferred. */
    ccb_sim_rel.cam_ch.cam_flags      = CAM_DIR_NONE;
    /* Set up the fields for the User Agent Ioctl call. */
    ua_ccb_sim_rel.uagt_ccb    = (CCB_HEADER *)&ccb_sim_rel;
                                                      /* where the CCB is */
    ua_ccb_sim_rel.uagt_ccblen = sizeof(CCB_RELSIM);  /* bytes in CCB */
    ua_ccb_sim_rel.uagt_buffer = (u_char *)0;         /* no data */
    ua_ccb_sim_rel.uagt_buflen = 0;                   /* no data */
    ua_ccb_sim_rel.uagt_snsbuf = (u_char *)0;         /* no Autosense data */
    ua_ccb_sim_rel.uagt_snslen = 0;                   /* no Autosense data */
    ua_ccb_sim_rel.uagt_cdb    = (CDB_UN *)NULL;      /* CDB is in the CCB */
    ua_ccb_sim_rel.uagt_cdblen = 0;                   /* CDB is in the CCB */
    if (ioctl(var->x.campath, UAGT_CAM_IO, (caddr_t)&ua_ccb_sim_rel) < 0 )
      {
      printf("error releasing SIM queue: %s\n", strerror(errno));
      return -1;
      }
    if (ccb_sim_rel.cam_ch.cam_status != CAM_REQ_CMP)
      {
      printf("fehler in enter_cam (release SIM queue)\n");
      return -1;
      }
    printf("SIM queue released.\n");
    }
  return -1;
  }
else
  return 0;
}
/*****************************************************************************/
int scsi_rewind(raw_scsi_var* var, int immediate)
{/*01*/
SEQ_REWIND_CDB6 cdb;
int res;

printf("scsi_rewind(immediate=%d)\n", immediate);
bzero((char*)&cdb, sizeof(cdb));
cdb.opcode=SOPC_REWIND;
cdb.lun=var->x.lun;
cdb.immed=immediate;
res=enter_cam(var, CAM_DIR_NONE, (unsigned char*)&cdb, sizeof(cdb), 0, 0, 600);
if (res<0) printf("error in scsi_rewind\n");
var->sanity.sw_position=0;
var->sanity.filemarks=0;
var->sanity.records=0;
return res;
}
/*---------------------------------------------------------------------------*/
int scsi_request_sense(raw_scsi_var* var, unsigned char** obuf, int* len)
{/*03*/
ALL_REQ_SENSE_CDB6 cdb;
int res;

/*printf("scsi_request_sense\n");*/
bzero((char*)&cdb, sizeof(cdb));
cdb.opcode=SOPC_REQUEST_SENSE;
cdb.lun=var->x.lun;
cdb.alloc_len=ALEN;

*len=ALEN;
res=enter_cam(var, CAM_DIR_IN, (unsigned char*)&cdb, sizeof(cdb), buf, len, 1);
if (res<0) printf("error in scsi_request_sense\n");
*obuf=buf;
return res;
}
/*---------------------------------------------------------------------------*/
int scsi_read_block_limits(raw_scsi_var* var, unsigned char** obuf, int* len)
{/*05*/
SEQ_BLOCK_LIMITS_CDB6 cdb;
int res;

bzero((char*)&cdb, sizeof(cdb));
cdb.opcode=SOPC_READ_BLOCK_LIMITS;
cdb.lun=var->x.lun;
*len=ALEN;
res=enter_cam(var, CAM_DIR_IN, (unsigned char*)&cdb, sizeof(cdb), buf, len, 1);
if (res<0) printf("error in scsi_read_block_limits\n");
*obuf=buf;
return res;
}
/*---------------------------------------------------------------------------*/
int scsi_write(raw_scsi_var* var, int num, int* data)
{/*0a*/
SEQ_WRITE_CDB6 cdb;
int res, len;

/*printf("scsi_write(num=%d)\n", num);*/
if (check_position(var, "scsi_write")<0) return -1;

len=num*sizeof(int);
bzero((char*)&cdb, sizeof(cdb));
cdb.opcode=SOPC_WRITE_6;
cdb.lun=var->x.lun;
cdb.fixed=0;
cdb.trans_len2=(len>>16)&0xff;
cdb.trans_len1=(len>>8)&0xff;
cdb.trans_len0=len&0xff;

res=enter_cam(var, CAM_DIR_OUT, (unsigned char*)&cdb, sizeof(cdb),
    (unsigned char*)data, &len, 180);

if (res<0)
  printf("error in scsi_write\n");
else
  {
  var->sanity.sw_position++;
  var->sanity.records++;
  }
var->sanity.filemarks=0;
return res;
}
/*---------------------------------------------------------------------------*/
int scsi_write_filemark(raw_scsi_var* var, int num, int immediate)
{/*10*/
SEQ_WRITEMARKS_CDB6 cdb;
int res;

printf("scsi_write_filemark; records=%d, filemarks=%d, sw_position=%d\n",
  var->sanity.records, var->sanity.filemarks, var->sanity.sw_position);
if (check_position(var, "scsi_write_filemark")<0) return -1;

bzero((char*)&cdb, sizeof(cdb));
cdb.opcode=SOPC_WRITE_FILEMARKS;
cdb.immed=immediate;
cdb.lun=var->x.lun;
cdb.trans_len2=(num>>16)&0xff;
cdb.trans_len1=(num>>8)&0xff;
cdb.trans_len0=num&0xff;
res=enter_cam(var, CAM_DIR_NONE, (unsigned char*)&cdb, sizeof(cdb),
    0, 0, 180);
if (res<0)
  printf("error in scsi_write_filemark\n");
else
  var->sanity.sw_position+=num;
var->sanity.records=0;
var->sanity.filemarks+=num;
return res;
}
/*---------------------------------------------------------------------------*/
int scsi_space(raw_scsi_var* var, int code, int count)
{/*11*/
SEQ_SPACE_CDB6 cdb;
int res;

printf("scsi_space(code=%d, count=%d)\n", code, count);
bzero((char*)&cdb, sizeof(cdb));
cdb.opcode=SOPC_SPACE;
cdb.lun=var->x.lun;
cdb.code=code;
cdb.count2=(count>>16)&0xff;
cdb.count1=(count>>8)&0xff;
cdb.count0=count&0xff;
res=enter_cam(var, CAM_DIR_NONE, (unsigned char*)&cdb, sizeof(cdb), 0, 0,
    200000);
if (res<0)
  {
  printf("error in scsi_space\n");
  var->sanity.sw_position=-1;
  }
else
  set_position(var, "scsi_space");
var->sanity.records=0;
var->sanity.filemarks++;
return res;
}
/*---------------------------------------------------------------------------*/
int scsi_inquiry(raw_scsi_var* var, unsigned char** obuf, int* len)
{/*12*/
ALL_INQ_CDB cdb;
int res;

bzero((char*)&cdb, sizeof(cdb));
cdb.opcode=SOPC_INQUIRY;
cdb.lun=var->x.lun;
cdb.evpd=0;
cdb.page=0;
cdb.alloc_len=ALEN;

*len=ALEN;
res=enter_cam(var, CAM_DIR_IN, (unsigned char*)&cdb, sizeof(cdb), buf, len, 1);
if (res<0) printf("error in scsi_inquiry\n");
*obuf=buf;
return res;
}
/*---------------------------------------------------------------------------*/
int scsi_mode_select(raw_scsi_var* var, int density)
{/*15*/
ALL_MODE_SEL_CDB6 cdb;
struct {
  SEQ_MODE_HEAD6 head;
  SEQ_MODE_DESC desc;
  } data;
int res=0, length;

printf("scsi_mode_select(density=%d)\n", density);
bzero((char*)&cdb, sizeof(cdb));
length=sizeof(data);
cdb.opcode=SOPC_MODE_SELECT_6;
cdb.lun=var->x.lun;
cdb.sp=0;
cdb.pf=1;
cdb.param_len=length;
data.head.mode_len=4;
data.head.medium_type=0;
data.head.speed=0;
data.head.buf_mode=1;
data.head.wp=0;
/*
data.head.blk_desc_len=;
data.desc.density_code=;
data.desc.num_blocks2=;
data.desc.num_blocks1=;
data.desc.num_blocks0=;
data.desc.block_len2=;
data.desc.block_len1=;
data.desc.block_len0=;
res=cam_out(var, (unsigned char*)&cdb, sizeof(cdb), (unsigned char*)&data,
  length, 1);
*/
printf("scsi_mode_select not yet implemented\n");
/*if (res<0) printf("error in scsi_mode_select\n");*/
return res;
}
/*---------------------------------------------------------------------------*/
int scsi_reserve_unit(raw_scsi_var* var, int thirt_party)
{/*16*/
printf("scsi_reserve_unit not yet implemented\n");
return -1;
}
/*---------------------------------------------------------------------------*/
int scsi_release_unit(raw_scsi_var* var, int thirt_party)
{/*17*/
printf("scsi_release_unit not yet implemented\n");
return -1;
}
/*---------------------------------------------------------------------------*/
int scsi_erase(raw_scsi_var* var, int immediate, int extend)
{/*19*/
SEQ_ERASE_CDB6 cdb;
int res;

printf("scsi_erase(immediate=%d, extend=%d)\n", immediate, extend);
bzero((char*)&cdb, sizeof(cdb));
cdb.opcode=SEQ_ERASE_OP;
cdb.lun=var->x.lun;
cdb.immed=immediate;
cdb.extend=extend;
res=enter_cam(var, CAM_DIR_NONE, (unsigned char*)&cdb, sizeof(cdb), 0, 0,
    200000);
if (res<0) printf("error in scsi_erase\n");
var->sanity.records=0;
var->sanity.filemarks=0;
var->sanity.sw_position=-1;
return res;
}
/*---------------------------------------------------------------------------*/
int scsi_mode_sense(raw_scsi_var* var, int page_control,
    int page_code, unsigned char** obuf, int* len)
{/*1a/5a*/
printf("scsi_mode_sense not yet implemented\n");
return -1;
}
/*---------------------------------------------------------------------------*/
int scsi_load_unload(raw_scsi_var* var, int load, int immediate)
{/*1b*/
SEQ_LOAD_CDB6 cdb;
int res;

printf("scsi_load_unload(load=%d, immediate=%d)\n", load, immediate);
bzero((char*)&cdb, sizeof(cdb));
cdb.opcode=SOPC_LOAD_UNLOAD;
cdb.lun=var->x.lun;
cdb.immed=immediate;
cdb.load=load;
res=enter_cam(var, CAM_DIR_NONE, (unsigned char*)&cdb, sizeof(cdb), 0, 0, 600);
if (res<0) printf("error in scsi_load_unload\n");
var->sanity.filemarks=0;
var->sanity.records=0;
var->sanity.sw_position=-1;
return res;
}
/*---------------------------------------------------------------------------*/
int scsi_prevent_allow(raw_scsi_var* var, int prevent)
{/*1e*/
DIR_PREVENT_CDB6 cdb;
int res;

printf("scsi_prevent_allow(prevent=%d)\n", prevent);
bzero((char*)&cdb, sizeof(cdb));
cdb.opcode=SOPC_PREVENT_ALLOW_REMOVAL;
cdb.lun=var->x.lun;
cdb.prevent=prevent;
res=enter_cam(var, CAM_DIR_NONE, (unsigned char*)&cdb, sizeof(cdb), 0, 0, 1);
if (res<0) printf("error in scsi_prevent_allow\n");
return res;
}
/*---------------------------------------------------------------------------*/
int scsi_locate(raw_scsi_var* var, int position, int immediate)
{/*2b*/
printf("scsi_locate not yet implemented\n");
return -1;
}
/*---------------------------------------------------------------------------*/
int scsi_read_position(raw_scsi_var* var, scsi_position_data* pos)
{/*34*/
PRIV_READ_POSITION_CDB10 cdb;
int res, len;

bzero((char*)&cdb, sizeof(cdb));
cdb.opcode=SOPC_READ_POSITION;
cdb.lun=var->x.lun;
cdb.bt=0;

len=ALEN;
res=enter_cam(var, CAM_DIR_IN, (unsigned char*)&cdb, sizeof(cdb), buf, &len, 1);
if (res<0)
  printf("error in scsi_read_position\n");
else
  {
  scsireq_buff_decode(buf, len,
    "b1"  /* BOP */
    "b1"  /* EOP */
    "*b3" /* res */
    "b1"  /* bpu (block position unknown) */
    "*b2" /* res */
    "i1" /* partition */
    "*i2" /* res */
    "i4"  /* first block location */
    "i4"  /* last block location */
    "*i1" /* res */
    "i3"  /* number of blocks in buffer */
    "i4", /* number of bytes in buffer */
      &pos->bop, &pos->eop, &pos->bpu, &pos->part, &pos->fbl, &pos->lbl,
      &pos->blib, &pos->byib);
  }
return res;
}
/*---------------------------------------------------------------------------*/
int scsi_log_select(raw_scsi_var* var)
{/*4c*/
printf("scsi_log_select not yet implemented\n");
return -1;
}
/*---------------------------------------------------------------------------*/
int scsi_log_sense(raw_scsi_var* var, int page_code,
    unsigned char** obuf, int* len)
{/*4d*/
/* here only used to obtain cumulative values */
ALL_LOG_SNS_CDB10 cdb;
int res;

/*printf("scsi_log_sense(page=%d)\n", page_code);*/
bzero((char*)&cdb, sizeof(cdb));
cdb.opcode=SOPC_LOG_SENSE;
cdb.lun=var->x.lun;
cdb.sp=0;
cdb.ppc=0;
cdb.pg_code=page_code;
cdb.pc=1;
cdb.param_ptr1=0;
cdb.param_ptr0=0;
cdb.alloc_len1=0;
cdb.alloc_len0=ALEN;

*len=ALEN;
res=enter_cam(var, CAM_DIR_IN, (unsigned char*)&cdb, sizeof(cdb), buf, len, 1);
if (res<0) printf("error in scsi_log_sense\n");
*obuf=buf;
return res;
}
/*****************************************************************************/
/*****************************************************************************/
