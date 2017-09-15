/*
 * lowlevel/scsicompat/scsi_interpret_error.c
 * created 24.03.1999 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: scsi_interpret_error.c,v 1.10 2011/04/06 20:30:27 wuestner Exp $";

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <rcs_ids.h>
#include "scsicompat.h"
#include "../../main/nowstr.h"

RCS_REGISTER(cvsid, "lowlevel/scsicompat")

/*****************************************************************************/
typedef struct
  {
  int code; char* s;
  } scsi_command;
scsi_command scsi_commandnames[]=
  {
  {0x00, "TEST_UNIT_READY"},
  {0x01, "REWIND"},
  {0x03, "REQUEST_SENSE"},
  {0x04, "FORMAT"},
  {0x05, "READ_BLOCK_LIMITS"},
  {0x07, "INITIALIZE_ELEMENT_STATUS"},
  {0x08, "READ"},
  {0x0A, "WRITE"},
  {0x0B, "SEEK_6"},
  {0x0F, "READ_REVERSE"},
  {0x10, "WRITE_FILEMARKS"},
  {0x11, "SPACE"},
  {0x12, "INQUIRY"},
  {0x13, "VERIFY"},
  {0x14, "RECOVER_BUFFERED_DATA"},
  {0x15, "MODE_SELECT"},
  {0x16, "RESERVE_UNIT"},
  {0x17, "RELEASE_UNIT"},
  {0x18, "COPY"},
  {0x19, "ERASE"},
  {0x1A, "MODE_SENSE"},
  {0x1B, "LOAD_UNLOAD"},
  {0x1C, "RECEIVE_DIAGNOSTIC"},
  {0x1D, "SEND_DIAGNOSTIC"},
  {0x1E, "PREVENT_ALLOW_REMOVAL"},
  {0x24, "SET_WINDOW"},
  {0x25, "READ_CAPACITY"},
  {0x28, "READ_10"},
  {0x29, "READ_GENERATION"},
  {0x2A, "WRITE_10"},
  {0x2B, "LOCATE"},
  {0x2C, "ERASE_10"},
  {0x2D, "READ_UPDATED_BLOCK"},
  {0x2E, "WRITE_VERIFY"},
  {0x2F, "VERIFY"},
  {0x30, "SEARCH_DATA_HIGH"},
  {0x31, "OBJECT_POSITION"},
  {0x32, "SEARCH_DATA_LOW"},
  {0x33, "SET_LIMITS"},
  {0x34, "READ_POSITION"},
  {0x35, "SYNCHRONIZE_CACHE"},
  {0x36, "LOCK_UNLOCK_CACHE"},
  {0x37, "READ_DEFECT_DATA"},
  {0x38, "MEDIUM_SCAN"},
  {0x39, "COMPARE"},
  {0x3A, "COPY_VERIFY"},
  {0x3B, "WRITE_BUFFER"},
  {0x3C, "READ_BUFFER"},
  {0x3D, "UPDATE_BLOCK"},
  {0x3E, "READ_LONG"},
  {0x3F, "WRITE_LONG"},
  {0x40, "CHANGE_DEFINITION"},
  {0x41, "WRITE_SAME"},
  {0x42, "READ_SUBCHANNEL"},
  {0x43, "READ_TOC"},
  {0x44, "READ_HEADER"},
  {0x45, "PLAY_AUDIO_10"},
  {0x47, "PLAY_AUDIO_MSF"},
  {0x48, "PLAY_AUDIO_TRACK_INDEX"},
  {0x49, "PLAY_TRACK_RELATIVE_10"},
  {0x4B, "PAUSE_RESUME"},
  {0x4C, "LOG_SELECT"},
  {0x4D, "LOG_SENSE"},
  {0x55, "MODE_SELECT_10"},
  {0x5A, "MODE_SENSE_10"},
  {0xA5, "MOVE_MEDIUM"},
  {0xA6, "EXCHANGE_MEDIUM"},
  {0xA8, "GET_MESSAGE_12"},
  {0xA9, "PLAY_TRACK_RELATIVE_12"},
  {0xAA, "WRITE_12"},
  {0xAC, "ERASE_12"},
  {0xAE, "WRITE_VERIFY_12"},
  {0xAF, "VERIFY_12"},
  {0xB0, "SEARCH_DATA_HIGH_12"},
  {0xB1, "SEARCH_DATA_EQUAL_12"},
  {0xB2, "SEARCH_DATA_LOW_12"},
  {0xB3, "SET_LIMITS_12"},
  {0xB5, "REQUEST_VOLUME_ELEMENT_ADDRESS"},
  {0xB6, "SEND_VOLUME_TAG"},
  {0xB7, "READ_DEFECT_DATA_12"},
  {0xB8, "READ_ELEMENT_STATUS"},
  {0xC0, "SET_ADDRESS_FORMAT"},
  {0xC4, "PLAYBACK_STATUS"},
  {0xC6, "PLAY_TRACK"},
  {0xC7, "PLAY_MSF"},
  {0xC8, "PLAY_VAUDIO"},
  {0xC9, "PLAYBACK_CONTROL"}
  };
