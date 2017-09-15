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

static u_char	no_init;
//
#if 1
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
#endif
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
//--------------------------- F1_bin_distribution ---------------------------
//
static int F1_bin_distribution(void)
{
	int		ret;
	u_int		h,i;
	u_long	cmask[2*C_UNIT];
	u_int		cha;
	char		mt;
	u_int		lx;
	u_int		s,e;
	u_int		m0,m1;			// Index Maximum gerader/ungerader Werte
	u_long	mx0,mx1;			// Maximum gerader/ungerader Werte
	float		wth0,wth1;		// Summe gerader/ungerader Werte
	u_long	mx;				// Maximalwert
	u_long	lp,zt;
	u_int		v0;
#if 0
	u_long	w0,w1;
	u_int		v1;
	u_int		ex;
#endif
	u_int		ln,c;

	for (i=0; i < 2*C_UNIT; cmask[i++]=0);
	mt=0; i=0;
	printf("use \"-1\" for main trigger or \"-2\" to start\n");
	while (1) {
		if (config.sel_cha[i] < 0)
			printf("enable channel %5d ", config.sel_cha[i]);
		else
			printf("enable channel 0x%03X ", config.sel_cha[i]);

		config.sel_cha[i]=(int)Read_Hexa(config.sel_cha[i], -1);
		if (errno) return CEN_MENU;

		if (config.sel_cha[i] <= -2) {
			config.sel_cha[i]=-2;
			break;
		}

		if (config.sel_cha[i] < 0) mt=1;
		else {
			if (config.sel_cha[i] >= 64*C_UNIT) {
				printf("max channel is 0x%03X\n", 64*C_UNIT-1);
				continue;
			}

			cmask[config.sel_cha[i] >>5] |=(1L <<(config.sel_cha[i] &0x1F));
		}

		i++;
		if (i >= C_SEL_CHA) break;
	}

	i=0;
	do WBUF_L(i)=0; while (++i);

	if ((ret=inp_mode(IM_NODAQTRG)) != 0) return ret;

	if (mt && (!rout_hdr || !(config.daq_mode &EVF_HANDSH))) {
		mt=0; printf("main trigger not available\n");
		if (WaitKey(0)) return CEN_PROMPT;
	}

	h=(rout_hdr) ? HDR_LEN : 0;
	prt_org=0xFFFF; clrscr();
	lp=0; timemark();
	while (1) {
		if (config.daq_mode &EVF_ON_TP) 	sc_conf->sc_unit->ctrl=TESTPULSE;

		if ((ret=read_event(0, RO_RAW)) != 0) {
			if (ret != (CE_KEY_PRESSED|CR)) break;

			prt_org=0xFFFF; clrscr();
			continue;
		}

		if (mt && (rout_hdr &MCR_DAQTRG)) WBUF_L(rout_trg_time)++;

		i=h; lx =rout_len >>2;
		while (i < lx) {
			cha=EV_CHAN(robf[i]);
			if (cmask[cha >>5] &(1L <<(cha &0x1F))) WBUF_L(EV_FTIME(robf[i]))++;

			i++;
		}

		lp++;
		if (!(lp &0x0FFF)) print_distribution();
#if 0
		if (!(lp &0x03FF)) {
			printf("\r%9lu", lp);
		}
#endif
	}

	zt=ZeitBin();
	printf("%04X\n", ret);
	printf("loops=0x%08lX\n", lp);
	if (zt) printf("%lu per sec\n", (100*lp)/zt);

	mx0=0; mx1=0; wth0=0.; wth1=0.; s=0xFFFF; i=0;
	do {
		if (!WBUF_L(i)) continue;

		if (s == 0xFFFF) s=i;	// start

		e=i;							// vorlaeufiges Ende
		if (!(i &1)) {
			wth0 +=WBUF_L(i);
			if (mx0 < WBUF_L(i)) { mx0 =WBUF_L(i); m0=i; }
			continue;
		}

		wth1 +=WBUF_L(i);
		if (mx1 < WBUF_L(i)) { mx1 =WBUF_L(i); m1=i; }

	} while (++i);

	if (!mx0 && !mx1) return ret;

	mx=(mx0 > mx1) ? mx0 : mx1;
#if 0
	printf("max0 at 0x%04X(%4.0fns) =%lu, area=%6.0f\n",
			 m0, m0*RESOLUTION/1000., mx0, wth0/mx0);
	printf("max1 at 0x%04X(%4.0fns) =%lu, area=%6.0f, width=0x%04X(%1.0fns)\n",
			 m1, m1*RESOLUTION/1000., mx1, wth1/mx1, e-s+1, (e-s+1)*RESOLUTION/1000.);

	ln=0; i =s &~1;
	printf("0x%04X:\n", s);
	do {
		w0=WBUF_L(i); i++;
		w1=WBUF_L(i); i++;
		if (!w0 && !w1) {
			do {
				w0=WBUF_L(i); i++;
				w1=WBUF_L(i); i++;
			} while (i && !w0 && !w1);

			if (!i) break;

			printf("0x%04X:\n", i-2);
		}

		v0=(u_int)((79L*w0)/mx);
		v1=(u_int)((79L*w1)/mx);
		c=0;
		do {
			if (v0 == c) {
				if (v0 == v1) { printf("Û"); c++; break; }

				printf("ß");
				if (v1 < v0) { c++; break; }

				continue;
			}

			if (v1 == c) {
				printf("Ü");
				if (v0 < v1) { c++; break; }

				continue;
			}

			printf("_");

		} while (++c < 80);

		if (c <= 10) {
			while (c < 10) { printf(" "); c++; }

			printf(" 0x%04X(%5.1fns):%lu/%lu", i-2, (i-2)*RESOLUTIONNS, w0, w1);
		}

		if (c < 80) Writeln();
	} while (++ln < 20);

	if (i) {
		ln=0; ex=e |1;
		if (i < (ex -39)) i=ex -39;

		printf("0x%04X:\n", i);
		while (1) {
			v0=(u_int)((79L*WBUF_L(i))/mx); i++;
			v1=(u_int)((79L*WBUF_L(i))/mx);
			c=0;
			do {
				if (v0 == c) {
					if (v0 == v1) { printf("Û"); c++; break; }

					printf("ß");
					if (v1 < v0) { c++; break; }

					continue;
				}

				if (v1 == c) {
					printf("Ü");
					if (v0 < v1) { c++; break; }

					continue;
				}

				printf("_");

			} while (++c < 80);

			if (c < 80) Writeln();
			if ((i >= ex) || (++ln >= 20)) break;

			i++;
		}
	}

	printf(":0x%04X\n", e);
#endif

	if (!logfile) return ret;

#define CPL		100
#define LPHP	49
	fprintf(logfile, "max0 at 0x%04X(%4.0fns) =%lu, area=%6.0f\n",
			  m0, m0*RESOLUTION/1000., mx0, wth0/mx0);
	fprintf(logfile, "max1 at 0x%04X(%4.0fns) =%lu, area=%6.0f, width=0x%04X(%1.0fns)\n",
			  m1, m1*RESOLUTION/1000., mx1, wth1/mx1, e-s+1, (e-s+1)*RESOLUTION/1000.);

#if 0
	ln=0; i =s &~1;
	fprintf(logfile, "0x%04X:\n", s);
	do {
		v0=(u_int)(((u_long)CPL*WBUF_L(i))/mx); i++;
		v1=(u_int)(((u_long)CPL*WBUF_L(i))/mx); i++;
		c=0;
		do {
			if (v0 == c) {
				if (v0 == v1) { fprintf(logfile, "Û"); c++; break; }

				fprintf(logfile, "ß");
				if (v1 < v0) { c++; break; }

				continue;
			}

			if (v1 == c) {
				fprintf(logfile, "Ü");
				if (v0 < v1) { c++; break; }

				continue;
			}

			fprintf(logfile, "-");

		} while (++c < CPL);

		fprintf(logfile, "\n");
	} while (++ln < LPHP);

	ln=0; ex=e |1; i=ex -(2*LPHP-1);
	fprintf(logfile, "0x%04X:\n", i);
	while (1) {
		v0=(u_int)(((u_long)CPL*WBUF_L(i))/mx); i++;
		v1=(u_int)(((u_long)CPL*WBUF_L(i))/mx);
		c=0;
		do {
			if (v0 == c) {
				if (v0 == v1) { fprintf(logfile, "Û"); c++; break; }

				fprintf(logfile, "ß");
				if (v1 < v0) { c++; break; }

				continue;
			}

			if (v1 == c) {
				fprintf(logfile, "Ü");
				if (v0 < v1) { c++; break; }

				continue;
			}

			fprintf(logfile, "-");

		} while (++c < CPL);

		fprintf(logfile, "\n");
		if ((i >= ex) || (++ln >= LPHP)) break;

		i++;
	} 

	fprintf(logfile, ":0x%04X\n", e);
	fprintf(logfile, "\n");
#else
	ln=0; i =s;
	fprintf(logfile, "0x%04X:\n", s);
	do {
		v0=(u_int)(((u_long)CPL*WBUF_L(i))/mx); i++;
		c=0;
		do {
			if (v0 == c) {
				fprintf(logfile, "*"); c++; break;
			}

			fprintf(logfile, "-");

		} while (++c < CPL);

		fprintf(logfile, "\n");
	} while (++ln < 2*LPHP);

	ln=0; i=(e+1) -(2*LPHP);
	fprintf(logfile, "0x%04X:\n", i);
	while (1) {
		v0=(u_int)(((u_long)CPL*WBUF_L(i))/mx);
		c=0;
		do {
			if (v0 == c) {
				fprintf(logfile, "*"); c++; break;
			}

			fprintf(logfile, "-");

		} while (++c < CPL);

		fprintf(logfile, "\n");
		if ((i >= e) || (++ln >= 2*LPHP)) break;

		i++;
	} 

	fprintf(logfile, ":0x%04X\n", e);
	fprintf(logfile, "\n");
#endif
	return ret;
}
//
//===========================================================================
//
//--------------------------- length_distribution ---------------------------
//
static int length_distribution(u_long cnt)
{
	int		ret;
//	char		err=0;
	u_int		h,i;
	int		i0;
	u_int		lx;
	u_int		cha;
	long		diff;
	u_long	lp,zt;
//	u_long	le,te;

	printf("channel    %03X ", measconf.chan);
	measconf.chan=(int)Read_Hexa(measconf.chan, 0x3FF);
	if (errno) return CEN_MENU;

	if ((config.f1_letra &0x03) != 0x3) {
		printf("select leading and trailing edge mode\n");
		return CEN_PROMPT;
	}

	i=0;
	do WBUF_L(i)=0; while (++i);

	if (!no_init) {
		if ((ret=inp_mode(0)) != 0) return ret;

	}

//	if (rout_hdr == 0) return CE_PARAM_ERR;

	use_default=0;
	h=(rout_hdr) ? HDR_LEN : 0;
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

		if ((lx =rout_len >>2) <= h) continue;

		i=h; i0=-1;
		lp++;
		do {
			cha=EV_CHAN(robf[i]);
			if (cha != measconf.chan) continue;

			if (i0 < 0) { i0=i; continue; }

//printf(" %d/%d %08lX/%08lX\n", i0, i, robf[i0], robf[i]);
			if ((config.f1_letra &0x04) && !((robf[i0] ^robf[i]) &1)) {
				Writeln(); printf("no double edge\n");
				break;
			}

			if (rout_hdr == MCR_DAQTRG) diff=(short)robf[i0]-(short)robf[i];
			else diff=F1_diff(robf[i0], robf[i]);

//printf(" %08lX", diff);
			if (config.f1_letra &0x04) diff >>=1;
			
//printf(" %08lX\n", diff);
			if ((diff == MAXLONG) || ((u_long)diff >= 0x10000L)) {
				Writeln(); printf("overflow\n");
				break;
			}

			WBUF_L(diff)++;
			if (!(lp &0x03FF)) {
				print_distribution();
#if 0
				le=robf[i0];
				te=robf[i];
				printf("\r%9lu: %X%04X %X%04X", lp,
					EV_MTIME(le) &0xF, EV_FTIME(le),
					EV_MTIME(te) &0xF, EV_FTIME(te));
#endif
			}

			i0=-2;
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
	u_int		h,i;
	int		i0,i1;
	u_int		lx;
	u_int		cha;
	long		diff;
	u_long	lp,zt;

	printf("use -1 for trigger input\n");
	if (measconf.chan < 0) printf("1. channel   %3d ", measconf.chan);
	else printf("1. channel   %03X ", measconf.chan);

	measconf.chan=(int)Read_Hexa(measconf.chan, 0xFFF);
	if (errno) return CEN_MENU;

	if (measconf.chan1 < 0) printf("2. channel   %3d ", measconf.chan1);
	else printf("2. channel   %03X ", measconf.chan1);

	measconf.chan1=(int)Read_Hexa(measconf.chan1, 0xFFF);
	if (errno) return CEN_MENU;

	i=0;
	do WBUF_L(i)=0; while (++i);

	if ((ret=inp_mode(IM_NODAQTRG)) != 0) return ret;

	if (rout_hdr == MCR_DAQTRG) {
		printf("use raw mode\n");
		return CE_PARAM_ERR;
	}

	if ((measconf.chan < 0) || (measconf.chan1 < 0)) {
		if (!(config.daq_mode &EVF_HANDSH)) {
			printf("use handshake mode\n");
			return CE_PARAM_ERR;
		}

		if (!(config.daq_mode &MCR_DAQTRG)) {
			printf("use trigger mode\n");
			return CE_PARAM_ERR;
		}
	}

	h=(rout_hdr) ? HDR_LEN : 0;
	prt_org=0xFFFF; clrscr();
	lp=0; timemark();
	if (measconf.chan < 0) {
		while ((ret=read_event(EVF_NODAC, RO_RAW)) == 0) {
			lx =rout_len >>2;
			for (i=h; i < lx; i++) {
				cha=EV_CHAN(robf[i]);
				if (cha != measconf.chan1) continue;

				if (!(lp &0x03FF)) {
					print_distribution();
					printf("\r%9lu: %X%04X %X%04X", lp,
						EV_MTIME(rout_trg_time)&0xF, EV_FTIME(rout_trg_time),
						EV_MTIME(robf[i]) &0xF, EV_FTIME(robf[i]));
				}

				diff=F1_diff(rout_trg_time, robf[i]);
				if ((diff == MAXLONG) || (diff < -0x8000L) || (diff >= 0x8000L)) {
					printf("\r%9lu: %X%04X %X%04X", lp,
						EV_MTIME(rout_trg_time)&0xF, EV_FTIME(rout_trg_time),
						EV_MTIME(robf[i]) &0xF, EV_FTIME(robf[i]));
					printf(" => overflow\n");
					continue;
				}

				diff+=0x8000L;
//printf("%u: %08lX %04X\n", i, diff, (u_int)diff >> 14);
				WBUF_L(diff)++;

			}

			lp++;
			if (cnt && (lp >= cnt)) break;
		}
	} else {
		if (measconf.chan1 < 0) {
			while ((ret=read_event(EVF_NODAC, RO_RAW)) == 0) {
				lx =rout_len >>2;
				for (i=h; i < lx; i++) {
					cha=EV_CHAN(robf[i]);
					if (cha != measconf.chan) continue;

					if (!(lp &0x03FF)) {
						print_distribution();
						printf("\r%9lu: %X%04X %X%04X", lp,
							EV_MTIME(robf[i]) &0xF, EV_FTIME(robf[i]),
							EV_MTIME(rout_trg_time)&0xF, EV_FTIME(rout_trg_time));
					}

					diff=F1_diff(robf[i], rout_trg_time);
					if ((diff == MAXLONG) || (diff < -0x8000L) || (diff >= 0x8000L)) {
						printf("\r%9lu: %X%04X %X%04X", lp,
							EV_MTIME(robf[i]) &0xF, EV_FTIME(robf[i]),
							EV_MTIME(rout_trg_time)&0xF, EV_FTIME(rout_trg_time));
						printf(" => overflow\n");
						continue;
					}

					diff+=0x8000L;
//printf("%u: %08lX %04X\n", i, diff, (u_int)diff >> 14);
					WBUF_L(diff)++;

				}

				lp++;
				if (cnt && (lp >= cnt)) break;
			}
		} else {
//
//--------------------------- 2 channels
//
			while (1) {
				if (config.daq_mode &EVF_ON_TP) 	sc_conf->sc_unit->ctrl=TESTPULSE;

				if ((ret=read_event(0, RO_RAW)) != 0) {
					if (ret != (CE_KEY_PRESSED|CR)) break;

					prt_org=0xFFFF; clrscr();
					continue;
				}

				lx =rout_len >>2; i0=-1; i1=-1;
				for (i=h; i < lx; i++) {
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
		}
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
		if (measconf.chan < 0)
			fprintf(logfile, "difference distribution of trigger => channel 0x%03X\n",
					  measconf.chan1);
		else {
			if (measconf.chan1 < 0)
				fprintf(logfile, "difference distribution of channel 0x%03X => trigger\n",
						  measconf.chan);
			else
				fprintf(logfile, "difference distribution of channel 0x%03X => 0x%03X\n",
						  measconf.chan, measconf.chan1);
		}

		fprintf(logfile, "f1_sync=%d, daq_mode=%d, f1_letra=%02X, resolution=%6.2fps\n",
				  config.f1_sync, config.daq_mode, config.f1_letra, RESOLUTION);
		fprintf(logfile, "loops=%lu\n", lp);
		fprintf(logfile, "\n");
	}

	display_distribution(1);
	return ret;
}
//
//--------------------------- channel_distribution --------------------------
//
static int channel_distribution(void)
{
	int		ret;
	u_int		i;
	u_long	cmask[2*C_UNIT];
	u_int		cha;
	u_int		diff;
	u_long	lp,zt;

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

	if (config.sel_cha[0] < 0)
		for (i=0; i < 2*C_UNIT; cmask[i++]=0xFFFFFFFFL);

	i=0;
	do WBUF_L(i)=0; while (++i);

	if ((ret=inp_mode(IM_STRIG)) != 0) return ret;

	prt_org=0xFFFF; clrscr();
	lp=0; timemark();
	while ((ret=read_event(0, RO_TRG)) == 0) {
		for (i=0; i < tm_cnt; i++) {

			cha=(*tm_hit)[i].tm_cha;
			if (cmask[cha >>5] &(1L <<(cha &0x1F))) {
				diff =(u_int)(*tm_hit)[i].tm_diff +0x8000;
				WBUF_L(diff)++;
				lp++;
				if (!(lp &0x03FF)) {
					print_distribution();
					printf("\r%9lu: %X%04X", lp,
							EV_MTIME(rout_trg_time)&0xF, EV_FTIME(rout_trg_time));
				}
			}

		}
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
//		fprintf(logfile, "difference distribution of channel 0x%03X => 0x%03X\n",
//				  measconf.chan, measconf.chan1);
		fprintf(logfile, "f1_sync=%d, daq_mode=%02X, f1_letra=%02X, resolution=%6.2fps\n",
				  config.f1_sync, config.daq_mode, config.f1_letra, RESOLUTION);
		fprintf(logfile, "loops=%lu\n", lp);
		fprintf(logfile, "\n");
	}

	display_distribution(1);
	return ret;
}
//
//--------------------------- multi_channel_distribution --------------------
//
static int multi_channel_distribution(void)
{
	int		ret;
	u_int		i;
	u_int		ign=0;
	u_int		cha,lcha=0xFFFF;
	u_int		diff;
	u_long	lp,zt;

	printf("max nr of cha %2d ", config.sel_cha[C_SEL_CHA-1]);
	config.sel_cha[C_SEL_CHA-1]=(int)Read_Hexa(config.sel_cha[C_SEL_CHA-1], -1);
	if (errno) return CEN_MENU;

	if (!config.sel_cha[C_SEL_CHA-1] || ((u_int)config.sel_cha[C_SEL_CHA-1] > 4))
		config.sel_cha[C_SEL_CHA-1]=1;

	i=0;
	do WBUF_L(i)=0; while (++i);

	if ((ret=inp_mode(IM_STRIG)) != 0) return ret;

	prt_org=0xFFFF; clrscr();
	lp=0; timemark();
	while (1) {
		if ((ret=read_event(0, RO_TRG)) != 0) {
			if (ret != (CE_KEY_PRESSED|CR)) break;

			do WBUF_L(i)=0; while (++i);
			prt_org=0xFFFF; clrscr();
			continue;
		}

		if (ign) { ign--; continue; }

		for (i=0; (i < tm_cnt) && (i < config.sel_cha[C_SEL_CHA-1]); i++) {

			if (i == 0) {
				cha=(*tm_hit)[i].tm_cha;
				if (cha != lcha) {
					lcha=cha; ign=500;
					do WBUF_L(i)=0; while (++i);
					prt_org=0xFFFF; clrscr();
					break;
				}
			}

			diff =(u_int)(*tm_hit)[i].tm_diff +0x8000;
			WBUF_L(diff)++;
			lp++;
			if (!(lp &0x03FF)) {
				print_distribution();
				printf("\n%9lu: %03X", lp, lcha);
			}

		}
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
//		fprintf(logfile, "difference distribution of channel 0x%03X => 0x%03X\n",
//				  measconf.chan, measconf.chan1);
		fprintf(logfile, "f1_sync=%d, daq_mode=%02X, f1_letra=%02X, resolution=%6.2fps\n",
				  config.f1_sync, config.daq_mode, config.f1_letra, RESOLUTION);
		fprintf(logfile, "loops=%lu\n", lp);
		fprintf(logfile, "\n");
	}

	display_distribution(1);
	return ret;
}
//
//--------------------------- save_distribution -----------------------------
//
static int save_distribution(void)
{
	char	*str;
	int	hdl;
	u_int	i;

	printf("enter file name: ");
	str=Read_String(0, 80);
	if (errno) return CEN_MENU;

	if ((hdl=_rtl_creat(str, 0)) == -1) {
		perror("error creating file");
		printf("name =%s\n", str);
		return CEN_PROMPT;
	}

	for (i=0; i < 16; i++)	// 8*64k => u_int *abuf[];
		if ((u_int)write(hdl, abuf[i >>1] +((i&1)<<14), 0x8000) != 0x8000) {
			perror("error writing file");
			printf("name =%s\n", str);
			close(hdl);
			return CEN_PROMPT;
		}

	close(hdl);
	return CEN_PROMPT;
}
//
//--------------------------- channel_nr_distribution -----------------------
//
static int channel_nr_distribution(void)
{
	int		ret;
	u_int		h,lx,i;
	u_long	lp,zt;
	u_long	tmp;
	u_int		cha;

	printf("first channel to display 0x%03X ", measconf.chan);
	measconf.chan=(int)Read_Hexa(measconf.chan, -1);
	if (errno) return CEN_MENU;

	i=0;
	do WBUF_L(i)=0; while (++i);

	if ((ret=inp_mode(0)) != 0) return ret;

	h=(rout_hdr) ? HDR_LEN : 0;

	prt_org=measconf.chan; clrscr();
	gotoxy(1, PRT_Y0+PRT_Y); printf(
"--------------------------------------------------------------------------------");
	for (i=0; i < PRT_X; prt_dis[i++]=0);

	lp=0; timemark();
	while (1) {
		if ((ret=read_event(0, RO_RAW)) != 0) {
			if (ret == (CE_KEY_PRESSED|CR)) {
				clrscr();
				gotoxy(1, PRT_Y0+PRT_Y); printf(
"--------------------------------------------------------------------------------");
				for (i=0; i < PRT_X; prt_dis[i++]=0);
				print_distribution();
			} else {
				if (ret > CEN_PPRINT) break;
			}

			print_distribution();
			gotoxy(1, PRT_Y0+PRT_Y+1);
			continue;
		}

		lx =rout_len >>2;
		for (i=h; i < lx; i++) {
			cha=EV_CHAN(robf[i]);
			if (cha != 0x3FF) WBUF_L(cha)++;
		}

		lp++;
		if (!(lp &0x0FF)) {
			print_distribution();
			for (i=0; i < 78;) {
				tmp=WBUF_L(prt_org+i);
				if (  ((i == 0) || (WBUF_L(prt_org+i-1) <= tmp))
					 && (WBUF_L(prt_org+i+1) < tmp)) {
					printf("%02X ", (u_char)(prt_org+i)); i+=3;
				} else { printf(" "); i++; }
			}

			gotoxy(1, PRT_Y0+PRT_Y+3);
			printf("\r%9lu", lp);
			gotoxy(1, PRT_Y0+PRT_Y+1);
		}
	}

	Writeln();
	printf("ret=%03X\n", ret);
	zt=ZeitBin();
	printf("loops=0x%08lX\n", lp);
	if (zt) printf("%lu per sec\n", (100*lp)/zt);

	if (logfile) {
		getdate(&daterec);
		gettime(&timerec);
		fprintf(logfile, "date/time %02u.%02u.%02u  %2u:%02u:%02u\n",
				 daterec.da_day, daterec.da_mon, daterec.da_year %100,
				 timerec.ti_hour, timerec.ti_min, timerec.ti_sec);
//		fprintf(logfile, "difference distribution of channel 0x%03X => 0x%03X\n",
//				  measconf.chan, measconf.chan1);
		fprintf(logfile, "f1_sync=%d, daq_mode=%02X, f1_letra=%02X, resolution=%6.2fps\n",
				  config.f1_sync, config.daq_mode, config.f1_letra, RESOLUTION);
		fprintf(logfile, "loops=%lu\n", lp);
		fprintf(logfile, "\n");
	}

	display_distribution(1);
	return ret;
}
//
//--------------------------- time_distribution -----------------------------
//
static int time_distribution(void)
{
	int		ret;
	u_int		h,lx,i;
	u_long	lp,zt;
	u_int		cha;
	u_long	tt;

	printf("first channel 0x%03X ", measconf.chan);
	measconf.chan=(int)Read_Hexa(measconf.chan, -1);
	if (errno) return CEN_MENU;

	printf("last channel  0x%03X ", measconf.chan1);
	measconf.chan1=(int)Read_Hexa(measconf.chan1, -1);
	if (errno) return CEN_MENU;

	i=0;
	do WBUF_L(i)=0; while (++i);

	if ((ret=inp_mode(0)) != 0) return ret;

	if (rout_hdr != MCR_DAQTRG) {
		printf("only for trigger mode\n");
		return CEN_PROMPT;
	}

	h=(rout_hdr) ? HDR_LEN : 0;

	prt_org=0; clrscr();
	gotoxy(1, PRT_Y0+PRT_Y); printf(
"--------------------------------------------------------------------------------");
	printf("%-7.1f                                                                %7.1fns",
			 (tlat*RESOLUTION)/1000., ((tlat-twin)*RESOLUTION)/1000.);
	for (i=0; i < PRT_X; prt_dis[i++]=0);

	lp=0; timemark();
	while (1) {
		if ((ret=read_event(0, RO_RAW)) != 0) {
			if (ret == (CE_KEY_PRESSED|CR)) {
				clrscr();
				gotoxy(1, PRT_Y0+PRT_Y); printf(
"--------------------------------------------------------------------------------");
	printf("%-7.1f                                                                %7.1fns",
			 (tlat*RESOLUTION)/1000., ((tlat-twin)*RESOLUTION)/1000.);
				for (i=0; i < PRT_X; prt_dis[i++]=0);
				print_distribution();
			} else {
				if (ret > CEN_PPRINT) break;
			}

			print_distribution();
			gotoxy(1, PRT_Y0+PRT_Y+1);
			continue;
		}

		lx =rout_len >>2;
		for (i=h; i < lx; i++) {
			cha=EV_CHAN(robf[i]);
			if ((cha < measconf.chan) || (cha > measconf.chan1)) continue;

			tt=tlat-(int)robf[i];
			if (tt > twin) {
printf("%02X %04X %08lX %08lX %08lX\n", cha, (int)robf[i], tlat, tt, twin);
				return CE_FATAL;
			}

			tt=tt*80 /twin;
			WBUF_L((u_int)tt)++;
		}

		lp++;
		if (!(lp &0x0FF)) {
			print_distribution();
			gotoxy(1, PRT_Y0+PRT_Y+3);
			printf("\r%9lu", lp);
			gotoxy(1, PRT_Y0+PRT_Y+1);
		}
	}

	Writeln();
	printf("ret=%03X\n", ret);
	zt=ZeitBin();
	printf("loops=0x%08lX\n", lp);
	if (zt) printf("%lu per sec\n", (100*lp)/zt);

	if (logfile) {
		getdate(&daterec);
		gettime(&timerec);
		fprintf(logfile, "date/time %02u.%02u.%02u  %2u:%02u:%02u\n",
				 daterec.da_day, daterec.da_mon, daterec.da_year %100,
				 timerec.ti_hour, timerec.ti_min, timerec.ti_sec);
//		fprintf(logfile, "difference distribution of channel 0x%03X => 0x%03X\n",
//				  measconf.chan, measconf.chan1);
		fprintf(logfile, "f1_sync=%d, daq_mode=%02X, f1_letra=%02X, resolution=%6.2fps\n",
				  config.f1_sync, config.daq_mode, config.f1_letra, RESOLUTION);
		fprintf(logfile, "loops=%lu\n", lp);
		fprintf(logfile, "\n");
	}

	display_distribution(1);
	return ret;
}
//
//===========================================================================
//										GPX special routines                          =
//===========================================================================
//
//--------------------------- GPX_print_distribution ------------------------
//
//--- print GPX distribution
//
#define PRT_X	80
#define PRT_Y0	1
#define PRT_Y	45
static u_long	lprt_org;			// Null Index
#define LORG_MAX	0x20000L

static u_long	gmax;

static void GPX_print_distribution(void)
{
	u_int		i,n;
	u_long	x,tmp;
	u_char	dis[PRT_X];

	if (lprt_org == LORG_MAX) {
		gotoxy(1, PRT_Y0+PRT_Y); printf(
"--------------------------------------------------------------------------------");
		for (n=0; n < PRT_X; prt_dis[n++]=0);

		gmax=0; x=0;
		do {
			if (!WBUF_L(x)) continue;

			if (gmax < WBUF_L(x)) { gmax =WBUF_L(x); tmp=x; }

		} while (++x < LORG_MAX);

		if (!gmax) { gotoxy(1, PRT_Y0+PRT_Y+1); return; }

		if (tmp < (PRT_X/2-1)) lprt_org=0;
		else {
			lprt_org=tmp-(PRT_X/2-1);
			if (lprt_org > (LORG_MAX-PRT_X)) lprt_org=LORG_MAX-PRT_X;
		}

		printf("max at %05lX(%1.1fns)", tmp, tmp*G_RESOLUTION/1000.);
	} else {
		for (tmp=0, x=lprt_org, n=0; n < PRT_X; x++, n++)
			if (tmp < WBUF_L(x)) tmp =WBUF_L(x);

		if (gmax < tmp) gmax=tmp;
	}

	for (x=lprt_org, n=0; n < PRT_X; x++, n++)
		if (gmax == 0) dis[n]=0;
		else dis[n]=(u_int)(((PRT_Y-1)*WBUF_L(x))/gmax);

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
//--------------------------- GPX_display_distribution ----------------------
//
// Achtung: wenn i-Quadrate addiert werden, muss mit double gerechnet werden.
//
static int GPX_display_distribution(void)
{
	u_long	x;
	u_long	mx;
	u_long	s,e,m;	// index fuer start, end, max
	u_int		v0;
	u_int		ln,c;
	float		area;
	u_long	v;
	float		n,ew,var,sigma;

	mx=0; x=0; s=LORG_MAX; e=0; area=0.;
	n=0.; ew=0.;
	do {
		if (!(v=WBUF_L(x))) continue;

		ew  +=((float)v *(float)x); n +=v;
		area +=WBUF_L(x);
		e=x;
		if (s == LORG_MAX) s=x;

		if (mx < WBUF_L(x)) { mx =WBUF_L(x); m=x; }

	} while (++x < LORG_MAX);

	if (!mx) return 0;

	ew=ew/n;
	x=0; var=0.;
	do {
		if (!(v=WBUF_L(x))) continue;

		var +=((float)v*( ((float)x-ew) *((float)x-ew) ));
	} while (++x < LORG_MAX);

	sigma=sqrt(var/n);
	printf("max=%lu, E(X)=%1.1f, sigma(X)=%1.2f\n", m, ew, sigma);
	area /=mx;
	printf("max at 0x%04lX(%1.1fns) =%lu, area=%1.2f, width=%u(%1.1fns)\n",
			 m, m*G_RESOLUTION/1000., mx, area, (u_int)(e-s+1),
			 (e-s+1)*G_RESOLUTION/1000.);

	if (!logfile) return 0;

	fprintf(logfile, "max at 0x%04lX(%1.1fns) =%lu, area=%1.2f, width=%u(%1.1fns)\n",
				m,
				m*G_RESOLUTION/1000.,
				mx, area, (u_int)(e-s+1), (e-s+1)*G_RESOLUTION/1000.);
	fprintf(logfile, "max at %ld, E(X)=%1.1f(%1.1fns), sigma(X)=%1.2f\n",
				m, ew,
				ew*G_RESOLUTION/1000.,
				sigma);

	fprintf(logfile, "\n");
	x=s;
	ln=0;
	fprintf(logfile, "0x%04lX:\n", s);
	while (1) {
		v0=(u_int)((79L*WBUF_L(x))/mx);

		c=0;
		do {
			if (v0 == c) {
				fprintf(logfile, "*");
				c++; break;
			}

			fprintf(logfile, "-");

		} while (++c < 80);

		fprintf(logfile, "\n");
		if ((x >= e) || (++ln >= 40)) break;

		x++;
	} 

	fprintf(logfile, ":0x%04lX\n", e);
	fprintf(logfile, "\n");
	return 0;
}
//--------------------------- GPX_bin_distribution ---------------------------
//
static int GPX_bin_distribution(void)
{
extern GPX_RG		*inp_unit;

	int		ret;
	char		key;
	u_long	x;
	u_int		h,i;
	u_int		lx,llx=0;
	u_int		s,e;
	u_int		m0,m1;			// Index Maximum gerader/ungerader Werte
	u_long	mx0,mx1;			// Maximum gerader/ungerader Werte
	float		wth0,wth1;		// Summe gerader/ungerader Werte
	u_long	mx;				// Maximalwert
	u_long	lp,zt;
	u_int		v0;
	u_int		ln,c;
	u_int		bt=1;

	x=0;
	do WBUF_L(x)=0; while (++x < LORG_MAX);

	if ((ret=inp_mode(IM_NODAQTRG)) != 0) return ret;

	h=(rout_hdr) ? HDR_LEN : 0;
	lprt_org=LORG_MAX; clrscr();
	s=0; lp=0; timemark();
	while (1) {
		if ((ret=read_event(EVF_NODAC, RO_RAW)) != 0) {
			if (!(ret &CE_KEY_PRESSED)) break;

			key=(char)ret;
			if (ret &CE_KEY_SPECIAL) {
				if ((key != LEFT) && (key != RIGHT)) break;

				if (lprt_org == LORG_MAX) continue;

				if (key == LEFT) {
					lprt_org -=(1L <<bt);
					if (lprt_org >= LORG_MAX) lprt_org=0;
				} else {
					lprt_org +=(1L <<bt);
					if (lprt_org > (LORG_MAX-PRT_X)) lprt_org=LORG_MAX-PRT_X;
				}

				gotoxy(1, PRT_Y0+PRT_Y+2);
				printf("org at %05lX(%1.1fns)        ", lprt_org, lprt_org*G_RESOLUTION/1000.);
				continue;
			}

			if ((key >= '0') && (key <= '9')) {
				if (key >= '5') key='4';
				bt=4*(key-'0');
				llx=0;
				continue;
			}

			if (key == CR) {
				lprt_org=LORG_MAX; clrscr();
				continue;
			}

			break;
		}
#if 0
		if ((i=inp_unit->gpx_state) &GPX_ERRFL) {
			inp_unit->gpx_seladdr=12; x=inp_unit->gpx_data;
			gotoxy(1, PRT_Y0+PRT_Y+3);
			printf("GPX error #%u reg12:%07lX    \n", ++s, x);
		}
#endif
		i=h; lx =rout_len >>2;
		if (llx < lx) llx=lx;
		while (i < lx) {
			WBUF_L(robf[i] &0x1FFFFL)++;
			i++;
		}

		lp++;
		if (!(lp &0x0FFF)) {
			GPX_print_distribution();
			gotoxy(40, PRT_Y0+PRT_Y+1);
			printf("len=%3u/%3u", lx, llx);
		}
	}

	zt=ZeitBin();
	printf("%04X\n", ret);
	printf("loops=0x%08lX\n", lp);
	if (zt) printf("%lu per sec\n", (100*lp)/zt);

	mx0=0; mx1=0; wth0=0.; wth1=0.; s=0xFFFF; i=0;
	do {
		if (!WBUF_L(i)) continue;

		if (s == 0xFFFF) s=i;	// start

		e=i;							// vorlaeufiges Ende
		if (!(i &1)) {
			wth0 +=WBUF_L(i);
			if (mx0 < WBUF_L(i)) { mx0 =WBUF_L(i); m0=i; }
			continue;
		}

		wth1 +=WBUF_L(i);
		if (mx1 < WBUF_L(i)) { mx1 =WBUF_L(i); m1=i; }

	} while (++i);

	if (!mx0 && !mx1) return ret;

	mx=(mx0 > mx1) ? mx0 : mx1;

	if (!logfile) return ret;

#define CPL		100
#define LPHP	49
	fprintf(logfile, "max0 at 0x%04X(%4.0fns) =%lu, area=%6.0f\n",
			  m0, m0*G_RESOLUTION/1000., mx0, wth0/mx0);
	fprintf(logfile, "max1 at 0x%04X(%4.0fns) =%lu, area=%6.0f, width=0x%04X(%1.0fns)\n",
			  m1, m1*G_RESOLUTION/1000., mx1, wth1/mx1, e-s+1, (e-s+1)*G_RESOLUTION/1000.);

	ln=0; i =s;
	fprintf(logfile, "0x%04X:\n", s);
	do {
		v0=(u_int)(((u_long)CPL*WBUF_L(i))/mx); i++;
		c=0;
		do {
			if (v0 == c) {
				fprintf(logfile, "*"); c++; break;
			}

			fprintf(logfile, "-");

		} while (++c < CPL);

		fprintf(logfile, "\n");
	} while (++ln < 2*LPHP);

	ln=0; i=(e+1) -(2*LPHP);
	fprintf(logfile, "0x%04X:\n", i);
	while (1) {
		v0=(u_int)(((u_long)CPL*WBUF_L(i))/mx);
		c=0;
		do {
			if (v0 == c) {
				fprintf(logfile, "*"); c++; break;
			}

			fprintf(logfile, "-");

		} while (++c < CPL);

		fprintf(logfile, "\n");
		if ((i >= e) || (++ln >= 2*LPHP)) break;

		i++;
	} 

	fprintf(logfile, ":0x%04X\n", e);
	fprintf(logfile, "\n");
	return ret;
}
//
//--------------------------- GPX_length_distribution -----------------------
//
static int GPX_length_distribution(void)
{
	int		ret;
	u_int		h,i;
	int		i0;
	u_int		lx;
	long		diff;
	u_long	lp,zt;
	u_long	le,te;
	u_char	err=0;
	u_int		ofl=0;
	u_int		cay=0;

	if ((config.f1_letra &0x03) != 0x3) {
		printf("select leading and trailing edge mode\n");
		return CEN_PROMPT;
	}

	i=0;
	do WBUF_L(i)=0; while (++i);

	if ((ret=inp_mode(0)) != 0) return ret;

	use_default=0;
	h=(rout_hdr) ? HDR_LEN : 0;
	lprt_org=LORG_MAX; clrscr();
	lp=0; timemark();
	while (1) {
		if ((ret=read_event(EVF_NODAC, RO_RAW)) != 0) {
			if (ret != (CE_KEY_PRESSED|CR)) break;

			lprt_org=LORG_MAX; clrscr();
			GPX_print_distribution();
			continue;
		}

		if ((lx =rout_len >>2) <= h) continue;

		i=h; i0=-1;
		lp++;
		do {
			if (i0 < 0) { i0=i; continue; }

			le=robf[i0]; te=robf[i];
//printf(" %d/%d %08lX/%08lX\n", i0, i, le, te);
			if (!(le &0x20000L) || (te &0x20000L)) {
				gotoxy(1, PRT_Y0+PRT_Y+2);
				printf("%u/%u no double edge\n", i0, i); err=1;
				break;
			}

			if ((le &0x3C0000L) == (te &0x3C0000L)) diff=(te &0x1FFFFL) -(le &0x1FFFFL);
			else {
				if (((le+0x40000L) &0x3C0000L) == (te &0x3C0000L)) {
					diff=(te &0x1FFFFL) +gpx_range -(le &0x1FFFFL);
					gotoxy(1, PRT_Y0+PRT_Y+3);
					printf("%u carry ", ++cay);
					i0=-2;
					continue;
				} else {
					gotoxy(1, PRT_Y0+PRT_Y+2);
					printf("%u/%u overrun\n", i0, i); err=1;
					break;
				}
			}

//printf(" %08lX\n", diff);
			if ((u_long)diff >= 0x1000L) {
				gotoxy(1, PRT_Y0+PRT_Y+2);
				printf("%u overflow ", ++ofl);
				i0=-2;
				continue;
			}

			WBUF_L(diff)++;
			if (!(lp &0x03FF)) {
				GPX_print_distribution();
			}

			i0=-2;
		} while (++i < lx);

		if (err) {
			printf("i0/i=%04X/%04X, len=%04X %X/%05lX %X/%05lX\n",
					 i0*4, i*4, lx*4,
					 (u_int)(le>>18)&0xF, le&0x1FFFFL,
					 (u_int)(te>>18)&0xF, te&0x1FFFFL);
			return CE_PROMPT;
		}
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
				  config.f1_sync, config.daq_mode, config.f1_letra, G_RESOLUTION);
		fprintf(logfile, "loops=%lu\n", lp);
		fprintf(logfile, "\n");
	}

	GPX_display_distribution();
	return ret;
}
//
//--------------------------- GPX_F1_comparing ------------------------------
//
#define F1_MOFFS	0			// 0x37
#define GPX_SOFFS	0x462		// 0x461

static int GPX_F1_comparing(void)
{
	int		ret;
	u_int		h,i;
	u_long	dat;
	u_int		un;
	int		fstrt,gstrt;
	long		fdat,gdat;
	u_int		lx;
	u_char	err=0;
	u_long	lp,zt;

	i=0;
	do WBUF_L(i)=0; while (++i);

	if ((ret=inp_mode(IM_NODAQTRG)) != 0) return ret;

	if (rout_hdr == MCR_DAQTRG) {
		printf("use raw mode\n");
		return CE_PARAM_ERR;
	}

	h=(rout_hdr) ? HDR_LEN : 0;
	prt_org=0xFFFF; clrscr();
	lp=0; timemark();
	while (1) {
		if ((ret=read_event(EVF_NODAC, RO_RAW)) != 0) {
			if (ret != (CE_KEY_PRESSED|CR)) break;

			prt_org=0xFFFF; clrscr();
			continue;
		}

		fstrt=-1; gstrt=-1;
		lx =rout_len >>2;
		for (i=h; i < lx; i++) {
			dat=robf[i];
			un=(u_int)(dat >>28);
			if (inp_type[un] == F1TDC) {
				if (fstrt >= 0) { err=30; break; }

				fstrt=(u_int)(dat >>16)&0xF;
				fdat=(u_int)dat;
			} else {
				if (inp_type[un] == GPXTDC) {
					if (gstrt >= 0) { err=31; break; }

					gstrt=(u_int)(dat>>18) &0xF;
					gdat=(dat &0x1FFFFL)-GPX_SOFFS;
					if (gdat < F1_MOFFS) { gdat+=gpx_range; gstrt=(gstrt-1)&0xF; }

					gdat=(3*gdat+2)/4;
				} else { err=32; break; }
			}

			if ((fstrt >= 0) && (gstrt >= 0)) {
				long		diff;

				if (fstrt == gstrt) diff=gdat-fdat;
				else
					if (((fstrt-1)&0xF) == gstrt) diff=gdat-(fdat+f1_range);
					else
						if (((fstrt+1)&0xF) == gstrt) diff=(gdat+f1_range)-fdat;
						else err=1;

				if (!err)
					if ((diff < -30) || (diff > 30)) {
						gotoxy(1, PRT_Y0+PRT_Y+2);
						printf("diff is %ld\n", diff); err=1;
					} else WBUF_L((u_int)(diff+0x8000))++;
			}

			if (err) break;
		}

		if (err) {
			gotoxy(1, PRT_Y0+PRT_Y+3);
			return err;
		}

		lp++;
		if (!(lp &0x03FF)) {
			print_distribution();
		}
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
		if (measconf.chan < 0)
			fprintf(logfile, "difference distribution of trigger => channel 0x%03X\n",
					  measconf.chan1);
		else {
			if (measconf.chan1 < 0)
				fprintf(logfile, "difference distribution of channel 0x%03X => trigger\n",
						  measconf.chan);
			else
				fprintf(logfile, "difference distribution of channel 0x%03X => 0x%03X\n",
						  measconf.chan, measconf.chan1);
		}

		fprintf(logfile, "f1_sync=%d, daq_mode=%d, f1_letra=%02X, resolution=%6.2fps\n",
				  config.f1_sync, config.daq_mode, config.f1_letra, RESOLUTION);
		fprintf(logfile, "loops=%lu\n", lp);
		fprintf(logfile, "\n");
	}

	display_distribution(1);
	return ret;
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
#if 1
//--------------------------- special ---------------------------------------
//
static int special(void)
{
	char		*str;
	int		hdl;
	u_int		i;
	u_long	x;
	u_long	mx,nx;
	u_int		v0,v1;
	u_int		c;
	u_int		ln;

	printf("enter file name: ");
	str=Read_String(0, 80);
	if (errno) return CEN_MENU;

	if (*str) {
		if ((hdl=_rtl_open(str, O_RDONLY)) == -1) {
			perror("error opening file");
			printf("name =%s\n", str);
			return CEN_PROMPT;
		}

		for (i=0; i < 16; i++)	// 8*64k => u_int *abuf[];
			if ((u_int)read(hdl, abuf[i >>1] +((i&1)<<14), 0x8000) != 0x8000) {
				perror("error reading file");
				printf("name =%s\n", str);
				close(hdl);
				return CEN_PROMPT;
			}

		close(hdl);
	}

	mx=0; x=0;
	do {
		if (!WBUF_L(x)) continue;

		if (mx < WBUF_L(x)) mx =WBUF_L(nx=x);

	} while (++x < LORG_MAX);

	if (!mx) {
		printf("no values\n");
		return CEN_PROMPT;
	}

	printf("max value at %05lX is %08lX\n", nx, mx);
	x=nx &~0x0F;
	while (1) {
		errno=0; use_default=0;
		printf("start at %05lX ", x);
		x=Read_Hexa(x, LORG_MAX-1);
		if (errno) break;

		if (x >= LORG_MAX-10) x =LORG_MAX-10;

		do {
			printf("%05lX:\n", x);
			ln=0;
			do {
//		printf("%*s*\n", (u_int)((79L*WBUF_L(x))/mx), "");
				v0=(u_int)((79L*WBUF_L(x))/mx); x++;
				v1=(u_int)((79L*WBUF_L(x))/mx); x++;
				c=0;
				do {
					if (v0 == c) {
						if (v0 == v1) { printf("Û"); c++; break; }

						printf("ß");
						if (v1 < v0) { c++; break; }

						continue;
					}

					if (v1 == c) {
						printf("Ü");
						if (v0 < v1) { c++; break; }

						continue;
					}

					printf(" ");

				} while (++c < 80);

				if (c < 80) Writeln();
			} while ((x < LORG_MAX) && (++ln < 48));

		} while (!WaitKey(0) && (x < LORG_MAX));
	}

	errno=0; use_default=0;
	if (open_logfile() || !logfile) return CE_PROMPT;

	for (x=0; x < 0x600; x++)
		fprintf(logfile, "%9lu %9lu\n", x, WBUF_L(x));

	for (x=0xEF00; x < 0x10970L; x++)
		fprintf(logfile, "%9lu %9lu\n", x, WBUF_L(x));

	close_logfile();
	return CE_PROMPT;
}
//
#endif
//===========================================================================
//
//--------------------------- measurement -----------------------------------
//
int measurement(void)
{
	u_int		i;
	char		rkey;
	int		ret;
	u_char	ni=0;

	while (1) {
		clrscr();
		i=0; no_init=0;
		gotoxy(SP_, i+++ZL_);
						printf("F1 value distribution ...........0  .n => ohne Init");
		gotoxy(SP_, i+++ZL_);
						printf("length distribution .............1");
		gotoxy(SP_, i+++ZL_);
						printf("difference distribution .........2");
		gotoxy(SP_, i+++ZL_);
						printf("distribution several channels ...3");
		gotoxy(SP_, i+++ZL_);
						printf("save bin distribution for 'X' ...4");
		gotoxy(SP_, i+++ZL_);
						printf("channel distribution ............5");
		gotoxy(SP_, i+++ZL_);
						printf("time distribution ...............6");
		gotoxy(SP_, i+++ZL_);
						printf("auto channel distribution .......7");
		gotoxy(SP_, i+++ZL_);
						printf("GPX value distribution ..........g");
		gotoxy(SP_, i+++ZL_);
						printf("GPX length distribution .........G");
		gotoxy(SP_, i+++ZL_);
						printf("GPX - F1 comparison .............f");
#if 0
		gotoxy(SP_, i+++ZL_);
						printf("special measurement 1 ...........4");
		gotoxy(SP_, i+++ZL_);
						printf("special measurement 2 ...........5");
#endif
		gotoxy(SP_, i+++ZL_);
						printf("display distribution buffer .....B");
		gotoxy(SP_, i+++ZL_);
						printf("graphical distribution buffer ...X and logfile");
		gotoxy(SP_, i+++ZL_);
						printf("open logfile ....................L");
		if (logfile) {
		gotoxy(SP_, i+++ZL_);
						printf("log info ........................I");
		}
		gotoxy(SP_, i+++ZL_);
						printf("setup ...........................s");
		gotoxy(SP_, i+++ZL_);
						printf("Ende ..........................ESC");
		gotoxy(SP_, i+++ZL_);
						printf("                                 %c ", measconf.menu_0);

		while (1) {
			rkey=getch();
			if (rkey == '.') { ni=no_init=1; continue; }

			if ((rkey == CTL_C) || (rkey == ESC)) {
				close_logfile();
				return 0;
			}

			break;
		}

		while (1) {
			use_default=(rkey == TAB);
			if ((rkey == TAB) || (rkey == CR)) { no_init=ni; rkey=measconf.menu_0; }

			ni=no_init;
//			clrscr(); gotoxy(1, 10);
			Writeln();
			errno=0; ret=CEN_MENU;
			switch (rkey) {

		case '0':
				ret=F1_bin_distribution();
				break;

		case '1':
				ret=length_distribution(0);
				break;

		case '2':
				ret=difference_distribution(0);
				break;

		case '3':
				ret=channel_distribution();
				break;

		case '4':
				ret=save_distribution();
				break;

		case '5':
				ret=channel_nr_distribution();
				break;

		case '6':
				ret=time_distribution();
				break;

		case '7':
				ret=multi_channel_distribution();
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

#if 1
		case 'x':
		case 'X':
				ret=special();
				break;
#endif

		case 'g':
				ret=GPX_bin_distribution();
				break;

		case 'G':
				ret=GPX_length_distribution();
				break;

		case 'f':
				ret=GPX_F1_comparing();
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
