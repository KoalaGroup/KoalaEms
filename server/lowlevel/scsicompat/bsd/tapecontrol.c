/*
 * dataout/cluster/tapecontrol.c
 * 
 * created 15.07.1998 PW
 */
static char *rcsid="$ZEL: tapecontrol.c,v 1.3 2002/09/28 17:41:03 wuestner Exp $";

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/scsiio.h>
#include "../../lowlevel/rawscsi/scsi.h"
#include "handlercom.h"
#include <xdrstring.h>
#include "tapecontrol.h"

static struct scsireq* req;
static unsigned char buf[256];

int init_scsi_commands(int path)
{
req=scsireq_new();
if (req==0)
  {
  printf("scsireq_new failed\n");
  return 0;
  }
return 1;
}

int status_DLT4000(int path, int* m)
{
int res, n;
int* help=m++;
static int logpages[]={2, 0x32, -1};

scsireq_reset(req);
/* request sense */
scsireq_build(req, 256, buf, SCCMD_READ,
  "{opcode} 3:i1"
  "{lun} 0:b3"
  "{res} 0:b5"
  "{res} 0:i2"
  "{length} ff:i1"
  "{unused} 0:b2"
  "{res} 0:b4"
  "{flag} 0:b1"
  "{link} 0:b1");
res=scsireq_enter(path, req);
if (res<0)
  {
  printf("scsireq_enter(request sense): %s\n", strerror(errno));
  *help=m-help-1;
  return m-help;
  }
if (SCSIREQ_ERROR(req))
  {
  printf("scsireq_enter(request sense): scsi error.\n");
  *help=m-help-1;
  return m-help;
  }
scsireq_decode(req,
  "*b1" /* valid*/
  "b7"  /* error code*/
  "s2"
  "i1" /* sense key */
  "s12"
  "i1" /* add sense code */
  "i1" /* add sense code qualifier */
  "s18"
  "i1" /* internal status code */
  "i2" /* tape motion hours */
  "i4",/* power on hours */
   m+0, m+1, m+2, m+3, m+4, m+5, m+6);
m+=7;

for (n=0; logpages[n]>0; n++)
  {
  int idx, rest, page_code, add_length;
  scsireq_reset(req);
  /* log sense */
  scsireq_build(req, 256, buf, SCCMD_READ,
    "{opcode} 4d:i1" /* log sense */
    "{lun} 0:b3"
    "{res} 0:b3"
    "{PPC} 0:b1"
    "{SP} 0:b1"
    "{PC} 1:b2"
    "{page code} v:b6"
    "{res} 0:i2"
    "{parptr} 0:i2"
    "{all. length} ff:i2"
    "{unused} 0:b2"
    "{res} 0:b4"
    "{flag} 0:b1"
    "{link} 0:b1",
    logpages[n]);
  res=scsireq_enter(path, req);
  if (res<0)
    {
    printf("scsireq_enter(log sense sense): %s\n", strerror(errno));
    *help=m-help-1;
    return m-help;
    }
  if (SCSIREQ_ERROR(req))
    {
    printf("scsireq_enter(log sense): scsi error.\n");
    return m-help;
    }
  
  idx=0; rest=req->datalen_used;
  /* decode header */
  scsireq_buff_decode(buf+idx, rest,
    "*b2" /* res */
    "b6"  /* page code */
    "*i1" /* res */
    "i2", /* add. length */
    &page_code, &add_length);
/*
 *   printf("log sense: page code=%d; add. length=%d, datalen_used=%d\n",
 *       page_code, add_length, req->datalen_used);
 */
  if (page_code!=logpages[n])
    {
    printf("log sense: page code=%d, requested code=%d\n",
        page_code, logpages[n]);
    return m-help;
    }
  idx+=4; rest=add_length;
  while (rest>0)
    {
    int par_code, size, data, data1;
    scsireq_buff_decode(buf+idx, rest,
      "i2" /* parameter code */
      "*i1" /* dummy */
      "i1", /* data size */
      &par_code, &size);
    idx+=4; rest-=4;
    switch (size)
      {
      case 2:
        scsireq_buff_decode(buf+idx, rest, "i2", &data);
        break;
      case 4:
        scsireq_buff_decode(buf+idx, rest, "i4", &data);
        break;
      case 8:
        scsireq_buff_decode(buf+idx, rest, "i4 i4", &data, &data1);
        break;
      default:
        printf("log sense: page code %d, parameter code %d, size=%d\n",
          page_code, par_code, size);
        data=0;
      }
    idx+=size; rest-=size;
    switch (page_code)
      {
      case 2: /* write error counter page */
        switch (par_code)
          {
          case 0: /* errors corrected with substantial delays */ break;
          case 1: /* errors corrected with possible delays */ break;
          case 2: /* total rewrites or rereads */
            *m++=data;
            break;
          case 3: /* total errors corrected */
            *m++=data;
            break;
          case 4: /* total times correction algorithm processed */ break;
          case 5: /* total bytes processed */
            *m++=data;
            *m++=data1;
            break;
          case 6: /* total uncorrected errors */
            *m++=data;
            break;
          case 0x8000: /* vendor unique */ break;
          case 0x8001: /* vendor unique */ break;
          case 0x9000: /* vendor unique */ break;
          case 0x9001: /* vendor unique */ break;
          case 0x9002: /* vendor unique */ break;
          case 0x9003: /* vendor unique */ break;
          case 0x9004: /* vendor unique */ break;
          case 0x9005: /* vendor unique */ break;
          case 0x9006: /* vendor unique */ break;
          case 0x9007: /* vendor unique */ break;
          default:
            printf("log sense(A): parameter code=%d, size=%d, data=%d\n",
                par_code, size, data);
          }
        break;
      case 0x32: /* compression ratio page */
        switch (par_code)
          {
          case 0: /* read compr. ratio */ break;
          case 1: /* write compr. ratio */
            *m++=data;
            break;
          case 2: /* MBytes transferred to host */ break;
          case 3: /* Bytes transferred to host */ break;
          case 4: /* MBytes read from tape */ break;
          case 5: /* Bytes read from tape */ break;
          case 6: /* MBytes transferred from host */
            *m++=data;
            break;
          case 7: /* Bytes transferred from host */
            *m++=data;
            break;
          case 8: /* MBytes written to tape */
            *m++=data;
            break;
          case 9: /* Bytes written to tape */
            *m++=data;
            break;
          default:
            printf("log sense(B): parameter code=%d, size=%d, data=%d\n",
                par_code, size, data);
          }
        break;
      default:
        printf("log sense: page code %d not decoded\n");
      }
    }
  }

scsireq_reset(req);
/* read position */
scsireq_build(req, 256, buf, SCCMD_READ,
  "{opcode} 34:i1"
  "{lun}    0:b3"
  "{res}    0:b4"
  "{bt}     0:b1"
  "{res}    0:i4"
  "{res}    0:i3"
  "{unused} 0:b2"
  "{res}    0:b4"
  "{flag}   0:b1"
  "{link}   0:b1");
res=scsireq_enter(path, req);
if (res<0)
  {
  /* printf("scsireq_enter(read position): %s\n", strerror(errno)); */
  *help=m-help-1;
  return m-help;
  }
if (SCSIREQ_ERROR(req))
  {
  printf("scsireq_enter(read position): scsi error.\n");
  *help=m-help-1;
  return m-help;
  }
{
int bop, eop, fbl, lbl, blib, byib;
scsireq_decode(req,
  "b1"  /* BOP */
  "b1"  /* EOP */
  "*b3" /* res */
  "*b1" /* bpu */
  "*b2" /* res */
  "*i1" /* partition */
  "*i2" /* res */
  "i4"  /* first block location */
  "i4"  /* last block location */
  "*i1" /* res */
  "i3"  /* number of blocks in buffer */
  "i4", /* number of bytes in buffer */
   &bop, &eop, &fbl, &lbl, &blib, &byib);
*m++=bop;
*m++=eop;
*m++=fbl;
}

*help=m-help-1;
return m-help;
}

