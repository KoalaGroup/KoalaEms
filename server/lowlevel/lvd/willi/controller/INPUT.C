// Borland C++ - (C) Copyright 1991, 1992 by Borland International

/*	input.c -- general functions  */

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
#include "daq.h"
#include "seq.h"

#ifndef SC_M
static LVD_BUS		(*lvd_units)[16];
#endif
SC_DB		*sc_conf=0;
#ifndef PBUS
static COUPLER		*sc_regs;	// selectierter System Controller (mit transparent)
static SYSCTRL_RG	*sy_regs=0;	// new system controller with GPX
#endif

F1TDC_BC	*inp_bc;
INP_DB	*inp_conf=0;
F1TDC_RG	*inp_unit;
u_char	inp_type[16];		// index is LVD address, see INP_DB.type
u_char	inp_offl[16];		// ix is unit nr, T: offline

static u_char		m_mode=0;

static u_char		std_setup;
static u_short		gtmp;			// bei einem Bit Test wird sonst Byte gelesen
static u_long		glp;			// global loop counter

static int	datafile=-1;
//
//--------------------------- close_datafile --------------------------------
//
static int close_datafile(void)
{
	if (datafile < 0) return 0;

	close(datafile); datafile=-1;
	return 0;
}
//
//--------------------------- open_datafile ---------------------------------
//
static int open_datafile(void)
{
	char	*str;

	str=Read_String(config.dlogname, sizeof(config.dlogname));
	if (errno) return CE_MENU;

	if ((datafile >= 0) && !strcmp(str, config.dlogname)) return 0;

	close_datafile();
	if (!*str) return 0;

	if ((datafile=_rtl_creat(str, 0)) == -1) {
		perror("error creating data file");
		printf("name =%s", str);
		return CE_PROMPT;
	}

	strcpy(config.dlogname, str);
	return 0;
}
//
//--------------------------- setup_dma -------------------------------------
//
void setup_dma(
	int		chn,	// -1: channel 0 demand mode
	u_int		dix,	// first descriptor index
	u_int		fpg,	// first page
	u_long	len,
	u_long	addr,
	u_long	ofs,	// offset
	u_int		mode)
{
	u_long	dmm;
	u_int		l2pci;
	u_int		pgs;
	u_int		ix;
	u_long	plen;
	u_int		fofs;
	u_int		pofs;

	PLX_DMA_DESC	*desc;

//printf("setup_dma(%i, %03X, %03X, %06lX, %02lX, %02lX, %02X)\n", chn, dix, fpg, len, addr, ofs, mode);
	pgs =(u_int)((len+SZ_PAGE-4) /SZ_PAGE);
	if ((pgs > dmapages) || (ofs >= len)) {
		printf("setup_dma error\n"); return;
	}

	l2pci=(mode &DMA_L2PCI) ? 0x08 : 0;
	dmm=0;
	if (chn < 0) { chn=0; dmm=0x1000L; }		// dmd demand mode

	if (!(mode &DMA_NOEOT)) {
		dmm |=0x4000L;									// eot
		if (mode &DMA_FASTEOT) dmm |=0x8000L;	// fst fast EOT
	}

	pofs=(u_int)(ofs >>12);
	fofs=(u_int)(ofs &0x0FFF);
	len -=ofs;
	fpg +=pofs;
	if (mode &DMA_ADDRINC) addr +=ofs;

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
		if (mode &DMA_ADDRINC) addr +=plen;

		if (!len || (ix >= (pgs-pofs))) {
			if (!(mode &DMA_CIRCULAR)) { desc->dmadpr=l2pci |0x3; break; }

			// circular
			desc->dmadpr=(dmadesc_phy[dix>>8] +(dix&0xFF)*sizeof(PLX_DMA_DESC)) |l2pci|0x1;
			break;
		}

		desc->dmadpr=(dmadesc_phy[dix>>8] +((dix&0xFF)+ix)*sizeof(PLX_DMA_DESC)) |l2pci|0x1;
		desc++;
	}
}
//
//------------------------------ inp_assign ---------------------------------
//
int inp_assign(	// 0:					Ueberpruefung ok
						// CEN_MENU:		Eingabeabbruch
						// CE_NO_INPUT:	Modul nicht vorhanden
						// CE_FATAL:		Plausibilitaetsfehler
	SC_DB		*sconf,
	u_char	mode)	// 0: neu zuordnen
						// 1: ueberpruefen
						// 2: Karte waehlen
{
	u_int		onl;
	u_int		un;
	INP_DB	*iconf;
	F1TDC_BC	*ibcregs=sconf->inp_bc;
	F1TDC_RG	*iregs;

printf("inp_assign(%08lX, %d)\n", sconf, mode);
	CLR_PROT_ERROR();

//--------------------------- select new card

	while (mode == 2) {
		if (!sconf->slv_cnt) {
			inp_conf=0;
			inp_unit=0;
			return CE_NO_INPUT;
		}

		if (std_setup || (sconf->slv_cnt == 1)) sconf->slv_sel=0;
		else {
			printf("select input unit %2u ", sconf->slv_sel);
			sconf->slv_sel=(u_int)Read_Deci(sconf->slv_sel, -1);
			if (errno) return CEN_MENU;

			if (sconf->slv_sel >= sconf->slv_cnt) {
				printf("max number is %u\n", sconf->slv_cnt-1);
				if (use_default) sconf->slv_sel=sconf->slv_cnt-1;
				continue;
			}
		}

		inp_conf=sconf->inp_db+sconf->slv_sel;
		inp_unit=inp_conf->inp_regs;
		gtmp=inp_unit->ident;
		if (!BUS_ERROR) return 0;

		printf("inp unit %u nicht vorhanden\n", inp_conf->slv_addr);
		inp_conf=0;
		return CE_NO_INPUT;
	}

//--------------------------- ueberpruefen

	if (mode == 1) {
		if (ibcregs->card_offl) return inp_assign(sconf, 0);

		onl=ibcregs->card_onl;
		if (!onl) {
			sconf->slv_cnt=0;
			inp_conf=0;
			inp_unit=0;
			return CE_NO_INPUT;
		}

		sconf->dac_iunit =-1;
		sconf->dac_iconf =0;
		sconf->dac_iregs =0;
		for (un=0, iconf=sconf->inp_db; un < sconf->slv_cnt; un++, iconf++) {
			iconf->unit=un;
			if (!(onl&(1<<iconf->slv_addr))) return inp_assign(sconf, 0);

			onl &=~(1<<iconf->slv_addr);
#ifdef SC_M
			iconf->inp_regs=bus1100->in_card +iconf->slv_addr;
#else
			iconf->inp_regs=&(*lvd_units)[iconf->slv_addr].in_card;
#endif
			iregs=iconf->inp_regs;
			gtmp=iregs->ident;
			if (BUS_ERROR) {
				printf("inp unit %u nicht vorhanden\n", iconf->slv_addr);
				use_default=0;
				return inp_assign(sconf, 0);
			}

			iconf->type=gtmp &0xF8;
			inp_type[iconf->slv_addr]=iconf->type;
			if ((sconf->dac_iunit < 0) && (iconf->type == F1TDC)) {
				sconf->dac_iunit =un;
				sconf->dac_iconf =iconf;
				sconf->dac_iregs =iconf->inp_regs;
			}
		}

		if (onl) {
			use_default=0;
			return inp_assign(sconf, 0);
		}

		if (sconf->slv_sel >= sconf->slv_cnt) sconf->slv_sel=0;

		inp_conf=sconf->inp_db+sconf->slv_sel;
		inp_unit=inp_conf->inp_regs;
		return 0;
	}
//
//--------------------------- neu zuordnen
//
	sconf->slv_cnt=0;
	inp_conf=0;
	inp_unit=0;
	ibcregs->ctrl=MRESET;
	if ((onl=ibcregs->card_onl) != 0) {
		printf("any input online bit set: %04X\n", onl);
		return CE_FATAL;
	}

	sconf->dac_iunit =-1;
	sconf->dac_iconf =0;
	sconf->dac_iregs =0;
	un=0;
	iconf=sconf->inp_db; onl=0;
	while (ibcregs->card_offl == 1) {
		iconf->unit=un;
		if (std_setup) iconf->slv_addr=un;
		else {
			printf("input unit %2u address: %2u ", un, iconf->slv_addr);
			iconf->slv_addr=(u_int)Read_Deci(iconf->slv_addr, -1);
			if (errno) return CEN_MENU;
		}

		if (onl &(1 <<iconf->slv_addr)) {
			printf("ist schon vorhanden\n");
			use_default=0;
			continue;
		}

#ifdef SC_M
		iconf->inp_regs=bus1100->in_card +iconf->slv_addr;
#else
		iconf->inp_regs=&(*lvd_units)[iconf->slv_addr].in_card;
#endif
		ibcregs->card_onl=iconf->slv_addr;
		printf("online Input Cards %04X\n", onl=ibcregs->card_onl);
		if (!(onl &(1 <<iconf->slv_addr))) {
			printf("card not online\n");
			return CE_FATAL;
		}

		iregs=iconf->inp_regs;
		gtmp=iregs->ident;
		if (BUS_ERROR) {
			printf("inp unit %u nicht vorhanden\n", iconf->slv_addr);
			use_default=0;
			return inp_assign(sconf, 0);
		}

		iconf->type=gtmp &0xF8;
		inp_type[iconf->slv_addr]=iconf->type;
		if ((sconf->dac_iunit < 0) && (iconf->type == F1TDC)) {
			sconf->dac_iunit =un;
			sconf->dac_iconf =iconf;
			sconf->dac_iregs =iconf->inp_regs;
		}

		un++; iconf++;
	}

	if (!un) return CE_NO_INPUT;

	sconf->slv_cnt=un;
	return inp_assign(sconf, 2);
}
//
#ifdef PBUS		// if_0
int sc_assign(	// 0:				alle zugewiesen, auch Input Units
					// CEN_MENU:	Eingabeabbruch, keine Aenderung
					// CE_NO_SC:	kein System Controller vorhanden
					// CE_NO_INPUT	inp_assign(), keine Input Unit vorhanden
	u_char	mode)	// 0: neu zuordnen
						// 1: ueberpruefen
						// 2: Karte waehlen
{
printf("sc_assign(%d)\n", mode);
	CLR_PROT_ERROR();
	if (mode) {
		sc_conf=&config.sc_db;
		sc_conf->sc_unit=0;
		sc_conf->inp_bc=&(*lvd_units)[0].in_card_bc;
		inp_bc=sc_conf->inp_bc;
	}

	return 0;
}
#else				// if_0
//------------------------------ sc_assign ----------------------------------
//
int sc_assign(	// 0:				alle zugewiesen, auch Input Units
					// CEN_MENU:	Eingabeabbruch, keine Aenderung
					// CE_NO_SC:	kein System Controller vorhanden
					// CE_NO_INPUT	inp_assign(), keine Input Unit vorhanden
	u_char	mode)	// 0: neu zuordnen
						// 1: ueberpruefen
						// 2: Karte waehlen
{

printf("sc_assign(%d)\n", mode);
	CLR_PROT_ERROR();
	gtmp=(u_short)cm_reg->version;
	if (protocol_error(-1, 0)) {
		sc_conf=0;
		return CE_NO_SC;
	}

//--------------------------- check availibitity

	if (mode == 1) {
		config.sc_db.sc_unit=&bus1100->l_coupler;
		config.sc_db.inp_bc=&bus1100->in_card_bc;
		sc_conf=&config.sc_db;
		sc_regs=sc_conf->sc_unit;
		inp_bc=sc_conf->inp_bc;
		gtmp=sc_regs->ident;
		if (BUS_ERROR) {
			sc_conf=0;
			return CE_NO_SC;
		}

		if ((gtmp &0x00F0) == 0x00) {		// new system controller
			sy_regs=(SYSCTRL_RG*)sc_regs;
		} else
			sy_regs=0;

		return inp_assign(&config.sc_db, 1);
	}

	return 0;
}
//
#endif	// if_0
//--------------------------- list_units ------------------------------------
//
static int list_units(void)
{
	SC_DB		*sconf=&config.sc_db;
	u_int		inp;
	INP_DB	*iconf;
	F1TDC_RG	*iregs;
	u_int		id;

	printf("unit fw hw serial\n");
#ifdef SC_M
	id=sc_regs->ident;
	printf("  -1 %02X %02X %6u System Controller", id>>8, id&0xFF, sc_regs->serial);
	if TST_BUS_ERROR {
		printf(" serial number not available");
		CLR_BUS_ERROR;
	}
	Writeln();
#endif
		for (inp=0, iconf=sconf->inp_db; inp < sconf->slv_cnt; inp++, iconf++) {
			iregs=iconf->inp_regs;
			id=iregs->ident;
			printf("  %2u %02X %02X %6u ", inp, id>>8, id&0xFF, iregs->serial);
			switch (id &0xF8) {
		case F1TDC:		printf("TDC F1     "); break;
		case VERTEX:	printf("VERTEX     "); break;
		case SYMASTER:	printf("SYNC Master"); break;
		case SYSLAVE:	printf("SYNC Slave "); break;
		case FQDC:		printf("QDC Fast   "); break;
		case FQDCTST:	printf("QDC Fast Test"); break;
		case GPXTDC:	printf("TDC GPX    "); break;
		case GPXTDCTST:printf("TDC GPX Test"); break;
		case SQDC:		printf("QDC Slow   "); break;
		case SQDCTST:	printf("QDC Slow Test"); break;
		case JTEST:		printf("JTAG Test  "); break;
			}

			if TST_BUS_ERROR {
				printf(" serial number not available");
				CLR_BUS_ERROR;
			}
			Writeln();
		}

	Writeln();
	return CE_PROMPT;
}
//
//------------------------------ inp_overrun --------------------------------
//
// Pruefe alle Input Karten, ob Overrun Fehler vorliegt
//
static int inp_overrun(void)	//          0: ok
										//         -1: overflow Fehler erkannt
										//  CE_PROMPT: fataler Bus Error
{
	u_int		inp;
	u_int		f1;
	SC_DB		*sconf=&config.sc_db;
	INP_DB	*iconf;
	F1TDC_RG	*iregs;
	int		ret=0;
	u_int		j;
	u_int		err;

	if (!pciconf.errlog) return 0;

#ifndef PBUS
	gtmp=sconf->sc_unit->sr;
	if (BUS_ERROR) return 50;
	if (gtmp &(OOFL|HOFL)) {
		printf("trigger inp: overflow %02X\n", gtmp);
		sconf->sc_unit->sr=gtmp &(OOFL|HOFL);
		if (!ret) ret=-1;
		gtmp=sconf->sc_unit->sr;
		if (BUS_ERROR) return 51;

		if (gtmp &(OOFL|HOFL)) {
			printf("continuous error\n");
			ret=CE_PROMPT;
		}
	}


#endif
	for (inp=0, iconf=sconf->inp_db; inp < sconf->slv_cnt; inp++, iconf++) {
//printf("inp: %d\n", inp);
		if (iconf->type != F1TDC) continue;

		iregs=iconf->inp_regs;
		gtmp=iregs->sr;
		if (BUS_ERROR) {
			printf("inp unit %u\n", inp);
			return 52;
		}

		if (gtmp &IN_FIFO_ERR) {
			printf("inp %u: fifo err\n", inp);
			iregs->sr =IN_FIFO_ERR;
			if (!ret) ret=-1;
		}

		err=0;
		for (f1=0; f1 < 8; f1++) {
			gtmp=iregs->f1_state[f1];
			if (BUS_ERROR) {
				printf("inp/F1 %u/%u\n", inp, f1);
				return 53;
			}

			if (gtmp &F1_INPERR) {
				printf("inp/F1 %u/%u: overflow %02X %02X\n",
						 inp, f1, gtmp, (f1 &1) ? iregs->hitofl[f1>>1] >>8
															  : iregs->hitofl[f1>>1] &0xFF);
				iregs->f1_state[f1] =gtmp;
				if (gtmp &PERM_HOFL) {
					printf("permanent Hit Overflow, F1 reinit\n");
if (WaitKey(0)) return 60;
					iregs->f1_addr=f1;
					iregs->f1_reg[7]=0;
					j=0;
					while ((gtmp=iregs->f1_state[0]) &WRBUSY) {
						if (BUS_ERROR) return 54;

						if (++j > 5) return 55;	// ca. 2
					}

					iregs->f1_reg[7]=config.inp_f1_reg[7];
					j=0;
					while ((gtmp=iregs->f1_state[0]) &WRBUSY) {
						if (BUS_ERROR) return 56;

						if (++j > 5) return 57;	// ca. 2
					}

					iregs->f1_state[f1] =gtmp;
				}

				if (BUS_ERROR) {
					printf("inp %u F1 %u\n", inp, f1);
					return 58;
				}

				if (!ret) ret=-1;
				err=1;
			}
		}

		if (err) iregs->hitofl[0]=0;
	}

	return ret;
}
//
//------------------------------ f1_regdiag ---------------------------------
//
int f1_regdiag(
	u_int	*regs)
{

	char		key;
	u_int		ux;

printf("Header[8]Trailer[8] enable                               %04X..0\n", regs[0]);
printf("hires|hitm|letra|sq[2]fake|ovlap|ibs[2]obs|inh|slw[4]dac %04X..1\n", regs[1]);
printf("tra2|lead2|adj2[6]tra1|lead1|adj1[6]                     %04X..2\n", regs[2]);
printf("tra4|lead4|adj4[6]tra3|lead3|adj3[6] ................... %04X..3\n", regs[3]);
printf("tra6|lead6|adj6[6]tra5|lead5|adj5[6]                     %04X..4\n", regs[4]);
printf("tra8|lead8|adj8[6]tra7|lead7|adj7[6] ................... %04X..5\n", regs[5]);
printf("busclkdel[4]adjustB[6]adjustA[6]                         %04X..6\n", regs[6]);
printf("beinit|refstart[9]hit_time[6] .......................... %04X..7\n", regs[7]);
printf("triggerWindow[16]                                        %04X..8\n", regs[8]);
printf("triggerLatency[16]                                       %04X..9\n", regs[9]);
printf("diag1|testpll|track|negPLL|ResAdj|ClkDiv[3]SpeedDiv[8]   %04X..A\n", regs[10]);
printf("dac2[8]dac1[8]                                           %04X..B\n", regs[11]);
printf("dac4[8]dac3[8] ......................................... %04X..C\n", regs[12]);
printf("dac6[8]dac5[8]                                           %04X..D\n", regs[13]);
printf("dac8[8]dac7[8]                                           %04X..E\n", regs[14]);
printf("diag2|Htst|StaTst|[5]diasel[4]rosta|syncstrt|common|bus8 %04X..F\n", regs[15]);
printf("                                                               %c ", config.i0_menu);
	if (use_default) key=config.i0_menu;
	else
		do {
			key=toupper(getch());
			if ((key == CTL_C) || (key == ESC)) { Writeln(); return -1; }

			use_default=(key == TAB);
			if (use_default || (key == CR)) key=config.i0_menu;

		} while ((key < '0') || (key > 'F'));

	printf("%c\n", key);
	ux=key-'0';
	if (ux > 9) ux=ux-7;

	if (ux >= 16) return -1;

	config.i0_menu=key;
	printf("Value %04X ", regs[ux]);
	regs[ux]=(u_int)Read_Hexa(regs[ux], 0xFFFF);
	if (errno) return -1;

	return ux;
}
//
#ifdef SC_M		// if_2
//===========================================================================
//										Master System Controller                      =
//===========================================================================
//
//--------------------------- system_control_master -------------------------
//
// show/modify CMASTER Operation Registers
//
const char *cmsr_txt[] ={
	"?", "?", "?", "?", "Ev", "F1e", "?", "?",
	"?", "?", "?", "?", "?",  "?",   "?", "?"
};

