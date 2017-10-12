/*
 * lowlevel/lvd/sis1100/sis1100_lvd.h
 * created 10.Dec.2003 PW
 * $ZEL: sis1100_lvd.h,v 1.6 2006/02/12 22:48:21 wuestner Exp $
 */

#ifndef _sis1100_lvd_h_
#define _sis1100_lvd_h_

#include <sconf.h>
#include "../../../main/scheduler.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <debug.h>
#include <errorcodes.h>
#include <emsctypes.h>
#include "../lvd.h"

struct lvd_irq_callback {
    struct lvd_irq_callback* next;
    struct lvd_irq_callback* prev;
    ems_u32 mask;
    lvdirq_callback callback;
};

struct rm /* read_multi*/ {
    ems_u32* incomplete_buf;  /* used to save an incomplete event */
    size_t   incomplete_size; /* size of incomplete_buf */
    int      incomplete_num;  /* actual content of incomplete_buf */
    int      last_evnr;       /* to check consistency */
};
struct ra /* read_async*/ {
    ems_u32* _demand_dma_buf; /* real dma_buf (for 'free') */
    ems_u32* demand_dma_buf;  /* page aligned dma_buf */
    size_t demand_dma_size;   /* size of each part of dma_buf (in byte) */
    int demand_dma_num;       /* number of dma_buf parts */
    int demand_dma_lastblock; /* last block received */
#if 0
    int demand_dma_lastevent;
    int demand_dma_ofs;
#endif
    int demand_dma_path; /* XXX only for RSS test! should be removed */
};

struct lvd_sis1100_info {
    char* ctrl_name;
    char* remote_name;
    int p_ctrl;
    int p_remote;
    int majorversion;
    int minorversion;

/*
    struct delayed_hunk* hunk_list;
    int hunk_list_size;
    int num_hunks;
    int delayed_read_enabled;
*/

    struct seltaskdescr* st;
    struct lvd_irq_callback* irq_callbacks;
    struct rm rm; /* state of read_multi */
    struct ra ra; /* state of read_async */

    int (*plxwrite)(struct lvd_dev* dev, ems_u32 reg, ems_u32 val, int verb);
    int (*plxread)(struct lvd_dev* dev, ems_u32 reg, ems_u32* val, int verb);
    int (*siswrite)(struct lvd_dev* dev, ems_u32 reg, ems_u32 val, int verb);
    int (*sisread)(struct lvd_dev* dev, ems_u32 reg, ems_u32* val, int verb);
    int (*mcwrite)(struct lvd_dev* dev, ems_u32 reg, ems_u32 val, int verb);
    int (*mcread)(struct lvd_dev* dev, ems_u32 reg, ems_u32* val, int verb);

    int (*sis_stat)(struct lvd_dev* dev);
};

errcode lvd_init_sis1100(char* pathname, struct lvd_dev* dev);

/*
int lvd_sis1100_write(int p, u_int32_t addr, int size, ems_u32 data, int trace);
int lvd_sis1100_read(int p, u_int32_t addr, int size, u_int32_t* data, int trace);
*/
void dump_shadow(void);
void lvd_decode_addr(struct lvd_dev* dev, int addr, int size);
int lvd_decode_check_integrity(void);
int sis1100_lvd_mcstat(struct lvd_dev* dev);


#endif
