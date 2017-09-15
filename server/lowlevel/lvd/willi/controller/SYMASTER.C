// Borland C++ - (C) Copyright 1991, 1992 by Borland International

/*	symaster.c -- general functions  */

#include <stddef.h>		// offsetof()
#include <stdio.h>
#include <dos.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>
#include <errno.h>
#include "pci.h"
#include "daq.h"

extern SYMST_BC		*inp_bc;
extern SYMST_RG		*inp_unit;

static u_int	gtmp;

//--------------------------- chr2hex ---------------------------------------
//
static int chr2hex(char chr)
{
	u_int	hex=chr-'0';

	if (hex > 9) hex-=7;

	if (hex >= 0x10) hex=0x0F;

	return hex;
}
//
//--------------------------- gen_test --------------------------------------
//
#define AUX0	0x0001
#define TCLR	0x0010
#define TAW		0x0040
#define GO		0x8000

#define _Y0		1
#define _Y1		2
#define _Y4		5
#define _Y6		8
#define _Y10	12

static union {
	char		s[18];
	u_short	d[9];
} p_width;

static int gen_test(void)
{
	char		rkey,key;
	u_int		i,u=0;
	u_long	l=0;
	u_int		tmp;
	u_long	trg[4]={0};
	u_int		auxo=0xF;
	char		show=1;
	char		sel=0;
	int		sreg=0;		// selected register
	u_int		creg=GO;

	clrscr();
	gotoxy(51, 1); printf("C  clear counter");
	gotoxy(51, 2); printf("S  select register");
	gotoxy(51, 3); printf("CR terminate select");
	gotoxy(51, 4); printf("A  set AUX output");
	gotoxy(51, 5); printf("T  AUX pulse");

	gotoxy(1, _Y0);
	printf("Register: PW0");
	inp_unit->trg_inh =0;
	inp_unit->sm_cr= 0;
	for (i=0; i < 9; i++) p_width.d[i]=inp_unit->pwidth[i];

	while (1) {
		if (show) {
			show=0;
			gotoxy(1, _Y1);
			printf("%-16s", "");
			gotoxy(1, _Y4);
			for (i=0; i < 4; printf("  %02X", p_width.s[i++]));
			for (i=16; i < 18; i++) printf((p_width.s[i] < 0) ? "  %2i"
																			 : "  %02X", p_width.s[i]);
			Writeln();
			printf("AUX out: 0x%X", auxo);
			creg &=~TAW;
			if (p_width.s[17] >= 0) creg |=TAW;
		}

		inp_unit->sm_cr= GO|TCLR;
		inp_unit->sm_cr= creg;
		inp_unit->sm_cr= creg |auxo;
		inp_unit->sm_cr= creg;
		while (1) {
			l++;
			if (!++u) {
				if (kbhit()) {
					key=toupper(rkey=getch());
					if (!rkey) {
						rkey=getch();
						if (sel == 1) {
							tmp=0;
							if ((rkey == UP) && (sreg < 18)) {
								tmp=1;
								if (++sreg == 4) sreg=16;
							}

							if ((rkey == DOWN) && sreg) {
								tmp=1;
								if (--sreg == 15) sreg=3;
							}

							if (!tmp) continue;

							gotoxy(11, _Y0);
							switch (sreg) {
					case 16:	printf("PWsum"); break;
					case 17:	printf("PWtaw"); break;
					case 18:	printf("PWall"); break;
					default:	printf("PW%u ", sreg); break;
							}

							continue;
						}

						if (sel != 0) continue;

						switch (rkey) {
				case UP:
							if (sreg == 18) {
								for (i=0; i < 18; i++)
									if (p_width.s[i] < 0x1F) { p_width.s[i]++; show=1; }

								break;
							}


							if (p_width.s[sreg] < 0x1F) { p_width.s[sreg]++; show=1; }

							break;

				case DOWN:
							if (sreg == 18) {
								for (i=0; i < 18; i++)
									if (   (p_width.s[i] > 0)
										 || ((i == 17) && !p_width.s[i])) { p_width.s[i]--; show=1; }

								break;
							}

							if (   (p_width.s[sreg] > 0)
								 || ((sreg == 17) && !p_width.s[sreg])) { p_width.s[sreg]--; show=1; }

							break;
						}

						if (!show) continue;

						for (i=0; i < 2; i++)
							inp_unit->pwidth[i]=p_width.d[i] ^0x8080;

						inp_unit->pwidth[8]=p_width.d[8] ^0x8080;
						inp_unit->setpoti=1;
						break;	// continue;
					}

					if (sel == 2) {
						sel=0; show=1;
						auxo=chr2hex(key);
						break;	// continue;
					}

					if (key == 'T') {
						inp_unit->sm_cr= creg |auxo;
						inp_unit->sm_cr= creg;
						continue;
					}

					if (key == 'A') {
						sel=2;
						gotoxy(1, _Y1);
						printf("%-16s", "set AUXout");
						continue;
					}

					if (key == 'C') {
						for (i=0; i < 4; trg[i++]=0);
						break;	// continue;
					}

					if (key == 'S') {
						sel=1;
						gotoxy(1, _Y1);
						printf("%-16s", "select register");
						continue;
					}

					if ((key == CR) || (key == ' ')) {
						sel=0;
						gotoxy(1, _Y1);
						printf("%-16s", "");
						continue;
					}

					gotoxy(1, _Y10);
					return CE_PROMPT;
				}
			}

			if ((tmp=inp_unit->trg_acc) != 0) {
				for (i=0; i < 4; i++)
					if (tmp &(1<<i)) trg[i]++;

				break;
			}

		}

		if (l >= 0x10000L) {
			gotoxy(1, _Y6);
			for (i=0; i < 4; printf(" %07lX", trg[i++]));
			printf("\nTrg Rej: %04X", inp_unit->sm_rej);
			l=0;
		}
	}
}
//
//--------------------------- special_test ----------------------------------
//
static int special_test(void)
{
#if 0
	u_long	t[128];
	u_int		i;

	for (i=0; i < 128; t[i++]=inp_unit->sm_tmc);
	for (i=0; i < 128; i++)
		printf("  %08lX", t[i]);

	return CEN_PROMPT;
#endif

#if 1
	u_long	t0,t1;
	u_int		lp=0;

	t1=inp_unit->sm_tmc;
	while (1) {
		if ((t0=inp_unit->sm_tmc) < t1) {
			printf("%3u %06lX %06lX\n", ++lp, t1, t0);
			if (kbhit()) {
				Writeln();
				return CEN_PROMPT;
			}
		}

		t1=t0;
	}
#endif

#if 0
	u_int	i;

	for (i=0; i < 40; i++) {
		printf("%10.3f\n", 0.1e-6*inp_unit->sm_tmc);
		delay(100);
	}

	return CEN_PROMPT;
#endif

#if 0
	u_int	i;
	u_int	pw=0;

	while (!kbhit()) {
		for (i=0; i < 2; i++)
			inp_unit->pwidth[i]=pw |0x8080;

		inp_unit->pwidth[8]=pw |0x8080;
		inp_unit->setpoti=1;
		pw+=0x0101;
		if ((u_char)pw >= 0x20) pw=0;

		delay(300);
	}

	return 0;
#endif
}
//
//===========================================================================
//										compatibility test                            =
//===========================================================================
//
#define _CVT_MIN		4				// if lower, Subevent can fail

