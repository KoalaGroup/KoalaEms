// Borland C++ - (C) Copyright 1991, 1992 by Borland International

/*	input.c -- general functions  */

#include <stddef.h>		// offsetof()
#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <io.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <mem.h>
#include <time.h>
#include <windows.h>
#include <fcntl.h>
#include "pci.h"
#include "math.h"
#include "daq.h"
#include "seq.h"

#define	MEN_SEQ				0
#define	MEN_ADC				1
#define	MEN_SLOWCTRL		2

#define	MEN_MEM				4
#define	MEN_MAIN				5

extern VTEX_BC		*inp_bc;
extern VTEX_RG		*inp_unit;

static char		fname[]="seq0.bin";
//
static const u_int va_ctrl0[] = {
0x8C44,0x4444,0x4444,0x4444,0x4444,0x459C,0x4444,0x4444,0x4000,0x0000,
0x00DE,0x299F,0x1965,0x6FF6,0x66FF,0xF966,0x56F6,0x1688,0xFFF6,0xF000,
0x0000,0x01BC,0x8FEF,0xF751,0xEFFE,0x6EE8,0xFAF7,0x085A,0x7F0E,0xF5CB,
0xE000,0x0000,0x037C,0x6BFF,0xFFD7,0xE747,0xF71F,0x3FDE,0x18D1,0xDDDC,
0xDFDD,0xC000,0x0000,0x06F9,0xFB7C,0x1CE4,0x6436,0xB313,0x37B3,0x47FD,
0x54FC,0xCCCB,0x0000,0x0000,0x0DE0
};

static const u_int va_ctrl1[] = {
0xAC44,0x4444,0x445C,0x0CCD,0x094C,0x4DC4,0x1141,0x48C8,0x8800,0x0000,
0x04FA,0x424B,0x1B91,0x1828,0x2988,0xB882,0x381A,0xCA89,0xA889,0x1000,
0x0000,0x0976,0x3230,0x2571,0x1311,0x7111,0x1325,0x7303,0x4434,0x4502,
0x4000,0x0000,0x12EC,0x28A2,0xE22C,0x7222,0xA686,0x6622,0x8E20,0x4C44,
0xA654,0x8000,0x0000,0x27D1,0x4C40,0x90C1,0x440D,0x4445,0x0045,0x41D5,
0x448C,0x48C4,0x0000,0x0000,0x4BA0
};

typedef struct { char *txt; u_int pos,len,cnt; } VAREG;
static const VAREG	vareg[5] = {
	{ "GlThr",   0,  5,  0 },
	{ "LoThr",   5,  4, 31 },
	{ "Disa",  133, 32,  0 },
	{ "GlRef", 165,  5,  0 },
	{ "se.ca", 170,  5,  0 }
};

//--------------------------- read SRAM -------------------------------------
//
static u_long	v_sraddr;

static int sram_out(void)
{
	u_int		i;
	u_long	a;

	printf("SR bank  %06lX\n", SQ_SRAM_LEN);
	printf("SR addr  %06lX ", v_sraddr);
	v_sraddr=Read_Hexa(v_sraddr, -1);
	if (errno) return CEN_MENU;

	inp_unit->ADR_CR=0;
	inp_unit->ADR_RAMT_ADR=a=v_sraddr;
	while (1) {
		printf("%06lX (pg %u):\n", a, (u_int)(a /SQ_SRAM_LEN));
		for (i=0; i < 0x100; i++) {
			printf(" %04X", inp_unit->ADR_RAMT_DATA);
		}
		a+=0x100;
		if (WaitKey(0)) return CE_PROMPT;
	}
}
//===========================================================================
//										vtx_ldac => load DAC values
//===========================================================================
//
static const char *d_txt[] = {
"VTHR", "REFBI", "VFP", "VFS", "DigTst", "SBI_TA", "VSFS", "T_OUT"
};

static int vtx_ldac(u_int hv, u_int dial)
{
	u_int		i,j;

	if (dial) {
		for (i=0; i < 8; i++) {
			printf("%u %-6s: %4d ", i, d_txt[i], config.vtx_dac[i]);
			config.vtx_dac[i]=(u_int)Read_Deci(config.vtx_dac[i], 0xFFF);
			if (errno)  return CEN_MENU;
		}
	}

	if (!hv) {
		for (i=0; i < 8; i++) {
			inp_unit->lv.ADR_DAC=(i<<12) |(config.vtx_dac[i] &0xFFF);
			j=0;
			while ((gtmp=inp_unit->ADR_SR) &0x0400)
				if (++j >= 10) {
					printf("timeout writing DAC\n"); return CE_PROMPT;
				}
		}

		inp_unit->lv.ADR_DAC=0x803C;
		return 0;
	}

	for (i=0; i < 8; i++) {
		inp_unit->hv.ADR_DAC=(i<<12) |(config.vtx_dac[i] &0xFFF);
		j=0;
		while ((gtmp=inp_unit->ADR_SR) &0x0800)
			if (++j >= 10) {
				printf("timeout writing DAC\n"); return CE_PROMPT;
			}
	}

	inp_unit->hv.ADR_DAC=0x803C;
	return 0;
}
//
//--------------------------- load_seq ----------------------------------- --
//
static int load_seq(void)
{
	u_int	i;
	u_int	cr;

	printf("VATA timing %u ", config.vtx_vtim);
	config.vtx_vtim=(u_int)Read_Deci(config.vtx_vtim, 9);
	if (errno) return CEN_MENU;

	cr=inp_unit->ADR_CR; inp_unit->ADR_CR=0; i=fname[3];
	fname[3]='0'+config.vtx_vtim;
	if (seq_load(inp_unit, fname, 0) != 0) {
		fname[3]=i;
		inp_unit->ADR_CR=cr;
		return CE_PROMPT;
	}

	seq_load(inp_unit, fname, 1);
	inp_unit->ADR_CR=cr;
	return CE_PROMPT;
}
//
//===========================================================================
//										vtx_lreg => load VATA reg values
//===========================================================================
//
static char	pbuf[60];

