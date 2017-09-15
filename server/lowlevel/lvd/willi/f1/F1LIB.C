// Borland C++ - (C) Copyright 1991, 1992 by Borland International

/*	f1lib.c -- general functions  */

#include <stddef.h>		// offsetof()
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <io.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <mem.h>
#include <windows.h>
#include <fcntl.h>
#include "pci.h"
#include "f1.h"

static COUPLER_BC	*sc_bc;
static COUPLER		*sc_regs;
static IN_CARD_BC	*inp_bc;
static IN_CARD		*inp_regs;

static int			act_sc;
static u_short		gtmp;

long		f1_range;					// max value of time low word (6.4us)
long		tlat,twin;
u_long	eventcount=0;
u_long	*robf;
u_int		rout_len;
u_long	rout_trg_time;
u_long	rout_ev_inf;
char		rout_hdr;

u_long	kbatt;
u_long	kbatt_msk;
long		pause;
//
//--------------------------- setup_dma -------------------------------------
//
static void setup_dma(
	int		chn,	// -1: channel 0 demand mode
	u_int		dix,	// first descriptor index
	u_int		fpg,	// first page
	u_long	len,
	u_long	addr,
	u_long	ofs,	// offset
	u_int		mode)	// b0/01: l2pci
						// b1/02: no EOT
						// b2/04: fast EOT
						// b4/10: circular
{
	u_long	dmm;
	u_int		l2pci;
	u_int		pgs;
	u_int		ix;
	u_long	plen;
	u_int		fofs;
	u_int		pofs;

	PLX_DMA_DESC	*desc;

	pgs =(u_int)((len+SZ_PAGE-4) /SZ_PAGE);
	if ((pgs > phypages) || (ofs >= len)) {
		printf("setup_dma error\n"); return;
	}

	l2pci=(mode &0x01) ? 0x08 : 0;
	dmm=0;
	if (chn < 0) { chn=0; dmm=0x1000L; }	// dmd demand mode

	if (!(mode &0x02)) {
		dmm |=0x4000L;								// eot
		if (mode &0x04) dmm |=0x8000L;		// fst fast EOT
	}

	pofs=(u_int)(ofs >>12);
	fofs=(u_int)(ofs &0x0FFF);
	len -=ofs;
	fpg +=pofs;
	if (addr) addr +=ofs;

//printf("init dma %u with %u pages\n", chn, pgs-pofs);
	if ((pgs-pofs) == 1) {
		plx_regs->dma[chn].dmamode =0x0209C3L |dmm;	// IS|fst.eot.dmd|AC.BU|BT.RDY|32
		plx_regs->dma[chn].dmapadr =dmabuf_phy[fpg]+fofs; 
		plx_regs->dma[chn].dmaladr =addr;
		plx_regs->dma[chn].dmasize =len;
		plx_regs->dma[chn].dmadpr  =l2pci;
		return;
	}

// setup descriptor in scatter/gather mode SG

	plx_regs->dma[chn].dmamode =0x020BC3L |dmm;	// IS|fst.eot.dmd|AC.SG.BU|BT.RDY|32
	desc=(PLX_DMA_DESC*)dmadesc +dix;
	ix  =0;
	while (1) {
		if (!ix && ofs) {
			plen=SZ_PAGE-fofs;
			if (plen > len) plen=len;

			desc->dmapadr =dmabuf_phy[fpg]+fofs;
			desc->dmaladr =addr;
			desc->dmasize =plen;
		} else {
			plen=SZ_PAGE;
			if (plen > len) plen=len;

			desc->dmapadr =dmabuf_phy[fpg+ix];
			desc->dmaladr =addr;
			desc->dmasize =plen;
		}

		ix++;
		len -=plen;
		if (addr) addr +=plen;

		if (!len || (ix >= (pgs-pofs))) {
			if (!(mode &0x10)) { desc->dmadpr=l2pci |0x3; break; }

			// circular
			desc->dmadpr=(dmadesc_phy +dix*sizeof(PLX_DMA_DESC)) |l2pci|0x1;
			break;
		}

		desc->dmadpr=(dmadesc_phy +(dix+ix)*sizeof(PLX_DMA_DESC)) |l2pci|0x1;
		desc++;
	}
}
//
//------------------------------ select_sc ----------------------------------
//
//		select address space of system controller
//
static void select_sc(
	int	sc)	// system controller
					// -1: master
					// else LVD frontbus controller
{
	act_sc=sc;
	if (sc < 0) {
		sc_regs=&bus1100->l_coupler;
		inp_bc=&bus1100->in_card_bc;
		inp_regs=bus1100->in_card;
	} else {
		sc_regs=bus1100->coupler +sc;
		inp_bc=&bus1100->c_in_card_bc;
		inp_regs=bus1100->c_in_card;

		sc_bc->transp=sc;
	}

	clr_prot_error();
}
//
//------------------------------ inp_assign ---------------------------------
//
//		assign addresses to F1 input cards
//
static int inp_assign(	// -1: error
								// else nr of input cards available
	int	sc)	// system controller
					// -1: master
					// else LVD frontbus controller
{
	u_int			onl;
	u_int			addr;
	IN_CARD		*inp_unit;

	select_sc(sc);
	inp_bc->ctrl=MRESET;
	if ((onl=inp_bc->card_onl) != 0) {
		printf("any input online bit set: %04X\n", onl);
		return -1;
	}

	addr=0; onl=0;
	while (inp_bc->card_offl == 1) {
		inp_unit=(sc < 0) ? bus1100->in_card +addr
								: bus1100->c_in_card +addr;
		inp_bc->card_onl=addr;
		onl=inp_bc->card_onl;
		if (!(onl &(1 <<addr))) {
			printf("card not online\n");
			return -1;
		}

		gtmp=inp_unit->ident;
		if (protocol_error(-1, 0)) {
			printf("card %u not online, bus error\n", addr);
			return -1;
		}

		addr++;
	}

	if (inp_bc->card_offl) {
		printf("illegal offline value\n");
		return -1;
	}

	return addr;
}
//
//------------------------------ sc_assign ----------------------------------
//
//		assign addresses to front bus system controller
//
static int sc_assign(	// -1: error
								// else nr of front bus system controller
	void)
{
	u_int		addr;
	u_int		onl;

	clr_prot_error();
	addr=(u_int)cm_reg->version;
	if (protocol_error(-1, 0)) {
		printf("missing master system controller\n");
		return -1;
	}

	sc_bc->ctrl=MRESET;
	if ((onl=sc_bc->card_onl) != 0) {
		printf("any slave online bit set: %04X\n", onl);
		return -1;
	}

	if (!sc_bc->card_offl) return 0;

	addr=0; onl=0;
	while (sc_bc->card_offl == 1) {
		sc_bc->card_onl=addr;
		onl=sc_bc->card_onl;
		if (!(onl &(1 <<addr))) {
			printf("sc card addr %u not online\n", addr);
			return -1;
		}

		sc_regs=bus1100->coupler +addr;
		gtmp=sc_regs->ident;
		if (protocol_error(-1, 0)) {
			printf("sc card %u not online, bus error\n", addr);
			return -1;
		}

		addr++;
	}

	if (sc_bc->card_offl) {
		printf("wrong SC offline bit set: %04X\n", sc_bc->card_offl);
		return -1;
	}

	return addr;
}
//
//------------------------------ sc_wr_f1reg --------------------------------
//
static int sc_wr_f1reg(		// -1: error
	u_int		reg,
	u_int		value)
{
	u_int		i;

	sc_regs->f1_reg[reg]=value; i=0;
	while ((gtmp=sc_regs->sr) &WRBUSY) {
		if (protocol_error(-1, 0) || (++i > 5)) return -1;
	}

	return 0;
}
//
//------------------------------ inp_wr_f1reg --------------------------------
//
static int inp_wr_f1reg(		// -1: error
	u_int		inp,		// nr of F1 input unit
	u_int		reg,
	u_int		value)
{
	u_int		i;

	inp_regs[inp].f1_reg[reg]=value; i=0;
	while ((gtmp=inp_regs[inp].f1_state[0]) &WRBUSY) {
		if (protocol_error(-1, 0) || (++i > 5)) return -1;
	}

	return 0;
}
//
//------------------------------ inpbc_wr_f1reg -----------------------------
//
static int inpbc_wr_f1reg(		// -1: error
	u_int		reg,
	u_int		value)
{
	u_int		i;

	inp_bc->f1_reg[reg]=value; i=0;
	while ((gtmp=inp_regs[0].f1_state[0]) &WRBUSY) {	// same for all cards
		if (protocol_error(-1, 0) || (++i > 5)) return -1;
	}

	return 0;
}
//
//------------------------------ f1_start -----------------------------------
//
//		start all ring oscillator
//
static int f1_start(	// 0: ok
	void)
{
	u_short	tmp;
	u_int		edge;
	u_int		i;
	u_int		f1;
//
//--- first the system controller
//
	sc_regs->ctrl=PURES;
	tmp=sc_regs->sr;
	if (protocol_error(-1, 0)) return -1;

	if (!(tmp &NOINIT)) {
		printf("%d: missing NOINIT state\n", act_sc);
		return -1;
	}

	if (sc_wr_f1reg(0, 0)) return -1;
	if (sc_wr_f1reg(1, 0)) return -1;

	edge=0x4040;	// rising edge
	if (sc_wr_f1reg(2, edge)) return -1;
	if (sc_wr_f1reg(3, edge)) return -1;
	if (sc_wr_f1reg(4, edge)) return -1;
	if (sc_wr_f1reg(5, edge)) return -1;

	if (sc_wr_f1reg(10, 0x1800|(REFCLKDIV<<8)|config.hsdiv)) return -1;
	if (sc_wr_f1reg(15, R15_ROSTA|R15_COM|R15_8BIT)) return -1;
	if (sc_wr_f1reg(7, R7_BEINIT)) return -1;

	i=0;
	while (1) {
		tmp=sc_regs->sr;
		if (protocol_error(-1, 0)) return -1;

		if (!(tmp &(NOLOCK|NOINIT))) break;

		if (++i >= 10) {
			printf("%d: F1 state 0x%04x\n", act_sc, tmp);
			printf("may be a known hardware problem\n");
			return -1;
		}

		delay(100);
	}
//
//--- now all F1 input cards
//
	inp_bc->ctrl=PURES;
	inp_bc->f1_addr=F1_ALL;
	if (inpbc_wr_f1reg(0, 0)) return -1;
	if (inpbc_wr_f1reg(1, 0)) return -1;

	if (inpbc_wr_f1reg(10, 0x1800|(REFCLKDIV<<8)|config.hsdiv)) return -1;
	if (inpbc_wr_f1reg(15, R15_ROSTA|R15_COM|R15_8BIT)) return -1;
	if (inpbc_wr_f1reg(7, R7_BEINIT)) return -1;

	i=0;
	while (1) {
		tmp=inp_bc->f1_error;
		if (protocol_error(-1, 0)) return -1;

		if (!tmp) break;

		if (++i >= 10) {
printf("%d: f1_error %04X\n", act_sc, tmp);
			for (i=0; i < 16; i++) {
				if (!(tmp &(1<<i))) continue;

				for (f1=0; f1 < 8; f1++) {
					u_short	state=inp_regs[i].f1_state[f1];

					if (state) {
						printf("%d/%u/%u: F1 state 0x%04x\n", act_sc, i, f1, state);
						printf("may be a known hardware problem\n");
					}
				}
			}

			return -1;
		}

		delay(100);
	}

	return 0;
}
//
//------------------------------ f1_hit_slope -------------------------------
//
//		set slope (rising/falling edge) and LeTra mode
//
int f1_hit_slope(	// 0: ok
	int	sc)	// -2: all
					// -1: master
					// else LVD frontbus controller
{
	u_int	edge;

	if (sc <= -2) {
		for (sc=-1; sc < config.nr_fbsc; sc++)
			if (f1_hit_slope(sc)) return -1;

		return 0;
	}

	select_sc(sc);
	edge=0;
	if (config.f1_slope &1) edge |=0x4040;
	if (config.f1_slope &2) edge |=0x8080;

	inp_bc->f1_addr=F1_ALL;
	if (inpbc_wr_f1reg(1, (config.f1_slope &4) ? R1_LETRA : 0)) return -1;

	if (inpbc_wr_f1reg(2, edge)) return -1;
	if (inpbc_wr_f1reg(3, edge)) return -1;
	if (inpbc_wr_f1reg(4, edge)) return -1;
	if (inpbc_wr_f1reg(5, edge)) return -1;

	return 0;
}
//
//------------------------------ set_dac ------------------------------------
//
//		set slope (rising/falling edge) and LeTra mode
//
int set_dac(		// 0: ok
	int	sc,
	int	inp,		// F1 input unit, -1: all
	int	con,
	u_int	dac)
{

	if (sc <= -2) {
		for (sc=-1; sc < config.nr_fbsc; sc++)
			if (set_dac(sc, inp, con, dac)) return -1;

		return 0;
	}

	select_sc(sc);
	if (inp < 0) {
		for (inp=0; inp < config.sc_db[sc+1].nr_f1units; inp++)
			if (set_dac(sc, inp, con, dac)) return -1;

		return 0;
	}

	if (con < 0) {
		for (con=0; con < 4; con++)
			if (set_dac(sc, inp, con, dac)) return -1;

		return 0;
	}

	inp_regs[inp].f1_addr=2*con;
	if (inp_wr_f1reg(inp, 11, dac)) return -1;

	if (inp_wr_f1reg(inp, 1, (config.f1_slope &0x04) ? R1_LETRA|R1_DACSTRT : R1_DACSTRT)) return -1;

	return 0;
}
//
//------------------------------ test_pulse ---------------------------------
//
//		send test pulse to all
//
void test_pulse(
	void)
{
	int	sc;

	for (sc=-1; sc < config.nr_fbsc; sc++) {
		select_sc(sc);
		sc_regs->ctrl=TESTPULSE;
	}
}
//
//------------------------------ event_counter ------------------------------
//
//		send test pulse to all
//
u_long event_counter(
	int	sc)
{
	select_sc(sc);
	return sc_regs->event_nr;
}
//
//------------------------------ inp_overrun --------------------------------
//
// Pruefe alle Input Karten, ob Overrun Fehler vorliegt
//
int inp_overrun(void)	//   0: ok
								//   1: overflow Fehler erkannt
								//  -1: fataler Bus Error
{
	int		sc;
	u_int		inp;
	IN_CARD	*iregs;
	int		ret=0;
	u_int		f1,j;
	u_int		err;

	for (sc=-1; sc < config.nr_fbsc; sc++) {
		select_sc(sc);
		gtmp=sc_regs->sr;
		if (protocol_error(-1, 0)) return -1;
		if (gtmp &(OOFL|HOFL)) {
			printf(" sc %i: trigger inp: overflow %02X\n", sc, gtmp);
			sc_regs->sr=gtmp &(OOFL|HOFL);
			if (!ret) ret=1;
			gtmp=sc_regs->sr;
			if (protocol_error(-1, 0)) return -1;

			if (gtmp &(OOFL|HOFL)) {
				printf(" sc %i: continuous error\n", sc);
				ret=-1;
			}
		}

		for (inp=0; inp < config.sc_db[sc+1].nr_f1units; inp++) {
			iregs=inp_regs +inp;
			gtmp=iregs->sr;
			if (protocol_error(-1, 0)) {
				printf(" sc/inp %i/%u\n", sc, inp);
				return -1;
			}

			if (gtmp &IN_FIFO_ERR) {
				printf(" sc/inp %i/%u: fifo err\n", sc, inp);
				iregs->sr =IN_FIFO_ERR;
				if (!ret) ret=1;
			}

			err=0;
			for (f1=0; f1 < 8; f1++) {
				gtmp=iregs->f1_state[f1];
				if (protocol_error(-1, 0)) {
					printf(" sc/inp %i/%u: F1 %u\n", sc, inp, f1);
					return -1;
				}

				if (gtmp &F1_INPERR) {
					printf(" sc/inp %i/%u F1 %u: overflow %02X %02X\n",
							 sc, inp, f1, gtmp, (f1 &1) ? iregs->hitofl[f1>>1] >>8 : iregs->hitofl[f1>>1] &0xFF);
					j=0; gtmp &=F1_INPERR;
					while (1) {
						iregs->f1_state[f1] =gtmp;
						if (!(iregs->f1_state[f1] &gtmp)) break;

						if (++j >= 1000) {
							printf(" continuous error\n");
							ret=-1;
							if (gtmp &HOFL) {
								iregs->f1_addr=f1;
								iregs->f1_reg[7]=0;
								j=0;
								while ((gtmp=iregs->f1_state[0]) &WRBUSY) {
									if (protocol_error(-1, 0)) return -1;

									if (++j > 5) return -1;	// ca. 2
								}

								iregs->f1_reg[7]=R7_BEINIT;
								j=0;
								while ((gtmp=iregs->f1_state[0]) &WRBUSY) {
									if (protocol_error(-1, 0)) return -1;

									if (++j > 5) return -1;	// ca. 2
								}
							}
						}
					}

					if (protocol_error(-1, 0)) {
						printf(" sc/inp %i/%u F1 %u\n", sc, inp, f1);
						return -1;
					}

					if (!ret) ret=1;
				}
			}

			if (err) iregs->hitofl[0]=0;
		}
	}

	return ret;
}
//
//===========================================================================
//
//------------------------------ inp_mode -----------------------------------
//
int inp_mode(
	void)
{
	int			sc;
	long			tmp;
	u_int			scr;	// system controller control register
	u_int			icr;	// F1 input control regiser

	if (!(config.daq_mode &MCR_DAQ)) {
		printf("%04X: this mode ist not allowed here\n");
		return -1;
	}

	if (send_dmd_eot()) return -1;
//
//--->>> setup all <<<---
//
//
//--- DAQ reset, alle coupler cr auf 0
//
	cm_reg->cr &=(SCR_LED1|SCR_LED0);
	bus1100->l_coupler.cr=0;
	bus1100->l_coupler.sr=0xFFFF;

	sc_bc->cr=0;
	sc_bc->sr=0xFFFF;
//
//--- daten variable
//
	kbatt_msk=0x0F;
	rout_trg_time=0;	// nur im handshake mode
	robf=dmabuf;
	tlat=(long)(config.ef_trg_lat*1000.0/RESOLUTION +0.5);
	twin=(long)(config.ef_trg_win*1000.0/RESOLUTION +0.5);
//
//--- control register of F1 input card
//
	switch (config.daq_mode &MCR_DAQ) {
case 0:
		icr=ICR_RAW;
		break;

case MCR_DAQTRG:
		icr=(config.f1_slope &0x04) ? ICR_LETRA : 0;
		break;

case MCR_DAQRAW:
		icr=ICR_TLIM|ICR_RAW;
		break;

case MCR_DAQRAW|MCR_DAQTRG:
		icr=(config.daq_mode &EVF_EXT_RAW) ? ICR_EXT_RAW|ICR_RAW : ICR_TLIM|ICR_RAW;
		break;
	}
//
//--- control register of system controller
//
	scr=(config.daq_mode &MCR_DAQ);
	if (   (scr == (MCR_DAQRAW|MCR_DAQTRG))
		 && (config.daq_mode &EVF_ON_TP)) scr=MCR_DAQ_TEST|MCR_DAQRAW|MCR_DAQTRG;

	if (config.daq_mode &EVF_HANDSH) scr|=MCR_DAQ_HANDSH;
//
//--- calculate trigger window
//
	if ((config.daq_mode &MCR_DAQ) == MCR_DAQTRG) {
		tmp=tlat;
		if (tlat <= -0x4000) tlat=-0x3FFF;
		if (tlat >= 0x4000) tlat=0x3FFF;
		if (tlat != tmp)
			printf("latency time corrected: %6.1fns\n", (tlat*RESOLUTION)/1000.);

		tmp=twin;
		if (tlat-twin <= -0x4000) {
			twin=tlat+0x4000;
			printf("window time corrected:  %6.1fns\n", (twin*RESOLUTION)/1000.);
		}
	}
//
//--- setup all system controller and F1 input modules
//
	for (sc=-1; sc < config.nr_fbsc; sc++) {
		select_sc(sc);

		inp_bc->cr=icr;
		if ((config.daq_mode &MCR_DAQ) == MCR_DAQTRG) {
			inp_bc->trg_lat =(u_short)tlat;
			inp_bc->trg_win =(u_short)twin;
			inp_bc->f1_range=(u_short)f1_range;
		}

		sc_regs->ro_delay=config.rout_delay;
		sc_regs->event_nr=eventcount;
		sc_regs->sr=sc_regs->sr;
		sc_regs->cr=scr;
		sc_regs->ctrl=SYNCRES;
	}
//
//--- setup LWL interface of the master system controller
//
	dma_buffer(1);
	gigal_regs->d0_bc_adr =0;	// sonst PLX kein EOT
	gigal_regs->d0_bc_len =0;
	plx_regs->l2pdbell=0xFFFFFFFFL;
	tmp=(cm_reg->cr &(SCR_LED1|SCR_LED0))|SCR_EV_DBELL;
	if (config.nr_fbsc) tmp |=SCR_SCAN;

	cm_reg->cr=tmp;

	gigal_regs->cr=SET_CR_IM(SR_PROT_ERROR|SR_S_XOFF|SR_DMD_EOT|CR_READY);
	plx_regs->intcsr =PLX_ILOC_ENA|PLX_IDOOR_ENA;		// enable PCI Interrupt
	cm_reg->timer=0;
	return 0;
}
//
//------------------------------ read_event ---------------------------------
//
int read_event(
	void)
{
	u_long	tmp;
	u_long	dbell;
	COUPLER	*sregs;
	int		sc;
	u_int		lerr;
	u_int		j;

	if (protocol_error(-1, "RO")) return -1;
	gigal_regs->sr =gigal_regs->sr;
	pause=0;
	rout_len=0;
//---------------
// DMA read block
//---------------
	setup_dma(-1, 0, 0, phypages*(u_long)SZ_PAGE, 0, 0, 0x01);
	gigal_regs->t_hdr =HW_BE0011 |HW_L_DMD |HW_R_BUS |HW_EOT |HW_FIFO |HW_BT |THW_SO_AD;
	gigal_regs->t_da  =phypages*(u_long)SZ_PAGE;
	while (1) {
		if (!(++kbatt &kbatt_msk)) {
			if (kbhit()) return -1;
		}

		dbell=plx_regs->l2pdbell;
		if (dbell &DBELL_F1_ERR) {
			printf(" F1 error at =%02X(%08lX)\n", (u_int)(dbell >>24), dbell);
			plx_regs->l2pdbell=dbell &DBELL_F1_ERR;
			if (WaitKey(0)) return 20;
		}

//---------------
// Event mode
//---------------
//--------------
// wait doorbell (interrupt)
//--------------
		if (   (config.daq_mode &EVF_DBELLPF)
			 && (dbell &DBELL_EVENTPF)) {
			for (sc=0; !(dbell &(0x10000L<<sc)); sc++);

			plx_regs->l2pdbell=0x10000L<<sc;
		} else {
			if (dbell &DBELL_EVENT) {
				for (sc=0; !(dbell &(0x100L<<sc)); sc++);

				plx_regs->l2pdbell=0x100L<<sc;
			} else {
				pause++; continue;
			}
		}

		sc--;
		if (sc < 0) {
			sregs=&bus1100->l_coupler;
		} else {
			sregs=bus1100->coupler +sc;
		}

		gigal_regs->d0_bc=0;
		plx_regs->dma[0].dmadpr=dmadesc_phy |0x9;
		plx_regs->dmacsr[0] =0x3;						// start
if (protocol_error(-1, "DD.5")) return -1;
		gigal_regs->t_ad=(u_int)&sregs->ro_data;

//-------------------
// warte auf DMA ende
//-------------------
		j=0;
		while (1) {		// SR_PROT_ERROR|SR_S_XOFF|SR_DMD_EOT
			_delay_(100);
			if (plx_regs->intcsr &PLX_ILOC_ACT) break;

			if (++j >= 1000) {
				printf(" EOT(DMA0) time out CSR=%02X\n", plx_regs->dmacsr[0]);
				printf(" sr=%08lX, prot_err=%03X, bal=%u\n",
						 gigal_regs->sr, (u_int)gigal_regs->prot_err,
						 (u_int)gigal_regs->balance);
				return -1;
			}
		}

		tmp=gigal_regs->sr;
		gigal_regs->sr=tmp;
		if (tmp &SR_PROT_ERROR) {	// wenn keine Daten vorhanden
			if ((lerr=(u_int)gigal_regs->prot_err) != (0x200|PE_BERR)) {
				printf(" prot_err 2 %03X\n", lerr);
				return -1;
			}

			printf(" no data available\n");
			send_dmd_eot();
			return -1;
		}

		if (!(tmp &SR_DMD_EOT)) {
			protocol_error(-1, "2");
			printf(" len: %03X\n", (u_int)gigal_regs->d0_bc);
			return -1;
		}

if (protocol_error(-1, "DD.7")) return -1;
		j=0;
		while (1) {
			_delay_(100);
			if ((plx_regs->dmacsr[0] &0x10)) break;

			if (++j >= 1000) {
				printf(" DMA0 not terminated CSR=%02X\n", plx_regs->dmacsr[0]);
				protocol_error(-1, "1");
				printf(" len: %03X\n", (u_int)gigal_regs->d0_bc);
				return -1;
			}
		}

//----------------------
// Event Daten im Kasten
//----------------------
		rout_len =(u_int)gigal_regs->d0_bc;
		if (!rout_len) {
			printf(" no data available\n");
			return -1;
		}

		if (config.daq_mode &EVF_HANDSH) {
			rout_trg_time=sregs->trg_data;
			rout_ev_inf=sregs->ev_info;		// Freigabe
			if ((u_short)rout_ev_inf != rout_len) {
				printf(" HS0 illegal block length: %08lX/%04X\n",
						 rout_ev_inf, rout_len);
				return -1;
			}
		}

		return 0;
	}
}
//--- end ---------------------- read_event ---------------------------------
//
//===========================================================================
//
//--------------------------- application -----------------------------------
//
void application(void)
{
	int	sc;
	u_int	inp;
	u_int	con;

	sc_bc =&bus1100->coupler_bc;
	if ((config.nr_fbsc=sc_assign()) < 0) return;

printf("nr of fb sc: %d\n", config.nr_fbsc);
	for (sc=-1; sc < config.nr_fbsc; sc++) {
		if ((config.sc_db[sc+1].nr_f1units=inp_assign(sc)) < 0) return;
printf("nr of inp:   %d/%d\n", sc, config.sc_db[sc+1].nr_f1units);

		if (!config.sc_db[sc+1].nr_f1units) {
			printf("no F1 input units found\n");
			return;
		}
	}

	config.hsdiv=RES_120_3;
	printf("selected hsdiv =0x%02x => %6.2fps\n", config.hsdiv, RESOLUTION);
	for (sc=-1; sc < config.nr_fbsc; sc++) {
		printf("%d: start ring oscillator of all f1\n", sc);
		select_sc(sc);
		if (f1_start()) return;
	}

	config.f1_slope=1;
	printf("set hit slope of all f1\n");
	if (f1_hit_slope(-2)) return;

	for (sc=-1; sc < config.nr_fbsc; sc++) {
		printf("%d: set threshold\n", sc);
		for (inp=0; inp < config.sc_db[sc+1].nr_f1units; inp++)
			for (con=0; con < 4; con++)
				if (set_dac(sc, inp, con, config.sc_db[sc+1].inp_db[inp].inp_dac[con])) return;
	}

	data_acquisition();
	return;
}
