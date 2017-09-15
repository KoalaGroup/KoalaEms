// Borland C++ - (C) Copyright 1991, 1992 by Borland International

//	seq.c -- Sequenzer Testprogramm

#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <mem.h>
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <sys\stat.h>
#include "pci.h"
#include "daq.h"
#include "seq.h"

static VTEX_RG		*inp_unit;
static u_long		*paddr;
static u_short		*pdata;
static u_short		*count;
static u_long		*pswregs;
static u_short		*pseqpol;
static u_short		*pcspol;
static u_short		squ_hlt;
static u_short		cntr;
//
//--------------------------- nbits -----------------------------------------
//
static u_int nbits(u_long len)
{
	u_int ix;

	ix=31;
	while (ix && !(len &(1L<<ix))) ix--;
	return ix;
}
//
//--------------------------- sram_test -------------------------------------
//
#define _DTEST_INC	0x0301

static int sram_test(void)
{
	u_short	dout,din;
	u_long	x;
	u_long	lp,lm;
	u_long	i;
	u_long	strt,end;

	printf("start address  %05lX ", config.srtst_strt);
	config.srtst_strt=Read_Hexa(config.srtst_strt, -1);
	if (errno) return CEN_MENU;

	printf("end address    %05lX ", config.srtst_end);
	config.srtst_end=Read_Hexa(config.srtst_end, -1);
	if (errno) return CEN_MENU;

	if (config.srtst_end > SRAM_LEN) config.srtst_end=SRAM_LEN;
	strt=config.srtst_strt;
	end =config.srtst_end;
	if (end == 0) end=SRAM_LEN;

	if (end <= strt) {
		printf("end offset must be higher than start\n");
		return CE_PROMPT;
	}

	end -=strt;
	lp=0;
	lm=0x0800>>nbits(end >>7);
	if (lm) lm--;
	dout=0; din=0;
	*paddr=strt;
	for (i=0; i < end; i++) {
		*pdata=dout;
		dout +=_DTEST_INC;
		if ((u_short)i == 0xFFFF) dout +=_DTEST_INC;
	}

	while (1) {
		*paddr=strt;
		for (i=0; i < end; i++) {
			x=*pdata;
			if (x != din) {
				printf("\r%10lu: error addr=%06lX, exp=%04X, is=%04X\n",
						 lp, strt +i, din, x);
				return CE_PROMPT;
			}

			din +=_DTEST_INC;
			if ((u_short)i == 0xFFFF) din +=_DTEST_INC;
		}

		*paddr=strt;
		for (i=0; i < end; i++) {
			*pdata=dout;
			dout +=_DTEST_INC;
			if ((u_short)i == 0xFFFF) dout +=_DTEST_INC;
		}

		if (!(++lp &lm)) {
			printf("\r%10lu", lp);
			if (kbhit()) {
				Writeln();
				return CE_PROMPT;
			}
		}
	}
}
//
#if 0
//--------------------------- sram_dma -------------------------------------
//
static int sram_dma(void)
{
	PLX_DMA_DESC	*desc;

	u_int		pg64k;
	u_int		pg64kmx=(u_int)(SRAM_LEN >>B_PAGE);
	u_long	addr;
	u_long	dout,din;
	u_long	x,y;
	u_int		lp;
	u_int		i;
	u_char	mode;

	if (dmapages < 16) {
		printf("it will use min 16 DMA pages\n");
		return CE_NO_DMA_BUF;
	}

	printf("mode (b0:wr, b1:rd) %3u ", config.srtst_mode);
	config.srtst_mode=(u_int)Read_Deci(config.srtst_mode, 3);
	mode=config.srtst_mode &3;
	if (!mode || errno) return CEN_MENU;

	if (mode&1) setup_dma(0, 0, 0, 0x10000L, ADDRSPACE1, 0);		// for SRAM write
	if (mode&2) setup_dma(1, 16, 0, 0x10000L, ADDRSPACE1, 1);	// for SRAM read
	lp=0;
	dout=0; din=0;
	if (!(mode&2)) {
		for (i=0; i < 0x4000; i++) {
			dmabuf[0][i] =dout; dout +=_DTEST_INC;
		}
	}

	timemark();
	while (1) {
		if (mode&1) {
			for (pg64k=0; pg64k < pg64kmx; pg64k++) {
				desc=(PLX_DMA_DESC*)dmadesc; addr=ADDRSPACE1+((u_long)pg64k <<16);
				for (i=0; i < 16; i++) {
					desc->dmaladr =addr;
					desc++; addr +=SZ_PAGE;
				}

				if (mode&2) {
					for (i=0; i < 0x4000; i++) {
						dmabuf[0][i] =dout; dout +=_DTEST_INC;
					}
				}

				dmabuf[0][0]=pg64k;
				plx_regs->dma[0].dmadpr=dmadesc_phy |0x9;
				plx_regs->dmacsr[0]=0x3;								// start DMA
				i=0;
				while (1) {
					_delay_(1000);
					if (plx_regs->dmacsr[0] &0x10) break;

					if (++i >= 1000) {
						printf("\r%5u: DMA0 wr time out CSR=%02X\n",
								 lp, plx_regs->dmacsr[0]);
						return CE_PROMPT;
					}
				}

				if (!lp && (pciconf.errlog == 2)) printf("wr wait loops =%u\n", i);
			}
		}

		if (mode&2) {
			for (pg64k=0; pg64k < pg64kmx; pg64k++) {
				desc=(PLX_DMA_DESC*)dmadesc +16; addr=ADDRSPACE1+((u_long)pg64k <<16);
				for (i=0; i < 16; i++) {
					desc->dmaladr =addr;
					desc++; addr +=SZ_PAGE;
				}

				plx_regs->dma[1].dmadpr=(dmadesc_phy +16*sizeof(PLX_DMA_DESC)) |0x9;
				plx_regs->dmacsr[1]=0x3;								// start DMA
				i=0;
				while (1) {
					_delay_(1000);
					if (plx_regs->dmacsr[1] &0x10) break;

					if (++i >= 1000) {
						printf("\r%5u: DMA1 rd time out CSR=%02X\n",
								 lp, plx_regs->dmacsr[1]);
						return CE_PROMPT;
					}
				}

				if (!lp && (pciconf.errlog == 2)) printf("rd wait loops =%u\n", i);

				if (mode&1) {
					for (i=0; i < 0x4000; i++) {
						x=dmabuf[0][i];
						if (!i) y=pg64k;
						else {
							if (!(i&2)) y=din;
							else {
								if (!(i&1)) y=(u_char)din;
								else y=0;
							}
						}

						if (x != y) {
							printf("\r%5u: error addr=%06lX, exp=%08lX, is=%08lX\n",
									 lp, ((u_long)pg64k <<16) +(i<<2), y, x);
							return CE_PROMPT;
						}

						din +=_DTEST_INC;
					}
				}
			}
		}

		lp++;
		printf("\r%5u", lp);
		if (kbhit()) {
			u_long 	tm;
			double	sec;
			u_long	mb;

			Writeln();
			tm=ZeitBin();
			if (!tm || !lp) return CE_PROMPT;

			sec=tm /100.0;
			mb =(u_long)lp *8;
			if (mode == 3) mb <<=1;
			printf("time = %4.2f sec, one loop = %4.2f sec\n", sec, sec/lp);
			printf("Rate = %1.3f MByte per sec\n", mb /sec);
			return CE_PROMPT;
		}
	}
}
#endif
//
//===========================================================================
//
//										Sequencer Untilities
//
//===========================================================================
//
static char		ibuf[80];
static char 	sbuf[80];
static int		sb_ix;
static u_int	sb_id;		// SB_...
static u_int	sb_val;
static u_long	sb_lw;

