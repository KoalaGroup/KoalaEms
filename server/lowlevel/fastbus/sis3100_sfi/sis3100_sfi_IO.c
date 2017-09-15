/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_IO.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_IO.c,v 1.5 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

/*****************************************************************************/
int
sis3100_sfi_LEVELIN(struct fastbus_dev* dev, int opt, ems_u32* dest)
{
    switch (opt) {
    case 0: /* input via sfi/ngf */
        SFI_R(dev, fifo_flags, dest);
        *dest=(*dest>>8)&0xff;
        break;
    case 1: /* input via sis3100 */
        *dest=VME_R(dev, in_out, dest);
        *dest=(*dest>>16)&0x7f;
        break;
    /*default: ignored*/
    }
    return 0;
}
/*****************************************************************************/
int
sis3100_sfi_LEVELOUT(struct fastbus_dev* dev, int opt, ems_u32 set, ems_u32 res)
{
    switch (opt) {
    case 0: /* output via sfi/ngf */
        SFI_W(dev, write_vme_out_signal, (set&0xffff)|((res&0xffff)<<16));
        break;
    case 1: /* output via sis3100 */
        VME_W(dev, in_out, (set&0x7f)|((res&0x7f)<<16));
        if ((set|res)&0x80) {
            VME_W(dev, vme_master_sc, (set&0x80)|((res&0x80)<<16));
        }
        break;
    /*default: ignored*/
    }
    return 0;
}
/*****************************************************************************/
int
sis3100_sfi_PULSEOUT(struct fastbus_dev* dev, int opt, ems_u32 mask)
{
    switch (opt) {
    case 0: /* pulse via sfi/ngf */
        SEQ_W(dev, seq_out, mask);
        SEQ_W(dev, seq_out, mask<<16);
        break;
    case 1: /* output via sis3100 */
        VME_W(dev, in_out, mask<<24);
        break;
    /*default: ignored*/
    }
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
