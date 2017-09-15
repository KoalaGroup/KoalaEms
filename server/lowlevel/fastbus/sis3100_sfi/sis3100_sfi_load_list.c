/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_load_list.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_load_list.c,v 1.2 2011/04/06 20:30:23 wuestner Exp $";

#include <rcs_ids.h>
#include "sis3100_sfi.h"
#include "../../oscompat/oscompat.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

int
sis3100_sfi_load_list(struct fastbus_dev* dev, int addr, int size,
        struct sficommand* commands)
{
    int i;

    if ((addr&0xffff8000) || (size<1) || (size>0x8000)) {
        printf("sis3100_sfi_load_list: invalid arguments\n");
        return -1;
    }

    SFI_W(dev, sequencer_disable, 0);
    SFI_W(dev, next_sequencer_ram_address, addr);
    SFI_W(dev, sequencer_ram_load_enable, 0);
    for (i=0; i<size; i++) {
        printf("loading 0x%08x 0x%08x\n", commands[i].cmd, commands[i].data);
        SEQ_W(dev, commands[i].cmd, commands[i].data);
    }
    tsleep(100);
    SFI_W(dev, sequencer_ram_load_disable, 0);
    SFI_W(dev, sequencer_enable, 0);

    return 0;
}