static int vtx_lreg(u_int hv,
	u_int mode)	// 0=binary; 1=functionally; 2=only load
{
	u_int		i,j;
	u_int		c,b;
	int		p;
	u_int		n,k,l;
	u_int		dat,tmp;
	u_long	ldat;

	const u_int	*defval;

	if (mode == 1) {
		printf("a: Gl Thre     %02X ", config.vtx_regp.glthr);
		config.vtx_regp.glthr =(u_char)Read_Hexa(config.vtx_regp.glthr, -1);
		if (errno) return CEN_MENU;
		printf("a: Loc Thre    %02X ", config.vtx_regp.locthr);
		config.vtx_regp.locthr =(u_int)Read_Hexa(config.vtx_regp.locthr, -1);
		if (errno) return CEN_MENU;
		for (c=0; c < 5; c++) {
			printf("%u: dable %08lX ", c, config.vtx_regp.disable[c]);
			config.vtx_regp.disable[c] =Read_Hexa(config.vtx_regp.disable[c], -1);
			if (errno) return CEN_MENU;
		}

		printf("a: Gl Ref      %02X ", config.vtx_regp.glref);
		config.vtx_regp.glref =(u_char)Read_Hexa(config.vtx_regp.glref, -1);
		if (errno) return CEN_MENU;
		printf("a: p s p n c   %02X ", config.vtx_regp.ctrl);
		config.vtx_regp.ctrl =(u_char)Read_Hexa(config.vtx_regp.ctrl, -1);
		if (errno) return CEN_MENU;

		b=0; i=0; dat=0;
		while (b < 5*175) {
			tmp=config.vtx_regp.glthr <<(16-5);
			dat |= (tmp >>(b-16*i));
//printf("gth %04X %04X\n", tmp, dat);
			b +=5;
			if (b >= 16*(i+1)) {
				config.vtx_reg[i++]=dat; dat=0;
				if (b != 16*i) dat=tmp <<(5-(b-16*i));
			}

         for (n=0; n < 32; n++) {
				tmp=config.vtx_regp.locthr <<(16-4);
				dat |= (tmp >>(b-16*i));
//printf("%3u %04X %04X\n", n, tmp, dat);
				b +=4;
				if (b >= 16*(i+1)) {
					config.vtx_reg[i++]=dat; dat=0;
					if (b != 16*i) dat=tmp <<(4-(b-16*i));
				}
			}

			c=b/175;
			tmp=(u_int)(config.vtx_regp.disable[c] >>16);
			dat |= (tmp >>(b-16*i));
			b +=16;
			if (b >= 16*(i+1)) {
				config.vtx_reg[i++]=dat; dat=0;
				if (b != 16*i) dat=tmp <<(16-(b-16*i));
			}

			tmp=(u_int)config.vtx_regp.disable[c];
			dat |= (tmp >>(b-16*i));
			b +=16;
			if (b >= 16*(i+1)) {
				config.vtx_reg[i++]=dat; dat=0;
				if (b != 16*i) dat=tmp <<(16-(b-16*i));
			}

			tmp=config.vtx_regp.glref <<(16-5);
			dat |= (tmp >>(b-16*i));
			b +=5;
			if (b >= 16*(i+1)) {
				config.vtx_reg[i++]=dat; dat=0;
				if (b != 16*i) dat=tmp <<(5-(b-16*i));
			}

			tmp=config.vtx_regp.ctrl <<(16-5);
			dat |= (tmp >>(b-16*i));
			b +=5;
			if (b >= 16*(i+1)) {
				config.vtx_reg[i++]=dat; dat=0;
				if (b != 16*i) dat=tmp <<(5-(b-16*i));

			}
		}

		if (i < 55) config.vtx_reg[i]=dat;
	}

	if (mode == 0) {
		defval=(!hv) ? va_ctrl0 : va_ctrl1;
		last_key=0;
		for (i=0; i < 55; i++) {
			if (last_key == CTL_D) { config.vtx_reg[i]=defval[i]; continue; }

			if (last_key == CTL_A) { config.vtx_reg[i]=config.vtx_reg[i-1]; continue; }

			c=(16*i)/175; b=(16*i) %175;
			printf("%2u: chip %u bit %3u %04X ", i, c +1, b +1, config.vtx_reg[i]);
			n=0; k=0;
			while ((n < 5) && ((vareg[n].pos +k*vareg[n].len) <= b)) {
				if (k < vareg[n].cnt) { k++; continue; }

				k=0; n++;
			}

			if (k) k--;
			else { n--; k=vareg[n].cnt; }

			l=sprintf(pbuf, "%s", vareg[n].txt);
			if (k) l +=sprintf(pbuf+l, "(%u)", k);
			l +=sprintf(pbuf+l, " ");
			printf("%-12s", pbuf);
			config.vtx_reg[i]=(u_int)Read_Hexa(config.vtx_reg[i], 0xFFFF);
			if (errno)  return CEN_MENU;

			if (last_key == CTL_D) config.vtx_reg[i]=defval[i];
		}
	}

	if (mode != 2) {
		for (i=0, b=0, c=0; c < 5; c++) {
			p=16*(i+1) -(b +5);
			if (p >= 0) dat =((config.vtx_reg[i] >>p) &0x1F);
			else dat = ((config.vtx_reg[i] <<(-p)) &0x1F)
						 |((config.vtx_reg[i+1] >>(16+p)) &0x1F);

			if (p <= 0) i++;
			printf(" %02X ", dat); b +=5;

			for (n=0; n < 32; n++) {
				if (!(n %4)) printf(" ");
				p=16*(i+1) -(b +4);
				if (p >= 0) dat =((config.vtx_reg[i] >>p) &0xF);
				else dat = ((config.vtx_reg[i] <<(-p)) &0xF)
							 |((config.vtx_reg[i+1] >>(16+p)) &0xF);

				if (p <= 0) i++;
				printf("%X", dat); b +=4;
			}

			p=16*(i+1) -(b +32);
			ldat= ((u_long)config.vtx_reg[i] <<(-p))
				  |((u_long)config.vtx_reg[i+1] <<(-p-16));
			i +=2; p +=32;
			if (p) ldat |=(config.vtx_reg[i] >>p);

			printf("  %08lX", ldat); b +=32;

			for (n=0; n < 2; n++) {
				p=16*(i+1) -(b +5);
				if (p >= 0) dat =((config.vtx_reg[i] >>p) &0x1F);
				else dat = ((config.vtx_reg[i] <<(-p)) &0x1F)
							 |((config.vtx_reg[i+1] >>(16+p)) &0x1F);

				if (p <= 0) i++;
				printf(" %02X", dat); b +=5;
			}

			Writeln();
		}
	}

//	inp_unit->ADR_SCALING=0;
	p=-1;
	if (!hv) {
		inp_unit->ADR_CR=6*VCR_MON;
		inp_unit->lv.ADR_REG_CR =0xF000 +5*175-1;
		for (i=0; i < 55; i++) inp_unit->lv.ADR_REG=config.vtx_reg[i];

		j=0;
		while ((gtmp=inp_unit->ADR_SR) &0x1000)
			if (++j >= 1000) {
				printf("busy time out\n");
				return CEN_PROMPT;
			}

		if (mode == 2) {
			// load twice to verify REGOUT
			inp_unit->lv.ADR_REG_CR =0xF000 +5*175-1;
			for (i=0; i < 55; i++) inp_unit->lv.ADR_REG=config.vtx_reg[i];

			j=0;
			while ((gtmp=inp_unit->ADR_SR) &0x1000)
				if (++j >= 1000) {
					printf("busy time out\n");
					return CEN_PROMPT;
				}

			// verify REGOUT
			for (i=0; i < 55; i++) {
				j=0;
				while (!(gtmp=inp_unit->ADR_SR) &0x4000)
				if (++j >= 10) {
					Writeln();
					printf("timeout reading REG\n"); return CEN_PROMPT;
				}

				dat=inp_unit->lv.ADR_REG;
				if ((p < 0) && (dat != config.vtx_reg[i])) p=i;
			}

			if (p < 0) return CE_PROMPT;

			printf("load LV VAREG error at word %u\n", p);
			return CEN_PROMPT;
		}

		Writeln();
		for (i=0; i < 55; i++) {
			j=0;
			while (!(gtmp=inp_unit->ADR_SR) &0x4000)
			if (++j >= 10) {
				Writeln();
				printf("timeout reading REG\n"); return CEN_PROMPT;
			}
			printf(" %04X", dat=inp_unit->lv.ADR_REG);
			if ((p < 0) && (dat != config.vtx_reg[i])) p=i;
		}

		Writeln();
		if (p < 0) return CE_PROMPT;
		printf("error at word %u\n", p);
		return CEN_PROMPT;
	}

	inp_unit->ADR_CR=7*VCR_MON;
	inp_unit->hv.ADR_REG_CR =0xF000 +5*175-1;
	for (i=0; i < 55; i++) inp_unit->hv.ADR_REG=config.vtx_reg[i];

	j=0;
	while ((gtmp=inp_unit->ADR_SR) &0x2000)
		if (++j >= 1000) {
			printf("busy time out\n");
			return CEN_PROMPT;
		}

	if (mode == 2) {
		// load twice to verify REGOUT
		inp_unit->hv.ADR_REG_CR =0xF000 +5*175-1;
		for (i=0; i < 55; i++) inp_unit->hv.ADR_REG=config.vtx_reg[i];

		j=0;
		while ((gtmp=inp_unit->ADR_SR) &0x2000)
			if (++j >= 1000) {
				printf("busy time out\n");
				return CEN_PROMPT;
			}

		// verify REGOUT
		for (i=0; i < 55; i++) {
			j=0;
			while (!(gtmp=inp_unit->ADR_SR) &0x8000)
			if (++j >= 10) {
				Writeln();
				printf("timeout reading REG\n"); return CEN_PROMPT;
			}

			dat=inp_unit->hv.ADR_REG;
				if ((p < 0) && (dat != config.vtx_reg[i])) p=i;
		}

		if (p < 0) return CE_PROMPT;

		printf("load HV VAREG error at word %u\n", p);
		return CEN_PROMPT;
	}

	Writeln();
	for (i=0; i < 55; i++) {
		j=0;
		while (!(gtmp=inp_unit->ADR_SR) &0x8000)
		if (++j >= 10) {
			Writeln();
			printf("timeout reading REG\n"); return CEN_PROMPT;
		}
		printf(" %04X", dat=inp_unit->hv.ADR_REG);
			if ((p < 0) && (dat != config.vtx_reg[i])) p=i;
	}

	Writeln();
	if (p < 0) return CE_PROMPT;
	printf(">>> error at word %u <<<\n", p);
	return CEN_PROMPT;
}
//===========================================================================
//										qdc_sample
//===========================================================================
//
static VTX_DATA	*sqd;
static VTX_GRP		*cha_unit;