static char* const reg_name[] ={
	"", "", "", "","", "",
	"CR",			// 0C
	"SR",
	"CVT",
	"INH",		// 10
	"INP",
	"ACC",
	"",
	"REJ",
	"",
	"GAP", "",	// 1C
	"", "", "", "", "", "", "", "", "", ""
	"TMC", "",	// 34
	"CVT", "",	// 38
	"TDT", "",	// 3C
	"", "", "", "", "",
	"FCAT",		// 4A
	"EVC", "",	// 4C
	"" };

//--------------------------- _reg_check -------------------------------------
//
static int _reg_check(		// 0: no error
	u_int	csr)
{
	int		ix;
	u_long	x0;

	x0=csr; ix=OFFSET(SYMST_RG, sm_sr);
	if (inp_unit->sm_sr == csr) {
		x0=0; ix=OFFSET(SYMST_RG, sm_cvt);
		if (inp_unit->sm_cvt == 0) {
			ix=OFFSET(SYMST_RG, sm_tdt);
			if (inp_unit->sm_tdt == 0) {
				ix=OFFSET(SYMST_RG, sm_fcat);
				if (inp_unit->sm_fcat == 0) {
					x0=-1; ix=OFFSET(SYMST_RG, sm_evc);
					if (inp_unit->sm_evc == 0xFFFFFFFFL) {
						x0=0; ix=OFFSET(SYMST_RG, sm_rej);
						if (inp_unit->sm_rej == 0) {
							x0=0; ix=OFFSET(SYMST_RG, sm_fclr);
							if (inp_unit->sm_fclr == 0) {
								x0=GAP_NO_GAP; ix=OFFSET(SYMST_RG, sm_gap);
								if ((inp_unit->sm_gap &0xFFFF0000L) == GAP_NO_GAP) {
									return 0;
//									x0=0; ix=OFFSET(SYMST_RG, _IMB4;
//									if (amcc_regs->imb[3] == 0) return 0;
								}
							}
						}
					}
				}
			}
		}
	}

	printf("register %s has not the expected value %08lX!\n",
			 reg_name[ix/2], x0);
	return -1;
}
//
//--------------------------- _test_counter ---------------------------------
//
static int _test_counter(void)
{
	int		ix;
	u_long	x0,x1;
	u_int		mx;
	u_int		ux;
	double	tm;
	char		setcvt;
	u_long	cvt,evc;
	u_long	tdt;
	u_int		csr,csr0;
	u_long	tm0,tmo;
	u_long	ecnt;
	u_long	elim;

	printf("Conversion Time  $%08lX ", config.tst_cvt);
	config.tst_cvt=Read_Hexa(config.tst_cvt, -1);
	if (errno) return 0;

	Writeln();
	if (!pciconf.errlog) printf("Error Detection is turned off\007\n");
	printf("disconnect all AUX.IN\n");
	printf("close the trigger ring bus\n");
	printf("connect any AUX.OUT with any TRIGGER.IN, o.k?\n");
	if (WaitKey(0)) return 0;

	inp_unit->sm_cr  =0;				// Master Reset
	inp_unit->sm_cr  =ST_GO;		// Set GO
	inp_unit->sm_ctrl=RST_TRG; ix=0;
	while (inp_unit->sm_gap &GAP_MORE) {	// Clear GAP FIFO
		ix++;
		if (ix >= 3) {
			printf("error reading Trigger Gap FIFO\n"); return 0;
		}
	}

	Writeln();
	printf("searching 1. zero transition of the time counter\n");
	x1=inp_unit->sm_tmc;
	do {
		if (kbhit()) return 0;

		x0=x1;
		x1=inp_unit->sm_tmc;
	} while (x1 >= x0);

	timemark();
	printf("searching next zero transition of the time counter\n");
	do {
		if (kbhit()) return 0;

		x0=x1;
		x1=inp_unit->sm_tmc;
	} while (x1 >= x0);

	tm0=ZeitBin();
	tm=(1.0*tm0)/100.;
	Writeln();
	printf("time = %1.2f Sec, expected 1.6 Sec\n", tm);
	Writeln();

	tm0=20*tm0*pciconf.ms_count;

	inp_unit->sm_cvt =0;		// Zero Conversion Time
	inp_unit->trg_inh=0xFFF0;
	if (_reg_check(CS_GO_RING)) return CE_PROMPT;

	cvt=_CVT_MIN; elim =0x10000L; setcvt =1;
	if (config.tst_cvt) {
		setcvt =0;
		cvt    =config.tst_cvt >>4; elim =0x10000L;
		while (cvt && (elim > 1)) { cvt >>=1; elim >>=1; }

//		printf("elim =%06lX\n", elim);
		cvt    =config.tst_cvt;
	}

	evc=-1; ecnt=0; csr0=0; mx=0; ux=0;
	while (1) {
		inp_unit->sm_cvt=cvt;
		inp_unit->sm_cr =ST_GO|ST_AUX_MASK;			// set/reset all AUX
		inp_unit->sm_cr =ST_GO;
		tmo=tm0; evc++;
		do {
			tmo--;
			if (!tmo) {
				printf("\r%08lX, no EOC, cvt=$%06lX\n", evc, cvt);
				return CE_PROMPT;
			}

			if (!++ux && kbhit()) {
				printf("\r%08lX\n", evc);
				return CE_PROMPT;
			}

			csr=inp_unit->sm_sr;
		} while (!(csr &CS_EOC));

		x0=inp_unit->sm_tmc;
		inp_unit->sm_ctrl =RST_TRG;			// Reset Trigger
		ix=0;
		while ((gtmp=inp_unit->sm_sr) &CS_TDT_RING) {
			if (++ix > 100) {
				printf("TDT Ring remains setting\n"); return CE_PROMPT;
			}
		}

		x1=inp_unit->sm_tmc;
		tdt=inp_unit->sm_tdt;
		if ((x1-tdt) < 100) mx=0;
		else ++mx;

		if ((x0 < cvt) || (x0 > tdt) || (mx >= 4)) {
			printf("\r%08lX, illegal time values\n", evc);
			printf("cvt=$%06lX, x0=$%08lX, tdt=$%08lX, x1=$%08lX\n",
					 cvt, x0, tdt, x1);
			return CE_PROMPT;
		}

		if (evc != inp_unit->sm_evc) {
			printf("\r%08lX, invalid event counter\n", evc); return CE_PROMPT;
		}

		if (++ecnt >= elim) {
			printf("\r%08lX", evc);
			ecnt=0;
			if (setcvt) {
				cvt <<=1;
				if (elim > 1) elim >>=1;

				if (cvt >= 0x1000000L) {
					cvt=_CVT_MIN; elim =0x10000L;
				}
			}
		}
//
// CSR: TDT_RING, GAP_DATA, EOC, EDTD, GO_RING
//
		if (!csr0) {
			if (!(csr &CS_TRIGGER)) {
				printf("\r%08lX, no trigger, csr:$%08lX\n", evc, csr);
				return CE_PROMPT;
			}

			csr0=csr;
			if ((csr &~CS_TRIGGER) !=
					(CS_TDT_RING |CS_EOC
					 |CS_INH |CS_GO_RING))
				csr0=0xFFFF;
		}

		if ((csr != csr0) && pciconf.errlog) {
			if (csr0 == 0xFFFF)
				csr0=(csr &CS_TRIGGER) |
						(CS_TDT_RING |CS_EOC
						 |CS_INH |CS_GO_RING);

			printf("\r%08lX, invalid csr, is:$%08lX, exp:$%08lX\n",
					 evc, csr, csr0);
			return CE_PROMPT;
		}

		if (!++ux && kbhit()) {
			printf("\r%08lX\n", evc);
			return CE_PROMPT;
		}

	}
}
//
//--------------------------- set_pulse_width --------------------------------
//
void set_pulse_width(
	u_int			pwsel,		// b0-b3: T1-T4
									// b4:    Tm
									// b5:    TATW
	u_int			pwval)		//   0.. 31 set but no save
									// 100..131 dez, set and save for powerup
{
	u_int	i;
	u_int	set;

	printf("set_pulse_width(%02X, %u)\n", pwsel, pwval);
	for (i=0; i < 9; i++) {
		p_width.d[i]=inp_unit->pwidth[i]; set=0;
		if (i < 2) {
			if (pwsel &(1<<(2*i))) { p_width.s[2*i]=pwval; set=0x0080; }

			if (pwsel &(1<<(2*i+1))) { p_width.s[2*i+1]=pwval; set|=0x8000; }
		}

		if (i >= 8) {
			if (pwsel &(1<<(2*(i-6)))) { p_width.s[2*i]=pwval; set=0x0080; }

			if (pwsel &(1<<(2*(i-6)+1))) { p_width.s[2*i+1]=pwval; set|=0x8000; }
		}

		if (set) inp_unit->pwidth[i]=p_width.d[i] |set;
	}

	inp_unit->setpoti=1;

}
//
//--------------------------- _test_first ------------------------------------
//
static int _test_first(void)
{
	int		ix;
	u_int		ux;
	u_long	x0,x1;
	u_long	cvt,lp;
	u_long	lpmsk;
	u_int		auxset;
	u_int		auxclr;
	u_long	tdt;
	u_long	csr,csr0;
	u_long	tm0,tmo;

	printf("disconnect all AUX.IN\n");
	printf("connect AUX.OUT as masked with any TRIGGER.IN, o.k?\n");
	Writeln();
	printf("Mask for AUX.OUT     $%02X ", config.tst_aux);
	config.tst_aux=(u_int)Read_Hexa(config.tst_aux, -1) &0x07;
	if (errno) return 0;

	auxset =((config.tst_aux *ST_AUX0) &ST_AUX_MASK) |ST_GO;
	if (!(auxset &ST_AUX_MASK)) {
		printf("mask is set to 0xF\n"); auxset=ST_AUX_MASK |ST_GO;
	}

	auxclr=auxset &~ST_AUX_MASK;
	printf("Conversion Time  $%06lX ", config.tst_cvt);
	config.tst_cvt=Read_Hexa(config.tst_cvt, -1);
	if (errno) return 0;

	cvt=config.tst_cvt;
	ux=24;
	do ux--;
	while (ux && !(cvt &(1L <<ux)));

	if (ux > 22) ux=22;
	if (ux < 6) ux=6;
	lpmsk=(1L <<(22-ux))-1;

	inp_unit->sm_cr  =0;						// Master Reset
	ix=0;
	while (inp_unit->sm_gap &GAP_MORE) {	// Clear GAP FIFO
		ix++;
		if (ix >= 3) {
			printf("error reading Trigger Gap FIFO\n"); return 0;
		}
	}

	inp_unit->trg_inh=0xFFF0;
	tm0=5000*pciconf.ms_count;
	lp=0; csr0=0; ux=0;
	while (1) {
		inp_unit->sm_cr =0;					// master reset
		inp_unit->sm_cr =ST_GO;				// Set GO
		inp_unit->sm_ctrl=RST_TRG;
		inp_unit->sm_cvt =0;
		x0=CS_GO_RING; ix=OFFSET(SYMST_RG, sm_sr);
		if (inp_unit->sm_sr == (u_short)x0) {
			x0=ST_GO; ix=OFFSET(SYMST_RG, sm_cr);
			if (inp_unit->sm_cr == (u_short)x0) {
				x0=-1; ix=OFFSET(SYMST_RG, sm_evc);
				if (inp_unit->sm_evc == 0xFFFFFFFFL) {
					x0=0; ix=OFFSET(SYMST_RG, sm_cvt);
					if (inp_unit->sm_cvt == 0) {
						ix=OFFSET(SYMST_RG, sm_tdt);
						if (inp_unit->sm_tdt == 0) {
							ix=-1;
//							ix=_IMB4;
//							if (amcc_regs->imb[3] == 0) ix=-1;
						}
					}
				}
			}
		}

		if (ix != -1) {
			printf("\r%10lu, register %s has invalid value %04lX!!\n",
					 lp, reg_name[ix/2], x0);
			return CE_PROMPT;
		}

		inp_unit->sm_cvt =cvt;
		inp_unit->sm_cr  =auxset;
		inp_unit->sm_cr  =auxclr;
		tmo=tm0;
		do {
			tmo--;
			if (!tmo) {
				printf("\r%10lu, no EOC, cvt=$%06lX\n", lp, cvt);
				return CE_PROMPT;
			}

			csr=inp_unit->sm_sr;
		} while (!(csr &CS_EOC));

		x0=inp_unit->sm_tmc;
		inp_unit->sm_ctrl=RST_TRG;
		ix=0;
		while ((gtmp=inp_unit->sm_sr) &CS_TDT_RING)
			if (++ix > 100) {
				printf("TDT Ring remains setting\n"); return CE_PROMPT;
			}

		x1=inp_unit->sm_tmc;
		tdt=inp_unit->sm_tdt;
		if ((x1-tdt) < 100) ux=0;
		else ++ux;

		if ((x0 < cvt) || (x0 > tdt) || (ux >= 4)) {
			printf("\r%10lu, illegal time values\n", lp);
			printf("cvt=$%08lX, x0=$%08lX, tdt=$%08lX, x1=$%08lX\n",
					 cvt, x0, tdt, x1);
			return CE_PROMPT;
		}

		if (inp_unit->sm_evc) {
			printf("\r%10lu, event counter not zero\n", lp); return CE_PROMPT;
		}

		if (!csr0) {
			if (!(csr &CS_TRIGGER)) {
				printf("\r%10lu, no trigger, csr:$%08lX\n", lp, csr);
				return CE_PROMPT;
			}

			csr0=csr;
			if ((csr &~CS_TRIGGER) !=
								(CS_TDT_RING |CS_EOC
								 |CS_INH |CS_GO_RING))
				csr0=0xFFFFFFFFL;
		}

		if ((csr != csr0) ||
			 (inp_unit->sm_sr !=
						(CS_GO_RING))) {
			if (csr0 == 0xFFFFFFFFL)
				csr0=(csr &CS_TRIGGER) |
						(CS_TDT_RING |CS_EOC
						 |CS_INH |CS_GO_RING);

			printf(
			"\r%10lu, invalid csr, is:$%08lX, exp:$%08lX, nowexp:$%08lX\n",
					 lp, csr, csr0, CS_GO_RING);
			return CE_PROMPT;
		}

		if ((x0=inp_unit->sm_gap) &GAP_MORE) {	// Clear GAP FIFO
			printf("\r%10lu, to many Trigger Gap's, csr=$%08lX, gap=$%08lX\n",
					 lp, csr, x0);
			return CE_PROMPT;

		}

		if (!(++lp &lpmsk)) {
			printf("\r%10lu", lp);
			if (kbhit()) {
				Writeln();
				return CE_PROMPT;
			}
		}
	}
}
//
//--------------------------- _real_test -------------------------------------
//
static int _real_test(void)
{
	int		ix;
	u_int		ux;
	long		x0;
	u_int		auxfcl;
	u_int		auxset;
	u_int		auxclr;
	u_long	evc;				// Event Counter
	u_long	tdt;
	u_int		csr;
	long		tm0,tmo;
	u_int		ln;
	u_int		trg[4];
	u_int		tm;
	u_int		ecnt;

	long		gap;
	u_int		g_min,g_max;
	u_long	g_ave;
	u_long	g_cnt;			// GAP counter
	u_int		g_lose;
	u_int		rej;
	u_long	rejcnt;
	u_int		rejovr;

	u_int		wx;
	char		wrk[8];
	char		cx;
	int		sel;
	u_int		pw;

	tm0=500*pciconf.ms_count;
	auxclr=ST_GO;
	printf("Conversion Time       $%06lX ", config.tst_cvt);
	config.tst_cvt=Read_Hexa(config.tst_cvt, -1);
	if (errno) return 0;

	printf("Fast Clear Acceptance   $%04X ", config.tst_fcat);
	config.tst_fcat=(u_int)Read_Hexa(config.tst_fcat, -1);
	if (errno) return 0;

	printf("Trigger Acceptance Time  %4u ", config.tst_tatw);
	config.tst_tatw=(u_int)Read_Deci(config.tst_tatw, 31);
	if (errno) return 0;

	printf("Trigger 4 Utilization    %4u ", config.tst_t4ut);
	config.tst_t4ut=Read_Deci(config.tst_t4ut, 1) != 0;
	if (errno) return 0;

	printf("Mask for AUX.OUT           $%X ", config.tst_aux);
	config.tst_aux=(u_int)Read_Hexa(config.tst_aux, 0xF) &0xF;
	if (errno) return 0;

	printf("AUX.OUT mask for FCLR      $%X ", config.tst_auxfclr);
	config.tst_auxfclr=(u_int)Read_Hexa(config.tst_auxfclr, 0xF) &0xF;
	if (errno) return 0;

	printf("determine statistic      %4u ", config.tst_stcs);
	config.tst_stcs=Read_Deci(config.tst_stcs, 1) != 0;
	if (errno) return 0;

	if (config.tst_t4ut) auxclr |=ST_T4_UTIL;

	if (config.tst_tatw) {
		auxclr |=ST_TAW_ENA;
		config.puwidth[5]=config.tst_tatw;
		set_pulse_width(1 <<5, config.tst_tatw);
	}

	config.tst_tatw=(config.tst_tatw %100) &0x1F;
	if (!pciconf.errlog) printf("Error Detection is turned off\007\n");
	Writeln();

	inp_unit->sm_cr =0;							// Master Reset
	inp_unit->sm_cvt  =config.tst_cvt;
	inp_unit->sm_fcat =config.tst_fcat;
	inp_unit->trg_inh=0xFFF0;
	auxset=auxclr |((config.tst_aux *ST_AUX0) &ST_AUX_MASK);
	auxfcl=auxclr |((config.tst_auxfclr *ST_AUX0) &ST_AUX_MASK);
	inp_unit->sm_cr  =auxclr;						// Set GO
	inp_unit->sm_ctrl=RST_TRG;
	evc=-1; ln=0; wx=0;
	ecnt=0;

	while (1) {
		tmo=tm0; 
		trg[0]=0; trg[1]=0; trg[2]=0; trg[3]=0; tm=0; rejcnt=0; rejovr=0;
		tdt=0; g_min=0xFFFF; g_max=0; g_ave=0; g_cnt=0; g_lose=0;
		if (!ecnt && (auxset &ST_AUX_MASK)) {
			inp_unit->sm_cr=auxset;
			if (auxfcl &ST_AUX_MASK) {
				gtmp=inp_unit->sm_cr;
				inp_unit->sm_cr=auxfcl;
			}
			inp_unit->sm_cr=auxclr;
		}

      ecnt=0;
		do {
			tmo--;
			if ((gtmp=inp_unit->sm_sr) &CS_EOC) {
				evc++; ecnt++;
				tmo--;
				csr=inp_unit->sm_sr;
				if (csr &CS_SI_RING) {
					printf("Subevent Invalid\n"); return CE_PROMPT;
				}

				tmo--;
				if (evc != inp_unit->sm_evc) {
					printf("\ninvalid event counter, exp:%08lX, is:%08lX\n", evc,
							  inp_unit->sm_evc);
					return CE_PROMPT;

				}

				ix=0;
				if (csr &0x1) { trg[0]++; ix++; }
				if (csr &0x2) { trg[1]++; ix++; }
				if (csr &0x4) { trg[2]++; ix++; }
				if (csr &0x8) { trg[3]++; ix++; }
				if (!ix) {
					printf("no event\n"); return CE_PROMPT;
				}

				if (ix > 1) tm++;
				tmo--;
				tdt +=inp_unit->sm_tdt;

				if (config.tst_stcs) {
					tmo--;
					rej=inp_unit->sm_rej;
					if (rej == 0xFFFF) rejovr++;
					rejcnt +=rej;

					if (!config.tst_t4ut || (csr &0x0007)) {	// T3..T1
						gap=-1;
						do {
							tmo--;
							x0=gap; gap=inp_unit->sm_gap;
							if (gap < 0) {
								printf("no gap, $%08lX->$%08lX\n", x0, gap);
								if (pciconf.errlog) return CE_PROMPT;
							}

							g_cnt++;
							if (gap &GAP_OVERRUN) g_lose++;
							ux=(u_int)gap;
							if (g_min > ux) g_min=ux;
							if (g_max < ux) g_max=ux;
							g_ave +=ux;
						} while (gap &GAP_MORE);
					}
				}

				tmo--;
				inp_unit->sm_ctrl =RST_TRG;	// Reset Trigger
				if (auxset &ST_AUX_MASK) {
#if 0
					while ((gtmp=inp_unit->sm_sr) &CS_TDT_RING) {
						tmo--;
						if (tmo < 0) {
							if (kbhit()) {
								printf("TDT Ring remains setting\n"); return CE_PROMPT;
							}
						}
					}
#endif
					tmo--;
					if (!((gtmp=inp_unit->trg_acc) &0x8) || !(auxclr &ST_T4_UTIL)) {
						inp_unit->sm_cr=auxset;
						if (auxfcl &ST_AUX_MASK) {
							gtmp=inp_unit->sm_cr;
							inp_unit->sm_cr=auxfcl;
						}
						inp_unit->sm_cr=auxclr;
					}
				}
			} else {
				tmo--;
				if (inp_unit->sm_fclr) {
					inp_unit->sm_cr=auxset;
					if (auxfcl &ST_AUX_MASK) {
						gtmp=inp_unit->sm_cr;
						inp_unit->sm_cr=auxfcl;
					}
					inp_unit->sm_cr=auxclr;
				}
			}

		} while ((tmo >= 0) && (ecnt != 0xFFFF));

		if (config.tst_stcs) {
			if (!(ln %24)) {
				printf(
" EVC   T1   T2   T3   T4   TM      TDT   REJ OVR G_MIN G_AVE G_MAX G_CNT G_LOS\n");
			}

			if (!ecnt)
				printf("%04X %04X %04X %04X %04X %04X	  ---- %05lX%4u sr/cr=%04X/%04X",
						 (u_int)evc, trg[0], trg[1], trg[2], trg[3], tm,
						 rejcnt, rejovr, inp_unit->sm_sr, inp_unit->sm_cr);
			else {
				printf("%04X %04X %04X %04X %04X %04X   %06lX %05lX%4u",
						 (u_int)evc, trg[0], trg[1], trg[2], trg[3], tm,
						 tdt /ecnt, rejcnt, rejovr);

				if (g_cnt)
					printf("  %04X %05lX  %04X %05lX%6u",
							 g_min, g_ave/g_cnt, g_max, g_cnt, g_lose);
			}
		} else {
			if (!(ln %24)) {
				printf(" EVC   T1   T2   T3   T4   TM      TDT\n");
			}

			if (!ecnt)
				printf("%04X %04X %04X %04X %04X %04X	  ---- sr/cr=%04X/%04X",
						 (u_int)evc, trg[0], trg[1], trg[2], trg[3], tm, inp_unit->sm_sr, inp_unit->sm_cr);
			else
				printf("%04X %04X %04X %04X %04X %04X   %06lX",
					 (u_int)evc, trg[0], trg[1], trg[2], trg[3], tm, tdt /ecnt);
		}

		Writeln(); ln++;

if (!ecnt && (evc != inp_unit->sm_evc)) {
	printf("invalid event counter x, exp:%08lX, is:%08lX\n", evc,
			  inp_unit->sm_evc);
	return CE_PROMPT;

}

		while (kbhit()) {
			cx=toupper(getch());
			if (!cx) continue;

			if (cx == CR) {
				if (!wx) {
					printf("Conversion Time       $%06lX\n", inp_unit->sm_cvt);
					printf("Fast Clear Acceptance     $%02X\n", inp_unit->sm_fcat);
					printf("Trigger Acceptance Time  %4u\n", config.tst_tatw);
					printf("Trigger 4 Utilization    %4u\n", config.tst_t4ut);
					printf("Mask for AUX.OUT           $%X\n", config.tst_aux);
					printf("AUX.OUT mask for FCLR      $%X\n", config.tst_auxfclr);
					printf("determine statistic      %4u\n", config.tst_stcs);
					continue;
				}

				wrk[wx]=0; ix=0;
				if (wrk[0] == 'X') {
					ix=1;
					pw=(u_int)value(wrk, &ix, 16);
					config.tst_aux=pw &0xF;
					auxset =auxclr |((pw *ST_AUX0) &ST_AUX_MASK);
					wx=0;
					continue;
				}

				if (wrk[0] == 'C') {
					u_long	tmp;

					ix=1;
					tmp=value(wrk, &ix, 16);
					config.tst_cvt=tmp;
					inp_unit->sm_cvt=config.tst_cvt;
					wx=0;
					continue;
				}

				if (wrk[0] == 'F') {
					if (wrk[1] == 'X') {
						ix=2;
						pw=(u_int)value(wrk, &ix, 16);
						config.tst_auxfclr=pw &0xF;
						auxfcl =auxclr |((pw *ST_AUX0) &ST_AUX_MASK);
						wx=0;
						continue;
					}

					ix=1;
					pw=(u_int)value(wrk, &ix, 16);
					config.tst_fcat=pw;
					inp_unit->sm_fcat =config.tst_fcat;
					wx=0;
					continue;
				}

				if (wrk[0] == 'U') {
					ix=1;
					pw=(u_int)value(wrk, &ix, 16);
					if ((config.tst_t4ut=(pw != 0)) != 0) auxclr |=ST_T4_UTIL;
					else auxclr &=~ST_T4_UTIL;

					auxset=auxclr |(auxset &ST_AUX_MASK);
					auxfcl=auxclr |(auxfcl &ST_AUX_MASK);
					inp_unit->sm_cr=auxclr;
					wx=0;
					continue;
				}

				sel=-1;
				if (wrk[0] == 'M') { ix=1; sel=4; }
				if (wrk[0] == 'A') { ix=1; sel=5; }
				pw=(u_int)value(wrk, &ix, 10);
				if ((ix == wx) && (pw < 32)) {
					if (sel == -1) {
						tm=0;
						for (ix=0; ix < 4; ix++)
							if (trg[ix] >= tm) { tm=trg[ix]; sel=ix; }
					}

					if (sel >= 0) {
						printf("pulse width for trigger %d set to %d\n", sel+1, pw);
						config.puwidth[sel]=pw;
						set_pulse_width(1 <<sel, pw);
					}

					if (sel == 5) {
						if ((config.tst_tatw=config.puwidth[5]) != 0) auxclr |=ST_TAW_ENA;
						else auxclr &=~ST_TAW_ENA;

						auxset=auxclr |(auxset &ST_AUX_MASK);
						auxfcl=auxclr |(auxfcl &ST_AUX_MASK);
						inp_unit->sm_cr=auxclr;
					}
				}

				wx=0;
				continue;
			}

			if (   ((cx >= '0') && (cx <= '9'))
				 || ((cx >= 'A') && (cx <= 'Z'))) {
				if (wx < sizeof(wrk)-1) wrk[wx++]=cx;
				continue;
			}

			if (cx == '?') {
printf("{M|A}pw => set pulse width\n");
printf("Xn      => set auxiliary pattern\n");
printf("Un      => set trigger utilisation\n");
printf("Cn      => set conversion time\n");
printf("Fn      => set fast clear acceptance time\n");
printf("FXn     => set auxiliary pattern for fast clear\n");
			} else return CE_PROMPT;
		}
	}
}
//
//--------------------------- comp_test -------------------------------------
//
static int comp_test(void)
{
	u_int	ux;
	char	key;
	int	ret;

	while (1) {
		clrscr();
		ux=0;
		gotoxy(SP_, ux+ZL_); ux++;
						  printf("Test Counters ...................4");
		gotoxy(SP_, ux+ZL_); ux++;
						  printf("Test first Trigger evt(-1 -> 0)..6");
		gotoxy(SP_, ux+ZL_); ux++;
						  printf("Real Test .......................7");
		gotoxy(SP_, ux+ZL_); ux++;
						  printf("Set Pulse Width .................8");
		gotoxy(SP_, ux+ZL_);
						  printf("Ende ..........................ESC");
		gotoxy(SP_, ux+ZL_+1);
						  printf("                                 %c ",
														  config.i3_menu);

		key=toupper(getch());
		if ((key == CTL_C) || (key == ESC)) {
			Writeln();
			return CE_MENU;
		}

		while (1) {
			use_default=(key == TAB);
			if ((key == CR) || use_default) key=config.i3_menu;

			clrscr(); gotoxy(1, 10);
			errno=0; ret=0;
			switch (key) {

		case '4':
				config.i3_menu=key;
				ret=_test_counter();
//				if (ret < 0) ret=sync_master_menu(1);
				break;

		case '6':
				config.i3_menu=key;
				ret=_test_first();
//				if (ret < 0) ret=sync_master_menu(1);
				break;

		case '7':
				config.i3_menu=key;
				ret=_real_test();
				break;

		case '8':
//				ret=_set_pulse_width();
				break;

			}

			while (kbhit()) getch();
			if ((ret >= CEN_MENU) && (ret <= CE_MENU)) break;

			if (ret > CE_PROMPT) display_errcode(ret);

			printf("select> ");
			key=toupper(getch());
			if ((key < ' ') && (key != TAB)) break;

			if (key == ' ') key=CR;
		}
	}
}
//
//===========================================================================
//										sync_master_menu                              =
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
	"Rg", "Go", "Taw", "?", "A0", "A1", "A2", "A3",
	"U0", "U1", "U2", "U3", "Lup", "?", "?",   "?"
};
static const char *inpsr_txt[] ={
	"T0", "T1", "T2", "T3", "RGo", "Ve",  "Si", "Inh",
	"A0", "A1", "A2", "A3", "Eoc", "Rsi", "Hl", "Rtdt"
};
//
int sync_master_menu(int mode)
{
	int		ret=0;
	u_int		i;
	u_int		tmp;
	char		hgh;
	char		rkey,key;
	u_int		ux;
	char		hd=1;
	char		bc;

static u_int	pw_value[9];

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
printf("input unit (addr %2u)               %4u ..U\n", inp_conf->slv_addr, sc_conf->slv_sel);
printf("Version                            %04X     .x => broadcast\n", inp_unit->ident);
				if (inp_conf->type != SYMASTER) {
					printf("this is no sync master module!!!\n");
				}
			}

			if (inp_conf && (inp_conf->type == SYMASTER)) {
printf("Control                            %04X ..2 ", tmp=inp_unit->sm_cr);
				i=16; hgh=0;
				while (i--)
					if (tmp &((u_long)0x1 <<i)) {
						if (hgh) highvideo(); else lowvideo();
						hgh=!hgh; cprintf("%s", inpcr_txt[i]);
					}
				lowvideo(); cprintf("\r\n");

printf("Status                             %04X ..3 ", tmp=inp_unit->sm_sr);
				i=16; hgh=0;
				while (i--)
					if (tmp &((u_long)0x1 <<i)) {
						if (hgh) highvideo(); else lowvideo();
						hgh=!hgh; cprintf("%s", inpsr_txt[i]);
					}
				lowvideo(); cprintf("\r\n");

printf("trigger inhibit                    %04X ..4\n", inp_unit->trg_inh);
printf("trigger raw input                  %04X\n", inp_unit->trg_inp);
printf("trigger input accepted             %04X\n", inp_unit->trg_acc);
printf("trigger input rejected             %04X\n", inp_unit->sm_rej);
printf("trigger fast clear                 %04X\n", inp_unit->sm_fclr);
printf("trigger gap                    %08lX\n", inp_unit->sm_gap);
printf("pulse width (p3/p0) %04X %04X %04X %04X ..p.\n", inp_unit->pwidth[3], inp_unit->pwidth[2], inp_unit->pwidth[1], inp_unit->pwidth[0]);
printf("pulse width (p7/p4) %04X %04X %04X %04X ..p.\n", inp_unit->pwidth[7], inp_unit->pwidth[6], inp_unit->pwidth[5], inp_unit->pwidth[4]);
printf("pulse width                        %04X ..p8\n", inp_unit->pwidth[8]);
printf("set poti                           %04X ..5\n", inp_unit->setpoti);
printf("time counter                   %08lX\n", inp_unit->sm_tmc);
printf("conversion time                %08lX ..6\n", inp_unit->sm_cvt);
printf("total dead time                %08lX\n", inp_unit->sm_tdt);
printf("fast clear acceptance time         %04X ..7\n", inp_unit->sm_fcat);
printf("event counter                  %08lX\n", inp_unit->sm_evc);
printf("                              Mrst(1) %X ..8\n", (u_int)config.sm_data[8]);
printf("JTAG/Test/Special/3hit/Hit              ..J/T/X/h/H\n");
				if (BUS_ERROR) CLR_PROT_ERROR();
			}

			printf("                                          %c ", config.i2_menu);
			bc=0;
			while (1) {
				key=toupper(rkey=getch());
				if ((key == CTL_C) || (key == ESC)) return CE_MENU;

				if (key == 'P') {
					if (!bc) { printf("p"); bc=1; }
					continue;
				}

				if (key == BS) {
					if (bc) { printf("\b \b"); bc=0; }
					continue;
				}

				break;
			}
		} else {
			hd=1; printf("select> ");
			bc=0;
			while (1) {
				key=toupper(rkey=getch());
				if (key == CTL_C) return CE_MENU;

				if (key == 'P') {
					if (!bc) { printf("p"); bc=1; }
					continue;
				}

				if (key == BS) {
					if (bc) { printf("\b \b"); bc=0; }
					continue;
				}

				break;
			}

			if (!bc && (key == ' ')) key=toupper(rkey=config.i2_menu);

			if ((key == ESC) || (key == CR)) continue;
		}

		if (bc) {
			if (inp_conf->type != SYMASTER) continue;

			ux=key-'0';
			if (ux > 8) continue;

			printf("%c\n", key);
			printf("Value  %04X ", pw_value[ux]);
			pw_value[ux]=(u_int)Read_Hexa(pw_value[ux], -1);
			if (errno) continue;

			inp_unit->pwidth[ux]=pw_value[ux];
			continue;
		}

		use_default=(key == TAB);
		if (use_default || (key == CR)) key=toupper(rkey=config.i2_menu);

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

		if (inp_conf->type != SYMASTER) continue;
