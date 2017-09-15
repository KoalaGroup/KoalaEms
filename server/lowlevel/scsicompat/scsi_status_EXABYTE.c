/*
 * lowlevel/scsicompat/scsi_status_EXABYTE.c
 * 
 * created 01.04.1999 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: scsi_status_EXABYTE.c,v 1.4 2011/04/06 20:30:28 wuestner Exp $";

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
 * remaining tape
 * read/write data error counter
 * byte2<<24+byte19<<16+byte20<<8+byte21
 * (add sense code<<8)+add sense code qualifier
 * 
 * old format:
 * 0 1 1 0x0 0x0 0x20000 0x0
 * | | | |   |   |       \ add. sense code
 * | | | |   |   \ byte2..21
 * | | | |   \ error counter
 * | | | \ remaining tape
 * | | \ disabled
 * | \ fertig
 * \ fehlercode
 */
int status_EXABYTE(raw_scsi_var* device, char* name, int* m)
{
int* help=m++;
int len;
int byte_2, byte_19, byte_20, byte_21, add_sense_code, add_sense_code_qualifier,
    data_error_counter, tape_remaining;

/*printf("status_EXABYTE\n");*/

if (scsi_request_sense(device, &buf, &len)<0)
  {
  printf("status_EXABYTE: scsi_request_sense failed.\n");
  *help=m-help-1;
  return -1;
  }
scsireq_buff_decode(buf, len,
  "*b1" /* valid*/
  "*b7" /* error code*/
  "s2"
  "i1"  /* sense key (byte 2) */
  "s12"
  "i1"  /* add sense code */
  "i1"  /* add sense code qualifier */
  "s16"
  "i3"  /* read/write data error counter */
  "i1"  /* byte 19 */
  "i1"  /* byte 20 */
  "i1"  /* byte 21 */
  "s23"
  "i3", /* tape remaining */
   &byte_2, &add_sense_code, &add_sense_code_qualifier,
   &data_error_counter, &byte_19, &byte_20, &byte_21, &tape_remaining);

/* printf("19: %x 20: %x 21: %x\n",  byte_19, byte_20, byte_21);*/
*m++=tape_remaining;
/* printf("tape_remaining=0x%x\n", tape_remaining);*/
*m++=data_error_counter;
/* printf("data_error_counter=0x%x\n", data_error_counter);*/
*m++=(byte_2<<24)+(byte_19<<16)+(byte_20<<8)+byte_21;
/* printf("byte_2..21=0x%x\n", (byte_2<<24)+(byte_19<<16)+(byte_20<<8)+byte_21);*/
*m++=(add_sense_code<<8)+add_sense_code_qualifier;
/* printf("sense_code=0x%x\n", (add_sense_code<<8)+add_sense_code_qualifier);*/

{
scsi_position_data position_data;
if (scsi_read_position(device, &position_data)<0)
  {
  /*printf("status_EXABYTE: scsi_read_position failed.\n");*/
  *help=m-help-1;
  return m-help;
  }
*m++=position_data.bop;
*m++=position_data.eop;
*m++=position_data.fbl;
}

*help=m-help-1;
/* printf("*help=0x%x\n", *help);*/
return m-help;
}
/*****************************************************************************/
/*****************************************************************************/
