// Borland C++ - (C) Copyright 1991, 1992 by Borland International

/*	jtest.c -- JTAG analyser */

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

extern SQDC_BC		*inp_bc;
extern JTAG_RG		*inp_unit;

#define JS_RESET			0
#define JS_IDLE			8
#define JS_DR_SCAN		1
#define JS_DR_CAPTURE	2
#define JS_DR_SHIFT		3
#define JS_DR_EXIT1		4
#define JS_DR_PAUSE		5
#define JS_DR_EXIT2		6
#define JS_DR_UPDATE		7
#define JS_IR_SCAN		9
#define JS_IR_CAPTURE	10
#define JS_IR_SHIFT		11
#define JS_IR_EXIT1		12
#define JS_IR_PAUSE		13
#define JS_IR_EXIT2		14
#define JS_IR_UPDATE		15

//static u_int	gtmp;

static const char *inpcr_txt[] ={
	"Run", "?", "?", "?", "?", "?", "?", "?",
	"?",   "?", "?", "?", "?", "?", "?", "?"
};
//
static const char *inpsr_txt[] ={
	"Pno", "RDerr", "?",  "?", "?", "?", "?", "?",
	"?",   "?",     "?",  "?", "?", "?", "?", "?"
};
//
int jtest_menu(void)
{
	int		ret=0;
	u_int		i;
	u_int		tmp;
	char		hgh;
	char		rkey,key;
	u_int		ux;
	char		hd=1;
	char		dac=0;

	if (!sc_conf) return CE_PARAM_ERR;

	for (i=0; i < 16; i++) inp_offl[i] =(i != sc_conf->slv_sel);
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
			printf("Master Reset, assign input Bus          ..M/B\n");
			if (inp_conf) {
printf("input unit (addr %2u)               %4u ..u\n", inp_conf->slv_addr, sc_conf->slv_sel);
printf("Version                            %04X\n", inp_unit->ident);
				if (inp_conf->type != JTEST) {
					printf("this is not a JTAG test module!!!\n");
					return CE_NO_INPUT;
				}

//				if (!maxcha) {
//					if (!inp_unit->mean_noise[5]) maxcha=5; else maxcha=16;
//				}

printf("Control                            %04X ..0 ", tmp=inp_unit->cr);
				i=16; hgh=0;
				while (i--)
					if (tmp &((u_long)0x1 <<i)) {
						if (hgh) highvideo(); else lowvideo();
						hgh=!hgh; cprintf("%s", inpcr_txt[i]);
					}
				lowvideo(); cprintf("\r\n");

printf("Status                             %04X", tmp=inp_unit->sr);
				i=16; hgh=0;
				while (i--)
					if (tmp &((u_long)0x1 <<i)) {
						if (hgh) highvideo(); else lowvideo();
						hgh=!hgh; cprintf("%s", inpsr_txt[i]);
					}
				lowvideo(); cprintf("\r\n");

printf("timer                          %08lX\n", inp_unit->timer);
printf("TCK counter                        %04X\n", inp_unit->tck_count);
printf("Access state                       %04X\n", inp_unit->acc_state);
printf("control(Mrst=1) / noise               %X ..7/n\n", inp_conf->slv_xdata.s[7]);
printf("read Event                              ..e\n");
				if (BUS_ERROR) CLR_PROT_ERROR();
			}

			printf("                                          %c ", config.i3_menu);
			while (1) {
				rkey=getch();
				if (rkey == 0) { getch(); continue; }

				if ((rkey == CTL_C) || (rkey == ESC)) return 0;

				if ((rkey == BS) && dac) { dac=0; printf("\b \b"); continue; }

				if (toupper(rkey) == 'D') { printf("%c", rkey); dac=1; continue; }

				if (dac && ((rkey < '0') || (rkey >= '5'))) { dac=0; continue; }
				break;
			}
		} else {
			hd=1; printf("select> ");
			while (1) {
				rkey=getch();
				if (rkey == 0) { getch(); continue; }

				if (rkey == CTL_C) return 0;

				if ((rkey == BS) && dac) { dac=0; printf("\b \b"); continue; }

				if (toupper(rkey) == 'D') { printf("%c", rkey); dac=1; continue; }

				if (dac && ((rkey < '0') || (rkey >= '5'))) { dac=0; continue; }
				break;
			}

			if (rkey == ' ') rkey=config.i3_menu;

			if ((rkey == ESC) || (rkey == CR)) continue;
		}

		use_default=(rkey == TAB);
		if (use_default || (rkey == CR)) rkey=config.i3_menu;
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

		if (!inp_conf) continue;

		if (rkey == 'u') {
//			maxcha=0;
			if ((ret=inp_assign(sc_conf, 2)) != 0) continue;
			if (inp_conf->type != JTEST) {
				printf("this is not a JTAG test module!!!\n");
				return CE_NO_INPUT;
			}
			continue;
		}

		if (inp_conf->type != JTEST) continue;
