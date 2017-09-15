/*
 * lowlevel/scsicompat/linux/scsicompat_linux.h
 * 
 * created 08.12.2000 PW
 */
/*$ZEL: scsicompat_linux.h,v 1.3 2002/09/28 17:41:04 wuestner Exp $*/

#ifndef _scsicompat_linux_h_
#define _scsicompat_linux_h_

typedef struct
  {
  int path;
  int bus, target, lun, density;
  } raw_scsi_info;

#define SCSICOMPAT_DIR_IN 1
#define SCSICOMPAT_DIR_OUT 2
#define SCSICOMPAT_DIR_NONE 3

#endif
