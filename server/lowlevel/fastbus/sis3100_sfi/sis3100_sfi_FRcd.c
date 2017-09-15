/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_FRcd.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_FRcd.c,v 1.4 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <emsctypes.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"

static volatile ems_u32 mist=0x0815;

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

static int
FRcd(struct fastbus_dev* dev, ems_u32 pa, ems_u32 sa, ems_u32* data, ems_u32* ss,
    int mod, int disco)
{
    int res=0;

SIS3100_CHECK_BALANCE(dev, "FRcd_0");
    res|=SEQ_W(dev, seq_prim_dsr|mod, pa);
    res|=SEQ_W(dev, seq_secad_w, sa);
    res|=SEQ_W(dev, seq_rndm_r|disco, mist);
    if (res) {
        printf("sis3100_sfi_FRcd(a) pa=%d: res=%d\n", pa, res);
        return res;
    }
    res=sis3100_sfi_read_fifoword(dev, data, ss);
    if (res<0) {
        printf("sis3100_sfi_FRcd(b) pa=%d: res=%d\n", pa, res);
        return res;
    }
SIS3100_CHECK_BALANCE(dev, "FRcd_1");
    return 0;
}

int
sis3100_sfi_FRC(struct fastbus_dev* dev, ems_u32 pa, ems_u32 sa, ems_u32* data,
        ems_u32* ss)
{
    return FRcd(dev, pa, sa, data, ss, SFI_MS0, SFI_FUNC_RNDM_DIS);
}
int
sis3100_sfi_FRCa(struct fastbus_dev* dev, ems_u32 pa, ems_u32 sa, ems_u32* data,
        ems_u32* ss)
{
    return FRcd(dev, pa, sa, data, ss, SFI_MS0, 0);
}
int
sis3100_sfi_FRD(struct fastbus_dev* dev, ems_u32 pa, ems_u32 sa, ems_u32* data,
        ems_u32* ss)
{
    return FRcd(dev, pa, sa, data, ss, 0, SFI_FUNC_RNDM_DIS);
}
int
sis3100_sfi_FRDa(struct fastbus_dev* dev, ems_u32 pa, ems_u32 sa, ems_u32* data,
        ems_u32* ss)
{
    return FRcd(dev, pa, sa, data, ss, 0, 0);
}
/*****************************************************************************/
/*****************************************************************************/