static u_long	sb_sadr;
static u_int	sb_pc;

#define PA_EOF		-1
#define PA_EOL		-2
#define PA_BOS		-3	// begin of sequence
#define PA_EOS		-4

#define SB_PC		1
#define SB_VAL		2
#define SB_JMP		3
#define SB_JSR		4
#define SB_RTS		5
#define SB_WHALT	6
#define SB_SW		11
#define SB_SWC		12
#define SB_WTRG	7
#define SB_WTIME	8
#define SB_COUNT	9
#define SB_DGAP	13
#define SB_TRG		10

typedef struct {
	char	*cw;
	u_int	id;
} SQ_TOKEN;

const SQ_TOKEN sq_token[] ={
	{ "JMP",	SB_JMP},
	{ "JUMP",SB_JMP},
	{ "JSR", SB_JSR},
	{ "RTS", SB_RTS},
	{ "WH",	SB_WHALT},
	{ "WHLT",SB_WHALT},
	{ "GAP",	SB_DGAP},
	{ "DGAP",SB_DGAP},
	{ "SW",	SB_SW},
	{ "SWC",	SB_SWC},
	{ "WTRG",SB_WTRG},
	{ "WTG",	SB_WTRG},
	{ "WTM",	SB_WTIME},
	{ "C",	SB_COUNT},
	{ "CNT",	SB_COUNT},
	{ "T",	SB_TRG},
	{ "TRG",	SB_TRG},
	{ 0, 0}
};

//--------------------------- seq_help --------------------------------------
//
static int seq_help(void)
{

	printf("SRAM L„nge ist %lX (%u k) word\n", SRAM_LEN, (u_int)(SRAM_LEN >>10));
	printf("\n");
	printf("10: 3*4711 T1\n");
	printf("JMP *+1\n");
	printf("10(1, 0 T2, 20(4711))\n");
	printf("\n");
	printf("Opcode Kennungen:\n");
	printf("  JMP n\n");
	printf("  JSR n\n");
	printf("  RTS\n");
	printf("  WH      warte, wenn HALT Signal gesetzt ist\n");
	printf("  SW r    switch reg \n");
	printf("  SWC r   switch reg r and clear enable\n");
	printf("  WTG i   warte auf Trigger i\n");
	printf("  WTM x   warte bis Zeit x abgelaufen\n");
	printf("  GAP x   Ausgabe GAP x Takte\n");
	printf("  C i     inkrementiere Counter i\n");
	printf("  T i     geben Triggerinpuls i aus\n");
	printf("\n");

	return CEN_PROMPT;
}
//
static int sram_inout(
	u_int		rd,	// read SRAM
	u_int		pg)	// page (=> 0x0800 SRAM words)
{
	u_int		i,err;
	u_short	*bf0,*bf1;

//printf("sram_inout(%d, 0x%X)\n", rd, pg);
	*paddr=(u_long)pg<<11;
		
	for (i=0, bf0=(u_short*)dmabuf; i < 0x800; i++, bf0++) {
		if (rd) {
			*bf0=*pdata;
		} else {
			*pdata=*bf0;
		}
	}

	if (rd) return 0;

	*paddr=(u_long)pg<<11;
	for (i=0, bf1=(u_short*)dmabuf+0x800; i < 0x800; i++, bf1++) {
		*bf1=*pdata;
	}

	bf0=(u_short*)dmabuf; bf1=bf0+0x800; err=0;
	for (i=0; i < 0x800; i++)
		if (bf0[i] != bf1[i]) {
			printf("read/write error at %05lX: is=%04X exp=%04X\n",
					 ((u_long)pg <<11)+i, bf1[i], bf0[i]);
			if (++err > 10) return CE_PROMPT;
		}

	if (err) return CE_PROMPT;

	return 0;
}

