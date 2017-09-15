// Borland C++ - (C) Copyright 1991, 1992 by Borland International

/*	qdcf.c -- 16 channel fast QDC */

#include <stddef.h>		// offsetof()
#include <dos.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>
#include <errno.h>
#include "pci.h"
#include "daq.h"

extern F1TDC_BC		*inp_bc;
extern F1TDC_RG		*inp_unit;

static u_int	gtmp;

//===========================================================================
//										tdcf1_menu                                    =
//===========================================================================
//
#ifdef SC_M
//------------------------------ rd_eventdata -------------------------------
//
static int rd_eventdata(void)
{
	u_long	sz=dmapages*(u_long)SZ_PAGE;
	u_long	lx;
	u_int		i,l;
	u_int		c=0;
	u_int		ln=0;

static u_long	dblen=4;

	gtmp=inp_bc->ro_data;
	if (!(gtmp &(1<<inp_conf->slv_addr))) {
		printf("Data FIFO empty\n");
		return CE_PROMPT;
	}

	if (send_dmd_eot()) return CEN_PROMPT;

	printf("DMA buffer length (max %05lX) %05lX ", sz, dblen);
	dblen=Read_Hexa(dblen, -1) &~0x3;

	if (errno) return CE_MENU;

	if (dblen < sz) sz=dblen;
	else dblen=sz;

	if (!sz) return CE_MENU;

	gigal_regs->d0_bc_adr =0;	// sonst PLX kein EOT
	gigal_regs->d0_bc_len =0;
	setup_dma(-1, 0, 0, dmapages*(u_long)SZ_PAGE, 0, 0, DMA_L2PCI);
	gigal_regs->t_hdr =HW_BE0011 |HW_L_DMD |HW_R_BUS |HW_EOT |HW_FIFO |HW_BT |THW_SO_DA;
	gigal_regs->t_ad  =(u_int)&inp_unit->ro_data;

	gigal_regs->sr     =gigal_regs->sr;
	gigal_regs->d0_bc  =0;
	plx_regs->dma[0].dmadpr=dmadesc_phy[0] |0x9;
	plx_regs->dmacsr[0]=0x3;						// start
	gigal_regs->t_da   =sz;
	i=0;
	while (1) {
		delay(1);
		if ((plx_regs->dmacsr[0] &0x10)) break;

		if (++i >= 1000) {
			printf("DMA0 not terminated CSR=%02X\n", plx_regs->dmacsr[0]);
			protocol_error(-1, "1");
			printf("len: %03X\n", (u_int)gigal_regs->d0_bc);
			return CE_PROMPT;
		}
	}

	if (protocol_error(-1, "ro")) return CE_PROMPT;

	lx =gigal_regs->d0_bc;
	if (lx > sz) {
		printf("extra words received is/max %04lX/%04lX\n", lx, sz);
		if (WaitKey(0)) return CE_PROMPT;
	}

	printf("readout length =0x%05lX\n", lx);
	l=(u_int)(lx >>2); c=0;
	for (i=0; i < l; i++) {
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

	return CE_PROMPT;
}
#else
static int rd_eventdata(void)
{
	printf("not implemented\n");
	return CEN_PROMPT;
}
#endif
//
//------------------------------ inp_f1_regs ---------------------------------
//
static int inp_f1_regs(void)
{
	int	i;

	if (!inp_conf) return CE_PARAM_ERR;

	if ((i=f1_regdiag(config.inp_f1_reg)) < 0) return CEN_MENU;

	inp_unit->f1_reg[i]=config.inp_f1_reg[i];
	return CE_MENU;
}
//
//------------------------------ xinp_f1reg ---------------------------------
//
int xinp_f1reg(		// 0: ok
	F1TDC_RG	*f1tdc_rg,
	u_int		reg,
	u_int		value)
{
	u_int	i;

	f1tdc_rg->f1_reg[reg]=value; i=0;
	config.inp_f1_reg[reg]=0xFFFF;
	while ((gtmp=f1tdc_rg->f1_state[0]) &WRBUSY) {
		if (BUS_ERROR) return CEN_PROMPT;

		if (++i > 5) return CE_FATAL;	// ca. 2
	}

	if (BUS_ERROR) return CEN_PROMPT;
	config.inp_f1_reg[reg]=value;
delay(1);	//?
	return 0;
}
//
//------------------------------ inp_f1reg ----------------------------------
//
static int inp_f1reg(		// 0: ok
	u_int		reg,
	u_int		value)
{
	return xinp_f1reg(inp_unit, reg, value);
}
//
//------------------------------ f1_resadjust_ ------------------------------
//
#define DV_INC		0x8
#define DV_LOOP	50

static struct {
	u_int	divmin,divmax;
} rrange[8];

static int f1_resadjust_(	// 0: ok
	u_int	f1)
{
	int	ret;
	u_int	ux;
	u_int	hsdiv_min,hsdiv_max;

	if (!inp_conf) return CE_PARAM_ERR;

	inp_unit->f1_addr=f1;
	ux=0;
	while (!((gtmp=inp_unit->f1_state[f1]) &NOINIT)) {
		if ((ret=inp_f1reg(7, 0)) != 0) return ret;

		delay(10);
		if (++ux <= 3) continue;

		printf("be_init should be zero\n");
		return CE_PROMPT;
	}

	if ((ret=inp_f1reg(15, R15_ROSTA)) != 0) return ret;

	if ((ret=inp_f1reg(7, R7_BEINIT)) != 0) return ret;

	if ((gtmp=inp_unit->f1_state[f1]) &NOINIT) {
		printf("no be_init\n"); return CE_PROMPT;
	}

	hsdiv_max=0xF0;
	while (1) {
		inp_f1reg(15, 0);
		inp_f1reg(7, 0);
		ux=0;
		while (1) {
			delay(1);
			gtmp=inp_unit->f1_state[f1]; ux++;
			if (ux < 3) continue;

			if ((gtmp &(NOLOCK|NOINIT)) == (NOLOCK|NOINIT)) break;

			return CE_FATAL;
		}

		inp_f1reg(10, 0x1800|(REFCLKDIV<<8)|hsdiv_max);
		inp_f1reg(15, R15_ROSTA);
		if ((ret=inp_f1reg(7, R7_BEINIT)) != 0) return ret;

		ux=0;
		do {
			delay(1); gtmp=inp_unit->f1_state[f1];
		} while ((gtmp &(NOLOCK|NOINIT)) && (++ux < DV_LOOP));

		if (!(gtmp &NOLOCK)) break;

		hsdiv_max -=DV_INC;
		if (hsdiv_max <= 0x60) return CE_F1_NOLOCK;
	}

	hsdiv_min=hsdiv_max;
	do {
		hsdiv_min -=DV_INC;
		if ((ret=inp_f1reg(10, 0x1800|(REFCLKDIV<<8)|hsdiv_min)) != 0) return ret;

		ux=0;
		do {
			delay(1); gtmp=inp_unit->f1_state[f1];
		} while ((gtmp &NOLOCK) && (++ux < DV_LOOP));	// ca. 10..16

	} while (!(gtmp &NOLOCK) && hsdiv_min);

	hsdiv_min +=DV_INC;
	rrange[f1].divmax=hsdiv_max;
	rrange[f1].divmin=hsdiv_min;
	printf("hsdiv min =0x%02X = %5.1fps\n", hsdiv_min, HSDIV/hsdiv_min);
	printf("hsdiv max =0x%02X = %5.1fps => recommend = %5.1f\n",
			 hsdiv_max, HSDIV/hsdiv_max, HSDIV/(hsdiv_max-8));

	return inp_f1reg(10, 0x1800|(REFCLKDIV<<8)|(hsdiv_max-8));
}
//
//------------------------------ f1_resadjust --------------------------------
//
static int f1_resadjust(void)
{
	int	ret;
	u_int	i;
	u_int	minmax,maxmin;

	if (!inp_conf) return CE_NO_INPUT;

	printf("F1 chip (-1:alle) %2i ", inp_conf->f1_chip);
	inp_conf->f1_chip=(int)Read_Deci(inp_conf->f1_chip, -1);
	if (errno) return CE_MENU;

	CLR_PROT_ERROR();
	i=inp_conf->f1_chip;
	if (inp_conf->f1_chip < 0) {
//		inp_unit->ctrl=PURES;
		delay(10); i=0;
	}

	while (i < 8) {
		printf("\nF1 #%u\n", i);
		if ((ret=f1_resadjust_(i)) != 0) return ret;

		if (inp_conf->f1_chip >= 0) return CE_PROMPT;

		i++;
	}

	if (inp_conf->f1_chip < 0) {
		minmax=0xFF;
		maxmin=0x00;
		for (i=0; i < 8; i++) {
			printf("%d: %02X %02X (%02X) => %6.2fps\n", i,
					 rrange[i].divmin, rrange[i].divmax, rrange[i].divmax -rrange[i].divmin,
					 HSDIV/(rrange[i].divmax -8));
			if (maxmin < rrange[i].divmin) maxmin =rrange[i].divmin;
			if (minmax > rrange[i].divmax) minmax =rrange[i].divmax;
		}

		printf("   %02X %02X (%02X)\n", maxmin, minmax, minmax -maxmin);
		minmax -=8;
		if (minmax > maxmin) {
			printf("max hsdiv =0x%02x => %6.2fps\n", minmax, HSDIV/minmax);
		}
	}

	return CE_PROMPT;
}
//
//------------------------------ spez_test ----------------------------------
//
static int spez_test(void)
{
	int		ret;
	u_int		i,n;
	u_int		hdr;
	u_int		ls,li;

	u_int		cha;
	u_int		chn[4],tchn[4];
	u_char	chg;
	u_char	zchk[2],ochk[2];
	u_int		ofl,tmp;
	u_long	mi,ma;
	u_int		lp;

	printf("\n supply pulses to group of 16 channals of this module\n\n");
	for (i=0; i < 16; i++) inp_offl[i] =1;

	inp_unit->cr=0;
#ifdef PBUS
	inp_mode(IM_VERTEX);
#else
	config.daq_mode=EVF_HANDSH|MCR_DAQRAW;
	config.rout_delay=0;
	inp_mode(IM_QUIET);
	kbatt_msk=0;
#endif

	hdr=rout_hdr ? HDR_LEN : 0;
	inp_unit->cr=ICR_TLIM; chg=1; lp=0;
	while (1) {
		if ((ret=read_event(EVF_NODAC, RO_RAW)) != 0) {
			if ((char)ret == CR) {
				for (n=0; n < 4; chn[n++]=0);
				continue;
			}

			Writeln();
			return ret;
		} else {
			ls=hdr; li=rout_len/4;
			if (chg) {
				chg=0;
				for (n=0; n < 4; chn[n++]=0);
				for (n=0; n < 2; n++) { zchk[n]=0xFF; ochk[n]=0xFF; }
				for (i=ls; i < li; i++) {
					cha=(u_int)(dmabuf[0][i] >>22) &0x3F;
					chn[cha>>4] |=(1 <<(cha&0x0F));

					if (cha &0x08) {
						zchk[1] &=~(u_char)dmabuf[0][i];
						ochk[1] &=(u_char)dmabuf[0][i];
					} else {
						zchk[0] &=~(u_char)dmabuf[0][i];
						ochk[0] &=(u_char)dmabuf[0][i];
					}
				}

				continue;
			}

			for (n=0; n < 4; tchn[n++]=0);
			mi=0xFFFFFFFFL; ma=0;
			for (i=ls; i < li; i++) {
				if ((u_int)dmabuf[0][i] != 0xFFFF) {
					if (mi > (dmabuf[0][i]&0x3FFFFFL)) mi=dmabuf[0][i]&0x3FFFFFL;
					if (ma < (dmabuf[0][i]&0x3FFFFFL)) ma=dmabuf[0][i]&0x3FFFFFL;
				}

				cha=(u_int)(dmabuf[0][i] >>22) &0x3F;
				tchn[cha>>4] |=(1 <<(cha&0x0F));

				if (cha &0x08) {
					zchk[1] &=~(u_char)dmabuf[0][i];
					ochk[1] &=(u_char)dmabuf[0][i];
				} else {
					zchk[0] &=~(u_char)dmabuf[0][i];
					ochk[0] &=(u_char)dmabuf[0][i];
				}
			}

			printf("\r");
			for (n=4; n;) {
				if (n != 4) printf(".");
				n--;
				printf("%04X", tchn[n]);
				if (tchn[n] != chn[n]) chg=1;
			}

			printf(" %02X/%02X", zchk[1], ochk[1]);
			printf(".%02X/%02X", zchk[0], ochk[0]);

			ofl=0;
			for (i=0; i < 8; i++) {
				tmp=inp_unit->f1_state[i];
				if (tmp) {
					ofl |=(1<<i);
					inp_unit->f1_state[i] =tmp;
					if (inp_unit->f1_state[i]) {
						printf("\nF1 state error\n");
						return CEN_PROMPT;
					}
				}
			}

			printf(" %02X ", ofl);

			for (i=4; i;) {
				if (i != 4) printf(".");
				i--;
				printf("%04X", inp_unit->hitofl[i]);
			}

			printf(" %5u", ++lp);
			if ((ma-mi) > 0x10) printf(" %06lX/%06lX", mi, ma);
			inp_unit->hitofl[0]=0;
			if (chg) {
				Writeln();
				for (i=ls; i < li;) {
					printf(" %02X/%06lX", (u_int)(dmabuf[0][i]>>22)&0xFF, dmabuf[0][i]&0x3FFFFFL);
					i++;
				}

				if (i &0x7) Writeln();
				Writeln();
			}
		}
	}
}
//
//------------------------------ spez_test_sy -------------------------------
//
static int spez_test_sy(void)
{
#ifndef PBUS
	int		ret;
	u_int		i,n;
	u_int		hdr;
	u_int		ls,li;

	u_int		cha;
	u_int		chn[4],tchn[4];
	u_char	chg;
	u_int		mi,ma;
	u_int		lp;
#endif

#ifdef PBUS
	printf("only for system with trigger input\n");
	return CEN_PROMPT;
#else
	printf("\n supply pulses to group of 16 channals of any module\n\n");
	for (i=0; i < 16; i++) inp_offl[i] =0;

	if ((ret=inp_mode(0)) != 0) return ret;
	kbatt_msk=0;

	hdr=rout_hdr ? HDR_LEN : 0;
	chg=1; lp=0;
	while (1) {
		if ((ret=read_event(EVF_NODAC, RO_RAW)) != 0) {
			if ((char)ret == CR) {
				for (n=0; n < 4; chn[n++]=0);
				continue;
			}

			Writeln();
			return ret;
		} else {
			ls=hdr; li=rout_len/4;
			if (chg) {
				chg=0;
				for (n=0; n < 4; chn[n++]=0);
				for (i=ls; i < li; i++) {
					cha=(u_int)(dmabuf[0][i] >>22) &0x3F;
					chn[cha>>4] |=(1 <<(cha&0x0F));
				}

				continue;
			}

			for (n=0; n < 4; tchn[n++]=0);
			mi=0xFFFF; ma=0;
			for (i=ls; i < li; i++) {
				if ((u_int)dmabuf[0][i] != 0xFFFF) {
					if (mi > (u_int)dmabuf[0][i]) mi=(u_int)dmabuf[0][i];
					if (ma < (u_int)dmabuf[0][i]) ma=(u_int)dmabuf[0][i];
				}

				cha=(u_int)(dmabuf[0][i] >>22) &0x3F;
				tchn[cha>>4] |=(1 <<(cha&0x0F));
			}

			printf("\r");
			for (n=4; n;) {
				if (n != 4) printf(".");
				n--;
				printf("%04X", tchn[n]);
				if (tchn[n] != chn[n]) chg=1;
			}

			printf(" %5u", ++lp);
			if ((ma-mi) > 0x10) printf(" %04X/%04X|%04X", mi, ma, ma-mi);
			if (chg) {
				Writeln();
				for (i=ls; i < li;) {
					printf(" %02X/%06lX", (u_int)(dmabuf[0][i]>>22)&0xFF, dmabuf[0][i]&0x3FFFFFL);
					i++;
				}

				if (i &0x7) Writeln();
				Writeln();
			}
		}
	}
#endif
}
//
//===========================================================================
//										tdcf1_menu                                    =
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
	inp_unit->jtag_csr=val;
	inp_unit->ident=0;			// nur delay
}
//
const char *inpcr_txt[] ={
	"TMt", "Raw", "ext", "?", "?", "?", "?", "?",
	"?",   "?",   "?",   "?", "?", "?", "?", "?"
};
//
const char *inpsr_txt[] ={
	"nDw", "?", "?", "?", "?", "?", "?", "?",
	"?",   "?", "?", "?", "?", "?", "?", "?"
};
//
const char *f1sr_txt[] ={
	"Bsy", "NoIn", "NoLo", "Pofl", "Hofl", "Oofl", "Iofl", "Serr",
	"?",    "?",   "?",    "?", "?",    "?",    "?",    "?"
};
//
int tdcf1_menu(void)
{
	int		ret=0;
	u_int		i;
	u_int		mx;
	u_int		tmp;
	char		hgh;
	char		rkey,key;
	u_int		ux;
	char		hd=1;
	char		bc;

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

		errno=0; ret=0;
		if (hd) {
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
printf("ident/serial                   %04X/%3u     .x => broadcast\n", mx=inp_unit->ident, inp_unit->serial);
				if (inp_conf->type != F1TDC) {
					printf("this is'nt a F1 TDC module!!!\n");
					return CE_NO_INPUT;
				}
			}

			if (inp_conf && (inp_conf->type == F1TDC)) {
printf("Control                            %04X ..0 ", tmp=inp_unit->cr);
				i=16; hgh=0;
				while (i--)
					if (tmp &((u_long)0x1 <<i)) {
						if (hgh) highvideo(); else lowvideo();
						hgh=!hgh; cprintf("%s", inpcr_txt[i]);
					}
				lowvideo(); cprintf("\r\n");

printf("Status Register (sel clr)          %04X ..1 ", mx=inp_unit->sr);
				i=16; hgh=0;
				while (i--)
					if (mx &(0x1 <<i)) {
						if (hgh) highvideo(); else lowvideo();
						hgh=!hgh; cprintf("%s", inpsr_txt[i]);
					}

				lowvideo(); cprintf("\r\n");

printf("Trigger latency                    %04X ..2 %7.1fns\n",
					inp_unit->trg_lat, (inp_unit->trg_lat*RESOLUTION)/1000.);
printf("Trigger window                     %04X ..3  %6.1fns\n",
					inp_unit->trg_win, (inp_unit->trg_win*RESOLUTION)/1000.);
printf("F1 range                           %04X ..4  %6.1fns\n",
					inp_unit->f1_range, (inp_unit->f1_range*RESOLUTION)/1000.);
printf("F1 address                        %5u ..5\n", inp_unit->f1_addr);
printf("F1 status 7..0: ");
					for (mx=0, i=8; i;) {
						i--;
						if (i != 7) printf(",");

						printf("%02X", tmp=inp_unit->f1_state[i]); mx |=tmp;
					}

					printf("     ");
					i=16; hgh=0;
					while (i--)
						if (mx &(0x1 <<i)) {
							if (hgh) highvideo(); else lowvideo();
							hgh=!hgh; cprintf("%s", f1sr_txt[i]);
						}

					lowvideo(); cprintf("\r\n");

printf("F1 HitFIFO overflow %04X.%04X.%04X.%04X ..6\n",
				inp_unit->hitofl[3], inp_unit->hitofl[2],
				inp_unit->hitofl[1], inp_unit->hitofl[0]);
printf("TestP(8), F1syn(4), F1rst(2), Mrst(1) %X ..7\n", inp_conf->slv_data[7]);
printf("next event data/block, period Test pulse..e/E/t\n");
printf("Resolution Adjust, F1 Regs              ..a/f\n");
printf("JTAG, Clear status, Setup, DAC, spz test..j/c/s/d/x\n");
				if (BUS_ERROR) CLR_PROT_ERROR();
			}

			printf("                                          %c ", config.i1_menu);
			bc=0;
			while (1) {
				rkey=getch();
				if ((rkey == CTL_C) || (rkey == ESC)) return CE_MENU;

				if (rkey == '.') {
					if (!bc) { printf("."); bc=1; }
					continue;
				}

				if (rkey == BS) {
					if (bc) { printf("\b \b"); bc=0; }
					continue;
				}

				break;
			}
		} else {
			hd=1; printf("select> ");
			bc=0;
			while (1) {
				rkey=getch();
				if (rkey == CTL_C) return CE_MENU;

				if (rkey == '.') {
					if (!bc) { printf("."); bc=1; }
					continue;
				}

				if (rkey == BS) {
					if (bc) { printf("\b \b"); bc=0; }
					continue;
				}

				break;
			}

			if (rkey == ' ') rkey=config.i1_menu;

			if ((rkey == ESC) || (rkey == CR)) continue;
		}

		use_default=(rkey == TAB);
		if (use_default || (rkey == CR)) rkey=config.i1_menu;
		if (rkey > ' ') printf("%c", rkey);

		key=toupper(rkey);
		Writeln();
		if (key == 'M') {
			inp_bc->ctrl=MRESET;
			continue;
		}

		if (key == 'B') {
			inp_assign(sc_conf, 0);
			continue;
		}

		if (bc && (key == 'U')) {
			sc_assign(2);
			continue;
		}

		if (!inp_conf) continue;

		if (key == 'U') {
			inp_assign(sc_conf, 2);
			continue;
		}

		if (inp_conf->type != F1TDC) continue;
