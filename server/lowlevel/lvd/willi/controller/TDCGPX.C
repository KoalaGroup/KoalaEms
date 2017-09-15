// Borland C++ - (C) Copyright 1991, 1992 by Borland International

/*	gpx.c -- general functions  */

// Resolution 97.47(0x98) *4 conforms to F1 resolution 129.95(0xA2) *3

#include <stddef.h>		// offsetof()
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>
#include <errno.h>
#include <dos.h>
#include "pci.h"
#include "daq.h"

extern GPX_BC		*inp_bc;
extern GPX_RG		*inp_unit;

static u_int	gpxnum;
static u_int	gtmp;

//--------------------------- gpx_test --------------------------------------
//
static int gpx_test(void)
{
	int		ret;
	u_int		i;
	u_long	dat,ma,mi;

	config.daq_mode=EVF_SORT;
	inp_mode(IM_QUIET|IM_VERTEX);
	while (1) {
		if ((ret=read_event(EVF_NODAC, RO_RAW)) != 0) return ret;

		ma=0; mi=0xFFFFFFFFL;
		printf("%04X Data read\n", rout_len);
		for (i=0; (i < rout_len/4) && (i < 16); i++) {
			if (!(i %4)) {
				if (i) printf("\n");
				printf("%04X:", i*4);
			}

			printf(" %03X.%06lX",
					 (u_int)(dmabuf[0][i]>>22), dat=dmabuf[0][i]&0x3FFFFFL);
			if (mi > dat) mi=dat;
			if (ma < dat) ma=dat;
		}

		printf(" diff=%u\n", ma-mi);
	}
}
//
//--------------------------- gpx_burst_test --------------------------------
//
#define W_LIMIT	500

static int gpx_burst_test(void)
{
	int		ret;
	u_int		h,i;
	u_int		lx;
	u_long	dat,xdat;
	u_long	ldat=0;
	long		diff;
	u_int		cnt,lcnt=0;
	char		err=0;
	u_long	lp=0;
	u_int		ln=0;

	if ((ret=inp_mode(0)) != 0) return ret;

	use_default=0;
	printf("enter CR => display, 'A' => sign adjust, 'a' => start adjust\n");
	h=(rout_hdr) ? HDR_LEN : 0;
	while (1) {
		if ((ret=read_event(EVF_NODAC, RO_RAW)) != 0) {
			if (ret == (CE_KEY_PRESSED|CR)) {
				err=1; continue;
			}

			if (ret == (CE_KEY_PRESSED|'A')) {
				err=-1; continue;
			}

			if (ret == (CE_KEY_PRESSED|'a')) {
				err=-2; continue;
			}

			return ret;

		}

		if ((i=inp_unit->gpx_int_err) &GPX_ERRFL) {
			inp_unit->gpx_seladdr=12;
			printf("GPX error %07lX\n", inp_unit->gpx_data);
			return CE_PROMPT;
		}

		lx=rout_len >>2; 
		for (i=h; i < lx; i++) {
			dat=robf[i];
			xdat=dat &0x1FFFFL;
			cnt=(u_int)(dat >>18) &0x0F;
			if (xdat >= 0x18000L) {
				xdat=(xdat+gpx_range) &0x1FFFFL;
				cnt=(cnt-1) &0x0F;
				if (err == -1) err=1;
			}

			if (i != h) {
				if (cnt == lcnt) {
					diff=xdat-ldat;
					if ((diff < -W_LIMIT) || (diff > W_LIMIT)) err=10;
				} else {
					if (((cnt-lcnt) &0x0F) == 1) {
						diff=gpx_range+xdat-ldat;
						if ((diff < -W_LIMIT) || (diff > W_LIMIT)) err=10;
						if (err == -2) err=1;
					} else {
						err=10;
					}
				}
			}

			ldat=xdat; lcnt=cnt;
		}

		if (!(++lp &0xFF)) printf("\r%10lu  ", lp);

		if (err <= 0) continue;

		printf("\r%10lu hits=%04X\n", lp, lx-h);
		for (i=h; i < lx; i++) {
			dat=robf[i];
			xdat=dat &0x1FFFFL;
			cnt=(u_int)(dat >>18) &0x0F;
			printf("%02X/%X/%u/%05lX",
					 (u_int)(dat >>22), cnt,
					 (u_int)(dat >>17) &0x01, xdat);
			if (xdat >= 0x18000L) {
				xdat=(xdat+gpx_range) &0x1FFFFL;
				cnt=(cnt-1) &0x0F;
				printf("  %X/%05lX", cnt, xdat);
			} else printf("         ");

			printf("  %8.2f", xdat*G_RESOLUTIONNS);
			if (i != h) {
				if (cnt == lcnt) {
					diff=xdat-ldat;
					if ((diff < -W_LIMIT) || (diff > W_LIMIT)) {
						printf("          *");
//						diff=(0x10000L-ldat)+xdat;
					} else

					printf("  %8.2f", diff*G_RESOLUTIONNS);
				} else {
					if (((cnt-lcnt) &0x0F) == 1) {
						diff=gpx_range+xdat-ldat;
						if ((diff < -W_LIMIT) || (diff > W_LIMIT)) {
							printf("          *");
//							diff=gpx_range-(0x10000L-xdat)-ldat;
						} else

						printf("  %8.2f*", diff*G_RESOLUTIONNS);
					} else {
						printf("          *");
					}
				}
			}

			Writeln();
			if ((err >= 10) && (++ln >= 40)) {
				ln=0;
				if (WaitKey(0)) break;
			}

			ldat=xdat; lcnt=cnt;
		}

		if (err >= 10) return CE_PROMPT;

		err=0;
	}
}
//
//--------------------------- GPX_reset -------------------------------------
//
static int GPX_reset(u_int	gpx)
{
	u_int		addr=gpx <<4;
	u_long	tmp;
	u_long	reg4=0x20000FFL;

	inp_unit->ctrl=PURES;
	inp_unit->gpx_seladdr=addr |0;
	tmp=inp_unit->gpx_data;
	if (BUS_ERROR || (tmp != 0x80)) return CE_FATAL;

	inp_unit->gpx_seladdr=addr |2;
	inp_unit->gpx_data=0x0000002L;
	inp_unit->gpx_seladdr=addr |0;
	inp_unit->gpx_data=0x007FC81L;
	inp_unit->gpx_seladdr=addr |7;
	inp_unit->gpx_data=0x0001FB0L;
	inp_unit->gpx_seladdr=addr |4;
	inp_unit->gpx_data=reg4;
	inp_unit->gpx_seladdr=addr |11;
	inp_unit->gpx_data=0x7FF0000L;
	inp_unit->gpx_seladdr=addr |12;
	inp_unit->gpx_data=0x4000000L;
	inp_unit->gpx_seladdr=addr |4;
	inp_unit->gpx_data=reg4 |0x0400000L;
	return 0;
}
//
//--------------------------- gpx_resadjust ---------------------------------
//
static struct {
	u_int	divmin,divmax;
} rrange[8];