static int space(char dg)
{
	return ((dg == ' ') || (dg == TAB));
}

static int seq_a2bin(	// count of valid characters
								// value in sb_lw
	char	hex,				// default base
	char	*str)
{
	u_int	i;
	char	base;
	char	dg;
	u_int	len;

	base=-1;
	for (i=0;; i++) {
		dg=str[i];
		if ((dg >= '0') && (dg <= '9')) continue;

		if (   (dg >= 'A') && (dg <= 'F')
			 && (hex || (base == 1))) { base=1; continue; }	// z.B. 0x

		if ((i == 1) && (dg == 'X') && (str[0] == '0')) { base=1; continue; }

		if ((dg == '.') && (base < 0)) { base=0; len=i+1; break; }

		len=i; break;
	}

	if (base < 0) base=hex;

	sb_lw=0;
	if (base) {							// Hexa
		for (i=0; i < len; i++) {
			dg=str[i];
			if (dg == 'X') continue;

			if (dg >= 'A') dg -=7;

			sb_lw =(sb_lw <<4) +(dg-'0');
		}

		return len;
	}

	for (i=0; (i < len) && (str[i] != '.'); i++)
		sb_lw=10*sb_lw +(str[i]-'0');

	return len;
}

static int seq_parser(void)
{
	u_int		dg;
	SQ_TOKEN	const *tk;
	int		err;
	u_int		i;

	while (space(sbuf[sb_ix])) sb_ix++;

	dg=sbuf[sb_ix];
	if (!dg) return PA_EOF;

	if ((dg == ',') || (dg == ';')) { sb_ix++; return PA_EOL; }

	if (dg == ')') { sb_ix++; return PA_EOS; }

	i=seq_a2bin(0, sbuf +sb_ix);
	while (space(sbuf[sb_ix+i])) i++;

// printf(" i=%d %08lX\n", i, sb_lw);
	if (sbuf[sb_ix+i] == '(') {
		sb_ix +=(i+1); return PA_BOS;
	}

	sb_val=0; sb_id=0; 
	if ((dg >= '0') && (dg <= '9')) {
		if (sbuf[sb_ix+i] == '*') {
			if ((sb_lw == 0) || (sb_lw > 64)) return 1;

			sb_val=(u_int)sb_lw-1;
			sb_ix+=(i+1); sb_id=SB_VAL;
		}

		i=seq_a2bin(1, sbuf +sb_ix);
		sb_ix +=i;
		if (sbuf[sb_ix] == ':') {
			if (sb_id) return 2;

			sb_id=SB_PC; sb_ix++;
		} else sb_id=SB_VAL;

		return 0;
	}

	tk=sq_token;
	do {
		i=0;
		while (tk->cw[i] && (tk->cw[i] == sbuf[sb_ix+i])) i++;
		if (!tk->cw[i] && (sbuf[sb_ix+i] <= '9')) {	// token gefunden
			sb_ix +=i;
			while (space(sbuf[sb_ix])) sb_ix++;

			if (sbuf[sb_ix] == '*') {						// pc einsetzen
				sb_ix++; sb_lw=sb_sadr+sb_pc;
				while (space(sbuf[sb_ix])) sb_ix++;

				if ((sbuf[sb_ix] == '+') || (sbuf[sb_ix] == '-')) {
					sb_ix++;
					i=seq_a2bin(0, sbuf +sb_ix);
					if (sbuf[sb_ix-1] == '+') sb_lw=sb_sadr+sb_pc+sb_lw;
					else sb_lw=sb_sadr+sb_pc-sb_lw;

					sb_ix +=i;
				}

				sb_val=(u_int)sb_lw;
				sb_id=tk->id;
				return 0;
			}

			if (   sbuf[sb_ix] 
				 && ((sbuf[sb_ix] < '0') || (sbuf[sb_ix] > '9'))) return 4;

			if ((sbuf[sb_ix] >= '0') && (sbuf[sb_ix] <= '9')) {
				if ((err=seq_parser()) != 0) return err;

				if (sb_id != SB_VAL) return 5;

				sb_val=(u_int)sb_lw;
			}

			sb_id=tk->id;
			return 0;
		}

		tk++;
	} while (tk->id);

	return 6;
}

