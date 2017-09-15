/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_restart_sequencer.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_restart_sequencer.c,v 1.4 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

static volatile ems_u32 mist=0x0815;

void
sis3100_sfi_restart_sequencer(struct fastbus_dev* dev)
{
    if (SFI_W(dev, sequencer_reset, mist)) {
        printf("sfi_restart_sequencer: write reset: %s\n", strerror(errno));
        return;
    }
    if (SFI_W(dev, sequencer_enable, mist)) {
        printf("sfi_restart_sequencer: write enable: %s\n", strerror(errno));
        return;
    }
    /*printf("sequencer restarted\n");*/
}

/*****************************************************************************/