const char *cmcr_txt[] ={
	"Dd", "sca", "sngE", "Led0", "Led1", "?", "?", "?",
	"?",  "?",   "?",    "?",    "?",    "?", "?", "?"
};
//
//--------------------------- jt_scm_getdata --------------------------------
//
static u_long jt_scm_getdata(void)
{
	return cm_reg->jtag_data;
}
//
//--------------------------- jt_scm_getcsr ---------------------------------
//
static u_int jt_scm_getcsr(void)
{
	return (u_int)cm_reg->jtag_csr;
}
//
//--------------------------- jt_scm_putcsr ---------------------------------
//
static void jt_scm_putcsr(u_int val)
{
	cm_reg->jtag_csr=val;
}
//
static int system_control_master(void)
{
	u_int		ux;
	char		hgh;
	char		key;
//	u_long	srg;
	u_long	tmp;
	char		hd;

	hd=1;
	while (1) {
//		CLR_PROT_ERROR();
		BUS_ERROR;
		errno=0;
		if (hd) {
			Writeln();
printf("Version                        %08lX\n", cm_reg->version);
		if (protocol_error(-1, 0)) return CEN_PROMPT;
printf("Status Register                %08lX     ", tmp=cm_reg->sr);
			hgh=0;
			if (tmp &DBELL_F1_ERR) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("F1err");
			}
			if (tmp &DBELL_EVENTPF) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("pfull");
			}
			if (tmp &DBELL_EVENT) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("sevt");
			}
			if (tmp &1) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("ddblk");
			}
			lowvideo(); cprintf("\r\n");
printf("Control Register               %08lX ..1 ", tmp=cm_reg->cr);
			ux=16; hgh=0;
			while (ux--)
				if (tmp &((u_long)0x1 <<ux)) {
					if (hgh) highvideo(); else lowvideo();
					hgh=!hgh; cprintf("%s", cmcr_txt[ux]);
				}
			lowvideo(); cprintf("\r\n");

printf("10 kHz timer                   %08lX\n", cm_reg->timer);
printf("direct data count              %08lX\n", cm_reg->dd_counter);
printf("direct data block size         %08lX ..2\n", cm_reg->dd_blocksz);
printf("JTAG control, Master reset, Clear       ..J/M/C\n");
			printf("                                          %c ",
					 pciconf.op2_menu);
			key=toupper(getch());
			if ((key == CTL_C) || (key == ESC)) return CE_MENU;

		} else {
			hd=1; printf("select> ");
			key=toupper(getch());
			if ((key == CTL_C) || (key == ESC) || (key == CR)) continue;

			if (key == ' ') key=pciconf.op2_menu;
		}

		use_default=(key == TAB);
		if (use_default || (key == CR)) key=pciconf.op2_menu;
		if (key > ' ') printf("%c\n", key);

		if (key == 'J') {
			JT_CONF	*jtc;

			pciconf.op2_menu=key;
			ux=config.sc_db.sc_unit->ident;
			switch (ux &0xF) {
		case 0:  jtc=&config.jtag_scm0; break;
		default: jtc=&config.jtag_scm0; break;
		case 1:  jtc=&config.jtag_scm1; break;
			}

			JTAG_menu(jt_scm_putcsr, jt_scm_getcsr, jt_scm_getdata, jtc, 0);
			continue;
		}

		if (key == 'M') {
			gigal_regs->cr =CR_REM_RESET;
			continue;
		}

		if (key == 'C') {
			cm_reg->timer=0;
			cm_reg->dd_counter=0;
			continue;
		}

		ux=key-'0';
		if (ux > 9) ux=ux-7;

		if (ux  > 2) continue;

		pciconf.op2_menu=key;
		printf("Value $%08lX ", config.ao_regs[ux+10]);
		config.ao_regs[ux+10]=Read_Hexa(config.ao_regs[ux+10], -1);
		if (errno) continue;

		switch (ux) {

case 1:
			cm_reg->cr=config.ao_regs[ux+10]; break;

case 2:
			cm_reg->dd_blocksz=config.ao_regs[ux+10]; break;

		}
	}
}
//
#endif	// if_2

#ifndef PBUS	// if_4
//===========================================================================
//										General System Controller                     =
//===========================================================================
//
//------------------------------ sc_f1reg -----------------------------------
//
static int sc_f1reg(		// 0: no error
	u_int	reg,
	u_int	value)
{
	int	i;

	sc_regs->f1_reg[reg]=value;
	config.sc_f1_reg[reg]=0xFFFF;
	i=0;
	while ((gtmp=sc_regs->sr) &WRBUSY) {
		if (++i > 5) return CE_FATAL;	// ca. 2
	}

	config.sc_f1_reg[reg]=value;
	return 0;
}
//
//------------------------------ sc_resolution ------------------------------
//
#define DV_INC		0x8
#define DV_LOOP	50

static int sc_resolution(void)	// 0: ok
{
	int	ret;
	u_int	ux;
	u_int	hsdiv_min,hsdiv_max;

	ux=0;
	while (!((gtmp=sc_regs->sr) &NOINIT)) {
		if ((ret=sc_f1reg(7, 0)) != 0) return ret;

		delay(10);
		if (++ux <= 3) continue;

		printf("be_init should be zero\n");
		return CE_PROMPT;
	}

	if ((ret=sc_f1reg(15, R15_ROSTA)) != 0) return ret;

	if ((ret=sc_f1reg(7, R7_BEINIT)) != 0) return ret;

	if ((gtmp=sc_regs->sr) &NOINIT) {
		printf("no be_init\n"); return CE_PROMPT;
	}

	hsdiv_max=0xF0;
	while (1) {
		sc_f1reg(15, 0);
		sc_f1reg(7, 0);
		ux=0;
		while (1) {
			delay(1);
			gtmp=sc_regs->sr; ux++;
			if (ux < 3) continue;

			if ((gtmp &(NOLOCK|NOINIT)) == (NOLOCK|NOINIT)) break;

			return CE_FATAL;
		}

		sc_f1reg(10, 0x1800|(REFCLKDIV<<8)|hsdiv_max);
		sc_f1reg(15, R15_ROSTA);
		if ((ret=sc_f1reg(7, R7_BEINIT)) != 0) return ret;

		ux=0;
		do {
			delay(1); gtmp=sc_regs->sr;
		} while ((gtmp &(NOLOCK|NOINIT)) && (++ux < DV_LOOP));

		if (!(gtmp &NOLOCK)) break;

		hsdiv_max -=DV_INC;
		if (hsdiv_max <= 0x60) return CE_F1_NOLOCK;
	}

	hsdiv_min=hsdiv_max;
	do {
		hsdiv_min -=DV_INC;
		if ((ret=sc_f1reg(10, 0x1800|(REFCLKDIV<<8)|hsdiv_min)) != 0) return ret;

		ux=0;
		do {
			delay(1); gtmp=sc_regs->sr;
		} while ((gtmp &NOLOCK) && (++ux < DV_LOOP));	// ca. 10..16

	} while (!(gtmp &NOLOCK) && hsdiv_min);

	hsdiv_min +=DV_INC;
	printf("hsdiv min =0x%02X = %5.1fps\n", hsdiv_min, HSDIV/hsdiv_min);
	printf("hsdiv max =0x%02X = %5.1fps => recommend = %5.1f\n",
			 hsdiv_max, HSDIV/hsdiv_max, HSDIV/(hsdiv_max-8));

	return sc_f1reg(10, 0x1800|(REFCLKDIV<<8)|(hsdiv_max-8));
}
//
//===========================================================================
//										System Controller
//===========================================================================
//
//------------------------------ rd_eventfifo -------------------------------
//
static int rd_eventfifo(u_char all)
{
	u_long	sz=dmapages*(u_long)SZ_PAGE;
	u_long	lx;
	u_int		i,l;
	char		hd=-1;
	u_int		sl=0;		// segment length
	u_int		c=0;
	u_int		ln=0;
	u_long	lp=0xFFFFFFFFl;
	u_int		max=0;
	u_int		min=0xFFFF;

static u_long	dblen=4;

	if (!((gtmp=sc_regs->sr) &SC_DATA_AV)) {
		printf("Event FIFO empty\n");
		return CE_PROMPT;
	}

	if (send_dmd_eot()) return CEN_PROMPT;

	if (!all) {
		hd=-2; sl=0xFFFF;
		printf("DMA buffer length (max %05lX) %05lX ", sz, dblen);
		dblen=Read_Hexa(dblen, -1) &~0x3;

		if (errno) return CE_MENU;

		if (dblen < sz) sz=dblen;
		else dblen=sz;

		if (!sz) { hd=1; sz=12; lp=0; }
	}

	gigal_regs->d0_bc_adr =0;	// sonst PLX kein EOT
	gigal_regs->d0_bc_len =0;
	setup_dma(-1, 0, 0, dmapages*(u_long)SZ_PAGE, 0, 0, DMA_L2PCI);
	gigal_regs->t_hdr =HW_BE0011 |HW_L_DMD |HW_R_BUS |HW_EOT |HW_FIFO |HW_BT |THW_SO_DA;
	gigal_regs->t_ad=OFFSET(BUS1100, l_coupler.ro_data);
	while (1) {
		gtmp=sc_regs->sr;
		if (protocol_error(lp, "sr")) return CE_PROMPT;

		if (!(gtmp &SC_DATA_AV)) {
			printf("\r%10lu\n", lp);
			printf("Event FIFO empty\n");
			return CE_PROMPT;
		}

		if (hd == 1) sz=12;
		gigal_regs->sr     =gigal_regs->sr;
		gigal_regs->d0_bc  =0;
		plx_regs->dma[0].dmadpr=dmadesc_phy[0] |0x9;
		plx_regs->dmacsr[0]=0x3;						// start
		gigal_regs->t_da   =sz;
		i=0;
		while (1) {
			_delay_(1000);
			if ((plx_regs->dmacsr[0] &0x10)) break;

			if (gigal_regs->sr &SR_PROT_ERROR) {
				protocol_error(-1, "1");
				printf("len: %03X\n", (u_int)gigal_regs->d0_bc);
				return CE_PROMPT;
			}

			if (++i >= 1000) {
				printf("\r%10lu\n", lp);
				printf("DMA0 not terminated CSR=%02X\n", plx_regs->dmacsr[0]);
				protocol_error(-1, "1");
				printf("len: %03X\n", (u_int)gigal_regs->d0_bc);
				return CE_PROMPT;
			}
		}

		if (protocol_error(lp, "ro")) return CE_PROMPT;

		lx =gigal_regs->d0_bc;
		if (lx > sz) {
			printf("extra words received is/max %04lX/%04lX\n", lx, sz);
			if (WaitKey(0)) return CE_PROMPT;
		}

		if ((hd != -1) && (lx != sz)) {
			printf("\r%10lu\n", lp);
			printf("block fragment(%u) ex/is %05lX/%05lX\n", hd, sz, gigal_regs->d0_bc);
			return CE_PROMPT;
		}

		if ((hd >= 0) && !(++lp &0xFE) && hd) {
			printf("\r%10lu %04X %04X", lp, min, max);
			if (kbhit()) {
				Writeln();
				return CE_PROMPT;
			}
		}

		if (hd == 0) {
			u_int		inp;
			u_int		f1;
			INP_DB	*iconf;
			F1TDC_RG	*iregs;

			hd=1;
			if (lp &0xE) continue;

			for (inp=0, iconf=sc_conf->inp_db; inp < sc_conf->slv_cnt; inp++, iconf++) {
				if (iconf->type != F1TDC) continue;

				iregs=iconf->inp_regs;
				gtmp=iregs->sr;
				if (BUS_ERROR) {
					printf("inp %u\n", inp);
					return 52;
				}

				if (gtmp &IN_FIFO_ERR) {
					iregs->sr =IN_FIFO_ERR;
				}

				for (f1=0; f1 < 8; f1++) {
					gtmp=iregs->f1_state[f1];
					if (BUS_ERROR) {
						printf("inp/F1 %u/%u\n", inp, f1);
						return 53;
					}

					if (gtmp &F1_INPERR) {
						iregs->f1_state[f1] =gtmp;
						if (gtmp &PERM_HOFL) {
							printf("permanent Hit Overflow, F1 reinit\n");
							return 60;
						}

						if (BUS_ERROR) {
							printf("inp %u F1 %u\n", inp, f1);
							return 58;
						}
					}
				}
			}

			continue;
		}

      if (hd == 1) {
			if (   ((u_int)(dmabuf[0][0] >>16) != 0x8000)
				 || ((u_int)dmabuf[0][0] <12)) {
				printf("\r%10lu\n", lp);
				printf("header error\n");
				return CE_PROMPT;
			}

			sz=(u_int)dmabuf[0][0];
			if (max < (u_int)sz) max =(u_int)sz;
			if (min > (u_int)sz) min =(u_int)sz;

			if ((sz -=12) != 0) hd=0;
			continue;
		}

		printf("readout length =0x%05lX\n", lx);
		if (lx > sz) {
			printf("extra words received\n");
			if (WaitKey(0)) return CE_PROMPT;
		}

		l=(u_int)(lx >>2); c=0;
		for (i=0; i < l; i++) {
			if (!sl && ((u_int)(dmabuf[0][i] >>16) == 0x8000)) {
				if (c) { ln++; Writeln(); c=0; }
				if (sl && (sl != 0xFFFF)) { printf("format error 0\n"); use_default=0; }
				sl=(u_int)dmabuf[0][i] >>2;
			}

			if (sl == 0) {
				if (c) { ln++; Writeln(); c=0; }
				printf("format error 1\n"); sl=0xFFFF; use_default=0; hd=-2;
			}

			if (sl != 0xFFFF) sl--;
			printf(" %08lX", dmabuf[0][i]);
			if (++c >= 8) {
				c=0; Writeln();
				if (++ln >= 40) {
					ln=0;
					if (WaitKey(0)) return CE_PROMPT;
				}
			}
		}

		if (c) { Writeln(); c=0; }

		if (sl && (hd == -1)) {
			hd=-2; use_default=0;
			sz=(u_long)sl<<2;
			continue;
		}

		if (sl && (sl != 0xFFFF)) printf("format error 2\n");
		return CE_PROMPT;
	}
}
//
//------------------------------ sc_f1_regs ---------------------------------
//
static int sc_f1_regs(void)
{
	int	i;

	if (!sc_conf) return CE_PARAM_ERR;

	if ((i=f1_regdiag(config.sc_f1_reg)) < 0) return CEN_MENU;

	sc_regs->f1_reg[i]=config.sc_f1_reg[i];

	return CE_MENU;
}
//
//--------------------------- system_control_F1 -----------------------------
//
const char *sccr_txt[] ={
	"Trig", "Raw", "TPu", "HSh", "nF1St", "Tst", "?", "?",
	"?",    "?",   "?",   "?",   "?",     "?",   "?", "?"
};
//
const char *scsr_txt[] ={
	"Bsy",  "NoIn", "NoLo", "Tofl", "Hofl", "Oofl", "F1er", "Tmis",
	"Dval", "Treq", "?",    "?",    "?",    "?",    "Eofl", "Serr"
};
//
int system_control_F1(void)
{
	int		ret;
	u_int		i;
	u_int		mx;
	char		hgh;
	char		rkey,key;
	char		hd;
	u_long	tmp;
	u_long	dbell;

	hd=1; ret=0;
	ret=sc_assign(1);
	while (1) {
		while (kbhit()) getch();
		Writeln();
		if ((ret < CEN_MENU) || (ret > CE_MENU)) {
			if (ret > CE_PROMPT) display_errcode(ret);

			hd=0;
		}

		if (!sc_conf) hd=1;

		errno=0; ret=0;
		if (hd) {
printf("CM Status Register             %08lX      ", tmp=cm_reg->sr);
			hgh=0;
			if (tmp &DBELL_F1_ERR) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("F1err");
			}
			if (tmp &DBELL_EVENTPF) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("pfull");
			}
			if (tmp &DBELL_EVENT) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("sevt");
			}
			if (tmp &1) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("ddblk");
			}
			lowvideo(); cprintf("\r\n");
printf("CM Control Register            %08lX ..1  ", tmp=cm_reg->cr);
			i=16; hgh=0;
			while (i--)
				if (tmp &((u_long)0x1 <<i)) {
					if (hgh) highvideo(); else lowvideo();
					hgh=!hgh; cprintf("%s", cmcr_txt[i]);
				}
			lowvideo(); cprintf("\r\n");

printf("CM direct data count           %08lX\n", cm_reg->dd_counter);
printf("CM direct data block size      %08lX\n", cm_reg->dd_blocksz);
printf("doorbell                       %08lX      ", dbell=plx_regs->l2pdbell);
			hgh=0;
			if (dbell &DBELL_F1_ERR) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("F1err");
			}
			if (dbell &DBELL_EVENTPF) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("pfull");
			}
			if (dbell &DBELL_EVENT) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("sevt");
			}
			if (dbell &1) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("ddblk");
			}
			lowvideo(); cprintf("\r\n");

printf("1100 Reset, CM Reset, assign front Bus  ..M/R/B\n");

			if (sc_conf) {
printf("online input cards                 %04X\n", inp_bc->card_onl);
printf("ident/serial                   %04X/%3u\n", sc_regs->ident, sc_regs->serial);
printf("Control Register                   %04X ..3  ", mx=sc_regs->cr);
			i=16; hgh=0;
			while (i--)
				if (mx &(0x1 <<i)) {
					if (hgh) highvideo(); else lowvideo();
					hgh=!hgh; cprintf("%s", sccr_txt[i]);
				}

			lowvideo(); cprintf("\r\n");

printf("Status Register (sel clr)          %04X ..4  ", mx=sc_regs->sr);
			i=16; hgh=0;
			while (i--)
				if (mx &(0x1 <<i)) {
					if (hgh) highvideo(); else lowvideo();
					hgh=!hgh; cprintf("%s", scsr_txt[i]);
				}

			lowvideo(); cprintf("\r\n");

printf("event number                   %08lX ..5\n", sc_regs->event_nr);
printf("readout delay                      %04X ..6  %3.0fns\n", sc_regs->ro_delay, 25.*sc_regs->ro_delay);
printf("trigger data (last)            %08lX\n", sc_regs->trg_data);
printf("event info (last)              %08lX ..i\n", rout_ev_inf);
printf("Tst(8), F1syn(4), F1rst(2), Mrst(1)  %02X ..7\n", sc_conf->cmst_data[7]);
printf("Event FIFO, R-Adjust, F1 Regs, Test, Dac..e/E/A/F/T/D\n");
printf("JTAG, Clear Status, Setup, SC Reset     ..J/C/S/R\n");
			}

			if (BUS_ERROR) CLR_PROT_ERROR();

			printf("                                          %c ",
					 config.c0_menu);
			rkey=getch();
			if ((rkey == CTL_C) || (rkey == ESC)) return CE_MENU;

		} else {
			hd=1; printf("select> ");
			rkey=getch();
			if (rkey == CTL_C) return CE_MENU;

			if ((rkey == ESC) || (rkey == CR)) continue;

			if (rkey == ' ') rkey=config.c0_menu;
		}

		use_default=(rkey == TAB);
		if (use_default || (rkey == CR)) rkey=config.c0_menu;

		if (rkey > ' ') printf("%c\n", rkey);

		key=toupper(rkey);
		if (key == 'M') {
			gigal_regs->cr =CR_RESET;
			continue;
		}

		if (key == 'R') {
			gigal_regs->cr =CR_REM_RESET;
			continue;
		}

		if (!sc_conf && ((key < '0') || (key > '1'))) continue;