//
//--------------------------- seq_inst --------------------------------------
//
// sequencer instruction
//
static int seq_inst(u_long pc, u_short *sram)
{
	u_int		i;
	u_int		l;
	u_char	opc,mul;
	u_char	err;

	opc=*sram >>B_DATA; err=0;
	l=sprintf(ibuf, "%05lX: ", pc);
	if (   ((opc &0xF0) == OPC_TRG)
		 || ((opc &0xE0) == OPC_OUT)) {	// data

		if ((opc &0xF0) == OPC_TRG) mul=0;
		else mul=(*sram >>B_XDATA) &M_DREPT;

		if (mul) l +=sprintf(ibuf+l, "%2u*", mul+1);
		else l +=sprintf(ibuf+l, "   ");

		l +=sprintf(ibuf+l, "%03X", *sram &M_XDATA);

	} else l +=sprintf(ibuf+l, "           ");

	switch (opc &0xF0) {
case 0x00:
		if ((opc &~0x7) == OPC_DGAP) {
			l +=sprintf(ibuf+l, " DGAP %04X", (*sram &M_GAP)+1);
			break;
		}

		if (*sram &M_DATA) err=13;

		if (opc == OPC_RTS) {
			l +=sprintf(ibuf+l, " RTS");
			break;
		}

		if (opc == OPC_WHALT) {
			l +=sprintf(ibuf+l, " WH");
			break;
		}

		if ((opc &~0x1) == 0x2) {
			err=11;
			break;
		}

		if ((opc &~0x3) == OPC_COUNT) {
			l +=sprintf(ibuf+l, " C%u", opc &3);
			break;
		}

		err=14;
		break;

case OPC_SW:
		l +=sprintf(ibuf+l, (opc &0x08) ? " SWC %u" : " SW %u", opc &0x07);
		break;

case OPC_WTRG:
case OPC_WTIME:
		for (i=0; i < 4; i++)
			if (opc &(1<<i)) l +=sprintf(ibuf+l, " WTG%u", i);

		if (!(opc &0x10)) {
			if (*sram &M_DATA) err=15;

			break;
		}

		l +=sprintf(ibuf+l, " WTM %04X", *sram &M_DATA);
		break;


case OPC_OUT:
case OPC_OUT+1:
		break;

case OPC_TRG:
		l +=sprintf(ibuf+l, " T%u", (opc >>2) &3);
		break;

case OPC_JMP:
		if ((opc &~0x7) == OPC_JMP) {
			l +=sprintf(ibuf+l, " JMP %05lX", (u_long)(*sram &M_GAP) <<B_JMP);
			break;
		}

		l +=sprintf(ibuf+l, " JSR %05lX", (u_long)(*sram &M_GAP) <<B_JMP);
		break;

	}

	if (err) l +=sprintf(ibuf+l, " ;ill opc %02X/%d", opc, err);

	return l;
}
//
//--------------------------- seq_edit --------------------------------------
//
static u_long	rtr_c[2];
static u_int	rtr_x[2];
static u_int	rtr_ix;

#define OPC(x)	((x)<<B_DATA)

