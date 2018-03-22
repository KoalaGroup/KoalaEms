/*
 * lowlevel/unixvme/sis3100_udp/vme_sis_udp.h
 * $ZEL: vme_sis_udp.h,v 1.1 2016/02/29 17:32:29 wuestner Exp $
 * created 2016-01-21 PeWue
 */

#ifndef _vme_sis_udp_h_
#define _vme_sis_udp_h_

#include <sconf.h>
#include <sys/types.h>
#include "../../../main/scheduler.h"

struct vme_sis_udp_info {
    //struct sis1100_ident ident;
    int current_datasize;
    int current_am;
    //u_int32_t bufferstart;
    //u_int32_t bufferaddr;
    //struct seltaskdescr* st;
};

errcode vme_init_sis3100_udp(struct vme_dev*);

#endif
