/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_FCPcd.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_FCPcd.c,v 1.4 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

/* primary address cycle */
static int
FCPcd(struct fastbus_dev* dev, ems_u32 pa, ems_u32* ss, int mod)
{
    int res;
SIS3100_CHECK_BALANCE(dev, "FCPcd_0");
    res=SEQ_W(dev, seq_prim_dsr|mod, pa);
    if (res) {
        printf("sis3100_sfi_FCPcd(a) pa=%d: res=%d\n", pa, res);
        return res;
    }
    res=sis3100_sfi_wait_sequencer(dev, ss);
    if (res<0) {
        printf("sis3100_sfi_FCPcd(b) pa=%d: res=%d\n", pa, res);
        return res;
    }
SIS3100_CHECK_BALANCE(dev, "FCPcd_1");
    return 0;
}

int sis3100_sfi_FCPC(struct fastbus_dev* dev, ems_u32 pa, ems_u32* ss)
{
    return FCPcd(dev, pa, ss, SFI_MS0);
}
int sis3100_sfi_FCPD(struct fastbus_dev* dev, ems_u32 pa, ems_u32* ss)
{
    return FCPcd(dev, pa, ss, 0);
}
/*****************************************************************************/
/*****************************************************************************/
