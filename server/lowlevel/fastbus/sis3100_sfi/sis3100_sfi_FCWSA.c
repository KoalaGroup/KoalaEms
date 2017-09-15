/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_FCWSA.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_FCWSA.c,v 1.3 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

int
sis3100_sfi_FCWSA(struct fastbus_dev* dev, ems_u32 sa, ems_u32* ss)
{
    int res;

SIS3100_CHECK_BALANCE(dev, "FCWSA_0");
    res=SEQ_W(dev, seq_secad_w, sa);
    if (res) {
        printf("sis3100_sfi_FCWSA(a) res=%d\n", res);
        return res;
    }
    res=sis3100_sfi_wait_sequencer(dev, ss);
    if (res<0) {
        printf("sis3100_sfi_FCWSA(b) res=%d\n", res);
        return res;
    }
SIS3100_CHECK_BALANCE(dev, "FCWSA_1");
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