//
//---------------------------
//
		if (key == 0) {
			key=getch();
			continue;
		}

		if (key == 'C') {
			inp_unit->sm_ctrl=RST_TRG;
			continue;
		}

		if (key == 'R') {
			tmp=inp_unit->sm_cr;
			inp_unit->sm_cr=0;
			inp_unit->sm_cr=tmp;
			inp_unit->sm_ctrl=RST_TRG;
			continue;
		}

		if (rkey == 'H') {
			tmp=inp_unit->sm_cr;
			tmp=inp_unit->sm_cr =tmp |ST_AUX_MASK;
			tmp=inp_unit->sm_cr =tmp &~ST_AUX_MASK;
			continue;
		}

		if (rkey == 'h') {
			tmp=inp_unit->sm_cr;
			tmp=inp_unit->sm_cr =tmp |ST_AUX_MASK;
			tmp=inp_unit->sm_cr =tmp &~ST_AUX_MASK;
			tmp=inp_unit->sm_cr =tmp |ST_AUX_MASK;
			tmp=inp_unit->sm_cr =tmp &~ST_AUX_MASK;
			tmp=inp_unit->sm_cr =tmp |ST_AUX_MASK;
			tmp=inp_unit->sm_cr =tmp &~ST_AUX_MASK;
			continue;
		}

		if (rkey == 't') {
			config.i2_menu=rkey;
			if (mode) return CE_MENU;

			ret=comp_test();
			continue;
		}

		if (rkey == 'T') {
			config.i2_menu=rkey; hd=0;
			ret=gen_test();
			continue;
		}

		if (key == 'X') {
			ret=special_test();
			continue;
		}

		if (key == 'J') {
			JTAG_menu(jt_inp_putcsr, jt_inp_getcsr, jt_inp_getdata, &config.jtag_symaster, 0);
			continue;
		}

		if (key == '9') {
			hd=0;
			config.i2_menu=key; i=0;
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

		config.i2_menu=key;
		if (ux == 5) {
			printf("Value  %5u ", (u_int)config.sm_data[ux]);
			config.sm_data[ux]=(u_int)Read_Deci(config.sm_data[ux], -1);
			if (errno) continue;

		} else {
			if (ux == 6) {
				printf("Value  %08lX ", config.sm_data[ux]);
				config.sm_data[ux]=Read_Hexa(config.sm_data[ux], -1);
				if (errno) continue;
			} else {
				printf("Value  %04X ", (u_int)config.sm_data[ux]);
				config.sm_data[ux]=(u_int)Read_Hexa(config.sm_data[ux], -1);
				if (errno) continue;
			}
		}

		switch (ux) {

case 2:
			inp_unit->sm_cr=(u_int)config.sm_data[ux]; break;

case 4:
			inp_unit->trg_inh=(u_int)config.sm_data[ux]; break;

case 5:
			inp_unit->setpoti=(u_int)config.sm_data[ux]; break;

case 6:
			inp_unit->sm_cvt=config.sm_data[ux]; break;

case 7:
			inp_unit->sm_fcat=(u_int)config.sm_data[ux]; break;

case 8:
			if ((u_int)config.sm_data[ux] == MRESET) {
				inp_bc->ctrl=MRESET;
				break;
			}

			inp_unit->sm_ctrl=(u_int)config.sm_data[ux]; break;

		}
	}
}
// --- end ------------------ sync_master_menu ------------------------------
//
