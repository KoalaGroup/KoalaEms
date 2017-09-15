/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_FWcd.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_FWcd.c,v 1.3 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

static int
FWcd(struct fastbus_dev* dev, ems_u32 pa, ems_u32 sa, ems_u32 data, ems_u32* ss,
    int mod, int disco)
{
    int res=0;

SIS3100_CHECK_BALANCE(dev, "FWcd_0");
    res=SEQ_W(dev, seq_prim_dsr|mod, pa);
    res|=SEQ_W(dev, seq_secad_w, sa);
    res|=SEQ_W(dev, seq_rndm_w|disco, data);
    if (res) {
        printf("sis3100_sfi_FWcd(a) pa=%d: res=%d\n", pa, res);
        return res;
    }
    res=sis3100_sfi_wait_sequencer(dev, ss);
    if (res<0) {
        printf("sis3100_sfi_FWcd(b) pa=%d: res=%d\n", pa, res);
        return res;
    }
SIS3100_CHECK_BALANCE(dev, "FWcd_1");
    return 0;
}

int
sis3100_sfi_FWC(struct fastbus_dev* dev, ems_u32 pa, ems_u32 sa, ems_u32 data,
        ems_u32* ss)
{
    return FWcd(dev, pa, sa, data, ss, SFI_MS0, SFI_FUNC_RNDM_DIS);
}
int
sis3100_sfi_FWCa(struct fastbus_dev* dev, ems_u32 pa, ems_u32 sa, ems_u32 data,
        ems_u32* ss)
{
    return FWcd(dev, pa, sa, data, ss, SFI_MS0, 0);
}
int
sis3100_sfi_FWD(struct fastbus_dev* dev, ems_u32 pa, ems_u32 sa, ems_u32 data,
        ems_u32* ss)
{
    return FWcd(dev, pa, sa, data, ss, 0, SFI_FUNC_RNDM_DIS);
}
int
sis3100_sfi_FWDa(struct fastbus_dev* dev, ems_u32 pa, ems_u32 sa, ems_u32 data,
        ems_u32* ss)
{
    return FWcd(dev, pa, sa, data, ss, 0, 0);
}
/*****************************************************************************/
/*****************************************************************************/
