/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_read_fifo_num.c
 * 
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_read_fifo_num.c,v 1.4 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

#ifdef SIS3100_SFI_MMAPPED
int sis3100_sfi_read_fifo_num(struct fastbus_dev* dev, ems_u32* dest,
        int num, ems_u32* ss)
{
    struct fastbus_sis3100_sfi_info* info=&dev->info.sis3100_sfi;
    int res, i, num_=num;
    ems_u32 flags;
    ems_u32 fifo_addr, flag_addr;
    volatile ems_u32* buf=info->pipe_base;

    if ((num+2)*sizeof(ems_u32)>info->pipe_size) {
        printf("sis3100_sfi_read_fifo_num(%d): buffer too small.\n", num);
    }

    if ((res=sis3100_sfi_wait_sequencer(dev, ss))) return res;

    /* if fifo empty the first word to be read is a dummy */
    SFI_R(dev, fifo_flags, &flags);
    if (flags&0x10) {num_++; buf++;}

    /* read num_-1 words, fifoflags and the last word */
    fifo_addr=(ems_u32)&((sfi_r)SFI_BASE)->read_seq2vme_fifo;
    flag_addr=(ems_u32)&((sfi_r)SFI_BASE)->fifo_flags;

    CTRL_W(dev, rd_pipe_buf, info->pipe_dma_addr);
    CTRL_W(dev, rd_pipe_blen, info->pipe_size);
    CTRL_W(dev, t_hdr, 0x0f410802);
    CTRL_W(dev, t_am, 0x39);
    for (i=num_-1; i; i--) 
        CTRL_W(dev, t_adl, fifo_addr);
    CTRL_W(dev, t_adl, flag_addr);
    CTRL_W(dev, t_adl, fifo_addr);

    /* wait for end of transfer */
    while (*info->p_balance);

    /* copy num-1 words to dest */
    for (i=num-1; i; i--) {
        *dest++=*buf++;
    }
    if (*buf++&0x10) { /* fifo was empty before the last word */
        printf("sis3100_sfi_read_fifo_num(%d): fifo empty\n", num);
        return -1;
    }
    /* copy the last word */
    *dest=*buf;

    return num;
}
#else
int sis3100_sfi_read_fifo_num(struct fastbus_dev* dev, ems_u32* dest,
        int num, ems_u32* ss)
{
    int res, i;
    ems_u32 flags;

    if ((res=sis3100_sfi_wait_sequencer(dev, ss))) return res;

    /* if fifo empty read dummy word */
    SFI_R(dev, fifo_flags, &flags);
    if (flags&0x10) SFI_R(dev, read_seq2vme_fifo, dest);

    /* read num-1 words */
    for (i=0; i<num-1; i++) SFI_R(dev, read_seq2vme_fifo, dest++);

    /* check if fifo is still not empty and read the last word */
    SFI_R(dev, fifo_flags, &flags);
    if (flags&0x10) {
        printf("sis3100_sfi_read_fifo_num(%d): fifo empty\n", num);
        return -1;
    }
    SFI_R(dev, read_seq2vme_fifo, dest);
    /* we do _not_ check if further words are available */
    return num;
}
#endif
/*****************************************************************************/
