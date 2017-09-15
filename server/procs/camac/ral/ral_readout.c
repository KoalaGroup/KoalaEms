/*
 * procs/camac/ral/ral_readout.c
 * 02.Aug.2001 PW: multicrate support
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: ral_readout.c,v 1.7 2011/04/06 20:30:30 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/camac/camac.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../procs.h"
#include "ralreg.h"

extern int* memberlist;
extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/camac/ral")

static ems_u32*
ral_testreadout(ml_entry* n, int what, int mode, ems_u32* data)
{
        struct camac_dev* dev=n->address.camac.dev;
	ems_u32 sta1, sta2;
        camadr_t addr;
	int ux;
	ems_u32 ret;

	sta1 = mode; /* XXX */

	switch (what) {
	    case 0:
		sta2 = RAL_CTL_TST_ZERO;
		break;
	    case 1:
		sta2 = RAL_CTL_TST_ONE;
		break;
	    case 2:
		sta2 = RAL_CTL_LD_TST;
		break;
	    default:
		return(0);
	}

	dev->CAMACwrite(dev, (addr=RAL_WR_DATA(n), &addr), 0); /* select global strobe */
	dev->CAMACwrite(dev, (addr=RAL_WR_STA1(n), &addr), sta1 & (RAL_BWS | RAL_DZS));
	dev->CAMACwrite(dev, (addr=RAL_WR_STA2(n), &addr), RAL_DXST);
	dev->CAMACcntl(dev, (addr=RAL_RESET_(n), &addr), &ret);
	dev->CAMACwrite(dev, (addr=RAL_WR_STA2(n), &addr), RAL_DXST | sta2);
	dev->CAMACcntl(dev, (addr=RAL_STROBE(n), &addr), &ret);
	dev->CAMACwrite(dev, (addr=RAL_WR_STA2(n), &addr), RAL_DXST);
	dev->CAMACwrite(dev, (addr=RAL_WR_STA2(n), &addr), RAL_DXST | RAL_CTL_ROUT);

	/* wait for conversion done */
	ux = 0;
	while (dev->CAMACread(dev, (addr=RAL_TST_FULL(n), &addr), &ret), !dev->CAMACgotQ(ret)) {
		if (ux++ > 10) {
			printf("testreadout failed\n");
			return(0);
		}
	}

	ux = 0;
	while (dev->CAMACread(dev, (addr=RAL_RD_FIFO(n), &addr), &ret), dev->CAMACgotQ(ret)) {
		*data++ = ret;
		ux++;
	}

	dev->CAMACwrite(dev, (addr=RAL_WR_STA2(n), &addr), RAL_DXST);

	return(data);
}

plerrcode
proc_RALtestreadout(ems_u32* p)
{
	ems_u32 *res;

	res = ral_testreadout(ModulEnt(p[1]), p[2], p[3], outptr);

	if (res)
		outptr = res;
	else
		return(plErr_HW);
	return(plOK);
}

plerrcode test_proc_RALtestreadout(ems_u32* p)
{
	if (p[0] != 3)
		return(plErr_ArgNum);
	return(plOK);
}

char name_proc_RALtestreadout[] = "RALtestreadout";

int ver_proc_RALtestreadout = 1;

plerrcode
proc_RALreadout(ems_u32* p)
{
        ml_entry* m=ModulEnt(p[1]);
        struct camac_dev* dev=m->address.camac.dev;
        camadr_t addr;
        ems_u32 ret;
	int ux;

	/* wait for conversion done */
	ux = 0;
	while (dev->CAMACread(dev, (addr=RAL_TST_FULL(m), &addr), &ret), !dev->CAMACgotQ(ret)) {
		if (ux++ > 1000) {
			printf("no data after trigger\n");
			return(plErr_HW);
		}
	}

	while (dev->CAMACread(dev, (addr=RAL_RD_FIFO(m), &addr), &ret), dev->CAMACgotQ(ret))
		*outptr++ = ret;

	dev->CAMACcntl(dev, (addr=RAL_START(m), &addr), &ret);

	return(plOK);
}

plerrcode
test_proc_RALreadout(ems_u32* p)
{
	if (p[0] != 1)
		return(plErr_ArgNum);
	if (!valid_module(p[1], modul_camac, 0))
		return (plErr_BadHWAddr);
	return(plOK);
}

char name_proc_RALreadout[] = "RALreadout";

int ver_proc_RALreadout = 1;
