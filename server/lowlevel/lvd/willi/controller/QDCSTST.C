// Borland C++ - (C) Copyright 1991, 1992 by Borland International

/*	qdcs.c -- slow QDC ADC  */

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
#include "seq.h"

#define _XF		1
#define _NS		12.5
#define _NSF	1
#define _MV		0.5
#define _FG		3

extern SQDCT_BC	*inp_bc;
extern SQDCT_RG	*inp_unit;

static FILE		*logfile=0;
#ifndef PBUS
static int		datafile=-1;
#endif
static SQ_DATA	*sqd;
static u_int	ld,li;
static u_int	hdr;
static char		offl;
static u_int	ro_seq;
static u_int	utmp;
static char		*pbuf;

//===========================================================================
//										qdcstst_menu                                  =
//===========================================================================
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
	char	ln[4];

	printf("logfile: "); errno=0;
	str=Read_String(config.logname, sizeof(config.logname));
	if (errno) {
		if (close_logfile()) return CEN_PROMPT;
		return CEN_MENU;
	}

	if (logfile && !strcmp(str, config.logname)) return 0;

	if (close_logfile()) return CEN_PROMPT;

	if (!*str) return 0;

	logfile=fopen(str, "r");
	if (logfile) {
		fclose(logfile); logfile=0;
		printf("overwride %s (Y) ", str);
		if (   Readln(ln, sizeof(ln))
			 || (ln[0] && (toupper(ln[0]) != 'Y') && (toupper(ln[0]) != 'J'))) {
			return CEN_MENU;
		}
	}

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
#ifndef PBUS
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
static int open_datafile(int rd)
{
	char	*str;

	printf("datfile: "); errno=0;
	str=Read_String(config.dlogname, sizeof(config.dlogname));
	if (errno) return CE_MENU;

	if ((datafile >= 0) && !strcmp(str, config.dlogname)) return 0;

	close_datafile();
	if (!*str) return 0;

	if (rd) {
		if ((datafile=_rtl_open(str, O_RDONLY)) == -1) {
			perror("error opening data file");
			printf("name =%s", str);
			return CE_PROMPT;
		}
	} else {
		if ((datafile=_rtl_creat(str, 0)) == -1) {
			perror("error creating data file");
			printf("name =%s", str);
			return CE_PROMPT;
		}
	}

	strcpy(config.dlogname, str);
	return 0;
}
//
//--------------------------- qdc_logdata -----------------------------------
//
static int qdc_logdata(void)
{
	int		ret;
	u_int		tmp;
	DAT_HDR	dhdr;
	u_long	dlen;
	u_long	lp=0;

	printf("channel  %5i. ", config.sq_cha);
	config.sq_cha=(u_int)Read_Deci(config.sq_cha, 1);
	if (errno) return CEN_MENU;

	if (config.sq_cha > 1) config.sq_cha=1;
	sqd=config.sq_data +config.sq_cha;
	printf("latency  %5i. ", sqd->latency);
	sqd->latency=(u_int)Read_Deci(sqd->latency, -1);
	if (errno) return CEN_MENU;

	printf("window   %5i. ", sqd->window);
	sqd->window=(u_int)Read_Deci(sqd->window, -1);
	if (errno) return CEN_MENU;

	printf("raw mode %5i. ", tmp=(sqd->bool &_RAW) != 0);
	tmp=(u_int)Read_Deci(tmp, 1);
	if (errno) return CEN_MENU;

	sqd->bool &=~_RAW;
	if (tmp) sqd->bool |=_RAW;

	inp_unit->cr=0;
	tmp=Q_ENA;
	if (sqd->bool &_RAW) tmp |=QST_RAW;
	if (sqd->bool &_ITG) {	// internal trigger
		tmp |=Q_LTRIG;
		if (sqd->tlevel) tmp |=Q_LEVTRG;
	}

	if (sqd->bool &_EDG) tmp |=QST_CLKPOL;
	if (sqd->bool &_POL) tmp |=QST_ADCPOL;
	tmp |=((config.sq_cha) ? QST_DIS0 : QST_DIS1);
	inp_unit->latency=sqd->latency;
	inp_unit->window=sqd->window;

	if ((ret=open_datafile(0)) != 0) return ret;
	if (datafile < 0) return CEN_MENU;

	dhdr.hd[0]=0x80000000L |sizeof(DAT_HDR);
	dhdr.hd[1]=inp_unit->ident;
	dhdr.hd[2]=0;
	dhdr.dac0=sqd->dac0;
	dhdr.dac1=sqd->dac1;
	dhdr.lat=inp_unit->latency;
	dhdr.win=inp_unit->window;
	dhdr.ntol=inp_unit->mean_tol;
	dhdr.tlev=inp_unit->trig_level;
	dhdr.cr=tmp;
	dhdr.mean=inp_unit->mean_level[config.sq_cha];
	ret=inp_unit->mean_noise[config.sq_cha];
	delay(500);
	dhdr.noise=inp_unit->mean_noise[config.sq_cha];
	if (write(datafile, &dhdr, sizeof(DAT_HDR)) != sizeof(DAT_HDR)) {
		perror("error writing config file");
		printf("name =%s", config.dlogname);
		close_datafile();
		return CEN_PROMPT;
	}

	dlen=sizeof(DAT_HDR);

	inp_unit->cr=tmp;
	{
		u_int		i;

		for (i=0; i < 16; i++) inp_offl[i] =(i != sc_conf->slv_sel);
	}

	if (sqd->bool &_ITG) {	// internal trigger
		config.daq_mode=EVF_HANDSH|MCR_DAQRAW;
		config.rout_delay=0;
	} else
		config.daq_mode=EVF_HANDSH|MCR_DAQTRG;

	inp_mode(IM_QUIET);
	kbatt_msk=0;

	while (1) {
		if ((ret=read_event(EVF_NODAC, RO_RAW)) != 0) {
			printf("\r%5lu\n", lp);
			close_datafile();
			return ret;
		}

		if (write(datafile, dmabuf[0], rout_len) != rout_len) {
			perror("error writing config file");
			printf("name =%s", config.dlogname);
			close_datafile();
			return CEN_PROMPT;
		}

		dlen+=rout_len;
		if (dlen >= 0x08000000L) {
			printf("\r%5lu\n", lp);
			close_datafile();
			return ret;
		}

		if (!(++lp &0x0F)) printf("\r%5lu", lp);
	}
}
//
#endif
//--------------------------- logdata ---------------------------------------
//
static int logdata(int csv)
{
	int		ret;
	u_int		i;
	u_int		id;
	u_long	tmp;

	if ((ret=open_logfile()) != 0) return ret;

	if (!logfile) return 0;

	if (csv) {
		int	mean=(int)dmabuf[0][ld+1];

		i=inp_unit->cr;
		fprintf(logfile, "** len  %4u/%u\n", ld-hdr, li-ld);
		fprintf(logfile, "** lat  %04X\n", inp_unit->latency);
		fprintf(logfile, "** win  %04X\n", inp_unit->window);
		fprintf(logfile, "** ntol %04X\n", inp_unit->mean_tol);
		if (i &Q_LEVTRG)
			fprintf(logfile, "** tlv  %04X\n", inp_unit->trig_level);

		fprintf(logfile, "** cr   %04X\n", i &~QST_CLKPOL);
		for (i=ld; i < li; i++) {
			fprintf(logfile, "0x%08lX;", dmabuf[0][i]);
		}
		fprintf(logfile, "\n");

		for (i=ld; i < li; i++) {
			tmp=dmabuf[0][i]; id=(u_int)(tmp >>16) &0x3F;
			if ((id == 0x30) || (id == 0x32) || (id == 0x34)) {
				if (id+1 != ((u_int)(dmabuf[0][i+1] >>16) &0x3F)) break;

				switch (id) {
		case 0x30:
					if (i == ld) fprintf(logfile, "** start%6.1fns %6.1fmV mean",
												((u_int)dmabuf[0][ld]-(int)tmp)*25.0/16.0,
												(int)dmabuf[0][i+1]*_MV);
					else         fprintf(logfile, "** end  %6.1fns %6.1fmV",
												((u_int)dmabuf[0][ld]-(int)tmp)*25.0/16.0,
												(int)dmabuf[0][i+1]*_MV);
					break;

		case 0x32:
					fprintf(logfile, "** min  %6.1fns %6.1fmV",
							  ((u_int)dmabuf[0][ld]-(int)tmp)*25.0/16.0,
							  (int)dmabuf[0][i+1]*_MV);
					break;

		case 0x34:
					fprintf(logfile, "** max  %6.1fns %6.1fmV",
							  ((u_int)dmabuf[0][ld]-(int)tmp)*25.0/16.0,
							  (int)dmabuf[0][i+1]*_MV);
					break;
				}

				i++;
				continue;
			}

			if (id == 0x36) {
				id=(u_int)tmp>>14;
				if (tmp &0x2000) tmp |=0xC000; else tmp &=~0xC000;

				fprintf(logfile, "** zcros%6.1fns    %u",
						  ((u_int)dmabuf[0][ld]-(int)tmp)*25.0/16.0,
						  id);
				continue;
			}

			if ((id &0x30) == 0x20) {
				fprintf(logfile, "** charg %7.2fVns", (tmp &0xFFFFFl)*_MV*_NS/1000.0);
				continue;
			}

			break;
		}

		fprintf(logfile, "\n");

		for (i=hdr; i < ld; i++) {
			sprintf(pbuf, "%1.*f;%1.1f",
					  _NSF, (i-hdr)*_NS, ((int)dmabuf[0][i]-mean)*_MV);
			for (id=0; pbuf[id]; id++)
				if (pbuf[id] == '.') pbuf[id]=',';

			fprintf(logfile, "%s\n", pbuf);
		}

		close_logfile();
		return 0;
	}

	i=inp_unit->cr;
	fprintf(logfile, "-- len  %4u/%u\n", ld-hdr, li-ld);
	fprintf(logfile, "-- lat  %04X\n", inp_unit->latency);
	fprintf(logfile, "-- win  %04X\n", inp_unit->window);
	fprintf(logfile, "-- ntol %04X\n", inp_unit->mean_tol);
	if (i &Q_LEVTRG)
		fprintf(logfile, "-- tlv  %04X\n", inp_unit->trig_level);

	fprintf(logfile, "-- cr   %04X\n", i &~QST_CLKPOL);
	for (i=ld; i < li; i++) {
		fprintf(logfile, "-- %08lX\n", dmabuf[0][i]);
	}

	if (i &QST_ADCPOL) {
		for (i=hdr; i < ld; i++)
			fprintf(logfile, "%04X%04X %04X\n",
					  (u_int)dmabuf[0][i]^0xFFF, (u_int)dmabuf[0][i]^0xFFF,
					  (u_int)dmabuf[0][i]);

		fprintf(logfile, "%04X%04X %04X\n",
				  (u_int)dmabuf[0][ld+1]^0xFFF, (u_int)dmabuf[0][ld+1]^0xFFF,
				  (u_int)dmabuf[0][ld+1]);
	} else {
		for (i=hdr; i < ld; i++)
			fprintf(logfile, "%04X%04X\n",
					  (u_int)dmabuf[0][i], (u_int)dmabuf[0][i]);

		fprintf(logfile, "%04X%04X\n",
				  (u_int)dmabuf[0][ld+1], (u_int)dmabuf[0][ld+1]);
	}

	close_logfile();
	return 0;
}
//
//--------------------------- qdc_statistics --------------------------------
//
//--------------------------- print_distribution ----------------------------
//
#define PRT_X	80
#define PRT_Y0	46
#define PRT_Y	45
static u_char	prt_dis[PRT_X];
static u_int	prt_org;			// Null Index

