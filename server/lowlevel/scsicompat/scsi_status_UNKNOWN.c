/*
 * lowlevel/scsicompat/scsi_status_UNKNOWN.c
 * 
 * created 01.04.1999 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: scsi_status_UNKNOWN.c,v 1.5 2011/04/06 20:30:28 wuestner Exp $";

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
int status_UNKNOWN(raw_scsi_var* device, char* name, int* m)
{
int* help=m++;
int len;

{
int sense_key, add_sense_code, add_sense_code_qualifier;
if (scsi_request_sense(device, &buf, &len)<0)
  {
  printf("status_UNKNOWN: scsi_request_sense failed.\n");
  *help=m-help-1;
  return -1;
  }
printf("status_UNKNOWN; request_sense:\n");
scsi_dump_buffer(buf, len);

scsireq_buff_decode(buf, len,
  "*b1" /* valid*/
  "*b7" /* error code*/
  "s2"
  "i1"  /* sense key (byte 2) */
  "s12"
  "i1"  /* add sense code */
  "i1",  /* add sense code qualifier */
   &sense_key, &add_sense_code, &add_sense_code_qualifier);

*m++=sense_key;
*m++=add_sense_code;
*m++=add_sense_code_qualifier;
}

{
int pos;
scsi_position_data data;

if (scsi_read_position(device, &data)<0)
  {
  printf("status_UNKNOWN: scsi_read_position failed.\n");
  *help=m-help-1;
  return m-help;
  }
printf("status_UNKNOWN; read_position:\n");
scsi_dump_buffer(buf, len);

pos=0;
if (data.bop) pos|=1;
if (data.eop) pos|=2;
if (pos==3) printf("status_UNKNOWN: BOP=1 and EOP=1!\n");
*m++=pos;
if (data.bpu)
  {
  data.fbl=-1;
  data.lbl=-1;
  }
*m++=data.fbl;
*m++=data.lbl;
*m++=data.blib;
*m++=data.byib;
}

*help=m-help-1;
/* printf("*help=0x%x\n", *help);*/
return m-help;
}
/*****************************************************************************/
/*****************************************************************************/
