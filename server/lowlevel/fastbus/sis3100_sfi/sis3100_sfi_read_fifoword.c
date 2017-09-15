/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_read_fifoword.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_read_fifoword.c,v 1.4 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

int sis3100_sfi_read_fifoword(struct fastbus_dev* dev, ems_u32* data,
        ems_u32* ss)
{
    ems_u32 flags;
    int res;

SIS3100_CHECK_BALANCE(dev, "read_fifoword_0");
    if ((res=sis3100_sfi_wait_sequencer(dev, ss))==0) {
SIS3100_CHECK_BALANCE(dev, "read_fifoword_1");
        if (SFI_R(dev, fifo_flags, &flags), flags&0x10) { /* FIFO empty */
            volatile ems_u32 d;
            SFI_R(dev, read_seq2vme_fifo, &d); /* dummy read */
            (void)d;
            SFI_R(dev, fifo_flags, &flags);
            if (flags&0x10) { /* FIFO empty */
                printf("sis3100_sfi_read_fifoword: FIFO empty\n");
                return -1;
            }
        }
        SFI_R(dev, read_seq2vme_fifo, data);
    }
    return res;
}
/*****************************************************************************/
/*****************************************************************************/