//
//		-----------------------------------------------------------------------
//
		if (key == 'C') {
			sc_regs->sr =sc_regs->sr;
			plx_regs->l2pdbell=dbell;
			cm_reg->dd_counter=0;
			sc_regs->event_nr=0;
			continue;
		}

		if (key == 'A') {
			ret=sc_resolution();
			continue;
		}

		if (key == 'E') {
			ret=rd_eventfifo(rkey == 'e');
//			config.c0_menu=rkey;
			continue;
		}

		if (key == 'F') {
			ret=sc_f1_regs();
			config.c0_menu=key;
			continue;
		}

		if (key == 'I') {
			rout_ev_inf=sc_regs->ev_info;
			continue;
		}

		if (key == 'J') {
			JT_CONF	*jtc;

			i=sc_regs->ident;
			switch (i &0xF) {
		case 0:  jtc=&config.jtag_scm0; break;
		default: jtc=&config.jtag_scm0; break;
		case 1:  jtc=&config.jtag_scm1; break;
			}

			JTAG_menu(jt_scm_putcsr, jt_scm_getcsr, jt_scm_getdata, jtc, 0);
			continue;
		}

		if (key == 'S') {
			ret=general_setup(-1, -2);
			continue;
		}

		if (key == 'D') {
			last_key=0;
			ret=inp_setdac(sc_conf, 0, -2, DAC_DIAL);
			hd=0;
			continue;
		}

		if (key == 'T') {
			printf("number of 0x%04X ", sc_conf->tst_cnt);
			sc_conf->tst_cnt=(u_int)Read_Hexa(sc_conf->tst_cnt, -1);
			if (errno) continue;

			printf("delay -1,0,..%3d ", sc_conf->tst_delay);
			sc_conf->tst_delay=(int)Read_Hexa(sc_conf->tst_delay, -1);
			if (errno) continue;

			config.c0_menu=rkey;
			i=0;
			do {
				sc_regs->ctrl=TESTPULSE;
				i++;
				if (sc_conf->tst_cnt && (i == sc_conf->tst_cnt)) break;

				if (sc_conf->tst_delay) {
					if (sc_conf->tst_delay >= 0) delay(sc_conf->tst_delay);

					if (kbhit()) break;
				}

			} while (i || !kbhit());

			continue;
		}

		if (   ((key < '0') || (key  > '9'))
			 && ((key < 'A') || (key  > 'F'))) continue;

		i=key-'0';
		if (i > 9) i=i-7;

		if (i > 7) continue;

		if (i == 2) {
			printf("Value  %02X ", (u_int)sc_conf->cmst_data[i]);
			sc_conf->cmst_data[i]=Read_Hexa(sc_conf->cmst_data[i], -1);
		} else {
			if ((i == 3) || (i == 4) || (i == 6) || (i == 7)) {
				printf("Value %04X ", (u_int)sc_conf->cmst_data[i]);
				sc_conf->cmst_data[i]=(u_int)Read_Hexa(sc_conf->cmst_data[i], -1);
			} else {
				printf("Value %08lX ", sc_conf->cmst_data[i]);
				sc_conf->cmst_data[i]=Read_Hexa(sc_conf->cmst_data[i], -1);
			}
		}

		if (errno) continue;

		config.c0_menu=key;
		switch (i) {
case 1:
			cm_reg->cr=sc_conf->cmst_data[i]; break;

case 3:
			sc_regs->cr=(u_int)sc_conf->cmst_data[i]; break;

case 4:
			sc_regs->sr=(u_int)sc_conf->cmst_data[i]; break;

case 5:
			sc_regs->event_nr=sc_conf->cmst_data[i]; break;

case 6:
			sc_regs->ro_delay=(u_short)sc_conf->cmst_data[i]; break;

case 7:
			sc_regs->ctrl=(u_int)sc_conf->cmst_data[i];

			break;
		}
	}
}
//--- end ------------------- system_control_F1 -----------------------------
//
#endif	// if_4

#ifdef SC_M	// if_40
//===========================================================================
//										New System Controller with GPX                =
//===========================================================================
//
//--------------------------- GPX_reset -------------------------------------
//
static int GPX_reset(void)
{
	u_long	tmp;
	u_long	reg4=0x20000FFL;

	sy_regs->ctrl=PURES;
	sy_regs->gpx_reg=0;
	tmp=sy_regs->gpx_data;
	if (BUS_ERROR || (tmp != 0x80)) return CE_FATAL;

	sy_regs->gpx_reg=2;
	sy_regs->gpx_data=0x0000002L;
	sy_regs->gpx_reg=0;
	sy_regs->gpx_data=0x007FC81L;
	sy_regs->gpx_reg=7;
	sy_regs->gpx_data=0x0001FB0L;
	sy_regs->gpx_reg=4;
	sy_regs->gpx_data=reg4;
	sy_regs->gpx_reg=11;
	sy_regs->gpx_data=0x7FF0000L;
	sy_regs->gpx_reg=12;
	sy_regs->gpx_data=0x4000000L;
	sy_regs->gpx_reg=4;
	sy_regs->gpx_data=reg4 |0x0400000L;
	return 0;
}
//
//--------------------------- gpx_resadjust ---------------------------------
//
static int gpx_resadjust(	// 0: ok
	void)
{
	int		ret;
	u_long	tmp;
	u_int		hsdiv_min,hsdiv_max;

	if ((ret=GPX_reset()) != 0) return ret;

	hsdiv_max=0xE0;
	while (1) {
		sy_regs->gpx_reg=7;
		sy_regs->gpx_data=0x0001800L|(REFCLKDIV<<8)|hsdiv_max;
		sy_regs->gpx_reg=12;
		delay(200);
		if (kbhit()) return 0;

		tmp=sy_regs->gpx_data;
		if (BUS_ERROR) return CE_FATAL;

		if (!(tmp &0x400)) break;
		printf("\rmax %02X ", hsdiv_max);
		hsdiv_max--;
		if (hsdiv_max <= 0x60) return CE_F1_NOLOCK;
	}

	hsdiv_min=hsdiv_max;
	do {
		hsdiv_min--;
		sy_regs->gpx_reg=7;
		sy_regs->gpx_data=0x0001800L|(REFCLKDIV<<8)|hsdiv_min;
		sy_regs->gpx_reg=12;
		delay(200);
		if (kbhit()) return 0;

		tmp=sy_regs->gpx_data;
		if (BUS_ERROR) return CE_FATAL;

		printf("\rmin %02X ", hsdiv_min);
	} while (!(tmp &0x400) && hsdiv_min);

	hsdiv_min++;
	printf("\rhsdiv max/min =0x%02X/%02X = %5.1f/%5.1fps => recommend = %5.1f\n",
			 hsdiv_max, hsdiv_min, G_HSDIV/hsdiv_max, G_HSDIV/hsdiv_min, G_HSDIV/(hsdiv_max-8));

	sy_regs->gpx_reg=7;
	sy_regs->gpx_data=0x0001800L|(REFCLKDIV<<8)|(hsdiv_max-8);
	return 0;
}
//
//--------------------------- system_control_GPX ----------------------------
//
const char *sycsr_txt[] ={
	"EF1",  "EF2",  "EFLG", "IFLG", "?", "?", "F1er", "Tmis",
	"Dval", "Treq", "?",    "?",    "?", "?", "Eofl", "Serr"
};
//
int system_control_GPX(void)
{
	int		ret;
	u_int		i;
	u_int		tmp;
	char		hgh;
	char		rkey,key;
	char		hd;
	u_long	ltmp;
	u_long	dbell;

	hd=1; ret=0;
	ret=sc_assign(1);
	if (!sc_conf) return ret;

	while (1) {
		while (kbhit()) getch();
		Writeln();
		if ((ret < CEN_MENU) || (ret > CE_MENU)) {
			if (ret > CE_PROMPT) display_errcode(ret);

			hd=0;
		}

		errno=0; ret=0;
		if (hd) {
//printf("CM Version                     %08lX\n", cm_reg->version);
printf("CM Status Register             %08lX      ", ltmp=cm_reg->sr);
			hgh=0;
			if (ltmp &DBELL_F1_ERR) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("F1err");
			}
			if (ltmp &DBELL_EVENTPF) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("pfull");
			}
			if (ltmp &DBELL_EVENT) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("sevt");
			}
			if (ltmp &1) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("ddblk");
			}
			lowvideo(); cprintf("\r\n");
printf("CM Control Register            %08lX ..1  ", ltmp=cm_reg->cr);
			i=16; hgh=0;
			while (i--)
				if (ltmp &((u_long)0x1 <<i)) {
					if (hgh) highvideo(); else lowvideo();
					hgh=!hgh; cprintf("%s", cmcr_txt[i]);
				}
			lowvideo(); cprintf("\r\n");

printf("CM direct data count           %08lX\n", cm_reg->dd_counter);
printf("CM direct data block size      %08lX\n", cm_reg->dd_blocksz);
printf("doorbell                       %08lX      ", dbell=plx_regs->l2pdbell);
			hgh=0;
			if (dbell &DBELL_F1_ERR) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("F1err");
			}
			if (dbell &DBELL_EVENTPF) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("pfull");
			}
			if (dbell &DBELL_EVENT) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("sevt");
			}
			if (dbell &1) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("ddblk");
			}
			lowvideo(); cprintf("\r\n");

printf("1100 Reset, CM Reset                    ..M/R\n");

printf("online input cards                 %04X\n", inp_bc->card_onl);
printf("ident/serial                   %04X/%3u\n", sy_regs->ident, sy_regs->serial);
printf("Control Register                   %04X ..3  ", tmp=sy_regs->cr);
			i=16; hgh=0;
			while (i--)
				if (tmp &(0x1 <<i)) {
					if (hgh) highvideo(); else lowvideo();
					hgh=!hgh; cprintf("%s", sccr_txt[i]);
				}

			lowvideo(); cprintf("\r\n");

printf("Status Register (sel clr)          %04X ..4  ", tmp=sy_regs->sr);
			i=16; hgh=0;
			while (i--)
				if (tmp &(0x1 <<i)) {
					if (hgh) highvideo(); else lowvideo();
					hgh=!hgh; cprintf("%s", sycsr_txt[i]);
				}

			lowvideo(); cprintf("\r\n");

printf("event number                   %08lX ..5\n", sy_regs->event_nr);
printf("readout delay                      %04X ..6  %3.0fns\n", sy_regs->ro_delay, 25.*sy_regs->ro_delay);
printf("trigger data (last)            %08lX\n", sy_regs->trg_data);
printf("event info (last)              %08lX ..i\n", rout_ev_inf);
printf("GPX addr                             %02X ..7\n", sy_regs->gpx_reg);
printf("GPX data / all data             %07lX ..8/G\n", sy_regs->gpx_data);
printf("Tst(8), F1syn(4), F1rst(2)           %02X ..9\n", sc_conf->cmst_data[9]);
printf("Event FIFO, R-Adjust, Test, Dac, GPXfifo..e/E/A/T/D/F\n");
printf("JTAG, Clear Status, Setup, SC Reset     ..J/C/S/R\n");

			if (BUS_ERROR) CLR_PROT_ERROR();

			printf("                                          %c ",
					 config.c0_menu);
			rkey=getch();
			if ((rkey == CTL_C) || (rkey == ESC)) return CE_MENU;

		} else {
			hd=1; printf("select> ");
			rkey=getch();
			if (rkey == CTL_C) return CE_MENU;

			if ((rkey == ESC) || (rkey == CR)) continue;

			if (rkey == ' ') rkey=config.c0_menu;
		}

		use_default=(rkey == TAB);
		if (use_default || (rkey == CR)) rkey=config.c0_menu;

		if (rkey > ' ') printf("%c\n", rkey);

		key=toupper(rkey);
		if (key == 'M') {
			gigal_regs->cr =CR_RESET;
			continue;
		}

		if (key == 'R') {
			gigal_regs->cr =CR_REM_RESET;
			continue;
		}
//
//		-----------------------------------------------------------------------
//
		if (key == 'C') {
			sy_regs->sr =sy_regs->sr;
			plx_regs->l2pdbell=dbell;
			cm_reg->dd_counter=0;
			sy_regs->event_nr=0;
			continue;
		}

		if (key == 'X') {
			char	lock,up;

			hd=0;
			if ((ret=GPX_reset()) != 0) continue;

			i=0xC0; lock=0; up=0;
			while (1) {
				sy_regs->gpx_reg=7;
				sy_regs->gpx_data=0x0001800L|(REFCLKDIV<<8)|i;
				sy_regs->gpx_reg=12;
				delay(200);
				if (kbhit()) {
					if (getch() != ' ') break;

					getch();
				}

				ltmp=sy_regs->gpx_data;
				if (BUS_ERROR) break;

				if (ltmp &0x400) {
					if (lock) {
						printf("unlocked at %02X\n", i);
						lock=0;
					}
				} else {
					if (!lock) {
						printf("locked at   %02X\n", i);
						lock=1;
					}
				}

				if (up) {
					i++;
					if (i >= 0xC0) up=0;
				} else {
					i--;
					if (i < 0x80) up=1;
				}
			}

			continue;
		}

		if (key == 'A') {
			ret=gpx_resadjust();
			hd=0;
			continue;
		}

		if (key == 'G') {
			tmp=sy_regs->gpx_reg;
			printf("Reg\n");
			for (i=0; i < 15; i++) {
				if ((i == 8) || (i == 9) || (i == 13)) continue;

				printf("%2u: ", i);
				sy_regs->gpx_reg=i;
				printf(" %07lX", sy_regs->gpx_data);
				Writeln();
			}

			sy_regs->gpx_reg=tmp;
			config.c0_menu=rkey;
			hd=0;
			continue;
		}

		if (key == 'E') {
			ret=rd_eventfifo(rkey == 'e');
//			config.c0_menu=rkey;
			continue;
		}

		if (key == 'I') {
			rout_ev_inf=sy_regs->ev_info;
			continue;
		}

		if (key == 'J') {
			JTAG_menu(jt_scm_putcsr, jt_scm_getcsr, jt_scm_getdata, &config.jtag_scgpx, 0);
			continue;
		}

		if (key == 'S') {
			ret=general_setup(-1, -2);
			continue;
		}

		if (key == 'D') {
			last_key=0;
			ret=inp_setdac(sc_conf, 0, -2, DAC_DIAL);
			hd=0;
			continue;
		}

		if (key == 'T') {
			printf("number of 0x%04X ", sc_conf->tst_cnt);
			sc_conf->tst_cnt=(u_int)Read_Hexa(sc_conf->tst_cnt, -1);
			if (errno) continue;

			printf("delay -1,0,..%3d ", sc_conf->tst_delay);
			sc_conf->tst_delay=(int)Read_Hexa(sc_conf->tst_delay, -1);
			if (errno) continue;

			config.c0_menu=rkey;
			i=0;
			do {
				sy_regs->ctrl=TESTPULSE;
				i++;
				if (sc_conf->tst_cnt && (i == sc_conf->tst_cnt)) break;

				if (sc_conf->tst_delay) {
					if (sc_conf->tst_delay >= 0) delay(sc_conf->tst_delay);

					if (kbhit()) break;
				}

			} while (i || !kbhit());

			continue;
		}

		if (key == 'F') {
			u_int	cofs;

			sy_regs->cr=0;
			sy_regs->gpx_reg=4;
			ltmp=sy_regs->gpx_data;
			sy_regs->gpx_data=ltmp |0x0400000L;		// Master Reset
			i=0;
			while (!kbhit()) {
				tmp=sy_regs->sr;
				if (BUS_ERROR) break;

				if ((tmp &0x3) == 0x3) continue;

				if (!(tmp &0x1)) {
					cofs=0;
					sy_regs->gpx_reg=8;
				} else {
					cofs=4;
					sy_regs->gpx_reg=9;
				}

				ltmp=sy_regs->gpx_data;
				printf(" %u%02X%u%05lX",
						 (u_int)(ltmp >>26)+cofs, (u_int)(ltmp >>18) &0xFF,
						 (u_int)(ltmp >>17) &1, ltmp &0x1FFFFL);
				i++;
			}

			if (i %8) Writeln();
			hd=0;
			continue;
		}

		if (   ((key < '0') || (key  > '9'))
			 && ((key < 'A') || (key  > 'F'))) continue;

		i=key-'0';
		if (i > 9) i=i-7;

		if (i > 9) continue;

		if (i == 2) {
			printf("Value  %02X ", (u_int)sc_conf->cmst_data[i]);
			sc_conf->cmst_data[i]=Read_Hexa(sc_conf->cmst_data[i], -1);
		} else {
			if ((i == 3) || (i == 4) || (i == 6) || (i == 7) || (i == 9)) {
				printf("Value %04X ", (u_int)sc_conf->cmst_data[i]);
				sc_conf->cmst_data[i]=(u_int)Read_Hexa(sc_conf->cmst_data[i], -1);
			} else {
				printf("Value %08lX ", sc_conf->cmst_data[i]);
				sc_conf->cmst_data[i]=Read_Hexa(sc_conf->cmst_data[i], -1);
			}
		}

		if (errno) continue;

		config.c0_menu=key;
		switch (i) {
case 1:
			cm_reg->cr=sc_conf->cmst_data[i]; break;

case 3:
			sy_regs->cr=(u_int)sc_conf->cmst_data[i]; break;

case 4:
			sy_regs->sr=(u_int)sc_conf->cmst_data[i]; break;

case 5:
			sy_regs->event_nr=sc_conf->cmst_data[i]; break;

case 6:
			sy_regs->ro_delay=(u_short)sc_conf->cmst_data[i]; break;

case 7:
			sy_regs->gpx_reg=(u_short)sc_conf->cmst_data[i]; break;

case 8:
			sy_regs->gpx_data=sc_conf->cmst_data[i]; break;

case 9:
			sy_regs->ctrl=(u_int)sc_conf->cmst_data[i];

			break;
		}
	}
}
//--- end ------------------- system_control_GPX ----------------------------
//
//===========================================================================
//										general setup                                 =
//===========================================================================
//
//------------------------------ scm_state ----------------------------------
//
// return F1 status bits
//
static int scm_state(		// -1: error
	COUPLER	*sregs)
{
	int	ret;

	ret=sregs->sr;
	if (BUS_ERROR) ret=-1;

	return ret;
}
//
//------------------------------ scm_f1reg ----------------------------------
//
static int scm_f1reg(		// -1: error
	COUPLER	*sregs,
	u_int		reg,
	u_int		value)
{
	u_int		i;

	sregs->f1_reg[reg]=value; i=0;
	while ((gtmp=sregs->sr) &WRBUSY) {
		if (++i > 5) {
			config.sc_f1_reg[reg]=0xFFFF;
			if (BUS_ERROR) return -1;

			return -1;
		}
	}

	config.sc_f1_reg[reg]=value;
	return 0;
}
//
//------------------------------ sc_gpx_setup -------------------------------
//
static int sc_gpx_setup(		// 0: no error
	u_char	letra)
{
	u_int		i;
	u_long	reg4;
	u_long	ltmp;

	sy_regs->ctrl=PURES;
	sy_regs->gpx_reg=0;
	ltmp=sy_regs->gpx_data;					// test value on reset
	if (BUS_ERROR || (ltmp != 0x80)) return CE_FATAL;

	sy_regs->gpx_reg=2;
	sy_regs->gpx_data=0x0000002L;	// I-Mode

	ltmp=0x0000481L;					// Tstrt, ROsc
	if (letra &0x01) ltmp |=0x0003800L;
	if (letra &0x02) ltmp |=0x0700000L;
	sy_regs->gpx_reg=0;
	sy_regs->gpx_data=ltmp;

	sy_regs->gpx_reg=7;	// start PLL
	sy_regs->gpx_data=0x0001800L|(REFCLKDIV<<8)|config.g_hsdiv;

	reg4=(config.f1_sync == 0) ? 0 :
		  (config.f1_sync == 1) ? G_ST_TMR-1 : 1;
	sy_regs->gpx_reg=4;
	sy_regs->gpx_data=0x2000000L|reg4;	// EFlag

	ltmp=(config.f1_sync < 2) ? 0x2000000L : 0xA000000L;	// StartRetrig | MasterOenTrig
	sy_regs->gpx_reg=5;
	sy_regs->gpx_data=ltmp |config.g_startoff1;

	sy_regs->gpx_reg=11;
	sy_regs->gpx_data=0x7FF0000L;	// enable err flags
	sy_regs->gpx_reg=12;
	sy_regs->gpx_data=0x4000000L;	// enable start'high to interrupt
	sy_regs->gpx_reg=4;
	sy_regs->gpx_data=0x2000000L |reg4 |0x0400000L;	// Master Reset

	i=0;
	do {
		sy_regs->gpx_reg=12;
		if (!((ltmp=sy_regs->gpx_data) &0x0400)) break;

		if (BUS_ERROR) return CE_FATAL;

	} while (++i < 10000);


//printf("i=%i\n", i);
	if (ltmp &0x0400) {
		printf("SYS GPX: no PLL lock!\n");
		use_default=0; WaitKey(0);
	}

	return 0;
}
//
//------------------------------ sc_f1_setup --------------------------------
//
static int sc_f1_setup(		// 0: no error
	COUPLER	*sregs,
	u_char	letra)
{
	int		ret;
	u_int		i;
	u_int		edge;

	sregs->ctrl=PURES;
	if ((ret=scm_state(sregs)) == -1) return 12;

	if (!(ret &NOINIT)) return 10;

	for (i=0; i < 16; config.sc_f1_reg[i++]=0xFFFF);

	scm_f1reg(sregs, 0, 0);
	if (letra &4) {
		scm_f1reg(sregs, 1, R1_LETRA);
	} else {
		scm_f1reg(sregs, 1, 0);
	}

	edge=0;
	if (letra &1) edge |=0x4040;
	if (letra &2) edge |=0x8080;

	scm_f1reg(sregs, 2, edge);
	scm_f1reg(sregs, 3, edge);
	scm_f1reg(sregs, 4, edge);
	scm_f1reg(sregs, 5, edge);
	scm_f1reg(sregs, 10, 0x1800|(REFCLKDIV<<8)|config.hsdiv);
	if (config.f1_sync == 1) {
		scm_f1reg(sregs, 15, R15_ROSTA|R15_SYNC|R15_COM|R15_8BIT);
		delay(10);	// >= 4
		ret=scm_f1reg(sregs, 7, R7_BEINIT|((REFCNT-2)<<6));
	} else {
		scm_f1reg(sregs, 15, R15_ROSTA|R15_COM|R15_8BIT);
		ret=scm_f1reg(sregs, 7, R7_BEINIT);
	}

	if (ret) return ret;

	for (i=0; (scm_state(sregs) &(NOLOCK|NOINIT)) && (i < 10); i++) delay(100);

	if (scm_state(sregs) &(NOLOCK|NOINIT)) {
		printf("restart\n");
		scm_f1reg(sregs, 7, 0);
		delay(10);
		scm_f1reg(sregs, 7, (config.f1_sync == 1) ? R7_BEINIT|((REFCNT-2)<<6)
													  : R7_BEINIT);
	}

// Achtung: LOCK kommt und geht eventuell wieder weg!
	delay(300);
	i=0;
	while (scm_state(sregs) &(NOLOCK|NOINIT)) {
		delay(100);
		if (++i < 10) continue;

		printf("Fehlerbit gesetzt!\n");
		return -1;
	}

	return 0;
}
//
#endif	// if_40
//------------------------------ inp_setdac ---------------------------------
//
int	last_thr_dac;

