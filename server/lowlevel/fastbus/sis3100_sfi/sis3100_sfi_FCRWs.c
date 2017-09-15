/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_FCRWs.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_FCRWs.c,v 1.3 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"

static volatile ems_u32 mist=0x0815;

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

static int
FCRWs(struct fastbus_dev* dev, ems_u32 sa, ems_u32* data, ems_u32* ss,
        int use_sa)
{
    int res=0;

SIS3100_CHECK_BALANCE(dev, "FCRWs_0");
    if (use_sa) res|=SEQ_W(dev, seq_secad_w, sa);
    res|=SEQ_W(dev, seq_rndm_r, mist);
    if (res) {
        printf("sis3100_sfi_FCRWs(a) res=%d\n", res);
        return res;
    }
    res=sis3100_sfi_read_fifoword(dev, data, ss);
    if (res<0) {
        printf("sis3100_sfi_FCRWs(b) res=%d\n", res);
        return res;
    }
SIS3100_CHECK_BALANCE(dev, "FCRWs_1");
    return 0;
}

int sis3100_sfi_FCRW(struct fastbus_dev* dev, ems_u32* data, ems_u32* ss)
{
    return FCRWs(dev, 0, data, ss, 0);
}
int sis3100_sfi_FCRWS(struct fastbus_dev* dev, ems_u32 sa, ems_u32* data,
        ems_u32* ss)
{
    return FCRWs(dev, sa, data, ss, 1);
}
/*****************************************************************************/
/*****************************************************************************/