static int seq_edit(void)
{
	u_int		i;
	u_short	*sram=(u_short*)dmabuf;
	u_int		sw;
	int		err;
	u_char	nopc;		// opcode speichern
	u_short	new=0;

	if (dmapages < 2) {
		printf("it will use min 2 DMA pages\n");
		return CE_NO_DMA_BUF;
	}

	sb_sadr=0; sb_pc=0;
	if (sram_inout(1, (u_int)(sb_sadr >>B_PAGE))) return CE_PROMPT;

	while (1) {
		i=seq_inst(sb_sadr+sb_pc, sram+sb_pc);
		ibuf[i]=0;
		printf("%-30s =>", ibuf);
		strcpy(sbuf, Read_String(0, sizeof(sbuf)));
		if (errno) {
			sram_inout(0, (u_int)(sb_sadr >>B_PAGE));
			return CE_PROMPT;
		}

		if ((sbuf[0] == '?') && !sbuf[1]) {
			Writeln(); seq_help();
			continue;
		}

		for (i=0; sbuf[i]; i++) sbuf[i]=toupper(sbuf[i]);

		sb_ix=0; rtr_ix=0; sw=1;
		while (1) {
			new=0; nopc=0;
			while ((err=seq_parser()) == 0) {
// printf(" %u %u %08lX\n", sb_id, sb_val, sb_lw);
				switch (sw) {
			default:
					err=20;
					break;

			case 1:							// start evt. PC
			case 2:							// start mit PC
					switch (sb_id) {
				default:
						err=21;
						break;

				case SB_PC:
						if (sw != 1) { err=22; break; }

						if (sb_lw >= SRAM_LEN) { err=23; break; }

						if (sb_sadr != (sb_lw &~M_PAGE)) {
							if (sram_inout(0, (u_int)(sb_sadr >>B_PAGE))) { err=100; break; }

							sb_sadr=sb_lw &~M_PAGE;
							if (sram_inout(1, (u_int)(sb_sadr >>B_PAGE))) { err=101; break; }
						}

						sb_pc=(u_int)sb_lw &M_PAGE;
						sw=2;
						break;

				case SB_VAL:
						if ((sb_lw > M_XDATA) || (sb_val > M_DREPT)) err=24;

						new=(u_short)sb_lw |OPC(OPC_OUT +(sb_val<<(B_XDATA-B_DATA))); nopc=1;
						sw=3;
						break;

				case SB_JMP:
						if ((sb_lw &M_JMP) || (sb_lw >= SRAM_LEN)) err=25;

						new=(u_short)(sb_lw >>B_JMP) |OPC(OPC_JMP); nopc=1;
						sw=0;
						break;

				case SB_JSR:
						if ((sb_lw &M_JMP) || (sb_lw >= SRAM_LEN)) err=26;

						new=(u_short)(sb_lw >>B_JMP) |OPC(OPC_JSR); nopc=1;
						sw=0;
						break;

				case SB_RTS:
						new=OPC(OPC_RTS); nopc=1;
						sw=0;
						break;

				case SB_WHALT:
						new=OPC(OPC_WHALT); nopc=1;
						sw=0;
						break;

				case SB_SW:
						if (sb_val >= 8) { err=27; break; }

						new=OPC(OPC_SW |sb_val); nopc=1;
						sw=0;
						break;

				case SB_SWC:
						if (sb_val >= 8) { err=28; break; }

						new=OPC(OPC_SWC |sb_val); nopc=1;
						sw=0;
						break;

				case SB_WTRG:
						new=OPC(OPC_WTRG); nopc=1;
						if (sb_val >= 4) { err=29; break; }

						new |=OPC(1<<sb_val);
						sw=4;
						break;

				case SB_WTIME:
						if (sb_lw > M_DATA) err=30;

						new=(u_short)sb_lw |OPC(OPC_WTIME); nopc=1;
						sw=4;
						break;

				case SB_COUNT:
						if (sb_val >= 4) { err=31; break; }

						new=OPC(OPC_COUNT |sb_val); nopc=1;
						sw=0;
						break;

				case SB_DGAP:
						sb_lw--;
						if (sb_lw > M_GAP) err=32;

						new=(u_short)sb_lw |OPC(OPC_DGAP); nopc=1;
						sw=0;
						break;

					}

					break;

			case 3:							// extention to data
					switch (sb_id) {
				default:
						err=40;
						break;

				case SB_TRG:
						if ((new >>B_DATA) &0x38) { err=42; break; }	// repeat

						if (sb_val >= 4) { err=43; break; }

						new |=OPC(OPC_TRG |(sb_val<<2));
						sw=0;
						break;

					}

					break;

			case 4:							// wait function
					switch (sb_id) {
				default:
						err=44;
						break;

				case SB_WTRG:
						if (sb_val >= 4) { err=46; break; }

						new |=OPC(1<<sb_val);
						break;

				case SB_WTIME:
						if (sb_lw > M_DATA) err=47;

						new=(u_short)sb_lw |OPC((new >>B_DATA) |OPC_WTIME);
						break;

					}

					break;

				}

				if (err) break;
			}

			if (err >= 0) {
				printf("syntax error %d: %u\n", err, sb_ix); break;
			}

			if (nopc) { sram[sb_pc]=new; sb_pc++; }
			else {
				if ((sw == 1) && (err == PA_EOF)) sb_pc++;
			}

			if (sb_pc >= C_PAGE) {
				if (sb_sadr >= (SRAM_LEN-C_PAGE)) { sb_pc=M_PAGE; break; }

				if (sram_inout(0, (u_int)(sb_sadr >>B_PAGE))) return CE_PROMPT;

				sb_sadr +=C_PAGE; sb_pc=0;
				if (sram_inout(1, (u_int)(sb_sadr >>B_PAGE))) return CE_PROMPT;
			}

			if (err == PA_EOF) break;

         sw=2; 
			if (err == PA_EOL) continue;

			if (err == PA_BOS) {
				if (rtr_ix >= 2) {
					printf("retry overrun\n"); break;
				}

				if (!sb_lw) sb_lw=1;

				rtr_c[rtr_ix]=sb_lw; rtr_x[rtr_ix]=sb_ix; rtr_ix++; continue;
			}

			if (err == PA_EOS) {
				if (rtr_ix == 0) {
					printf("retry underrun\n"); break;
				}

				if (--rtr_c[rtr_ix-1]) { sb_ix=rtr_x[rtr_ix-1];  continue; }

				rtr_ix--;
				continue;
			}

			printf("syntax error %d: %u\n", err, sb_ix);
			break;
		}
	}
}
//