static u_int	hdr;
static u_char	single;		// 1 = single step
									// 2 = single step
									// 3 = ignore errors
static u_int	strtd,endi;	// ADC data start, end(start information), end info
static u_int	imx=0,imn;

#define MVAL		0x2000
#define PRT_X		80
#define QPRT_Y		44
#define QPRT_Y0	45			// X axis

static u_char	prt_dis[PRT_X];

static void chg_help(void)
{
	printf(" L   lower edge of display window\n");
	printf(" u   upper edge of display window\n");
	printf(" a   adjust values for lower and upper edge\n");
	printf(" f   full range\n");
	printf(" F   toggle both channels on      => %4s\n", (sqd->bool &_XALL) ? "on" : "off");
	printf(" r   toggle raw data on           => %4s\n", (sqd->bool &_XRAW) ? "on" : "off");
	printf(" V   toggle verbose com data      => %4s\n", (sqd->bool &_XVEB) ? "on" : "off");
	printf(" z   toggle displaying zero line  => %4s\n", (sqd->bool &_XZRO) ? "on" : "off");
	printf(" S   shift timing left            => %4u\n", sqd->shift);
	printf(" p   compress factor for time axis=> %4u\n", sqd->comp);
	printf(" t   set trigger source           => %4u\n", config.vtx_trg);
	printf(" i   VATA timing                  => %4u\n", config.vtx_vtim);
	printf(" P   set pedestal                 => %04X\n", sqd->pedestal);
	printf(" T   set threshold                => %04X\n", sqd->threshold);
	printf(" N   set noise threshold          => %04X\n", sqd->noisethr);
}

static void seq_function(char key)
{
	u_int		i;
	u_int		cr=inp_unit->ADR_CR;

	inp_unit->ADR_CR=0;			// stop
	cha_unit->ADR_SWREG=0x0888;
	inp_unit->ADR_CR=cr|VCR_RUN;	// and run
	if ((key >= '0') && (key <= '2')) {
		config.vtx_mode=key-'0';
		cha_unit->ADR_SWREG=0x00C+config.vtx_mode;
		if (cha_unit->ADR_COUNTER[0] == 1) return;

		gotoxy(1,1);
		printf("seq counter 0: exp 1, is %u\n", cha_unit->ADR_COUNTER[0]);
		WaitKey(0);
		return;
	}

	switch (key) {
case '3':
		cha_unit->ADR_SWREG=0x0C0;
		cha_unit->ADR_SWREG=0x00F; break;

case '4':
		cha_unit->ADR_SWREG=0x0D0;
		cha_unit->ADR_SWREG=0x00F; break;

case '5':
		cha_unit->ADR_SWREG=0x0E0;
		cha_unit->ADR_SWREG=0x00F; break;

case '6':
		cha_unit->ADR_SWREG=0x0F0;
		cha_unit->ADR_SWREG=0x00F; break;

case '7':
		cha_unit->ADR_SWREG=0xC80;
		cha_unit->ADR_SWREG=0x00F; break;

case '8':
		gotoxy(1, 1);
		printf("repeat  %u ", config.vtx_varep);
		config.vtx_varep=(u_char)Read_Deci(config.vtx_varep, -1);
		cha_unit->ADR_LP_COUNTER[0]=config.vtx_varep;
		config.vtx_varep=(gtmp=cha_unit->ADR_LP_COUNTER[0]);
		cha_unit->ADR_SWREG=0xD80;
		cha_unit->ADR_SWREG=0x00F; break;

case '9':
		gotoxy(1, 1);
		printf("repeat  %u ", config.vtx_varep);
		config.vtx_varep=(u_char)Read_Deci(config.vtx_varep, -1);
		cha_unit->ADR_LP_COUNTER[0]=config.vtx_varep;
		config.vtx_varep=(gtmp=cha_unit->ADR_LP_COUNTER[0]);
		cha_unit->ADR_SWREG=0xE80;
		cha_unit->ADR_SWREG=0x00F; break;

default:
		gotoxy(1, 1);
		printf("unknown command %c", key);
		WaitKey(0);
		return;

	}

	i=0;
	while (cha_unit->ADR_COUNTER[0] != 2)
		if (++i >= 100) {
			gotoxy(1,1);
			printf("seq counter 0: exp 2, is %u\n", cha_unit->ADR_COUNTER[0]);
			WaitKey(0);
			break;
		}
}

static int chg_config(u_char rkey)	// 1: do smp_adjust(0);
{
	u_int		i;
	u_char	key=toupper(rkey);

	if (rkey == 't') {
		printf("%25s\r", "");
		printf("trigger src  %u ", config.vtx_trg);
		config.vtx_trg=(u_char)Read_Deci(config.vtx_trg, -1) &3;
		return 1;
	}

	if (key == 'R') {
		sqd->bool ^=_XRAW;
		return 1;
	}

	if (rkey == 'V') {
		sqd->bool ^=_XVEB;
		return 1;
	}

	if (rkey == 'F') {
		if (sqd->bool &_XALL) sqd->bool &=~_XALL;
		else sqd->bool |=_XALL;

		return 1;
	}

	if (rkey == 'L') {
		printf("%25s\r", "");
		printf("lower lim  %04X ", sqd->a_min);
		sqd->a_min=(u_int)Read_Hexa(sqd->a_min, -1);
		return 1;
	}

	if (key == 'U') {
		printf("%25s\r", "");
		printf("upper lim  %04X ", sqd->a_max);
		sqd->a_max=(u_int)Read_Hexa(sqd->a_max, -1);
		return 1;
	}

	if (rkey == 'S') {
		printf("%25s\r", "");
		printf("shift     %4u. ", sqd->shift);
		sqd->shift=(u_int)Read_Deci(sqd->shift, -1);
		return 1;
	}

	if (rkey == 'p') {
		printf("%25s\r", "");
		printf("compress  %4u. ", sqd->comp);
		sqd->comp=(u_int)Read_Deci(sqd->comp, -1);
		return 1;
	}

	if (key == 'Z') {
		sqd->bool ^=_XZRO;
		return 1;
	}

	if (rkey == 'P') {
		printf("%25s\r", "");
		printf("pedestal  %04X ", sqd->pedestal);
		sqd->pedestal=(u_int)Read_Hexa(sqd->pedestal, -1);
		inp_unit->ADR_CR=0;

		inp_unit->ADR_RAMT_ADR=(config.vtx_cha == 0) ? 0 : BLOCK1_HV;
		for (i=0; i < 200; i++) inp_unit->ADR_RAMT_DATA=sqd->pedestal;
		return 1;
	}

	if (rkey == 'T') {
		printf("%25s\r", "");
		printf("threshold  %04X ", sqd->threshold);
		sqd->threshold=(u_int)Read_Hexa(sqd->threshold, -1);
		inp_unit->ADR_CR=0;

		inp_unit->ADR_RAMT_ADR=(config.vtx_cha == 0) ? 0x1000 : BLOCK1_HV+0x1000;
		for (i=0; i < 200; i++) inp_unit->ADR_RAMT_DATA=sqd->threshold;
		return 1;
	}

	if (key == 'I') {
		printf("%25s\r", "");
		printf("VATA timing %u ", config.vtx_vtim);
		config.vtx_vtim=(u_int)Read_Deci(config.vtx_vtim, 9);
		inp_unit->ADR_CR=0; i=fname[3];
		fname[3]='0'+config.vtx_vtim;
		if (seq_load(inp_unit, fname, 0) != 0) {
			fname[3]=i; return 1;
		}

		seq_load(inp_unit, fname, 1);
		return 1;
	}

	if (key == 'N') {
		printf("%25s\r", "");
		printf("noise thrh  %04X ", sqd->noisethr);
		sqd->noisethr=(u_int)Read_Hexa(sqd->noisethr, -1);
		return 1;
	}

	if (rkey == 'a') {
		if ((imx-imn) < 2*(QPRT_Y-2)) {
			sqd->a_min=(imx+imn)/2 -QPRT_Y;
			sqd->a_max=imx;
		} else {
			sqd->a_min=imn-(imx-imn)/20;
			if (sqd->a_min >= MVAL) sqd->a_min=0;
			sqd->a_max=imx+(imx-imn)/20;
			if (sqd->a_max >= MVAL) sqd->a_max=MVAL;
		}

		return 1;
	}

	if (rkey == 'f') {
		if ((sqd->bool &_XRAW) || (imn >= MVAL/2)) {
			sqd->a_min=MVAL/2;
			sqd->a_max=MVAL;
			return 1;
		}

		if (imx <= MVAL/2) {
			sqd->a_min=0;
			sqd->a_max=MVAL/2;
			return 1;
		}

		sqd->a_min=0;
		sqd->a_max=MVAL;
		return 1;
	}

	return 0;
}

