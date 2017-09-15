/*
 * lowlevel/unixvme/drv/vme_drv.h
 * created 23.Jan.2001 PW
 * $ZEL: vme_drv.h,v 1.4 2003/07/03 19:10:41 wuestner Exp $
 */

#ifndef _vme_drv_h_
#define _vme_drv_h_

struct vme_drv_info
  {
  int handle;
  int current_datasize;
  };

errcode vme_init_drv(char*, struct vme_dev*);

#endif