int inp_setdac(	//  0: ok
	SC_DB		*sconf,		//  0 alle
	INP_DB	*inp_cnf,	//  0 alle
	int		con,			// -1 single
								// -2 alle
	int		dac)			//>=0 alle auf diesen Wert setzen
								// -1 Dialog
								// -2 um 1 erhoehen
								// -3 um 1 erniedrigen
{
	int		ret;
	int		i;
	F1TDC_RG	*f1tdc_rg;
	float		thrh;
	int		xdac;

//printf("inp_setdac(%08lX, %08lX, %i, %i)\n", sconf, inp_cnf, con, dac);
	if (!sconf) {
		if (dac < 0) Writeln();
		if ((ret=inp_setdac(&config.sc_db, inp_cnf, con, dac)) != 0) return ret;

		return 0;
	}

	if (!inp_cnf) {
		for (i=0; i < sconf->slv_cnt; i++) {
			if (sconf->inp_db[i].type != F1TDC) continue;

			if (i && (dac < 0)) Writeln();
			if ((ret=inp_setdac(sconf, sconf->inp_db+i, con, dac)) != 0) return ret;
		}

		return 0;
	}

	if (con == -2) {
		for (i=0; i < 4; i++)
			if ((ret=inp_setdac(sconf, inp_cnf, i, dac)) != 0) return ret;

		return 0;
	}

	if (con < 0) {
		printf("unit %2u connector 0..3                       %u ",
				 inp_cnf->unit, inp_cnf->inp_con);
		inp_cnf->inp_con=(u_int)Read_Deci(inp_cnf->inp_con, 3) &0x3;
		if (errno) return CE_MENU;

		con=inp_cnf->inp_con;
	}

	if ((xdac=dac) < 0) {
		if (dac == DAC_DIAL) {
			if (last_key == CTL_A) xdac=last_thr_dac;
			else {
				xdac=inp_cnf->inp_dac[con];
				thrh=THRH_MIN+THRH_SLOPE*xdac;
				if (std_setup && (thrh < 0.0)) thrh=0.;

				printf("unit %2u con %u: threshold (%5.*f bis %4.*f) %*.*f ",
								 inp_cnf->unit, con,
								 THRHW, THRH_MIN, THRHW, THRH_MAX, THRHW+3, THRHW, thrh);
				thrh=Read_Float(thrh);
				if (errno) return CE_MENU;

				xdac=(u_int)((thrh-THRH_MIN)/THRH_SLOPE +0.5);
			}
		} else {
			if (dac == DAC_ADD1) xdac=inp_cnf->inp_dac[con]+1;
			else xdac=inp_cnf->inp_dac[con]-1;
		}
	}

	if (xdac < 0) xdac=0;
	if (xdac > 0xFF) xdac=0xFF;
	last_thr_dac=xdac;
	f1tdc_rg=inp_cnf->inp_regs;
	if (dac < 0) inp_cnf->inp_dac[con]=xdac;
//printf("threshold set to %5.3f\n", THRH_MIN+THRH_SLOPE*xdac);
	CLR_PROT_ERROR();
	f1tdc_rg->f1_addr=2*con;
	if ((ret=xinp_f1reg(f1tdc_rg, 11, xdac)) != 0) return ret;

	xinp_f1reg(f1tdc_rg, 1, (config.f1_letra &0x04) ? R1_LETRA|R1_DACSTRT : R1_DACSTRT);

	return 0;
}
//
//--------------------------- general_setup ---------------------------------
//
int general_setup(		// 0: no error
	int	sc,	// system controller
					//  -3: keiner, use sc_conf
					//  -2: alle
					//  -1: master
					// >=0: slave
	int	inp)	//  -2: keine (nur system controller)
					//  -1: alle
					//  -3: alle fuer DAQ
					// >=0: input unit
{
	int		ret;
	float		resol,g_resol;

printf("general_setup(%d, %d)\n", sc, inp);
	ret=sc_assign(1);
	if (ret == CE_NO_INPUT) {
		if (inp != -1) display_errcode(ret);
		else inp=-2;
	} else {
		if (ret) return ret;
	}

	CLR_PROT_ERROR();
	if (std_setup) {
		resol=120.0;
		config.hsdiv=(u_int)(HSDIV/resol +.5);
		g_resol=85.0;
		config.g_hsdiv=(u_int)(G_HSDIV/g_resol +.5);
		config.f1_sync=2;
		config.f1_letra=0x11;
	} else {
		resol=HSDIV/config.hsdiv;
		if ((resol < 120.) || (resol > 150.)) {
			use_default=0;
			if (inp == -3) inp=-1;
		}

		g_resol=G_HSDIV/config.g_hsdiv;
		if ((g_resol < 83.) || (g_resol > 110.)) {
			use_default=0;
			if (inp == -3) inp=-1;
		}

		if (inp != -3) {
			printf("F1 resolution 129.95 *3 conforms to GPX Resolution 97.47 *4\n");
			printf("select F1  resolution %6.2f ", resol);
			resol=Read_Float(resol);
			if (errno) return CEN_MENU;

			config.hsdiv=(u_int)(HSDIV/resol +.5);

			printf("select GPX resolution %6.2f ", g_resol);
			g_resol=Read_Float(g_resol);
			if (errno) return CEN_MENU;

			config.g_hsdiv=(u_int)(G_HSDIV/g_resol +.5);

			printf("select GPX StartOffset %05lX ", config.g_startoff1);
			config.g_startoff1=Read_Hexa(config.g_startoff1, 0x3FFFFL);
			if (errno) return CEN_MENU;
		}

		printf("selected F1  hsdiv =0x%02x => %6.2fps\n", config.hsdiv, RESOLUTION);
		printf("selected GPX hsdiv =0x%02x => %6.2fps\n", config.g_hsdiv, G_RESOLUTION);


		if (inp == -3) config.f1_sync=2;
		else {
			printf("synchron mode (2=ex) %2d ", config.f1_sync);
			config.f1_sync=(int)Read_Deci(config.f1_sync, 2);
			if (errno) return CEN_MENU;
		}


		if (inp == -3) {
			printf("letra/falling/rising %02X ", config.f1_letra &0x7);
			config.f1_letra=(u_char)Read_Hexa(config.f1_letra, 0x7F);
			if (errno) return CEN_MENU;

			config.f1_letra =(config.f1_letra&0x7) |0x10;
		} else {
			printf("letra/falling/rising %02X ", config.f1_letra);
			config.f1_letra=(u_char)Read_Hexa(config.f1_letra, 0x7F);
			if (errno) return CEN_MENU;
		}
	}

	while (sc < 0) {
		u_int		i;
		SC_DB		*sconf;
		F1TDC_BC	*ibcregs;
		COUPLER	*sregs;
		u_int		slv;

		sconf=&config.sc_db;
		ibcregs=sconf->inp_bc;
		sregs=sconf->sc_unit;
#ifndef PBUS
		if (sregs && (sc > -3)) {	// else keiner
			printf("system controller\n");
			if (sy_regs) ret=sc_gpx_setup(config.f1_letra>>4);
			else ret=sc_f1_setup(sregs, config.f1_letra>>4);

			if (ret) break;
		}
#endif

		if (inp != -2) {
			ret=inp_assign(sconf, 1);
			if (ret && (ret != CE_NO_INPUT)) break;
		}

		if ((inp != -2) && !ret) {
			for (i=0; i < 16; config.inp_f1_reg[i++]=0xFFFF);

			slv=inp;
			if (inp < 0) slv=0;
			last_key=0;
			for (; slv < sconf->slv_cnt; slv++) {
				INP_DB	*iconf;
				F1TDC_RG	*iregs;
				u_int		edge;

				iconf=sconf->inp_db+slv;
				if ((iconf->type == GPXTDC) || (iconf->type == GPXTDCTST)) {
					GPX_RG	*gregs=iconf->inp_regs;
					u_long	reg4;
					u_long	ltmp;
					u_int		gpx,msk;
					u_int		gpxnum=(inp_conf->type == GPXTDC) ? 8 : 1;

					Writeln();
					printf("input unit %u:\n", slv);

					gregs->ctrl=PURES;
					gregs->gpx_seladdr=0x80 |0;
					ltmp=gregs->gpx_data;					// test value on reset
					if (BUS_ERROR || (ltmp != 0x80)) return CE_FATAL;

					gregs->gpx_seladdr=0x80 |2;
					gregs->gpx_data=0x0000002L;	// I-Mode

					ltmp=0x0000481L;					// Tstrt, ROsc
					if (config.f1_letra &0x01) ltmp |=0x007F800L;
					if (config.f1_letra &0x02) ltmp |=0xFF00000L;
					gregs->gpx_seladdr=0x80 |0;
					gregs->gpx_data=ltmp;

					gregs->gpx_seladdr=0x80 |7;	// start PLL
					gregs->gpx_data=0x0001800L|(REFCLKDIV<<8)|config.g_hsdiv;

					reg4=(config.f1_sync == 0) ? 0 :
						  (config.f1_sync == 1) ? G_ST_TMR-1 : 1;
					gregs->gpx_seladdr=0x80 |4;
					gregs->gpx_data=0x2000000L|reg4;	// EFlag

					ltmp=(config.f1_sync < 2) ? 0x2000000L : 0xA000000L;	// StartRetrig | MasterOenTrig
					gregs->gpx_seladdr=0x80 |5;
					gregs->gpx_data=ltmp |config.g_startoff1;

					gregs->gpx_seladdr=0x80 |11;
					gregs->gpx_data=0x7FF0000L;	// enable err flags
					gregs->gpx_seladdr=0x80 |12;
					gregs->gpx_data=0x4000000L;	// enable start'high to interrupt
					gregs->gpx_seladdr=0x80 |4;
					gregs->gpx_data=0x2000000L |reg4 |0x0400000L;	// Master Reset

					i=0; msk=0;
					do {
						for (gpx=0; gpx < gpxnum; gpx++) {
							gregs->gpx_seladdr=(gpx<<4) |12;
							if (!((ltmp=gregs->gpx_data) &0x0400)) msk|=(1<<gpx);
						}

						if (BUS_ERROR) return CE_FATAL;

					} while ((++i < 10000) && (msk != ((1<<gpxnum)-1)));


//printf("i=%i\n", i);
					if (msk != ((1<<gpxnum)-1)) {
						for (gpx=0; gpx < gpxnum; gpx++)
							if (!(msk &(1<<gpx)))
								printf("GPX %u: no PLL lock!\n", gpx);

						use_default=0; WaitKey(0);
					}

					if (inp >= 0) break;

					continue;
				}

				if (iconf->type != F1TDC) {
					if (inp >= 0) break;

					continue;
				}

				Writeln();
				iregs=iconf->inp_regs;
				iregs->cr=0;
				printf("input unit %u:\n", slv);
				if ((ret=inp_setdac(sconf, iconf, -2, DAC_DIAL)) != 0) break;

				iregs->ctrl=PURES;
				iregs->f1_addr=F1_ALL;
				for (i=0; i < 8; i++) {
					if (iregs->f1_state[i] != (NOLOCK|NOINIT)) {
						printf("unit %2u, F1 %u illegal status\n", slv, i);
						if (WaitKey(0)) break;
					}
				}

				xinp_f1reg(iregs, 0, 0);
				if (config.f1_letra &0x04) {
					xinp_f1reg(iregs, 1, R1_LETRA);
				} else {
					xinp_f1reg(iregs, 1, 0);
				}

				edge=0;
				if (config.f1_letra &0x01) edge |=0x4040;
				if (config.f1_letra &0x02) edge |=0x8080;

				xinp_f1reg(iregs, 2, edge);
				xinp_f1reg(iregs, 3, edge);
				xinp_f1reg(iregs, 4, edge);
				xinp_f1reg(iregs, 5, edge);
				xinp_f1reg(iregs, 10, 0x1800|(REFCLKDIV<<8)|config.hsdiv);
				if (config.f1_sync == 1) {
					xinp_f1reg(iregs, 15, R15_ROSTA|R15_SYNC|R15_COM|R15_8BIT);
					delay(10);	// >= 4
					ret=xinp_f1reg(iregs, 7, R7_BEINIT|((REFCNT-2)<<6));
				} else {
					xinp_f1reg(iregs, 15, R15_ROSTA|R15_COM|R15_8BIT);
					ret=xinp_f1reg(iregs, 7, R7_BEINIT);
				}

				if (ret) break;

				for (i=0; i < 4; i++) {
					iregs->f1_addr=2*i;
					xinp_f1reg(iregs, 11, iconf->inp_dac[i]);
					xinp_f1reg(iregs, 1, (config.f1_letra &0x04) ? R1_LETRA|R1_DACSTRT : R1_DACSTRT);
				}

				for (i=0; ((gtmp=ibcregs->f1_error) &(1<<slv)) && (i < 10); i++) {
#if 0
					u_int f1;

					printf(" %04X:", gtmp);
					for (f1=0; f1 < 8; printf(" %02X", iregs->f1_state[f1++]));
					Writeln();
#endif
					delay(100);
				}

				if ((gtmp=ibcregs->f1_error) &(1<<slv)) {
//printf("e: %04X\n", ibcregs->f1_error);
					printf("restart\n");
					iregs->f1_addr=F1_ALL;
					xinp_f1reg(iregs, 7, 0);
					delay(10);
					xinp_f1reg(iregs, 7, (config.f1_sync == 1) ? R7_BEINIT|((REFCNT-2)<<6)
																  : R7_BEINIT);
				}

		// Achtung: LOCK kommt und geht eventuell wieder weg!
				delay(300);
				i=0;
				while ((gtmp=ibcregs->f1_error) &(1<<slv)) {
//printf("E: %04X\n", ibcregs->f1_error);
					delay(100);
					if (++i < 10) continue;

					printf("Fehlerbit gesetzt!\n");
					printf("F1 status 0..7: ");
					for (i=0; i < 8; i++) {
						if (i) printf(",");
						printf("%02X", iregs->f1_state[i]);
					}

					Writeln();
					use_default=0; WaitKey(0);
					break;
				}

				if (inp >= 0) break;
			}

			if (ret) break;
		}

		if (!sregs || (sc <= -3)) break;

		sregs->cr=(config.f1_sync == 2) ? 0 : MCR_NO_F1STRT;
		if (config.f1_sync) {
			sregs->ctrl=SYNCRES;
			if (config.f1_sync == 1) sregs->ctrl=F1START;
		} else sregs->ctrl=F1START;	// start all F1 and counter for synchron trigger signal

		break;
	}

	f1_range=(long)(6400000.0/RESOLUTION +0.5);
//	gpx_range=(long)(6400000.0/G_RESOLUTION +0.5);
	gpx_range=((u_long)G_ST_TMR*216*config.g_hsdiv) >>REFCLKDIV;
	if (!config.f1_sync) f1_range=0x10000L;
	printf("F1  range: 0x%05lX(%1.1fns)\n", f1_range, f1_range*RESOLUTIONNS);
	printf("GPX range: 0x%05lX(%1.1fns)\n", gpx_range, gpx_range*G_RESOLUTIONNS);

	return ret;
}
//--- end ------------------- general_setup ---------------------------------
//
//--------------------------- verify_dac ------------------------------------
//
static int verify_dac(void)	//  0: no key pressed
										// CEN_DAC_CHG: DAC geaendert
										// else: CE_KEY_PRESSED or error
{
	int		ret;
	u_char	key0,key1;
	int		iu;
	SC_DB		*sconf;

	if (!kbhit()) return 0;

	if ((key0=getch()) == 0) key1=getch();

//printf("\n%3d %3d\n", key0,key1);
	sconf=&config.sc_db;
	if (sconf->dac_con > 4) sconf->dac_con=4;

	if (!key0) {
		if ((key1 == LEFT) || (key1 == RIGHT)) {			// next F1TDC Card
			if ((iu=sconf->dac_iunit) >= 0) {
				if (key1 == RIGHT) {
					while (--iu >= 0)
						if (sconf->inp_db[iu].type == F1TDC) break;
				} else {
					while (++iu < sconf->slv_cnt)
						if (sconf->inp_db[iu].type == F1TDC) break;
				}

				if ((iu >= 0) && (iu < sconf->slv_cnt)) {
					sconf->dac_iunit =iu;
					sconf->dac_iconf=sconf->inp_db+iu;
					sconf->dac_iregs=sconf->dac_iconf->inp_regs;
				}
			}

			printf("\ninput unit = %d\n", sconf->dac_iunit);
			return CEN_FPRINT;
		}

		if ((key1 == CTLUP) || (key1 == CTLDOWN)) {		// next Connector
			if (key1 == CTLUP) {
				if (sconf->dac_con < 4) sconf->dac_con++;
			} else {
				if (sconf->dac_con) sconf->dac_con--;
			}

			if (sconf->dac_con < 4) printf("\nconnector = %d\n",
														sconf->dac_con);
			else printf("\nconnector = all\n");

			return CEN_FPRINT;
		}

		if ((key1 == UP) || (key1 == DOWN)) {				// next DAC Value
			u_int	con;
			u_int	dac;

			if (sconf->dac_iunit < 0) return 0;

			if ((con=sconf->dac_con) >= 4) con=0;

			dac=sconf->dac_iconf->inp_dac[con];
			if (key1 == UP) {
				if (dac < 255) dac++;
			} else {
				if (dac) dac--;
			}

			if (sconf->dac_con < 4) sconf->dac_iconf->inp_dac[con]=dac;
			else for (con=0; con < 4; sconf->dac_iconf->inp_dac[con++]=dac);

			printf("\nthreshold %u/", sconf->dac_iunit);
			if (sconf->dac_con < 4) printf("%u", con); else printf("all");
			printf(" set to %*.*f\n",
					 THRHW+3, THRHW, THRH_MIN+THRH_SLOPE*dac);
			if (sconf->dac_con < 4) {
				sconf->dac_iregs->f1_addr=2*con;
				if ((ret=xinp_f1reg(sconf->dac_iregs, 11, dac)) != 0) return ret;
				xinp_f1reg(sconf->dac_iregs, 1, (config.f1_letra &0x04) ?
																R1_LETRA|R1_DACSTRT : R1_DACSTRT);
				return CEN_DAC_CHG;
			}

			for (con=0; con < 4; con++) {
				sconf->dac_iregs->f1_addr=2*con;
				if ((ret=xinp_f1reg(sconf->dac_iregs, 11, dac)) != 0) return ret;
				xinp_f1reg(sconf->dac_iregs, 1, (config.f1_letra &0x04) ?
																R1_LETRA|R1_DACSTRT : R1_DACSTRT);
			}

			return CEN_DAC_CHG;
		}

		return (CE_KEY_PRESSED|CE_KEY_SPECIAL) |key1;
	}

	if (key0 == 't') {
		sconf->sc_unit->ctrl=TESTPULSE;
		return 0;
	}

	if (key0 == 'T') {
		sconf=&config.sc_db;
		sconf->sc_unit->ctrl=TESTPULSE;
		return 0;
	}

	if (toupper(key0) == 'O') {
		ret=pciconf.errlog; pciconf.errlog=1;
		inp_overrun();
		pciconf.errlog=ret;
		return 0;
	}

	if (toupper(key0) == 'D') {
		errno=0; use_default=0; last_key=0;
		Writeln();
		inp_setdac(sconf, sconf->dac_iconf, -2, DAC_DIAL);
		return CEN_PPRINT;
	}

   return CE_KEY_PRESSED |key0;
}
//
#ifdef SC_M
//--------------------------- direct_read16 ---------------------------------
//
static int direct_read16(u_short *addr, u_short *data)
{
	u_int		perr;
	u_int		lo=0;
	u_long	hdr;
	u_long	da;

	*data=*addr;
	while (1) {
		if ((perr=(u_int)gigal_regs->prot_err) == 0) return 0;

		if (perr == LPE_DLOCK) break;

		if ((perr != PE_DLOCK) && (perr != PE_TO)) return perr;

		if (perr == PE_DLOCK) {
			if (++lo >= 1000) return perr;

			_delay_(500);
		} else {
			if (++lo >= 10) return perr;
		}
	}

	while ((perr=(u_int)gigal_regs->prot_err) == PE_DLOCK)
		if (++lo >= 1000) return perr;

	if (perr) return perr;

	hdr=gigal_regs->tc_hdr;
	if ((hdr &0x300) == P_ECON) return 0x200 |(u_int)(hdr >>24);

	da=gigal_regs->tc_da;
	if (hdr &0x0C000000L) *data=(u_int)(da >>16);
	else *data=(u_int)da;

	gigal_regs->sr=SR_PROT_ERROR|SR_PROT_END;
//if ((long)glp != -1) printf("\r%10ld: ", glp);
//printf("recovery %08lX %08lX %04X\n", hdr, da, *data);
	return 0;
}
//
#endif
//===========================================================================
//
//------------------------------ read_event ---------------------------------
//
#ifdef SC_M
#define C_DDMA		0x100		// damit ring buf nicht voll wird, max is SZ_PAGE

