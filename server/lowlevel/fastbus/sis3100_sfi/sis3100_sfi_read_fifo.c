/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_read_fifo.c
 * 
 * created Oct.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_read_fifo.c,v 1.4 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

int sis3100_sfi_read_fifo(struct fastbus_dev* dev, ems_u32* dest,
        int max, ems_u32* ss)
{
    int res;

    if ((res=sis3100_sfi_wait_sequencer(dev, ss))==0) {
        ems_u32 flags;
        int num=0;

        if (SFI_R(dev, fifo_flags, &flags), flags&0x10) {
            volatile ems_u32 d;
            SFI_R(dev, read_seq2vme_fifo, &d);
            (void) d;
            SFI_R(dev, fifo_flags, &flags);
        }
        while (((flags&0x10)==0) && (num<max)) {
            SFI_R(dev, read_seq2vme_fifo, &dest[num]);
            num++;
            SFI_R(dev, fifo_flags, &flags);
        }
        return num;
    } else
        return res;
}
/*****************************************************************************/
/*****************************************************************************/