static void print_distribution(void)
{
	u_int		i,n;
	u_long	mx;	// max
	u_char	dis[PRT_X];

	if (prt_org == 0xFFFF) {
		u_char	rising=1;
		u_long	last=0;

		gotoxy(1, PRT_Y0); printf(
"--------------------------------------------------------------------------------");
		for (n=0; n < PRT_X; prt_dis[n++]=0);

		mx=0; i=0;
		do {
			if (!WBUF_L(i)) continue;

			if (mx < WBUF_L(i)) { mx =WBUF_L(i); n=i; }

			if (rising && (WBUF_L(i) < last)) {
//				printf("max at %04X(%1.1fns)\n", i+0x7FFF, (int)(i+0x7FFF)*25.0/16.0);
				rising=0;
			} else {
				if (WBUF_L(i) > last) rising=1;
			}

			last=WBUF_L(i);
		} while (++i);

		if (!mx) { gotoxy(1, PRT_Y0+1); return; }

		prt_org=n-(PRT_X/2-1);
		if (prt_org > (0xFFFF-(PRT_X-1))) prt_org=0xFFFF-(PRT_X-1);

	} else {
		for (mx=0, i=prt_org, n=0; n < PRT_X; i++, n++)
			if (mx < WBUF_L(i)) mx =WBUF_L(i);
	}

	if (!mx) return;

	for (i=prt_org, n=0; n < PRT_X; i++, n++) {
		dis[n]=(u_int)(((PRT_Y-1)*WBUF_L(i))/mx);
		if (!dis[n] && WBUF_L(i)) dis[n]=1;
	}

	for (n=0; n < PRT_X; n++) {
		if (prt_dis[n] == dis[n]) continue;

//gotoxy(1, PRT_Y0+1);
//printf("\r%2d %2d ", dis[n], prt_dis[n]); if (WaitKey(0)) return;
		if (dis[n] >= prt_dis[n]) {
			i=dis[n];
			gotoxy(n+1, PRT_Y0-i); printf("*");
			while (--i >= prt_dis[n]) {
				gotoxy(n+1, PRT_Y0-i); printf("|");
				if (!i) break;
			}
		} else {
			i=prt_dis[n];
			while (i > dis[n]) { gotoxy(n+1, PRT_Y0-i); printf(" "); i--; }

			gotoxy(n+1, PRT_Y0-i); printf("*");
		}

		prt_dis[n]=dis[n];
	}

	gotoxy(1, PRT_Y0+1);
}
//
static int qdc_statistics(void)
{
	u_int		tmp;
	int		ret;
	u_int		i;
	u_long	lp;
	char		rkey,key;
	u_char	mx=3;

	printf("channel  %5i. ", config.sq_cha);
	config.sq_cha=(u_int)Read_Deci(config.sq_cha, 1);
	if (errno) return CEN_MENU;

	if (config.sq_cha > 1) config.sq_cha=1;
	sqd=config.sq_data +config.sq_cha;
	printf("latency  %5i. ", sqd->latency);
	sqd->latency=(u_int)Read_Deci(sqd->latency, -1);
	if (errno) return CEN_MENU;

	printf("window   %5i. ", sqd->window);
	sqd->window=(u_int)Read_Deci(sqd->window, -1);
	if (errno) return CEN_MENU;

	printf("raw mode %5i. ", tmp=(sqd->bool &_RAW) != 0);
	tmp=(u_int)Read_Deci(tmp, 1);
	if (errno) return CEN_MENU;

	sqd->bool &=~_RAW;
	if (tmp) sqd->bool |=_RAW;

	inp_unit->cr=0;
	tmp=Q_ENA;
	if (sqd->bool &_RAW) tmp |=QST_RAW;
	if (sqd->bool &_ITG) {	// internal trigger
		tmp |=Q_LTRIG;
		if (sqd->tlevel) tmp |=Q_LEVTRG;
	}

	if (sqd->bool &_EDG) tmp |=QST_CLKPOL;
	if (sqd->bool &_POL) tmp |=QST_ADCPOL;
	tmp |=((config.sq_cha) ? QST_DIS0 : QST_DIS1);
	inp_unit->latency=sqd->latency;
	inp_unit->window=sqd->window;
	inp_unit->cr=tmp;

#ifdef PBUS
	inp_mode(IM_VERTEX);
#else
	{
		u_int	i;

		for (i=0; i < 16; i++) inp_offl[i] =(i != sc_conf->slv_sel);
	}

	if (sqd->bool &_ITG) {	// internal trigger
		config.daq_mode=EVF_HANDSH|MCR_DAQRAW;
		config.rout_delay=0;
	} else
		config.daq_mode=EVF_HANDSH|MCR_DAQTRG;

	inp_mode(IM_QUIET);
	kbatt_msk=0;
#endif
	hdr=rout_hdr ? HDR_LEN : 0;
	i=0;
	do WBUF_L(i)=0; while (++i);

	prt_org=0xFFFF; clrscr();
	lp=0;
	while (1) {
		if ((ret=read_event(EVF_NODAC, RO_RAW)) != 0) {
			if ((ret &(CE_KEY_PRESSED|CE_KEY_SPECIAL)) != CE_KEY_PRESSED) {
				gotoxy(1, 49); Writeln();
				return ret;
			}

			use_default=0;
			key=toupper(rkey=(char)ret);
			if (rkey == ESC) {
				gotoxy(1, 49); Writeln();
				return CEN_PROMPT;
			}

			if (key == CR) {
				u_long	last;
				u_char	rising=1;

				gotoxy(1, PRT_Y0+1);
				last=0; i=0; tmp=0;
				do {
					if (rising && (WBUF_L(i) < last)) {
						printf("max at %04X(%1.1fns)   ", i+0x7FFF, (int)(i+0x7FFF)*25.0/16.0);
						if (++tmp == 3) { Writeln(); tmp=0; }
						rising=0;
					} else {
						if (WBUF_L(i) > last) rising=1;
					}

					last=WBUF_L(i);
				} while (++i);

				continue;
			}

			if ((key >= '0') && (key <= '4')) {
				if (key == '4') {
					mx=3;
					inp_unit->cr=((tmp=inp_unit->cr) |QST_SGRD);
				} else {
					mx=key-'0';
					inp_unit->cr=((tmp=inp_unit->cr) &~QST_SGRD);
				}

				i=0;
				do WBUF_L(i)=0; while (++i);

				prt_org=0xFFFF; clrscr();
				continue;
			}

			gotoxy(1, 49); Writeln();
			return CEN_PROMPT;
		}

		li=rout_len/4; ld=li;
//		while ((ld > hdr) && (dmabuf[0][ld-1] &0x00200000L)) ld--;
ld=hdr;
while ((ld < li) && !(dmabuf[0][ld] &0x00200000L)) ld++;
li=ld;
while ((li < rout_len/4) && (dmabuf[0][li] &0x00200000L)) li++;

		for (i=ld; i < li; i++) {
			if (((u_int)(dmabuf[0][i] >>16) &0x3F) == 0x36) {
				u_char	id;

				tmp=(u_int)dmabuf[0][i]; id=(u_int)tmp>>14;
				if (tmp &0x2000) tmp |=0xC000; else tmp &=~0xC000;

				if ((mx == 3) || (mx == id)) WBUF_L(tmp+0x8000)++;

				break;
			}
		}

		if (i == li) {
			gotoxy(1,1);
			printf("no data %u./%u. \n", ld, li);
			for (i=ld; i < li; i++) {
				printf("%08lX\n", dmabuf[0][i]);
			}

			logdata(0);
			gotoxy(1,49);
			return CEN_PROMPT;
		}

		lp++;
		if (!(lp &0x0FF)) print_distribution();

	}
}
//
//--------------------------- qdc_sample ------------------------------------
//
#define PRT_X		80
#define QPRT_Y		44
#define QPRT_Y0	45

