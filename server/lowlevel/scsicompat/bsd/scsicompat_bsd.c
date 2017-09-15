/*
 * lowlevel/scsicompat/bsd/scsicompat_bsd.c
 * created 08.02.1999 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: scsicompat_bsd.c,v 1.6 2007/10/15 14:23:58 wuestner Exp $";

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/scsiio.h>
#include "../../../main/nowstr.h"
#include "../scsicompat.h"
#include "scsicompat_bsd_local.h"

static unsigned char buf[256];
static struct scsireq* req;

/*****************************************************************************/
int scsicompat_init_(char* filename, raw_scsi_var* var, int pid)
{
req=scsireq_new();
if (req==0)
  {
  printf("scsicompat_init_%d: scsireq_new failed\n", pid);
  return -1;
  }
printf("scsicompat_init_%d: try open %s\n", pid, filename);
var->x.path=open(filename, O_RDWR, 0); /* MUST NOT be O_WRONLY on BSD */
printf("scsicompat_init_%d: nach open: path=%d\n", pid, var->x.path);
if (var->x.path<0)
  {
  printf("scsicompat_init_%d: %s not open: %s\n", pid, filename, strerror(errno));
  free(req);
  return -1;
  }
printf("scsicompat_init_%d: %s open\n", pid, filename);
if (fcntl(var->x.path, F_SETFD, FD_CLOEXEC)<0)
  {
  printf("scsicompat_bsd.c: fcntl(campath, FD_CLOEXEC): %s\n", strerror(errno));
  }

var->x.lun=0; /* immer? */
return 0;
}

int scsicompat_done_(raw_scsi_var* var)
{
close(var->x.path);
free(req);
return 0;
}
/*****************************************************************************/
static void scsi_decode_error_(raw_scsi_var* var, struct scsireq* req)
{
  scsi_decode_error(var, req->flags,
    req->cmd, req->cmdlen, req->sense, req->senselen_used,
    req->databuf, req->datalen_used);
}
/*****************************************************************************/
#if 0
#define SENSEBUFLEN 48

