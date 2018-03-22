/*
 * lowlevel/unixvme/sis3153_usb/vme_sis_usb.h
 * $ZEL: vme_sis_usb.h,v 1.1 2016/02/29 17:33:10 wuestner Exp $
 * created 2016-01-21 PeWue
 */

#ifndef _vme_sis_usb_h_
#define _vme_sis_usb_h_

#include <sconf.h>
#include <sys/types.h>
#include "../../../main/scheduler.h"

struct vme_sis_usb_info {
    //struct sis1100_ident ident;
    int current_datasize;
    int current_am;
    u_int32_t bufferstart;
    u_int32_t bufferaddr;
    struct seltaskdescr* st;
};

errcode vme_init_sis3153_usb(struct vme_dev*);

#endif
