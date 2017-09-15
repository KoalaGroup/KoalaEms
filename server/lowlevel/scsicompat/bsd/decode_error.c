/*
 * lowlevel/scsicompat/bsd/decode_error_bsd.c
 * 
 * created 10.03.1999 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: decode_error.c,v 1.4 2011/04/06 20:30:28 wuestner Exp $";

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/scsiio.h>
#include <rcs_ids.h>
#include "../scsicompat.h"
#include "scsicompat_bsd_local.h"

RCS_REGISTER(cvsid, "lowlevel/scsicompat/bsd")

int xx_scsi_decode_error(raw_scsi_var* var, char* proc, struct scsireq* req,
  int res)
{
int statuscode; 
printf("%s: SCSI error.\n", proc);
printf("command:\n");
scsi_dump_buffer(req->cmd, req->cmdlen);
if (req->datalen_used==0)
  printf("no data:\n");
else
  {
  printf("data:\n");
  scsi_dump_buffer(req->databuf, req->datalen_used);
  }
if (res<0)
  {
  printf("scsireq_enter returns %d\n", res);
  fflush(stdout);
  return res;
  }
printf("  sense_used=%d\n", req->senselen_used);
scsi_dump_buffer(req->sense, req->senselen_used);
printf("  data_used=%ld\n", req->datalen_used);
scsi_dump_buffer(req->databuf, req->datalen_used);

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
    &var->scsi_error.valid, &var->scsi_error.error_code,
    &var->scsi_error.filemark, &var->scsi_error.eom,
    &var->scsi_error.sense_key, &var->scsi_error.infobytes,
    &var->scsi_error.add_sense_code, &var->scsi_error.add_sense_code_qual,
    &var->scsi_error.code_data, &var->scsi_error.bpv, &var->scsi_error.bitptr,
    &var->scsi_error.field_ptr, &statuscode);
/*
 * printf("internal status %x\n", statuscode);
 * printf("  error code              : %3d (0x%02x)\n",
 *     var->scsi_error.error_code, var->scsi_error.error_code);
 * printf("  sense key               : %3d (0x%02x)\n",
 *     var->scsi_error.sense_key, var->scsi_error.sense_key);
 * printf("  add sense code          : %3d (0x%02x)\n",
 *     var->scsi_error.add_sense_code, var->scsi_error.add_sense_code);
 * printf("  add sense code qualifier: %3d (0x%02x)\n",
 *     var->scsi_error.add_sense_code_qual, var->scsi_error.add_sense_code_qual);
 * printf("  code/data               : %3d\n", var->scsi_error.code_data);
 * printf("  field ptr               : %3d (0x%02x)\n",
 *     var->scsi_error.field_ptr, var->scsi_error.field_ptr);
 * if (var->scsi_error.valid)
 *     printf("  info bytes              : %3d (0x%02x)\n",
 *       var->scsi_error.infobytes, var->scsi_error.infobytes);
 * if (var->scsi_error.bpv)
 *     printf("  bit ptr                 : %3d (0x%02x)\n",
 *       var->scsi_error.bitptr, var->scsi_error.bitptr);
 * printf("  filemark %x, eom %x\n", var->scsi_error.filemark,
 *     var->scsi_error.eom);
 */

interpret_error(var);
fflush(stdout);
return -1;
}
/*****************************************************************************/
/*****************************************************************************/