static void cmd_string(char *str)
{
	u_int	i;

	i=0;
	while (str[i]) {
		if (str[i] == '_') {
			i++;
			if (!str[i]) break;

			highvideo(); cprintf("%c", str[i++]); lowvideo();
			continue;
		}

		cprintf("%c", str[i++]);
	}
}

static void smp_adjust(void)
{
	u_int		i;
	u_int		tmp;

	if (config.vtx_cha >= 2) config.vtx_cha=0;

	sqd=config.vtx_data +config.vtx_cha;
	cha_unit=(config.vtx_cha == 0) ? &inp_unit->lv : &inp_unit->hv;

	clrscr();
	gotoxy(1, QPRT_Y0);
	for (i=0; i < 80;) {
		if (i == 0) { printf("%X", sqd->shift &0xF); i++; continue; }

		if ((i&3) == 1) { printf("--"); i+=2; continue; }

		if (i == 79) {
			printf("%X", ((sqd->shift +(i+1)*(sqd->comp+1)) >>4) &0xF);
			i++; continue;
		}

		printf("%02X", (sqd->shift +(i+1)*(sqd->comp+1)) &0xFF);
		i+=2;
	}

	for (i=0; i < PRT_X; prt_dis[i++]=255);

	if ((sqd->a_max < sqd->a_min) || ((sqd->a_max-sqd->a_min) < 2*QPRT_Y)) {
		sqd->a_max =sqd->a_min+2*QPRT_Y;
		if (sqd->a_max >= MVAL) { sqd->a_max=MVAL; sqd->a_min=MVAL-2*QPRT_Y; }
	}

	inp_unit->ADR_CR=0;
	inp_unit->ADR_DIG_TST=config.vtx_digtst;
	config.vtx_digtst=inp_unit->ADR_DIG_TST;
	inp_unit->ADR_ACLK_PAR=config.vtx_cpar;
	config.vtx_cpar=inp_unit->ADR_ACLK_PAR;
	inp_unit->ADR_SEQ_CSR=0;
	cha_unit->ADR_SWREG=0x88;
	cha_unit->ADR_CLK_DELAY=sqd->clkdelay;
	sqd->clkdelay=cha_unit->ADR_CLK_DELAY;
	cha_unit->ADR_NOISE_THR=sqd->noisethr;
	sqd->noisethr=cha_unit->ADR_NOISE_THR;
	gotoxy(1, QPRT_Y0+3);
	if (config.vtx_cha) cmd_string("H_v"); else cmd_string("L_v");
	cmd_string(", _Lo/_up limit=");
	printf("%04X/%04X", sqd->a_min, sqd->a_max);
	cmd_string(", _Shi=");
	printf("%u.", sqd->shift);
	cmd_string(", c_pr=");
	printf("%2u.", sqd->comp);
	cmd_string(", _dAC=");
	printf("%04X", sqd->dac0);
	cmd_string(", d_elay=");
	printf("%04X", cha_unit->ADR_CLK_DELAY);
	cmd_string(", _cpar=");
	printf("%04X", config.vtx_cpar);
	printf("    ");

	gotoxy(1, QPRT_Y0+4);
	cmd_string("_raw=");
	printf((sqd->bool &_XRAW) ? "T" : "F");
	if (!(sqd->bool &_XRAW)) {
		cmd_string(", _Ver=");
		printf((sqd->bool &_XVEB) ? "T" : "F");
	}
	cmd_string(", _Adat=");
	printf((sqd->bool &_XCDA) ? "T" : "F");
	cmd_string(", _Fcha=");
	printf((sqd->bool &_XALL) ? "T" : "F");
	gotoxy(1, QPRT_Y0+5);
	cmd_string("press _h for help     ");

	cha_unit->ADR_POTI =sqd->dac0;

	tmp=VCR_TM_MODE|VCR_RUN;
	if (sqd->bool&_XRAW) tmp |=VCR_RAW;
	if (sqd->bool &_XVEB) tmp |=VCR_VERB;
	if (!(sqd->bool &_XALL)) {
		if (config.vtx_cha == 0) tmp |=VCR_IH_HV; else tmp |=VCR_IH_LV;
	}

	if (config.vtx_cha) tmp |=VCR_MON;

	tmp |=(config.vtx_trg*VCR_IH_TRG);
	inp_unit->ADR_CR=(config.vtx_mon*(VCR_MON<<1)) |tmp;
	cha_unit->ADR_SWREG=0x00C+config.vtx_mode;
	if (sqd->bool &_XALL)
		if (config.vtx_cha == 0) inp_unit->hv.ADR_SWREG=0x00C+config.vtx_mode;
		else inp_unit->lv.ADR_SWREG=0x00C+config.vtx_mode;

#ifdef PBUS
	config.daq_mode =(config.vtx_trg) ? EVF_ON_TP : 0;
	config.rout_delay=100;
	inp_mode(IM_QUIET);
#else
	if (!config.vtx_trg) config.daq_mode=EVF_HANDSH|MCR_DAQTRG;
	else config.daq_mode=EVF_ON_TP|EVF_HANDSH|MCR_DAQTRG;

	inp_mode(IM_QUIET);
	kbatt_msk=0; tstevc=0;
#endif
	hdr=rout_hdr ? HDR_LEN : 0;
}

// -------------------------- qdc_sample --------------------------------- --