static u_char	prt_dis[PRT_X];
static u_char	prt_mdis;
static u_int	dq_len=0x10;

static void cmd_string(char *str)
{
	u_int	i;

	i=0;
	while (str[i]) {
		if (str[i] == '_') {
			i++;
			if (!str[i]) break;

			highvideo(); cprintf("%c", str[i++]); lowvideo();
			continue;
		}

		cprintf("%c", str[i++]);
	}
}

static void smp_adjust(int ol)
{
	u_int		i;
	u_int		tmp;

	if (config.sq_cha > 1) config.sq_cha=1;
	sqd=config.sq_data +config.sq_cha;
	clrscr();
	gotoxy(1, QPRT_Y0);
	for (i=0; i < 80;) {
		if (i == 0) { printf("%X", sqd->shift &0xF); i++; continue; }

		if ((i&3) == 1) { printf("--"); i+=2; continue; }

		if (i == 79) {
			printf("%X", ((sqd->shift +(i+1)*(sqd->comp+1)) >>4) &0xF);
			i++; continue;
		}

		printf("%02X", (sqd->shift +(i+1)*(sqd->comp+1)) &0xFF);
		i+=2;
	}

	prt_mdis=255;
	for (i=0; i < PRT_X; prt_dis[i++]=255);

	if ((sqd->a_max < sqd->a_min) || ((sqd->a_max-sqd->a_min) < 2*QPRT_Y)) {
		sqd->a_max =sqd->a_min+2*QPRT_Y;
		if (sqd->a_max >= 0x1000) { sqd->a_max=0x1000; sqd->a_min=0x1000-2*QPRT_Y; }
	}

	inp_unit->cr=0;
	inp_unit->latency=sqd->latency;
	inp_unit->window=sqd->window;
	sqd->window=inp_unit->window;
	inp_unit->mean_tol=sqd->noise;
	sqd->noise=inp_unit->mean_tol;
	inp_unit->trig_level=sqd->tlevel;
	inp_unit->dac[config.sq_cha]=0x2000 |(sqd->dac0 &0xFFF);
	utmp=inp_unit->sr;	// delay
	inp_unit->dac[config.sq_cha]=0x6000 |(sqd->dac1 &0xFFF);

	gotoxy(1, QPRT_Y0+3);
	cmd_string("_cha=");
	printf("%u", config.sq_cha);
	cmd_string(", _lo/_up limit=");
	printf("%04X/%04X", sqd->a_min, sqd->a_max);
	cmd_string(", _Shi=");
	printf("%u.", sqd->shift);
	cmd_string(", _dAC0/_D1=");
	printf("%04X/%04X", sqd->dac0, sqd->dac1);
	cmd_string(", c_pr=");
	printf("%3u.        ", sqd->comp);
	gotoxy(1, QPRT_Y0+4);
#ifdef PBUS
	sqd->bool |=_ITG;
#else
	if (sqd->bool &_ITG) cmd_string("InTr_ig, "); else cmd_string("ExTr_ig, ");
#endif
	cmd_string("_Lat=");
	printf("%i.", inp_unit->latency);
	cmd_string(", _win=");
	printf("%i.", inp_unit->window);
	cmd_string(", co_nf=");
	printf("%04X", inp_unit->mean_tol);
	cmd_string(", tle_v=");
	printf("%u.", inp_unit->trig_level);
	cmd_string(", p_ol=");
	printf((sqd->bool &_POL) ? "-" : "+");
	cmd_string(", s_grd=");
	printf((sqd->bool &_GRD) ? "T" : "F");
	cmd_string("   press _h for help     ");

	tmp=QST_RAW|Q_ENA;
	if (sqd->bool &_ITG) {	// internal trigger
		tmp |=Q_LTRIG;
		if (sqd->tlevel) tmp |=Q_LEVTRG;
	}

	if (sqd->bool &_EDG) tmp |=QST_CLKPOL;
	if (sqd->bool &_POL) tmp |=QST_ADCPOL;
	if (sqd->bool &_GRD) tmp |=QST_SGRD;
	tmp |=((config.sq_cha) ? QST_DIS0 : QST_DIS1);
	inp_unit->cr=tmp;
	if (offl || ol) {
		hdr=HDR_LEN; return;
	}

#ifdef PBUS
	inp_mode(IM_VERTEX);
#else
	{
		u_int	i;

		for (i=0; i < 16; i++) inp_offl[i] =(i != sc_conf->slv_sel);
	}

	if (sqd->bool &_ITG) {	// internal trigger
		config.daq_mode=EVF_HANDSH|MCR_DAQRAW;
		config.rout_delay=0;
	} else
		config.daq_mode=EVF_HANDSH|MCR_DAQTRG;

	inp_mode(IM_QUIET);
	kbatt_msk=0;
#endif
	hdr=rout_hdr ? HDR_LEN : 0;
}