//--------------------------- seq_save --------------------------------------
//
static int seq_save(void)
{
	int		hx;
	u_int		pg,l;
	int		ret;
	u_long	lx;

	printf("save to ");
	strcpy(config.sq_save_file,
			 Read_String(config.sq_save_file, sizeof(config.sq_save_file)));
	if (errno) return CEN_MENU;

	printf("end address %05lX ", config.save_end);
	config.save_end=Read_Hexa(config.save_end, -1);
	if (errno) return CEN_MENU;

	if (config.save_end > SRAM_LEN) config.save_end=SRAM_LEN;
	if ((hx=_rtl_creat(config.sq_save_file, 0)) == -1) {
		perror("error creating file");
		return CEN_PROMPT;
	}

	ret=CEN_MENU; pg=0; lx=config.save_end <<1; l=SZ_PAGE;
	if (!lx) lx=SRAM_LEN<<1;
	do {
		if (sram_inout(1, pg)) { ret=CEN_PROMPT; break; }

		if ((u_long)l > lx) l=(u_int)lx;
		if (write(hx, dmabuf, l) != l) {
			perror("error writing file");
			ret=CEN_PROMPT; break;
		}

		lx-=l;
	} while (lx);

	if (close(hx) == -1) {
		perror("error closing file");
		ret=CEN_PROMPT;
	}

	return ret;
}
//
//--------------------------- seq_load --------------------------------------
//
static int seq_load(void)
{
	int		hx;
	int		len;
	u_int		pg;
	int		ret;
	u_short	*sr=(u_short*)dmabuf;
	u_int		i;

	printf("load from ");
	strcpy(config.sq_load_file,
			 Read_String(config.sq_load_file, sizeof(config.sq_load_file)));
	if (errno) return CEN_MENU;

	if ((hx=_rtl_open(config.sq_load_file, O_RDONLY)) == -1) {
		perror("error opening file");
		return CEN_PROMPT;
	}

	ret=CEN_MENU; pg=0;
	do {
		if ((len=read(hx, dmabuf, SZ_PAGE)) < 0) {
			perror("error reading file");
			len=0; ret=CEN_PROMPT; break;
		}

//printf("%3X %4X\n",pg,len);
		if (!len) break;

		if (len != SZ_PAGE)
			for (i=len >>(12-B_PAGE); i < (1 <<B_PAGE); i++) sr[i]=0;

		if (sram_inout(0, pg)) { len=0; ret=CEN_PROMPT; break; }
		if (len != SZ_PAGE) { ret=CEN_PROMPT; break; }

		pg++; len=0;
	} while (pg < (u_int)(SRAM_LEN >>B_PAGE));

	printf("0x%06lX SRAM words loaded\n", ((u_long)pg <<B_PAGE) +(len >>(12-B_PAGE)));
	if (close(hx) == -1) {
		perror("error closing file");
		ret=CEN_PROMPT;
	}

	return ret;
}
//
//--------------------------- seq_fill --------------------------------------
//
static int seq_fill(void)
{
	u_short	*sram=(u_short*)dmabuf;
	u_int		i;

	printf("data  %04X ", config.srf_lw);
	config.srf_lw=(u_short)Read_Hexa(config.srf_lw, -1);
	if (errno) return CEN_MENU;

	for (i=0; i < 0x800; i++) sram[i]=config.srf_lw;

	for (i=0; i < (SRAM_LEN >>B_PAGE); i++)
		if (sram_inout(0, i)) return CEN_PROMPT;

	return CEN_MENU;
}
//
//--------------------------- seq_simu --------------------------------------
//
#define C_STACK	0x10
static u_long	stack[C_STACK];

