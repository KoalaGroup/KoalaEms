// Borland C++ - (C) Copyright 1991, 1992 by Borland International

//	measurex.c -- statistic Messungen

#include "math.h"

static FILE		*logfile=0;

static struct time	timerec;
static struct date	daterec;

static u_char	no_init;
//
//--------------------------- sel_sc ----------------------------------------
//
static int sel_sc(void)
{
	if (sc_assign(2)) return -1;
#if 0
	if (config.sc_units <= LOW_SC_UNIT+1) sc_conf=config.sc_db +LOW_SC_UNIT+1;
	else
		while (1) {
			printf("select SC unit     %2d ", config.sc_unitnr);
			config.sc_unitnr=(int)Read_Deci(config.sc_unitnr, -1);
			if (errno) return -1;

			if (   (config.sc_unitnr >= LOW_SC_UNIT)
				 && (config.sc_unitnr < config.sc_units)) {
				sc_conf=config.sc_db +config.sc_unitnr+1;
				break;
			}

			printf("max number is %u\n", config.sc_units-1);
			use_default=0;
		}
	}
#endif

	return sc_conf->sc_addr;
}
//
//--------------------------- desel_sc --------------------------------------
//
static void desel_sc(void)
{
	int	sc;

	for (sc=LOW_SC_UNIT; sc < config.sc_units; sc++) {
		if (sc == config.sc_unitnr) continue;

		config.sc_db[sc+1].sc_unit->cr=0;
		sc_bc->transp=config.sc_db[sc+1].sc_addr;
		config.sc_db[sc+1].inp_bc->cr=0;
	}

}
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
//===========================================================================
//
//--------------------------- F1_bin_distribution ---------------------------
//
static int F1_bin_distribution(void)
{
	int		ret;
	int		sc;
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

	if ((sc=sel_sc()) < 0) return CEN_MENU;

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

	desel_sc();
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

		if (h && ((u_int)(robf[1]>>16) != sc)) continue;

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
//--------------------------- length_distribution ---------------------------
//
static int length_distribution(u_long cnt)
{
	int		ret;
	int		sc;
//	char		err=0;
	u_int		h,i;
	int		i0;
	u_int		lx;
	u_int		cha;
	long		diff;
	u_long	lp,zt;
//	u_long	le,te;

	if ((sc=sel_sc()) < 0) return CEN_MENU;

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

		desel_sc();
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

		if (h && ((u_int)(robf[1]>>16) != sc)) continue;

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
	int		sc;
	u_int		h,i;
	int		i0,i1;
	u_int		lx;
	u_int		cha;
	long		diff;
	u_long	lp,zt;

	if ((sc=sel_sc()) < 0) return CEN_MENU;

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

	desel_sc();
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
			if (h && ((u_int)(robf[1]>>16) != sc)) continue;

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
				if (h && ((u_int)(robf[1]>>16) != sc)) continue;

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

				if (h && ((u_int)(robf[1]>>16) != sc)) continue;

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
	int		sc;
	u_int		h,i;
	u_long	cmask[2*C_UNIT];
	u_int		cha;
	u_int		diff;
	u_long	lp,zt;

	if ((sc=sel_sc()) < 0) return CEN_MENU;

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

	desel_sc();
	h=(rout_hdr) ? HDR_LEN : 0;
	prt_org=0xFFFF; clrscr();
	lp=0; timemark();
	while ((ret=read_event(0, RO_TRG)) == 0) {
		if (h && ((u_int)(robf[1]>>16) != sc)) continue;

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

	for (i=0; i < 8; i++)
		if ((u_int)write(hdl, dmabuf[(i >>1)+4] +((i&1)<<13), 0x8000) != 0x8000) {
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
	int		sc;
	u_int		h,lx,i;
	u_long	lp,zt;
	u_long	tmp;
	u_int		cha;

	if ((sc=sel_sc()) < 0) return CEN_MENU;

	printf("first channel to display 0x%03X ", measconf.chan);
	measconf.chan=(int)Read_Hexa(measconf.chan, -1);
	if (errno) return CEN_MENU;

	i=0;
	do WBUF_L(i)=0; while (++i);

	if ((ret=inp_mode(0)) != 0) return ret;

	desel_sc();
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

		if (h && ((u_int)(robf[1]>>16) != sc)) continue;

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
	int		sc;
	u_int		h,lx,i;
	u_long	lp,zt;
	u_int		cha;
	u_long	tt;

	if ((sc=sel_sc()) < 0) return CEN_MENU;

	printf("selected channel 0x%03X ", measconf.chan1);
	measconf.chan1=(int)Read_Hexa(measconf.chan1, -1);
	if (errno) return CEN_MENU;

	i=0;
	do WBUF_L(i)=0; while (++i);

	if ((ret=inp_mode(0)) != 0) return ret;

	if (rout_hdr != MCR_DAQTRG) {
		printf("only for trigger mode\n");
		return CEN_PROMPT;
	}

	desel_sc();
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

		if (h && ((u_int)(robf[1]>>16) != sc)) continue;

		lx =rout_len >>2;
		for (i=h; i < lx; i++) {
			cha=EV_CHAN(robf[i]);
			if (cha != measconf.chan1) continue;

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
#if 0
//--------------------------- special_measurement ---------------------------
//
static int special_measurement(void)
{
	int		ret;

#if 0
	u_long	cnt=80L*60*1000;	// ca. 1000/sec

	use_default=1;
	config.f1_sync=2;
	config.f1_letra=0x11;
	if ((ret=general_setup(-2, -1)) != 0) return ret;

	config.daq_mode=1;
	measconf.chan=0x47;
	measconf.chan1=0x46;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=0x48;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=0x58;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=0xA8;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=-1;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	config.f1_sync=1;
	if ((ret=general_setup(-2, -1)) != 0) return ret;

	config.daq_mode=1;
	measconf.chan=0x47;
	measconf.chan1=0x46;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=0x48;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=0x58;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=0xA8;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=-1;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	return CEN_PROMPT;
#endif

#if 0
	u_long	cnt=45L*60*1000;	// ca. 1000/sec

	use_default=1;
	config.f1_sync=2;
	config.f1_letra=0x11;
	if ((ret=general_setup(-2, -1)) != 0) return ret;

	config.daq_mode=1;
	measconf.chan=0x47;
	measconf.chan1=0x46;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=0x48;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=0x58;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=0xA8;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=-1;
	if ((ret=difference_distribution(cnt)) != 0) return ret;
//
//---
//
	config.f1_sync=1;
	if ((ret=general_setup(-2, -1)) != 0) return ret;

	config.daq_mode=1;
	measconf.chan=0x47;
	measconf.chan1=0x46;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=0x48;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=0x58;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=0xA8;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=-1;
	if ((ret=difference_distribution(cnt)) != 0) return ret;
//
//--- das ganze mit Rueckflanke
//
	config.f1_sync=2;
	config.f1_letra=0x22;
	if ((ret=general_setup(-2, -1)) != 0) return ret;

	config.daq_mode=1;
	measconf.chan=0x47;
	measconf.chan1=0x46;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=0x48;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=0x58;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=0xA8;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=-1;
	if ((ret=difference_distribution(cnt)) != 0) return ret;
//
//---
//
	config.f1_sync=1;
	if ((ret=general_setup(-2, -1)) != 0) return ret;

	config.daq_mode=1;
	measconf.chan=0x47;
	measconf.chan1=0x46;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=0x48;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=0x58;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=0xA8;
	if ((ret=difference_distribution(cnt)) != 0) return ret;

	measconf.chan1=-1;
	if ((ret=difference_distribution(cnt)) != 0) return ret;
//
//--- Laengenmessung
//
	config.f1_sync=2;
	config.f1_letra=0x23;
	if ((ret=general_setup(-2, -1)) != 0) return ret;

	if ((ret=length_distribution(cnt)) != 0) return ret;

	config.f1_sync=1;
	if ((ret=general_setup(-2, -1)) != 0) return ret;

	if ((ret=length_distribution(cnt)) != 0) return ret;

	return CEN_PROMPT;
#endif

#if 1
	u_long	cnt=30*1000;	// ca. 1000/sec

	use_default=1;
	config.f1_sync=2;
	config.f1_letra=0x13;
	if ((ret=general_setup(-2, -1)) != 0) return ret;

	config.daq_mode=0x07;
	config.rout_delay=0x00;
	use_default=1;
	measconf.chan=0x04;
	if ((ret=length_distribution(cnt)) != 0) return ret;

	use_default=1;
	measconf.chan=0x08;
	if ((ret=length_distribution(cnt)) != 0) return ret;

	use_default=1;
	measconf.chan=0x14;
	if ((ret=length_distribution(cnt)) != 0) return ret;

	use_default=1;
	measconf.chan=0x18;
	if ((ret=length_distribution(cnt)) != 0) return ret;

	use_default=1;
	measconf.chan=0x24;
	if ((ret=length_distribution(cnt)) != 0) return ret;

	use_default=1;
	measconf.chan=0x28;
	if ((ret=length_distribution(cnt)) != 0) return ret;

	use_default=1;
	measconf.chan=0x34;
	if ((ret=length_distribution(cnt)) != 0) return ret;

	use_default=1;
	measconf.chan=0x38;
	if ((ret=length_distribution(cnt)) != 0) return ret;

	return CEN_PROMPT;
#endif
}
//
//--------------------------- special_measurement2 --------------------------
//
static int special_measurement2(void)
{
	int		ret;
	u_int		mt,df;
	long		lst,ddf,dddf;
	u_long	lpse;
	u_int		sc;

	if (config.sc_units <= LOW_SC_UNIT+1) {
		printf("nur mit min 2 Systemcontroller moeglich\n");
		return CEN_PROMPT;
	}

	config.daq_mode=EVF_HANDSH|MCR_DAQRAW|MCR_DAQTRG;
	config.rout_delay=0x40;
	use_default=1;
	if ((ret=inp_mode(0)) != 0) return ret;

	use_default=0; lst=0; dddf=0;
	while ((ret=read_event(EVF_NODAC, RO_RAW)) == 0) {
		if (rout_len < 4*HDR_LEN) return CE_FATAL;

		if (pause) {
			lpse=pause; sc=(u_int)(robf[1]>>16);
			mt=(u_int)rout_trg_time; continue;
		}

		if ((u_int)(robf[1]>>16) < sc) {
			df=(u_int)rout_trg_time -mt;
			if (mt > (u_int)rout_trg_time) df+=(u_int)f1_range;
		} else {
			df=mt-(u_int)rout_trg_time;
			if ((u_int)rout_trg_time > mt) df+=(u_int)f1_range;
		}

		ddf=lst-df;
		lst=df;
		if (ddf < 0) ddf+=f1_range;
		if (abs((int)(ddf-dddf)) > 100) {
			dddf=ddf;
			printf("%2d %04lX %7.1fns %7.1fns\n", sc, lpse, df*RESOLUTIONNS,
											 ddf*RESOLUTIONNS);
		}
	}

	return ret;
}
//
#endif
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
static int	blk;

	char		*str;
	int		hdl;
	u_int		i;
	u_long	mx;
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

		for (i=0; i < 8; i++)
			if ((u_int)read(hdl, dmabuf[(i >>1)+4] +((i&1)<<13), 0x8000) != 0x8000) {
				perror("error reading file");
				printf("name =%s\n", str);
				close(hdl);
				return CEN_PROMPT;
			}

		close(hdl);
	}

	mx=0; i=0;
	do {
		if (!WBUF_L(i)) continue;

		if (mx < WBUF_L(i)) mx =WBUF_L(i);

	} while (++i);

	if (!mx) return CEN_PROMPT;

	printf("number of block %2d ", blk);
	blk=(int)Read_Deci(blk, -1);
	if (errno) return CEN_MENU;

	if ((blk < 0) || (blk >= 16)) return CE_PARAM_ERR;

	i=0x1000*blk; ln=0;
	do {
//		printf("%*s*\n", (u_int)((79L*WBUF_L(i))/mx), "");
		v0=(u_int)((79L*WBUF_L(i))/mx); i++;
		v1=(u_int)((79L*WBUF_L(i))/mx); i++;
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

		if (++ln >= 48) {
			ln=0; printf("%04X: ", i);
			if (WaitKey(0)) return CEN_PROMPT;
		}

	} while (i);

	return CEN_PROMPT;
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
	char		key;
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
						printf("save distribution             ...4");
		gotoxy(SP_, i+++ZL_);
						printf("channel distribution ............5");
		gotoxy(SP_, i+++ZL_);
						printf("time distribution ...............6");
#if 0
		gotoxy(SP_, i+++ZL_);
						printf("special measurement 1 ...........4");
		gotoxy(SP_, i+++ZL_);
						printf("special measurement 2 ...........5");
#endif
		gotoxy(SP_, i+++ZL_);
						printf("display distribution buffer .....B");
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
			key=toupper(getch());
			if (key == '.') { ni=no_init=1; continue; }

			if ((key == CTL_C) || (key == ESC)) {
				close_logfile();
				return 0;
			}

			break;
		}

		while (1) {
			use_default=(key == TAB);
			if ((key == TAB) || (key == CR)) { no_init=ni; key=measconf.menu_0; }

			ni=no_init;
//			clrscr(); gotoxy(1, 10);
			Writeln();
			errno=0; ret=CEN_MENU;
			switch (key) {

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

#if 0
		case '4':
				ret=special_measurement();
				break;

		case '5':
				ret=special_measurement2();
				break;
#endif

		case 'B':
				ret=show_dbuf(-1);
				break;

		case 'L':
				open_logfile();
				break;

		case 'I':
				loginfo();
				ret=CEN_PROMPT;
				break;

		case 'S':
				ret=general_setup(-2, -1);
				if ((ret <= CE_PROMPT) || (ret >= CE_KEY_PRESSED)) ret=CEN_PROMPT;
				break;

#if 1
		case 'X':
				ret=special();
				break;
#endif

			}

			stop_ddma();
			while (kbhit()) getch();
			if ((ret &~0x1FF) == CE_KEY_PRESSED) ret=CE_PROMPT;

			if ((ret >= 0) && (ret <= CE_PROMPT)) measconf.menu_0=key;

			if ((ret >= CEN_MENU) && (ret <= CE_MENU)) break;

			if (ret > CE_PROMPT) display_errcode(ret);

			printf("select> ");
			key=toupper(getch());
			if ((key < ' ') && (key != TAB)) break;

			if (key == ' ') key=CR;
		}
	}
}