int status_DLT7000(int path, int* m)
{
return 0;
}

int status_EXABYTE(int path, int* m)
{
return 0;
}

void make_tapename(int path)
{
int res;
scsireq_reset(req);
scsireq_build(req, 256, buf, SCCMD_READ,
  "{opcode} 12:i1"
  "{lun}    0:b3"
  "{res}    0:b4"
  "{EVPD}   0:b1"
  "{page code} 0:i1"
  "{res}    0:i1"
  "{legth}  ff:i1"
  "{unused} 0:b2"
  "{res}    0:b4"
  "{flag}   0:b1"
  "{link}   0:b1");
res=scsireq_enter(path, req);
if (res<0)
  {
  printf("scsireq_enter: %s\n", strerror(errno));
  goto fehler;
  }
if (SCSIREQ_ERROR(req))
  {
  int i;
  int valid, errcode, filemrk, eom, sensekey;
  int infobytes, sensecode, sensequal, cd , bpv, bitptr, fieldptr;
  int statuscode; 
  printf("scsireq_enter: scsi error.\n");
  printf("  sense_used=%d\n", req->senselen_used);
  for (i=0; i<req->senselen_used; i++)  printf("%02x ", req->sense[i]);
  printf("\n");
  printf("  data_used=%d\n", req->datalen_used);
  for (i=0; i<req->datalen_used; i++)  printf("%02x ", req->databuf[i]);
  printf("\n");
  scsireq_buff_decode(req->sense, 48,
      "b1"  /* valid */
      "b7"  /* error code */
      "*i1" /* segment */
      "b1"  /* filemark */
      "b1"  /* eom */
      "*b1" /* ili */
      "*b1" /* res */
      "b4"  /* sense key */
      "i4"  /* infobytes */
      "*i1" /* add sense length */
      "*i4" /* command specific bytes */
      "i1"  /* add sense code */
      "i1"  /* add sense code qualifier */
      "*i1" /* sub ass code */
      "*b1" /* sksv */
      "b1"  /* cd */
      "*b2" /* res */
      "b1"  /* bpv */
      "b3"  /* bit ptr */
      "i2"  /* field ptr */
      "i1", /* internal status */
      &valid, &errcode, &filemrk, &eom, &sensekey, &infobytes, &sensecode,
      &sensequal, &cd, &bpv, &bitptr, &fieldptr, &statuscode);
  printf("error code %x, sense key %x, add sense code %x, qual. %x\n",
      errcode, sensekey, sensecode, sensequal);
  printf("filemark %x, eom %x\n", filemrk, eom);
  if (valid) printf("info bytes %x\n", infobytes);
  printf("c/d %x\n", cd);
  if (bpv) printf("bit ptr %x\n", bitptr);
  printf("field ptr %x\n", fieldptr);
  printf("internal status %x\n", statuscode);
  printf("  error code              : %d\n", req->sense[0]);
  printf("  sense key               : %d\n", req->sense[2]&0xf);
  printf("  add sense code          : %d\n", req->sense[12]);
  printf("  add sense code qualifier: %d\n", req->sense[13]);
  goto fehler;
  }
strncpy(tapename, buf+16, 16);
tapename[16]=0;
return;

fehler:
tapename[0]=0;
}