static int seq_simu(void)
{
	u_short	*sram=(u_short*)dmabuf;
	u_int		i;
	u_char	opc;
	int		err=0;
	char		key;
	u_char	swreg[8]={0};
	int		csw=-2;
	u_char	halt=0;
	u_char	whlt=0;
	u_int		sp=0;
	u_long	newpc;
	u_long	count[4]={0};
	u_long	trig[4]={0};
	u_int		itrig=0;
	char		ctrg=0;
	char		debug=0;
	u_int		ln=0;
	u_int		lp=0;
	u_int		spm=0;

	if (dmapages < 2) {
		printf("it will use min 2 DMA pages\n");
		return CE_NO_DMA_BUF;
	}

	printf("debug %d ", debug);
	debug=(u_int)Read_Deci(debug, 1);

	sb_sadr=0;
	if (sram_inout(1, (u_int)(sb_sadr >>B_PAGE))) return CE_PROMPT;

	while (1) {
		i=(u_int)sb_sadr &M_PAGE;
		newpc=sb_sadr+1;
		opc=sram[i] >>B_DATA;
#if 0
if (debug) {
	printf("%06lX: %02X\n", sb_sadr, opc);
	if (++ln >= 40) {
		ln=0;
		if (WaitKey(0)) debug=0;
	}
}
#endif
		switch (opc >>4) {
	default:
			err=1;
			break;

	case 0x00:
			if (opc == OPC_RTS) {
				if (sp == 0) {
					printf("%05lX: stack underflow\n", sb_sadr);
					err=-1; break;
				}

				newpc=stack[--sp];
				break;
			}

			if (opc == OPC_WHALT) {
				if (halt) { newpc=sb_sadr; whlt=1; break; }

				whlt=0;
				break;
			}

			if ((opc &~3) == OPC_COUNT) {
				count[opc &3]++;
				break;
			}

			err=2;
			break;

	case OPC_SW>>4:
			if (!(swreg[opc &0x7] &4)) { newpc +=4; break; }

			newpc +=(swreg[opc &0x7] &0x3);
			if (!(opc &0x08)) break;

			swreg[opc &0x7] &=0x3;		// OPC_SWC
			break;

	case OPC_WTRG>>4:
	case OPC_WTIME>>4:
			if (!(opc &0x1F)) { err=5; break; }

			if ((opc &1) && (itrig &1)) break;

			if ((opc &2) && (itrig &2)) { newpc +=1; break; }

			if ((opc &4) && (itrig &4)) { newpc +=2; break; }

			if ((opc &8) && (itrig &8)) { newpc +=3; break; }

			if (opc &0x10) { newpc +=4; break; }

			newpc=sb_sadr;
			break;

	case OPC_OUT>>4:
	case (OPC_OUT>>4)+1:
			break;

	case OPC_TRG>>4:
			trig[(opc >>2) &3]++;
			break;

	case OPC_JMP>>4:
				if ((opc &~7) == OPC_JMP) {
					if (((u_long)(sram[i] &M_GAP) <<B_JMP) >= SRAM_LEN) err=3;

					newpc=(u_long)(sram[i] &M_GAP) <<B_JMP;
#if 0
					if (!newpc) {
						printf("%05lX -> jmp 0\n", sb_sadr);
						err=-1;
					}
#endif

					break;
				}

//	OPC_JSR
				if (((u_long)(sram[i] &M_GAP) <<B_JMP) >= SRAM_LEN) err=4;

				if (sp >= C_STACK) {
					printf("%05lX: stack overflow\n", sb_sadr);
					err=-1; break;
				}

				stack[sp++]=newpc;
				newpc=(u_long)(sram[i] &M_GAP) <<B_JMP;
				if (sp > spm) { spm=sp; printf("sp=%2d\n", spm); }
				if (debug) {
					printf("%u: %05lX -> %05lX\n", sp-1, sb_sadr, newpc);
					if (++ln >= 40) {
						ln=0;
						if (WaitKey(0)) debug=0;
					}
				}

				break;

		}

		if (err) break;

//if (debug) { printf("%06lX/%06lX\n", sb_sadr, newpc); ln++; }
		if ((newpc &~M_PAGE) != (sb_sadr &~M_PAGE)) {
			if (newpc >= SRAM_LEN) {
				printf("pc overrun\n");
				err=-1; break;
			}

//if (debug) { printf("load page %04X\n", (u_int)(newpc >>B_PAGE)); ln++; }
			if (sram_inout(1, (u_int)(newpc >>B_PAGE))) return CE_PROMPT;
		}

		sb_sadr=newpc;
		if ((++lp >= 1000) && kbhit()) {
			lp=0;
			key=toupper(getch());
			switch (key) {
	case '?':
				printf("H   toggle halt\n");
				printf("C   clear all counter\n");
				printf("D   set debug mode\n");
				printf("Sni set switch n to i\n");
				printf("Tn  toggle trigger n\n");
				printf("cr  show state\n");
				printf("\n");
				break;

	case 'H':
				halt=!halt; break;

	case 'C':
				for (i=0; i < 4; i++) { count[i]=0; trig[i]=0; }
				break;

	case 'D':
				debug=1; break;

	case 'S':
				ctrg=0; csw=-1; break;

	case 'T':
				ctrg=1; csw=-2; break;

	case CR:
	case ' ':
				break;

	case '0':
	case '1':
	case '2':
	case '3':
				if (ctrg) {
					itrig ^=(1 <<(key-'0'));
					ctrg=0; break;
				}

				if (csw >= 0) {
					swreg[csw]=4 |(key-'0');
					csw=-2; break;
				}
	case '4':
	case '5':
	case '6':
	case '7':
				if (csw == -1) csw=key-'0';

				break;

	default:
				err=-1; break;
			}

			ibuf[seq_inst(newpc, sram+i)]=0;
			printf("%s\n", ibuf);
			printf("itrig=%X", itrig);
			if (whlt) printf(", halt");
			printf(", switch=");
			for (i=0; i < 8; i++) printf("%X", swreg[7-i]);
			printf("\n");
			printf("counter(3:0) %08lX/%08lX/%08lX/%08lX\n",
					 count[3], count[2], count[1], count[0]);
			printf("trigger(3:0) %08lX/%08lX/%08lX/%08lX\n",
					 trig[3], trig[2], trig[1], trig[0]);

			if (err) break;
		}
	}

	printf("error=%d\n", err);
	i=0;
	while (i < spm) {
		if ((i %8) == 0)
			if (!i) printf("stack:");
			else printf("\n      ");

		printf(" %05lX", stack[i++]);
	}

	if (i) Writeln();

	return CE_PROMPT;
}
//
//------------------------------ seq_edit_menu ------------------------------
//
#define P_CR	0
#define P_SQCR	1
#define P_SQPL	2
#define P_CSPL	3
#define P_SW	4

