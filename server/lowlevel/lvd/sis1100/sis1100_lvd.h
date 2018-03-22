/*
 * lowlevel/lvd/sis1100/sis1100_lvd.h
 * created 10.Dec.2003 PW
 * $ZEL: sis1100_lvd.h,v 1.23 2017/10/20 23:21:31 wuestner Exp $
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
#include "../lvdbus.h"
#include "dev/pci/sis1100_var.h"

#if 0
#define NEXTBLOCK(block, num) \
    (((block)+1)==(num)?0:(block)+1)
#endif
#define NEXTBLOCK(block) \
    (((block)+1)==(info->ra.dma_num)?0:(block)+1)

struct lvd_irq_callback {
    struct lvd_irq_callback* next;
    struct lvd_irq_callback* prev;
    ems_u32 mask;
    lvdirq_callback callback;
    void *data;
    char *procname;
};
struct ra_statist {
    ems_u64 blocks;
    ems_u64 events;
    ems_u64 fragments;
    ems_u64 words;
    int max_evsize;
};
struct fragmentcollector {
    int hsize;      /* length part of header */
    int num;        /* actual number of words in *data */
    ems_u32 *data;
};
struct ra /* read_async*/ {
    ems_u32* _dma_buf; /* real dma_buf (for 'free') */
    ems_u32*  dma_buf; /* page aligned dma_buf */
    unsigned int dma_size;      /* size of each part of dma_buf (in 32-bit-words) */
    int dma_num;       /* number of dma_buf parts */
    int dma_oldblock;  /* last block received */
    int dma_newblock;  /* block announced by IRQ procedure */
    int last_released_block; /* only for sanity check */
    int activeblock;   /* only for sanity check */

    /* partially received event */
#if 0
    struct fragmentcollector fragment;
#endif
    struct fragmentcollector event;

    /* following items are for statistics only */
    struct ra_statist statist;
    struct ra_statist old_statist;
    struct timeval tv0;
};

struct lvd_sis1100_link {
    struct lvd_sis1100_link *next;
    struct lvd_sis1100_link *prev;
    struct lvd_dev* dev;
    char *pathname;
    int p_ctrl;
    int p_remote;
#ifdef LVD_SIS1100_MMAPPED
    struct sis1100_reg *pci_ctrl_base;
    struct lvd_reg     *lvd_ctrl_base;
    struct lvd_bus1100 *remote_base;
    size_t ctrl_size;
    size_t remote_size;
#endif
    struct seltaskdescr* st;
    struct sis1100_ident ident;
    ems_u32 controller_id;
    ems_u32 controller_serial;
    int speed;    /* 0: 1GBit/s 1: 2GBit/s */
    int online;   /* link is working, regardless of remote type */
    int priority; /* lower link: 0, upper link: 1; the link with the highest */
                  /*   priority should be used for readout */
};

struct lvd_sis1100_info {
    struct lvd_sis1100_link *A;
    struct lvd_sis1100_link *B;

    struct lvd_irq_callback* irq_callbacks;
    ems_u32 irq_mask;
    ems_u32 autoack_mask;
    struct ra ra; /* state of read_async */
    int ddma_active;

    plerrcode (*plxwrite)(struct lvd_dev*, int link, ems_u32 reg, ems_u32 val);
    plerrcode (*plxread)(struct lvd_dev*, int link, ems_u32 reg, ems_u32* val);
    plerrcode (*siswrite)(struct lvd_dev*, int link, ems_u32 reg, ems_u32 val);
    plerrcode (*sisread)(struct lvd_dev*, int link, ems_u32 reg, ems_u32* val);
    plerrcode (*mcwrite)(struct lvd_dev*, int link, ems_u32 reg, ems_u32 val);
    plerrcode (*mcread)(struct lvd_dev*, int link, ems_u32 reg, ems_u32* val);

    plerrcode (*sis_stat)(struct lvd_dev*, int link, void*, int level);
    plerrcode (*mc_stat)(struct lvd_dev*, int link, void*, int level);
    plerrcode (*plx_stat)(struct lvd_dev*, int link, void*, int level);
    plerrcode (*link_stat)(struct lvd_dev*, void*);
#if 0
    plerrcode (*ddma_stat)(struct lvd_dev*, void*, int level);
    plerrcode (*ddma_flush)(struct lvd_dev*, void*);
#endif
};

errcode lvd_init_sis1100(struct lvd_dev* dev);
errcode lvd_init_sis1100X(struct lvd_dev* dev);
void init_ra_statist(struct ra_statist *ra_st);
int sis1100_lvd_statist(struct lvd_dev* dev, ems_u32 flags, ems_u32 *ptr);
void sis1100_lvd_print_statist(struct lvd_dev* dev, struct timeval tv1);

void dump_shadow(void);
void lvd_decode_addr(struct lvd_dev* dev, int addr, int size);
int lvd_decode_check_integrity(void);

void sis1100_init(void);
void sis1100_done(void);

#endif