static int gpx_resadjust(	// 0: ok
	u_int	gpx)
{
	int		ret;
	u_int		addr=gpx <<4;
	u_long	tmp;
	u_int		hsdiv_min,hsdiv_max;

	if ((ret=GPX_reset(gpx)) != 0) return ret;

	hsdiv_max=0xE0;
	while (1) {
		inp_unit->gpx_seladdr=addr |7;
		inp_unit->gpx_data=0x0001800L|(REFCLKDIV<<8)|hsdiv_max;
		inp_unit->gpx_seladdr=addr |12;
		delay(200);
		tmp=inp_unit->gpx_data;
		if (BUS_ERROR) return CE_FATAL;

		if (!(tmp &0x400)) break;
		printf("\rmax %02X ", hsdiv_max);
		hsdiv_max--;
		if (hsdiv_max <= 0x60) return CE_F1_NOLOCK;
	}

	hsdiv_min=hsdiv_max;
	do {
		hsdiv_min--;
		inp_unit->gpx_seladdr=addr |7;
		inp_unit->gpx_data=0x0001800L|(REFCLKDIV<<8)|hsdiv_min;
		inp_unit->gpx_seladdr=addr |12;
		delay(200);
		tmp=inp_unit->gpx_data;
		if (BUS_ERROR) return CE_FATAL;

		printf("\rmin %02X ", hsdiv_min);
	} while (!(tmp &0x400) && hsdiv_min);

	hsdiv_min++;
	rrange[gpx].divmax=hsdiv_max;
	rrange[gpx].divmin=hsdiv_min;
	printf("\r%u: hsdiv max/min =0x%02X/%02X = %5.1f/%5.1fps => recommend = %5.1f\n",
			 gpx, hsdiv_max, hsdiv_min, G_HSDIV/hsdiv_max, G_HSDIV/hsdiv_min, G_HSDIV/(hsdiv_max-8));

	inp_unit->gpx_seladdr=addr |7;
	inp_unit->gpx_data=0x0001800L|(REFCLKDIV<<8)|(hsdiv_max-8);
	return 0;
}
//
//------------------------------ gpx_regdiag --------------------------------
//
static u_int	setnr=0;
#ifndef PBUS
static char		stmode[]="S";
#endif

