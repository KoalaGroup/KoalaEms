// Borland C++ - (C) Copyright 1991, 1992 by Borland International

//	measure.c -- statistic Messungen

#include <stddef.h>		// offsetof()
#include <stdlib.h>
#include <values.h>
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
#include "math.h"

static FILE		*logfile=0;

static struct time	timerec;
static struct date	daterec;

//--------------------------- raw_data --------------------------------------
//
static void raw_data(void)
{
	u_int		i,j;
	u_int		lx;

	lx=rout_len/4;
	if (rout_hdr) {
		i=0;
		while (i < HDR_LEN) printf(" %08lX", robf[i++]);

		Writeln();
		if (lx <= HDR_LEN) return;
	}

	j=0;
	while (i < lx) {
		if (j %8) printf(" ");
		printf(" %02X:%X%04X", EV_CHAN(robf[i]),
									  EV_MTIME(robf[i]) &0xF, EV_FTIME(robf[i]));
		i++; j++;
		if (!(j %8)) Writeln();
	}

	if (rout_hdr &MCR_DAQTRG) {
		if (j %8) printf(" ");
		printf(" MT:%X%04X", EV_MTIME(rout_trg_time)&0xF, EV_FTIME(rout_trg_time));
		Writeln();
	} else {
		if (j %8) Writeln();
	}
}
//
//--------------------------- close_logfile ----------------------------------
//
static int close_logfile(void)
{
	int	ret;

	if (!logfile) return 0;

	fflush(logfile); ret=0;
	if (fclose(logfile)) {
		perror("can't close logfile");
		printf("name =%s", config.logname);
		ret=1;
	}

	logfile=0;
	return ret;
}
//
//--------------------------- open_logfile -----------------------------------
//
static int open_logfile(void)
{
	char	*str;

	printf("logfile: ");
	str=Read_String(config.logname, sizeof(config.logname));
	if (errno) {
		if (close_logfile()) return CEN_PROMPT;
		return CEN_MENU;
	}

	if (logfile && !strcmp(str, config.logname)) return 0;

	if (close_logfile()) return CEN_PROMPT;

	if (!*str) return 0;

	logfile=fopen(str, "w");
	if (!logfile) {
		perror("can't open logfile");
		printf("name =%s\n", str);
		return CEN_MENU;
	}

	strcpy(config.logname, str);
	return 0;
}
//
//--------------------------- loginfo ---------------------------------------
//
static void loginfo(void)
{
	char	*str;

	if (!logfile) return;

	printf("enter text\n");
	while (1) {
		str=Read_String(0, 80);
		if (errno) return;

		fprintf(logfile, "%s\n", str);
	}
}
//
//------------------------------ show_dbuf ----------------------------------
//
static int show_dbuf(int pg)
{
static int	blk;

	u_int	ix;
	u_int	ln;

	if (pg < 0) {
		printf("number of block %2d ", blk);
		blk=(u_int)Read_Deci(blk, -1);
		if (errno) return CEN_MENU;

		pg=blk;
	}

	if ((pg < 0) || (pg >= 16)) return CE_PARAM_ERR;

	ix=0x1000*pg; ln=0;
	do {
		if (!(ix &0x7)) printf("%04X: ", ix);

		printf(" %08lX", WBUF_L(ix)); ix++;
		if (!(ix &0x7)) {
			printf("\n");
			if (++ln >= 32) {
				if (WaitKey(0)) return CEN_PROMPT;

				ln=0;
			}
		}
	} while (ix);

	return CEN_PROMPT;
}
//
//--------------------------- F1_diff ---------------------------------------
//
static long F1_diff(		// of (v1-v0)
								// MAXLONG: overflow
	u_long v0,
	u_long v1)
{
	u_int		tm_l0,tm_h0;
	u_int		tm_l1,tm_h1;
	long		diff;

//printf("%08lX %08lX\n", v0, v1);
	if (config.f1_sync == 0) return ((int)EV_FTIME(v1)-(int)EV_FTIME(v0));

	tm_h0=EV_MTIME(v0);
	tm_h1=EV_MTIME(v1);
	if (   (tm_h0 != tm_h1)
		 && (((tm_h0+1) &C_MTIME) != tm_h1)
		 && (tm_h0 != ((tm_h1+1) &C_MTIME))) return MAXLONG;

	tm_l0=EV_FTIME(v0);
	tm_l1=EV_FTIME(v1);
	diff=(u_long)tm_l1-(u_long)tm_l0;
	if (tm_h0 != tm_h1) {
		if (((tm_h0+1) &C_MTIME) == tm_h1) diff +=f1_range;	// diff=(tm_l1+f1_range)-tm_l0;
		else diff -=f1_range;											// diff=tm_l1-(tm_l0+f1_range);
	}

	return diff;
}
//
//--------------------------- print_distribution ----------------------------
//
#define PRT_X	80
#define PRT_Y0	1
#define PRT_Y	45
static u_char	prt_dis[PRT_X];
static u_int	prt_org;			// Null Index

