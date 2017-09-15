/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_FCDISC.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_FCDISC.c,v 1.4 2011/04/06 20:30:23 wuestner Exp $";

#include <rcs_ids.h>
#include "sis3100_sfi.h"

static volatile ems_u32 mist=0x0815;

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

int
sis3100_sfi_FCDISC(struct fastbus_dev* dev)
{
    ems_u32 ss;
    int res;
SIS3100_CHECK_BALANCE(dev, "FCDISC_0");
    res=SEQ_W(dev, SFI_FUNC_DISCON, mist);
    if (res) {
        printf("sis3100_sfi_FCDISC(a): res=%d\n", res);
        return res;
    }
    res=sis3100_sfi_wait_sequencer(dev, &ss);
    if (res<0) {
        printf("sis3100_sfi_FCDISC(b): res=%d\n", res);
        return res;
    }
SIS3100_CHECK_BALANCE(dev, "FCDISC_1");
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