//
//---------------------------
//
		if (key == 0) {
			key=getch();
			continue;
		}

		if (key == 'S') {
			ret=general_setup(-3, sc_conf->slv_sel);	// nur Inputkarte
			continue;
		}

		if (key == 'D') {
			last_key=0;
			ret=inp_setdac(sc_conf, inp_conf, -2, DAC_DIAL);
			continue;
		}

		if (key == 'A') {
			config.i1_menu=rkey;
			ret=f1_resadjust();
			continue;
		}

		if (key == 'T') {
			i=0;
			do {
				inp_bc->ctrl=TESTPULSE;
			} while (++i || !kbhit());

			continue;
		}

		if (key == 'J') {
			JT_CONF	*jtc;

			if ((inp_unit->ident &0xF) == 0) jtc=&config.jtag_input0;
			else jtc=&config.jtag_input;

			JTAG_menu(jt_inp_putcsr, jt_inp_getcsr, jt_inp_getdata, jtc, 0);
			continue;
		}

		if (key == 'F') {
			config.i1_menu=rkey;
			inp_f1_regs();
			continue;
		}

		if (rkey == 'e') {
			u_int	datl,dath;

			hd=0;
			config.i1_menu=rkey;
			if (!((gtmp=inp_bc->ro_data) &(1<<inp_conf->slv_addr))) {
				printf("no data indicated\n"); continue;
			}

			i=0;
			while ((gtmp=inp_bc->ro_data) &(1<<inp_conf->slv_addr)) {
				datl=inp_unit->ro_data; dath=inp_unit->ro_data;
				if TST_BUS_ERROR {
					CLR_BUS_ERROR;
					if (i &0x07) Writeln();

					printf(">>> unexpected bus error <<<\n");
					i=0; break;
				}

				printf(" %02X/%02X%04X", (dath>>6)&0xFF, dath&0x3F, datl);

				i++;
				if (!(i &0x07)) {
//					Writeln();
					if (!(i &0x7F) && WaitKey(0)) break;
				}
			}

			if (i &0x07) Writeln();

			continue;
		}

		if (rkey == 'x') {
			hd=0;
			config.i1_menu=rkey;
			while (1) {
				ret=spez_test();
				if ((ret&0xFE00) == CE_KEY_PRESSED) { ret=CEN_PROMPT; break; }
				printf("\n%3d\n", ret);
				if (WaitKey(0)) break;
			}
			continue;
		}

		if (rkey == 'X') {
			hd=0;
			config.i1_menu=rkey;
			while (1) {
				ret=spez_test_sy();
				if ((ret&0xFE00) == CE_KEY_PRESSED) { ret=CEN_PROMPT; break; }
				printf("\n%3d\n", ret);
				if (WaitKey(0)) break;
			}
			continue;
		}

		if (key == 'E') {
			hd=0;
			config.i1_menu=rkey;

			rd_eventdata();
			continue;
		}

		if (key == 'C') {
			inp_unit->sr =inp_unit->sr;
			inp_unit->hitofl[0]=0;			// loescht alle
			for (i=0; i < 8; i++) {
				tmp=inp_unit->f1_state[i];
				inp_unit->f1_state[i] =tmp;
			}

			continue;
		}

		if (key == '6') {
			if (bc) inp_bc->hitofl=0;
			else inp_unit->hitofl[0]=0;

			continue;
		}

		ux=key-'0';
		if (ux > 9) ux=ux-7;

		if ((ux > 7) || (ux == 6)) continue;

		config.i1_menu=rkey;
		if (ux == 4) {
			printf("Value  %5u ", inp_conf->slv_data[ux]);
			inp_conf->slv_data[ux]=(u_int)Read_Deci(inp_conf->slv_data[ux], -1);
		} else {
			printf("Value  %04X ", inp_conf->slv_data[ux]);
			inp_conf->slv_data[ux]=(u_int)Read_Hexa(inp_conf->slv_data[ux], -1);
		}

		if (errno) continue;

		if (bc) {
			switch (ux) {

	case 0:
				inp_bc->cr=inp_conf->slv_data[ux]; break;

	case 1:
				inp_bc->f1_error=inp_conf->slv_data[ux]; break;

	case 2:
				inp_bc->trg_lat=inp_conf->slv_data[ux]; break;

	case 3:
				inp_bc->trg_win=inp_conf->slv_data[ux]; break;

	case 4:
				inp_bc->f1_range=inp_conf->slv_data[ux]; break;

	case 5:
				inp_bc->f1_addr=inp_conf->slv_data[ux]; break;

	case 7:
				inp_bc->ctrl=inp_conf->slv_data[ux]; break;

			}

			continue;
		}

		switch (ux) {

case 0:
			inp_unit->cr=inp_conf->slv_data[ux]; break;

case 1:
			inp_unit->sr=inp_conf->slv_data[ux]; break;

case 2:
			inp_unit->trg_lat=inp_conf->slv_data[ux]; break;

case 3:
			inp_unit->trg_win=inp_conf->slv_data[ux]; break;

case 4:
			inp_unit->f1_range=inp_conf->slv_data[ux]; break;

case 5:
			inp_unit->f1_addr=inp_conf->slv_data[ux]; break;

case 7:
			if (inp_conf->slv_data[ux] &MRESET) {
				inp_bc->ctrl=MRESET;
				break;
			}

			inp_unit->ctrl=inp_conf->slv_data[ux]; break;

		}
	}
}
// --- end ------------------ tdcf1_menu ------------------------------------
//