static int rd_event(void)
{
#ifdef PBUS
	return read_event(EVF_NODAC, RO_RAW);
#else
	int	ret;

	if (!offl) return read_event(EVF_NODAC, RO_RAW);

	if (datafile < 0) return CE_PROMPT;

	if (kbhit()) {
		ret=getch();
		if (ret) return CE_KEY_PRESSED|ret;

		return CE_KEY_PRESSED|CE_KEY_SPECIAL|getch();
	}

	if (rout_len) return 0;

	if ((ret=read(datafile, dmabuf[0], 4*HDR_LEN)) != 4*HDR_LEN) {
		gotoxy(1, 1);
		printf("%04X  \n", ret);
		return CE_PROMPT;
	}

	if ((dmabuf[0][0]>>16) != 0x8000) return CE_PROMPT;

	rout_len=(u_int)dmabuf[0][0];
	if (rout_len <= 4*HDR_LEN) return 0;

	if (read(datafile, dmabuf[0]+HDR_LEN, rout_len-4*HDR_LEN) != rout_len-4*HDR_LEN) return CE_PROMPT;

	if (ro_seq == 0xFFFF) {
		DAT_HDR	*qconf=(DAT_HDR*)dmabuf[0];

		if ((u_int)qconf->hd[1] != inp_unit->ident) {
			printf("\nwrong QDC type\n");
			return CE_PROMPT;
		}

		inp_unit->latency=sqd->latency=qconf->lat;
		inp_unit->window=sqd->window=qconf->win;
		inp_unit->mean_tol=sqd->window=qconf->ntol;
		inp_unit->trig_level=sqd->tlevel=qconf->tlev;
		ro_seq=0; rout_len=0;
		return rd_event();
	}

	ro_seq++;
	return 0;
#endif
}

