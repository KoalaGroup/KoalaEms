/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_FRcdM.c
 * 
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_FRcdM.c,v 1.4 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

static volatile ems_u32 mist=0x0815;

static int
FRcdM(struct fastbus_dev* dev, ems_u32 ba, ems_u32* data, ems_u32* ss, int mod)
{
    int res=0;

SIS3100_CHECK_BALANCE(dev, "FRcdM_0");
    res=SEQ_W(dev, seq_prim_dsrm|mod, ba);
    res|=SEQ_W(dev, seq_rndm_r_dis, mist);
    if (res) {
        printf("sis3100_sfi_FRcdM(a) ba=0x%x: res=%d\n", ba, res);
        return res;
    }
    res=sis3100_sfi_read_fifoword(dev, data, ss);
    if (res<0) {
        printf("sis3100_sfi_FRcdM(b) ba=%d: res=%d\n", ba, res);
        return res;
    }
SIS3100_CHECK_BALANCE(dev, "FRcdM_1");
    return 0;
}

int
sis3100_sfi_FRDM(struct fastbus_dev* dev, ems_u32 ba, ems_u32* data,
        ems_u32* ss)
{
    return FRcdM(dev, ba, data, ss, 0);
}

int
sis3100_sfi_FRCM(struct fastbus_dev* dev, ems_u32 ba, ems_u32* data,
        ems_u32* ss)
{
    return FRcdM(dev, ba, data, ss, SFI_MS0);
}
/*****************************************************************************/
/*****************************************************************************/
