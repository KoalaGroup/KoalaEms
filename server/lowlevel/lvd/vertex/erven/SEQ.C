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
static u_long		addr_ofs;
static u_short		squ_hlt;
static VTX_GRP		*pgrp;
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
#define _PTEST_INC	0x0301

int sram_test(
	u_long	*addr,
	u_short	*data,
	u_long	mlen)		// Byte
{
	u_short	dout,din;
	u_short	x;
	u_long	lp,lm;
	u_long	i;
	u_long	strt,end;

	printf("start address  %06lX ", config.srtst_strt);
	config.srtst_strt=Read_Hexa(config.srtst_strt, -1);
	if (errno) return CEN_MENU;

	printf("end address    %06lX ", config.srtst_end);
	config.srtst_end=Read_Hexa(config.srtst_end, -1);
	if (errno) return CEN_MENU;

	if (config.srtst_end > mlen/2) config.srtst_end=mlen/2;
	strt=config.srtst_strt;
	end =config.srtst_end;
	if (end == 0) end=mlen/2;

	if (end <= strt) {
		printf("end offset must be higher than start\n");
		return CE_PROMPT;
	}

	end -=strt;
	lp=0;
	lm=0x0800>>nbits(end >>7);
	if (lm) lm--;
	dout=0; din=0;
	*addr=strt;
	for (i=0; i < end; i++) {
		*data=dout;
		dout +=_PTEST_INC;
		if ((u_short)i == 0xFFFF) dout +=_PTEST_INC;
	}

	while (1) {
		*addr=strt;
		for (i=0; i < end; i++) {
			x=*data;
			if (x != din) {
				printf("\r%10lu: error addr=%06lX, exp=%04X, is=%04X\n",
						 lp, strt +i, din, x);
				*addr=strt +i; x=*data;
				printf("read again %04X,%04X\n", x, *data);
				return CE_PROMPT;
			}

			din +=_PTEST_INC;
			if ((u_short)i == 0xFFFF) din +=_PTEST_INC;
		}

		*addr=strt;
		for (i=0; i < end; i++) {
			*data=dout;
			dout +=_PTEST_INC;
			if ((u_short)i == 0xFFFF) dout +=_PTEST_INC;
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
//--------------------------- sram_dtest ------------------------------------
//
#define _DTEST_INC	0xFFFF0001L

#ifdef SC_M
static u_long	blen=0x10000L;

#define _BLEN_			0x100000L
#define _BLENPGS_		((u_int)((_BLEN_+SZ_PAGE-1)/SZ_PAGE))

int sram_dtest(
	u_long	*addr,
	u_short	*data,
	u_long	mlen)
{
	u_int		blk;
	u_int		blks;
	u_int		pgs;
	u_long	dout,din;
	u_long	x,y;
	u_long	lp;
	u_long	u;
	u_int		i;
	u_char	mode;

#if 0
	if (dmapages < _BLENPGS_) {
		printf("it will use min 0x%02X DMA pages\n", _BLENPGS_);
		return CE_NO_DMA_BUF;
	}
#endif

	printf("DMA block len  %06lX ", blen);
	blen=Read_Hexa(blen, -1);
	if ((blen < 4) || errno) return CEN_MENU;

	pgs=(u_int)((blen+SZ_PAGE-1)/SZ_PAGE);
	blks=(u_int)(mlen/blen);
	if (dmapages < pgs) {
		printf("it will use min 0x%02X DMA pages\n", pgs);
		return CE_NO_DMA_BUF;
	}

	printf("mode (rd:2, wr:1) %3u ", config.srtst_mode);
	config.srtst_mode=(u_int)Read_Deci(config.srtst_mode, 3);
	mode=config.srtst_mode &3;
	if (!mode || errno) return CEN_MENU;

	blks=(u_int)(mlen/blen);
	setup_dma(1, 0, 0, blen, (u_int)data, 0, 0);	// for SRAM write
	setup_dma(-1, _BLENPGS_, 0, blen, 0, 0, DMA_L2PCI);							// for SRAM read
	lp=0;
	dout=0; din=0;
	if (!(mode&2)) {
		for (u=0; u < blen/4; u++) {
			DBUF_L(u) =dout; dout +=_DTEST_INC;
		}
	}

	gigal_regs->d0_bc_adr =0;
	gigal_regs->d0_bc_len =0;
	timemark();
	while (1) {
		if (mode&1) {								// DMA write
			*addr=0;
			for (blk=0; blk < blks; blk++) {
				if (mode&2) {						// auch DMA read
					for (u=0; u < blen/4; u++) {
						DBUF_L(u) =dout; dout +=_DTEST_INC;
					}
				}

				dmabuf[0][0]=blk;
				gigal_regs->d_hdr=HW_R_BUS|HW_FIFO|HW_BT|HW_WR;
				gigal_regs->d_ad =0;
				gigal_regs->d_bc =blen;
if (blen > 0x1000L)
				plx_regs->dma[1].dmadpr=dmadesc_phy[0] |0x9;

				plx_regs->dmacsr[1]=0x3;								// start DMA
				i=0;
				while (1) {
					_delay_(5000L);
					if (plx_regs->dmacsr[1] &0x10) break;

					if (!++i) {
						printf("\r%10lu: DMA1 wr time out CSR=%02X\n",
								 lp, plx_regs->dmacsr[1]);
						return CE_PROMPT;
					}
				}

//if (gigal_regs->balance) printf(" balance\n");
				if (protocol_error(-1, 0)) return CEN_PROMPT;
				if (!lp && (pciconf.errlog == 2)) printf("wr wait loops =%u\n", i);
			}
		}

		if (mode&2) {
			*addr=0;
			for (blk=0; blk < blks; blk++) {
				gigal_regs->d0_bc=0;
				gigal_regs->t_hdr=HW_BE|HW_L_DMD|HW_R_BUS|HW_EOT|HW_FIFO|HW_BT|THW_SO_DA;
				gigal_regs->t_ad =(u_int)data;
if (blen > 0x1000L)
				plx_regs->dma[0].dmadpr=(dmadesc_phy[_BLENPGS_>>8] +(_BLENPGS_&0xFF)*sizeof(PLX_DMA_DESC)) |0x9;

				plx_regs->dmacsr[0]=0x3;								// start DMA
				gigal_regs->t_da =blen;
				i=0;
				while (1) {
					_delay_(5000L);
					if ((plx_regs->dmacsr[0] &0x10)) break;

					if (!++i) {
						printf("\r%10lu\n", lp);
						printf("DMA0 not terminated CSR=%02X\n", plx_regs->dmacsr[0]);
						protocol_error(-1, "1");
						printf("len: %03X\n", (u_int)gigal_regs->d0_bc);
						return CEN_PROMPT;
					}
				}

				if (protocol_error(lp, 0)) return CE_PROMPT;

				if (gigal_regs->d0_bc != blen) {
					printf("\r%10lu: illegal len %05lX\n", lp, gigal_regs->d0_bc);
					return CE_PROMPT;
				}

				if (!lp && (pciconf.errlog == 2)) printf("rd wait loops =%u\n", i);
				if (mode&1) {
					for (u=0; u < blen/4; u++) {
						x=DBUF_L(u);
						if (!u) y=blk;
						else y=din;

						if (x != y) {
							printf("\r%10lu: error addr=%06lX/%05lX, exp=%08lX, is=%08lX\n",
									 lp, ((u_long)blk *blen) +(u<<2),
									 (((u_long)blk *blen)>>1) +(u<<1),
									 y, x);
							*addr=(((u_long)blk *blen)>>1) +(u<<1);
							printf("read again %08lX\n", *(u_long*)data);
							return CE_PROMPT;
						}

						din +=_DTEST_INC;
					}
				}
			}
		}

		lp++;
		printf("\r%10lu", lp);
		if (kbhit()) {
			u_long 	tm;
			double	sec;
			double	mb;

			Writeln();
			tm=ZeitBin();
			if (!tm || !lp) return CE_PROMPT;

			sec=tm /100.0;
			mb =1.0*lp*mlen/1000000.;
			if (mode == 3) mb *=2.;
			printf("time = %4.2f sec, one loop = %4.2f sec\n", sec, sec/lp);
			printf("Rate = %1.3f MByte per sec\n", mb /sec);
			return CE_PROMPT;
		}
	}
}

#else
static u_long	blen=0x1000;

int sram_dtest(
	u_long	*addr,
	u_short	*data,
	u_long	mlen)
{
	u_int		blk;
	u_int		blks;
	u_int		pgs;
	u_long	dout,din;
	u_long	x,y;
	u_long	lp;
	u_long	u;
	u_int		i;
	u_char	mode;

	printf("mode (rd:2, wr:1) %3u ", config.srtst_mode);
	config.srtst_mode=(u_int)Read_Deci(config.srtst_mode, 3);
	mode=config.srtst_mode &3;
	if (!mode || errno) return CEN_MENU;

	printf("DMA block len  %06lX ", blen);
	blen=Read_Hexa(blen, -1);
	if ((blen < 4) || errno) return CEN_MENU;

	pgs=(u_int)((blen+SZ_PAGE-1)/SZ_PAGE);
	blks=(u_int)(mlen/blen);
	if (dmapages < pgs) {
		printf("it will use min 0x%02X DMA pages\n", pgs);
		return CE_NO_DMA_BUF;
	}

	setup_dma(1, 0, 0, blen, ADDRSPACE1+(u_int)data, 0, 0);	// for SRAM write
	setup_dma(-1, pgs, 0, blen, 0, 0, DMA_FASTEOT|DMA_L2PCI);							// for SRAM read
	lp=0;
	dout=0; din=0;
	if (!(mode&2)) {
		for (u=0; u < blen/4; u++) {
			DBUF_L(u) =dout; dout +=_DTEST_INC;
		}
	}

	timemark();
	while (1) {
		if (mode&1) {								// DMA write
			*addr=0;
			for (blk=0; blk < blks; blk++) {
				if (mode&2) {						// auch DMA read
					for (u=0; u < blen/4; u++) {
						DBUF_L(u) =dout; dout +=_DTEST_INC;
					}
				}

				dmabuf[0][0]=blk;
				if (blen > 0x1000L) plx_regs->dma[1].dmadpr=dmadesc_phy[0] |0x9;

				plx_regs->dmacsr[1]=0x3;								// start DMA
				i=0;
				while (1) {
					_delay_(5000L);
					if (plx_regs->dmacsr[1] &0x10) break;

					if (!++i) {
						printf("\r%10lu: DMA1 wr time out CSR=%02X\n",
								 lp, plx_regs->dmacsr[1]);
						return CE_PROMPT;
					}
				}

				if (bus_error()) return CEN_PROMPT;

				if (!lp && (pciconf.errlog == 2)) printf("wr wait loops =%u\n", i);
			}
		}

		if (mode&2) {
			lvd_regs->sr =lvd_regs->sr;
			*addr=0;
			for (blk=0; blk < blks; blk++) {
				if (blen > 0x1000L)
					plx_regs->dma[0].dmadpr=(dmadesc_phy[pgs>>8] +(pgs&0xFF)*sizeof(PLX_DMA_DESC)) |0x9;

				plx_regs->dmacsr[0]=0x3;								// start DMA
				lvd_regs->lvd_scan =0;
				lvd_regs->lvd_blk_max =blen;
				lvd_regs->lvd_block= (u_int)data/2;
				i=0;
				while (1) {
					_delay_(5000L);
					if ((plx_regs->dmacsr[0] &0x10)) break;

					if (!++i) {
						printf("\r%10lu\n", lp);
						printf("DMA0 not terminated CSR=%02X\n", plx_regs->dmacsr[0]);
						bus_error();
						printf("len: %04lX\n", lvd_regs->lvd_blk_cnt);
						return CEN_PROMPT;
					}
				}

				if (bus_error()) return CE_PROMPT;

				lvd_regs->sr=lvd_regs->sr;
				if (lvd_regs->lvd_blk_cnt != blen) {
					printf("\r%10lu: illegal len %05lX\n", lp, lvd_regs->lvd_blk_cnt);
					return CE_PROMPT;
				}

				if (!lp && (pciconf.errlog == 2)) printf("rd wait loops =%u\n", i);

				if (mode&1) {
					for (u=0; u < blen/4; u++) {
						x=DBUF_L(u);
						if (!u) y=blk;
						else y=din;

						if (x != y) {
							printf("\r%10lu: error addr=%06lX/%05lX, exp=%08lX, is=%08lX\n",
									 lp, ((u_long)blk *blen) +(u<<2),
									 (((u_long)blk *blen)>>1) +(u<<1),
									 y, x);
							*addr=(((u_long)blk *blen)>>1) +(u<<1);
							printf("read again %08lX\n", *(u_long*)data);
							return CE_PROMPT;
						}

						din +=_DTEST_INC;
					}
				}
			}
		}

		lp++;
		printf("\r%10lu", lp);
		if (kbhit()) {
			u_long 	tm;
			double	sec;
			double	mb;

			Writeln();
			tm=ZeitBin();
			if (!tm || !lp) return CE_PROMPT;

			sec=tm /100.0;
			mb =1.0*lp*mlen/1000000.;
			if (mode == 3) mb *=2.;
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
#define SB_JMPS	14
#define SB_JMP		3
#define SB_JSR		4
#define SB_RTS		5
#define SB_WHALT	6
#define SB_SW		11
#define SB_SWC		12
#define SB_WTRG	7
#define SB_TDBNZ	8
#define SB_COUNT	9
#define SB_DGAP	13
#define SB_TRG		10
#define SB_LPCNT	15

typedef struct {
	char	*cw;
	u_int	id;
} SQ_TOKEN;

const SQ_TOKEN sq_token[] ={
	{ "JMPS",SB_JMPS},
	{ "JMP",	SB_JMP},
	{ "JUMP",SB_JMP},
	{ "JSR", SB_JSR},
	{ "RTS", SB_RTS},
	{ "TDBNZ", SB_TDBNZ},
	{ "TB",  SB_TDBNZ},
	{ "LPCNT", SB_LPCNT},
	{ "LC",  SB_LPCNT},
	{ "WH",	SB_WHALT},
	{ "WHLT",SB_WHALT},
	{ "GAP",	SB_DGAP},
	{ "DGAP",SB_DGAP},
	{ "SW",	SB_SW},
	{ "SWC",	SB_SWC},
	{ "WTRG",SB_WTRG},
	{ "WTG",	SB_WTRG},
	{ "C",	SB_COUNT},
	{ "CNT",	SB_COUNT},
	{ "T",	SB_TRG},
	{ "TRG",	SB_TRG},
	{ 0, 0}
};

static int sram_inout(
	u_int		rd,	// read SRAM
	u_int		pg)	// page (=> 0x0800 SRAM words)
{
	u_int		i,err;
	u_short	*bf0,*bf1;

//printf("sram_inout(%d, 0x%X)\n", rd, pg);
	inp_unit->ADR_SRAM_ADR=((u_long)pg<<11) +addr_ofs;
		
	for (i=0, bf0=(u_short*)dmabuf[0]; i < 0x800; i++, bf0++) {
		if (rd) {
			*bf0=inp_unit->ADR_SRAM_DATA;
		} else {
			inp_unit->ADR_SRAM_DATA=*bf0;
		}
	}

	if (rd) return 0;

	inp_unit->ADR_SRAM_ADR=((u_long)pg<<11) +addr_ofs;
	for (i=0, bf1=(u_short*)dmabuf[0]+0x800; i < 0x800; i++, bf1++) {
		*bf1=inp_unit->ADR_SRAM_DATA;
	}

	bf0=(u_short*)dmabuf[0]; bf1=bf0+0x800; err=0;
	for (i=0; i < 0x800; i++)
		if (bf0[i] != bf1[i]) {
			printf("read/write error at %05lX: is=%04X exp=%04X\n",
					 ((u_long)pg <<11)+i, bf1[i], bf0[i]);
			if (++err > 10) return CE_PROMPT;
		}

	if (err) return CE_PROMPT;

	return 0;
}

//===========================================================================
//
//										Timing Generator
//
//===========================================================================
#define C_TB	16
#define C_RPST	8
#define C_TPAR	9

#define I_END		0
#define I_HIGH		1
#define I_LOW		2
#define I_REP		3
#define I_REPEND	4

#define S_VACLK	0
#define S_ASYNCH	1
#define S_HOLD		2
#define S_TESTOFF	3
#define S_CS		4
#define S_RESET	5
#define S_DIGTST	6

#define S_BUSY		8

static struct {
	u_short	ix,st;
	struct {
		u_short	inst,par,var,res;
	} tb[C_TB];
	u_short	rpst[C_RPST];
} tpar[C_TPAR];

static u_int	vtime,qval;
static u_int	qlcnt,qlval;

static void tg_dump(void)
{
	u_int	n,i;

	printf("vtime %u; qval %04X; qlcnt %u; qlval %04X\n", vtime, qval, qlcnt, qlval);
	for (n=0; n < C_TPAR; n++) {
		printf("%2u:%2u %2u ", n, tpar[n].ix, tpar[n].st);
		for (i=C_RPST-4; i < C_RPST; i++) printf(" %2u", tpar[n].rpst[i]);
		printf("\n  ");
		for (i=0; i < 10; i++) {
			if (i == 5) printf("\n  ");
			else {
				if (i) printf(" |");
			}
			printf(" %2u %4u %4u",
					 tpar[n].tb[i].inst, tpar[n].tb[i].par, tpar[n].tb[i].var);
		}
		Writeln();
	}
}

static int tm_save(u_int val)
{
	u_short	*sram=(u_short*)dmabuf[0];

	sram[sb_pc++]=val;
	if (sb_pc < C_PAGE) return 0;

	if (sb_sadr >= (SQ_SRAM_LEN-C_PAGE)) { sb_pc=M_PAGE; return 0; }

	if (sram_inout(0, (u_int)(sb_sadr >>B_PAGE))) return CE_PROMPT;

	sb_sadr +=C_PAGE; sb_pc=0;
	if (sram_inout(1, (u_int)(sb_sadr >>B_PAGE))) return CE_PROMPT;

	return 0;
}

static int tm_next(u_int n)
{
	u_int	i,v;

	while (1) {
		if (tpar[n].ix >= C_TB) return 13;

		i=tpar[n].tb[tpar[n].ix].inst;
		if (i == I_END) return 0;

		tpar[n].tb[tpar[n].ix].var=tpar[n].tb[tpar[n].ix].par+vtime;
		if (i == I_HIGH) {
			qval |=(1<<n);
			return 0;
		}

		if (i == I_LOW) {
			qval &=~(1<<n);
			return 0;
		}

		if (i == I_REP) {
			if (tpar[n].st == 0) return 10;

			tpar[n].tb[tpar[n].ix].var=tpar[n].tb[tpar[n].ix].par;
			tpar[n].rpst[--tpar[n].st]=tpar[n].ix++;
			continue;
		}

		if (i == I_REPEND) {
			if (tpar[n].st == C_RPST) return 11;

			v=tpar[n].tb[tpar[n].rpst[tpar[n].st]].var;
			if (!v) return (n<<8)+14;

			v--;
			tpar[n].tb[tpar[n].rpst[tpar[n].st]].var=v;
			if (v) {
				tpar[n].ix=tpar[n].rpst[tpar[n].st]+1;
				continue;
			}

			tpar[n].st++;
			tpar[n].ix++;
			continue;
		}

		return 12;
	}
}

static int tm_generate(void)
{
	int	ret;
	u_int	i,is,tend;

	qlcnt=0; qval=0;
	for (i=0; i < C_TPAR; i++) {
		tpar[i].ix=0;
		if ((ret=tm_next(i)) != 0) return ret;
	}

	while (1) {
		tend=1;
		for (i=0; i < C_TPAR; i++) {
			is=tpar[i].tb[tpar[i].ix].inst;
			if (is == I_END) continue;

			if ((is == I_HIGH) || (is == I_LOW)) {
				if (tpar[i].tb[tpar[i].ix].var == vtime) {
					tpar[i].ix++;
					if ((ret=tm_next(i)) != 0) return ret;
				}

				if (tpar[i].tb[tpar[i].ix].inst != I_END) tend=0;
				continue;
			}

			return 20;
		}

      vtime++;
		if (tend || ((qlcnt != 0) && (qlval != qval))) {
			if (qlcnt <= 32) {
				if ((ret=tm_save(((OPC_OUT|(qlcnt-1))<<B_DATA)|qlval)) != 0) return ret;
			} else {
				if ((ret=tm_save((OPC_OUT<<B_DATA)|qlval)) != 0) return ret;
				qlcnt--;
				while (1) {
					if (qlcnt > M_GAP) {
						if ((ret=tm_save((OPC_DGAP<<B_DATA)|M_GAP)) != 0) return ret;
						qlcnt -=(M_GAP+1);
						continue;
					}

					if ((ret=tm_save((OPC_DGAP<<B_DATA)|(qlcnt-1))) != 0) return ret;
					break;
				}
			}

			qlcnt=0;
		}

		qlval=qval; qlcnt++;

		if (tend) return 0;

	}
}

static int timing_generator(void)
{
	u_int	i,tend;
	u_int	fh2t;		// falling Hold to TestOn
	u_int	stm[18];

	for (i=0; i < 13;) {
		if (i == 4) config.vtx_tm[4]=(config.vtx_tm[2]-config.vtx_tm[1])/2;

		printf("tpar %2u: %5u ", i, (u_int)config.vtx_tm[i]);
		config.vtx_tm[i]=(u_int)Read_Deci(config.vtx_tm[i], 0xFFFF);
		if (errno) return -1;

		if (config.vtx_tm[i] %10) {
			printf("time must be a multiple of 10\n");
			use_default=0; continue;
		}

		if (!config.vtx_tm[i] && (i != 3) && (i != 5) && (i != 6) && (i != 7) && (i != 11) && (i != 12)) {
			printf("zero not allowed\n");
			use_default=0; continue;
		}

		if ((i == 3) && (config.vtx_tm[i] %config.vtx_tm[0])) {
			printf("must be a multiple of tpar 0\n");
			use_default=0; continue;
		}

      stm[i]=config.vtx_tm[i]/10;
		i++;
	}

	for (i=0; i < 9; i++) {
		tpar[i].ix=0; tpar[i].st=C_RPST; tpar[i].tb[0].inst=I_END;
	}

	fh2t=stm[5]+stm[6]+stm[11];
	tend=stm[7]+stm[8]+stm[4]+5*32*stm[0]+stm[0]+4*stm[3]+stm[9]+fh2t;
	if (stm[7]) {
		tpar[S_HOLD].tb[tpar[S_HOLD].ix].inst=I_LOW;
		tpar[S_HOLD].tb[tpar[S_HOLD].ix++].par=stm[7];
	}
	tpar[S_HOLD].tb[tpar[S_HOLD].ix].inst=I_HIGH;
	tpar[S_HOLD].tb[tpar[S_HOLD].ix++].par=tend-(stm[7]+fh2t);
	tpar[S_HOLD].tb[tpar[S_HOLD].ix].inst=I_LOW;
	tpar[S_HOLD].tb[tpar[S_HOLD].ix++].par=1;
	tpar[S_HOLD].tb[tpar[S_HOLD].ix].inst=I_END;

	tpar[S_TESTOFF].tb[tpar[S_TESTOFF].ix].inst=I_LOW;
	tpar[S_TESTOFF].tb[tpar[S_TESTOFF].ix++].par=stm[7]+stm[8];
	tpar[S_TESTOFF].tb[tpar[S_TESTOFF].ix].inst=I_HIGH;
	tpar[S_TESTOFF].tb[tpar[S_TESTOFF].ix++].par=tend-(stm[7]+stm[8]);
	tpar[S_TESTOFF].tb[tpar[S_TESTOFF].ix].inst=I_LOW;
	tpar[S_TESTOFF].tb[tpar[S_TESTOFF].ix++].par=1;
	tpar[S_TESTOFF].tb[tpar[S_TESTOFF].ix].inst=I_END;

	tpar[S_CS].tb[tpar[S_CS].ix].inst=I_LOW;
	tpar[S_CS].tb[tpar[S_CS].ix++].par=stm[7]+stm[8];
	tpar[S_CS].tb[tpar[S_CS].ix].inst=I_HIGH;
	tpar[S_CS].tb[tpar[S_CS].ix++].par=stm[2];
	tpar[S_CS].tb[tpar[S_CS].ix].inst=I_LOW;
	tpar[S_CS].tb[tpar[S_CS].ix++].par=1;
	tpar[S_CS].tb[tpar[S_CS].ix].inst=I_END;

	i=stm[7]+stm[8]+stm[4];
	i=i-(i/stm[0])*stm[0];	// very first time to synchronize ADC clock
	tpar[S_ASYNCH].tb[tpar[S_ASYNCH].ix].inst=I_LOW;
	tpar[S_ASYNCH].tb[tpar[S_ASYNCH].ix++].par=i;
	tpar[S_ASYNCH].tb[tpar[S_ASYNCH].ix].inst=I_HIGH;
	tpar[S_ASYNCH].tb[tpar[S_ASYNCH].ix++].par=1;
	tpar[S_ASYNCH].tb[tpar[S_ASYNCH].ix].inst=I_LOW;
	tpar[S_ASYNCH].tb[tpar[S_ASYNCH].ix++].par=1;
	tpar[S_ASYNCH].tb[tpar[S_ASYNCH].ix].inst=I_END;

	tpar[S_VACLK].tb[tpar[S_VACLK].ix].inst=I_LOW;
	tpar[S_VACLK].tb[tpar[S_VACLK].ix++].par=stm[7]+stm[8]+stm[4];
	tpar[S_VACLK].tb[tpar[S_VACLK].ix].inst=I_REP;
	tpar[S_VACLK].tb[tpar[S_VACLK].ix++].par=4;
	tpar[S_VACLK].tb[tpar[S_VACLK].ix].inst=I_REP;
	tpar[S_VACLK].tb[tpar[S_VACLK].ix++].par=32;
	tpar[S_VACLK].tb[tpar[S_VACLK].ix].inst=I_HIGH;
	tpar[S_VACLK].tb[tpar[S_VACLK].ix++].par=stm[1];
	tpar[S_VACLK].tb[tpar[S_VACLK].ix].inst=I_LOW;
	tpar[S_VACLK].tb[tpar[S_VACLK].ix++].par=stm[0]-stm[1];
	tpar[S_VACLK].tb[tpar[S_VACLK].ix++].inst=I_REPEND;
	if (stm[3]) {
		tpar[S_VACLK].tb[tpar[S_VACLK].ix].inst=I_LOW;
		tpar[S_VACLK].tb[tpar[S_VACLK].ix++].par=stm[3];
	}
	tpar[S_VACLK].tb[tpar[S_VACLK].ix++].inst=I_REPEND;
	tpar[S_VACLK].tb[tpar[S_VACLK].ix].inst=I_REP;
	tpar[S_VACLK].tb[tpar[S_VACLK].ix++].par=33;
	tpar[S_VACLK].tb[tpar[S_VACLK].ix].inst=I_HIGH;
	tpar[S_VACLK].tb[tpar[S_VACLK].ix++].par=stm[1];
	tpar[S_VACLK].tb[tpar[S_VACLK].ix].inst=I_LOW;
	tpar[S_VACLK].tb[tpar[S_VACLK].ix++].par=stm[0]-stm[1];
	tpar[S_VACLK].tb[tpar[S_VACLK].ix++].inst=I_REPEND;
	tpar[S_VACLK].tb[tpar[S_VACLK].ix].inst=I_END;

	if (stm[6]) {
		tpar[S_RESET].tb[tpar[S_RESET].ix].inst=I_LOW;
		tpar[S_RESET].tb[tpar[S_RESET].ix++].par=tend-fh2t;
		tpar[S_RESET].tb[tpar[S_RESET].ix].inst=I_HIGH;
		tpar[S_RESET].tb[tpar[S_RESET].ix++].par=stm[6];
		tpar[S_RESET].tb[tpar[S_RESET].ix].inst=I_LOW;
		tpar[S_RESET].tb[tpar[S_RESET].ix++].par=1;
		tpar[S_RESET].tb[tpar[S_RESET].ix].inst=I_END;
	}


	if (stm[12]) {
		tpar[S_DIGTST].tb[tpar[S_DIGTST].ix].inst=I_HIGH;
		tpar[S_DIGTST].tb[tpar[S_DIGTST].ix++].par=stm[12];
		tpar[S_DIGTST].tb[tpar[S_DIGTST].ix].inst=I_LOW;
		tpar[S_DIGTST].tb[tpar[S_DIGTST].ix++].par=1;
		tpar[S_DIGTST].tb[tpar[S_DIGTST].ix].inst=I_END;
	}

	if (stm[7]) {
		tpar[S_BUSY].tb[tpar[S_BUSY].ix].inst=I_LOW;
		tpar[S_BUSY].tb[tpar[S_BUSY].ix++].par=stm[7];
	}
	tpar[S_BUSY].tb[tpar[S_BUSY].ix].inst=I_HIGH;
	tpar[S_BUSY].tb[tpar[S_BUSY].ix++].par=tend+stm[10]-stm[7];
	tpar[S_BUSY].tb[tpar[S_BUSY].ix].inst=I_LOW;
	tpar[S_BUSY].tb[tpar[S_BUSY].ix++].par=1;
	tpar[S_BUSY].tb[tpar[S_BUSY].ix].inst=I_END;

//	tg_dump();
	return tm_generate();
}

static int timing_generator_tst(void)
{
	u_int	i;
	u_int	stm[18];

	for (i=12; i < 18;) {
		printf("tpar %2u: %5u ", i, (u_int)config.vtx_tm[i]);
		config.vtx_tm[i]=(u_int)Read_Deci(config.vtx_tm[i], 0xFFFF);
		if (errno) return -1;

		if (config.vtx_tm[i] %10) {
			printf("time must be a multiple of 10\n");
			use_default=0; continue;
		}

		if (!config.vtx_tm[i] && ((i == 14) || (i == 15))) {
			printf("zero not allowed\n");
			use_default=0; continue;
		}

		if ((i == 14) && (config.vtx_tm[i] < 2)) {
			printf("must be greater or equal 20\n");
			use_default=0; continue;
		}

		stm[i]=config.vtx_tm[i]/10;
		i++;
	}

	for (i=0; i < 9; i++) {
		tpar[i].ix=0; tpar[i].st=C_RPST; tpar[i].tb[0].inst=I_END;
	}

	if (stm[17]) {
		if (stm[16]) {
			tpar[S_HOLD].tb[tpar[S_HOLD].ix].inst=I_LOW;
			tpar[S_HOLD].tb[tpar[S_HOLD].ix++].par=stm[16];
		}
		tpar[S_HOLD].tb[tpar[S_HOLD].ix].inst=I_HIGH;
		tpar[S_HOLD].tb[tpar[S_HOLD].ix++].par=stm[17];
		tpar[S_HOLD].tb[tpar[S_HOLD].ix].inst=I_LOW;
		tpar[S_HOLD].tb[tpar[S_HOLD].ix++].par=1;
		tpar[S_HOLD].tb[tpar[S_HOLD].ix].inst=I_END;
	}

	tpar[S_ASYNCH].tb[tpar[S_ASYNCH].ix].inst=I_HIGH;
	tpar[S_ASYNCH].tb[tpar[S_ASYNCH].ix++].par=1;
	tpar[S_ASYNCH].tb[tpar[S_ASYNCH].ix].inst=I_LOW;
	tpar[S_ASYNCH].tb[tpar[S_ASYNCH].ix++].par=stm[14]-1;
	tpar[S_ASYNCH].tb[tpar[S_ASYNCH].ix].inst=I_HIGH;
	tpar[S_ASYNCH].tb[tpar[S_ASYNCH].ix++].par=stm[15];
	tpar[S_ASYNCH].tb[tpar[S_ASYNCH].ix].inst=I_LOW;
	tpar[S_ASYNCH].tb[tpar[S_ASYNCH].ix++].par=1;
	tpar[S_ASYNCH].tb[tpar[S_ASYNCH].ix].inst=I_END;

	if (stm[12]) {
		if (stm[13]) {
			tpar[S_DIGTST].tb[tpar[S_DIGTST].ix].inst=I_LOW;
			tpar[S_DIGTST].tb[tpar[S_DIGTST].ix++].par=stm[13];
		}
		tpar[S_DIGTST].tb[tpar[S_DIGTST].ix].inst=I_HIGH;
		tpar[S_DIGTST].tb[tpar[S_DIGTST].ix++].par=stm[12];
		tpar[S_DIGTST].tb[tpar[S_DIGTST].ix].inst=I_LOW;
		tpar[S_DIGTST].tb[tpar[S_DIGTST].ix++].par=1;
		tpar[S_DIGTST].tb[tpar[S_DIGTST].ix].inst=I_END;
	}

	i=stm[12]+stm[13];
	if (stm[14]+stm[15] > i) i=stm[14]+stm[15];
	if (stm[16]+stm[17] > i) i=stm[16]+stm[17];
	tpar[S_BUSY].tb[tpar[S_BUSY].ix].inst=I_HIGH;
	tpar[S_BUSY].tb[tpar[S_BUSY].ix++].par=i+100;
	tpar[S_BUSY].tb[tpar[S_BUSY].ix].inst=I_LOW;
	tpar[S_BUSY].tb[tpar[S_BUSY].ix++].par=1;
	tpar[S_BUSY].tb[tpar[S_BUSY].ix].inst=I_END;

//	tg_dump();
	return tm_generate();
}

//===========================================================================
//
//										Sequencer Editor
//
//===========================================================================

//--------------------------- seq_help --------------------------------------
//
static int seq_help(void)
{

	printf("SRAM L„nge ist %lX (%u k) word\n", SQ_SRAM_LEN, (u_int)(SQ_SRAM_LEN >>10));
	printf("\nOpcode Kennungen:\n");
	printf("  JMP n   n muss Vielfaches von 0x40\n");
	printf("  JMPS n  n darf sich nur in den unteren 11 Bit aendern\n");
	printf("  JSR n   n muss Vielfaches von 0x40\n");
	printf("  RTS\n");
	printf("  LC n c  lade loop counter n(2,3) mit Wert c\n");
	printf("  TB n a  test, dec counter n(0..3), branch no zero a\n");
	printf("  WH      warte, wenn HALT Signal gesetzt ist\n");
	printf("  SW r    switch reg \n");
	printf("  SWC r   switch reg r and clear enable\n");
	printf("  d WTG i warte auf Trigger i mit Datenwort d\n");
	printf("  GAP x   Ausgabe GAP x Takte\n");
	printf("  C i     inkrementiere Counter i\n");
	printf("  T i     geben Triggerinpuls i aus\n");
	printf("\nSonderformen\n");
	printf("#         Umschalten auf binaer Mode\n");
	printf("!         weiter bis Opcode kein Datencode\n");
	printf("10: 3*1F T1          1F mit Wiederholung und Trigger T1\n");
	printf("JMPS *+1             jump relativ von aktueller Adresse\n");
	printf("10(1, 0 T2, 20(1F))  verschachtelte Wiederholungen\n");
	printf("\n");

	return CEN_PROMPT;
}
//
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

//printf(" i=%d %08lX\n", i, sb_lw);
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

//--- suche Token

	tk=sq_token;
	do {
		i=0;
		while (tk->cw[i] && (tk->cw[i] == sbuf[sb_ix+i])) i++;
		if (!tk->cw[i] && (sbuf[sb_ix+i] <= '9')) {	// token gefunden
//printf("%s %c\n", tk->cw, sbuf[sb_ix+i]);
			sb_ix +=i;
			while (space(sbuf[sb_ix])) sb_ix++;

//printf("%c\n", sbuf[sb_ix]);
			if ((sbuf[sb_ix] == ')') || (sbuf[sb_ix] == ',')) {
				sb_id=tk->id;
				return 0;
			}

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
	u_int		l;
	u_char	opc,mul;
	u_char	err;

	opc=*sram >>B_DATA; err=0;
	l=sprintf(ibuf, "%05lX: ", pc);
	if (   ((opc &~0x01) == OPC_TRG)
		 || ((opc &~0x01) == OPC_WTRG)
		 || ((opc &~0x1F) == OPC_OUT)) {	// data

		mul=0;
		if ((opc &~0x1F) == OPC_OUT) mul=(*sram >>B_DATA) &M_DREPT;

		if (mul) l +=sprintf(ibuf+l, "%2u*", mul+1);
		else l +=sprintf(ibuf+l, "   ");

		l +=sprintf(ibuf+l, "%03X", *sram &M_DATA);

	} else l +=sprintf(ibuf+l, "           ");

	switch (opc &0x78) {
case 0x00:
		if ((opc &~0x1) == OPC_WTRG) {
			l +=sprintf(ibuf+l, " WTG%u", opc&1);
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

		if ((opc &~0x1) == OPC_RES) {
			err=11;
			break;
		}

		if ((opc &~0x1) == OPC_COUNT) {
			l +=sprintf(ibuf+l, " C%u", opc &1);
			break;
		}

		err=12;
		break;

case OPC_DGAP:
		l +=sprintf(ibuf+l, " DGAP %4u", (*sram &M_GAP)+1);
		break;

case OPC_SW:
case OPC_SW+0x08:
		l +=sprintf(ibuf+l, (opc &0x08) ? " SWC %u" : " SW %u", opc &0x07);
		break;

case 0x20:
		if ((opc &~0x1) == OPC_TRG) {
			l +=sprintf(ibuf+l, " T%u", opc &1);
			break;
		}

		if ((opc &~0x1) == OPC_LPCNT) {
			l +=sprintf(ibuf+l, " LC%u %3u", (opc &1)+2, *sram &0xFF);
			break;
		}

		if ((opc &~0x3) == OPC_JMPS) {
			l +=sprintf(ibuf+l, " JMPS %05lX", (pc&~M_JMPS)|(*sram &M_JMPS));
			break;
		}

		err=14;
		break;

case OPC_TDJNZ:
case OPC_TDJNZ+0x8:
		l +=sprintf(ibuf+l, " TDJNZ %u %05lX", (opc>>2) &3, (pc&~M_JMPS)|(*sram &M_JMPS));
		break;

case OPC_JMP:
case OPC_JMP+0x8:
		l +=sprintf(ibuf+l, " JMP %05lX", (u_long)(*sram &M_GAP) <<B_EJMP);
		break;

case OPC_JSR:
case OPC_JSR+0x8:
		l +=sprintf(ibuf+l, " JSR %05lX", (u_long)(*sram &M_GAP) <<B_EJMP);
		break;

case OPC_OUT:
case OPC_OUT+0x08:
case OPC_OUT+0x10:
case OPC_OUT+0x18:
		break;

default:
		err=15;
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
	u_short	*sram=(u_short*)dmabuf[0];
	u_int		sw;
	int		err;
	char		nopc;		// opcode speichern
	u_short	new=0;
	u_char	raw=0;

	if (dmapages < 2) {
		printf("it will use min 2 DMA pages\n");
		return CE_NO_DMA_BUF;
	}

	sb_sadr=0; sb_pc=0;
	if (sram_inout(1, (u_int)(sb_sadr >>B_PAGE))) return CE_PROMPT;

	while (1) {
		if (raw) printf("%05lX: %04X =>", sb_sadr+sb_pc, sram[sb_pc]);
		else {
			i=seq_inst(sb_sadr+sb_pc, sram+sb_pc);
			ibuf[i]=0;
			printf("%-30s =>", ibuf);
		}

		use_default=0;
		strcpy(sbuf, Read_String(0, sizeof(sbuf)));
		for (i=0; sbuf[i]; i++) sbuf[i]=toupper(sbuf[i]);

		if (errno) {
			sram_inout(0, (u_int)(sb_sadr >>B_PAGE));
			return CE_PROMPT;
		}

		if (!sbuf[1]) {
			if (sbuf[0] == '?')  {
				Writeln(); seq_help();
				continue;
			}

			if (sbuf[0] == '#')  {
				raw=!raw;
				continue;
			}

			if (sbuf[0] == '!')  {
				while ((*(sram+sb_pc) &0xC000) == 0x8000) {
					sb_pc++;
					if (sb_pc >= C_PAGE) {
						if (sb_sadr >= (SQ_SRAM_LEN-C_PAGE)) { sb_pc=M_PAGE; break; }

						if (sram_inout(0, (u_int)(sb_sadr >>B_PAGE))) return CE_PROMPT;

						sb_sadr +=C_PAGE; sb_pc=0;
						if (sram_inout(1, (u_int)(sb_sadr >>B_PAGE))) return CE_PROMPT;
					}
				}

				continue;
			}

			if (sbuf[0] == 'U')  {
				if ((err=timing_generator_tst()) == 0) continue;

				if (err >= 0) {
					printf("timing generator error: %u/%u\n", err>>8, (u_char)err);
					tg_dump();
				}

				return CE_PROMPT;
			}

			if (sbuf[0] == 'V')  {
				if ((err=timing_generator()) == 0) continue;

				if (err >= 0) {
					printf("timing generator error: %u/%u\n", err>>8, (u_char)err);
					tg_dump();
				}

				return CE_PROMPT;
			}

		}

		sb_ix=0;
		if (raw) {
			new=0; nopc=-1;
			if ((err=seq_parser()) == 0) {
//printf("a %u %u %08lX\n", sb_id, sb_val, sb_lw);
				switch (sb_id) {
			default:
					err=21;
					break;

			case SB_PC:
					if (sb_lw >= SQ_SRAM_LEN) { err=23; break; }

					if (sb_sadr != (sb_lw &~M_PAGE)) {
						if (sram_inout(0, (u_int)(sb_sadr >>B_PAGE))) { err=100; break; }

						sb_sadr=sb_lw &~M_PAGE;
						if (sram_inout(1, (u_int)(sb_sadr >>B_PAGE))) { err=101; break; }
					}

					sb_pc=(u_int)sb_lw &M_PAGE; nopc=0;
					break;

			case SB_VAL:
					if ((sb_lw > 0x10000L) || sb_val) err=24;

					new=(u_short)sb_lw; nopc=1;
					break;
				}

				if (err) {
					printf("syntax error %d: %u %08lX\n", err, sb_ix, sb_lw);
					continue;
				}
			}

			if ((err=seq_parser()) != PA_EOF) {
				printf("syntax error %d: %u\n", err, sb_ix);
				continue;
			}

			if (nopc == 0) continue;

			if (nopc == 1) sram[sb_pc]=new;

			sb_pc++;

			if (sb_pc >= C_PAGE) {
				if (sb_sadr >= (SQ_SRAM_LEN-C_PAGE)) { sb_pc=M_PAGE; continue; }

				if (sram_inout(0, (u_int)(sb_sadr >>B_PAGE))) return CE_PROMPT;

				sb_sadr +=C_PAGE; sb_pc=0;
				if (sram_inout(1, (u_int)(sb_sadr >>B_PAGE))) return CE_PROMPT;
			}

			continue;
		}

//-----------------------------

		rtr_ix=0; sw=1;
		while (1) {
			new=0; nopc=0;
			while ((err=seq_parser()) == 0) {
//printf("b %u %u %08lX\n", sb_id, sb_val, sb_lw);
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

						if (sb_lw >= SQ_SRAM_LEN) { err=23; break; }

						if (sb_sadr != (sb_lw &~M_PAGE)) {
							if (sram_inout(0, (u_int)(sb_sadr >>B_PAGE))) { err=100; break; }

							sb_sadr=sb_lw &~M_PAGE;
							if (sram_inout(1, (u_int)(sb_sadr >>B_PAGE))) { err=101; break; }
						}

						sb_pc=(u_int)sb_lw &M_PAGE;
						sw=2;
						break;

				case SB_VAL:
						if ((sb_lw > M_DATA) || (sb_val > M_DREPT)) err=25;

						new=(u_short)sb_lw |OPC(OPC_OUT +sb_val);
						nopc=1;
						sw=3;
						break;

				case SB_JMPS:
						if (   (sb_lw >= SQ_SRAM_LEN)
							 || (((sb_sadr+sb_pc) ^sb_lw) &~M_JMPS)) err=27;

						new=((u_short)sb_lw &M_JMPS) |OPC(OPC_JMPS); nopc=1;
						sw=0;
						break;

				case SB_TDBNZ:
						if (sb_val >= 4) { err=28; break; }

						new=OPC(OPC_TDJNZ |(sb_val <<2)); nopc=1;
						sw=4;
						break;

				case SB_LPCNT:
						if ((sb_val < 2) || (sb_val >= 4)) { err=30; break; }

						new=OPC(OPC_LPCNT |(sb_val-2)); nopc=1;
						sw=5;
						break;

				case SB_JMP:
						if ((sb_lw &M_EJMP) || (sb_lw >= SQ_SRAM_LEN)) err=31;

						new=(u_short)(sb_lw >>B_EJMP) |OPC(OPC_JMP); nopc=1;
						sw=0;
						break;

				case SB_JSR:
						if ((sb_lw &M_EJMP) || (sb_lw >= SQ_SRAM_LEN)) err=32;

						new=(u_short)(sb_lw >>B_EJMP) |OPC(OPC_JSR); nopc=1;
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
						if (sb_val >= 8) { err=33; break; }

						new=OPC(OPC_SW |sb_val); nopc=1;
						sw=0;
						break;

				case SB_SWC:
						if (sb_val >= 8) { err=35; break; }

						new=OPC(OPC_SWC |sb_val); nopc=1;
						sw=0;
						break;

				case SB_COUNT:
						if (sb_val >= 2) { err=37; break; }

						new=OPC(OPC_COUNT |sb_val); nopc=1;
						sw=0;
						break;

				case SB_DGAP:
						sb_lw--;
						if (sb_lw > M_GAP) err=38;

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

				case SB_WTRG:
						if (sb_val >= 2) { err=41; break; }

						new =(new &M_DATA) |OPC(OPC_WTRG +sb_val); nopc=1;
						sw=0;
						break;

				case SB_TRG:
//printf(" %04X\n",new);
						if ((new >>B_DATA) &0x1F) { err=42; break; }

						if (sb_val >= 2) { err=43; break; }

						new =(new &M_DATA) |OPC(OPC_TRG |sb_val);
						sw=0;
						break;

					}

					break;

			case 4:							// TDBNZ function
					if (sb_id == SB_VAL) {
						if (   (sb_lw >= SQ_SRAM_LEN)
							 || (((sb_sadr+sb_pc) ^sb_lw) &~M_JMPS)) err=44;

						new |=((u_short)sb_lw &M_JMPS); nopc=1;
						sw=0;
						break;
					}

			case 5:							// LPCNT function
					if (sb_id == SB_VAL) {
						if (sb_lw >= 0x100) err=45;

						new |=(u_short)sb_lw; nopc=1;
						sw=0;
						break;
					}

					err=46;
					break;

				}

				if (err) break;
			}

			if (err >= 0) {
				printf("syntax error %d: i:%u id:%u %08lX\n", err, sb_ix, sb_id, sb_lw); break;
			}

			if (nopc) { sram[sb_pc]=new; sb_pc++; }
			else {
				if ((sw == 1) && (err == PA_EOF)) sb_pc++;
			}

			if (sb_pc >= C_PAGE) {
				if (sb_sadr >= (SQ_SRAM_LEN-C_PAGE)) { sb_pc=M_PAGE; break; }

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
	int		ret;
	int		hx;
	u_int		l;			// byte count
	u_long	ad;		// short
	u_long	lx;		// byte

	printf("save to ");
	strcpy(config.sq_save_file,
			 Read_String(config.sq_save_file, sizeof(config.sq_save_file)));
	if (errno) return CEN_MENU;

	printf("end address %05lX ", config.save_end);
	config.save_end=Read_Hexa(config.save_end, -1);
	if (errno) return CEN_MENU;

	if (config.save_end > SQ_SRAM_LEN) config.save_end=SQ_SRAM_LEN;
	if ((hx=_rtl_creat(config.sq_save_file, 0)) == -1) {
		perror("error creating file");
		return CEN_PROMPT;
	}

	ret=CEN_MENU; ad=addr_ofs; l=0x8000; lx=config.save_end <<1;
	if (!lx) lx=SQ_SRAM_LEN<<1;
	do {
		if ((u_long)l > lx) l=(u_int)lx;
		if (sram_copyin(inp_unit, 0x0, l, ad)) { ret=CEN_PROMPT; break; }

		if (write(hx, dmabuf[0], l) != l) {
			perror("error writing file");
			ret=CEN_PROMPT; break;
		}

		lx-=l; ad+=(l>>1);
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
int seq_load(VTEX_RG	*_inp_unit, char *fname, char hv)
{
	int		hx;
	u_int		l;			// byte count
	u_long	ad;		// short
	u_long	lx;		// byte
	int		ret=0;

	if (dmapages < 16) return CE_NO_DMA_BUF;

	inp_unit=_inp_unit;
	if (!fname) {
		printf("load from ");
		strcpy(config.sq_load_file,
				 Read_String(config.sq_load_file, sizeof(config.sq_load_file)));
		if (errno) return CEN_MENU;

		fname=config.sq_load_file;
	}

	if ((hx=_rtl_open(fname, O_RDONLY)) == -1) {
		perror("error opening file");
		return CEN_PROMPT;
	}

	ad=(hv) ? C_HV_SRAM_OFFSET : 0; lx=0;
	do {
		if ((l=read(hx, dmabuf[0], 0x8000)) == 0xFFFF) {
			perror("error reading file");
			ret=CEN_PROMPT;
			break;
		}

//printf("%06lX %04X\n",ad,l);
		if (!l) break;

		if (sram_copyout(inp_unit, 0x0, l, ad)) {
			ret=CEN_PROMPT;
			break;
		}

		lx+=l; ad+=(l>>1);
	} while ((l == 0x8000) && (lx < (SQ_SRAM_LEN<<1)));

	printf("0x%06lX SRAM words loaded\n", lx>>1);
	if (close(hx) == -1) {
		perror("error closing file");
		return CEN_PROMPT;
	}

	return ret;
}
//
//--------------------------- seq_fill --------------------------------------
//
static int seq_fill(void)
{
	u_short	*sram=(u_short*)dmabuf[0];
	u_int		i;
	u_long	adr;

	printf("data  %04X ", config.srf_lw);
	config.srf_lw=(u_short)Read_Hexa(config.srf_lw, -1);
	if (errno) return CEN_MENU;

	for (i=0; i < 0x8000; i++) sram[i]=config.srf_lw;

	for (i=0, adr=addr_ofs; i < (2*SQ_SRAM_LEN)/0x10000L; i++, adr+=0x8000L)
		if (sram_copyout(inp_unit, 0, 0x10000L, adr)) return CEN_PROMPT;

	return CEN_MENU;
}
//
//--------------------------- seq_simu --------------------------------------
//
#define C_STACK	0x10
static u_long	stack[C_STACK];
static char		debug=0;

static int seq_simu(void)
{
	u_short	*sram=(u_short*)dmabuf[0];
	u_int		i;
	u_int		opcd,opc;
	int		err=0;
	char		key;
	u_char	swreg[8]={0};
	int		csw=-2;
	u_char	halt=1;
	u_char	whlt=0;
	u_int		sp=0;
	u_long	newpc;
	u_long	count[2]={0};
	u_long	trig[2]={0};
	u_int		scnt[4]={3,3,3,3};
	u_int		cnt[4]={3,3,3,3};
	u_int		itrig=0;
	char		ctrg=0;
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
		opcd=sram[(u_int)sb_sadr &M_PAGE];
		opc=opcd >>B_DATA;
		newpc=sb_sadr+1;
#if 1
if (debug > 1) {
	printf("%06lX: %02X\n", sb_sadr, opc);
	if (++ln >= 40) {
		ln=0;
		if (WaitKey(0)) debug=1;
	}
}
#endif
		switch (opc >>3) {
	default:
			err=1;
			break;

	case 0x00:
			if ((opc &~1) == OPC_WTRG) {
				if (!(opc &1) && (itrig &1)) {
					itrig &=~1;
					break;
				}

				if ((opc &1) && (itrig &2)) {
					itrig &=~2;
					break;
				}

				newpc=sb_sadr;
				break;
			}

			if (opcd &M_DATA) err=2;

			if (opc == OPC_RTS) {

				if (sp == 0) {
					printf("%05lX: stack underflow\n", sb_sadr);
					err=-1; break;
				}

				newpc=stack[--sp];
				if (debug) {
					printf("rts %u: %05lX -> %05lX\n", sp, sb_sadr, newpc);
					if (++ln >= 40) {
						ln=0;
						if (WaitKey(0)) debug=0;
					}
				}
				break;
			}

			if (opc == OPC_WHALT) {
				if (halt) { newpc=sb_sadr; whlt=1; break; }

				whlt=0;
				break;
			}

			if ((opc &~1) == OPC_COUNT) {
				count[opc &1]++;
				break;
			}

			err=3;
			break;

	case OPC_DGAP>>3:
			break;


	case OPC_SW>>3:
	case (OPC_SW>>3)+1:
			if (opcd &M_DATA) err=4;

			if (!(swreg[opc &0x7] &4)) { newpc +=4; break; }

			newpc +=(swreg[opc &0x7] &0x3);
			if (!(opc &0x08)) break;

			swreg[opc &0x7] &=0x3;		// OPC_SWC
			break;

	case OPC_TRG>>3:
			if ((opc &~1) == OPC_TRG) {
				trig[opc &1]++;
				break;
			}

			if ((opc &~1) == OPC_LPCNT) {
				scnt[2+(opc &1)] =opcd &0xFF;
				cnt[2+(opc &1)] =opcd &0xFF;
				break;
			}

			if ((opc &~3) == OPC_JMPS) {
				newpc=(newpc &~M_JMPS) |(opcd &M_JMPS);
				break;
			}

			err=3;
			break;

	case OPC_TDJNZ>>3:
	case (OPC_TDJNZ>>3)+1:
			i=cnt[(opc>>2)&3]; cnt[(opc>>2)&3]--;
			if (i) {
				newpc=(newpc &~M_JMPS) |(opcd &M_JMPS);
				break;
			}

			cnt[(opc>>2)&3]=scnt[(opc>>2)&3];
			break;

	case OPC_JMP>>3:
	case (OPC_JMP>>3)+1:
			if (((u_long)(opcd &M_JMP) <<B_EJMP) >= SQ_SRAM_LEN) err=7;

			newpc=(u_long)(opcd &M_JMP) <<B_EJMP;
			break;

	case OPC_JSR>>3:
	case (OPC_JSR>>3)+1:
			if (((u_long)(opcd &M_JMP) <<B_EJMP) >= SQ_SRAM_LEN) err=8;

			if (sp >= C_STACK) {
				printf("%05lX: stack overflow\n", sb_sadr);
				err=-1; break;
			}

			stack[sp++]=newpc;
			newpc=(u_long)(opcd &M_JMP) <<B_EJMP;
			if (sp > spm) { spm=sp; printf("sp=%2d\n", spm); }
			if (debug) {
				printf("jsr %u: %05lX -> %05lX\n", sp-1, sb_sadr, newpc);
				if (++ln >= 40) {
					ln=0;
					if (WaitKey(0)) debug=0;
				}
			}

			break;

	case OPC_OUT>>3:
	case (OPC_OUT>>3)+1:
	case (OPC_OUT>>3)+2:
	case (OPC_OUT>>3)+3:
			break;

		}

		if (err) break;

//if (debug) { printf("%06lX/%06lX\n", sb_sadr, newpc); ln++; }
		if ((newpc &~M_PAGE) != (sb_sadr &~M_PAGE)) {
			if (newpc >= SQ_SRAM_LEN) {
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
				for (i=0; i < 2; i++) { count[i]=0; trig[i]=0; }
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
				if (ctrg) {
					itrig ^=(1 <<(key-'0'));
					ctrg=0; break;
				}

	case '2':
	case '3':
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

			ibuf[seq_inst(newpc, sram+((u_int)newpc &M_PAGE))]=0;
			printf("%s\n", ibuf);
			printf("itrig=%X", itrig);
			if (halt) printf(", halt");
			if (whlt) printf(", halted");
			printf(", switch=");
			for (i=0; i < 8; i++) printf("%X", swreg[7-i]);
			printf(", lpcnt=");
			for (i=0; i < 4; i++) printf(" %u/%u", scnt[3-i], cnt[3-i]);
			printf("\n");
			printf("counter(1:0) %08lX/%08lX\n",
					 count[1], count[0]);
			printf("trigger(1:0) %08lX/%08lX\n",
					 trig[1], trig[0]);

			if (err) break;
		}
	}

	Writeln();
	printf("error=%d\n", err);
	printf("pc:%06lX opc:%02X\n", newpc, opc);
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
static const char *sqsta_txt[] ={
	"Hlt",  "QHlt", "Qofl", "Qufl", "Qerr", "Qofe", "?", "?",
	"Hlt",  "QHlt", "Qofl", "Qufl", "Qerr", "Qofe", "?", "?"
};

static const char *lhv_txt[] ={
	"LV",  "HV"
};

#define P_CR	0
#define P_SQCR	1
#define P_LC0	2
#define P_LC1	3
#define P_SW	4

int seq_edit_menu(
	VTEX_RG	*_inp_unit,
	int		hv)
{
	int	ret;
	char	rkey;
	char	key;
	u_int	tmp;
	u_int	i,j;
	char	hgh;

	const char	*lhv;

	if (dmapages < 16) return CE_NO_DMA_BUF;

	inp_unit=_inp_unit;
	if (hv) {
		lhv=lhv_txt[1];
		pgrp=&inp_unit->hv;
		addr_ofs=C_HV_SRAM_OFFSET;
		squ_hlt=SEQ_CSR_HV_HALT;
		cntr=R_CNTRSTHV;
	} else {
		lhv=lhv_txt[0];
		addr_ofs=0;
		pgrp=&inp_unit->lv;
		squ_hlt=SEQ_CSR_LV_HALT;
		cntr=R_CNTRSTLV;
	}

	while (1) {
		Writeln();
		printf("%s sequencer editor ............... 0\n", lhv);
		printf("%s save SRAM to file .............. 1\n", lhv);
		printf("%s load SRAM from file ............ 2\n", lhv);
		printf("   toggle run ..................... r %s\n", ((gtmp=inp_unit->ADR_CR) &VCR_RUN) ? "run" : "");
		printf("%s toggle halt .................... h %s\n", lhv, (inp_unit->ADR_SEQ_CSR &squ_hlt) ? "halt" : "");
		printf("%s fill SRAM ...................... f\n", lhv);
		printf("   help ........................... ?\n");
		printf("%s cnt0/1 ............... %04X/%04X C\n", lhv, pgrp->ADR_COUNTER[0], pgrp->ADR_COUNTER[1]);
		printf("%s loop cnt0/1 .......... %04X/%04X 3/4\n", lhv, pgrp->ADR_LP_COUNTER[0], pgrp->ADR_LP_COUNTER[1]);
		printf("   control ................... %04X 5\n", inp_unit->ADR_CR);
		printf("   seq csr ................... %04X 6 ", tmp=inp_unit->ADR_SEQ_CSR);
		i=(hv) ? 16 : 8; hgh=0;
		for (j=0; j < 8; j++) {
			i--;
			if (tmp &(0x1 <<i)) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("%s", sqsta_txt[7-j]);
			}
		}
		lowvideo(); cprintf("\r\n");

		printf("%s switch registers ...... %08lX 7\n", lhv, pgrp->ADR_SWREG);
		printf("%s simulation ..................... s\n", lhv);
		printf("   SEQ SRAM test (single/DMA) ..... t/T\n");
		printf("   Table SRAM test (single/DMA) ... x/X\n");
		printf("   toggle high/low voltage ........ V\n");
		BUS_ERROR;
		printf("                                    %c ", config.c3_menu);

		rkey=getch();
		if ((rkey == CTL_C) || (rkey == ESC)) {
			return 0;
		}

		if (rkey >= ' ') printf("%c", rkey);

		Writeln();
		while (1) {
			use_default=(rkey == TAB);
			if ((rkey == TAB) || (rkey == CR)) rkey=config.c3_menu;

			key=toupper(rkey);
			errno=0; ret=CEN_MENU;
			Writeln();
			if (key == '0') {
				inp_unit->ADR_CR &=~VCR_RUN;
				ret=seq_edit();
			}

			if (key == '1') {
				inp_unit->ADR_CR &=~VCR_RUN;
				ret=seq_save();
			}

			if (key == '2') {
				inp_unit->ADR_CR &=~VCR_RUN;
				ret=seq_load(_inp_unit, 0, hv);
			}

			if (key == 'R') {
				inp_unit->ADR_CR ^=VCR_RUN;
			}

			if (key == 'H') {
				inp_unit->ADR_SEQ_CSR ^=squ_hlt;
			}

			if (key == 'F') {
				inp_unit->ADR_CR &=~VCR_RUN;
				ret=seq_fill();
			}

			if (key == '3') {
				printf("Value $%04X ", (u_int)config.ao_regs[P_LC0]);
				config.ao_regs[P_LC0]=(u_int)Read_Hexa(config.ao_regs[P_LC0], 0xFFFF);
				if (errno) break;

				pgrp->ADR_LP_COUNTER[0] =(u_short)config.ao_regs[P_LC0];
			}

			if (key == '4') {
				printf("Value $%04X ", (u_int)config.ao_regs[P_LC1]);
				config.ao_regs[P_LC1]=(u_int)Read_Hexa(config.ao_regs[P_LC1], 0xFFFF);
				if (errno) break;

				pgrp->ADR_LP_COUNTER[1] =(u_short)config.ao_regs[P_LC1];
			}

			if (key == '5') {
				printf("Value $%04X ", (u_int)config.ao_regs[P_CR]);
				config.ao_regs[P_CR]=(u_int)Read_Hexa(config.ao_regs[P_CR], 0xFFFF);
				if (errno) break;

				inp_unit->ADR_CR =(u_short)config.ao_regs[P_CR];
			}

			if (key == '6') {
				printf("Value $%04X ", (u_int)config.ao_regs[P_SQCR]);
				config.ao_regs[P_SQCR]=(u_int)Read_Hexa(config.ao_regs[P_SQCR], 0xFFFF);
				if (errno) break;

				inp_unit->ADR_SEQ_CSR =(u_short)config.ao_regs[P_SQCR];
			}

			if (key == '7') {
				printf("Value $%08lX ", config.ao_regs[P_SW]);
				config.ao_regs[P_SW]=Read_Hexa(config.ao_regs[P_SW], -1);
				if (errno) break;

				pgrp->ADR_SWREG =config.ao_regs[P_SW];
				ret=CE_MENU;
			}

			if (key == 'S') {
				inp_unit->ADR_CR &=~VCR_RUN;
				ret=seq_simu();
			}

			if (key == 'C') {
				inp_unit->ADR_ACTION=cntr;
			}

			if (key == '?') {
				ret=seq_help();
			}

			if (rkey == 't') {
				inp_unit->ADR_CR &=~VCR_RUN;
				ret=sram_test(&inp_unit->ADR_SRAM_ADR, &inp_unit->ADR_SRAM_DATA, 4*SQ_SRAM_LEN);
			}

			if (rkey == 'T') {
				inp_unit->ADR_CR &=~VCR_RUN;
				ret=sram_dtest(&inp_unit->ADR_SRAM_ADR, &inp_unit->ADR_SRAM_DATA, 4*SQ_SRAM_LEN);
			}

			if (rkey == 'x') {
				inp_unit->ADR_CR &=~VCR_RUN;
				ret=sram_test(&inp_unit->ADR_RAMT_ADR, &inp_unit->ADR_RAMT_DATA, 2*TB_SRAM_LEN);
			}

			if (rkey == 'X') {
				inp_unit->ADR_CR &=~VCR_RUN;
				ret=sram_dtest(&inp_unit->ADR_RAMT_ADR, &inp_unit->ADR_RAMT_DATA, 2*TB_SRAM_LEN);
			}

			if (key == 'V') {
				if (!hv) {
					hv=1;
					lhv=lhv_txt[1];
					addr_ofs=C_HV_SRAM_OFFSET;
					pgrp=&inp_unit->hv;
					squ_hlt=SEQ_CSR_HV_HALT;
					cntr=R_CNTRSTHV;
				} else {
					hv=0;
					lhv=lhv_txt[0];
					addr_ofs=0;
					pgrp=&inp_unit->lv;
					squ_hlt=SEQ_CSR_LV_HALT;
					cntr=R_CNTRSTLV;
				}
			}

			while (kbhit()) getch();
			if ((ret == CE_MENU) || (ret == CE_PROMPT)) config.c3_menu=rkey;

			if ((ret == CEN_MENU) || (ret == CE_MENU)) break;

			if (ret > CE_PROMPT) display_errcode(ret);

			printf("select> ");
			rkey=getch();
			key=toupper(rkey);
			if (rkey >= ' ') printf("%c", rkey);
			Writeln();

			if ((rkey < ' ') && (rkey != TAB)) break;

			if (rkey == ' ') rkey=CR;
		}
	}
}