#define C_ROBUF	0x4000
static u_long		*robuf=0;

u_long	kbatt;
static u_long	*bcbuf;		// byte count for demand DMA
static u_int	bcbix;		// index for byte count buffer
static u_int	bcblen;		// rest length of byte count buffer
static u_long	dd_ix;		// demand DMA ring buf index
static u_long	dd_next;		// demand DMA ring buf currend end
static u_long	dd_max;		// demand DMA ring buf size
static u_char	ddat;			// demant data ready
static u_int	lerr;
#endif

u_long	*robf;

u_int		rout_len;
char		rout_hdr;
u_long	rout_trg_time;
u_long	rout_ev_inf;
u_int		tm_cnt;						// Anzahl Zeitmarken, sortiert nach Kanal und Zeit
TM_HIT	*tm_hit=0;					// Zeitmarken (Kanal und Zeit)
long		tm_win_le,tm_win_he;		// trigger window, low/high edge
long		tlat,twin;

long		f1_range;					// max value of time low word (6.4us)
long		gpx_range;					// dto.
long		pause;
u_long	eventcount=0;

static SC_DB	*i_sconf;			// readout from input cards

#ifndef PBUS
u_long	kbatt_msk;
static u_int	rlen;					// mehrere Events im DMA Buffer
static u_long	evcounter;			// zur Kontrolle

static u_int	mev;					// max. Anzahl Events gefunden im DMA Buffer
static char		tstlen;				// zur Kontrolle der Eventlaengen, muessen gleich bleiben
static u_long	evlen;				// zur Kontrolle, Eventlaengen der Controller
#endif