static int qdc_sample(void)
{
	int		ret;
	u_int		i,n,tmp;
	u_int		imx=0,imn=0;
	char		rkey,key;
	u_char	single=0;
	u_char	dis;
	u_char	mdis;
	u_char	con;

	smp_adjust(0);
	use_default=0;
	rout_len=0; ld=0; li=0;
	con=1; ro_seq=0xFFFF;
	while (1) {
		if (con) { _setcursortype(_NOCURSOR); con=0; }

		if ((ret=rd_event()) != 0) {
			_setcursortype(_NORMALCURSOR); con=1;
			if ((ret &(CE_KEY_PRESSED|CE_KEY_SPECIAL)) != CE_KEY_PRESSED) {
				gotoxy(1, QPRT_Y0+5);
				return ret;
			}

			use_default=0;
			key=toupper(rkey=(char)ret);
			if (rkey == ESC) {
				gotoxy(1, QPRT_Y0+5);
				return CE_PROMPT;
			}

			gotoxy(1, QPRT_Y0+2);
			if ((key == '?') || (key == 'H')) {
				clrscr();
				gotoxy(1, 26);
				printf("CR  Information min, max and start value of display; mean and noise level\n");
				printf("s   step\n");
				printf("c   toggle channel, 0 or 1\n");
				printf("l   lower edge of display window\n");
				printf("u   upper edge of display window\n");
				printf("a   adjust values for lower and upper edge\n");
				printf("f   full range\n");
				printf("g   single gradient\n");
				printf("p   compress factor for time axis\n");
				printf("S   shift timing left\n");
				printf("m   toggle displaying mean line\n");
				printf("o   change signal polarity\n");
				printf("d   set DAC 0, increase mean level\n");
				printf("D   set DAC 1, decrease mean level\n");
				printf("L   latency, -512..511(*12.5ns)\n");
				printf("w   window, max 511(*12.5ns)\n");
				printf("i   toggle internal external trigger\n");
				printf("v   trigger level, 0=LEMO input\n");
				printf("e	change clock edge to clock ADC data\n");
				WaitKey(0);
				smp_adjust(0);
				continue;
			}

			if (key == CR) {
				gotoxy(26, QPRT_Y0+2);
				printf("min/max(dif)/mean/noi=%03X/%03X(%u.)/%03X/%u.    ",
						 imn, imx, imx-imn,
						 inp_unit->mean_level[config.sq_cha],
						 inp_unit->mean_noise[config.sq_cha]);
				gotoxy(26, QPRT_Y0+5);
				printf("len=%03X/%X, strt=%08lX/%08lX", ld, li-ld, dmabuf[0][ld], dmabuf[0][ld+1]);
				continue;
			}

			if (key == 'C') {
				config.sq_cha=config.sq_cha ? 0 : 1;
				smp_adjust(0);
				continue;
			}

			if (rkey == 's') {
				if (single == 1) single=0; else single=1;
				continue;
			}

			if (rkey == ' ') {
				if (single == 2) single=0; else single=2;
				continue;
			}

			if (key == 'G') {
				if (sqd->bool &_GRD) sqd->bool &=~_GRD; else sqd->bool |=_GRD;
				smp_adjust(0);
				continue;
			}

			if (rkey == 'l') {
				printf("%25s\r", "");
				printf("lower lim  %04X ", sqd->a_min);
				sqd->a_min=(u_int)Read_Hexa(sqd->a_min, -1);
				smp_adjust(0);
				continue;
			}

			if (key == 'U') {
				printf("%25s\r", "");
				printf("upper lim  %04X ", sqd->a_max);
				sqd->a_max=(u_int)Read_Hexa(sqd->a_max, -1);
				smp_adjust(0);
				continue;
			}

			if (rkey == 'S') {
				printf("%25s\r", "");
				printf("shift     %4u. ", sqd->shift);
				sqd->shift=(u_int)Read_Deci(sqd->shift, -1);
				smp_adjust(0);
				continue;
			}

			if (key == 'P') {
				printf("%25s\r", "");
				printf("compress  %4u. ", sqd->comp);
				sqd->comp=(u_int)Read_Deci(sqd->comp, -1);
				smp_adjust(0);
				continue;
			}

			if (rkey == 'm') {
				sqd->bool ^=_MEA;
				smp_adjust(0);
				continue;
			}

			if (rkey == 'L') {
				printf("%25s\r", "");
				printf("latency  %5i. ", sqd->latency);
				sqd->latency=(u_int)Read_Deci(sqd->latency, -1);
				smp_adjust(0);
				continue;
			}

			if (key == 'W') {
				printf("%25s\r", "");
				printf("window   %5i. ", sqd->window);
				sqd->window=(u_int)Read_Deci(sqd->window, -1);
				smp_adjust(0);
				continue;
			}

			if (key == 'N') {
				printf("%25s\r", "");
				printf("config   %04X ", sqd->noise);
				sqd->noise=(u_int)Read_Hexa(sqd->noise, -1);
				smp_adjust(0);
				continue;
			}

			if (key == 'V') {
				u_int	tl=sqd->tlevel;

				printf("%25s\r", "");
				printf("trg level %4u. ", sqd->tlevel);
				sqd->tlevel=(u_int)Read_Deci(sqd->tlevel, -1);
				if (sqd->tlevel) sqd->bool |=_ITG;
				else {
					if (tl) sqd->bool &=~_ITG;
				}

				smp_adjust(0);
				continue;
			}

			if (key == 'O') {
				if (sqd->bool &_POL) sqd->bool &=~_POL; else sqd->bool |=_POL;
				smp_adjust(0);
				continue;
			}

			if (rkey == 'i') {
				if (sqd->bool &_ITG) { sqd->bool &=~_ITG; sqd->tlevel=0; }
				else sqd->bool |=_ITG;
				smp_adjust(0);
				continue;
			}

			if (rkey == 'd') {
				printf("%25s\r", "");
				printf("DAC 0  %04X ", sqd->dac0);
				sqd->dac0=(u_int)Read_Hexa(sqd->dac0, -1);
				smp_adjust(0);
				continue;
			}

			if (rkey == 'D') {
				printf("%25s\r", "");
				printf("DAC 1  %04X ", sqd->dac1);
				sqd->dac1=(u_int)Read_Hexa(sqd->dac1, -1);
				smp_adjust(0);
				continue;
			}

			if (key == 'E') {
				sqd->bool ^=_EDG;
				smp_adjust(0);
				continue;
			}

			if (key == 'A') {
				if ((imx-imn) < 2*(QPRT_Y-2)) {
					sqd->a_min=(imx+imn)/2 -QPRT_Y;
					sqd->a_max=imx;
				} else {
					sqd->a_min=imn-(imx-imn)/20;
					if (sqd->a_min >= 0x1000) sqd->a_min=0;
					sqd->a_max=imx+(imx-imn)/20;
					if (sqd->a_max >= 0x1000) sqd->a_max=0x1000;
				}

				smp_adjust(0);
				continue;
			}

			if (key == 'F') {
				sqd->a_min=0;
				sqd->a_max=0x1000;
				smp_adjust(0);
				continue;
			}

			if ((rkey == 'M') || (key == 'X')) {
				printf("%25s\r", "");
				logdata(key == 'X');
				continue;
			}

			if (rkey == 'I') {
				u_long	tmp;
				u_int		id;

				gotoxy(1, 1);
				for (i=ld; i < li; i++) {
					tmp=dmabuf[0][i]; id=(u_int)(tmp >>16) &0x3F;
					if ((id == 0x30) || (id == 0x32) || (id == 0x34) || (id == 0x36))
						printf("  %04X%04X",
								 (u_int)(tmp >>16),
								 ((u_int)dmabuf[0][ld]-(u_int)tmp) >>_FG);
					else printf("  %08lX", tmp);
				}

				if ((li-ld) %8) Writeln();

				WaitKey(0);
				smp_adjust(0);
				continue;
			}

			continue;
		}

		while (1) {
			if (offl) { gotoxy(1, 1); printf("%u", ro_seq-1); }

			li=rout_len/4; ld=li;
	//		while ((ld > hdr) && (dmabuf[0][ld-1] &0x00200000L)) ld--;
	ld=hdr;
	while ((ld < li) && !(dmabuf[0][ld] &0x00200000L)) ld++;
	li=ld;
	while ((li < rout_len/4) && (dmabuf[0][li] &0x00200000L)) li++;

			if (single == 0) {
				u_int	id;

				if (ld != hdr+_XF*(sqd->window+1)) {
					gotoxy(1, 1);
					printf("l=%04X/%04X exp %04X ", ld, li, hdr+sqd->window+1);
					_setcursortype(_NORMALCURSOR); con=1;
					if ((rkey=getch()) == ESC) {
						gotoxy(1, QPRT_Y0+5);
						return CE_PROMPT;
					}

					if (rkey == TAB) single=3;
				}

				for (i=ld; i < li; i++) {
					id=(u_int)(dmabuf[0][i] >>16) &0x3F;
					if ((id == 0x30) || (id == 0x32) || (id == 0x34)) {
						if (id+1 != ((u_int)(dmabuf[0][i+1] >>16) &0x3F)) break;
						i++; continue;
					}

					if ((id == 0x36) || ((id &0x30) == 0x20)) continue;
					break;
				}

				if (i < li) {
					smp_adjust(1);
					single=2;
					continue;
				}
			}

			imx=0; imn=0xFFFF;
			if (sqd->bool &_MEA) {
				tmp=(offl) ? (u_int)dmabuf[0][ld+1] : inp_unit->mean_level[config.sq_cha];
				if (tmp <= sqd->a_min) mdis=0;
				else
					if (tmp > sqd->a_max) mdis=2*QPRT_Y;
					else mdis=(u_int)(
								 ((u_long)2*QPRT_Y*(tmp-sqd->a_min)+(sqd->a_max-sqd->a_min)/2)
								 /(sqd->a_max-sqd->a_min)
										 );
			}

			for (n=0, i=sqd->shift+hdr; (n < PRT_X) && (i < ld); n++) {
				u_long	ltmp=(tmp=(u_int)dmabuf[0][i++]) &0x0FFF;
				u_int		j=1;
				u_int		ldis;		// last position
				u_int		ovr=0;

				if (tmp &0x1000) ovr=1;
				while ((j <= sqd->comp) && (i < ld)) {
					j++; ltmp+=(tmp=(u_int)dmabuf[0][i++]) &0x0FFF;
					if (tmp &0x1000) ovr=1;
				}

				tmp=(u_int)(ltmp/j);
				if (tmp > imx) imx=tmp;
				if (tmp < imn) imn=tmp;
				if (ovr) {
					if (tmp > 0x800) dis=2*QPRT_Y -1; else dis=0;
				} else {
					if (tmp <= sqd->a_min) dis=0;
					else {
						if (tmp > sqd->a_max) dis=2*QPRT_Y -1;
						else dis=(u_int)(
									 ((u_long)2*QPRT_Y*(tmp-sqd->a_min))	// +(sqd->a_max-sqd->a_min)/2
									 /(sqd->a_max-sqd->a_min)
											 );
					}
				}
#if 0
				if (tmp <= sqd->a_min) dis=0;
				else
					if (tmp > sqd->a_max) dis=2*QPRT_Y;
					else dis=(u_int)(
								 ((u_long)2*QPRT_Y*(tmp-sqd->a_min)+(sqd->a_max-sqd->a_min)/2)
								 /(sqd->a_max-sqd->a_min)
										 );

	gotoxy(1, QPRT_Y0+5);
	printf(" %3u %2u %2u %2u ", prt_mdis, mdis, prt_dis[n], dis);
	_setcursortype(_NORMALCURSOR); con=1;
	if (WaitKey('0')) return 0;
#endif
				ldis=prt_dis[n];
				prt_dis[n]=dis;
				if (ldis != dis) {
					if (ldis != 255) {
						gotoxy(n+1, QPRT_Y0-(ldis/2));
						if (ldis/2) printf(" "); else printf("-");
					}

					gotoxy(n+1, QPRT_Y0-dis/2);
					if (ovr) printf("!");
					else printf("%c", (dis&1) ? 'ß' : 'Ü');
				}

				if (!(sqd->bool &_MEA)) continue;
#if 0
	gotoxy(1, QPRT_Y0+5);
	printf(" %3u %2u %2u %2u ", prt_mdis, mdis, x, dis);
	_setcursortype(_NORMALCURSOR); con=1;
	if (WaitKey('1')) return 0;
#endif
				if (   (mdis == prt_mdis)
					 && (mdis/2 != ldis/2)
					 && ((mdis/2 != dis/2) || (mdis == dis))) continue;

				if (prt_mdis != 255) {
					gotoxy(n+1, QPRT_Y0-(prt_mdis/2));
					if (prt_mdis/2 == dis/2) {
						if (!ovr) printf("%c", (dis&1) ? 'ß' : 'Ü');
					} else {
						if (prt_mdis/2) printf(" "); else printf("-");
					}
				}

				if (mdis != dis) {
					gotoxy(n+1, QPRT_Y0-(mdis/2));
					if ((mdis/2 != dis/2) || !ovr)
						printf("%c", (mdis/2==dis/2) ? 'Û'
															  : (mdis&1) ? 'ß' : 'Ü');
				}
			}

			prt_mdis=mdis;
			for (; (n < PRT_X); n++) {

				if (prt_dis[n] == 255) continue;

				if (sqd->bool &_MEA) {
					gotoxy(n+1, QPRT_Y0-(prt_mdis/2));
					if (prt_mdis/2) printf(" "); else printf("-");
				}

				gotoxy(n+1, QPRT_Y0-(prt_dis[n]/2));
				if (prt_dis[n]/2) printf(" "); else printf("-");
				prt_dis[n]=255;
			}

			gotoxy(1, QPRT_Y0+2);
			if (single == 1) {
	_setcursortype(_NORMALCURSOR); con=1;
	key=toupper(rkey=getch());
	if (rkey == ESC) {
		gotoxy(1, QPRT_Y0+5);
		return CE_PROMPT;
	}

	if (key == 'M') {
		gotoxy(1, QPRT_Y0+2);
		printf("%25s\r", "");
		logdata(0);
	}

	if (key != 'S') single=0;
			}	// (single == 1)

			if (single == 2) {
				u_long	tmp;
				u_int		id;
				u_int		x=80-24, y=1;

				for (i=ld; i < li; i++) {
					tmp=dmabuf[0][i]; id=(u_int)(tmp >>16) &0x3F;
					gotoxy(x, y++);
					if ((id == 0x30) || (id == 0x32) || (id == 0x34)) {
						if (id+1 != ((u_int)(dmabuf[0][i+1] >>16) &0x3F)) break;

						switch (id) {
				case 0x30:
							if (i == ld) printf("start");
							else         printf("end  ");
							break;

				case 0x32:
							printf("min  "); break;

				case 0x34:
							printf("max  "); break;
						}

						printf("%5.0fns %6.1fmV %03X", (int)tmp*25.0/16.0,
														  (int)dmabuf[0][i+1]*_MV,
														  ((u_int)dmabuf[0][ld]-(u_int)tmp) >>_FG);
						i++;
						continue;
					}

					if (id == 0x36) {
						id=(u_int)tmp>>14;
						if (tmp &0x2000) tmp |=0xC000; else tmp &=~0xC000;

						printf("zcros%5.0fns      %u   %03X", (int)tmp*25.0/16.0, id,
													((u_int)dmabuf[0][ld]-(u_int)tmp) >>_FG);
						continue;
					}

					if ((id &0x30) == 0x20) {
						printf("charg %6.2fnVs", (tmp &0xFFFFFl)*_MV*_NS/1000.0);
						continue;
					}

					break;
				}

				gotoxy(1, y);
				if (i < li) {
					printf("sequence error\n");
					for (i=ld; i < li; i++) printf("  %08lX", dmabuf[0][i]);

					if ((li-ld) %8) Writeln();
				}

				_setcursortype(_NORMALCURSOR); con=1;
				while (1) {
					key=toupper(rkey=getch());
					if (key != 'R') break;

					for (i=ld; i < li; i++) printf("  %08lX", dmabuf[0][i]);

					if ((li-ld) %8) Writeln();
				}

				if (rkey == 0) {
					rkey=getch();
					switch (rkey) {
				case RIGHT:
						sqd->shift +=(16*(sqd->comp+1));
						break;

				case LEFT:
						if (sqd->shift < (16*(sqd->comp+1))) sqd->shift=0;
						else sqd->shift -=(16*(sqd->comp+1));
						break;

				case CTLRIGHT:
						sqd->shift +=(64*(sqd->comp+1));
						break;

				case CTLLEFT:
						if (sqd->shift < (64*(sqd->comp+1))) sqd->shift=0;
						else sqd->shift -=(64*(sqd->comp+1));
						break;

				default:
						single=0;
						break;
					}

					if (!single) break;

					smp_adjust(1);
					continue;
				}

				if (key == 'P') {
					printf("%25s\r", "");
					printf("compress  %4u. ", sqd->comp);
					sqd->comp=(u_int)Read_Deci(sqd->comp, -1);
					smp_adjust(1);
					continue;
				}

				if (key == 'L') {
					printf("%25s\r", "");
					printf("lower lim  %04X ", sqd->a_min);
					sqd->a_min=(u_int)Read_Hexa(sqd->a_min, -1);
					smp_adjust(1);
					continue;
				}

				if (key == 'U') {
					printf("%25s\r", "");
					printf("upper lim  %04X ", sqd->a_max);
					sqd->a_max=(u_int)Read_Hexa(sqd->a_max, -1);
					smp_adjust(1);
					continue;
				}

				if (key == 'A') {
					if (sqd->bool &_MEA) {
						u_int	tmp=(offl) ? (u_int)dmabuf[0][ld+1] : inp_unit->mean_level[config.sq_cha];

						if (tmp > imx) imx=tmp;
						if (tmp < imn) imn=tmp;
					}

					if ((imx-imn) < 2*(QPRT_Y-2)) {
						sqd->a_min=(imx+imn)/2 -QPRT_Y;
						sqd->a_max=imx;
					} else {
						sqd->a_min=imn-(imx-imn)/20;
						if (sqd->a_min >= 0x1000) sqd->a_min=0;
						sqd->a_max=imx+(imx-imn)/20;
						if (sqd->a_max >= 0x1000) sqd->a_max=0x1000;
					}

					smp_adjust(1);
					continue;
				}

				if (rkey == 'm') {
					sqd->bool ^=_MEA;
					smp_adjust(1);
					continue;
				}

				if ((key == 'M') || (key == 'X')) {
					printf("%25s\r", "");
					logdata(key == 'X');
					smp_adjust(1);
					continue;
				}

				if (rkey == ESC) {
					gotoxy(1, QPRT_Y0+5);
					return CE_PROMPT;
				}

				if (key == 'S') {
					dq_len=li-ld;
				}

				if (key == ' ') rout_len=0; else single=0;

				if (key == TAB) { rout_len=0; single=3; }

				smp_adjust(0);
			}

			break;
		}	// while (1)
	}
}
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
	"Run", "Ltrg", "Tlev", "Edg", "Di0", "Di1", "Neg", "Raw",
	"Grd", "?",    "?",    "?",   "?",   "?",   "?",   "?"
};
//
static const char *inpsr_txt[] ={
	"RDerr", "NIM", "?",   "?", "?", "?", "?", "?",
	"?",     "?",   "?",   "?", "?", "?", "?", "?"
};
//
static u_int	dac_max=0x300;