typedef struct  scsireq {
        u_long  flags;          /* info about the request status and type */
        u_long  timeout;
        u_char  cmd[16];        /* 12 is actually the max */
        u_char  cmdlen;
        caddr_t databuf;        /* address in user space of buffer */
        u_long  datalen;        /* size of user buffer (request) */
        u_long  datalen_used;   /* size of user buffer (used)*/
        u_char  sense[SENSEBUFLEN]; /* returned sense will be in here */
        u_char  senselen;       /* sensedata request size (MAX of SENSEBUFLEN)*/
        u_char  senselen_used;  /* return value only */
        u_char  status;         /* what the scsi status was from the adapter */
        u_char  retsts;         /* the return status for the command */
        int     error;          /* error bits */
} scsireq_t;
#endif
#if 0
static void show_scsireq_enter(raw_scsi_var* var, scsireq_t* req)
{
static int writecount=0;
int command;
command=req->cmd[0];
if (command==0xa)
  {
  writecount++;
  return;
  }
if (writecount)
  {
  printf("scsi_write: %d times\n", writecount);
  writecount=0;
  }
switch (command)
  {
  case 0x1: printf("scsi_rewind\n"); break;
  case 0x3: printf("scsi_request_sense\n"); break;
  case 0x5: printf("scsi_read_block_limits\n"); break;
  case 0x10: printf("scsi_write_filemark\n"); break;
  case 0x11: printf("scsi_space\n"); break;
  case 0x12: printf("scsi_inquiry\n"); break;
  case 0x15: printf("scsi_mode_select\n"); break;
  case 0x16: printf("scsi_reserve_unit\n"); break;
  case 0x17: printf("scsi_release_unit\n"); break;
  case 0x19: printf("scsi_erase\n"); break;
  case 0x1a:
  case 0x5a: printf("scsi_mode_sense\n"); break;
  case 0x1b: printf("scsi_load_unload\n"); break;
  case 0x1e: printf("scsi_prevent_allow\n"); break;
  case 0x2b: printf("scsi_locate\n"); break;
  case 0x34: printf("scsi_read_position\n"); break;
  case 0x4c: printf("scsi_log_select\n"); break;
  case 0x4d: printf("scsi_log_sense\n"); break;
  default: printf("scsi_%d\n", command); break;
  }
}
#endif
/*****************************************************************************/
static int suppress_error_reporting(raw_scsi_var* var, scsireq_t* req)
{
return 0;
if ((req->cmd[0]==0x34) &&                  /* read position */
    (var->scsi_error.sense_key==2) &&       /* not ready */
    (var->scsi_error.add_sense_code==0x3a)) /* tape not present */
  return 1;
return 0;
}
/*****************************************************************************/
static int scsireq_enter_(raw_scsi_var* var, scsireq_t* req)
{
int res, media_changed, count=0;

do
  {
  /*show_scsireq_enter(var, req);*/
  res=scsireq_enter(var->x.path, req);
  if (res<0)
    {
    printf("???????????????????????\n");
    printf("scsireq_enter: %s\n", strerror(errno));
    var->scsi_error.error_code=-1;
    return -1;
    }
  var->scsi_error.eom=0;
  if (SCSIREQ_ERROR(req))
    {
    printf("!!!!!!!!!!!!!!!!!!!!!!! count=%d\n", count);
    if (!suppress_error_reporting(var, req)) scsi_decode_error_(var, req);
    media_changed=(res==0)&&
        SCSIREQ_ERROR(req)&&
        (var->scsi_error.sense_key==6)&&
        (var->scsi_error.add_sense_code==0x28);

    if (media_changed) printf("%s MEDIA CHANGED!!\n", nowstr());
    if (!(media_changed && var->policy.ignore_medium_changed && ++count<3))
      {
      if ((var->scsi_error.sense_key==0) &&
          (var->scsi_error.add_sense_code==0) &&
          (var->scsi_error.add_sense_code_qual==2) &&
          (var->scsi_error.eom==1))
        {
        printf("scsireq_enter_: early end of tape warning\n");
        return 0;
        }
      else
        return -1;
      }
    }
  else
    media_changed=0;
  }
while (media_changed);
return 0;
}
/*****************************************************************************/
int scsi_rewind(raw_scsi_var* var, int immediate)
{/*01*/
int res;
printf("scsi_rewind\n");
scsireq_reset(req);

scsireq_build(req, 0, buf, SCCMD_READ,
  "{opcode} 1:i1"
  "{lun}    v:b3"
  "{res}    0:b4"
  "{immed}  v:b1"
  "{res}    0:i3"
  "{unused} 0:b2"
  "{res}    0:b4"
  "{flag}   0:b1"
  "{link}   0:b1", var->x.lun, immediate);
req->timeout=600000;
if ((res=scsireq_enter_(var, req))<0) printf("error in scsi_rewind\n");
var->sanity.sw_position=0;
var->sanity.filemarks=0;
var->sanity.records=0;
return res;
}
/*****************************************************************************/
int scsi_request_sense(raw_scsi_var* var, unsigned char** obuf, int* len)
{/*03*/
int res;
/* printf("scsi_request_sense\n"); */
scsireq_reset(req);

scsireq_build(req, 256, buf, SCCMD_READ,
  "{opcode} 3:i1"
  "{lun}    v:b3"
  "{res}    0:b5"
  "{res}    0:i2"
  "{length} ff:i1"
  "{unused} 0:b2"
  "{res}    0:b4"
  "{flag}   0:b1"
  "{link}   0:b1", var->x.lun);
res=scsireq_enter_(var, req);
*obuf=buf;
*len=req->datalen_used;
return res;
}
/*****************************************************************************/
/*05*/ int scsi_read_block_limits(raw_scsi_var* var, unsigned char** obuf,
    int* len)
{
int res;
printf("scsi_read_block_limits\n");
scsireq_reset(req);

scsireq_build(req, 256, buf, SCCMD_READ,
  "{opcode} 5:i1"
  "{lun}    v:b3"
  "{res}    0:b5"
  "{res}    0:i3"
  "{unused} 0:b2"
  "{res}    0:b4"
  "{flag}   0:b1"
  "{link}   0:b1", var->x.lun);
res=scsireq_enter_(var, req);
*obuf=buf;
*len=req->datalen_used;
return res;
}
/*****************************************************************************/
int scsi_write(raw_scsi_var* var, int num, int* data)
{/*0a*/
int res;

/*if (check_position(var, "scsi_write")<0) return -1;*/

scsireq_reset(req);
scsireq_build(req, 0, buf, SCCMD_WRITE,
  "{opcode} 0a:i1"
  "{lun}    v:b3"
  "{res}    0:b4"
  "{fixed}  0:b1"
  "{length} v:i3"
  "{unused} 0:b2"
  "{res}    0:b4"
  "{flag}   0:b1"
  "{link}   0:b1", var->x.lun, num*4);

req->timeout=180000;
req->databuf=(caddr_t)data;
req->datalen=num*4;

if ((res=scsireq_enter_(var, req))==0)
  {
  var->sanity.sw_position++;
  var->sanity.records++;
  }
var->sanity.filemarks=0;
if (res!=0) printf("scsi_write: res=%d\n", res);
return res;
}
/*****************************************************************************/
int scsi_write_filemark(raw_scsi_var* var, int num, int immediate)
{/*10*/
int res;
printf("%s scsi_write_filemark; records=%d, filemarks=%d, position=%d\n",
  nowstr(), var->sanity.records, var->sanity.filemarks, var->sanity.sw_position);
if (check_position(var, "scsi_write_filemark")<0) return -1;


scsireq_reset(req);

scsireq_build(req, 0, buf, SCCMD_WRITE,
  "{opcode} 10:i1"
  "{lun}    v:b3"
  "{res}    0:b3"
  "{WSmk}   0:b1"
  "{immed}  v:b1"
  "{num}    v:i3"
  "{unused} 0:b2"
  "{res}    0:b4"
  "{flag}   0:b1"
  "{link}   0:b1", var->x.lun, immediate, num);

req->timeout=180000;
if ((res=scsireq_enter_(var, req))==0)
  var->sanity.sw_position+=num;
else
  printf("%s error in scsi_write_filemark\n", nowstr());
var->sanity.records=0;
var->sanity.filemarks+=num;
return res;
}
/*****************************************************************************/
int scsi_space(raw_scsi_var* var, int code, int count)
{/*11*/
int res;
printf("%s scsi_space(code=%d, count=%d)\n", nowstr(), code, count);
scsireq_reset(req);

scsireq_build(req, 0, buf, SCCMD_READ,
  "{opcode} 11:i1"
  "{lun}    v:b3"
  "{res}    0:b2"
  "{code}   v:b3" /*0: blocks, 1: filemarks, 3: eod, 4: setmarks*/
  "{count}  v:i3"
  "{unused} 0:b2"
  "{res}    0:b4"
  "{flag}   0:b1"
  "{link}   0:b1", var->x.lun, code, count);
req->timeout=20000000;
if ((res=scsireq_enter_(var, req))==0)
  set_position(var, "scsi_space");
else
  {
  printf("error in scsi_space\n");
  var->sanity.sw_position=-1;
  }
var->sanity.records=0;
var->sanity.filemarks++;
return res;
}
/*****************************************************************************/
int scsi_inquiry(raw_scsi_var* var, unsigned char** obuf, int* len)
{/*12*/
int res;
/*printf("scsi_inquiry\n");*/
scsireq_reset(req);

scsireq_build(req, 256, buf, SCCMD_READ,
  "{opcode} 12:i1"
  "{lun}    v:b3"
  "{res}    0:b4"
  "{EVPD}   0:b1"
  "{page code} 0:i1"
  "{res}    0:i1"
  "{length} ff:i1"
  "{unused} 0:b2"
  "{res}    0:b4"
  "{flag}   0:b1"
  "{link}   0:b1", var->x.lun);
res=scsireq_enter_(var, req);
*obuf=buf;
*len=req->datalen_used;
return res;
}
/*****************************************************************************/
int scsi_mode_select(raw_scsi_var* var, int density)
{/*15*/
int length;
printf("%s scsi_mode_select(density=%d)\n", nowstr(), density);
scsireq_reset(req);

length=12;
scsireq_build(req, 256, buf, SCCMD_WRITE,
  "{opcode} 15:i1"
  "{lun}    v:b3"
  "{PF}     1:b1"
  "{res}    0:b3"
  "{SP}     0:b1"
  "{res}    0:i2"
  "{length} v:i1"
  "{unused} 0:b2"
  "{res}    0:b4"
  "{flag}   0:b1"
  "{link}   0:b1", var->x.lun, length);

/*req->datalen=length;*/
scsireq_encode(req,
  "{res}       0:i1"
  "{media}     0:i1"
  "{ign}       0:b1"
  "{buffmode}  1:b3"
  "{speed}     0:b4"
  "{length}    8:i1"
  /* Block Descriptor */
  "{dens code} v:i1"
  "{blocknum}  0:i3"
  "{res}       0:i1"
  "{bllength}  0:i3"
  /* no pages */
  , density);
return scsireq_enter_(var, req);
}
/*****************************************************************************/
int scsi_reserve_unit(raw_scsi_var* var, int thirt_party)
{/*16*/
printf("scsi_reserve_unit\n");
scsireq_reset(req);

scsireq_build(req, 0, buf, SCCMD_READ,
  "{opcode} 16:i1"
  "{lun}     v:b3"
  "{3rdPty}  v:b1"
  "{3rdPty ID} v:b3"
  "{res}     0:b1"
  "{res}     0:i3"
  "{unused}  0:b2"
  "{res}     0:b4"
  "{flag}    0:b1"
  "{link}    0:b1", var->x.lun, thirt_party>=0?1:0,
      thirt_party>=0?thirt_party:0);
return scsireq_enter_(var, req);
}
/*****************************************************************************/
int scsi_release_unit(raw_scsi_var* var, int thirt_party)
{/*17*/
printf("scsi_release_unit\n");
scsireq_reset(req);

scsireq_build(req, 0, buf, SCCMD_READ,
  "{opcode} 17:i1"
  "{lun}     v:b3"
  "{3rdPty}  v:b1"
  "{3rdPty ID} v:b3"
  "{res}     0:b1"
  "{res}     0:i3"
  "{unused}  0:b2"
  "{res}     0:b4"
  "{flag}    0:b1"
  "{link}    0:b1", var->x.lun, thirt_party>=0?1:0,
      thirt_party>=0?thirt_party:0);
return scsireq_enter_(var, req);
}
/*****************************************************************************/
int scsi_erase(raw_scsi_var* var, int immediate, int extend)
{/*19*/
int res;
printf("%s scsi_erase(immediate=%d, extend=%d)\n", nowstr(), immediate, extend);
scsireq_reset(req);

scsireq_build(req, 0, buf, SCCMD_READ,
  "{opcode} 19:i1"
  "{lun}     v:b3"
  "{res}     0:b3"
  "{immed}   v:b1"
  "{long}    1:b1"
  "{res}     0:i3"
  "{unused}  0:b2"
  "{res}     0:b4"
  "{flag}    0:b1"
  "{link}    0:b1", var->x.lun, immediate);
req->timeout=20000000;

res=scsireq_enter_(var, req);
var->sanity.filemarks=0;
var->sanity.records=0;
var->sanity.sw_position=-1;
return res;
}
/*****************************************************************************/
int scsi_mode_sense(raw_scsi_var* var, int page_control, int page_code,
    unsigned char** obuf, int* len)
{/*1a/5a*/
int res;
printf("scsi_mode_sense\n");
scsireq_reset(req);

scsireq_build(req, 256, buf, SCCMD_READ,
  "{opcode} 5a:i1"
  "{lun}     v:b3"
  "{res}     0:b1"
  "{DBD}     0:b1"
  "{res}     0:b3"
  "{PC}      v:b2"
  "{page code} v:b6"
  "{res}    0:i4"
  "{length} ff:i2:"
  "{unused} 0:b2"
  "{res}    0:b4"
  "{flag}   0:b1"
  "{link}   0:b1", var->x.lun, page_control, page_code);
res=scsireq_enter_(var, req);
*obuf=buf;
*len=req->datalen_used;
return res;
}
/*****************************************************************************/
int scsi_load_unload(raw_scsi_var* var, int load, int immediate)
{/*1b*/
int res;
printf("%s scsi_load_unload(load=%d, immediate=%d)\n", nowstr(), load,
    immediate);
scsireq_reset(req);

scsireq_build(req, 0, buf, SCCMD_READ,
  "{opcode} 1b:i1"
  "{lun}     v:b3"
  "{res}     0:b4"
  "{immed}   v:b1"
  "{res}     0:i2"
  "{res}     0:b5"
  "{EOT}     0:b1"
  "{ReTen}   0:b1"
  "{load}    v:b1"
  "{unused}  0:b2"
  "{res}     0:b4"
  "{flag}    0:b1"
  "{link}    0:b1", var->x.lun, immediate, load);
req->timeout=20000000;
res=scsireq_enter_(var, req);
var->sanity.filemarks=0;
var->sanity.records=0;
var->sanity.sw_position=-1;
return res;
}
/*****************************************************************************/
int scsi_prevent_allow(raw_scsi_var* var, int prevent)
{/*1e*/
printf("%s scsi_prevent_allow(prevent=%d)\n", nowstr(), prevent);
scsireq_reset(req);

scsireq_build(req, 0, buf, SCCMD_READ,
  "{opcode} 1e:i1"
  "{lun}     v:b3"
  "{res}     0:b5"
  "{res}     0:i2"
  "{res}     0:b7"
  "{prevent} v:b1"
  "{unused}  0:b2"
  "{res}     0:b4"
  "{flag}    0:b1"
  "{link}    0:b1", var->x.lun, prevent);
return scsireq_enter_(var, req);
}
/*****************************************************************************/
int scsi_locate(raw_scsi_var* var, int position, int immediate)
{/*2b*/
int res;
printf("scsi_locate\n");
scsireq_reset(req);

scsireq_build(req, 0, buf, SCCMD_READ,
  "{opcode} 2b:i1"
  "{lun}     v:b3"
  "{res}     0:b2"
  "{BT}      0:b1"
  "{CP}      0:b1"
  "{immed}   v:b1"
  "{res}     0:i1"
  "{addr}    v:i4"
  "{res}     0:i1"
  "{partition} 0:i1"
  "{unused}  0:b2"
  "{res}     0:b4"
  "{flag}    0:b1"
  "{link}    0:b1", var->x.lun, immediate, position);
req->timeout=20000000;
res=scsireq_enter_(var, req);
var->sanity.filemarks=0;
var->sanity.records=0;
var->sanity.sw_position=-1;
return res;
}
/*****************************************************************************/
int scsi_read_position(raw_scsi_var* var, scsi_position_data* pos)
{/*34*/
/* printf("scsi_read_position\n"); */
scsireq_reset(req);

scsireq_build(req, 256, buf, SCCMD_READ,
  "{opcode} 34:i1"
  "{lun}     v:b3"
  "{res}     0:b4"
  "{BT}      0:b1"
  "{res}     0:i4"
  "{res}     0:i3"
  "{unused}  0:b2"
  "{res}     0:b4"
  "{flag}    0:b1"
  "{link}    0:b1", var->x.lun);
req->timeout=60000;
if (scsireq_enter_(var, req)<0) return -1;

scsireq_buff_decode(buf, req->datalen_used,
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
return 0;
}
/*****************************************************************************/
int scsi_log_select(raw_scsi_var* var)
{/*4c*/
/* here only used to clear cumulative values */
printf("scsi_log_select\n");
scsireq_reset(req);

scsireq_build(req, 256, buf, SCCMD_WRITE,
  "{opcode} 4c:i1"
  "{lun}     v:b3"
  "{res}     0:b3"
  "{PCR}     0:b1"
  "{SP}      0:b1"
  "{PC}      3:b2"
  "{res}     0:b6"
  "{res}     0:i4"
  "{length}  0:i2"
  "{unused}  0:b2"
  "{res}     0:b4"
  "{flag}    0:b1"
  "{link}    0:b1", var->x.lun);
return scsireq_enter_(var, req);
}
/*****************************************************************************/
int scsi_log_sense(raw_scsi_var* var, int page_code, unsigned char** obuf,
    int* len)
{/*4d*/
/* here only used to obtain cumulative values */
int res;
/* printf("scsi_log_sense\n"); */
scsireq_reset(req);

scsireq_build(req, 256, buf, SCCMD_READ,
  "{opcode} 4d:i1"
  "{lun}     v:b3"
  "{res}     0:b3"
  "{PPC}     0:b1"
  "{SP}      0:b1"
  "{PC}      1:b2"
  "{page code} v:b6"
  "{res}     0:i2"
  "{par pointer} 0:i2"
  "{length}  ff:i2"
  "{unused}  0:b2"
  "{res}     0:b4"
  "{flag}    0:b1"
  "{link}    0:b1", var->x.lun, page_code);
res=scsireq_enter_(var, req);
*obuf=buf;
*len=req->datalen_used;
return res;
}
/*****************************************************************************/
/*****************************************************************************/
