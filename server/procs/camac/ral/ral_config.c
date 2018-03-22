/*
 * procs/camac/ral/ral_config.c
 *
 * 19.Jan.2001 PW: multicrate support (CAMACslot --> ModulEnt)
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: ral_config.c,v 1.8 2017/10/20 23:20:52 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../../../lowlevel/camac/camac.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../procs.h"
#include "ralreg.h"
#include "ral_config.h"

#define DEF_STA1 0

RCS_REGISTER(cvsid, "procs/camac/ral")

static int rtest_q(struct camac_dev* dev, camadr_t addr)
{
	ems_u32 val;

	dev->CAMACread(dev, &addr, &val);
	if (!dev->CAMACgotX(val) || !dev->CAMACgotQ(val))
		return -1;
	return 0;
}

static int rtest_noq(struct camac_dev* dev, camadr_t addr)
{
	ems_u32 val;

	dev->CAMACread(dev, &addr, &val);
	if (!dev->CAMACgotX(val) || dev->CAMACgotQ(val))
		return -1;
	return 0;
}

static int test_q(struct camac_dev* dev)
{
	ems_u32 stat;

	dev->CAMACstatus(dev, &stat);
	if (!dev->CAMACgotX(stat) || !dev->CAMACgotQ(stat))
		return -1;
	return 0;
}

int ral_reset_module(ml_entry* n)
{
        struct camac_dev* dev=n->address.camac.dev;
        camadr_t addr;

	if (rtest_q(dev, RAL_GEN_CLR(n)))
		return 1;

	if (rtest_q(dev, RAL_RESET_(n)))
		return 2;

	if (rtest_noq(dev, RAL_RD_FIFO(n)))
		return 3;

	if (rtest_noq(dev, RAL_TST_FIFO(n)))
		return 4;

	dev->CAMACwrite(dev, (addr=RAL_WR_DATA(n), &addr), 0x0f);
	if (test_q(dev))
		return 5;

	dev->CAMACwrite(dev, (addr=RAL_WR_STA1(n), &addr), DEF_STA1);
	if (test_q(dev))
		return 6;

	dev->CAMACwrite(dev, (addr=RAL_WR_STA2(n), &addr), 0x00);
	if (test_q(dev))
		return 7;

	return 0;
}

static void ral_select_column(ml_entry* n, int col)
{
        struct camac_dev* dev=n->address.camac.dev;
        camadr_t addr;
        ems_u32 qx;
	int ix;

	dev->CAMACwrite(dev, (addr=RAL_WR_STA2(n), &addr), 0);
	/* STP_FCLK must not be set */
	dev->CAMACcntl(dev, (addr=RAL_RESET_(n), &addr), &qx);
        dev->CAMACwrite(dev, (addr=RAL_WR_STA2(n), &addr), RAL_STP_FCLK);

	for (ix = 0; ix < col; ix++) {
		dev->CAMACwrite(dev, (addr=RAL_WR_STA2(n), &addr), RAL_CTL_LD_DAC | RAL_STP_FCLK);
		dev->CAMACwrite(dev, (addr=RAL_WR_STA2(n), &addr), RAL_STP_FCLK);
	}
}

static int ral_countchips(ml_entry* n, int col)
{
        struct camac_dev* dev=n->address.camac.dev;
	int rpc, maxregs, ux, n5, na, nx;
        camadr_t addr;
        ems_u32 qx;
        ems_u32 ret;

	dev->CAMACwrite(dev, (addr=RAL_WR_STA1(n), &addr), 0);

	ral_select_column(n, col);

#if 1
	dev->CAMACwrite(dev, (addr=RAL_WR_STA2(n), &addr), RAL_CTL_LD_DAC | RAL_STP_FCLK);
	rpc = 1;
#else
	dev->CAMACwrite(dev, (addr=RAL_WR_STA2(n), &addr), RAL_CTL_LD_TST | RAL_STP_FCLK);
	rpc = 4;
#endif
	maxregs = RAL_MAX_ROWS * rpc + 1;

	/* load registers with 0x05 */
	dev->CAMACwrite(dev, (addr=RAL_WR_DATA(n), &addr), 0x5);
	for (ux = 0; ux < maxregs; ux++)
		dev->CAMACcntl(dev, (addr=RAL_CLOCK(n), &addr), &qx);

	/* clear FIFO */
	ux = 0;
	while (dev->CAMACread(dev, (addr=RAL_RD_FIFO(n), &addr), &ret), dev->CAMACgotQ(ret))
		ux++;
	if (ux != maxregs) {
		printf("countchips: bad FIFO count (%d)\n", ux);
		return(-1);
	}

	/* load registers with 0x0a */
	dev->CAMACwrite(dev, (addr=RAL_WR_DATA(n), &addr), 0xa);
	for (ux = 0; ux < maxregs; ux++)
		dev->CAMACcntl(dev, (addr=RAL_CLOCK(n), &addr), &qx);

	/* count number of 0x05 and 0x0a */
	n5 = na = nx = 0;
	while (dev->CAMACread(dev, (addr=RAL_RD_FIFO(n), &addr), &ret), dev->CAMACgotQ(ret)){
		ret &= 0x0f;
		if (ret == 0x05) {
			if (na) /* 5's after a's are errors */
				nx++;
			else
				n5++;
		} else if (ret == 0x0a)
			na++;
		else
			nx++;
	}

	if (n5 + na + nx != maxregs) {
		printf("countchips: bad FIFO count (%d)\n", n5 + na + nx);
		return(-1);
	}

	if (nx || n5 < 2 || (n5 - 1) % rpc) {
#if 0
		printf("countchips: %d, %d, %d\n", n5, na, nx);
#endif
		return(0);
	}

	return((n5 - 1)/ rpc);
}

