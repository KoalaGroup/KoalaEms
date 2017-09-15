/*
 * lowlevel/scsicompat/linux/scsicompat_linux.c
 * created 08.12.2000 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: scsicompat_linux.c,v 1.10 2011/04/06 20:30:28 wuestner Exp $";

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/ioctl.h>
#include <rcs_ids.h>
#include "../../../main/nowstr.h"
#include "../scsicompat.h"
#include "/usr/include/scsi/sg.h"
#include "/usr/include/scsi/scsi.h"
#include "scsi_private.h"

RCS_REGISTER(cvsid, "lowlevel/scsicompat/linux")

static int seq;

/*****************************************************************************/
int scsicompat_init_(char* filename, raw_scsi_var* var, int pid)
{
var->x.path=open(filename, O_RDWR, 0);
if (var->x.path<0)
  {
  printf("scsicompat_init_%d: %s not open: %s\n",
      pid, filename, strerror(errno));
  return -1;
  }
if (fcntl(var->x.path, F_SETFD, FD_CLOEXEC)<0)
  {
  printf("scsicompat_linux.c: fcntl(campath, FD_CLOEXEC): %s\n",
      strerror(errno));
  }

var->x.lun=0; /* immer? */
seq=0;
return 0;
}

int scsicompat_done_(raw_scsi_var* var)
{
close(var->x.path);
return 0;
}
/*****************************************************************************/
static void scsi_decode_error_(raw_scsi_var* var,
    unsigned char* req, int reqlen,
    unsigned char* conf, int conflen)
{
scsi_decode_error(var, 6/*XXX*/,
    req+sizeof(struct sg_header), 6/*XXX*/,
    ((struct sg_header*)conf)->sense_buffer, 12/*XXX*/,
    conf+sizeof(struct sg_header), conflen-sizeof(struct sg_header));
}
/*****************************************************************************/
static int sg_enter(raw_scsi_var* var, void* req, int reqlen, char* conf,
    int conflen, int timeout)
{
/*
 *   NOTE: Up to recent kernel versions, it is necessary to block the
 *   SIGINT signal between the write() and the corresponding read() call
 *   (i.e. via sigprocmask()). A return after the write() part without any
 *   read() to fetch the results will block on subsequent accesses.
 */

int res, media_changed, old_timeout=-1;

if (timeout!=0)
  {
  if (ioctl(var->x.path, SG_GET_TIMEOUT, &old_timeout)<0)
    {
    printf("sg_enter: ioctl(SG_GET_TIMEOUT): %s\n", strerror(errno));
    old_timeout=-1;
    }
  if (ioctl(var->x.path, SG_SET_TIMEOUT, &timeout)<0)
    {
    printf("sg_enter: ioctl(SG_SET_TIMEOUT, %d): %s\n", timeout,
        strerror(errno));
    }
  }
res=write(var->x.path, req, reqlen);
if (res!=reqlen)
  {
  /* some error happened */
  printf("sg_enter(write): result=%d; errno=%s\n", res, strerror(errno));
  res=-1;
  }
else
  {
  /* retrieve result */
  res=read(var->x.path, conf, conflen);
  if (res!=conflen
      || ((struct sg_header*)conf)->result
      || ((struct sg_header*)conf)->target_status
      || ((struct sg_header*)conf)->driver_status)
    {
    struct sg_header* c=(struct sg_header*)conf;
    printf("sg_enter(read): result=0x%x; errno=%s\n", c->result,
        strerror(errno));
    printf("                         target_status=0x%x;\n",
        c->target_status);
    printf("                         driver_status=0x%x;\n",
        c->driver_status);
    if (c->target_status==CHECK_CONDITION
        || c->target_status==COMMAND_TERMINATED
        /*|| c->driver_status&DRIVER_SENSE*/)
      {
      scsi_decode_error_(var, req, reqlen, (unsigned char*)conf, conflen);
      media_changed=(res==0)&&
          (var->scsi_error.sense_key==6)&&
          (var->scsi_error.add_sense_code==0x28);

      if (media_changed) printf("%s MEDIA CHANGED!!\n", nowstr());
      
      res=-1;
      }
    }
  else /* all ok. */
    {
    res=0;
    }
  }
if (old_timeout!=-1)
  {
  if (ioctl(var->x.path, SG_SET_TIMEOUT, &old_timeout)<0)
    {
    printf("sg_enter: ioctl(SG_SET_TIMEOUT, %d): %s\n", old_timeout,
        strerror(errno));
    }
  }
return res;
}
/*****************************************************************************/
int scsi_rewind(raw_scsi_var* var, int immediate)
{/*01*/
int res;
struct
  {
  struct sg_header head;
  SEQ_REWIND_CDB6 cdb;
  } req;
struct
  {
  struct sg_header head;
  } conf;

printf("%s scsi_rewind(immediate=%d)\n", nowstr(), immediate);

bzero((char*)&req, sizeof(req));
req.cdb.opcode=SOPC_REWIND;
req.cdb.lun=var->x.lun;
req.cdb.immed=immediate;

req.head.pack_len=sizeof(req);/* length of incoming packet (including header) */
req.head.reply_len=sizeof(conf); /* maximum length of expected reply */
req.head.pack_id=++seq;          /* id number of packet */
req.head.result=0;               /* 0==ok, otherwise refer to errno codes */
req.head.twelve_byte=0;          /* Force 12 byte command length */
req.head.other_flags=0;          /* for future use */

res=sg_enter(var, (void*)&req, sizeof(req), (void*)&conf, sizeof(conf), 600000);

if (res) printf("Error in scsi_rewind\n");
var->sanity.sw_position=0;
var->sanity.filemarks=0;
var->sanity.records=0;
return res;
}
/*****************************************************************************/
int scsi_mode_select(raw_scsi_var* var, int density)
{/*15*/
int res;
struct
  {
  struct sg_header head;
  ALL_MODE_SEL_CDB6 cdb;
  SEQ_MODE_HEAD6 mode_head;
  SEQ_MODE_DESC mode_desc;
  } req;
struct
  {
  struct sg_header head;
  } conf;

printf("%s scsi_mode_select(density=%d)\n", nowstr(), density);

bzero((char*)&req, sizeof(req));
req.head.pack_len=sizeof(req);
req.head.reply_len=sizeof(conf);
req.head.pack_id=++seq;

req.cdb.opcode=SOPC_MODE_SELECT_6;
req.cdb.lun=var->x.lun;
req.cdb.sp=0;
req.cdb.pf=1;
req.cdb.param_len=sizeof(SEQ_MODE_HEAD6)+sizeof(SEQ_MODE_DESC);

req.mode_head.mode_len=0;
req.mode_head.medium_type=0;
req.mode_head.speed=0;
req.mode_head.buf_mode=1;
req.mode_head.blk_desc_len=sizeof(SEQ_MODE_DESC);

req.mode_desc.density_code=density;
req.mode_desc.num_blocks2=0;
req.mode_desc.num_blocks1=0;
req.mode_desc.num_blocks0=0;
req.mode_desc.block_len2=0;
req.mode_desc.block_len1=0;
req.mode_desc.block_len0=0;

res=sg_enter(var, (void*)&req, sizeof(req), (void*)&conf, sizeof(conf), 0);

return res;
}
/*****************************************************************************/
int scsi_read_position(raw_scsi_var* var, scsi_position_data* pos)
{/*34*/
int res;
struct
  {
  struct sg_header head;
  PRIV_READ_POSITION_CDB10 cdb;
  } req;
struct
  {
  struct sg_header head;
  unsigned char buf[20];
  } conf;

bzero((char*)&req, sizeof(req));
req.head.pack_len=sizeof(req);
req.head.reply_len=sizeof(conf);
req.head.pack_id=++seq;

req.cdb.opcode=SOPC_READ_POSITION;
req.cdb.lun=var->x.lun;
req.cdb.bt=0;

res=sg_enter(var, (void*)&req, sizeof(req), (void*)&conf, sizeof(conf), 0);

if (res<0)
  printf("%s error in scsi_read_position\n", nowstr());
else
  {
  scsireq_buff_decode(conf.buf, 20,
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
/*****************************************************************************/
int scsi_write(raw_scsi_var* var, int num, int* data)
{/*0a*/
int res, len;

char req[262144+sizeof(struct sg_header)+sizeof(SEQ_WRITE_CDB6)];
struct sg_header* head=(struct sg_header*)req;
SEQ_WRITE_CDB6* cdb=(SEQ_WRITE_CDB6*)(req+sizeof(struct sg_header));
char* reqdata=req+sizeof(struct sg_header)+sizeof(SEQ_WRITE_CDB6);

/*
 * struct req
 *   {
 *   struct sg_header head;
 *   SEQ_WRITE_CDB6 cdb;
 *   char data[262144];
 *   } req;
 */
struct
  {
  struct sg_header head;
  } conf;

/*if (check_position(var, "scsi_write")<0) return -1;*/

len=num*sizeof(int);
bzero((char*)&req, sizeof(req));
/*req.head.pack_len=sizeof(req);*/
head->pack_len=sizeof(struct sg_header)+sizeof(SEQ_WRITE_CDB6)+len;
head->reply_len=sizeof(conf);
head->pack_id=++seq;

cdb->opcode=SOPC_WRITE;
cdb->lun=var->x.lun;
cdb->fixed=0;
cdb->trans_len2=(len>>16)&0xff;
cdb->trans_len1=(len>>8)&0xff;
cdb->trans_len0=len&0xff;

bcopy(data, reqdata, len);

res=sg_enter(var, req, head->pack_len, (void*)&conf, sizeof(conf), 180000);

if (res==0)
  {
  var->sanity.sw_position++;
  var->sanity.records++;
  }
var->sanity.filemarks=0;
return res;
}
/*****************************************************************************/
int scsi_write_filemark(raw_scsi_var* var, int num, int immediate)
{/*10*/
int res;
struct
  {
  struct sg_header head;
  SEQ_WRITEMARKS_CDB6 cdb;
  } req;
struct
  {
  struct sg_header head;
  } conf;

printf("%s scsi_write_filemark; records=%d, filemarks=%d, position=%d\n",
    nowstr(),
  var->sanity.records, var->sanity.filemarks, var->sanity.sw_position);

if (check_position(var, "scsi_write_filemark")<0) return -1;

bzero((char*)&req, sizeof(req));
req.head.pack_len=sizeof(req);
req.head.reply_len=sizeof(conf);
req.head.pack_id=++seq;

req.cdb.opcode=SOPC_WRITE_FILEMARKS;
req.cdb.lun=var->x.lun;
req.cdb.immed=immediate;
req.cdb.trans_len2=(num>>16)&0xff;
req.cdb.trans_len1=(num>>8)&0xff;
req.cdb.trans_len0=num&0xff;

res=sg_enter(var, (void*)&req, sizeof(req), (void*)&conf, sizeof(conf), 60000);
if (res<0)
  printf("%s error in scsi_write_filemark\n", nowstr());
else
  var->sanity.sw_position+=num;
var->sanity.records=0;
var->sanity.filemarks++;
return res;
}
/*****************************************************************************/
int scsi_inquiry(raw_scsi_var* var, unsigned char** obuf, int* len)
{/*12*/
int res;
struct
  {
  struct sg_header head;
  ALL_INQ_CDB cdb;
  } req;
static struct
  {
  struct sg_header head;
  unsigned char data[256];
  } conf;

bzero((char*)&req, sizeof(req));
req.head.pack_len=sizeof(req);
req.head.reply_len=sizeof(conf);
req.head.pack_id=++seq;

req.cdb.opcode=SOPC_INQUIRY;
req.cdb.lun=var->x.lun;
req.cdb.evpd=0;
req.cdb.page=0;
req.cdb.alloc_len=60; /* XXX */

res=sg_enter(var, (void*)&req, sizeof(req), (void*)&conf, sizeof(conf), 0);

if (res==0)
  {
  *obuf=conf.data;
  *len=0; /* XXX */
  }
return res;
}
/*****************************************************************************/
int scsi_load_unload(raw_scsi_var* var, int load, int immediate)
{/*1b*/
int res;
struct
  {
  struct sg_header head;
  SEQ_LOAD_CDB6 cdb;
  } req;
struct
  {
  struct sg_header head;
  char dummy[100];
  } conf;

printf("%s scsi_load_unload(load=%d, immediate=%d)\n",
    nowstr(), load, immediate);

bzero((char*)&req, sizeof(req));
req.head.pack_len=sizeof(req);
req.head.reply_len=sizeof(conf);
req.head.pack_id=++seq;

req.cdb.opcode=SOPC_LOAD_UNLOAD;
req.cdb.lun=var->x.lun;
req.cdb.immed=immediate;
req.cdb.load=load;

res=sg_enter(var, (void*)&req, sizeof(req), (void*)&conf, sizeof(conf), 600000);

var->sanity.filemarks=0;
var->sanity.records=0;
var->sanity.sw_position=-1;
return res;
}
/*****************************************************************************/
int scsi_prevent_allow(raw_scsi_var* var, int prevent)
{/*1e*/
int res;
struct
  {
  struct sg_header head;
  DIR_PREVENT_CDB6 cdb;
  } req;
struct
  {
  struct sg_header head;
  } conf;

printf("%s scsi_prevent_allow(prevent=%d)\n", nowstr(), prevent);

bzero((char*)&req, sizeof(req));
req.head.pack_len=sizeof(req);
req.head.reply_len=sizeof(conf);
req.head.pack_id=++seq;

req.cdb.opcode=SOPC_PREVENT_ALLOW_REMOVAL;
req.cdb.lun=var->x.lun;
req.cdb.prevent=prevent;

res=sg_enter(var, (void*)&req, sizeof(req), (void*)&conf, sizeof(conf), 0);

return res;
}
/*****************************************************************************/
int scsi_erase(raw_scsi_var* var, int immediate, int extend)
{/*19*/
int res;
struct
  {
  struct sg_header head;
  SEQ_ERASE_CDB6 cdb;
  } req;
struct
  {
  struct sg_header head;
  char dummy[100];
  } conf;

printf("%s scsi_erase(immediate=%d, extend=%d)\n", nowstr(), immediate, extend);
if (check_position(var, "scsi_erase")<0) return -1;

bzero((char*)&req, sizeof(req));
req.head.pack_len=sizeof(req);
req.head.reply_len=sizeof(conf);
req.head.pack_id=++seq;

req.cdb.opcode=SOPC_ERASE;
req.cdb.lun=var->x.lun;
req.cdb.immed=immediate;
req.cdb.extend=extend;

res=sg_enter(var, (void*)&req, sizeof(req), (void*)&conf, sizeof(conf), 200000);

var->sanity.filemarks=0;
var->sanity.records=0;
var->sanity.sw_position=-1;
return res;
}
/*****************************************************************************/
int scsi_locate(raw_scsi_var* var, int position, int immediate)
{
printf("scsi_locate not implemented\n");
return -1;
}
/*****************************************************************************/
int scsi_space(raw_scsi_var* var, int code, int count)
{/*11*/
int res;
struct
  {
  struct sg_header head;
  SEQ_SPACE_CDB6 cdb;
  } req;
struct
  {
  struct sg_header head;
  char dummy[100];
  } conf;

printf("%s scsi_space(code=%d, count=%d)\n", nowstr(), code, count);

bzero((char*)&req, sizeof(req));
req.head.pack_len=sizeof(req);
req.head.reply_len=sizeof(conf);
req.head.pack_id=++seq;

req.cdb.opcode=SOPC_SPACE;
req.cdb.lun=var->x.lun;
req.cdb.code=code;
req.cdb.count2=(count>>16)&0xff;
req.cdb.count1=(count>>8)&0xff;
req.cdb.count0=count&0xff;

res=sg_enter(var, (void*)&req, sizeof(req), (void*)&conf, sizeof(conf), 200000);

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
/*****************************************************************************/
int scsi_log_select(raw_scsi_var* var)
{
printf("scsi_log_select not implemented\n");
return -1;
}
/*****************************************************************************/
int scsi_request_sense(raw_scsi_var* var, unsigned char** obuf, int* len)
{/*03*/
int res;
struct
  {
  struct sg_header head;
  ALL_REQ_SENSE_CDB6 cdb;
  } req;
static struct
  {
  struct sg_header head;
  unsigned char data[256];
  } conf;

bzero((char*)&req, sizeof(req));
req.head.pack_len=sizeof(req);
req.head.reply_len=sizeof(conf);
req.head.pack_id=++seq;

req.cdb.opcode=SOPC_REQUEST_SENSE;
req.cdb.lun=var->x.lun;
req.cdb.alloc_len=60; /* XXX */

res=sg_enter(var, (void*)&req, sizeof(req), (void*)&conf, sizeof(conf), 0);

if (res==0)
  {
  *obuf=conf.data;
  *len=0; /* XXX */
  }
return res;
}
/*****************************************************************************/
int scsi_log_sense(raw_scsi_var* var, int page_code,
    unsigned char** obuf, int* len)
{/*4d*/
int res, leng=300;
struct
  {
  struct sg_header head;
  ALL_LOG_SNS_CDB10 cdb;
  } req;
static struct
  {
  struct sg_header head;
  unsigned char data[256];
  } conf;

bzero((char*)&req, sizeof(req));
req.head.pack_len=sizeof(req);
req.head.reply_len=sizeof(conf);
req.head.pack_id=++seq;

req.cdb.opcode=SOPC_LOG_SENSE;
req.cdb.lun=var->x.lun;
req.cdb.pg_code=page_code;
req.cdb.pc=1;
req.cdb.alloc_len1=(leng>>8)&0xff; /* XXX */
req.cdb.alloc_len0=leng&0xff; /* XXX */

res=sg_enter(var, (void*)&req, sizeof(req), (void*)&conf, sizeof(conf), 0);

if (res==0)
  {
  /* scsi_dump_buffer(conf.data, leng); */
  *obuf=conf.data;
  *len=0; /* XXX */
  }
return res;
}
/*****************************************************************************/
#if 0
/*****************************************************************************/
int (raw_scsi_var* var)
{/**/
int res;
struct
  {
  struct sg_header head;
   cdb;
  } req;
struct
  {
  struct sg_header head;
  } conf;

printf("(=%d)\n", );

bzero((char*)&req, sizeof(req));
req.head.pack_len=sizeof(req);
req.head.reply_len=sizeof(conf);
req.head.pack_id=++seq;

req.cdb.opcode=;
req.cdb.lun=var->x.lun;
req.cdb.=;

res=sg_enter(var, (void*)&req, sizeof(req), (void*)&conf, sizeof(conf), 0);

return res;
}
/*****************************************************************************/
#endif
/*****************************************************************************/
