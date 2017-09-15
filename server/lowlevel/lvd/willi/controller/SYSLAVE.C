// Borland C++ - (C) Copyright 1991, 1992 by Borland International

/*	syslave.c -- general functions  */

#include <stddef.h>		// offsetof()
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>
#include <errno.h>
#include "pci.h"
#include "daq.h"

extern SYSLV_BC		*inp_bc;
extern SYSLV_RG		*inp_unit;

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
#define SYS_SRAM_LEN	0x00010000L		// SRAM word
#define _SYSTEST_INC	0xFF01

static int sram_test(void)
{
	u_short	dout,din;
	u_long	x;
	u_long	lp,lm;
	u_long	i;
	u_long	strt,end;

	printf("start address  %06lX ", config.sytst_strt);
	config.sytst_strt=Read_Hexa(config.sytst_strt, -1);
	if (errno) return CEN_MENU;

	printf("end address    %06lX ", config.sytst_end);
	config.sytst_end=Read_Hexa(config.sytst_end, -1);
	if (errno) return CEN_MENU;

	if (config.sytst_end > SYS_SRAM_LEN) config.sytst_end=SYS_SRAM_LEN;
	strt=config.sytst_strt;
	end =config.sytst_end;
	if (end == 0) end=SYS_SRAM_LEN;

	if (end <= strt) {
		printf("end offset must be higher than start\n");
		return CE_PROMPT;
	}

	end -=strt;
	lp=0;
	lm=0x1000>>nbits(end >>5);
	if (lm) lm--;
	dout=0; din=0;
	inp_unit->sram_addr=(u_short)strt;
	for (i=0; i < end; i++) {
		inp_unit->sram_data=dout;
		dout +=_SYSTEST_INC;
		if ((u_short)i == 0xFFFF) dout +=_SYSTEST_INC;
	}

	while (1) {
		inp_unit->sram_addr=(u_short)strt;
		for (i=0; i < end; i++) {
			x=inp_unit->sram_data;
			if (x != din) {
				printf("\r%10lu: error addr=%06lX, exp=%04X, is=%04X\n",
						 lp, strt +i, din, x);
				return CE_PROMPT;
			}

			din +=_SYSTEST_INC;
			if ((u_short)i == 0xFFFF) din +=_SYSTEST_INC;
		}

		inp_unit->sram_addr=(u_short)strt;
		for (i=0; i < end; i++) {
			inp_unit->sram_data=dout;
			dout +=_SYSTEST_INC;
			if ((u_short)i == 0xFFFF) dout +=_SYSTEST_INC;
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
#define _BLEN_			0x10000L
#define _BLENPGS_		((u_int)((_BLEN_+SZ_PAGE-1)/SZ_PAGE))
#define _BTCNT_		((2*SYS_SRAM_LEN)/_BLEN_)

#ifdef SC_M
static int sram_dtest(void)
{
	u_int		blk;
	u_long	dout,din;
	u_long	x,y;
	u_long	lp;
	u_long	u;
	u_int		i;
	u_char	mode;

	if (dmapages < 16) {
		printf("it will use min 16 DMA pages\n");
		return CE_NO_DMA_BUF;
	}

	printf("mode (rd:2, wr:1) %3u ", config.sytst_mode);
	config.sytst_mode=(u_int)Read_Deci(config.sytst_mode, 3);
	mode=config.sytst_mode &3;
	if (!mode || errno) return CEN_MENU;

	setup_dma(1, 0, 0, _BLEN_, (u_int)&inp_unit->sram_data, 0, 0);	// for SRAM write
	setup_dma(-1, 0x100, 0, _BLEN_, 0, 0, DMA_L2PCI);							// for SRAM read
	lp=0;
	dout=0; din=0;
	if (!(mode&2)) {
		for (u=0; u < _BLEN_/4; u++) {
			DBUF_L(u) =dout; dout +=_DTEST_INC;
		}
	}

	gigal_regs->d0_bc_adr =0;
	gigal_regs->d0_bc_len =0;
	timemark();
	while (1) {
		if (mode&1) {								// DMA write
			inp_unit->sram_addr=0;
			for (blk=0; blk < _BTCNT_; blk++) {
				if (mode&2) {						// auch DMA read
					for (u=0; u < _BLEN_/4; u++) {
						DBUF_L(u) =dout; dout +=_DTEST_INC;
					}
				}

				dmabuf[0][0]=blk;
				gigal_regs->d_hdr=HW_R_BUS|HW_FIFO|HW_BT|HW_WR;
				gigal_regs->d_ad =0;
				gigal_regs->d_bc =_BLEN_;
#if (_BLEN_ > 0x1000L)
				plx_regs->dma[1].dmadpr=dmadesc_phy[0] |0x9;
#endif
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

				if (protocol_error(-1, 0)) return CEN_PROMPT;
				if (!lp && (pciconf.errlog == 2)) printf("wr wait loops =%u\n", i);
			}
		}

		if (mode&2) {
			inp_unit->sram_addr=0;
			for (blk=0; blk < _BTCNT_; blk++) {
				gigal_regs->d0_bc=0;
				gigal_regs->t_hdr=HW_BE|HW_L_DMD|HW_R_BUS|HW_EOT|HW_FIFO|HW_BT|THW_SO_DA;
				gigal_regs->t_ad =(u_int)&inp_unit->sram_data;
#if (_BLEN_ > 0x1000L)
				plx_regs->dma[0].dmadpr=dmadesc_phy[1] |0x9;
#endif
				plx_regs->dmacsr[0]=0x3;								// start DMA
				gigal_regs->t_da =_BLEN_;
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

				if (gigal_regs->d0_bc != _BLEN_) {
					printf("\r%10lu: illegal len %05lX\n", gigal_regs->d0_bc);
					return CE_PROMPT;
				}

				if (!lp && (pciconf.errlog == 2)) printf("rd wait loops =%u\n", i);

				if (mode&1) {
					for (u=0; u < _BLEN_/4; u++) {
						x=DBUF_L(u);
						if (!u) y=blk;
						else y=din;

						if (x != y) {
							printf("\r%10lu: error addr=%05lX, exp=%08lX, is=%08lX\n",
									 lp, ((u_long)blk*_BLEN_) +(u<<2), y, x);
							return CE_PROMPT;
						}

						din +=_DTEST_INC;
					}
				}
			}
		}

		if (!(++lp &0x3F)) {
			printf("\r%10lu", lp);
			if (kbhit()) {
				u_long 	tm;
				double	sec;
				double	mb;

				Writeln();
				tm=ZeitBin();
				if (!tm || !lp) return CE_PROMPT;

				sec=tm /100.0;
				mb =1.0*lp*(2*SYS_SRAM_LEN)/1000000.;
				if (mode == 3) mb *=2.;
				printf("time = %4.2f sec, one loop = %4.2f sec\n", sec, sec/lp);
				printf("Rate = %1.3f MByte per sec\n", mb /sec);
				return CE_PROMPT;
			}
		}
	}
}

#else

static int sram_dtest(void)
{
	u_int		blk;
	u_long	dout,din;
	u_long	x,y;
	u_long	lp;
	u_long	u;
	u_int		i;
	u_char	mode;

	if (dmapages < _BLENPGS_) {
		printf("it will use min 0x%02X DMA pages\n", _BLENPGS_);
		return CE_NO_DMA_BUF;
	}

	printf("mode (rd:2, wr:1) %3u ", config.sytst_mode);
	config.sytst_mode=(u_int)Read_Deci(config.sytst_mode, 3);
	mode=config.sytst_mode &3;
	if (!mode || errno) return CEN_MENU;

	setup_dma(1, 0, 0, _BLEN_, ADDRSPACE1+(u_int)&inp_unit->sram_data, 0, 0);	// for SRAM write
	setup_dma(-1, _BLENPGS_, 0, _BLEN_, 0, 0, DMA_FASTEOT|DMA_L2PCI);							// for SRAM read
	lp=0;
	dout=0; din=0;
	if (!(mode&2)) {
		for (u=0; u < _BLEN_/4; u++) {
			DBUF_L(u) =dout; dout +=0xFFFF0001L;
		}
	}

	timemark();
	while (1) {
		if (mode&1) {								// DMA write
			inp_unit->sram_addr=0;
			for (blk=0; blk < _BTCNT_; blk++) {
				if (mode&2) {						// auch DMA read
					for (u=0; u < _BLEN_/4; u++) {
						DBUF_L(u) =dout; dout +=_DTEST_INC;
					}
				}

				dmabuf[0][0]=blk;
#if (_BLEN_ > 0x1000L)
				plx_regs->dma[1].dmadpr=dmadesc_phy[0] |0x9;
#endif
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
			inp_unit->sram_addr=0;
			for (blk=0; blk < _BTCNT_; blk++) {
#if (_BLEN_ > 0x1000L)
				plx_regs->dma[0].dmadpr=(dmadesc_phy[0] +_BLENPGS_*sizeof(PLX_DMA_DESC)) |0x9;
#endif
				plx_regs->dmacsr[0]=0x3;								// start DMA
				lvd_regs->lvd_scan =0;
				lvd_regs->lvd_blk_max =_BLEN_;
				lvd_regs->lvd_block= (u_int)&inp_unit->sram_data /2;
//				(inp_conf->slv_addr <<8)
//										  +OFFSET(SYSLV_RG, sram_data)/2;
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

//printf("%lu %04lX\n", lp, lvd_regs->sr=lvd_regs->sr);
lvd_regs->sr=lvd_regs->sr;
//if (WaitKey('2')) return 20;
				if (lvd_regs->lvd_blk_cnt != _BLEN_) {
					printf("\r%10lu: illegal len %05lX\n", lp, lvd_regs->lvd_blk_cnt);
					return CE_PROMPT;
				}

				if (!lp && (pciconf.errlog == 2)) printf("rd wait loops =%u\n", i);

				if (mode&1) {
					for (u=0; u < _BLEN_/4; u++) {
						x=DBUF_L(u);
						if (!u) y=blk;
						else y=din;

						if (x != y) {
							printf("\r%10lu: error addr=%05lX, exp=%08lX, is=%08lX\n",
									 lp, ((u_long)blk*_BLEN_) +(u<<2), y, x);
							return CE_PROMPT;
						}

						din +=_DTEST_INC;
					}
				}
			}
		}

		if (!(++lp &0x3F)) {
			printf("\r%10lu", lp);
			if (kbhit()) {
				u_long 	tm;
				double	sec;
				double	mb;

				Writeln();
				tm=ZeitBin();
				if (!tm || !lp) return CE_PROMPT;

				sec=tm /100.0;
				mb =1.0*lp*(2*SYS_SRAM_LEN)/1000000.;
				if (mode == 3) mb *=2.;
				printf("time = %4.2f sec, one loop = %4.2f sec\n", sec, sec/lp);
				printf("Rate = %1.3f MByte per sec\n", mb /sec);
				return CE_PROMPT;
			}
		}
	}
}
#endif
//
//===========================================================================
//										sync_slave_menu                               =
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
static const char *inpcr_txt[] ={
	"?",   "?",   "?",   "?", "?", "?", "?", "?",
	"?",   "?",   "?",   "?", "?", "?", "?", "?"
};
//
static u_long	fill_value;

int sync_slave_menu(void)
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
			if (inp_bc->f1_error)
				printf("error flags                        %04X     <===>\n", inp_bc->f1_error);
			printf("Master Reset, assign input Bus          ..M/B\n");
			if (inp_conf) {
printf("input unit (addr %2u)               %4u ..u\n", inp_conf->slv_addr, sc_conf->slv_sel);
printf("Version                            %04X\n", inp_unit->ident);
				if (inp_conf->type != SYSLAVE) {
					printf("this is no sync slave module!!!\n");
					return CE_NO_INPUT;
				}

printf("Control                            %04X ..2 ", tmp=inp_unit->cr);
				i=16; hgh=0;
				while (i--)
					if (tmp &((u_long)0x1 <<i)) {
						if (hgh) highvideo(); else lowvideo();
						hgh=!hgh; cprintf("%s", inpcr_txt[i]);
					}
				lowvideo(); cprintf("\r\n");

printf("Status                             %04X ..3 ", tmp=inp_unit->sr);
				i=16; hgh=0;
				while (i--)
					if (tmp &((u_long)0x1 <<i)) {
						if (hgh) highvideo(); else lowvideo();
						hgh=!hgh; cprintf("%s", inpcr_txt[i]);
					}
				lowvideo(); cprintf("\r\n");

printf("SRAM address                       %04X ..4/s\n", inp_unit->sram_addr);
printf("                              Mrst(1) %X ..8\n", inp_conf->slv_data[7]);
printf("JTAG, Test/FillSR, Lread, special       ..J/T/t/F/L/l/X\n");
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

		if (rkey == 'U') {
			sc_assign(2);
			if (inp_conf->type != SYSLAVE) {
				printf("this is no sync slave module!!!\n");
				return CE_NO_INPUT;
			}
			continue;
		}

		if (!inp_conf) continue;

		if (rkey == 'u') {
			inp_assign(sc_conf, 2);
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

		if (rkey == 't') {
			ret=sram_test();
			config.i3_menu=rkey;
			continue;
		}

		if (rkey == 'T') {
			ret=sram_dtest();
			config.i3_menu=rkey;
			continue;
		}

		if (key == 'F') {
			printf("Value  %08lX ", fill_value);
			fill_value=Read_Hexa(fill_value, -1);
			if (errno) continue;

			tmp=inp_unit->cr;
			inp_unit->cr=tmp &~0x100;
			inp_unit->sram_addr=0;
			for (i=0; i < SYS_SRAM_LEN/2; i++)
				*(u_long*)&inp_unit->sram_data=fill_value;

			inp_unit->cr=tmp;
			continue;
		}

		if (key == 'X') {
			inp_unit->cr=0;
			inp_unit->sram_addr=0;
			for (i=0; i < SYS_SRAM_LEN/2; i++)
				*(u_long*)&inp_unit->sram_data=0xFFFFFFFFL;

			while (1) {
				inp_unit->cr=0x100;
				tmp=inp_unit->cr;
				inp_unit->cr=0x101;
				tmp=inp_unit->cr;
				if (!++i && kbhit()) break;
			}

			continue;
		}

		if (key == 'S') {
			u_int	adr=inp_unit->sram_addr;
			u_int	cr =inp_unit->cr;

			inp_unit->cr=cr &~0x100;	// clear run
//			inp_unit->sram_addr=inp_conf->slv_data[4];
			tmp=inp_unit->sram_data;
			printf("%04X: %04X ", adr, tmp);
			tmp=(u_int)Read_Hexa(tmp, 0xFFFF);
			if (errno) continue;

//			inp_unit->sram_addr=inp_conf->slv_data[4]++;
			inp_unit->sram_addr=adr;
			inp_unit->sram_data=tmp;
			inp_unit->cr=cr;
			continue;
		}

		if (rkey == 'l') {
			u_long tmp;

			i=0;
			while (1) {
				tmp=*(u_long*)&inp_unit->sram_data;
				if (!++i) {
					printf("\r%08lX", tmp);
					if (kbhit()) break;
				}
			}

			continue;
		}

		if (rkey == 'L') {
			i=0;
			while (1) {
				tmp=inp_unit->sram_data;
				if (!++i) {
					printf("\r%04X", tmp);
					if (kbhit()) break;
				}
			}

			continue;
		}

		if (key == '9') {
			hd=0;
			config.i3_menu=key; i=0;
			while (1) {
				tmp=inp_unit->ro_data;
				if TST_BUS_ERROR {
					CLR_BUS_ERROR;
					if (i &0xF) printf("\n");
					break;
				}

				if (i &1) printf(" %04X", tmp);
				else printf(" %X/%02X", (tmp>>6) &0x0F, tmp &0x3F);

				i++;
				if (!(i &0xFF) && WaitKey(0)) break;
			}

			continue;
		}

		ux=key-'0';
		if (ux > 9) ux=ux-7;

		if ((ux < 1) || (ux > 8)) continue;

		config.i3_menu=key;
		if (ux == 5) {
			printf("Value  %5u ", inp_conf->slv_data[ux]);
			inp_conf->slv_data[ux]=(u_int)Read_Deci(inp_conf->slv_data[ux], -1);
			if (errno) continue;

		} else {
			if (ux != 7) {
				printf("Value  %04X ", inp_conf->slv_data[ux]);
				inp_conf->slv_data[ux]=(u_int)Read_Hexa(inp_conf->slv_data[ux], -1);
				if (errno) continue;
			}
		}

		switch (ux) {

case 2:
			inp_unit->cr=inp_conf->slv_data[ux]; break;

case 4:
			inp_unit->sram_addr=inp_conf->slv_data[ux]; break;

case 8:
			if (inp_conf->slv_data[ux] == MRESET) {
				inp_bc->ctrl=MRESET;
				break;
			}

			inp_unit->ctrl=inp_conf->slv_data[ux]; break;

		}
	}
}
// --- end ------------------ sync_slave_menu -------------------------------
//