int read_event(
	char	mode,
	int	trig)		// -3: TO_XRAQ, multi events too
						// -2: RO_RAW,  raw
						// -1: RO_TRG,  passend zu rout_trg_time
						//  c: trigger channel
{
	int		ret;
#ifdef SC_M
	u_int		req;
	u_long	ltmp;
	u_long	dbell;
#endif
	SC_DB		*sconf;
#ifndef PBUS
	COUPLER	*sregs;
#endif
	u_int		i,j,h;
	u_int		lx;			// count of long words

//printf("read_event(%02X, %i);\n", mode, trig);
#ifdef SC_M
	if (protocol_error(glp, "RO")) return CE_PROMPT;
	gigal_regs->sr =gigal_regs->sr;
#endif
	tm_cnt=0; pause=0;
	while (1) {			// Rohdaten und Auswertung
		while (1) {		// Rohdaten
#ifndef PBUS
			if (rlen) {
//printf("%04X %04X %08lX\n", rlen, rout_len, robf[0]);
				robf +=(rout_len/4);
				rout_len=(u_int)robf[0];
				if (!rout_len || (rout_len > rlen)) {
					return CE_FATAL;
				}

				rlen -=rout_len;
//printf("%04X %04X %08lX\n", rlen, rout_len, robf[0]); if (WaitKey(0)) return 20;
				break;
			}
#endif

			rout_len=0;
#ifdef SC_M		// if_5
			if (config.daq_mode &(EVF_NOBCBUF|EVF_DDATA)) {	// demand DMA
//---------------
// demand DMA
//---------------
				if (!(++kbatt &kbatt_msk)) {
					if (mode &EVF_NODAC) {
						if (kbhit()) {
							if ((ret=getch()) != 0) return CE_KEY_PRESSED|ret;

							return (CE_KEY_PRESSED|CE_KEY_SPECIAL) |getch();
						}
					} else {
						if ((ret=verify_dac()) != 0) return ret;
					}
				}

				dbell=plx_regs->l2pdbell;
				plx_regs->l2pdbell=dbell;
//if (dbell) { printf("dbell %08lX\n", dbell); if (WaitKey(0)) return 21; }
				if (dbell &DBELL_F1_ERR) {
					printf("DBELL: F1 error at =%02X(%08lX)\n", (u_int)(dbell >>24), dbell);
					if (WaitKey(0)) return 22;
				}

				if (dbell &DBELL_BLOCK) { 
//printf("%08lX %08lX\n", gigal_regs->d0_bc, cm_reg->dd_counter);

					if (ddat) {
						printf("direct data overflow\n");
						return CE_PROMPT;
					}

					ddat=1; dd_next+=(config.dd_block>>2);
				}

				if (ddat) {
					lx=(u_int)DBUF_L(dd_ix)>>2;
					if (((DBUF_L(dd_ix) >>16) != 0x8000) || (lx < 3)) {
						printf("format error 3\n");
						return CE_PROMPT;
					}

					if ((dd_ix+lx) > dd_next) ddat=0;
					else {
						for (i=0; i < lx; i++) {
							robuf[i] =DBUF_L(dd_ix); dd_ix++;
							if (dd_ix >= dd_max) { dd_ix=0; dd_next -=dd_max; }
						}

						rout_len=lx<<2;
						break;
					}
				}

				pause++;
				continue;
#if 0
				if (bcblen == gigal_regs->d0_bc_len) {
					printf("no ddt data\n");
					if (WaitKey(0)) return 25;
					pause++; continue;
				}

				lerr=(u_int)gigal_regs->prot_err;
				if (lerr && (lerr != LPE_DLOCK) && (lerr != LPE_RESOURCE)) {
					cm_reg->cr =0;
					gigal_regs->d0_bc_adr =dmabuf_phy[dmapages-1] |0x1;
					gigal_regs->d0_bc_len =SZ_PAGE;

					printf("prot_err 0 %03X\n", lerr);
//	printf("prot_err =%03X, ", (u_int)gigal_regs->prot_err);
//	printf("balance  =%3d\n", (u_int)gigal_regs->balance);
					protocol_error(glp, "DD.1");
					return CE_FATAL;
				}

				bcblen -=4;
				rout_len=(u_int)bcbuf[bcbix++];
				if (bcblen == 0) {
					bcbix=0;
					gigal_regs->d0_bc_adr =dmabuf_phy[dmapages-1] |0x1;
					gigal_regs->d0_bc_len =bcblen =C_DDMA;
				}

//	if (protocol_error(glp, "DD.2")) return CE_PROMPT;

//	printf("rout_len =%04X\n", rout_len);
				lx=rout_len /4;
				for (i=0; i < lx; i++) {
					robuf[i] =DBUF_L(dd_ix); dd_ix++;
					if (dd_ix >= dd_max) dd_ix=0;
				}

printf("total count: %08lX\n", cm_reg->dd_counter);
				i=(u_char)(robuf[1] >>16);
				if (i == 15) i=0; else i++;
				if (robuf[2] != evcounter) {
					printf("lost event, ex/is %08lX/%08lX, ix %04lX\n",
							 evcounter, robuf[1], dd_ix);
					cm_reg->cr =0;
					gigal_regs->d0_bc_adr =dmabuf_phy[dmapages-1] |0x1;
					gigal_regs->d0_bc_len =SZ_PAGE;

					protocol_error(glp, "DD.3");
					return CE_FATAL;
				}

				evcounter++;
				break;
#endif
			} else {
//---------------
// DMA read block
//---------------
				setup_dma(-1, 0, 0, dmapages*(u_long)SZ_PAGE, 0, 0,
							 (rout_hdr) ? DMA_L2PCI
											: (DMA_NOEOT|DMA_L2PCI));	// um mehrere Karten zu lesen
				gigal_regs->t_hdr =HW_BE0011 |HW_L_DMD |HW_R_BUS |HW_EOT |HW_FIFO |HW_BT |THW_SO_AD;
				gigal_regs->t_da  =dmapages*(u_long)SZ_PAGE;
				req=0;
				while (1) {			// Rohdaten
					F1TDC_BC	*ibcregs;
					INP_DB	*iconf;

					if (!(++kbatt &kbatt_msk)) {
						if (mode &EVF_NODAC) {
							if (kbhit()) {
								if ((ret=getch()) != 0) return CE_KEY_PRESSED|ret;

								return (CE_KEY_PRESSED|CE_KEY_SPECIAL) |getch();
							}
						} else {
							if ((ret=verify_dac()) != 0) return ret;
						}
					}

					if (rout_hdr) {
						dbell=plx_regs->l2pdbell;
//if (dbell) { printf("dbell %08lX\n", dbell); if (WaitKey(0)) return 26; }
						if (dbell &DBELL_F1_ERR) {
							printf("DBELL: F1 error at =%02X(%08lX)\n", (u_int)(dbell >>24), dbell);
							plx_regs->l2pdbell=dbell &DBELL_F1_ERR;
							if (WaitKey(0)) return 27;
						}

	//---------------
	// Event mode
	//---------------
						if (config.daq_mode &EVF_DBELL) {
		//--------------
		// wait doorbell (interrupt)
		//--------------
							if (config.daq_mode &EVF_DBELLPF) {
								if (!(dbell &DBELL_EVENTPF)) { pause++; continue; }

								for (i=0; !(dbell &(0x10000L<<i)); i++);

								plx_regs->l2pdbell=0x10000L<<i;
							} else {
								if (!(dbell &DBELL_EVENT)) { pause++; continue; }

								for (i=0; !(dbell &(0x100L<<i)); i++);

								plx_regs->l2pdbell=0x100L<<i;
							}
						} else {
		//--------------
		// poll status register
		//--------------
							if ((ret=direct_read16(&bus1100->l_coupler.sr, &gtmp)) != 0) {
								printf("prot_err 1 %03X\n", ret);
								return CE_PROMPT;
							}

//printf("sc.sr %04X\n", gtmp);
							if (!(gtmp &SC_DATA_AV)) {
								if ((ret=inp_overrun()) > 0) return ret;

								pause++; continue;
							}
						}

						sconf=&config.sc_db;
						sregs=sconf->sc_unit;
						gigal_regs->d0_bc=0;
						plx_regs->dma[0].dmadpr=dmadesc_phy[0] |0x9;
						plx_regs->dmacsr[0] =0x3;						// start
if (protocol_error(glp, "DD.5")) return CE_PROMPT;
						gigal_regs->t_ad=(u_int)&sregs->ro_data;
					} else {
	//---------------
	// Input poll mode, kein Event Header
	//---------------
						sconf=i_sconf;
						sregs=sconf->sc_unit;
						ibcregs=sconf->inp_bc;
						if ((req=ibcregs->ro_data) == 0) {
							if (protocol_error(glp, "4")) return CE_PROMPT;

							if ((ret=inp_overrun()) > 0) return ret;

							continue;
						}

						i=0;
						while (!(req &(1 << i))) i++;
						req &=~(1 <<i);
						iconf=sconf->inp_db; j=0;
						while (i != iconf->slv_addr) {
							iconf++; j++;
							if (j >= sconf->slv_cnt) return CE_FATAL;
						}

						gigal_regs->d0_bc=0;
						plx_regs->dma[0].dmadpr=dmadesc_phy[0] |0x9;
						plx_regs->dmacsr[0] =0x3;						// start

if (protocol_error(glp, "DD.6")) return CE_PROMPT;
						gigal_regs->t_ad=(u_int)&((F1TDC_RG*)iconf->inp_regs)->ro_data;
					}

//-------------------
// warte auf DMA ende
//-------------------
					while (1) {		// um mehrere Input Cards zu lesen
						j=0;
						while (1) {		// SR_PROT_ERROR|SR_S_XOFF|SR_DMD_EOT
							_delay_(100);
							if (plx_regs->intcsr &PLX_ILOC_ACT) break;

							if (++j >= 1000) {
								printf("EOT(DMA0) time out CSR=%02X\n", plx_regs->dmacsr[0]);
								printf("sr=%08lX, prot_err=%03X, bal=%u\n",
										 gigal_regs->sr, (u_int)gigal_regs->prot_err,
										 (u_int)gigal_regs->balance);
//								BUS_ERROR;
								printf("len: %03X\n", (u_int)gigal_regs->d0_bc);
								return CE_PROMPT;
							}
						}

						ltmp=gigal_regs->sr;
						gigal_regs->sr=ltmp;
						if (ltmp &SR_PROT_ERROR) {	// wenn keine Daten vorhanden
							if ((lerr=(u_int)gigal_regs->prot_err) != (0x200|PE_BERR)) {
								printf("prot_err 2 %03X\n", lerr);
								return CE_PROMPT;
							}

							if (send_dmd_eot()) return CEN_PROMPT;

							break;	// keine Daten
						}

						if (!(ltmp &SR_DMD_EOT)) {
							protocol_error(glp, "2");
							printf("len: %03X\n", (u_int)gigal_regs->d0_bc);
							return CE_PROMPT;
						}

						if (!req) break;

						// lese naechste Input Karte
						i=0;
						while (!(req &(1 << i))) i++;
						req &=~(1 <<i);
						iconf=sconf->inp_db; j=0;
						while (i != iconf->slv_addr) {
							iconf++; j++;
							if (j >= sconf->slv_cnt) return CE_FATAL;
						}

						j=(u_int)gigal_regs->d0_bc;			// bisher gelesen
						gigal_regs->t_da =dmapages*(u_long)SZ_PAGE -j;
						gigal_regs->t_ad=(u_int)&((F1TDC_RG*)iconf->inp_regs)->ro_data;
					}

if (protocol_error(glp, "DD.7")) return CE_PROMPT;
					if (!rout_hdr) {
						plx_regs->dma[0].dmamode |=0x4000;	// EOT enable
						gigal_regs->sr =SR_SEND_EOT;
					}

					j=0;
					while (1) {
						_delay_(100);
						if ((plx_regs->dmacsr[0] &0x10)) break;

						if (++j >= 1000) {
							printf("DMA0 not terminated CSR=%02X\n", plx_regs->dmacsr[0]);
							protocol_error(glp, "1");
							printf("len: %03X\n", (u_int)gigal_regs->d0_bc);
							return CE_PROMPT;
						}
					}

//----------------------
// Event Daten im Kasten
//----------------------
					rout_len =(u_int)gigal_regs->d0_bc;
					if (!rout_len) {
//printf("keine daten\n");
						continue;
					}

					if (config.daq_mode &EVF_HANDSH) {
						rout_trg_time=sregs->trg_data;
						rout_ev_inf=sregs->ev_info;		// Freigabe
						if ((u_short)rout_ev_inf != rout_len) {
							printf("HS0 illegal block length: %08lX/%04X\n",
									 rout_ev_inf, rout_len);
							return CE_FATAL;
						}
					}

					if (rout_hdr && (trig >= RO_RAW)) {
	//----------------------
	// Plausibilitaetskontrolle
	//----------------------
//printf("inf/d0_bc: %08lX/%04X\n", rout_ev_inf, rout_len);
						i=0; j=0; h=0;
						while (1) {
							j +=(u_int)dmabuf[0][i]; h++;
							if (   (tstlen && (dmabuf[0][i] != evlen))
								 || (dmabuf[0][i+2] != evcounter)) {
								if (!evlen) evlen=dmabuf[0][i];
								else {
									if ((long)glp != -1) printf("\r%10ld\n", glp);
									printf("error, blk=%u, len=%04X, h=%08lX/%08lX/%08lX(%08lX/%08lX), i=%04X/%04X\n",
											 h-1, rout_len, dmabuf[0][i], dmabuf[0][i+1], dmabuf[0][i+2],
											 evlen, evcounter, i, 4*i);
									if (pciconf.errlog) return CE_FATAL;

									evlen=dmabuf[0][i];
									evcounter=dmabuf[0][i+2];
								}
							}

							evcounter++;
							if (j == rout_len) break;

							if (!(u_int)dmabuf[0][i] || (j > rout_len)) {
								if ((long)glp != -1) printf("\r%10ld: ", glp);
								printf("length err, blk=%u, len=%04X/%04X, h=%08lX, i=%04X/%04X\n",
										 h-1, rout_len, j, dmabuf[0][i], i, 4*i);
								return CE_FATAL;
							}

							i=j/4;
						}

						if (h > mev) {
							mev=h;
							if (tstlen) {
								if ((long)glp != -1) printf("\r%10ld: ", glp);
								printf("multiple event blocks =%u, len =%04X\n",
										 h, rout_len);
							}
						}

						rlen=rout_len;
						rout_len=(u_int)dmabuf[0][0];
						rlen -=rout_len;
						robf=dmabuf[0];
					}

					break;
				}
			}
#else		// if_5
// ======================================================================= --
//										mit PCI LVD Master                           --
// ======================================================================= --
			setup_dma(-1, 0, 0, dmapages*(u_long)SZ_PAGE, 0, 0, DMA_FASTEOT|DMA_L2PCI);
			lvd_regs->sr =lvd_regs->sr;
			while (1) {
				if (mode &EVF_NODAC) {
					if (kbhit()) {
						if ((ret=getch()) != 0) return CE_KEY_PRESSED|ret;

						return (CE_KEY_PRESSED|CE_KEY_SPECIAL) |getch();
					}
				} else {
					if ((ret=verify_dac()) != 0) return ret;
				}

//---------------
// Input poll mode, kein Event Header
//---------------
				sconf=i_sconf;
				if (sconf->inp_bc->ro_data == 0) {
					if (lvd_regs->sr &SR_LB_TOUT) return CE_PROMPT;

					if (!rout_hdr && ((ret=inp_overrun()) > 0)) return ret;

					continue;
				}

//printf("%04X\n", sconf->inp_bc->ro_data);
//gtmp=((FQDC_RG*)inp_unit)->mean_noise[2];
//_delay_(10000);
				plx_regs->dma[0].dmadpr=dmadesc_phy[0] |0x9;
				plx_regs->dmacsr[0]=0x3;
				lvd_regs->lvd_scan =LVD_SCAN_BLOCK |(OFFSET(LVD_BUS, in_card_bc.ro_data)/2);
				lvd_regs->lvd_blk_max=dmapages*(u_long)SZ_PAGE;
				lvd_regs->lvd_block=OFFSET(LVD_BUS, in_card.ro_data)/2;

				j=0;
				while (!(plx_regs->dmacsr[0] &0x10)) {
					_delay_(1000);
					if (++j >= 100) {
						printf("DMA0 time out CSR=%02X\n", plx_regs->dmacsr[0]);
						return CE_PROMPT;
					}
				}

//printf("%04X\n", lvd_regs->sr);
//if (WaitKey('2')) return 20;
				if (lvd_regs->sr &SR_BLKTX) {
					printf("block transfer not terminated\n");
					return CE_PROMPT;
				}

				for (rout_len=0, i=0; i < 16; i++) {
					rout_len +=(u_int)lvd_regs->lvd_scan_cnt[i];
					if (rout_len > 0x8000) {
						printf("%u: %04X\n", i, rout_len);
						return 21;
					}
				}

//printf("len: %03X\n", rout_len);
				break;
			}
#endif		// if_5
			break;
		}

		lx=rout_len /4;
		h=(rout_hdr) ? HDR_LEN : 0;
		if (lx <= h) {
			if (lx && (config.daq_mode &EVF_IGNNUL)) continue;

			return 0;
		}

		if (config.daq_mode &EVF_SORT) {
//
//--- sortieren, raw: absolut, sonst signed ---------------------------------
//
			if (rout_hdr == MCR_DAQTRG) {
				do {
					u_long	ltmp;

					j=0;
					for (i=h; i+1 < lx; i++) {
						if (EV_CHAN(robf[i]) < EV_CHAN(robf[i+1])) continue;

						if (   (EV_CHAN(robf[i]) == EV_CHAN(robf[i+1]))
							 && ((int)EV_FTIME(robf[i]) >= (int)EV_FTIME(robf[i+1]))) continue;

						ltmp=robf[i];
						robf[i]=robf[i+1];
						robf[i+1]=ltmp;
						j=1;
					}

				} while (j);
			} else {
				do {
					u_long	ltmp;

					j=0;
					for (i=h; i+1 < lx; i++) {
						if (EV_CHAN(robf[i]) < EV_CHAN(robf[i+1])) continue;

						if (EV_CHAN(robf[i]) == EV_CHAN(robf[i+1])) {
							u_long tm0=((u_long)EV_MTIME(robf[i]) <<16) |EV_FTIME(robf[i]);
							u_long tm1=((u_long)EV_MTIME(robf[i+1]) <<16) |EV_FTIME(robf[i+1]);

							if (tm0 <= tm1) continue;
						}

						ltmp=robf[i];
						robf[i]=robf[i+1];
						robf[i+1]=ltmp;
						j=1;
					}
				} while (j);
			}
		}

		if (trig <= RO_RAW) return 0;
//
// ==========================================================================
//										relative Zeit berechnen
// ==========================================================================
//
//--- beim MCR_DAQTRG Mode wird die relative Zeiten zum Trigger
//		im Input Module bestimmt
//
		if (rout_hdr == MCR_DAQTRG) {

			tm_cnt=0;
			for (i=HDR_LEN; i < lx; i++) {
				u_long	x=robf[i];
				u_int		typ=inp_type[(u_int)(x >>28)];

				if ((typ != F1TDC) && (typ != GPXTDC) && (typ != GPXTDCTST)) continue;

				(*tm_hit)[tm_cnt].tm_cha=EV_CHAN(x);
				(*tm_hit)[tm_cnt].tm_diff=(int)EV_FTIME(x);
				(*tm_hit)[tm_cnt].tm_width=0;
				tm_cnt++;
			}
		} else {
			u_int		tm_ix;
			u_int		cha;
//			u_int		typ;
			u_int		tm_l,tm_h;
			u_int		tm_ll,tm_hh;
			u_long	x;
			long		diff;
//
//--- Berechnung der Zeiten und Laengen (beim letra Mode)
//
//printf("lx %d\n", lx);
			if (trig == RO_TRG) {
				if (!rout_hdr) return 0;

				tm_ix=-1;
				tm_h=(u_int)(rout_trg_time >>16) &C_MTIME;
				tm_l=(u_int)rout_trg_time;
			} else {
				for (i=h; (i < lx) && (EV_CHAN(robf[i]) != trig); i++);
				if (i >= lx) continue;			// start new readout

				tm_ix=i;		// der erste Wert zu diesem Kanal im Buffer
				tm_h=EV_MTIME(robf[i]);
				tm_l=EV_FTIME(robf[i]);
			}

tm_h &=0xF;
//printf("%03X %3d %04X %04X %08lX\n", lx, tm_ix, tm_h, tm_l, rout_trg_time);
			if (config.f1_letra &0x04) tm_l &=~1;

			for (i=h, tm_cnt=0; i < lx; i++) {
				if (i == tm_ix) continue;

				x=robf[i];
//				typ=inp_type[(u_int)(x >>28)];
				cha=EV_CHAN(x);
//printf("%08lX %02X %03X\n", x, typ, cha);
#if 0
				if ((typ == GPXTDC) || (typ == GPXTDCTST)) {
//printf("%06lX %X %X %05lX", rout_trg_time, (u_int)(x>>28), (u_int)(x>>18) &0xF, x &0x1FFFFL);
					if ((x &0x1FFFFL) >= 0x18000L) {
						x= ((x+gpx_range) &0x1FFFFL)
						  |((x-0x40000L) &0x3C0000L)
						  |(x &0xFFC00000L);
printf(" => %X %05lX", (u_int)(x>>18) &0xF, x &0x1FFFFL);
					}
					x= ((3*(x &0x1FFFFL)+2)/4)
					  |((x>>2) &0xF0000L)
					  |(x &0xFFC00000L);
//printf(" => %X %04X\n", (u_int)(x>>16) &0xF, (u_int)x);
				}
#endif
				tm_hh=EV_MTIME(x);
tm_hh &=0xF;
//printf("%04X %04X\n", tm_hh, cha);
				if (   (tm_hh != tm_h)
					 && (((tm_hh+1) &C_MTIME) != tm_h)
					 && (tm_hh != ((tm_h+1) &C_MTIME))) continue;

				tm_ll=EV_FTIME(x);
				diff=(u_long)tm_l-(u_long)tm_ll;
//printf("%02X: %04X.%04X %04X.%04X %4ld\n", cha, tm_h, tm_l, tm_hh, tm_ll, diff);
				if (tm_hh != tm_h) {
					if (((tm_hh+1) &C_MTIME) == tm_h) diff +=f1_range;	// diff=(tm_l+f1_range)-tm_ll;
					else diff -=f1_range;										// diff=tm_l-(tm_ll+f1_range);
				}

				if ((diff < tm_win_le) || (diff > tm_win_he)) continue;

//printf("%03X: %4ld\n", cha, diff);
				if (tm_cnt >= 64*2*C_UNIT) {
					printf("buffer overrun\n"); break;
				}

				(*tm_hit)[tm_cnt].tm_diff=diff;
				(*tm_hit)[tm_cnt].tm_cha=cha;
				(*tm_hit)[tm_cnt].tm_width=0;
				tm_cnt++;
			}
		}

//printf("tm_cnt    %d\n", tm_cnt);

		if (tm_cnt == 0) continue;
//
//--- sortieren, fallend!
//
		if (!(config.daq_mode &EVF_SORT))		// wurde schon sortiert
			do {
				u_int m_cha;
				long	m_tm;

				j=0;
				for (i=0; i+1 < tm_cnt; i++) {
					if ((*tm_hit)[i].tm_cha < (*tm_hit)[i+1].tm_cha) continue;

					if (   ((*tm_hit)[i].tm_cha == (*tm_hit)[i+1].tm_cha)
						 && ((*tm_hit)[i].tm_diff >= (*tm_hit)[i+1].tm_diff)) continue;

					m_cha=(*tm_hit)[i].tm_cha; m_tm=(*tm_hit)[i].tm_diff;
					(*tm_hit)[i].tm_cha=(*tm_hit)[i+1].tm_cha; (*tm_hit)[i].tm_diff=(*tm_hit)[i+1].tm_diff;
					(*tm_hit)[i+1].tm_cha=m_cha; (*tm_hit)[i+1].tm_diff=m_tm;
					j=1;
				}
			} while (j);

#if 0
		if (!(config.f1_letra &0x04)) return 0;

		for (i=0, j=0; i < tm_cnt; i++, j++) {
//printf("%d %d %X %lX\n", i, j, (*tm_hit)[i].tm_cha, (*tm_hit)[i].tm_diff);
			if (   (i+1 >= tm_cnt)
				 || ((*tm_hit)[i].tm_cha != (*tm_hit)[i+1].tm_cha)
				 || !(((*tm_hit)[i].tm_diff ^(*tm_hit)[i+1].tm_diff) &1)) {
				(*tm_hit)[j].tm_cha=(*tm_hit)[i].tm_cha;
				(*tm_hit)[j].tm_diff=(*tm_hit)[i].tm_diff &~1;
				(*tm_hit)[j].tm_width=0; continue;
			}

			(*tm_hit)[j].tm_cha=(*tm_hit)[i].tm_cha;
			(*tm_hit)[j].tm_diff=(*tm_hit)[i].tm_diff &~1;
			(*tm_hit)[j].tm_width=(u_int)((*tm_hit)[i].tm_diff -(*tm_hit)[i+1].tm_diff); i++;
		}

		tm_cnt=j;
//printf("tm_cnt le %d\n", tm_cnt);
#endif
		return 0;
	}
}
//--- end ---------------------- read_event ---------------------------------
//
//------------------------------ inp_mode -----------------------------------
//
int inp_mode(
	u_char	mode)
{
	u_int		i;
	SC_DB		*sconf;
	F1TDC_BC	*sibc;
	u_int		inp;
	INP_DB	*iconf;
	u_int		icr;
	u_int		tcr;
#ifndef PBUS
	COUPLER	*sregs;
	long		ltmp;
	u_int		scr;		// system controller control register
#endif

//printf("inp_mode(%02X)\n", mode);
	if (dmapages < 2) {
		printf("at least 2 DMA pages requested\n");
		return CE_NO_DMA_BUF;
	}

#ifdef SC_M
	if (send_dmd_eot()) return CEN_PROMPT;
#else
	lvd_regs->sr =lvd_regs->sr;
	if (!(plx_regs->dmacsr[0] &0x10)) {
		plx_regs->dmacsr[0]=0x00;
		lvd_regs->sr =SR_DMA0_ABORT; i=0;
		while (plx_regs->dmacsr[0] != 0x10) {
			if (++i < 4) continue;

			printf("DMA is hanging up\n");
			return CEN_PROMPT;
		}
	}
#endif

	if (mode &IM_STRIG) {
#ifdef PBUS
		return CE_PARAM_ERR;
#else
		config.daq_mode=(config.daq_mode &~MCR_DAQRAW) |(EVF_HANDSH|MCR_DAQTRG);
		config.inp_trigger=-1;
#endif
	} else {
		if (mode &(IM_QUIET|IM_VERTEX)) {
			if ((mode &(IM_QUIET|IM_VERTEX)) == IM_VERTEX) config.daq_mode=0;
		} else
			while (1) {
				printf("%03X=>VTEX dummy, %02X=>ign nul\n", EVF_VTDUM, EVF_IGNNUL);
				printf("%02X=>sort, %02X=>tpulse, ", EVF_SORT, EVF_ON_TP);
#ifdef SC_M
				printf("%02X=>ign ovr, %02X=>no BCbuf, %02X=>directTx, %02X=>noBC/noCNT\n",
						 EVF_IGNOVR, EVF_NOBCBUF, EVF_DDATA, EVF_NOBCBUF|EVF_DDATA);
				printf("%02X=>pf dbell, %02X=>dbell, ",
						 EVF_DBELLPF, EVF_DBELL);
#endif
#ifndef PBUS
				printf("%02X=>handsh, 02=>scan/raw, 01=>MainTrig\n", EVF_HANDSH);
#endif
				printf("DAQ Mode                   %03X ", config.daq_mode);
				config.daq_mode=(u_int)Read_Hexa(config.daq_mode, 0xFFF);
				if (errno) return CEN_MENU;

				i=config.daq_mode;
#ifdef PBUS
				config.daq_mode &=(EVF_VTDUM|EVF_IGNNUL|EVF_SORT|EVF_ON_TP);
#endif
				if (config.daq_mode != i) {
					printf("illegal value\n"); continue;
				}

//				if (config.daq_mode &EVF_NOBCBUF) config.daq_mode |=EVF_DDATA;
#ifdef SC_M
				if (config.daq_mode &EVF_DBELLPF) config.daq_mode |=EVF_DBELL;
#endif
				if ((mode &IM_NODAQTRG) && ((config.daq_mode &MCR_DAQ) == MCR_DAQTRG)) {
					printf("window mode not allowed\n");
					continue;
				}

				break;
			}

		if (!(config.daq_mode &MCR_DAQ)) {
			config.daq_mode &=EVF_SORT;
			i_sconf=&config.sc_db;
		}

#ifdef SC_M
		if (config.daq_mode &(EVF_NOBCBUF|EVF_DDATA)) {
			if (mode &IM_NOBCBUF) config.daq_mode |=EVF_NOBCBUF;

			printf("direct data block       %06lX ", config.dd_block);
			config.dd_block=Read_Hexa(config.dd_block, -1);
			if (errno) return CEN_MENU;

			if (config.dd_block >= ((dmapages-1)*(u_long)SZ_PAGE-0x800))
				printf("attention max is %06lX\n", (dmapages-1)*(u_long)SZ_PAGE);
		}
#endif

		if (!(mode &IM_QUIET) && (config.daq_mode &MCR_DAQRAW)) {
			printf("delay before ReadOut (25ns) %02X ", config.rout_delay);
			config.rout_delay=(u_char)Read_Hexa(config.rout_delay, 0xFF);
			if (errno) return CEN_MENU;
		}
	}

	rout_hdr=config.daq_mode &MCR_DAQ;
#ifdef SC_M
	if ((config.daq_mode &(EVF_NOBCBUF|EVF_DDATA)) ? (dmapages < 5)
												  : (dmapages < 2)) return CE_NO_DMA_BUF;
#else
	if (dmapages < 2) return CE_NO_DMA_BUF;
#endif

	while (mode &IM_TREF) {
#ifdef PBUS
		return CE_PARAM_ERR;
#else
		if (rout_hdr == MCR_DAQTRG) config.inp_trigger=-1;
		else {
			printf("Trigger Channel (-1,0..) 0x%03X ", config.inp_trigger &0xFFF);
			config.inp_trigger=(int)Read_Hexa(config.inp_trigger, -1);
			if (errno) return CEN_MENU;
		}

		if ((config.inp_trigger == -1) && !rout_hdr) {
			printf("no trigger awailable\n"); continue;
		}

		if ((config.inp_trigger < -1) || (config.inp_trigger >= 64*C_UNIT)) {
			printf("select trigger channel: -1 => system controller, 0..%u => input card\n", (64*C_UNIT)-1);
			continue;
		}

		break;
#endif
	}

	if (!(mode &IM_QUIET) && ((rout_hdr == MCR_DAQTRG) || (mode &IM_TREF))) {
		printf("trigger latency (ns)   %7.1f ", config.ef_trg_lat);
		config.ef_trg_lat=Read_Float(config.ef_trg_lat);
		if (errno) return CEN_MENU;

		printf("trigger window  (ns)    %6.1f ", config.ef_trg_win);
		config.ef_trg_win=Read_Float(config.ef_trg_win);
		if (errno) return CEN_MENU;
	}
//
// ----------------------->>> setup all devices <<<--------------------------
//
//--- DAQ reset, alle coupler cr auf 0
//
#ifdef SC_M
	cm_reg->cr &=(SCR_LED1|SCR_LED0);
	bus1100->l_coupler.cr=0;
	bus1100->l_coupler.sr=0xFFFF;
#endif
//
//--- daten variable
//
	if (!(mode &(IM_QUIET|IM_VERTEX))) {
		printf("F1  range: 0x%05lX(%1.1fns)\n", f1_range, f1_range*RESOLUTIONNS);
		printf("GPX range: 0x%05lX(%1.1fns)\n", gpx_range, gpx_range*G_RESOLUTIONNS);
	}

	rout_trg_time=0;	// nur im handshake mode
	robf=dmabuf[0];
	glp=-1L;
#ifndef PBUS
	rlen=0;
	kbatt_msk=0x0F;
	mev=1; tstlen=0;
	evlen=0;
	evcounter=0;
#endif
	for (i=0; i < 64*2*C_UNIT; (*tm_hit)[i++].tm_width=0);
	tm_win_he=(long)(config.ef_trg_lat*1000.0/RESOLUTION +0.5);
	tm_win_le=(long)((config.ef_trg_lat-config.ef_trg_win)*1000.0/RESOLUTION +0.5);

	tlat=(long)(config.ef_trg_lat*1000.0/RESOLUTION +0.5);
	twin=(long)(config.ef_trg_win*1000.0/RESOLUTION +0.5);
	if (rout_hdr == MCR_DAQTRG) {
		icr=(config.f1_letra &0x04) ? ICR_LETRA : ICR_TMATCHING;
	} else {
		if (rout_hdr) icr=ICR_TLIM;
		else icr=ICR_RAW;
	}

#ifndef PBUS
	scr=rout_hdr;
	if ((scr == (MCR_DAQRAW|MCR_DAQTRG)) && (config.daq_mode &EVF_ON_TP))
		scr |=MCR_DAQ_TEST;

	if (config.daq_mode &EVF_HANDSH) scr|=MCR_DAQ_HANDSH;
	if (config.f1_sync != 2) scr|=MCR_NO_F1STRT;

	if (rout_hdr == MCR_DAQTRG) {
		ltmp=tlat;
		if (tlat <= -0x4000) tlat=-0x3FFF;
		if (tlat >= 0x4000) tlat=0x3FFF;
		if (tlat != ltmp)
			printf("latency time corrected: %6.1fns\n", (tlat*RESOLUTION)/1000.);

		ltmp=twin;
		if (tlat-twin <= -0x4000) {
			twin=tlat+0x4000;
			printf("window time corrected:  %6.1fns\n", (twin*RESOLUTION)/1000.);
		}
	}
#endif

	sconf=&config.sc_db;
	sibc=sconf->inp_bc;
#ifndef PBUS
	sregs=sconf->sc_unit;
#endif

	if (rout_hdr == MCR_DAQTRG) {
		sibc->trg_lat =(u_short)tlat;
		sibc->trg_win =(u_short)twin;
		sibc->f1_range=(u_short)f1_range;
	}

	if (m_mode) sibc->cr=0x6;
	else sibc->cr=icr;

#ifdef PBUS
	sibc->ctrl=SYNCRES;
#else
	if (rout_hdr) {
		sregs->ro_delay=config.rout_delay;
		sregs->event_nr=eventcount;
	}

	sregs->sr=sregs->sr;
	sregs->cr=scr;
	sregs->ctrl=SYNCRES;
	if (config.f1_sync < 2) sregs->ctrl=F1START;	// start all F1 and counter for synchron trigger signal
#endif
	for (inp=0, iconf=sconf->inp_db; inp < sconf->slv_cnt; inp++, iconf++) {
//printf("%u\n", inp_offl[inp]); WaitKey(0);
		switch (iconf->type) {
	case F1TDC:
			if (inp_offl[inp]) {
				((F1TDC_RG*)iconf->inp_regs)->cr=0;
				break;
			}

			if ((mode &IM_VERTEX) && (iconf != inp_conf))
				((F1TDC_RG*)iconf->inp_regs)->cr=0;

//printf("%04X\n", ((F1TDC_RG*)iconf->inp_regs)->cr);
			break;

	case VERTEX:
			{
			VTEX_RG	*vtex_rg=iconf->inp_regs;

			vtex_rg->ADR_CTRL  =0x0000;
			if (inp_offl[inp]) break;

			if ((mode &IM_VERTEX) && (iconf != inp_conf)) break;

			if (LoadSequencer(vtex_rg, LV, "C:\\LVDATA.TXT")) return CEN_PROMPT;
			if (LoadSequencer(vtex_rg, HV, "C:\\HVDATA.TXT")) return CEN_PROMPT;

			vtex_rg->ADR_CTRL =(config.daq_mode &EVF_VTDUM) ? 0x00E4 : 0x0064;
			break;
			}

	case FQDC:
	case FQDCTST:
			if (inp_offl[inp]) {
				((FQDCT_RG*)iconf->inp_regs)->cr=0;
				break;
			}
			break;

	case GPXTDC:
	case GPXTDCTST:
			{
			GPX_RG	*gregs=iconf->inp_regs;

			gregs->cr=0;
			if (   inp_offl[inp]
				 || ((mode &IM_VERTEX) && (iconf != inp_conf))) break;

			gregs->f1_range=(u_short)f1_range;
			if (rout_hdr == MCR_DAQTRG) {
				gregs->trg_lat =(u_short)tlat;
				gregs->trg_win =(u_short)twin;
			}

			if (!(tcr=rout_hdr)) tcr=MCR_DAQRAW;
			if (config.f1_letra &0x04) tcr |=0x04;
			if (m_mode) tcr |=0x8;

			gregs->cr=tcr;
			break;
			}

	case SQDCTST:
	case SQDC:
//printf("%u %04X\n", inp, ((SQDCT_RG*)iconf->inp_regs)->cr); WaitKey(0);
			if (inp_offl[inp]) {
				((SQDCT_RG*)iconf->inp_regs)->cr=0;
				break;
			}

			gtmp=((SQDCT_RG*)iconf->inp_regs)->cr;
			((SQDCT_RG*)iconf->inp_regs)->cr=0;
			((SQDCT_RG*)iconf->inp_regs)->cr=gtmp;
			break;

		}

	}

	dma_buffer(1);

#ifdef SC_M
	{
	long		ltmp;

	gigal_regs->d0_bc_adr =0;	// sonst PLX kein EOT
	gigal_regs->d0_bc_len =0;
	plx_regs->l2pdbell=0xFFFFFFFFL;
	ltmp=cm_reg->cr &(SCR_LED1|SCR_LED0);
	if (config.daq_mode &EVF_DBELL)  ltmp |=SCR_EV_DBELL;
	cm_reg->cr=ltmp;

	cm_reg->dd_blocksz=config.dd_block-1;
	cm_reg->dd_counter=0;

	gigal_regs->cr=SET_CR_IM(SR_PROT_ERROR|SR_S_XOFF|SR_DMD_EOT|CR_READY);
	plx_regs->intcsr =PLX_ILOC_ENA|PLX_IDOOR_ENA;		// enable PCI Interrupt
	if (config.daq_mode &(EVF_NOBCBUF|EVF_DDATA)) {
		gigal_regs->cr=SET_CR_IM(SR_PROT_ERROR|SR_S_XOFF|CR_READY);
		setup_dma(-1, 0, 0, (dmapages-1)*(u_long)SZ_PAGE, 0, 0,
					 (config.daq_mode &EVF_NOBCBUF) ? DMA_CIRCULAR|DMA_NOEOT|DMA_L2PCI
															  : DMA_CIRCULAR|DMA_L2PCI);
		bcbix=0;
		ddat=0; dd_ix=0; dd_next=0;
		dd_max=(u_long)(dmapages-1)*SZ_PAGE_L;
		if (config.daq_mode &EVF_NOBCBUF) {
			gigal_regs->d0_bc_adr =0;
			gigal_regs->d0_bc_len =bcblen =0;
		} else {
			gigal_regs->d0_bc_adr =dmabuf_phy[dmapages-1] |0x1;
			gigal_regs->d0_bc_len =bcblen =C_DDMA;
		}

		gigal_regs->sr =gigal_regs->sr;
		gigal_regs->d0_bc=0;
		plx_regs->dma[0].dmadpr=dmadesc_phy[0] |0x9;
		plx_regs->dmacsr[0]    =0x3;					// start DMA
		cm_reg->cr=ltmp |SCR_DDTX;		// erst hier starten
		bcbuf=&DBUF_L(dd_max);
		robf=robuf;
	}

	cm_reg->timer=0;
	}
#endif

	plx_regs->l2pdbell=plx_regs->l2pdbell;
	return 0;
}
//
//------------------------------ stop_ddma ----------------------------------
//
int stop_ddma(void)
{
#ifdef SC_M
	u_long	ltmp;

	if ((ltmp=cm_reg->cr) &SCR_DDTX) {
		cm_reg->cr =ltmp &~SCR_DDTX;

		if (send_dmd_eot()) return CEN_PROMPT;
	}

#endif
	return 0;
}
//
#ifdef SC_M
//--------------------------- ev_counter ------------------------------------
//
#if 0
static int ev_counter(void)
{
	int		ret;
	u_long	ltmp;
	u_int		i,j;
	u_int		tmp;
	u_int		perr;
	u_long	l,lx;
	long		ldiff;
	u_int		chk=0;
	u_long	dbell;
	u_int		ml=0;
	char		sft;

	if (!pciconf.errlog) printf("attention: error logging is off\n");

	if ((ret=inp_mode(0, -1)) != 0) return ret;

	tstlen=1;
	sft=(config.daq_mode &(EVF_NOBCBUF|EVF_DDATA)) == (EVF_NOBCBUF|EVF_DDATA);
	glp=0; timemark();
	if (config.daq_mode &(EVF_NOBCBUF|EVF_DDATA)) {	// demand DMA
		printf("direct data block doorbell interrupt used\n");
		l=0; lx=0; i=0; ret=0;
		while (!ret) {
			ltmp=plx_regs->intcsr;
			if ((++chk > 1000) || !(ltmp &(PLX_ILOC_ACT|PLX_IDOOR_ACT))) {
				chk=0;
				if (kbhit()) {
					if (!getch()) getch();

					break;
				}

				continue;
			}

//printf("intcsr =%08lX\n", ltmp);
			if (ltmp &PLX_ILOC_ACT) {
				ltmp=gigal_regs->sr;
				gigal_regs->sr=ltmp;
//printf("sr     =%08lX\n", ltmp);
				if (ltmp &SR_S_XOFF) {
					gigal_regs->d0_bc_adr =dmabuf_phy[dmapages-1] |0x1;
					gigal_regs->d0_bc_len =bcblen =SZ_PAGE;
					continue;
				}
			}

			dbell=plx_regs->l2pdbell;
         plx_regs->l2pdbell=dbell; 
//printf("dbell  =%08lX\n", dbell);
			if (!(dbell &1)) continue;

			if (sft) l +=config.dd_block;
			else {
				tmp=0;
				while (1) {
					l=cm_reg->dd_counter;
					if ((perr=(u_int)gigal_regs->prot_err) == 0) break;

					ltmp=gigal_regs->sr; gigal_regs->sr=ltmp;
//printf("sr     =%08lX\n", ltmp);
					if (ltmp &SR_S_XOFF) {
						gigal_regs->d0_bc_adr =dmabuf_phy[dmapages-1] |0x1;
						gigal_regs->d0_bc_len =bcblen =SZ_PAGE;
						continue;
					}

					_delay_(5000);
					if ((++tmp < 10) && ((perr &~0x100) == PE_DLOCK)) continue;

					printf("sr %08lX, p_err %03X, intcsr %08lX, cnt=%u\n",
							 ltmp, perr, plx_regs->intcsr, tmp);
					return CE_PROMPT;
				}

				if (tmp > ml) { ml=tmp; printf("dlock %u\n", tmp); }
			}

			while (1) {
				u_int k;

				ldiff=l-lx;
				if (ldiff < 3*4) break;

				tmp=(u_int)dmabuf[0][i];
				if ((j=i+1) >= dd_max) j=0;
				if ((k=j+1) >= dd_max) k=0;

				if ((tmp != (u_int)evlen) || (dmabuf[0][k] != evcounter)) {
					if (!pciconf.errlog) {
						evlen=dmabuf[0][i];
						evcounter=dmabuf[0][k];
					} else {
						if (!evlen) evlen=dmabuf[0][i];
						else {
							printf("\r%10lu\n", glp);
							printf("error, h=%08lX/%08lX/%08lX (%08lX/%08lX), i=%04X/%04X\n",
									 dmabuf[0][i], dmabuf[0][j], dmabuf[0][k],
									 evlen, evcounter, i, 4*i);
							ret=CE_PROMPT; break;
						}
					}
				}

				evcounter++;
				lx +=tmp;
				i +=(tmp/4);
				if (i >= dd_max) i -=dd_max;
			}

			glp++;
			if (!(glp &0x0FF)) printf("\r%10lu ", glp);
		}

		stop_ddma();
		printf("\r%10lu lx/l/is=%08lX/%08lX/%08lX\n",
				 glp, lx, l, cm_reg->dd_counter);
		ret=CE_PROMPT;
	} else {
		while ((ret=read_event(EVF_NODAC, RO_RAW)) == 0) {
			glp++;
			if (!(glp &0x03FF)) printf("\r%10lu ", glp);

			if (!(config.daq_mode &EVF_IGNOVR)) {
				tmp=sc_regs->sr;
				if ((perr=(u_int)gigal_regs->prot_err) == 0) {
					if (tmp &SC_TRG_LOST) {
						printf("\r%10lu trigger overrun", glp); ret=CE_PROMPT; break;
					}
				} else {
					gigal_regs->sr=SR_PROT_ERROR|SR_PROT_END;
					if (perr != (0x100|PE_DLOCK)) {
						printf("\r%10lu prot_err %03X\n", glp, perr); break;
					}
				}
			}
		}

		if (config.daq_mode &EVF_IGNOVR) {
			tmp=sc_regs->sr;
			if ((perr=(u_int)gigal_regs->prot_err) == 0) {
				if (tmp &SC_TRG_LOST) {
					printf("\r%10lu trigger overrun", glp);
				}
			}
		}
	}

	ltmp=ZeitBin();
	Writeln();
	i=pciconf.errlog; pciconf.errlog=1;
	inp_overrun();
	pciconf.errlog=i;
	printf("event length="); l=0;
	for (i=0; i < 6; i++)
		if (evlen) {
			if (l) printf("/");
			printf("%08lX(%u)", evlen, i);
			l +=evlen;
		}

	Writeln();
	printf("loops=0x%08lX, events=", glp); l=0;
	if (evcounter) {
		if (l) printf("/");
		printf("%08lX", evcounter);
		evcounter;
	}

	Writeln();
	if (ltmp) printf("%lu per sec\n", (100*l)/ltmp);

	printf("%d\n", ret);
	return CE_PROMPT; //ret;
}
#endif
#endif
//
//--------------------------- dac_moves -------------------------------------
//
static int dac_moves(void)
{
	SC_DB		*sconf=&config.sc_db;
	F1TDC_BC	*ibcregs;
	u_int		inp;
	INP_DB	*iconf;
	GPX_RG	*iregs;
	u_int		lp=0;
#if 1
	u_int		dac,up;

	dac=0; up=1;
	while (1) {
			ibcregs=sconf->inp_bc;
			ibcregs->f1_addr=8;
			ibcregs->f1_reg[11]=dac;
			gtmp=ibcregs->f1_error;
			gtmp=ibcregs->f1_error;
			gtmp=ibcregs->f1_error;
			ibcregs->f1_reg[1]=0x0001;
			gtmp=ibcregs->f1_error;
			gtmp=ibcregs->f1_error;
			gtmp=ibcregs->f1_error;
			for (inp=0, iconf=sconf->inp_db; inp < sconf->slv_cnt; inp++, iconf++) {
				if (iconf->type != GPXTDC) continue;

				iregs=iconf->inp_regs;
				iregs->gpx_dac=dac;
			}

		if (up) {
			if (++dac >= 0x100) {
				dac=0xFF; up=0;
				printf("\r%5d", ++lp);
			}
		} else {
			if (--dac >= 0x100) {
				dac=0x00; up=1;
				printf("\r%5d", ++lp);
			}
		}

#ifdef PBUS
		inp_bc->ctrl=TESTPULSE;
#else
		sc_regs->ctrl=TESTPULSE;
#endif
		delay(1);
		if ((dac == 0x80) && kbhit()) return CEN_MENU;
	}
#else
	u_int	dac=0;
	int	i;

	while (1) {
			sconf=&config.sc_db;
			ibcregs=sconf->inp_bc;
printf("%08lX\n", ibcregs);
			ibcregs->f1_addr=8;
			ibcregs->f1_reg[11]=dac;
			gtmp=ibcregs->f1_error;
			gtmp=ibcregs->f1_error;
			gtmp=ibcregs->f1_error;
			ibcregs->f1_reg[1]=0x0001;
			gtmp=ibcregs->f1_error;
			gtmp=ibcregs->f1_error;
			gtmp=ibcregs->f1_error;

		for (i=0; i < 2; i++) {
			if (kbhit()) return CEN_MENU;

			sleep(1);
		}

		if (dac == 0) dac=0x80;
		else {
			if (dac == 0x80) dac=0xFF;
			else dac=0;
		}
	}
#endif
}
//
//--------------------------- ext_time_apl ----------------------------------
//
static int ext_time_apl(void)
{
//	F1TDC_BC	*sibc;
	int		ret;
	u_int		i;
	u_int		lx;
	int		cha;
	u_int		col;
	u_long	xtm;
	double	ftm;

	m_mode=1;
	if ((ret=inp_mode(0)) != 0) return ret;

	if (rout_hdr != MCR_DAQRAW) {
		printf("you must select \"scan/raw\"\n");
		return CE_PROMPT;
	}

	use_default=0; cha=-1;
	while (1) {
		if ((ret=read_event(EVF_NODAC, RO_RAW)) != 0) {
			if (ret == CEN_DAC_CHG) continue;

			return ret;
		}

		if (rout_len != (u_int)robf[0]) {
			printf("rout_len %04X/%08lX\n", rout_len, robf[0]);
			return CE_FATAL;
		}

      lx =rout_len >>2; 
		for (i=HDR_LEN, col=0; i < lx; i+=2) {
			if (cha < 0) cha=EV_CHAN(robf[i]);
			else {
				if (cha != EV_CHAN(robf[i])) continue;
			}

			if ((u_int)(robf[i+1] >>16) != ((u_int)(robf[i] >>16) |0x0FFF)) {
				printf("data pair %08lX/%08lX\n", robf[i], robf[i+1]);
				return CE_FATAL;
			}

			xtm=((u_long)(u_int)robf[i+1] <<6) |EV_MTIME(robf[i]);
			ftm=(double)xtm*6400. +(double)EV_FTIME(robf[i])*RESOLUTIONNS;
			if (col) printf(" ");
			printf(" %02X:%06lX/%04X %11.0f", cha,
					 xtm, EV_FTIME(robf[i]), ftm);
			col++;

			if (col >= 2) { col=0; Writeln(); }
		}

		if (col) Writeln();

	}
}
//
#if 0
//--------------------------- spec_test -------------------------------------
//
static int spec_test(void)
{
	F1TDC_BC	*sibc;
	int		ret;
	u_int		i;
	u_int		lx;
	char		m;
	char		err=0;
	int		cha=-1;
	u_int		col;
	u_long	xtm;
	double	ftm;
	double	lst=0.;
	double	blst;
	double	cyc;
	double	bst;
	double	df;

	if ((ret=inp_mode(0)) != 0) return ret;

	if (rout_hdr != MCR_DAQRAW) {
		printf("you must select \"scan/raw\"\n");
		return CE_PROMPT;
	}

	sibc=config.sc_db.inp_bc;
	sibc->cr=ICR_EXT_RAW;

	// es koennen korrupte alte Daten sein
	printf("dummy read\n");
	if ((ret=read_event(EVF_NODAC, RO_RAW)) != 0) return ret;

	glp=0;
#ifndef PBUS
	tstlen=1;
#endif
	for (col=0; col < 2; col++) {
		if ((ret=read_event(EVF_NODAC, RO_RAW)) != 0) return ret;

		if (rout_len != (u_int)robf[0]) return CE_FATAL;

		lx =rout_len >>2;
		for (i=HDR_LEN, m=0; i < lx; i+=2) {
			xtm=((u_long)(u_int)robf[i+1] <<6) |EV_MTIME(robf[i]);
			ftm=(double)xtm*6400. +(double)EV_FTIME(robf[i])*RESOLUTIONNS;
			if (cha < 0) cha=EV_CHAN(robf[i]);
			else {
				if (cha != EV_CHAN(robf[i])) continue;
			}

			if (!m) {
				cyc=ftm-lst;
				lst=ftm;
				blst=ftm;
			} else {
				bst=ftm-blst;
				blst=ftm;
			}

			m++;
		}
	}

	printf("cha=%03X, cycle=%1.2fms, burst=%u*%1.0fns\n", cha, cyc/1000000., m, bst);
	while (!err) {
		if ((ret=read_event(EVF_NODAC, RO_RAW)) != 0) {
			printf("\r%10lu\n", glp);
			break;
		}

		if (rout_len != (u_int)robf[0]) return CE_FATAL;

		lx =rout_len >>2;
		for (i=HDR_LEN, m=0; i < lx; i+=2) {
			if (cha != EV_CHAN(robf[i])) continue;

			if ((u_int)(robf[i+1] >>16) != 0x7FFF) return CE_FATAL;

			xtm=((u_long)(u_int)robf[i+1] <<6) |EV_MTIME(robf[i]);
			ftm=(double)xtm*6400. +(double)EV_FTIME(robf[i])*RESOLUTIONNS;
			if (!m) {
				m=1;
				blst=ftm;
				df=ftm-lst;
				lst=ftm;
				if (df < 0.) df=df+(double)0x400000L*6400.;

				if ((df-50. >= cyc) || (df+50. < cyc)) {
					printf("\r%10lu error cycle\n", glp); err=1; break;
				}
			} else {
				df=ftm-blst;
				blst=ftm;
				if ((df-5. >= bst) || (df+5. < bst)) {
					printf("\r%10lu error burst\n", glp); err=1; break;
				}
			}
		}

		glp++;
		if (!(glp &0x0FF)) {
			printf("\r%10lu", glp);
		}
	}

	for (i=HDR_LEN, col=0; i < lx; i+=2) {
		if (cha < 0) cha=EV_CHAN(robf[i]);
		else {
			if (cha != EV_CHAN(robf[i])) continue;
		}

		if ((u_int)(robf[i+1] >>16) != 0x7FFF) return CE_FATAL;

		xtm=((u_long)(u_int)robf[i+1] <<6) |EV_MTIME(robf[i]);
		ftm=(double)xtm*6400. +(double)EV_FTIME(robf[i])*RESOLUTIONNS;
		if (col) printf(" ");
		printf(" %02X:%06lX/%04X %11.0f", cha,
				 xtm, EV_FTIME(robf[i]), ftm);
		col++;

		if (col >= 2) { col=0; Writeln(); }
	}

	if (col) Writeln();

	printf("ret=%02X, cha=%03X, cycle=%1.0fms, burst=%1.0fns\n",
			 ret, cha, cyc/1000000., bst);
	return CE_PROMPT;
}
//
#else
//--------------------------- spec_test -------------------------------------
//
static int spec_test(void)
{
	int		ret;
	u_int		i;
	u_int		lx;
	u_int		un;
	u_int		cha;
	u_int		col;
	u_char	typ=0;
	u_long	dat;
	int		fstrt,gstrt;
	long		fdat,gdat;
	u_int		mx=0;
	GPX_RG	*gregs=(GPX_RG*)inp_unit;

	if ((ret=inp_mode(0)) != 0) return ret;

	use_default=0;
	while (1) {
		if ((ret=read_event(0, RO_RAW)) != 0) {
			if (ret <= CEN_PPRINT) continue;

			return ret;
		}

		if ((i=gregs->gpx_int_err) &GPX_ERRFL) {
			gregs->gpx_seladdr=12;
			printf("GPX error reg12=%07lX\n", gregs->gpx_data);
		}

		if (++mx &0xFF) continue;

		fstrt=-1; gstrt=-1;
		i=0; lx =rout_len >>2; col=0;
		while (i < lx) {
			dat=robf[i];
			if (rout_hdr && (i < HDR_LEN)) {
				if (col) printf(" ");
				printf(" %08lX", dat); col++;

				if (i == (HDR_LEN-1)) { Writeln(); col=0; }
			} else {
				cha=EV_CHAN(dat);
				un=(u_int)(dat >>28);
				if (i == 0) {
					printf("cnt=%04X(%4u)\n", lx, lx); typ=inp_type[un];
				} else {
					if (inp_type[un] != typ) {
						if ((i != HDR_LEN) && col) { Writeln(); col=0; }

						typ=inp_type[un];
					}
				}

				if (col) printf(" ");
				if (typ == F1TDC) {
					fstrt=(u_int)(dat >>16)&0xF;
					fdat=(u_int)dat;
					printf(" %02X:%02X/%04X", cha,
							 EV_MTIME(dat), EV_FTIME(dat));
				} else {
					if ((typ == GPXTDC) || (typ == GPXTDCTST)) {

						gdat=(dat &0x1FFFFL)-0x461;
						gstrt=(u_int)(dat>>18) &0xF;
						if (gdat < 0x37) { gdat+=gpx_range; gstrt=(gstrt-1)&0xF; }

						gdat=(3*gdat+2)/4;
						printf(" %02X:%2X/%04lX", cha, gstrt, gdat);

						printf("  %X/%X/%05lX",
								 (u_int)(dat>>18) &0xF,
								 (u_int)(dat>>17) &0x01, dat &0x1FFFFL);
						col++;
					} else printf(" %08lX", dat);
				}

				if ((fstrt >= 0) && (gstrt >= 0)) {
					u_char	err=0;
					long		diff;

					if (fstrt == gstrt) diff=gdat-fdat;
					else
						if (((fstrt-1)&0xF) == gstrt) diff=gdat-(fdat+f1_range);
						else
							if (((fstrt+1)&0xF) == gstrt) diff=(gdat+f1_range)-fdat;
							else err=1;

					if (!err) printf(" ==> %3ld", diff);
					else printf(" == > error");
				}

				col++;
			}

			i++;
			if (col >= 8) { col=0; Writeln(); }
		}

		if (col) Writeln();
	}
}
#endif
//--------------------------- inp_offline -----------------------------------
//
static int inp_offline(void)
{
	u_int	inp;
	u_int	lst=0;

	last_key=0;
	for (inp=0; inp < config.sc_db.slv_cnt; inp++) {
		if (last_key == CTL_A) {
			config.inp_offl[inp]=lst;
			continue;
		}

		printf("unit %2u disable %u ", inp, config.inp_offl[inp]);
		config.inp_offl[inp]=lst=(u_char)Read_Deci(config.inp_offl[inp], 1);
		if (errno) return CEN_MENU;
	}

	for (inp=0; inp < 16; inp++) inp_offl[inp]=config.inp_offl[inp];
	return CEN_PROMPT;
}
//
//--------------------------- err_logging -----------------------------------
//
static int err_logging(void)
{
	printf("Error Logging %4d ", pciconf.errlog);
	pciconf.errlog=(u_short)Read_Deci(pciconf.errlog, -1);
	return 0;
}
//
//===========================================================================
//
static int input_cards(void)
{
	int		ret=0;
	char		rkey,key;

	if (!sc_conf) return CE_PARAM_ERR;

	while (1) {
		while (kbhit()) getch();

#ifdef SC_M
		if (BUS_ERROR) CLR_PROT_ERROR();
#else
		if (lvd_regs->sr &SR_LB_TOUT) {
			printf("<===> bus error <===>\n");
			lvd_regs->sr =SR_LB_TOUT;
		}
#endif
		if (ret > CE_PROMPT) display_errcode(ret);

		errno=0; ret=0;
		Writeln();
		inp_assign(sc_conf, 1);
		if ((gtmp=inp_bc->card_offl) != 0) {
			printf("not all Card online                %04X     <===>", gtmp);
			if (gtmp != 1) printf(" error");
			Writeln();
		}

		printf("online Input Cards                 %04X\n", inp_bc->card_onl);
		if (inp_bc->ro_data)
			printf("hitdata                            %04X     <===>\n", inp_bc->ro_data);
		if (inp_bc->f1_error)
			printf("error flags                        %04X     <===>\n", inp_bc->f1_error);
		printf("Master Reset, assign input Bus          ..M/B\n");
		if (inp_conf) {
printf("input unit (addr %2u)               %4u ..U\n", inp_conf->slv_addr, sc_conf->slv_sel);
printf("Version                            %04X\n", inp_unit->ident);
			switch (inp_conf->type) {
		case F1TDC:		return tdcf1_menu();
		case VERTEX:	return vertex_menu();
		case SYMASTER:	return sync_master_menu(0);
		case SYSLAVE:	return sync_slave_menu();
		case FQDCTST:	return qdcftst_menu();
		case FQDC:		return qdcf_menu();
		case GPXTDC:
		case GPXTDCTST:return gpxtdc_menu();
		case SQDCTST:	return qdcstst_menu();
		case SQDC:		return qdcs_menu();
		case JTEST:		return jtest_menu();
			}

			printf("unknown module!!!\n");
		}

		printf("                                          %c ", config.tst_menu);
		while (1) {
			rkey=getch();
			key=toupper(rkey);
			if ((key == CTL_C) || (key == ESC)) return CE_MENU;

			break;
		}

		if ((key == ESC) || (key == CR)) continue;

		if (key > ' ') printf("%c", key);

		Writeln();
		if (key == 'M') {
			inp_bc->ctrl=MRESET;
			continue;
		}

		if (key == 'B') {
			inp_assign(sc_conf, 0);
			continue;
		}

		if (!inp_conf) continue;

		if (rkey == 'u') {
			inp_assign(sc_conf, 2);
			continue;
		}
	}
}
//
//===========================================================================
//
//------------------------------ raw_input ----------------------------------
//
int raw_input(
	int	sel)	// selected channels
{
	int		ret;
	u_int		i;
	u_int		lx;
	u_long	cmask[2*C_UNIT];
	u_int		un;
	u_int		cha;
	u_int		col;
	u_char	typ=0;
	u_long	dat;

	if (sel) {
		for (i=0; i < 2*C_UNIT; cmask[i++]=0);

		printf("use \"-1\" to start\n");
		i=0;
		while (1) {
			if (config.sel_cha[i] < 0)
				printf("enable channel %5d ", config.sel_cha[i]);
			else
				printf("enable channel 0x%03X ", config.sel_cha[i]);

			config.sel_cha[i]=(int)Read_Hexa(config.sel_cha[i], -1);
			if (errno) return CEN_MENU;

			if (config.sel_cha[i] < 0) {
					if (config.sel_cha[i] == -2) {
					for (i=0; i < 2*C_UNIT; cmask[i++]=0xFFFFFFFFL);
					break;
				}

				config.sel_cha[i]=-2;
				break;
			}

			if (config.sel_cha[i] >= 64*C_UNIT) {
				printf("max channel is 0x%03X\n", 64*C_UNIT-1);
				continue;
			}

			cmask[config.sel_cha[i] >>5] |=(1L <<(config.sel_cha[i] &0x1F));
			i++;
			if (i >= C_SEL_CHA) break;
		}
	}

	if ((ret=inp_mode(0)) != 0) return ret;

	use_default=0;
	while (1) {
		if ((ret=read_event(EVF_NODAC, RO_RAW)) != 0) {
			if (ret <= CEN_PPRINT) continue;

			return ret;
		}

		if (!sel) for (i=0; i < 2*C_UNIT; cmask[i++]=0);

		i=0; lx =rout_len >>2; col=0;
		if (rout_hdr) {
			while ((i < HDR_LEN) && (i < lx)) printf("  %08lX", robf[i++]);

			if (config.daq_mode &EVF_HANDSH) printf("    %06lX", rout_trg_time);

			Writeln();
		}

		while (i < lx) {
			dat=robf[i];
			cha=(u_int)(dat >>22);
			un=(u_int)(dat >>28);
			if (i == 0) {
				printf("cnt=%04X(%4u)\n", lx, lx); typ=inp_type[un];
			} else {
				if (inp_type[un] != typ) {
					if ((i != HDR_LEN) && col) {
						if (col) Writeln();
						col=0;
					}

					typ=inp_type[un];
				}
			}

			if (sel) {
				if (/*(typ == F1TDC) && */(cmask[(cha >>5) &(2*C_UNIT-1)] &(1L <<(cha &0x1F)))) {
//						if (col) printf(" ");
					printf(" %X%02X%06lX", cha>>6, cha&0x3F, dat &0x3FFFFFL);
					col++;
				}
			} else {
//					if (col) printf(" ");
//					if (typ == F1TDC) {
					if (cmask[(cha >>5) &(2*C_UNIT-1)] &(1L <<(cha &0x1F))) {
						printf(".");
					} else {
						cmask[(cha >>5) &(2*C_UNIT-1)] |=(1L <<(cha &0x1F));
						printf(" ");
					}

					printf("%X%02X%06lX", cha>>6, cha&0x3F, dat &0x3FFFFFL);
#if 0
				} else {
					if (typ == GPXTDC) {
						printf(" %X/%X/%X/%X/%05lX", (u_int)(dat>>28),
								 (u_int)(dat>>22) &0x3F, (u_int)(dat>>18) &0xF,
								 (u_int)(dat>>17) &0x01, dat &0x1FFFFL);
						col++;
					} else printf(" %08lX", dat);
				}
#endif

				col++;
			}

			i++;
			if (col >= 8) { col=0; /*Writeln();*/ }
		}

		if (col) Writeln();
	}
}
//
//------------------------------ log_data -----------------------------------
//
int log_data(void)
{
	int		ret;

	printf("datafile: ");
	if ((ret=open_datafile()) != 0) return ret;

	if (datafile < 0) return 0;

	if ((ret=inp_mode(0)) != 0) return ret;

	use_default=0;
	while (1) {
		if ((ret=read_event(EVF_NODAC, RO_RAW)) != 0) {
			if (ret <= CEN_PPRINT) continue;

			close_datafile();
			Writeln();
			return ret;
		}

		if (write(datafile, robf, rout_len) != rout_len) {
			perror("error writing config file");
			printf("name =%s", config.dlogname);
			close_datafile();
			return CEN_PROMPT;
		}
	}
}
//
//===========================================================================
//
//--------------------------- application_dial ------------------------------
//
int application_dial(
	u_char	pup)	// 1: Last Values
						// 2: Standard Values
{
	u_int	ux;
	char	key;
	int	ret;

	if (!tm_hit && ((tm_hit=malloc(sizeof(TM_HIT))) == 0)) {
		printf("can't allocate buffer\n");
		return CEN_PROMPT;
	}

#ifdef SC_M
	if (!robuf && ((robuf=calloc(0x8000, 2)) == 0)) {
		printf("can't allocate buffer\n");
		return CEN_PROMPT;
	}
#endif

	std_setup=(pup == 2);
	errno=0; use_default=pup;

#ifdef SC_M
	if ((ret=sc_assign(1)) == CE_NO_SC) return CEN_PROMPT;

	if (ret == CE_NO_INPUT) ret=0;
#endif

#ifdef PBUS
	lvd_units=lvd_bus;
	lvd_regs->lvd_scan_win=0x14040L;
	ret=sc_assign(1);
#endif

//printf("pup=%u, ret=%d\n",pup, ret); use_default=0; if (WaitKey(0)) return 0;
	if (pup && ((ret == 0) || (ret == CE_PROMPT))) ret=general_setup(-2, -1);

	if ((ret < 0) || (ret > CE_MENU)) {
		display_errcode(ret);
		use_default=0; WaitKey(0);
	} else ret=0;

	f1_range=(long)(6400000.0/RESOLUTION +0.5);
	if (!config.f1_sync) f1_range=0x10000L;

	tm_win_he=(long)(config.ef_trg_lat*1000.0/RESOLUTION +0.5);
	tm_win_le=(long)((config.ef_trg_lat-config.ef_trg_win)*1000.0/RESOLUTION +0.5);
#ifdef MWPC
	if (ret == 0) {
		if (pup == 2) config.c0_menu='8';

		if (pup && (config.c0_menu == '8')) mwpc_application();
	}
#endif

	std_setup=0;
	for (ux=0; ux < 16; ux++) inp_offl[ux]=config.inp_offl[ux];
	while (1) {
		clrscr();
		ux=0;
#ifdef SC_M
		gotoxy(SP_, ux+++ZL_);
						printf("System Controller main Register .0");
#endif
#ifndef PBUS
		gotoxy(SP_, ux+++ZL_);
						printf("system controller ...............1");
#endif

		gotoxy(SP_, ux+++ZL_);
						printf("LVD Process Bus .................2");

		gotoxy(SP_, ux+++ZL_);
						printf("raw input / selective ...........6/7");
#ifndef PBUS
		gotoxy(SP_, ux+++ZL_);
						printf("relative time / selective .......8/9");
		gotoxy(SP_, ux+++ZL_);
						printf("pulse width .....................A");
#ifdef MWPC
		gotoxy(SP_, ux+++ZL_);
						printf("MWPC application ................B");
#endif
#ifdef TOF
		gotoxy(SP_, ux+++ZL_);
						printf("TOF application .................B");
#endif
#endif
#ifdef MEASURE
		gotoxy(SP_, ux+++ZL_);
						printf("measurement .....................C");
#endif
		gotoxy(SP_, ux+++ZL_);
						printf("set DAC .........................D");
		gotoxy(SP_, ux+++ZL_);
						printf("DAC ramp ........................F");
		gotoxy(SP_, ux+++ZL_);
						printf("extended time measuring .........G");
		gotoxy(SP_, ux+++ZL_);
						printf("special test ....................X");
		gotoxy(SP_, ux+++ZL_);
						printf("setup ...........................S");
		gotoxy(SP_, ux+++ZL_);
						printf("set inp offline .................H");
		gotoxy(SP_, ux+++ZL_);
						printf("list of units ...................U");
		gotoxy(SP_, ux+++ZL_);
						printf("log data to file ................L");
		gotoxy(SP_, ux+++ZL_);
						printf("set error log / check F1ovfl.....E/O");
#ifdef SC_M
#if 0
		gotoxy(SP_, ux+++ZL_);
						printf("event counter ...................I");
#endif
#endif
		gotoxy(SP_, ux+++ZL_);
						printf("Ende ..........................ESC");
		gotoxy(SP_, ux+++ZL_);
						printf("                                 %c ",
									config.c2_menu);

		key=toupper(getch());
		if ((key == CTL_C) || (key == ESC)) return 0;

		while (1) {
			use_default=(key == TAB);
			if ((key == TAB) || (key == CR)) key=config.c2_menu;

			clrscr(); gotoxy(1, 10);
			errno=0; ret=CEN_MENU;
			eventcount=0;
			switch (key) {

#ifdef SC_M
		case '0':
				ret=system_control_master();
				break;
#endif
#ifndef PBUS
		case '1':
#ifdef SC_M
				if (sy_regs) {
					ret=system_control_GPX();
					break;
				}
#endif
				ret=system_control_F1();
				break;
#endif

		case '2':
				while ((ret=input_cards()) == CE_NO_INPUT);

				break;

		case '6':
				ret=raw_input(0);
				break;

		case '7':
				ret=raw_input(1);
				break;

#ifndef PBUS
		case '8':
				ret=triggered_input(0);
				ux=pciconf.errlog; pciconf.errlog=1;
				inp_overrun();
				pciconf.errlog=ux;
				break;

		case '9':
				ret=triggered_input(1);
				ux=pciconf.errlog; pciconf.errlog=1;
				inp_overrun();
				pciconf.errlog=ux;
				break;

		case 'A':
				ret=pulse_width();
				break;
#endif

#ifdef MWPC
		case 'B':
				ret=mwpc_application();
				break;
#endif
#ifdef TOF
		case 'B':
				ret=tof_application();
				break;
#endif

#ifdef MEASURE
		case 'C':
				ret=measurement();
				break;
#endif

#ifdef SC_M
#if 0
		case 'I':
				ret=ev_counter();
				break;
#endif
#endif

		case 'D':
				last_key=0;
				if ((ret=inp_setdac(0, 0, -2, DAC_DIAL)) <= CE_PROMPT) ret=CEN_PROMPT;
				break;

		case 'E':
				err_logging();
				break;

		case 'F':
				ret=dac_moves();
				break;

		case 'G':
				ret=ext_time_apl();
				m_mode=0;
				break;

		case 'H':
				ret=inp_offline();
				break;

		case 'X':
				ret=spec_test();
				break;

		case 'L':
				ret=log_data();
				break;

		case 'O':
				ret=CEN_PROMPT;
				ux=pciconf.errlog; pciconf.errlog=1;
				inp_overrun();
				pciconf.errlog=ux;
				break;

		case 'S':
				if ((ret=general_setup(-2, -1)) <= CE_PROMPT) ret=CEN_PROMPT;
				break;

		case 'U':
				ret=list_units();
				break;

			}

			for (ux=0; ux < 16; ux++) inp_offl[ux]=config.inp_offl[ux];
			stop_ddma();
			while (kbhit()) getch();
			if ((ret &~0x1FF) == CE_KEY_PRESSED) ret=CE_PROMPT;

			if ((ret >= 0) && (ret <= CE_PROMPT)) config.c2_menu=key;

			if ((ret >= CEN_MENU) && (ret <= CE_MENU)) break;

			if (ret > CE_PROMPT) display_errcode(ret);

			printf("select> ");
			key=toupper(getch());
			if ((key < ' ') && (key != TAB)) break;

			if (key == ' ') key=CR;
		}
	}
}