int scsi_control_tape(int path, int* in, int* out)
{
int res=0, code=in[0];
int* help=out;

scsireq_reset(req);
switch (code)
  {
  case hndlcode_none: break;
  case hndlcode_Inquire:
    scsireq_build(req, 256, buf, SCCMD_READ,
      "{opcode} 12:i1"
      "{lun}    0:b3"
      "{res}    0:b4"
      "{EVPD}   0:b1"
      "{page code} 0:i1"
      "{res}    0:i1"
      "{legth}  ff:i1"
      "{unused} 0:b2"
      "{res}    0:b4"
      "{flag}   0:b1"
      "{link}   0:b1");
    break;
  case hndlcode_Load:
    scsireq_build(req, 256, buf, SCCMD_READ,
      "{opcode} 1b:i1"
      "{lun}    0:b3"
      "{res}    0:b4"
      "{immed}  1:b1"
      "{res}    0:i2"
      "{res}    0:b5"
      "{eot}    0:b1"
      "{re-ten} 0:b1"
      "{load}   v:b1"
      "{unused} 0:b2"
      "{res}    0:b4"
      "{flag}   0:b1"
      "{link}   0:b1", !!in[1]);
    req->timeout=in[1]?60000:120000;
    break;
  case hndlcode_Prevent:
    scsireq_build(req, 256, buf, SCCMD_READ,
      "{opcode} 1e:i1"
      "{lun}    0:b3"
      "{res}    0:i2"
      "{res}    0:b7"
      "{prevent}v:b1"
      "{unused} 0:b2"
      "{res}    0:b4"
      "{flag}   0:b1"
      "{link}   0:b1", in[1]!=0);
    break;
  case hndlcode_Erase:
    printf("auftape_%d::Erase\n", pid);
    scsireq_build(req, 256, buf, SCCMD_READ,
      "{opcode} 19:i1"
      "{lun}    0:b3"
      "{res}    0:b3"
      "{immed}  1:b1"
      "{long}   1:b1"
      "{res}    0:i3"
      "{unused} 0:b2"
      "{res}    0:b4"
      "{flag}   0:b1"
      "{link}   0:b1");
    req->timeout=14000000;
    break;
  case hndlcode_Locate:
    printf("auftape_%d::Locate(%d)\n", pid, in[1]);
    scsireq_build(req, 256, buf, SCCMD_READ,
      "{opcode} 2b:i1"
      "{lun}    0:b3"
      "{res}    0:b2"
      "{bt}     0:b1"
      "{cp}     0:b1"
      "{immed}  1:b1"
      "{res}    0:i1"
      "{addr}   v:i4"
      "{res}    0:i1"
      "{partit} 0:i1"
      "{unused} 0:b2"
      "{res}    0:b4"
      "{flag}   0:b1"
      "{link}   0:b1", in[1]);
    req->timeout=100000;
    break;
  case hndlcode_Space:
    {
    int i;
    printf("auftape_%d::Space(%d, %d)\n", pid, in[1], in[2]);
    scsireq_build(req, 256, buf, SCCMD_READ,
      "{opcode} 11:i1"
      "{lun}    0:b3"
      "{res}    0:b2"
      "{code}   v:b3"
      "{count}  v:i3"
      "{unused} 0:b2"
      "{res}    0:b4"
      "{flag}   0:b1"
      "{link}   0:b1", in[1], in[2]);
    req->timeout=100000;
    }
    break;
  case hndlcode_Rewind:
    printf("auftape_%d::Rewind\n", pid);
    scsireq_build(req, 256, buf, SCCMD_READ,
      "{opcode} 1:i1"
      "{lun}    0:b3"
      "{res}    0:b4"
      "{immed}  1:b1"
      "{res}    0:i3"
      "{unused} 0:b2"
      "{res}    0:b4"
      "{flag}   0:b1"
      "{link}   0:b1");
    req->timeout=100000;
    break;
  case hndlcode_ReadPos:
    scsireq_build(req, 256, buf, SCCMD_READ,
      "{opcode} 34:i1"
      "{lun}    0:b3"
      "{res}    0:b4"
      "{bt}     0:b1"
      "{res}    0:i4"
      "{res}    0:i3"
      "{unused} 0:b2"
      "{res}    0:b4"
      "{flag}   0:b1"
      "{link}   0:b1");
    break;
  case hndlcode_ClearLog:
    printf("auftape_%d::ClearLog(%d)\n", pid, in[1]);
    scsireq_build(req, 256, buf, SCCMD_READ,
      "{opcode} 4c:i1" /* log select */
      "{lun} 0:b3"
      "{res} 0:b3"
      "{PCR} 1:b1"
      "{SP} 0:b1"
      "{PC} 1:b2"
      "{res} 0:i4"
      "{par. list length} 0:i2"
      "{unused} 0:b2"
      "{res} 0:b4"
      "{flag} 0:b1"
      "{link} 0:b1");
    break;
  case hndlcode_Filemark: break;
  default: printf("hndlcode (A): %d; not decoded\n", code);
  }

res=scsireq_enter(path, req);
if (res<0)
  {
  printf("scsireq_enter: %s\n", strerror(errno));
  *out++=1;
  return 1;
  }
if (SCSIREQ_ERROR(req))
  {
  int i;
  int valid, errcode, filemrk, eom, sensekey;
  int infobytes, sensecode, sensequal, cd , bpv, bitptr, fieldptr;
  int statuscode; 
  printf("auftape_%d::scsireq_enter: scsi error.\n", pid);
  *out++=2;
  printf("  sense_used=%d\n", req->senselen_used);
  for (i=0; i<req->senselen_used; i++)  printf("%02x ", req->sense[i]);
  printf("\n");
  printf("  data_used=%d\n", req->datalen_used);
  for (i=0; i<req->datalen_used; i++)  printf("%02x ", req->databuf[i]);
  printf("\n");
  scsireq_buff_decode(req->sense, 48,
      "b1"  /* valid */
      "b7"  /* error code */
      "*i1" /* segment */
      "b1"  /* filemark */
      "b1"  /* eom */
      "*b1" /* ili */
      "*b1" /* res */
      "b4"  /* sense key */
      "i4"  /* infobytes */
      "*i1" /* add sense length */
      "*i4" /* command specific bytes */
      "i1"  /* add sense code */
      "i1"  /* add sense code qualifier */
      "*i1" /* sub ass code */
      "*b1" /* sksv */
      "b1"  /* cd */
      "*b2" /* res */
      "b1"  /* bpv */
      "b3"  /* bit ptr */
      "i2"  /* field ptr */
      "i1", /* internal status */
      &valid, &errcode, &filemrk, &eom, &sensekey, &infobytes, &sensecode,
      &sensequal, &cd, &bpv, &bitptr, &fieldptr, &statuscode);
  printf("  error code            : %xh\n", errcode);
  printf("  sense key             : %xh\n", sensekey);
  printf("  add. sense code       : %xh\n", sensecode);
  printf("  add. sense code qual. : %xh\n", sensequal);
  printf("  internal status code  : %xh\n", statuscode);
  printf("  filemark %x, eom %x\n", filemrk, eom);
  if (valid) printf("  info bytes            : %x\n", infobytes);
  printf("  error in %s\n", cd?"cdb":"parameter list");
  if (bpv)
  printf("  bit ptr               : %xh\n", bitptr);
  printf("  field ptr             : %xh\n", fieldptr);
  *out++=sensekey;
  *out++=sensecode;
  *out++=sensequal;
  }
else
  {
  int i;
  *out++=0;
  switch (code)
    {
    case hndlcode_none: break;
    case hndlcode_Inquire:
      {
      *out++=2;
      out=outnstring(out, buf+16, 16);
      out=outnstring(out, buf+8, 8);
      }
      break;
    case hndlcode_ReadPos:
      {
      int bop, eop, fbl, lbl, blib, byib;
      scsireq_decode(req,
        "b1"  /* BOP */
        "b1"  /* EOP */
        "*b3" /* res */
        "*b1" /* bpu */
        "*b2" /* res */
        "*i1" /* partition */
        "*i2" /* res */
        "i4"  /* first block location */
        "i4"  /* last block location */
        "*i1" /* res */
        "i3"  /* number of blocks in buffer */
        "i4", /* number of bytes in buffer */
         &bop, &eop, &fbl, &lbl, &blib, &byib);
      *out++=fbl;
      *out++=lbl;
      *out++=bop;
      *out++=eop;
      *out++=blib;
      *out++=byib;
      }
      break;
    case hndlcode_Load: break;
    case hndlcode_Erase: break;
    case hndlcode_Locate: break;
    case hndlcode_ClearLog: break;
    case hndlcode_Rewind: break;
    case hndlcode_Space: break;
    case hndlcode_Filemark: break;
    case hndlcode_Prevent: break;
    default: printf("hndlcode (B): %d; not decoded\n", code);
    }
  }
return out-help;
}
