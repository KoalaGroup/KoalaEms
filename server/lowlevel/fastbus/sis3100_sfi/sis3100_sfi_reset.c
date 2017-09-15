/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_reset.h
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_reset.c,v 1.4 2011/04/06 20:30:23 wuestner Exp $";

#include <rcs_ids.h>
#include "sis3100_sfi.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

static volatile ems_u32 mist=0x0815;

int
sis3100_sfi_reset(struct fastbus_dev* dev)
{
    SFI_W(dev, reset_register_group_lca2, mist);
    SFI_W(dev, sequencer_reset, mist);
    SFI_W(dev, sequencer_enable, mist);
    return 0;
}

int
sis3100_restart_sequencer(struct fastbus_dev* dev)
{
    SFI_W(dev, sequencer_reset, mist);
    SFI_W(dev, sequencer_enable, mist);
    return 0;
}

int
sis3100_sfi_status(struct fastbus_dev* dev, ems_u32* status, int max)
{
    if (max<7) return -1;
    SFI_R(dev, sequencer_status, status++);
    SFI_R(dev, fastbus_status_1, status++);
    SFI_R(dev, fastbus_status_2, status++);
    return 3;
}
/*****************************************************************************/
/*****************************************************************************/