/*****************************************************************************/
char* sensen[]=
  {
  /* 0 */ "no sense",
  /* 1 */ "recovered error",
  /* 2 */ "not ready",
  /* 3 */ "medium error",
  /* 4 */ "hardware error",
  /* 5 */ "illegal request",
  /* 6 */ "unit attention",
  /* 7 */ "data protect",
  /* 8 */ "blank check",
  /* 9 */ "vendor specific",
  /* a */ "copy aborted",
  /* b */ "aborted command",
  /* c */ "equal",
  /* d */ "volume overflow",
  /* e */ "miscompare"
  /* e */ "reserved"
  };
/*****************************************************************************/
typedef struct
  {
  int asc; int ascq; char* s;
  } asc_code;
asc_code asc_codes[]=
  {
  0, 0, "no additional information",
  0, 1, "unexpected filemark encountered",
  0, 2, "end of tape or partition encountered",
  0, 3, "setmark encountered",
  0, 4, "begin of tape encountered",
  0, 5, "end of data on read operation",
  3, 2, "excessive write errors",
  4, 0, "cause unknown",
  4, 1, "in progress (rewinding or loading)",
  4, 2, "tape motion command (load command) required",
  4, 3, "manual intervention needed",
  8, 1, "logical unit communication timeout",
  8, 2, "logical unit communication parity error",
  9, 0, "tracking error",
  0xa, 0, "error log overflow",
  0xc, 0, "hard write error",
  0x10, 0, "compression integrity check failed",
  0x11, 0, "uncorrectable block",
  0x11, 0, "hardware error during read",
  0x11, 2, "read decompression failed",
  0x11, 1, "hard read error",
  0x11, 3, "too many read errors",
  0x11, 8, "incomplete block read",
  0x14, 0, "medium error or record entity not found",
  0x15, 0, "no information at this position, cannot space",
  0x15, 1, "servo hardware failure, random mechanical positioning error",
  0x15, 2, "positioning error",
  0x1a, 0, "parameter list error in CDB",
  0x20, 0, "illegal operation code",
  0x21, 0, "logical block out of range",
  0x21, 1, "invalid element address",
  0x24, 0, "invalid field in CDB",
  0x24, 0x81, "invalid mode on write buffer",
  0x24, 0x82, "media in drive",
  0x24, 0x84, "insufficient resources",
  0x24, 0x86, "invalid offset",
  0x24, 0x87, "invalid size",
  0x24, 0x89, "image data over limit",
  0x24, 0x8b, "image/personality is bad",
  0x24, 0x8c, "not immediate command",
  0x24, 0x8d, "bad drive/server image EDC",
  0x24, 0x8e, "invalid personality for code update",
  0x24, 0x8f, "bad controller image EDC",
  0x25, 0, "logical unit not supported",
  0x26, 0, "invalid field in parameter list",
  0x26, 1, "parameter not supported",
  0x26, 2, "write buffer parameter value invalid",
  0x27, 0, "write protected",
  0x27, 0, "tape is write protected",
  0x27, 0x80, "hardware write protect",
  0x27, 0x82, "data safety write protect",
  0x28, 0, "tape load has occured, media may have been changed",
  0x29, 0, "power-on reset, SCSI bus reset or device reset",
  0x2a, 1, "mode select parameters have been changed",
  0x2a, 2, "log parameter changed",
  0x30, 0, "incompatible medium installed",
  0x30, 1, "tape format incompatible",
  0x31, 0, "tape format error",
  0x31, 1, "medium format corrupted",
  0x37, 0, "rounded parameter",
  0x39, 0, "saving parameters not supported",
  0x3a, 0, "tape not present",
  0x3b, 0, "position error",
  0x3b, 2, "tape position error at end of medium",
  0x3b, 8, "repositioning error",
  0x3b, 0xd, "media destination element full",
  0x3b, 0xe, "media source element empty",
  0x3d, 0, "illegal bit set in identyfy message",
  0x3f, 1, "new microcode was loaded",
  0x40, 0x80, "controller hardware failure",
  0x40, 0x81, "ram failure",
  0x40, 0x82, "bad drive status",
  0x40, 0x83, "loader diags failure",
  0x43, 0, "message error",
  0x44, 0, "Internal failure",
  0x44, 0xc1, "internal target failure",
  0x44, 0xc2, "internal target failure",
  0x44, 0xc3, "internal target failure",
  0x44, 0x80, "unexpected selection interrupt",
  0x44, 0x82, "command complete sequence failure",
  0x44, 0x83, "SCSI chip gross error",
  0x44, 0x84, "unexplained residue in TC registers",
  0x44, 0x85, "immediate data transfer timeout",
  0x44, 0x86, "insufficient CDB bytes",
  0x44, 0x87, "disconnect/SDP sequence failed",
  0x44, 0x88, "bus DMA transfer timeout",
  0x45, 0, "reselect failed",
  0x47, 0, "SCSI parity error",
  0x48, 0, "initiator detected error message received",
  0x49, 0, "invalid message from initiator",
  0x4e, 0, "overlapped commands attempted",
  0x50, 1, "write append position error",
  0x51, 0, "erase failure",
  0x53, 0, "media load/eject failure",
  0x53, 1, "unload tape failure",
  0x53, 2, "media removal prevented",
  0x5a, 1, "eject button pressed",
  0x5b, 1, "log threshold met",
  0x5b, 2, "log counter overflow",
  0x80, 2, "cleaning requested",
  0x81, 0, "mode mismatch",
  0x82, 0, "command requires no tape, but tape is present",
  0x84, 0, "??? compression mode mismatch ???",
  -1, -1, (char*)0
  };