static int qdc_sample(void)
{
	int		ret;
	u_int		i,n,tmp;
	u_long	ltmp,ltmpx;
	u_char	rkey=0,key;
	u_char	dis;
	u_char	mdis;
	u_char	con;
	u_int		cha,icha;
	u_int		imx_cha=0,imn_cha=0;
	u_int		trgmsk=0;
	u_int		commode,comvalues;

	inp_unit->ADR_CR=0;
	fname[3]='0'+config.vtx_vtim;
	if ((ret=seq_load(inp_unit, fname, 0)) != 0) {
		config.vtx_vtim=0; fname[3]='0';
		if ((ret=seq_load(inp_unit, fname, 0)) != 0) return ret;
	}

	if ((ret=seq_load(inp_unit, fname, 1)) != 0) return ret;

	inp_unit->ADR_RAMT_ADR=0;
	for (i=0; i < 200; i++) inp_unit->ADR_RAMT_DATA=config.vtx_data[0].pedestal;

	inp_unit->ADR_RAMT_ADR=BLOCK1_HV;
	for (i=0; i < 200; i++) inp_unit->ADR_RAMT_DATA=config.vtx_data[1].pedestal;

	inp_unit->ADR_RAMT_ADR=0x1000;
	for (i=0; i < 200; i++) inp_unit->ADR_RAMT_DATA=config.vtx_data[0].threshold;

	inp_unit->ADR_RAMT_ADR=BLOCK1_HV+0x1000;
	for (i=0; i < 200; i++) inp_unit->ADR_RAMT_DATA=config.vtx_data[1].threshold;

	vtx_ldac(0, 0);
	vtx_ldac(1, 0);

	if (vtx_lreg(0, 2) == CEN_PROMPT) WaitKey(0);
	if (vtx_lreg(1, 2) == CEN_PROMPT) WaitKey(0);

	use_default=0;
	single=0;
	rout_len=0; endi=0;
	con=1;
	for (i=0; i < 16; i++) inp_onl[i] =(i == sc_conf->slv_sel);
	smp_adjust();
	while (1) {
		if (con) { _setcursortype(_NOCURSOR); con=0; }

		if ((ret=read_event(EVF_NODAC, RO_RAW)) != 0) {
			_setcursortype(_NORMALCURSOR); con=1;
			if (!(ret &CE_KEY_PRESSED)) {
				gotoxy(1, QPRT_Y0+5);
				lowvideo(); cprintf(" \r");
				return ret;
			}

			if (ret &CE_KEY_SPECIAL) {
				rkey=(u_char)ret;
				continue;
			}

			use_default=0;
			key=toupper(rkey=(char)ret);
			if (rkey == ESC) {
				gotoxy(1, QPRT_Y0+5);
				lowvideo(); cprintf(" \r");
				return CE_PROMPT;
			}

			gotoxy(1, QPRT_Y0+1);
			if ((key == '?') || (key == 'H')) {
				clrscr();
				gotoxy(1, 10);
				printf(" CR  Information: min, max and diff value of display\n");
				printf(" s   step\n");
				printf(" v   toggle HV                    => %4s\n", (config.vtx_cha) ? "on" : "off");
				printf(" A   toggle display commod data   => %4s\n", (sqd->bool &_XCDA) ? "on" : "off");
				printf(" c   set ADC clock parameter      => %04X %uns\n", config.vtx_cpar, (((config.vtx_cpar>>8)&0x3F)+(config.vtx_cpar&0x3F)+2)*10);
				printf(" e   change VA clock delay        =>  %03X %u/%uns\n", sqd->clkdelay, ((sqd->clkdelay>>8)+1)*10, ((u_char)sqd->clkdelay+1)*10);
				printf(" d   set offset (DAC) level       => %04X\n", sqd->dac0);
				printf(" D   length of DigTstP            =>  %03X %uns\n", config.vtx_digtst, (config.vtx_digtst+1)*10);
				printf(" m   set monitoring source        => %4u\n", config.vtx_mon);
				printf(" 0-4 call sequencer function (RstA,Rst,Cha0,Cha0All,VAclk)\n");
				chg_help();
				WaitKey(0);
				smp_adjust();
				continue;
			}

			if (key == CR) {
				gotoxy(1, QPRT_Y0+2);
				printf("min/max(dif)tmsk=%03X,%02X/%03X,%02X(%u.)%02X",
						 imn-0x1000, imn_cha, imx-0x1000, imx_cha, (imx-imn), trgmsk);
				printf("; len=%04X    ", endi);
				continue;
			}

			if (rkey == 'v') {
				config.vtx_cha =!config.vtx_cha;
				smp_adjust();
				continue;
			}

			if (rkey == 's') {
				single=1;
				continue;
			}

			if (rkey == ' ') {
				if (single == 2) single=0; else single=2;
				continue;
			}

			if (rkey == 'A') {
				sqd->bool ^=_XCDA;
				smp_adjust();
				continue;
			}

			if (key == 'C') {
				printf("%25s\r", "");
				printf("clkpar  %04X ", config.vtx_cpar);
				config.vtx_cpar=(u_int)Read_Hexa(config.vtx_cpar, -1);
				smp_adjust();
				continue;
			}

			if (rkey == 'd') {
				printf("%25s\r", "");
				printf("DAC    %04X ", sqd->dac0);
				sqd->dac0=(u_int)Read_Hexa(sqd->dac0, -1);
				smp_adjust();
				continue;
			}

			if (rkey == 'D') {
				printf("%25s\r", "");
				printf("len DigTstP  %04X ", config.vtx_digtst);
				config.vtx_digtst=(u_int)Read_Hexa(config.vtx_digtst, -1);
				smp_adjust();
				continue;
			}

			if (key == 'E') {
				printf("%25s\r", "");
				sqd->clkdelay=cha_unit->ADR_CLK_DELAY;
				printf("clk delay %03X(%u) ", sqd->clkdelay, (u_char)sqd->clkdelay);
				sqd->clkdelay=(u_int)Read_Hexa(sqd->clkdelay, -1);
				cha_unit->ADR_CLK_DELAY=sqd->clkdelay;
				continue;
			}

			if (key == 'M') {
				printf("%25s\r", "");
				printf("monitoring  %u ", config.vtx_mon);
				config.vtx_mon=(u_int)Read_Deci(config.vtx_mon, -1);
				inp_unit->ADR_CR=(inp_unit->ADR_CR &~(6*VCR_MON)) |(config.vtx_mon*(VCR_MON<<1));
				continue;
			}

			if ((key >= '0') && (key <= '9')) {
				seq_function(key);
				smp_adjust();
				continue;
			}

			if (chg_config(rkey) == 1) smp_adjust();

			continue;
		}

// ========= data computing =========

//gotoxy(1,QPRT_Y0+1);
//printf("%04X %08lX %08lX", rout_len, dmabuf[0][0], dmabuf[0][1]);
//if (WaitKey(0)) return CE_PROMPT;
		if((gtmp=inp_unit->ADR_SR) &1) {
			gotoxy(1, 1);
			printf("fat err, sr=%04X, seq=%04X\n", gtmp, inp_unit->ADR_SEQ_CSR);
#if 0
			if (WaitKey(0)) {
				gotoxy(1, QPRT_Y0+5);
				lowvideo(); cprintf(" \r");
				return CE_PROMPT;
			}
#endif
		}

		while (1) {		// WHILE_1

			strtd=hdr; endi=rout_len/4;
			if (strtd == endi) break;
#if 0
			if (single <= 1) {
				if ((config.vtx_mode < 2) && (sqd->bool &_XRAW)) {
					tmp=(sqd->bool &_XALL) ? 2*0xA1 : 0xA1;
					if (   (endi-strtd != tmp)				// wrong data length
						 && ((config.vtx_trg != 2) || (endi-strtd != 0xA1))) {
						gotoxy(1, 1);
						printf("rlen=%04X, l=%04X exp %04X (TAB=ignore) ",
								 rout_len, endi-strtd, tmp);
						_setcursortype(_NORMALCURSOR); con=1;
						key=toupper(getch());
						if (key == ESC) {
							gotoxy(1, QPRT_Y0+5);
							lowvideo(); cprintf(" \r");
							return CE_PROMPT;
						}

						if (key == TAB) single=3;
					}
				}

			}
#endif
			imx=0; imn=0xFFFF; mdis=0xFF;
			if (sqd->bool &_XZRO) {
				tmp=0x1000;	// inp_unit->hv.ADR_COMVAL;
				if ((tmp > sqd->a_min) && (tmp <= sqd->a_max))
					mdis=(u_int)( ( (u_long)2*QPRT_Y*(tmp-sqd->a_min)
										  +(sqd->a_max-sqd->a_min)/2)
										/(sqd->a_max-sqd->a_min)
									  );
			}

//	-------------------------- display curve	----------------------------- --

			n=0; i=strtd; cha=sqd->shift; icha=0; trgmsk=0; commode=0; comvalues=0;
			while ((n < PRT_X) && (i < endi)) {		// WHILE_0
				u_int		j,k;
				u_int		ldis;		// last position
				u_int		ovr;

				ltmp=dmabuf[0][i];
				if (((u_int)(ltmp>>27)&1) != config.vtx_cha) { i++; continue; }

				if (ltmp &0x04000000L) {	// extra word
					switch ((u_int)(ltmp>>24) &0x3) {
				case 0:	// trigger mask
						trgmsk |=(u_int)ltmp;
						if (sqd->bool &_XRAW) {
							gotoxy(68, 1); printf("tmsk  %3u|%02X", cha, (u_int)ltmp);
						}
						break;

				case 1:	// com mode
						commode =(u_int)ltmp;
						break;

				case 2:	// com values
						comvalues =(u_int)ltmp;
						break;

					}

					i++; continue;
				}


//gotoxy(1,1);
//printf("n=%u cha=%u %u\n", n, cha, (u_int)(ltmp>>16)&0xFF);
//WaitKey(0);
				ltmpx=0; k=0;
				if (config.vtx_mode < 2) icha=(u_int)(ltmp>>16)&0xFF;
				if (cha > icha) {
//gotoxy(1,1); WaitKey('0');
					i++; icha++; continue;
				}

				if (cha == icha) {
					i++; ovr=0;
					tmp=(u_int)ltmp;
					if (sqd->bool &_XRAW) {
						if (tmp &0x1000) ovr=1;

						tmp &=0xFFF;
					}

					tmp +=0x1000;	// zero to the middle
					if (tmp > imx) { imx=tmp; imx_cha=cha; }
					if (tmp < imn) { imn=tmp; imn_cha=cha; }
					ltmpx=tmp; k=1;
				}

				j=1; cha++; icha++;
				while ((j <= sqd->comp) && (i < endi)) {
					ltmp=dmabuf[0][i];
					if (config.vtx_cha == 0) {
						if (ltmp &0x08000000L) { i++; continue; }
					} else {
						if (!(ltmp &0x08000000L)) { i++; continue; }
					}

					j++;
					if (config.vtx_mode < 2) icha=(u_int)(ltmp>>16)&0xFF;
					if (cha != icha) { cha++; icha++; continue; }

					i++;
					tmp=(u_int)ltmp;
					if (sqd->bool &_XRAW) {
						if (tmp &0x1000) ovr=1;

						tmp &=0xFFF;
					}

					tmp +=0x1000;	// zero to the middle
					if (tmp > imx) { imx=tmp; imx_cha=cha; }
					if (tmp < imn) { imn=tmp; imn_cha=cha; }
               cha++; icha++; k++; 
				}

				if (!k) {
					ldis=prt_dis[n];
					prt_dis[n]=255;
					if (ldis != 255) {	// clear last point
						gotoxy(n+1, QPRT_Y0-(ldis/2));
						if (ldis/2) printf(" "); else printf("-");
					}
				} else {
					tmp=(u_int)(ltmpx/k);
					if (ovr) {		// only raw mode
						if (tmp > 0x1800) dis=2*QPRT_Y -1; else dis=0;
					} else {
						if (tmp <= sqd->a_min) dis=0;
						else {
							if (tmp > sqd->a_max) dis=2*QPRT_Y -1;
							else dis=(u_int)(
										 ((u_long)2*QPRT_Y*(tmp-sqd->a_min))	// +(sqd->a_max-sqd->a_min)/2
										 /(sqd->a_max-sqd->a_min)
												 );
						}
					}

					ldis=prt_dis[n];
					prt_dis[n]=dis;
					if ((ldis != dis) || ovr) {
						if (ldis != 255) {		// clear last point
							gotoxy(n+1, QPRT_Y0-(ldis/2));
							if (ldis/2) printf(" "); else printf("-");
						}

						gotoxy(n+1, QPRT_Y0-dis/2);
						if (ovr) { printf("!"); prt_dis[n]=254; }
						else printf("%c", (dis&1) ? 'ß' : 'Ü');
					}
				}

				if (mdis == 0xFF) { n++; continue; }	// no zero line
#if 0
	gotoxy(1, QPRT_Y0+5);
	printf(" %3u %2u %2u %2u ", prt_mdis, mdis, x, dis);
	_setcursortype(_NORMALCURSOR); con=1;
	if (WaitKey('1')) return 0;
#endif
				if (!k || (mdis != dis)) {
					gotoxy(n+1, QPRT_Y0-(mdis/2));
					printf("%c", (k&&(mdis/2==dis/2)) ? 'Û'
																 : (mdis&1) ? 'ß' : 'Ü');
				}

				n++;
			}	// WHILE_0

			for (; (n < PRT_X); n++) {
				if (prt_dis[n] != 255) {
					gotoxy(n+1, QPRT_Y0-(prt_dis[n]/2));
					if (prt_dis[n]/2) printf(" "); else printf("-");
					prt_dis[n]=255;
				}

				if (mdis != 0xFF) {
					gotoxy(n+1, QPRT_Y0-(mdis/2));
					printf("%c", (mdis&1) ? 'ß' : 'Ü');
				}
			}

			if (!(sqd->bool &_XRAW) && (sqd->bool &_XCDA)) {
				gotoxy(70, 1); printf("tmsk  %02X", trgmsk);
				if (sqd->bool &_XVEB) {
					gotoxy(70, 2); printf("cmod%4d", commode);
					gotoxy(70, 3); printf("ccnt%4d", comvalues);
				} else {
					gotoxy(70, 2); printf("cmod%4d", cha_unit->ADR_COMVAL);
					gotoxy(70, 3); printf("ccnt%4d", cha_unit->ADR_COMCNT);
				}
			}

//	---	---	---	---	---

			gotoxy(1, QPRT_Y0+1);
			if (single == 1) {
				_setcursortype(_NORMALCURSOR); con=1;
				while (1) {
					key=toupper(rkey=getch());
					if ((ret=chg_config(rkey)) == 2) continue;
					break;
				}

				if (ret == 1) smp_adjust();

				if (ret == 0) {
					if (rkey == ESC) {
						gotoxy(1, QPRT_Y0+5);
						lowvideo(); cprintf(" \r");
						return CE_PROMPT;
					}

					if (key != 'S') single=0;
				}
			}	// (single == 1)

			if (single == 2) {	// SINGLE_2

				gotoxy(1, 1);
				_setcursortype(_NORMALCURSOR); con=1;
				while (1) {
					key=toupper(rkey=getch());
					if ((key == 'H') || (key == '?')) {
						gotoxy(1, 1);
						printf(" CTL/RIGHT shift\n");
						chg_help();
						continue;
					}

					if (rkey != 'R') break;

				}

				if (rkey == 0) {
					rkey=getch();
					switch (rkey) {
				case RIGHT:
						sqd->shift +=(16*(sqd->comp+1));
						break;

				case LEFT:
						if (sqd->shift < (16*(sqd->comp+1))) sqd->shift=0;
						else sqd->shift -=(16*(sqd->comp+1));
						break;

				case CTLRIGHT:
						sqd->shift +=(64*(sqd->comp+1));
						break;

				case CTLLEFT:
						if (sqd->shift < (64*(sqd->comp+1))) sqd->shift=0;
						else sqd->shift -=(64*(sqd->comp+1));
						break;

				default:
						single=0;
						break;
					}

					if (!single) break;	// WHILE_1

					smp_adjust();
					continue;				// WHILE_1
				}

				if ((ret=chg_config(rkey)) != 0) {
					if (ret == 1) smp_adjust();
					continue;
				}

				if (rkey == ESC) {
					gotoxy(1, QPRT_Y0+5);
					lowvideo(); cprintf(" \r");
					return CE_PROMPT;
				}

				if (key == ' ') rout_len=0; else single=0;

				if (key == TAB) { rout_len=0; single=3; }

				smp_adjust();

			}	// SINGLE_2

			break;
		}	// WHILE_1
	}
}
//
//===========================================================================
//										vtx_data
//===========================================================================
//
static int vtx_data(void)
{
	int		ret;
	u_int		i;
	u_int		l;

	for (i=0; i < 16; i++) inp_onl[i] =(i == sc_conf->slv_sel);

	if (!((gtmp=inp_bc->ADR_FIFO) &(1<<sc_conf->slv_sel))) {
		printf("no data\n"); return CE_PROMPT;
	}

#ifdef PBUS
	config.daq_mode =EVF_ON_TP;
	config.rout_delay=100;
	inp_mode(IM_QUIET);
#else
	config.daq_mode=EVF_ON_TP|EVF_HANDSH|MCR_DAQTRG;
	config.rout_delay=0xFF;
	inp_mode(IM_QUIET);
	kbatt_msk=0; tstevc=0;
#endif

	if ((ret=read_event(EVF_NODAC, RO_RAW)) != 0) return ret;

	l=rout_len/4;
	printf("%04X/%04X\n", rout_len, l);
	for (i=0; (i < l); i++) {
		if (!(i &0x7)) {
			if (i) {
				Writeln();
				if (!(i&0xFF)) {
					if (WaitKey(0)) break;
				}
			}

			printf("%04X:", 4*i);
		}

		printf(" %08lX", dmabuf[0][i]);
	}

	Writeln();
	return CE_PROMPT;
}
//
// ==========================================================================
//										vertex_menu
// ==========================================================================
//
static const char *cr_txt[] ={
	"Run", "Raw", "Vb", "?", "iLV", "iHV", "iTg", "Tou",
	"mHV", "m0",  "m1", "?", "?",   "?",   "?",   "?"
};

