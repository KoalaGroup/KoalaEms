/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_FWcdM.c
 * 
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_FWcdM.c,v 1.3 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

static int
FWcdM(struct fastbus_dev* dev, ems_u32 ba, ems_u32 data, ems_u32* ss, int mod)
{
    int res=0;

SIS3100_CHECK_BALANCE(dev, "FWcdM_0");
    res=SEQ_W(dev, seq_prim_dsrm|mod, ba);
    res|=SEQ_W(dev, seq_rndm_w_dis, data);
    if (res) {
        printf("sis3100_sfi_FWcdM(a) ba=%d: res=%d\n", ba, res);
        return res;
    }
    res=sis3100_sfi_wait_sequencer(dev, ss);
    if (res<0) {
        printf("sis3100_sfi_FWcdM(b) ba=%d: res=%d\n", ba, res);
        return res;
    }
SIS3100_CHECK_BALANCE(dev, "FWcdM_1");
    return 0;
}

int
sis3100_sfi_FWCM(struct fastbus_dev* dev, ems_u32 ba, ems_u32 data, ems_u32* ss)
{
    return FWcdM(dev, ba, data, ss, SFI_MS0);
}
int
sis3100_sfi_FWDM(struct fastbus_dev* dev, ems_u32 ba, ems_u32 data, ems_u32* ss)
{
    return FWcdM(dev, ba, data, ss, 0);
}
/*****************************************************************************/
/*****************************************************************************/
