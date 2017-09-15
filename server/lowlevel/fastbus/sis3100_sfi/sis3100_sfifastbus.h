/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfifastbus.h
 * created 22.May.2001 PW
 * $ZEL: sis3100_sfifastbus.h,v 1.15 2008/06/28 21:47:53 wuestner Exp $
 */

#ifndef _fastbus_sis3100_sfi_h_
#define _fastbus_sis3100_sfi_h_

#include <sconf.h>
#include "../../../main/scheduler.h"
#include <debug.h>
#include <sys/types.h>
#include <emsctypes.h>
#include "dspprocs.h"

struct sficommand{
  u_int32_t cmd;
  u_int32_t data;
};

struct fastbus_sis3100_sfi_info {
    int p_ctrl;
    int p_vme;
    int p_mem;
    int p_dsp;
    off_t ram_size; /* size of sis3100-RAM*/
    off_t ram_free; /* first free byte in RAM */
#ifdef DELAYED_READ
    struct delayed_hunk* hunk_list;
    int hunk_list_size;
    int num_hunks;
    ems_u8* delay_buffer;
    size_t delay_buffer_size;
#endif
#ifdef SIS3100_SFI_MMAPPED
    off_t vme_size;
    off_t ctrl_size;
    volatile ems_u32* vme_base;
    volatile ems_u32* ctrl_base;
    volatile ems_u32* mem_base;
    volatile ems_u32* dsp_base;
    volatile ems_u32* pipe_base;
    volatile ems_u32* sr;
    volatile ems_u32* p_balance;
    volatile ems_u32* prot_error;
             size_t   pipe_size;
             off_t    pipe_offset;
             ems_u32  pipe_dma_addr;
#endif
    int (*load_list)(struct fastbus_dev* dev, int addr, int size,
            struct sficommand*);
#ifdef XXX
    struct dsp_procs* (*get_dsp_procs)(struct fastbus_dev*);
    struct mem_procs* (*get_mem_procs)(struct fastbus_dev*);
#endif
    struct seltaskdescr* st;
};

/*
 * This is the only function which is really exported, all other functions are
 * only available via struct fastbus_dev.
 */
errcode fastbus_init_sis3100_sfi(struct fastbus_dev* dev);

#endif
