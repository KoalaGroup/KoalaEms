/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_arbitrationlevel.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_arbitrationlevel.c,v 1.4 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

int
sis3100_sfi_setarbitrationlevel(struct fastbus_dev* dev, unsigned int arblevel)
{
    if (arblevel&~0xbf) return -1;
    return SFI_W(dev, fastbus_arbitration_level, arblevel);
}

int
sis3100_sfi_getarbitrationlevel(struct fastbus_dev* dev, unsigned int* arblevel)
{
    ems_u32 level;
    if (SFI_R(dev, fastbus_arbitration_level, &level)) return -1;
    *arblevel=level&0xff;
    return 0;
}
