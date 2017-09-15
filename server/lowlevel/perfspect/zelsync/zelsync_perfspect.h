/*
 * lowlevel/perfspect/zelsync/zelsync_perfspect.h
 * 
 * $ZEL: zelsync_perfspect.h,v 1.4 2007/04/19 22:17:29 wuestner Exp $
 */

#ifndef zelsync_perfspect_h_
#define zelsync_perfspect_h_

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>

errcode perfspect_init_pcisync(struct perf_dev* dev);

struct perfspect_pcisync_info {
    int sync_id;
#ifdef DELAYED_READ
    /*struct delayed_hunk* hunk_list;*/
    /*int hunk_list_size;*/
    /*int num_hunks;*/
#endif
};
/*
int perfspect_add_current(int id);
int perfspect_get_current(void);
void perfspect_add(int id, int tics);
void perfspect_notify_event(ems_u32 eventcnt);
*/
#endif
