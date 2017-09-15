#ifndef _scsicompat_bsd_h_
#define _scsicompat_bsd_h_

#include <sys/scsiio.h>

typedef struct
  {
  int path;
  int lun;
  } raw_scsi_info;

#endif

#define SCSICOMPAT_DIR_IN SCCMD_READ
#define SCSICOMPAT_DIR_OUT SCCMD_WRITE
#define SCSICOMPAT_DIR_NONE 0