//
//---------------------------
//
		if (key == 'R') {
			inp_unit->cr=0; inp_unit->cr=1;
			continue;
		}

		if (rkey == 'e') {
			u_int		sw=0;
			u_long	dat;
			u_long	us=0;
			u_long	cnt=0;

			config.i3_menu=rkey;
			Writeln();
			for (i=0; inp_bc->ro_data;) {
				dat=inp_unit->ro_data; i++;
				switch (sw) {
			case 0:
					if (!(dat &0x80000000L)) {
						printf("\n sequence error\n"); break;
					}

					if (!us) us=dat;
					printf(" t%08lX", (dat-us)&0x7FFFFFFFL);
					us=dat;
					sw=1;
					break;

         case 1:
					if (dat &0x80000000L) {
						printf("\n sequence error\n"); break;
					}

					printf("  %X:%06lX", tmp=(u_int)(dat>>27), (dat-cnt)&0x07FFFFFFL);
					cnt=dat;
					if ((tmp == JS_DR_EXIT1) || (tmp == JS_IR_EXIT1)) sw=2;
					else sw=0;

					break;

			default:
					printf("  %08lX", dat); sw++;
					if (sw >= 4) sw=0;
					break;

				}

//				if (!(i %8)) printf("\n");
				if (!(i %320)) if (WaitKey(0)) break;
			}

			if (i %8) printf("\n");
			hd=0;
			continue;
		}

		if (rkey == 'E') {
			u_long	dat;

			config.i3_menu=rkey;
			Writeln(); i=0;
			while (inp_bc->ro_data) {
				dat=inp_unit->ro_data; i++;
				printf("  %08lX", dat);
				if (!(i %320)) if (WaitKey(0)) break;
			}

			if (i %8) printf("\n");
			hd=0;
			continue;
		}

		if (key == 'A') {
			u_int		sw=0;
			u_long	dat,tdat,idat0,idat1,odat0;
			u_int		ln=0;
			u_int		sdat;
			u_long	us=0;
			u_long	cnt=0,lcnt;

			config.i3_menu=rkey;
			use_default=0;
			Writeln();
			while (1) {
				while (inp_bc->ro_data) {
					dat=inp_unit->ro_data;
					switch (sw) {
				case 0:
						if (!(dat &0x80000000L)) {
							printf("\n sequence error\n"); ln++; break;
						}

						tdat=dat; sw=1;
						break;

				case 1:
						sw=0;
						if (dat &0x80000000L) {
							printf("\n sequence error\n"); ln++; break;
						}

						sdat=(u_int)(dat>>27);
						lcnt=(dat-cnt)&0x07FFFFFFL; cnt=dat;
						if (sdat == JS_RESET) {
							printf("RESET\n"); ln++; us=tdat; break;
						}

						if (sdat == JS_IDLE) {
							printf("IDLE    %6lu %7lu us\n",
									 lcnt, (tdat-us)&0x7FFFFFFFL);
							ln++;
							us=tdat; break;
						}

						if ((sdat == JS_DR_EXIT1) || (sdat == JS_IR_EXIT1)) sw=2;
						break;

				case 2:
						idat0=dat;
						sw=3;
						break;

				case 3:
						idat1=dat;
						sw=4;
						break;

				case 4:
						odat0=dat;
						sw=5;
						break;

				case 5:
						sw=0;
						if (sdat == JS_DR_EXIT1) printf("DR      ");
						else printf("IR      ");

						printf("%6lu %08lX:%08lX %08lX:%08lX\n",
								 lcnt, idat1, idat0, dat, odat0);
						ln++;
						break;

					}

					if (ln == 40) {
						ln=0;
						if (WaitKey(0)) break;
					}
				}

				if (kbhit()) break;
			}

			hd=0;
			continue;
		}

		ux=key-'0';
		if (ux > 9) ux=ux-7;

		if (ux > 8) continue;

		config.i3_menu=rkey;
		printf("Value  %04X ", inp_conf->slv_data[ux]);
		inp_conf->slv_data[ux]=(u_int)Read_Hexa(inp_conf->slv_data[ux], -1);

		if (errno) continue;
		switch (ux) {

case 0:
			inp_unit->cr=inp_conf->slv_data[ux]; break;

case 7:
			if (inp_conf->slv_data[ux] == MRESET) {
				inp_bc->ctrl=MRESET;
				break;
			}

		}
	}
}