static int gpx_regdiag(
	u_long	*regs)
{
	char		rkey,key;
	u_int		ux;
	char		hd=1;
	u_char	cnt=0;
	u_int		addr=inp_conf->slv_data[12] <<4;
	u_long	tmp;

	while (1) {
		use_default=0;
		while (kbhit()) getch();

#ifdef SC_M
		if (BUS_ERROR) {
			CLR_PROT_ERROR();
			return -1;
		}
#else
		if (lvd_regs->sr &SR_LB_TOUT) {
			printf("<===> bus error <===>\n\n");
			lvd_regs->sr =SR_LB_TOUT;
			return -1;
		}
#endif
		if (hd) {
printf("GPX chip select                                           %7u..g\n", inp_conf->slv_data[12]);
printf("2|2|2|2|     |2|     |1|     |1|     |0|     |0|     |0|\n");
printf("7|6|5|4| | | |0| | | |6| | | |2| | | |8| | | |4| | | |0|\n");
		inp_unit->gpx_seladdr=addr |0;
printf("Fed[9]           |Red[9]           |0|0|1|0[6]      |Osc  %07lX..0\n", inp_unit->gpx_data);
		inp_unit->gpx_seladdr=addr |1;
printf("                                                  0[28]|  %07lX..1\n", inp_unit->gpx_data);
		inp_unit->gpx_seladdr=addr |2;
printf("0[16]                          |dis[9]           |R|I|G|  %07lX..2\n", inp_unit->gpx_data);
		inp_unit->gpx_seladdr=addr |3;
printf("                                                  0[28]|  %07lX..3\n", inp_unit->gpx_data);
		inp_unit->gpx_seladdr=addr |4;
printf("TBteEF0|PRmr0[14]                      |STim[8]        |  %07lX..4\n", inp_unit->gpx_data);
		inp_unit->gpx_seladdr=addr |5;
printf("RTrg|PRo|MRo|PRa|MRa|0[5]|SOffs[18]                    |  %07lX..5\n", inp_unit->gpx_data);
		inp_unit->gpx_seladdr=addr |6;
printf("ECL[2]|0[18]                           |Fill[8]        |  %07lX..6\n", inp_unit->gpx_data);
		inp_unit->gpx_seladdr=addr |7;
printf("MTim[13]                 |0|Tr|Ne|Adj|Cdiv[3] |HSdiv[8]|  %07lX..7\n", inp_unit->gpx_data);
//		inp_unit->gpx_seladdr=addr |8;
//printf("FIFO1                                                     %07lX\n", inp_unit->gpx_data);
//		inp_unit->gpx_seladdr=addr |9;
//printf("FIFO2                                                     %07lX\n", inp_unit->gpx_data);
		inp_unit->gpx_seladdr=addr |10;
printf("Start Time                                                %07lX\n", inp_unit->gpx_data);
		inp_unit->gpx_seladdr=addr |11;
printf("0|NLock|FF[2]|HFF[8]   |       -       |       -       |  %07lX..B\n", inp_unit->gpx_data);
		inp_unit->gpx_seladdr=addr |12;
printf("0|St|Tm|HEF|NLock|FF[2]|HFF[8]|Tm|HEF|NLock|FF[2]|HFF[8]  %07lX..C\n", inp_unit->gpx_data);

		inp_unit->gpx_seladdr=addr |14;
printf("0[23]                                       |16|0[4]   |  %07lX..E\n", inp_unit->gpx_data);
printf("GPX int/err flags                                            %04X\n", inp_unit->gpx_int_err);
printf("GPX FIFO empty flags                                         %04X\n", inp_unit->gpx_emptfl);
printf("FIFO1/2 reset/setup/Start/mres                                   ..8/9/r/s/S/m\n");
printf("                                                                   %c ", config.i0_menu);
			rkey=getch();
			if (!rkey) { getch(); continue; }

			if ((rkey == CTL_C) || (rkey == ESC)) { Writeln(); return -1; }

		} else {
			hd=1; printf("select> ");
			rkey=getch();
			if (rkey == CTL_C) return CE_MENU;

			if (rkey == ' ') rkey=config.i0_menu;

			if ((rkey == ESC) || (rkey == CR)) continue;
		}

		use_default=(rkey == TAB);
		if (use_default || (rkey == CR)) rkey=config.i0_menu;

		if (rkey >= ' ') printf("%c", rkey);

		Writeln();
		key=toupper(rkey);

		if (key == 'G') {
			printf("GPX chip  %5u ", inp_conf->slv_data[12]);
			inp_conf->slv_data[12]=(u_int)Read_Deci(inp_conf->slv_data[12], -1) &0x7;
			addr=inp_conf->slv_data[12] <<4;
			continue;
		}

		if (rkey == 's') {
			printf("setup %u ", setnr);
			setnr=(u_int)Read_Deci(setnr, -1);
			if (errno) continue;

			switch (setnr) {
		case	0:
				{
				u_long	reg4=0x2000000L;

				inp_unit->ctrl=PURES;
				inp_unit->gpx_seladdr=addr |2;
				inp_unit->gpx_data=0x0000002L;	// I-Mode
				inp_unit->gpx_seladdr=addr |0;
				inp_unit->gpx_data=0x007FC81L;	// rising, start ros
				inp_unit->gpx_seladdr=addr |7;
				inp_unit->gpx_data=0x0001FB0L;	// phase, ResAdj, ClkDiv=7, HSDiv=176
				inp_unit->gpx_seladdr=addr |4;
				inp_unit->gpx_data=reg4;			// EFlagHi
				inp_unit->gpx_seladdr=addr |11;
				inp_unit->gpx_data=0x7FF0000L;	// unmask all
				inp_unit->gpx_seladdr=addr |4;
				inp_unit->gpx_data=reg4 |0x0400000L;	// MasterReset
				break;
				}

		case	1:
				{
				u_long	reg4=0x20000FFL;

				inp_unit->ctrl=PURES;
				inp_unit->gpx_seladdr=addr |2;
				inp_unit->gpx_data=0x0000002L;	// I-Mode
				inp_unit->gpx_seladdr=addr |0;
				inp_unit->gpx_data=0xFFFFC81L;	// falling, rising, start ros
				inp_unit->gpx_seladdr=addr |7;
				inp_unit->gpx_data=0x0001FB0L;	// phase, ResAdj, ClkDiv=7, HSDiv=176
				inp_unit->gpx_seladdr=addr |4;
				inp_unit->gpx_data=reg4;
				inp_unit->gpx_seladdr=addr |11;
				inp_unit->gpx_data=0x7FF0000L;
				inp_unit->gpx_seladdr=addr |12;
				inp_unit->gpx_data=0x4000000L;
				inp_unit->gpx_seladdr=addr |4;
				inp_unit->gpx_data=reg4 |0x0400000L;
				break;
				}

		default:
				printf("\n>>> max value is 1 <<<\n\n");
				break;
			}

			cnt=0;
			continue;
		}

		if (rkey == 'S') {
#ifdef PBUS
			lvd_regs->lvd_scan_win=0x04040L;
			((u_short*)lvd_bus)[1]=0;
			lvd_regs->lvd_scan_win=0x14040L;
#else
			char	keys[2];

			printf("Single, Peri %s ", stmode);
			if (Readln(keys, 1)) continue;

			if ((keys[0]=toupper(keys[0])) != 0) {
				if ((keys[0] != 'S') && (keys[0] != 'P')) continue;

				stmode[0]=keys[0];
			}

			if (stmode[0] == 'S') {
				sc_conf->sc_unit->cr=MCR_NO_F1STRT;
				sc_conf->sc_unit->ctrl=F1START;
			} else sc_conf->sc_unit->cr=0;
#endif
			continue;
		}

		if (key == 'R') {
			inp_unit->ctrl=PURES;
			continue;
		}

		if (key == 'M') {
			inp_unit->gpx_seladdr=addr |4;
			tmp=inp_unit->gpx_data;
			inp_unit->gpx_data=tmp |0x0400000L;
			continue;
		}

		ux=key-'0';
		if (ux > 9) ux=ux-7;

		if ((ux == 8) || (ux == 9)) {
			config.i0_menu=rkey; hd=0;
			inp_unit->gpx_seladdr=addr |ux;
			tmp=inp_unit->gpx_data;
			printf("FIFO%u %02X: %07lX => %u/%02X/%u/%05lX  %04X\n", ux-7, cnt++, tmp,
					 (u_int)(tmp >>26), (u_int)(tmp >>18) &0xFF,
					 (u_int)(tmp >>17) &0x01, tmp &0x1FFFFL, inp_unit->gpx_emptfl);
			continue;
		}

		if (((ux >= 8) && (ux < 11)) || (ux >= 15)) continue;

		config.i0_menu=rkey;
		printf("Value %07lX ", regs[ux]);
		regs[ux]=Read_Hexa(regs[ux], 0xFFFFFFFL);
		if (errno) continue;

		inp_unit->gpx_seladdr=addr |ux;
		inp_unit->gpx_data=regs[ux];
	}
}
//===========================================================================
//										gpxtdc_menu                                   =
//===========================================================================
//
//--------------------------- jt_inp_getdata --------------------------------
//
static u_long jt_inp_getdata(void)
{
	return inp_unit->jtag_data;
}
//
//--------------------------- jt_inp_getcsr ---------------------------------
//
static u_int jt_inp_getcsr(void)
{
	return inp_unit->jtag_csr;
}
//
//--------------------------- jt_inp_putcsr ---------------------------------
//
static void jt_inp_putcsr(u_int val)
{
	inp_unit->jtag_csr=val |JT_SLOW_CLOCK;
	inp_unit->ident=0;			// nur delay
}
//
static const char *inpcr_txt[] ={
	"Trg", "Raw", "LeT", "Ext", "GPX", "?", "?", "?",
	"?",   "?",   "?",   "?",   "?",   "?", "?", "?"
};
//
static const char *inpsr_txt[] ={
	"RDerr", "?", "?", "?", "?", "?", "?", "?",
	"?",     "?", "?", "?", "?", "?", "?", "?"
};
//
static int		g_gpx=-1;
static u_long	cha_ena[2]={0xFFFFFFFFL,0xFFFFFFFFL};