extern int sram_test(
	u_long	*addr,
	u_short	*data,
	u_long	mlen);

extern int sram_dtest(
	u_long	*addr,
	u_short	*data,
	u_long	mlen);

int qdcstst_menu(void)
{
	int		ret=0;
	u_int		i;
	u_int		tmp;
	char		hgh;
	char		rkey,key;
	u_int		ux;
	char		hd=1;

	if (!sc_conf) return CE_PARAM_ERR;

	pbuf=(char*)abuf[0];
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
				if (inp_conf->type != SQDCTST) {
					printf("this is not a QDC SLOW module!!!\n");
					return CE_NO_INPUT;
				}

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

printf("max noise level                    %04X ..2\n", inp_unit->mean_tol);
printf("trigger level                      %04X ..3\n", inp_unit->trig_level);
printf("latency                            %04X ..4\n", inp_unit->latency);
printf("window                             %04X ..5\n", inp_unit->window);
printf("set DAC                       %04X/%04X ..6/7\n", inp_conf->slv_data[6], inp_conf->slv_data[7]);
printf("           means =%03X/%03X, noise =%02X/%02X\n", inp_unit->mean_level[0], inp_unit->mean_level[1], inp_unit->mean_noise[0], inp_unit->mean_noise[1]);
printf("                              Mrst(1) %X ..8\n", inp_conf->slv_xdata.s[7]);
printf("read Event, Picture, Statistics, LogDat ..E/P/S/L\n");
printf("JTAG                                    ..J\n");
				if (BUS_ERROR) CLR_PROT_ERROR();
printf("DAC test                               .. x\n");
#if 0
printf("SEQ SRAM test (single/DMA) .............. t/T\n");
printf("Table SRAM test (single/DMA) ............ x/X\n");
#endif
			}

			printf("                                          %c ", config.i3_menu);
			while (1) {
				rkey=getch();
				if ((rkey == CTL_C) || (rkey == ESC)) return 0;

				break;
			}
		} else {
			hd=1; printf("select> ");
			while (1) {
				rkey=getch();
				if (rkey == CTL_C) return 0;

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

		if (rkey == 'U') {
			sc_assign(2);
			continue;
		}

		if (!inp_conf) continue;

		if (rkey == 'u') {
			if ((ret=inp_assign(sc_conf, 2)) != 0) continue;
			if (inp_conf->type != SQDCTST) {
				printf("this is not a QDC module!!!\n");
				return CE_NO_INPUT;
			}
			continue;
		}

		if (inp_conf->type != SQDCTST) continue;
//
//---------------------------
//
		if (key == 0) {
			key=getch();
			continue;
		}

		if (key == 'X') {
			u_int	tmp=0;

			printf("max Value  %04X ", dac_max);
			dac_max=(u_int)Read_Hexa(dac_max, 0x1000);
			if (errno) continue;
			if (dac_max > 0x1000) dac_max=0x1000;

			config.i3_menu=rkey;
			inp_unit->dac[0]=0x3000;
			inp_unit->dac[1]=0x3000;
			utmp=inp_unit->sr;	// delay
			inp_unit->dac[0]=0x6000;
			inp_unit->dac[1]=0x6000;
			utmp=inp_unit->sr;	// delay
			while (1) {
				inp_unit->dac[0]=0x3000 |tmp;
//				inp_unit->dac[1]=0x2000 |tmp;
				utmp=inp_unit->sr;	// delay
				inp_unit->dac[0]=0x6000 |tmp;
//				inp_unit->dac[1]=0x6000 |tmp;
				utmp=inp_unit->sr;	// delay
				_delay_(10000L);
//				delay(1);
				tmp++;
				if (tmp == dac_max) {
					tmp=0;
					if (kbhit()) {
						inp_unit->dac[0]=0x3000;
						inp_unit->dac[1]=0x3000;
						utmp=inp_unit->sr;	// delay
						inp_unit->dac[0]=0x6000;
						inp_unit->dac[1]=0x6000;
						break;
					}
				}
			}

			continue;
		}

		if (rkey == 't') {
			ret=sram_test(&inp_unit->qram_addr, &inp_unit->qram_data, 4*SQ_SRAM_LEN);
			continue;
		}

		if (rkey == 'T') {
			ret=sram_dtest(&inp_unit->qram_addr, &inp_unit->qram_data, 4*SQ_SRAM_LEN);
			continue;
		}

		if (rkey == 'x') {
			ret=sram_test(&inp_unit->tram_addr, &inp_unit->tram_data, 2*TB_SRAM_LEN);
			continue;
		}

		if (rkey == 'X') {
			ret=sram_dtest(&inp_unit->tram_addr, &inp_unit->tram_data, 2*TB_SRAM_LEN);
			continue;
		}

		if (key == 'J') {
			JTAG_menu(jt_inp_putcsr, jt_inp_getcsr, jt_inp_getdata, &config.jtag_qdcs, 0);
			continue;
		}

		if (rkey == 'e') {
			u_int		dat;
			u_int		min=0xFFF,max=0;
			u_long	mean[8]={0};
			u_int		cmean[8]={0};

			config.i3_menu=rkey;
			{
				u_int		i;

				for (i=0; i < 16; i++) inp_offl[i] =(i != sc_conf->slv_sel);
			}

			inp_mode(IM_VERTEX);
			if ((ret=read_event(EVF_NODAC, RO_RAW)) != 0) continue;

			printf("%04X Data read\n", rout_len);
			for (i=0; i < (rout_len/4 -1);) {
				dat=(u_int)dmabuf[0][i];
				if (max < dat) max=dat;
				if (min > dat) min=dat;

				mean[i&7] +=dat; cmean[i&7]++;
				if (!(i %8)) printf("%04X:", i*4);
				printf(" %08lX", dmabuf[0][i]);
				if (!(++i %8)) printf("\n");
				if (!(i %320)) if (WaitKey(0)) break;
			}

			if (i %8) printf("\n");
			printf("cnt,min,max,dif = %04X/%04X/%04X/%u\n", i, min, max, max-min);
			printf("     ");
			for (i=0; i < 8; i++) printf(" %08X", mean[i]/cmean[i]);
			Writeln();
			hd=0;
			continue;
		}

		if (rkey == 'E') {
			u_long	cnt=0;
			u_int		dat;
			u_int		min=0xFFF,max=0;
			u_int		n=0;

			{
				u_int		i;

				for (i=0; i < 16; i++) inp_offl[i] =(i != sc_conf->slv_sel);
			}

			inp_mode(IM_VERTEX);
			while (1) {
				if (n || (ret=read_event(EVF_NODAC, RO_RAW)) != 0) {
					printf("cnt,min,max,dif = %04lX/%04X/%04X/%u\n", cnt, min, max, max-min);
					break;
				}

				n=getch()-'0';
				if ((n >= 2) && (n <= 9)) {
					u_int		j=0;
					u_long	sum=0;

					for (i=0; i < rout_len/4; i++) {
						sum+=(u_int)dmabuf[0][i]; j++;
						if (j >= (1<<n)) {
							sum+=(1<<(n-1));
							sum >>=n;
							printf(" %04lX", sum);
							j=0; sum=0;
						}
					}

					Writeln(); n=0;
				}

				cnt++;
				for (i=0; i < (rout_len/4 -1); i++) {
					dat=(u_int)dmabuf[0][i];
					if (max < dat) max=dat;
					if (min > dat) min=dat;
				}
			}

			hd=0;
			continue;
		}

		if (key == 'P') {
			config.i3_menu=rkey;
#ifdef PBUS
			ret=qdc_sample();
#else
			offl=rkey == 'P';
			if (offl) {
				if ((ret=open_datafile(1)) != 0) continue;
				if (datafile < 0) continue;
			}

			ret=qdc_sample();
			close_datafile();
#endif
			continue;
		}

#ifndef PBUS
		if (key == 'L') {
			config.i3_menu=rkey;
			ret=qdc_logdata();
			continue;
		}
#endif

		if (key == 'S') {
			config.i3_menu=rkey;
			ret=qdc_statistics();
			continue;
		}

		ux=key-'0';
		if (ux > 9) ux=ux-7;

		if (ux > 8) continue;

		config.i3_menu=rkey;
//		if (ux == 5) {
//			printf("Value  %5u ", inp_conf->slv_data[ux]);
//			inp_conf->slv_data[ux]=(u_int)Read_Deci(inp_conf->slv_data[ux], -1);
		if (ux < 8) {
			printf("Value  %04X ", inp_conf->slv_data[ux]);
			inp_conf->slv_data[ux]=(u_int)Read_Hexa(inp_conf->slv_data[ux], -1);
		}


		if (ux == 8) {
			printf("Value  %04X ", inp_conf->slv_data[ux]);
			inp_conf->slv_xdata.s[7]=(u_int)Read_Hexa(inp_conf->slv_xdata.s[7], -1);
		}

		if (errno) continue;
		switch (ux) {

case 0:
			inp_unit->cr=inp_conf->slv_data[ux]; break;

case 2:
			inp_unit->mean_tol=inp_conf->slv_data[ux]; break;

case 3:
			inp_unit->trig_level=inp_conf->slv_data[ux]; break;

case 4:
			inp_unit->latency=inp_conf->slv_data[ux]; break;

case 5:
			inp_unit->window=inp_conf->slv_data[ux]; break;

case 6:
			inp_unit->dac[0]=inp_conf->slv_data[ux]; break;

case 7:
			inp_unit->dac[1]=inp_conf->slv_data[ux]; break;

case 8:
			if (inp_conf->slv_xdata.s[7] == MRESET) {
				inp_bc->ctrl=MRESET;
				break;
			}

			inp_unit->ctrl=inp_conf->slv_xdata.s[7]; break;

		}
	}
}
// --- end ------------------ sync_slave_menu -------------------------------
//
