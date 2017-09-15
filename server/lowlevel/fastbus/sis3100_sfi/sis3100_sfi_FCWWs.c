/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_FCWWs.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_FCWWs.c,v 1.3 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

static int
FCWWs(struct fastbus_dev* dev, ems_u32 sa, ems_u32 data, ems_u32* ss,
        int use_sa)
{
    int res=0;

SIS3100_CHECK_BALANCE(dev, "FCWWs_0");
    if (use_sa) res|=SEQ_W(dev, seq_secad_w, sa);
    res|=SEQ_W(dev, seq_rndm_w, data);
    if (res) {
        printf("sis3100_sfi_FCWWs(a) res=%d\n", res);
        return res;
    }
    res=sis3100_sfi_wait_sequencer(dev, ss);
    if (res<0) {
        printf("sis3100_sfi_FCWWs(b) res=%d\n", res);
        return res;
    }
SIS3100_CHECK_BALANCE(dev, "FCWWs_1");
    return 0;
}

int sis3100_sfi_FCWW(struct fastbus_dev* dev, ems_u32 data, ems_u32* ss)
{
    return FCWWs(dev, 0, data, ss, 0);
}
int sis3100_sfi_FCWWS(struct fastbus_dev* dev, ems_u32 sa, ems_u32 data,
        ems_u32* ss)
{
    return FCWWs(dev, sa, data, ss, 1);
}
/*****************************************************************************/
/*****************************************************************************/
