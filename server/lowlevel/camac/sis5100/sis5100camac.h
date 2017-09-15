/*
 * lowlevel/camac/sis5100/sis5100camac.h
 *
 * $ZEL: sis5100camac.h,v 1.8 2007/08/20 18:03:45 wuestner Exp $
 *
 * created: 04.Apr.2004 PW
 */

#ifndef _sis5100camac_h_
#define _sis5100camac_h_

#include <sconf.h>
#include <sys/types.h>
#include <emsctypes.h>
#include "../../../objects/domain/dom_ml.h"
#include "../../../main/scheduler.h"
#include "sis5100camaddr.h"

/*
struct camac_irq_callback_5100 {
    struct camac_irq_callback_3100* next;
    struct camac_irq_callback_3100* prev;
    ems_u32 mask;
    camacirq_callback callback;
};
*/

struct camac_sis5100_info {
    int camac_path;
    int ram_path;
    int ctrl_path;
    int dsp_path;
    /*off_t mem_size;*/ /* size of sis5100-RAM*/
    /*off_t mem_free;*/ /* first free byte in RAM */
    ems_u32 camacstatus;
#ifdef DELAYED_READ
    /*struct delayed_hunk* hunk_list;*/
    /*int hunk_list_size;*/
    /*int num_hunks;*/
#endif
#ifdef SIS5100_MMAPPED
    volatile ems_u32* camac_base;
    volatile struct sis1100_reg* ctrl_base;
    volatile struct sis5100_reg* ctrl_base_rem;
    /*volatile ems_u32* mem_base;*/
    size_t mem_size;
    /*volatile ems_u32* dsp_base;*/
    size_t dsp_size;
    /*volatile ems_u32* pipe_base;*/
    size_t pipe_size;
    /*volatile ems_u32* sr;*/
    /*volatile ems_u32* p_balance;*/
    /*volatile ems_u32* prot_error;*/
    /*         off_t    pipe_offset;*/
    /*         ems_u32  pipe_dma_addr;*/
#endif
    /*int (*load_list)(struct camac_dev* dev, int addr, int size,
            struct camaccommand*);*/
    /*struct dsp_procs* (*get_dsp_procs)(struct camac_dev*);*/
    /*struct mem_procs* (*get_mem_procs)(struct camac_dev*);*/
    /*ems_u32 lammask;*/
    struct seltaskdescr* st;
    /*struct camac_irq_callback_5100* irq_callbacks;*/
};

int sis5100_open_devices(struct camac_sis5100_info* info, const char* stub);

#endif