/*****************************************************************************/
static char* asc_text(int asc, int ascq)
{
int i=0;
do
  {
  if ((asc_codes[i].asc==asc) && (asc_codes[i].ascq==ascq))
      return asc_codes[i].s;
  i++;
  }
while (asc_codes[i].asc!=-1);
return "???";
}
/*****************************************************************************/
void scsi_interpret_error(raw_scsi_var* var)
{
int error_code         =var->scsi_error.error_code;
int sense_key          =var->scsi_error.sense_key;
int add_sense_code     =var->scsi_error.add_sense_code;
int add_sense_code_qual=var->scsi_error.add_sense_code_qual;
int code_data          =var->scsi_error.code_data;
int field_ptr          =var->scsi_error.field_ptr;
int valid              =var->scsi_error.valid;
int infobytes          =var->scsi_error.infobytes;
int bpv                =var->scsi_error.bpv;
int bitptr             =var->scsi_error.bitptr;
if (error_code==0x70)
  printf("current error (0x70)\n");
else
  printf("deferred error (0x%x)\n", error_code);
printf("  sense key  : %3d (0x%02x)", sense_key, sense_key);
if (sense_key<=sizeof(sensen)/sizeof(char*))
  printf(" (%s)", sensen[sense_key]);
printf("\n");
printf("  asc        : %3d (0x%02x)", add_sense_code, add_sense_code);
printf(" (%s)\n", asc_text(add_sense_code, add_sense_code_qual));
printf("  ascq       : %3d (0x%02x)\n",
    add_sense_code_qual, add_sense_code_qual);
printf("  c/d        : error in %s\n", code_data?"code":"data");
printf("  field ptr  : %3d (0x%02x)\n", field_ptr, field_ptr);
printf("info bytes %s\n", valid?"valid":"invalid");
if (valid)
    printf("  info bytes: %3d (0x%02x)\n", infobytes, infobytes);
printf("bit pointer %s\n", bpv?"valid":"invalid");
if (bpv)
    printf("  bit pointer: %3d (0x%02x)\n", bitptr, bitptr);
printf("  filemark=%x, eom=%x\n", var->scsi_error.filemark,
    var->scsi_error.eom);
}
/*****************************************************************************/
static char* get_scsi_commandname(int code)
{
int i;
static char s[1024];
for (i=0; i<sizeof(scsi_commandnames)/sizeof(scsi_command); i++)
  {
  if (scsi_commandnames[i].code==code) return scsi_commandnames[i].s;
  }
snprintf(s, 1024, "command 0x%02X", code);
return s;
}
/*****************************************************************************/
void scsi_decode_error(raw_scsi_var* var, int direction,
    unsigned char *cdb, int cdblen,
    unsigned char *sense, int senselen,
    unsigned char *data, int datalen)
{
int statuscode;

printf("%s SCSI error in %s; ", nowstr(), get_scsi_commandname(cdb[0]));
switch (direction)
  {
  case SCSICOMPAT_DIR_IN: printf("DIR_IN"); break;
  case SCSICOMPAT_DIR_OUT: printf("DIR_OUT"); break;
  case SCSICOMPAT_DIR_NONE: printf("DIR_NONE"); break;
  default: printf("unknown direction %d", direction);
  }
printf("\ncommand:\n");
scsi_dump_buffer(cdb, cdblen);
if (datalen==0)
  printf("no data\n");
else
  {
  printf("data: (len=%d)\n", datalen);
  scsi_dump_buffer(data, datalen);
  }

printf("sense buffer (len=%d)\n", senselen);
scsi_dump_buffer(sense, senselen);

scsireq_buff_decode(sense, senselen,
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

scsi_interpret_error(var);
fflush(stdout);
}
/*****************************************************************************/
/*****************************************************************************/
