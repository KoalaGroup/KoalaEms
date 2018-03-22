/*
 * procs/camac/ral/ral_filterram.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: ral_filterram.c,v 1.8 2017/10/20 23:20:52 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../../../lowlevel/camac/camac.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../procs.h"
#include "ralreg.h"
#include "ral_config.h"

RCS_REGISTER(cvsid, "procs/camac/ral")

static void ral_load_filter(ml_entry* n, u_int8_t* data)
{
        struct camac_dev* dev=n->address.camac.dev;
        camadr_t addr;
	int ax;

	/* BadWireSuppress, disable filter access */
	dev->CAMACwrite(dev, (addr=RAL_WR_STA1(n), &addr), RAL_BWS);
	/* dto. enable, counter cleared */
	dev->CAMACwrite(dev, (addr=RAL_WR_STA1(n), &addr), 0);

	for (ax = 0; ax < RAL_FRAM_LEN; ax += 8) {
		u_int8_t help;
		int b;

		help = *data++;
		for (b = 0; b < 8; b++) {
			dev->CAMACwrite(dev, (addr=RAL_WR_RAM(n), &addr), help & 1);
			help >>= 1;
		}
	}
}

static void ral_read_filter(ml_entry* n, u_int8_t* data)
{
        struct camac_dev* dev=n->address.camac.dev;
        camadr_t addr;
        ems_u32 res;
	int ax;

	/* BadWireSuppress, disable filter access */
	dev->CAMACwrite(dev, (addr=RAL_WR_STA1(n), &addr), RAL_BWS);
	/* dto. enable, counter cleared */
	dev->CAMACwrite(dev, (addr=RAL_WR_STA1(n), &addr), 0);

	for (ax = 0; ax < RAL_FRAM_LEN; ax += 8) {
		u_int8_t help;
		int b;

		help = 0;
		for (b = 0; b < 8; b++) {
			if (dev->CAMACread(dev, (addr=RAL_RD_RAM(n), &addr), &res), dev->CAMACgotQ(res))
				help |= (1 << b);
		}
		*data++ = help;
	}
}

static int ral_test_filter(ml_entry* n)
{
	u_int8_t data[RAL_FRAM_LEN / 8];
	u_int8_t pattern;
	int i, erc;

	pattern = 0xa5;
	for (i = 0; i < RAL_FRAM_LEN / 8; i++) {
		data[i] = pattern;
		pattern = (pattern << 1) ^ i;
	}

	ral_load_filter(n, data);

	bzero(data, sizeof(data));

	ral_read_filter(n, data);

	erc = 0;
	pattern = 0xa5;
	for (i = 0; i < RAL_FRAM_LEN / 8; i++) {
		if (data[i] != pattern) {
			erc++;
		}
		pattern = (pattern << 1) ^ i;
	}

	return(erc);
}

plerrcode proc_RALfilterramload(ems_u32* p)
{
	u_int8_t *data;
	unsigned int i;

	data = malloc(RAL_FRAM_LEN / 8);

	switch (p[2]) {
	    case 0:
	    case 1:
		memset(data, p[2] == 0 ? 0 : 0xff, RAL_FRAM_LEN / 8);
		for (i = 3; i <= p[0]; i++) {
			int bytepos;
			u_int8_t bit;
			bytepos = p[i] / 8;
			bit = (1 << (p[i] % 8));
			if (p[2] == 0)
				data[bytepos] |= bit;
			else
				data[bytepos] &= ~bit;
		}
		break;
	    case 2:
		extractstring((char*)data, &p[3]);
		break;
	}
	ral_load_filter(ModulEnt(p[1]), data);
	free(data);

	return(plOK);
}

plerrcode test_proc_RALfilterramload(ems_u32* p)
{
	if (p[0] < 2)
		return(plErr_ArgNum);
	switch (p[2]) {
	    case 0:
	    case 1:
		if (p[0] > RAL_FRAM_LEN + 2)
			return(plErr_ArgNum);
		break;
	    case 2:
		if ((p[0] < 3) ||
		    (p[0] != xdrstrlen(&p[3]) + 2) ||
		    (xdrstrclen(&p[3]) != RAL_FRAM_LEN / 8))
			return(plErr_ArgNum);
		break;
	    default:
		return(plErr_ArgRange);
	}
	return(plOK);
}

char name_proc_RALfilterramload[] = "RALfilterramload";

int ver_proc_RALfilterramload = 1;

plerrcode proc_RALfilterramread(ems_u32* p)
{
	u_int8_t data[RAL_FRAM_LEN / 8];

	ral_read_filter(ModulEnt(p[1]), data);
	outptr = outnstring(outptr, (char*)data, RAL_FRAM_LEN / 8);

	return(plOK);
}

plerrcode test_proc_RALfilterramread(ems_u32* p)
{
	if (p[0] != 1)
		return(plErr_ArgNum);
	return(plOK);
}

char name_proc_RALfilterramread[] = "RALfilterramread";

int ver_proc_RALfilterramread = 1;

plerrcode proc_RALfilterramtest(ems_u32* p)
{
	int res;

	if (ral_reset_module(ModulEnt(p[1])))
		return(plErr_HW);

	res = ral_test_filter(ModulEnt(p[1]));
	if (res != 0) {
		*outptr++ = res;
		return(plErr_HW);
	}

	ral_reset_module(ModulEnt(p[1]));
	return(plOK);
}

plerrcode test_proc_RALfilterramtest(ems_u32* p)
{
	if (p[0] != 1)
		return(plErr_ArgNum);
	return(plOK);
}

char name_proc_RALfilterramtest[] = "RALfilterramtest";

int ver_proc_RALfilterramtest = 1;