int gpxtdc_menu(void)
{
	int		ret=0;
	u_int		i;
	u_int		tmp;
	char		hgh;
	char		rkey,key;
	u_int		ux;
	char		hd=1;

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

		if ((ret <= CEN_PROMPT) || (ret >= CE_PROMPT)) hd=0;

		errno=0; ret=0; use_default=0;
		if (hd) {
			Writeln();
			inp_assign(sc_conf, 1);
			if (inp_bc->card_offl)
				printf("not all Card online                %04X\n", inp_bc->card_offl);

			printf("online Input Cards                 %04X\n", inp_bc->card_onl);
			if (inp_bc->ro_data)
				printf("hitdata                            %04X     <===>\n", inp_bc->ro_data);
			if (inp_bc->any_error)
				printf("error flags                        %04X     <===>\n", inp_bc->any_error);
			printf("Master Reset, assign input Bus          ..M/b\n");
			if (inp_conf) {
printf("input unit (addr %2u)               %4u ..u\n", inp_conf->slv_addr, sc_conf->slv_sel);
printf("ident/serial                   %04X/%3d\n", inp_unit->ident, inp_unit->serial);
				if ((inp_conf->type != GPXTDC) && (inp_conf->type != GPXTDCTST)) {
					printf("this is not a GPX TDC module!!!\n");
					return CE_NO_INPUT;
				}

				gpxnum=(inp_conf->type == GPXTDC) ? 8 : 1;
printf("Control                            %04X ..0 ", tmp=inp_unit->cr);
				i=16; hgh=0;
				while (i--)
					if (tmp &((u_long)0x1 <<i)) {
						if (hgh) highvideo(); else lowvideo();
						hgh=!hgh; cprintf("%s", inpcr_txt[i]);
					}
				lowvideo(); cprintf("\r\n");

printf("Status                             %04X ..1 ", tmp=inp_unit->sr);
				i=16; hgh=0;
				while (i--)
					if (tmp &((u_long)0x1 <<i)) {
						if (hgh) highvideo(); else lowvideo();
						hgh=!hgh; cprintf("%s", inpsr_txt[i]);
					}
				lowvideo(); cprintf("\r\n");

printf("Trigger latancy                    %04X ..2 %7.1fns\n",
					inp_unit->trg_lat, (inp_unit->trg_lat*RESOLUTION)/1000.);
printf("Trigger window                     %04X ..3  %6.1fns\n",
					inp_unit->trg_win, (inp_unit->trg_win*RESOLUTION)/1000.);
printf("F1 range                           %04X ..4\n",
					inp_unit->f1_range);
printf("GPX chip number                      %2d ..c\n", inp_conf->slv_data[12]);
printf("GPX sel/addr (b7=common)             %02X ..5\n", inp_unit->gpx_seladdr);
printf("GPX data / all data             %07lX ..6/G\n", inp_unit->gpx_data);
printf("GPX int/err flags                  %04X\n", inp_unit->gpx_int_err);
printf("GPX FIFO empty flags               %04X\n", inp_unit->gpx_emptfl);

printf("TestP(8), F1syn(4), F1rst(2), Mrst(1) %X ..7\n", inp_conf->slv_data[7]);
printf("read Event/direct, BrstTst, TstPse, tst  ..e/E/B/T/t\n");
printf("JTAG, Setup, Adjust, GPX Mreset, DACrmp ..j/s/a/m/d\n");
				if (BUS_ERROR) CLR_PROT_ERROR();
			}

			printf("                                          %c ", config.i3_menu);
			while (1) {
				key=toupper(rkey=getch());
				if ((key == CTL_C) || (key == ESC)) return CE_MENU;

				break;
			}
		} else {
			hd=1; printf("select> ");
			while (1) {
				key=toupper(rkey=getch());
				if (key == CTL_C) return CE_MENU;

				break;
			}

			if (key == ' ') key=toupper(rkey=config.i3_menu);

			if ((key == ESC) || (key == CR)) continue;
		}

		use_default=(key == TAB);
		if (use_default || (key == CR)) key=toupper(rkey=config.i3_menu);
		if (key > ' ') printf("%c", rkey);

		Writeln();
		if (rkey == 'M') {
			inp_bc->ctrl=MRESET;
			continue;
		}

		if (rkey == 'b') {
			inp_assign(sc_conf, 0);
			continue;
		}

		if (rkey == 'U') {
			sc_assign(2);
			continue;
		}

		if (!inp_conf) continue;

		if (rkey == 'u') {
			if ((ret=inp_assign(sc_conf, 2)) != 0) continue;
			if ((inp_conf->type != GPXTDC) && (inp_conf->type != GPXTDCTST)) {
				printf("this is not a GPX TDC module!!!\n");
				return CE_NO_INPUT;
			}

			gpxnum=(inp_conf->type == GPXTDC) ? 8 : 1;
			continue;
		}
//
//---------------------------
//
		if (key == 0) {
			key=getch();
			continue;
		}

		if (key == 'J') {
			JTAG_menu(jt_inp_putcsr, jt_inp_getcsr, jt_inp_getdata, &config.jtag_syslave, 0);
			continue;
		}

		if (key == 'D') {
			u_int	d=0;

			while (1) {
				inp_unit->gpx_dac=d; d++;
				while (((gtmp=inp_unit->sr) &2) && !(tmp=BUS_ERROR));

				if (tmp || kbhit()) break;
			}

			continue;
		}

		if (key == 'R') {
			u_long	wr,rd;
			u_long	dat;
			u_long	lp;

			inp_unit->cr=0;
			wr=0; rd=0;
			for (i=0; i < 8; i++) {
				inp_unit->gpx_seladdr=(i<<4)|1;
				inp_unit->gpx_data=wr; wr +=0x3003003L;
			}

			lp=0;
			while (1) {
				for (i=0; i < 8; i++) {
					inp_unit->gpx_seladdr=(i<<4)|1;
					dat=inp_unit->gpx_data;
					if (dat != (rd&0xFFFFFFFL)) {
						printf("\r%8lu: gpx %u, is:%07lX ex:%07lX\n", lp, i, dat, rd&0xFFFFFFFL);
						break;
					}

					rd +=0x3003003L;
					inp_unit->gpx_data=wr; wr +=0x3003003L;
				}

				if (i != 8) break;

				lp++;
				if (!(lp &0x3FFF)) {
					printf("\r%8lu", lp);

					if (kbhit()) { Writeln(); break; }
				}
			}

			continue;
		}

		if (rkey == 'T') {
			while (!kbhit()) {
				inp_unit->ctrl=TESTPULSE;
				delay(1);
			}

			continue;
		}

		if (key == 'X') {
			printf("31.. 0 %08lX ", cha_ena[0]);
			cha_ena[0] =Read_Hexa(cha_ena[0], -1);
			if (errno) continue;

			printf("63..16 %08lX ", cha_ena[1]);
			cha_ena[1] =Read_Hexa(cha_ena[1], -1);
			if (errno) continue;

			for (i=0; i < 8; i++) {
				inp_unit->gpx_seladdr=0x10*i;
				inp_unit->gpx_data=0x481 |(( (cha_ena[i/4] >>((i&3)*8)) &0xFF) <<11);
			}

			continue;
		}

		if (key == 'A') {
			hd=0;
			Writeln();
			printf("GPX (-1 alle)  %2i ", g_gpx);
			g_gpx=(int)Read_Deci(g_gpx, -1);
			if (errno) continue;

			if (g_gpx >= 8) g_gpx=-1;
			if (g_gpx < 0) {
				for (i=0; i < gpxnum; i++)
					if (((ret=gpx_resadjust(i)) != 0) || kbhit()) break;

				continue;
			}

			ret=gpx_resadjust(g_gpx);
			continue;
		}

		if (rkey == 'B') {
			config.i3_menu=rkey;
			hd=0;
			Writeln();
			ret=gpx_burst_test();
			continue;
		}

		if (rkey == 't') {
			config.i3_menu=rkey;
			hd=0;
			Writeln();
			ret=gpx_test();
			continue;
		}

		if ((rkey == 'm') || (rkey == 'e')) {
			u_long	ltmp;

			inp_unit->gpx_seladdr=4;
			ltmp=inp_unit->gpx_data;
			inp_unit->gpx_seladdr=0x80 |4;
			inp_unit->gpx_data=ltmp |0x0400000L;
#ifndef PBUS
			if ((u_char)ltmp == 0xFF) {
				sc_conf->sc_unit->cr=MCR_NO_F1STRT;
				sc_conf->sc_unit->ctrl=F1START;
			} else sc_conf->sc_unit->cr=0;
#endif
			if (rkey == 'm') continue;

			hd=0;
			config.i3_menu=rkey;
			tmp=inp_unit->cr;
			inp_unit->cr=0;
			inp_unit->cr=tmp;
			if ((tmp &0x3) == 0) {
				u_int		cofs;
				u_long	dat,xdat;
				u_long	ldat=0;
				u_long	diff;
				u_int		cnt,lcnt=0;
				int		burst=0,lburst=-1;
				u_char	err=0;

				gpx_range=(long)(6400000.0/G_RESOLUTION +0.5);
				printf("GPX range: 0x%05lX(%1.1fns)\n", gpx_range, gpx_range*G_RESOLUTIONNS);
				i=0; ux=0;
				while (1) {
					if (!(++i&0x0FFF)) {
						if (err || kbhit()) break;

						if ((tmp=inp_unit->gpx_int_err) &GPX_ERRFL) {
							inp_unit->gpx_seladdr=12;
							printf("error %07lX\n", inp_unit->gpx_data);
							if (WaitKey(0)) break;
						}

						if (lburst >= 0) {
							if (lburst != burst) lburst=burst;
							else {
								lburst=-1; printf("burst= %04X(%u)\n", burst, burst);
							}
						}
					}

					if ((tmp=inp_unit->gpx_int_err) &GPX_ERRFL) {
						inp_unit->gpx_seladdr=12; dat=inp_unit->gpx_data;
						printf("GPX error reg12:%07lX    \n", dat);
						break;
					}

					tmp=inp_unit->gpx_int_err;
					if ((tmp &0x3) != 0x3) {
						if (!(tmp &0x1)) {
							cofs=0; inp_unit->gpx_seladdr=8;
						} else {
							cofs=4; inp_unit->gpx_seladdr=9;
						}

						dat=inp_unit->gpx_data;
						xdat=dat &0x1FFFFL;
//if ((xdat < 0x0200) || (xdat >= 0x11000L)) err=1;
						cnt=(u_int)(dat >>18) &0xFF;
						printf("%04X %07lX => %u/%02X/%u/%05lX",
								 ux++, dat,
								 cofs+(u_int)(dat >>26), cnt,
								 (u_int)(dat >>17) &0x01, xdat);
						printf("  %8.2f", xdat*G_RESOLUTIONNS);
						if (lburst < 0) {
							lburst=0; burst=1;
						} else {
							if (cnt == lcnt) {
								printf("  %8.2f", (diff=(xdat-ldat))*G_RESOLUTIONNS);
								if (diff > 0x500) err=1;
							} else {
								if (((cnt-lcnt) &0xFF) == 1) {
									printf("  %8.2f *", (diff=(gpx_range+xdat-ldat))*G_RESOLUTIONNS);
									if (diff > 0x500) err=1;
								} else {
									printf("          *");
									err=1;
								}
							}

							burst++;
						}

						Writeln();
						ldat=xdat; lcnt=cnt;
					}
				}

				inp_unit->gpx_seladdr=0;
				continue;
			}

			config.daq_mode=EVF_SORT;
			inp_mode(IM_QUIET|IM_VERTEX);
			while (1) {
				if ((ret=read_event(EVF_NODAC, RO_RAW)) != 0) break;

				printf("%04X Data read\n", rout_len);
				for (i=0; (i < rout_len/4) && (i < 16);) {
					if (!(i %4)) printf("%04X:", i*4);
					printf(" %03X.%06lX",
							 (u_int)(dmabuf[0][i]>>22), dmabuf[0][i]&0x3FFFFFL);
					if (!(++i %4)) printf("\n");
					if (!(i %320)) if (WaitKey(0)) break;
				}

				if (i %4) printf("\n");
			}

			hd=0;
			continue;
		}

		if (rkey == 'E') {
			u_long	dat;

			config.i3_menu=rkey;
			tmp=inp_unit->cr;
			inp_unit->cr=0;
			inp_unit->cr=tmp;
			hd=0; ux=0;
			while (1) {
				if (!(inp_bc->ro_data &(1<<inp_conf->slv_addr))) {
					if (!++ux && kbhit()) break;

					continue;
				}

            i=0; 
				while (inp_bc->ro_data &(1<<inp_conf->slv_addr)) {
					dat=inp_unit->ro_data;
					if (!(inp_bc->ro_data &(1<<inp_conf->slv_addr))) {
						if (i &0x3) { Writeln(); i=0; }

						printf("\nillegal word count\n"); break;
					}

					dat |=((u_long)inp_unit->ro_data <<16);
					printf(" %X/%02X/%X/%X/%05lX",
							 (u_int)(dat>>28), (u_int)(dat>>22) &0x3F,	// channel
							 (u_int)(dat>>18) &0xF,
							 (u_int)(dat>>17) &0x01, dat &0x1FFFFL);
					i++;
					if (!(i &0x3)) Writeln();
				}

				if (i &0x3) Writeln();

				if (kbhit()) break;
			}

			continue;
		}

		if (rkey == 'g') {
			config.i3_menu=rkey;
			ret=gpx_regdiag(config.inp_gpx_reg);
			continue;
		}

		if (rkey == 'G') {
			u_int	gpx;

			printf("Reg/GPX    7       6       5       4        3       2       1       0\n");
			for (i=0; i < 15; i++) {
				if ((i == 8) || (i == 9) || (i == 13)) continue;

				printf("%2u: ", i);
				for (gpx=gpxnum; gpx;) {
					gpx--;
					if (gpx == 3) printf(" ");
					inp_unit->gpx_seladdr=(gpx <<4) |i;
					printf(" %07lX", inp_unit->gpx_data);
				}

				Writeln();
			}

			config.i3_menu=rkey;
			hd=0;
			continue;
		}

		if (rkey == 's') {
			ret=general_setup(-3, sc_conf->slv_sel);	// nur Inputkarte
			continue;
		}

		if (rkey == 'S') {
			u_int		addr=inp_conf->slv_data[12] <<4;
			u_long	reg4=0x20000FFL;

			inp_unit->ctrl=PURES;
			inp_unit->gpx_seladdr=addr |2;
			inp_unit->gpx_data=0x0000002L;
			inp_unit->gpx_seladdr=addr |0;
			inp_unit->gpx_data=0xFFFFC81L;
			inp_unit->gpx_seladdr=addr |7;
			inp_unit->gpx_data=0x0001FB0L;
			inp_unit->gpx_seladdr=addr |4;
			inp_unit->gpx_data=reg4;
			inp_unit->gpx_seladdr=addr |11;
			inp_unit->gpx_data=0x7FF0000L;
			inp_unit->gpx_seladdr=addr |12;
			inp_unit->gpx_data=0x4000000L;
			inp_unit->gpx_seladdr=addr |4;
			inp_unit->gpx_data=reg4 |0x0400000L;

			inp_unit->gpx_seladdr=addr |12;
			i=0;
			while (++i && ((reg4=inp_unit->gpx_data) &0x0400));

			if (reg4 &0x400) {
				printf("no PLL Lock\n");
				continue;
			}

			printf("i=%u\n", i);
#ifdef PBUS
			lvd_regs->lvd_scan_win=0x04040L;
			((u_short*)lvd_bus)[1]=0;
			lvd_regs->lvd_scan_win=0x14040L;
#else
			sc_conf->sc_unit->cr=MCR_NO_F1STRT;
			sc_conf->sc_unit->ctrl=F1START;
#endif
			continue;
		}

		ux=key-'0';
		if (ux > 9) ux=ux-7;

		if ((ux > 8) && (ux != 12)) continue;

		config.i3_menu=rkey;
		if (ux == 6) {
			printf("Value  %08lX ", inp_conf->slv_xdata.l[0]);
			inp_conf->slv_xdata.l[0]=Read_Hexa(inp_conf->slv_xdata.l[0], -1);
		} else {
			if (ux == 12) {
				printf("Value  %5u ", inp_conf->slv_data[ux]);
				inp_conf->slv_data[ux]=(u_int)Read_Deci(inp_conf->slv_data[ux], -1);
			} else {
				printf("Value  %04X ", inp_conf->slv_data[ux]);
				inp_conf->slv_data[ux]=(u_int)Read_Hexa(inp_conf->slv_data[ux], -1);
			}
		}

		if (errno) continue;

		switch (ux) {

case 0:
			inp_unit->cr=inp_conf->slv_data[ux]; break;

case 2:
			inp_unit->trg_lat=inp_conf->slv_data[ux]; break;

case 3:
			inp_unit->trg_win=inp_conf->slv_data[ux]; break;

case 4:
			inp_unit->f1_range=inp_conf->slv_data[ux]; break;

case 5:
			inp_unit->gpx_seladdr=inp_conf->slv_data[ux]; break;

case 6:
			inp_unit->gpx_data=inp_conf->slv_xdata.l[0]; break;

case 7:
			if (inp_conf->slv_data[ux] &MRESET) {
				inp_bc->ctrl=MRESET;
				break;
			}

			inp_unit->ctrl=inp_conf->slv_data[ux]; break;

		}
	}
}
// --- end ------------------ sync_slave_menu -------------------------------
//
