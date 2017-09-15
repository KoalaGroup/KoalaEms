/*
 * lowlevel/unixvme/sis3100/vme_sis.h
 * $ZEL: vme_sis.h,v 1.15 2007/10/15 14:33:50 wuestner Exp $
 * created 23.Jan.2001 PW
 * 07.Sep.2002 PW: current_am added
 */

#ifndef _vme_sis_h_
#define _vme_sis_h_

#include <sconf.h>
#include <sys/types.h>
#include "../../../main/scheduler.h"

#ifdef DELAYED_READ
struct delayed_hunk {
    off_t src;
    ems_u8* dst;
    size_t size;
};
#endif

struct vme_irq_callback_3100 {
    struct vme_irq_callback_3100* next;
    struct vme_irq_callback_3100* prev;
    ems_u32 mask;
    ems_u32 vector;
    vmeirq_callback callback;
    void *data;
};

struct vme_sis_info {
    int p_ctrl;
    int p_vme;
    int p_mem;
    int p_dsp;
    int current_datasize;
    int current_am;
#ifdef DELAYED_READ
    struct delayed_hunk* hunk_list;
    int hunk_list_size;
    int num_hunks;
    ems_u8* delay_buffer;
    size_t delay_buffer_size;
#endif
    u_int32_t bufferstart;
    u_int32_t bufferaddr;
    struct seltaskdescr* st;
    struct vme_irq_callback_3100* irq_callbacks;
};

errcode vme_init_sis3100(struct vme_dev*);

#ifdef DELAYED_READ
int sis3100_vme_delay_read(struct vme_dev*, off_t, void*, size_t);
#endif

#endif
