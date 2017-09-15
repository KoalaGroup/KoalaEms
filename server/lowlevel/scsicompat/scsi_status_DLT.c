/*
 * lowlevel/scsicompat/scsi_status_DLT.c
 * 
 * created 01.04.1999 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: scsi_status_DLT.c,v 1.5 2011/04/06 20:30:27 wuestner Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rcs_ids.h>
#include "scsicompat.h"

RCS_REGISTER(cvsid, "lowlevel/scsicompat")

static unsigned char* buf;
/*****************************************************************************/
/*
 * output:
 *   (request_sense)
 * error code
 * sense key
 * add sense code
 * add sense code qualifier
 * internal status code
 * tape motion hours
 * power on hours
 *   (log_sense)
 * total rewrites or rereads
 * total errors corrected
 * total bytes processed (64 bit)
 * total uncorrected errors
 * write compression ratio
 * MBytes transferred from host
 * Bytes transferred from host
 * MBytes written to tape
 * Bytes written to tape
 * bop (begin of tape)
 * eop (end of tape)
 * fbl (first block location)
 *
 * if (DLT7000) remaining tape (in 4K blocks)
 */
int status_DLT(raw_scsi_var* device, char* name, int* m)
{
int n, len;
int* help=m++;
int tape_remaining=-1;
int drive_temperature=-1;
static int logpages[]={2, 0x32, 0x3e, -1};
/* printf("status_DLT\n"); */

if (scsi_request_sense(device, &buf, &len)<0)
  {
  printf("status_DLT: scsi_request_sense failed.\n");
  *help=m-help-1;
  return -1;
  }
if ((strncmp(name, "DLT7", 4)==0) || (strncmp(name, "DLT8", 4)==0))
  {
  scsireq_buff_decode(buf, len,
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
    "i4" /* power on hours */
    "i4",/* tape remaining */
     m+0, m+1, m+2, m+3, m+4, m+5, m+6, &tape_remaining);
  }
else
  {
  scsireq_buff_decode(buf, len,
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
  }
m+=7;
for (n=0; logpages[n]>0; n++)
  {
  int rest, idx;
  int page_code, add_length;
  if (scsi_log_sense(device, logpages[n], &buf, &len)<0)
    {
    printf("status_DLT: scsi_log_sense(%d) failed.\n", logpages[n]);
    *help=m-help-1;
    return -1;
    }
  idx=0; rest=len;
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
      case 0x3e: /* device status page */
        switch (par_code)
          {
          case 0: /* device type */ break;
          case 1: /* cleaning related status */ break;
          case 2: /* total number of loads */ break;
          case 3: /* number of cleaning sessions per cartridge */ break;
          case 4: /* vendor unique */ break;
          case 5: /* drive temperature */
            drive_temperature=data;
            break;
          default:
            printf("log sense(C): parameter code=%d, size=%d, data=%d\n",
                par_code, size, data);
          }
        break;
      default:
        printf("log sense: page code %d not decoded\n", page_code);
      }
    }
  }

{
scsi_position_data position_data;
if (scsi_read_position(device, &position_data)<0)
  {
  printf("status_DLT: scsi_read_position failed.\n");
  *help=m-help-1;
  return m-help;
  }
*m++=position_data.bop;
*m++=position_data.eop;
*m++=position_data.fbl;
}
if (tape_remaining!=-1) *m++=tape_remaining;
if (drive_temperature!=-1) *m++=drive_temperature;
*help=m-help-1;
return m-help;
}
/*****************************************************************************/
/*****************************************************************************/