static int ral_datapath_test(ml_entry* n, int col, int cnt)
{
        struct camac_dev* dev=n->address.camac.dev;
	int chips, rpc, regs, dat_o, dat_i, ux;
        camadr_t addr;
        ems_u32 ret;

	chips = ral_countchips(n, col);
	if (chips < 1)
		return(1);

	ral_select_column(n, col);
#if 1
	dev->CAMACwrite(dev, (addr=RAL_WR_STA2(n), &addr), RAL_CTL_LD_DAC | RAL_STP_FCLK);
	rpc = 1;
#else
	dev->CAMACwrite(dev, (addr=RAL_WR_STA2(n), &addr), RAL_CTL_LD_TST | RAL_STP_FCLK);
	rpc = 4;
#endif
	regs = chips * rpc + 1; /* note intermediate register */

	dat_o = 0;
	dat_i = 0;

	for (ux = 0; ux < regs; ux++) {
		dev->CAMACwrite(dev, (addr=RAL_WR_DATA(n), &addr), dat_o);
		dev->CAMACcntl(dev, (addr=RAL_CLOCK(n), &addr), &ret);
		dat_o++;
	}

	ux = 0;
	while (dev->CAMACread(dev, (addr=RAL_RD_FIFO(n), &addr), &ret), dev->CAMACgotQ(ret))
		ux++; /* clear FIFO */
	if (ux != regs) {
		printf("datapath test: bad FIFO count (%d)\n", ux);
		return(2);
	}

	while (cnt > 0) {
		regs = RAL_FIFO_LEN + 1;
		if (regs > cnt)
			regs = cnt;
		cnt -= regs;

		for (ux = 0; ux < regs; ux++) {
			dev->CAMACwrite(dev, (addr=RAL_WR_DATA(n), &addr), dat_o);
			dev->CAMACcntl(dev, (addr=RAL_CLOCK(n), &addr), &ret);
			dat_o++;
		}

		ux = 0;
		while (dev->CAMACread(dev, (addr=RAL_RD_FIFO(n), &addr), &ret), dev->CAMACgotQ(ret)) {
			if (((u_int)ret ^ dat_i) & 0xF) {
				printf("datapath test: bad data (%x/%x)\n",
				       ret, dat_i);
				return(3);
			}
			ux++;
			dat_i++;
		}
		if (ux != regs) {
			printf("datapath test: %d expexted / %d read\n",
			       regs, ux);
			return(4);
		}
	}
	return(0);
}

plerrcode proc_RALconfig(ems_u32* p)
{
	int i, res;

	if ((res = ral_reset_module(ModulEnt(p[1])))) {
		*outptr++ = res;
		return(plErr_HW);
	}

	for(i = 0; i < RAL_MAX_COLUMNS; i++) {
		res = ral_countchips(ModulEnt(p[1]), i);
		if (res == -1)
			return(plErr_HW);
		if (res == 0)
			break;
		*outptr++ = res;
	}

	ral_reset_module(ModulEnt(p[1]));
	return(plOK);
}

plerrcode test_proc_RALconfig(ems_u32* p)
{
	if (p[0] != 1)
		return(plErr_ArgNum);
	return(plOK);
}

char name_proc_RALconfig[] = "RALconfig";

int ver_proc_RALconfig = 1;

plerrcode proc_RALdatapathtest(ems_u32* p)
{
	int res;

	if (ral_reset_module(ModulEnt(p[1])))
		return(plErr_HW);

	res = ral_datapath_test(ModulEnt(p[1]), p[2], p[3]);
	if (res != 0) {
		*outptr++ = res;
		return(plErr_HW);
	}

	ral_reset_module(ModulEnt(p[1]));
	return(plOK);
}

plerrcode test_proc_RALdatapathtest(ems_u32* p)
{
	if (p[0] != 3)
		return(plErr_ArgNum);
	return(plOK);
}

char name_proc_RALdatapathtest[] = "RALdatapathtest";

int ver_proc_RALdatapathtest = 1;

