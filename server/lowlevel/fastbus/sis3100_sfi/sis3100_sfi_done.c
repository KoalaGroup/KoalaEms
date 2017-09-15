/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_done.c
 * created: ?
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_done.c,v 1.8 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include "sis3100_sfi.h"
#include <sys/mman.h>
#include <rcs_ids.h>

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

errcode
sis3100_sfi_done(struct generic_dev* gdev)
{
    struct fastbus_dev* dev=(struct fastbus_dev*)gdev;
    struct fastbus_sis3100_sfi_info* info=(struct fastbus_sis3100_sfi_info*)dev->info;
#ifdef SIS3100_SFI_MMAPPED
    struct sis1100_dma_alloc dma_alloc;
#endif

    if (info->st) {
        sched_delete_select_task(info->st);
    }

#ifdef DELAYED_READ
    free(info->hunk_list);
    info->hunk_list=0;
    info->hunk_list_size=0;
    info->num_hunks=0;
    free(sis3100_sfi_delay_buffer);
    sis3100_sfi_delay_buffer=0;
    sis3100_sfi_delay_buffer_size=0;
#endif
#ifdef SIS3100_SFI_MMAPPED
    munmap((void*)info->ctrl_base, info->ctrl_size);
    munmap((void*)info->vme_base, SFI_MAPSIZE);
    munmap((void*)info->pipe_base, info->pipe_size);
    dma_alloc.size    =info->pipe_size;
    dma_alloc.offset  =info->pipe_offset;
    dma_alloc.dma_addr=info->pipe_dma_addr;
    ioctl(info->ctrl_path, SIS1100_DMA_FREE, &dma_alloc);
#endif

printf("sfi_done: close %d\n", info->p_ctrl);
    close(info->p_ctrl);
printf("sfi_done: close %d\n", info->p_vme);
    close(info->p_vme);
printf("sfi_done: close %d\n", info->p_mem);
    close(info->p_mem);
printf("sfi_done: close %d\n", info->p_dsp);
    close(info->p_dsp);

    free(info);
    dev->info=0;
    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