static void print_distribution(void)
{
	u_int		i,n;
	u_long	mx;	// max
	u_char	dis[PRT_X];

	if (prt_org == 0xFFFF) {
		gotoxy(1, PRT_Y0+PRT_Y); printf(
"--------------------------------------------------------------------------------");
		for (n=0; n < PRT_X; prt_dis[n++]=0);

		mx=0; i=0;
		do {
			if (!WBUF_L(i)) continue;

			if (mx < WBUF_L(i)) { mx =WBUF_L(i); n=i; }

		} while (++i);

		if (!mx) { gotoxy(1, PRT_Y0+PRT_Y+1); return; }

		prt_org=n-(PRT_X/2-1);
		if (prt_org > (0xFFFF-(PRT_X-1))) prt_org=0xFFFF-(PRT_X-1);

		printf("max at %04X(%1.1fns)", n, n*RESOLUTION/1000.);
	} else {
		for (mx=0, i=prt_org, n=0; n < PRT_X; i++, n++)
			if (mx < WBUF_L(i)) mx =WBUF_L(i);
	}

	if (!mx) return;

	for (i=prt_org, n=0; n < PRT_X; i++, n++)
		dis[n]=(u_int)(((PRT_Y-1)*WBUF_L(i))/mx);

	for (n=0; n < PRT_X; n++) {
		if (prt_dis[n] == dis[n]) continue;

//gotoxy(1, PRT_Y0+PRT_Y+1);
//printf("\r%2d %2d ", dis[n], prt_dis[n]); if (WaitKey(0)) return;
		if (dis[n] >= prt_dis[n]) {
			i=dis[n];
			gotoxy(n+1, PRT_Y0+(PRT_Y-i)); printf("*");
			while (--i >= prt_dis[n]) {
				gotoxy(n+1, PRT_Y0+(PRT_Y-i)); printf("|");
				if (!i) break;
			}
		} else {
			i=prt_dis[n];
			while (i > dis[n]) { gotoxy(n+1, PRT_Y0+(PRT_Y-i)); printf(" "); i--; }

			gotoxy(n+1, PRT_Y0+(PRT_Y-i)); printf("*");
		}

		prt_dis[n]=dis[n];
	}

	gotoxy(1, PRT_Y0+PRT_Y+1);
}
//
//--------------------------- display_distribution --------------------------
//
// Achtung: wenn i-Quadrate addiert werden, muss mit double gerechnet werden.
//
static int display_distribution(char sign)
{
	u_int		i;
	u_long	mx;
	u_int		s,e,m;	// index fuer start, end, max
	u_int		v0;
	u_int		ln,c;
	float		area;
	u_long	x;
	float		n,ew,var,sigma;

	mx=0; i=0; s=0xFFFF; e=0; area=0.;
	n=0.; ew=0.;
	do {
		if (!(x=WBUF_L(i))) continue;

		ew  +=((float)x *(float)i); n +=x;
		area +=WBUF_L(i);
		e=i;
		if (s == 0xFFFF) s=i;

		if (mx < WBUF_L(i)) { mx =WBUF_L(i); m=i; }

	} while (++i);

	if (!mx) return 0;

	ew=ew/n;
	i=0; var=0.;
	do {
		if (!(x=WBUF_L(i))) continue;

		var +=((float)x*( ((float)i-ew) *((float)i-ew) ));
	} while (++i);

	sigma=sqrt(var/n);
	printf("max=%u, E(X)=%1.1f, sigma(X)=%1.2f\n", m+0x8000, ew, sigma);
	area /=mx;
	if (sign)
		printf("max at 0x%04X(%1.1fns) =%lu, area=%1.2f, width=%u(%1.1fns)\n",
				 m+0x8000, (int)(m-0x8000)*RESOLUTION/1000., mx, area, e-s+1, (e-s+1)*RESOLUTION/1000.);
	else
		printf("max at 0x%04X(%1.1fns) =%lu, area=%1.2f, width=%u(%1.1fns)\n",
				 m+0x8000, m*RESOLUTION/1000., mx, area, e-s+1, (e-s+1)*RESOLUTION/1000.);

#if 0
	{
	u_long	x1;

	i=s;
	ln=0;
	printf("0x%04X:\n", i); x1=WBUF_L(i);
	while (1) {
		x=x1;
		x1=WBUF_L(i+1);
		if (!x && !x1) {
			while (i && (i < e) && ((x=WBUF_L(i)) == 0)) i++;

			if (!i) break;

			x1=WBUF_L(i+1);
			printf("0x%04X:\n", i);
		}

		v0=(u_int)((79L*x)/mx);

		c=0;
		do {
			if (v0 == c) {
				printf("*");
				c++; break;
			}

			if (!c && (i &1)) printf("-");	// ungerader Wert
			else printf("-");

		} while (++c < 80);

		if (c < 80) Writeln();

		if ((i >= e) || (++ln >= 40)) break;

		i++;
	} 

	printf(":0x%04X\n", e);
	}
#endif

	if (!logfile) return 0;

	fprintf(logfile, "max at 0x%04X(%1.1fns) =%lu, area=%1.2f, width=%u(%1.1fns)\n",
				m+0x8000,
				(sign) ? (int)(m-0x8000)*RESOLUTION/1000. : m*RESOLUTION/1000.,
				mx, area, e-s+1, (e-s+1)*RESOLUTION/1000.);
	fprintf(logfile, "max at %d, E(X)=%1.1f(%1.1fns), sigma(X)=%1.2f\n",
				m+0x8000, ew,
				(sign) ? (ew-0x8000L)*RESOLUTION/1000. : ew*RESOLUTION/1000.,
				sigma);

	fprintf(logfile, "\n");
	i=s;
	ln=0;
	fprintf(logfile, "0x%04X:\n", s+0x8000);
	while (1) {
		v0=(u_int)((79L*WBUF_L(i))/mx);

		c=0;
		do {
			if (v0 == c) {
				fprintf(logfile, "*");
				c++; break;
			}

			fprintf(logfile, "-");

		} while (++c < 80);

		fprintf(logfile, "\n");
		if ((i >= e) || (++ln >= 40)) break;

		i++;
	} 

	fprintf(logfile, ":0x%04X\n", e+0x8000);
	fprintf(logfile, "\n");
	return 0;
}
//===========================================================================
//
//--------------------------- length_distribution ---------------------------
//
static int length_distribution(u_long cnt)
{
	int		ret;
//	char		err=0;
	u_int		i;
	int		i0;
	u_int		lx;
	u_int		cha,lcha;
	long		diff;
	u_long	lp,zt;
//	u_long	le,te;

	if ((int)measconf.chan < 0) printf("channel    %3d ", measconf.chan);
	else printf("channel    %03X ", measconf.chan);

	measconf.chan=(int)Read_Hexa(measconf.chan, 0x3FF);
	if (errno) return CEN_MENU;

	if ((config.f1_letra &0x03) != 0x3) {
		printf("select leading and trailing edge mode\n");
		return CEN_PROMPT;
	}

	i=0;
	do WBUF_L(i)=0; while (++i);

	config.daq_mode=0;
	if ((ret=inp_mode(IM_QUIET|IM_NODAQTRG)) != 0) return ret;

	use_default=0;
	prt_org=0xFFFF; clrscr();
	lp=0; timemark();
	while (1) {
		if (config.daq_mode &EVF_ON_TP) 	sc_conf->sc_unit->ctrl=TESTPULSE;

		if ((ret=read_event(0, RO_RAW)) != 0) {
			if (ret != (CE_KEY_PRESSED|CR)) break;

			prt_org=0xFFFF; clrscr();
			print_distribution();
			continue;
		}

		lx =rout_len >>2;
		i=0; i0=-1;
		lp++;
		do {
			cha=EV_CHAN(robf[i]);
			if (((int)measconf.chan >= 0) && (cha != measconf.chan)) continue;

			if (i0 < 0) { i0=i; lcha=cha; continue; }

			if (cha != lcha) { i0=-1; break; }

//printf(" %d/%d %08lX/%08lX\n", i0, i, robf[i0], robf[i]);
			if ((config.f1_letra &0x04) && !((robf[i0] ^robf[i]) &1)) {
				Writeln(); printf("no double edge\n");
				break;
			}

			if (rout_hdr == MCR_DAQTRG) diff=(short)robf[i0]-(short)robf[i];
			else diff=F1_diff(robf[i0], robf[i]);

//printf(" %08lX", diff);
//			if (config.f1_letra &0x04) diff >>=1;
			
//printf(" %08lX\n", diff);
			if ((diff == MAXLONG) || ((u_long)diff >= 0x10000L)) {
				Writeln(); printf("overflow\n");
				break;
			}

			WBUF_L(diff)++;
			if (!(lp &0x03FF)) {
				print_distribution();
				printf("\n%9lu: %03X", lp, lcha);
#if 0
				le=robf[i0];
				te=robf[i];
				printf("\r%9lu: %X%04X %X%04X", lp,
					EV_MTIME(le) &0xF, EV_FTIME(le),
					EV_MTIME(te) &0xF, EV_FTIME(te));
#endif
			}

			i0=-2;
			break;

		} while (++i < lx);

#if 0
if (i0 != -2) {
	if (!err) {
		err=1; printf("%d Kanal oder Paar nicht gefunden\n", i0);
	}
//	printf("\r%9lu\n", lp); raw_data(); if (WaitKey(0)) return CE_PROMPT; }
}
#endif
		if (cnt && (lp >= cnt)) break;
	}

	Writeln();
	zt=ZeitBin();
	printf("loops=0x%08lX\n", lp);
	if (zt) printf("%lu per sec\n", (100*lp)/zt);

	if (logfile) {
		getdate(&daterec);
		gettime(&timerec);
		fprintf(logfile, "date/time %02u.%02u.%02u  %2u:%02u:%02u\n",
				  daterec.da_day, daterec.da_mon, daterec.da_year %100,
				  timerec.ti_hour, timerec.ti_min, timerec.ti_sec);
		fprintf(logfile, "length distribution at channel 0x%03X\n",
				  measconf.chan);
		fprintf(logfile, "f1_sync=%d, daq_mode=%d, f1_letra=%02X, resolution=%6.2fps\n",
				  config.f1_sync, config.daq_mode, config.f1_letra, RESOLUTION);
		fprintf(logfile, "loops=%lu\n", lp);
//		fprintf(logfile, "last leading/trailing edge: 0x%X%04X/%X%04X\n",
//							  EV_MTIME(le) &0xF, EV_FTIME(le),
//							  EV_MTIME(te) &0xF, EV_FTIME(te));
		fprintf(logfile, "\n");
	}

	display_distribution(0);
	return ret;
}
//
//--------------------------- difference_distribution -----------------------
//
static int difference_distribution(u_long cnt)
{
	int		ret;
	u_int		i;
	int		i0,i1;
	u_int		lx;
	u_int		cha;
	long		diff;
	u_long	lp,zt;

	printf("1. channel   %03X ", measconf.chan);
	measconf.chan=(int)Read_Hexa(measconf.chan, 0xFFF);
	if (errno) return CEN_MENU;

	printf("2. channel   %03X ", measconf.chan1);
	measconf.chan1=(int)Read_Hexa(measconf.chan1, 0xFFF);
	if (errno) return CEN_MENU;

	i=0;
	do WBUF_L(i)=0; while (++i);

	config.daq_mode=0;
	if ((ret=inp_mode(IM_QUIET|IM_NODAQTRG)) != 0) return ret;

	prt_org=0xFFFF; clrscr();
	lp=0; timemark();
//
//--------------------------- 2 channels
//
	while (1) {
		if ((ret=read_event(0, RO_RAW)) != 0) {
			if (ret != (CE_KEY_PRESSED|CR)) break;

			prt_org=0xFFFF; clrscr();
			continue;
		}

		lx =rout_len >>2; i0=-1; i1=-1;
		for (i=0; i < lx; i++) {
			cha=EV_CHAN(robf[i]);
			if (cha == measconf.chan) {
				if (i0 >= 0) continue;

				i0=i;
				if (i1 < 0) continue;
			} else {
				if ((cha != measconf.chan1) || (i1 >= 0)) continue;

				i1=i;
				if (i0 < 0) continue;
			}

			if (!(lp &0x03FF)) {
				print_distribution();
				printf("\r%9lu: %X%04X %X%04X", lp,
					EV_MTIME(robf[i0]) &0xF, EV_FTIME(robf[i0]),
					EV_MTIME(robf[i1]) &0xF, EV_FTIME(robf[i1]));
			}

			diff=F1_diff(robf[i0], robf[i1]);
			if ((diff == MAXLONG) || (diff < -0x8000L) || (diff >= 0x8000L)) {
if (!use_default) {
				printf("\r%9lu: %X%04X %X%04X", lp,
					EV_MTIME(robf[i0]) &0xF, EV_FTIME(robf[i0]),
					EV_MTIME(robf[i1]) &0xF, EV_FTIME(robf[i1]));
				printf(" => overflow\n");
#if 1
Writeln();
printf("%d %d %d\n", i, i0, i1);
raw_data();
if (WaitKey(0)) return CE_PROMPT;
#endif
}
				break;
			}

			diff+=0x8000L;
//printf("%u: %08lX %04X\n", i, diff, (u_int)diff >> 14);
			WBUF_L(diff)++;
			i0=-1; i1=-1;

		}

		lp++;
		if (cnt && (lp >= cnt)) break;
	}

	Writeln();
	zt=ZeitBin();
	printf("loops=0x%08lX\n", lp);
	if (zt) printf("%lu per sec\n", (100*lp)/zt);

	if (logfile) {
		getdate(&daterec);
		gettime(&timerec);
		fprintf(logfile, "date/time %02u.%02u.%02u  %2u:%02u:%02u\n",
				 daterec.da_day, daterec.da_mon, daterec.da_year %100,
				 timerec.ti_hour, timerec.ti_min, timerec.ti_sec);
		fprintf(logfile, "difference distribution of channel 0x%03X => 0x%03X\n",
				  measconf.chan, measconf.chan1);

		fprintf(logfile, "f1_sync=%d, daq_mode=%d, f1_letra=%02X, resolution=%6.2fps\n",
				  config.f1_sync, config.daq_mode, config.f1_letra, RESOLUTION);
		fprintf(logfile, "loops=%lu\n", lp);
		fprintf(logfile, "\n");
	}

	display_distribution(1);
	return ret;
}
//
//===========================================================================
//
//--------------------------- measurement -----------------------------------
//
int measurement(void)
{
	u_int		i;
	char		rkey;
	int		ret;

	while (1) {
		clrscr();
		i=0;
		gotoxy(SP_, i+++ZL_);
						printf("length distribution .............1");
		gotoxy(SP_, i+++ZL_);
						printf("difference distribution .........2");
		gotoxy(SP_, i+++ZL_);
						printf("display distribution buffer .....B");
		gotoxy(SP_, i+++ZL_);
						printf("setup ...........................s");
		gotoxy(SP_, i+++ZL_);
						printf("Ende ..........................ESC");
		gotoxy(SP_, i+++ZL_);
						printf("                                 %c ", measconf.menu_0);

		while (1) {
			rkey=getch();
			if ((rkey == CTL_C) || (rkey == ESC)) {
				close_logfile();
				return 0;
			}

			break;
		}

		while (1) {
			use_default=(rkey == TAB);
			if ((rkey == TAB) || (rkey == CR)) rkey=measconf.menu_0;

//			clrscr(); gotoxy(1, 10);
			Writeln();
			errno=0; ret=CEN_MENU;
			switch (rkey) {

		case '1':
				ret=length_distribution(0);
				break;

		case '2':
				ret=difference_distribution(0);
				break;

		case 'b':
		case 'B':
				ret=show_dbuf(-1);
				break;

		case 'l':
		case 'L':
				open_logfile();
				break;

		case 'i':
		case 'I':
				loginfo();
				ret=CEN_PROMPT;
				break;

		case 's':
		case 'S':
				ret=general_setup(-2, -1);
				if ((ret <= CE_PROMPT) || (ret >= CE_KEY_PRESSED)) ret=CEN_PROMPT;
				break;

			}

			stop_ddma();
			while (kbhit()) getch();
			if ((ret &~0x1FF) == CE_KEY_PRESSED) ret=CE_PROMPT;

			if ((ret >= 0) && (ret <= CE_PROMPT)) measconf.menu_0=rkey;

			if ((ret >= CEN_MENU) && (ret <= CE_MENU)) break;

			if (ret > CE_PROMPT) display_errcode(ret);

			printf("select> ");
			rkey=getch();
			if ((rkey < ' ') && (rkey != TAB)) break;

			if (rkey == ' ') rkey=CR;
		}
	}
}
