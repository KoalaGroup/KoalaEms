#ifndef _scsicompat_osf_h_
#define _scsicompat_osf_h_

#include <sys/types.h>
#include <sys/ioctl.h>

#include <sys/devgetinfo.h>
#include <io/common/iotypes.h>
#include <io/common/devdriver.h>
#include <io/cam/cam.h>

typedef struct
  {
  int path;
  int campath;
  int bus, target, lun, density;
  } raw_scsi_info;

#define SCSICOMPAT_DIR_IN CAM_DIR_IN
#define SCSICOMPAT_DIR_OUT CAM_DIR_OUT
#define SCSICOMPAT_DIR_NONE CAM_DIR_NONE

#endif