int seq_edit_menu(
	VTEX_RG	*_inp_unit,
	int		hv)
{
	int	ret;
	char	key;

	if (phypages < 16) return CE_NO_DMA_BUF;

	inp_unit=_inp_unit;
	if (hv == HV) {
		paddr=&inp_unit->ADR_RAM_ADR_HV;
		pdata=&inp_unit->ADR_RAM_DATA_HV;
		count=inp_unit->ADR_COUNTER_HV;
		pswregs=&inp_unit->ADR_SWREG_HV;
		pseqpol=&inp_unit->ADR_SEQPOL_HV;
		pcspol=&inp_unit->ADR_CSPOL_HV;
		squ_hlt=SQCR_HALT_HV;
		cntr=AR_CNTR_HV;
	} else {
		paddr=&inp_unit->ADR_RAM_ADR_LV;
		pdata=&inp_unit->ADR_RAM_DATA_LV;
		count=inp_unit->ADR_COUNTER_LV;
		pswregs=&inp_unit->ADR_SWREG_LV;
		pseqpol=&inp_unit->ADR_SEQPOL_LV;
		pcspol=&inp_unit->ADR_CSPOL_LV;
		squ_hlt=SQCR_HALT_LV;
		cntr=AR_CNTR_LV;
	}

	while (1) {
		Writeln();
		printf("sequencer editor ............... 0\n");
		printf("save SRAM to file .............. 1\n");
		printf("load SRAM from file ............ 2\n");
		printf("toggle run ..................... r %s\n", (inp_unit->ADR_CTRL &CR_RUN) ? "run" : "");
		printf("toggle halt .................... h %s\n", (inp_unit->ADR_SEQCTRL &squ_hlt) ? "halt" : "");
		printf("fill SRAM ...................... f\n");
		printf("help ........................... ?\n");
		printf("cnt0/3 ..... %04X/%04X/%04X/%04X C\n", count[0], count[1], count[2], count[3]);
		printf("status .................... %04X\n", inp_unit->ADR_SEQSTATE);
		printf("control ................... %04X 5\n", inp_unit->ADR_CTRL);
		printf("seq control ............... %04X 6\n", inp_unit->ADR_SEQCTRL);
		printf("switch registers ...... %08lX 7\n", *pswregs);
		printf("seq polarity ............... %03X 8\n", *pseqpol);
		printf("chip sel polarity .......... %03X 9\n", *pcspol);
		printf("simulation ..................... s\n");
		protocol_error(-1, 0);
		printf("                                 %c ", config.i3_menu);

		key=toupper(getch());
		if ((key == CTL_C) || (key == ESC)) {
			return 0;
		}

		if (key >= ' ') printf("%c", key);

		Writeln();
		while (1) {
			use_default=(key == TAB);
			if ((key == TAB) || (key == CR)) key=config.i3_menu;

			errno=0; ret=CEN_MENU;
			Writeln();
			switch (key) {

		case '0':
				inp_unit->ADR_CTRL &=~CR_RUN;
				ret=seq_edit();
				break;

		case '1':
				inp_unit->ADR_CTRL &=~CR_RUN;
				ret=seq_save();
				break;

		case '2':
				inp_unit->ADR_CTRL &=~CR_RUN;
				ret=seq_load();
				break;

		case 'R':
				inp_unit->ADR_CTRL ^=CR_RUN;
				break;

		case 'H':
				inp_unit->ADR_SEQCTRL ^=squ_hlt;
				break;

		case 'F':
				inp_unit->ADR_CTRL &=~CR_RUN;
				ret=seq_fill();
				break;

		case '5':
				printf("Value $%04X ", (u_int)config.ao_regs[P_CR]);
				config.ao_regs[P_CR]=(u_int)Read_Hexa(config.ao_regs[P_CR], 0xFFFF);
				if (errno) break;

				inp_unit->ADR_CTRL =(u_short)config.ao_regs[P_CR];
				break;

		case '6':
				printf("Value $%04X ", (u_int)config.ao_regs[P_SQCR]);
				config.ao_regs[P_SQCR]=(u_int)Read_Hexa(config.ao_regs[P_SQCR], 0xFFFF);
				if (errno) break;

				inp_unit->ADR_SEQCTRL =(u_short)config.ao_regs[P_SQCR];
				break;

		case '7':
				printf("Value $%08lX ", config.ao_regs[P_SW]);
				config.ao_regs[P_SW]=Read_Hexa(config.ao_regs[P_SW], -1);
				if (errno) break;

				*pswregs =config.ao_regs[P_SW];
				ret=CE_MENU;
				break;

		case '8':
				printf("Value $%04X ", (u_int)config.ao_regs[P_SQPL]);
				config.ao_regs[P_SQPL]=(u_int)Read_Hexa(config.ao_regs[P_SQPL], 0xFFFF);
				if (errno) break;

				*pseqpol =(u_short)config.ao_regs[P_SQPL];
				ret=CE_MENU;
				break;

		case '9':
				printf("Value $%04X ", (u_int)config.ao_regs[P_CSPL]);
				config.ao_regs[P_CSPL]=(u_int)Read_Hexa(config.ao_regs[P_CSPL], 0xFFFF);
				if (errno) break;

				*pcspol =(u_short)config.ao_regs[P_CSPL];
				ret=CE_MENU;
				break;

		case 'S':
				inp_unit->ADR_CTRL &=~CR_RUN;
				ret=seq_simu();
				break;

		case 'C':
				inp_unit->ADR_ACTION=cntr;
				break;

		case '?':
				ret=seq_help();
				break;

		case 'T':
				inp_unit->ADR_CTRL &=~CR_RUN;
				ret=sram_test();
				break;

			}

			while (kbhit()) getch();
			if ((ret == CE_MENU) || (ret == CE_PROMPT)) config.i3_menu=key;

			if ((ret == CEN_MENU) || (ret == CE_MENU)) break;

			if (ret > CE_PROMPT) display_errcode(ret);

			printf("select> ");
			key=toupper(getch());
			if (key >= ' ') printf("%c", key);
			Writeln();

			if ((key < ' ') && (key != TAB)) break;

			if (key == ' ') key=CR;
		}
	}
}