static const char *sr_txt[] ={
	"Fat", "ROe", "?",   "?",   "?",   "?",   "DQl", "DQh",
	"PTl", "PTh", "DCl", "DCh", "VOl", "VOh", "VIl", "VIh"
};

static const char *sqcsr_txt[] ={
	"Hlt", "Hltd", "ofl", "ufl", "opc", "nda", "?", "?"
};

//--------------------------- jt_inp_putcsr ---------------------------------
//
static void jt_inp_putcsr(u_int val)
{
	inp_unit->ADR_JTAG_CSR = val;
	inp_unit->ADR_IDENT    = 0;			// nur delay
}

//--------------------------- jt_inp_getcsr ---------------------------------
//
static u_int jt_inp_getcsr(void)
{
	return inp_unit->ADR_JTAG_CSR;
}

//--------------------------- jt_inp_getdata --------------------------------
//
static u_long jt_inp_getdata(void)
{
	return inp_unit->ADR_JTAG_DATA;
}
//
//--------------------------- vertex_menu -----------------------------------
//
int vertex_menu(void)
{
	int		ret=0;
	u_int		i,tmp;
	char		hgh;
	char		rkey,key;
	u_char	dot;
	char		hd=1;
	char		hv;
	char		*lhv;

static u_int	z_pat=0xA,z_bit=5,z_del=15,z_scl=4;

	if (!sc_conf) return CE_PARAM_ERR;

	for (i=0; i < 16; i++) inp_onl[i] =0;

	hv=(config.vtx_cha != 0);
	lhv=(!hv) ?  "LV" : "HV";
	cha_unit=(!hv) ? &inp_unit->lv : &inp_unit->hv;

	while (1) {
		while (kbhit()) getch();

		if (BUS_ERROR) CLR_PROT_ERROR();

		if (ret > CE_PROMPT) display_errcode(ret);

		if ((ret <= CEN_PROMPT) || (ret >= CE_PROMPT)) hd=0;

		errno=0; ret=0; use_default=0; dot=0;
		if (hd) {
			Writeln();
			inp_assign(sc_conf, 1);
			if (inp_bc->ADR_CARD_OFFL)
				printf("not all Card online        %04X\n", inp_bc->ADR_CARD_OFFL);

			printf("online Input Cards         %04X\n", inp_bc->ADR_CARD_ONL);
			if (inp_bc->ADR_FIFO)
				printf("FIFO Data                  %04X     <===>\n", inp_bc->ADR_FIFO);
			if (inp_bc->ADR_ERROR)
				printf("ERROR Bit                  %04X     <===>\n", inp_bc->ADR_ERROR);
			if (inp_bc->ADR_TRIGGER_L)
				printf("BUSY Bit                   %04X     <===>\n", inp_bc->ADR_TRIGGER_L);
			printf("assign input Bus                ..B\n");
			printf("\n");
			if (inp_conf) {
				printf("Karten Adresse (addr %2u)   %4u ..U\n", inp_conf->slv_addr, sc_conf->slv_sel);
				printf("ident/serial               %04X/%u\n", inp_unit->ADR_IDENT, inp_unit->ADR_SERIAL);
				if (inp_conf->type != VERTEX) {
					printf("das ist keine VERTEX Karte!!!\n");
					return CE_NO_INPUT;
				}

			printf("Control                    %04X ..0 ", tmp=inp_unit->ADR_CR);
			hgh=0;
			for (i=16; i != 0;) {
				i--;
				if (tmp &(0x1 <<i)) {
					if (hgh) highvideo(); else lowvideo();
					hgh=!hgh; cprintf("%s", cr_txt[i]);
				}
			}
			lowvideo(); cprintf("\r\n");

			printf("Status                     %04X     ", tmp=inp_unit->ADR_SR);
			hgh=0;
			for (i=16; i != 0;) {
				i--;
				if (tmp &(0x1 <<i)) {
					if (hgh) highvideo(); else lowvideo();
					hgh=!hgh; cprintf("%s", sr_txt[i]);
				}
			}
			lowvideo(); cprintf("\r\n");

			printf("ADC clock parameter        %04X ..1\n", inp_unit->ADR_ACLK_PAR);
			printf("Sequencer CSR              %04X ..2 ", tmp=inp_unit->ADR_SEQ_CSR);
			hgh=0;
			for (i=8; i != 0;) {
				i--;
				if (tmp &(0x1 <<i)) {
					if (hgh) highvideo(); else lowvideo();
					hgh=!hgh; cprintf("%s", sqcsr_txt[i]);
				}
			}
			lowvideo(); cprintf("\r\n");

			printf("%s poti(ADC offset)        %04X ..3\n", lhv, inp_conf->slv_data.s[3]);
			printf("%s ADC sample              %04X\n", lhv, cha_unit->ADR_ADC_VALUE);
			printf("%s VA DAC                  %04X ..4\n", lhv, inp_conf->slv_data.s[4]);
			printf("%s VA reg lenght           %04X ..5\n", lhv, cha_unit->ADR_REG_CR);
			printf("%s VA reg                  %04X ..6/7\n", lhv, inp_conf->slv_data.s[6]);
			printf("%s clock delay             %04X ..8\n", lhv, cha_unit->ADR_CLK_DELAY);
			printf("%s num of chan             %04X ..9\n", lhv, cha_unit->ADR_NR_CHAN +1);
			printf("%s commod thresh           %04X ..a\n", lhv, cha_unit->ADR_NOISE_THR);
			printf("%s commod/comcnt      %04X/%04X\n", lhv, cha_unit->ADR_COMVAL, cha_unit->ADR_COMCNT);
			printf("%s Software Trigger    %08lX ..b\n", lhv, cha_unit->ADR_SWREG);
			printf("%s Counter  %04X/%04X/%04X/%04X ..c/d\n", lhv,
				cha_unit->ADR_COUNTER[0], cha_unit->ADR_COUNTER[1], cha_unit->ADR_LP_COUNTER[0], cha_unit->ADR_LP_COUNTER[1]);
			printf("serial clock scaling       %04X ..e\n", inp_unit->ADR_SCALING);
			printf("length of dig testp        %04X ..f\n", inp_unit->ADR_DIG_TST);
			printf("action 8=TP .1=MRST        %04X ..g\n", inp_conf->slv_data.s[16]);

			printf("chg LV/HV, Sequencer menu       ..v,s\n");
			printf("JTAG utilities                  ..j\n");
			printf("set DAC/VATA reg/com            ..D/r/R\n");
			printf("Get Data/tst VA reg/tst VA DAC  ..q/z/Z\n");
			if (BUS_ERROR) { CLR_PROT_ERROR(); Writeln(); }

		  }

			printf("                                  %c ",	config.i3_menu);
			while (1) {
				rkey=getch();
				if ((rkey == CTL_C) || (rkey == ESC)) return 0;

				if (rkey != '.') break;

				dot=1;
			}

			if (rkey == ' ') { Writeln(); continue; }
		} else {
			hd=1; printf("select> ");
			while (1) {
				rkey=getch();
				if (rkey == CTL_C) return 0;

				if (rkey != '.') break;

				dot=1;
			}

			if (rkey == ' ') rkey=config.i3_menu;

			if ((rkey == ESC) || (rkey == CR)) continue;
		}

		use_default=(rkey == TAB);
		if (use_default || (rkey == CR)) rkey=config.i3_menu;
		if (rkey > ' ') printf("%c", rkey);

		key=toupper(rkey);
		Writeln();
		if (rkey=='B') {
			inp_assign(sc_conf, 0);
			config.i3_menu=rkey;
			continue;
		}

		if (!inp_unit) continue;

		if (rkey == 'X') {
			u_long	*xp=(u_long*)&inp_unit->ADR_RAMT_DATA;
			u_long	x;
			u_long	i=0;

			config.i3_menu=rkey;
			inp_unit->ADR_RAMT_ADR =0;
			*xp =0xFFFFL;
			hd=1;
			while (1) {
				inp_unit->ADR_RAMT_ADR =0;
				if ((x=*xp) != 0xFFFFL) {
					printf("\r%4u: error %08lX\n", (u_int)i, x);
					hd=0;
					if (!use_default || kbhit()) break;
				}

				if (!(++i &0xFFFF)) {
					printf("\r%4u", (u_int)(i>>16));
					if (kbhit()) break;
				}
			}

			continue;
		}

		if (rkey == 'x') {
			u_short	x;
			u_long	i=0;

			config.i3_menu=rkey;
			inp_unit->ADR_RAMT_ADR =0;
			inp_unit->ADR_RAMT_DATA =0x1234;
			inp_unit->ADR_RAMT_DATA =0xF;
			hd=1;
			while (1) {
				inp_unit->ADR_RAMT_ADR =0;
				if ((x=inp_unit->ADR_RAMT_DATA) != 0x1234) {
					printf("\r%4u: error %04X\n", (u_int)i, x);
					hd=0;
					if (!use_default || kbhit()) break;
				}

				if (!(++i &0xFFFF)) {
					printf("\r%4u", (u_int)(i>>16));
					if (kbhit()) break;
				}
			}

			continue;
		}

		switch (rkey) {
case 'D':
			config.i3_menu=rkey;
			ret=vtx_ldac(hv, 1);
			break;

case 'r':
			config.i3_menu=rkey;
			ret=vtx_lreg(hv, 0);
			break;

case 'R':
			config.i3_menu=rkey;
			ret=vtx_lreg(hv, 1);
			break;

case 'p':
			config.i3_menu=rkey;
			ret=qdc_sample();
			hv=(config.vtx_cha != 0);
			lhv=(!hv) ?  "LV" : "HV";
			cha_unit=(!hv) ? &inp_unit->lv : &inp_unit->hv;
			break;

case 'P':
			config.i3_menu=rkey;
			ret=sram_out();
			break;

case 'q':
case 'Q':ret=vtx_data();
			break;

case 's':config.i3_menu=rkey;
			ret=load_seq();
			break;

case 'S':config.i3_menu=rkey;
			seq_edit_menu((VTEX_RG*)inp_unit, hv);
			break;

case 'Z':
			config.i3_menu=rkey;
			printf("Value 0x%04X ", inp_conf->slv_data.s[4]);
			inp_conf->slv_data.s[4]=(u_int)Read_Hexa(inp_conf->slv_data.s[4], -1);
			if (errno) continue;

			printf("clk scal  %2u ", z_scl);
			z_scl=(u_int)Read_Deci(z_scl, 15) &0xF;
			if (errno) continue;

			inp_unit->ADR_SCALING=(inp_unit->ADR_SCALING &~0x0F0) |(z_scl<<4);
			while (1) {
				for (i=0; i < 8; i++) {
					cha_unit->ADR_DAC=(i<<12) |(inp_conf->slv_data.s[4] &0xFFF);
					delay(10);
				}

				cha_unit->ADR_DAC=0x803C;
				delay(10);
				if (kbhit()) break;
			}
			break;

case 'z':
			config.i3_menu=rkey;
			printf("clk scal %2u ", z_scl);
			z_scl=(u_int)Read_Deci(z_scl, 15);
			if (errno) continue;
			printf("nrof bit %2u ", z_bit);
			z_bit=(u_int)Read_Deci(z_bit, 15);
			if (errno) continue;
			printf("5bit pat %02X ", z_pat);
			z_pat=(u_int)Read_Hexa(z_pat, -1);
			if (errno) continue;
			printf("delay    %2u ", z_del);
			z_del=(u_int)Read_Deci(z_del, 15);
			if (errno) continue;

			inp_unit->ADR_CR=(!hv) ? (6*VCR_MON) : (7*VCR_MON);	// monitoring
			inp_unit->ADR_SCALING=(z_scl<<8) |0x21;
			while (1) {
				cha_unit->ADR_REG_CR=(z_del<<12) |(z_bit-1);
				cha_unit->ADR_REG=z_pat<<(16-z_bit);
				delay(10);
				printf("%04X\n", cha_unit->ADR_REG>>(16-z_bit));
				if (kbhit()) break;
			}

			ret=CE_PROMPT;
			break;
default:
			i=key-'0';
			if (i > 9)
				if ((key >= 'A') && (key <= 'H')) i=i-7;
				else i=20;

			if (key == 'B') {
				config.i3_menu=rkey;
				i=9;
				printf("Value 0x%08lX ", inp_conf->slv_data.l[i]);
				inp_conf->slv_data.l[i]=Read_Hexa(inp_conf->slv_data.l[i], -1);
				if (errno) continue;
			} else {
				if ((i < 20) && (i != 7)) {
					config.i3_menu=rkey;
					printf("Value 0x%04X ", inp_conf->slv_data.s[i]);
					inp_conf->slv_data.s[i]=(u_int)Read_Hexa(inp_conf->slv_data.s[i], -1);
					if (errno) continue;
				}
			}

			switch (key) {


	case 'J':i=inp_unit->ADR_IDENT;
				if (i &1) {
					config.jtag_vertex_1.mode=0;
					JTAG_menu(jt_inp_putcsr, jt_inp_getcsr, jt_inp_getdata,
								 &config.jtag_vertex_1, 0);
					break;
				}

				config.jtag_vertex_0.mode=0;
				JTAG_menu(jt_inp_putcsr, jt_inp_getcsr, jt_inp_getdata,
							 &config.jtag_vertex_0,
							 (i >= 0x3000) ? 0 :1);
				break;

	case '0':inp_unit->ADR_CR=inp_conf->slv_data.s[i];
				break;

	case '1':inp_unit->ADR_ACLK_PAR=inp_conf->slv_data.s[i];
				break;

	case '2':inp_unit->ADR_SEQ_CSR=inp_conf->slv_data.s[i];
				break;

	case '3':cha_unit->ADR_POTI=inp_conf->slv_data.s[i];
				break;

	case '4':cha_unit->ADR_DAC=inp_conf->slv_data.s[i];
				break;

	case '5':cha_unit->ADR_REG_CR=inp_conf->slv_data.s[i];
				break;

	case '6':cha_unit->ADR_REG=inp_conf->slv_data.s[i];
				break;

	case '7':printf("reg out: %04X\n", cha_unit->ADR_REG);
				if (BUS_ERROR) CLR_PROT_ERROR();
				ret=CE_PROMPT;
				break;

	case '8':cha_unit->ADR_CLK_DELAY=inp_conf->slv_data.s[i];
				break;

	case '9':cha_unit->ADR_NR_CHAN=inp_conf->slv_data.s[i] -1;
				break;

	case 'A':cha_unit->ADR_NOISE_THR=inp_conf->slv_data.s[i];
				break;

	case 'B':cha_unit->ADR_SWREG=inp_conf->slv_data.l[9];
				break;

	case 'C':cha_unit->ADR_LP_COUNTER[0]=inp_conf->slv_data.s[i];
				break;

	case 'D':cha_unit->ADR_LP_COUNTER[1]=inp_conf->slv_data.s[i];
				break;

	case 'E':inp_unit->ADR_SCALING=inp_conf->slv_data.s[i];
				break;

	case 'F':inp_unit->ADR_DIG_TST=inp_conf->slv_data.s[i];
				break;

	case 'G':if (dot) inp_bc->ADR_ACTION=inp_conf->slv_data.s[i];
				else inp_unit->ADR_ACTION=inp_conf->slv_data.s[i];
				break;

	case 'U':if (inp_assign(sc_conf, 2)) continue;
				if (inp_conf->type != VERTEX) {
					printf("das ist keine VERTEX Karte!!!\n");
					return CE_NO_INPUT;
				}
				break;

	case 'V':if (hv) {
					hv=config.vtx_cha=0;
					lhv="LV";
					cha_unit=&inp_unit->lv;
				} else {
					hv=config.vtx_cha=1;
					lhv="HV";
					cha_unit=&inp_unit->hv;
				}
				break;

			}
		}
	}
}