static int loadregs(ml_entry* n, int col, int which, u_int8_t* data, int len)
{
        struct camac_dev* dev=n->address.camac.dev;
        camadr_t addr;
        ems_u32 res;
	int i;

	ral_select_column(n, col);

	if (which)
		dev->CAMACwrite(dev, (addr=RAL_WR_STA2(n), &addr), RAL_CTL_LD_DAC | RAL_STP_FCLK);
	else
		dev->CAMACwrite(dev, (addr=RAL_WR_STA2(n), &addr), RAL_CTL_LD_TST | RAL_STP_FCLK);

	for (i = 0; i < len; i++) {
		dev->CAMACwrite(dev, (addr=RAL_WR_DATA(n), &addr), *data++);
		dev->CAMACcntl(dev, (addr=RAL_CLOCK(n), &addr), &res);
	}

	i = 0;
	while (dev->CAMACread(dev, (addr=RAL_RD_FIFO(n), &addr), &res), dev->CAMACgotQ(res)) {
#if 0
                printf("loadregs: got %x\n", res);
#endif
		i++; /* clear FIFO */
	}
	if (i != len)
		return(-1);

	return(0);
}

plerrcode proc_RALloadtestregs(ems_u32* p)
{
	u_int8_t data[RAL_MAX_ROWS * 4];
	unsigned int len;
	unsigned int i;
	char *tmp;

	if (ral_reset_module(ModulEnt(p[1])))
		return(plErr_HW);

	len = ral_countchips(ModulEnt(p[1]), p[2]) * 4;

	switch (p[3]) {
	    case 0:
	    case 1:
		memset(data, p[3] == 0 ? 0 : 0x0f, len);
		for (i = 4; i <= p[0]; i++) {
			unsigned int bytepos;
			u_int8_t bit;
			bytepos = (p[i] / 16) * 4 + (3 - (p[i] / 4) % 4);
			if (bytepos >= len)
			    return(plErr_ArgRange);
			bit = (1 << (p[i] % 4));
			if (p[3] == 0)
				data[bytepos] |= bit;
			else
				data[bytepos] &= ~bit;
		}
		break;
	    case 2:
		if (xdrstrclen(&p[4]) * 2 != len)
			return(plErr_ArgNum);
		xdrstrcdup(&tmp, &p[4]);
		for (i = 0; i < len / 4; i++) {
			data[4 * i + 3] = tmp[2 * i] & 0x0f;
			data[4 * i + 2] = tmp[2 * i] >> 4;
			data[4 * i + 1] = tmp[2 * i + 1] & 0x0f;
			data[4 * i] = tmp[2 * i + 1] >> 4;
		}
		free(tmp);
		break;
	}

	if (loadregs(ModulEnt(p[1]), p[2], 0, data, len))
		return(plErr_HW);

	ral_reset_module(ModulEnt(p[1]));
	return(plOK);
}

plerrcode test_proc_RALloadtestregs(ems_u32* p)
{
	if (p[0] < 3)
		return(plErr_ArgNum);
	switch (p[3]) {
	    case 0:
	    case 1:
		if (p[0] > RAL_MAX_ROWS * 16 + 3)
			return(plErr_ArgNum);
		break;
	    case 2:
		if ((p[0] < 4) ||
		    (p[0] != xdrstrlen(&p[4]) + 3))
			return(plErr_ArgNum);
		break;
	    default:
		return(plErr_ArgRange);
	}
	return(plOK);
}

char name_proc_RALloadtestregs[] = "RALloadtestregs";

int ver_proc_RALloadtestregs = 1;

plerrcode proc_RALloaddac(ems_u32* p)
{
	ems_u8 data[RAL_MAX_ROWS];
	unsigned int len;

	if (ral_reset_module(ModulEnt(p[1])))
		return(plErr_HW);

	len = ral_countchips(ModulEnt(p[1]), p[2]);

	switch (p[3]) {
	    case 0:
		memset(data, p[4], len);
		break;
	    case 1:
		if (xdrstrclen(&p[4]) != len)
		    return(plErr_ArgNum);
		extractstring((char*)data, &p[4]);
		break;
	}

	if (loadregs(ModulEnt(p[1]), p[2], 1, data, len))
	    return(plErr_HW);

	ral_reset_module(ModulEnt(p[1]));
	return(plOK);
}

plerrcode test_proc_RALloaddac(ems_u32* p)
{
	if (p[0] < 3)
		return(plErr_ArgNum);
	switch (p[3]) {
	    case 0:
		if (p[0] != 4)
			return(plErr_ArgNum);
		break;
	    case 1:
		if ((p[0] < 4) ||
		    (p[0] != xdrstrlen(&p[4]) + 3))
			return(plErr_ArgNum);
		break;
	    default:
		return(plErr_ArgRange);
	}
	return(plOK);
}

char name_proc_RALloaddac[] = "RALloaddac";

int ver_proc_RALloaddac = 1;
