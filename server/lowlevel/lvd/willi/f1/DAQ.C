// Borland C++ - (C) Copyright 1991, 1992 by Borland International

/*	daq.c -- general functions  */

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
#include "f1.h"
//
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
static void open_datafile(void)
{
	char	*str;

	str=Read_String(config.dataname, sizeof(config.dataname));
	if (errno) return;

	if ((datafile >= 0) && !strcmp(str, config.dataname)) return;

	close_datafile();
	if (!*str) return;

	if ((datafile=_rtl_creat(str, 0)) == -1) {
		perror("error creating data file");
		printf("name =%s", str);
		return;
	}

	strcpy(config.dataname, str);
	return;
}
//
//===========================================================================
//
//										general DAQ
//
//===========================================================================
//
typedef struct {
	u_long	hlen;
	u_short	sec;
	u_short	min;
	u_short	hour;
	u_short	day;
	u_short	mon;
	u_short	year;
	u_short	edge;
	u_short	trig;
	u_short	hsdiv;
	short		lat;
	u_short	win;
	short		tdac;
	u_char	dac[100];		// u32 align
} DAT_INFO;

static DAT_INFO	dat_info;
//
//--------------------------- setup -----------------------------------------
//
static void setup(void)
{

	printf("falling/rising/both(LeTra) edge, 0/1/2: %u ", config.daq_edge);
	config.daq_edge=(u_char)Read_Deci(config.daq_edge, 2);
	if (errno) return;

	printf("triggered/raw hits                 0/1: %u ", config.trig_mode);
	config.trig_mode=(u_char)Read_Deci(config.trig_mode, 1);
	if (errno) return;

	if (!config.trig_mode) {
		printf("trigger latency (ns)               %6.1f ", config.ef_trg_lat);
		config.ef_trg_lat=Read_Float(config.ef_trg_lat);
		if (errno) return;

		printf("trigger window  (ns)               %6.1f ", config.ef_trg_win);
		config.ef_trg_win=Read_Float(config.ef_trg_win);
		if (errno) return;
	}

	use_default=1;
	switch (config.daq_edge) {
case 0:
		config.f1_slope=0x12; break;

case 1:
		config.f1_slope=0x11; break;

default:
		config.f1_slope=0x17; break;
	}

	f1_hit_slope(SC_ALL);
}
//
//--------------------------- setdac ----------------------------------------
//
static void setdac(void)
{
	int	sc;
	u_int	inp,con;
	float	thrh;
	int	dac;

static int	last_thr_dac;

	last_key=0;
	for (sc=-1; sc < config.nr_fbsc; sc++) {
		for (inp=0; inp < config.sc_db[sc+1].nr_f1units; inp++)
			for (con=0; con < 4; con++) {
				if (last_key == CTL_A) dac=last_thr_dac;
				else {
					dac=config.sc_db[sc+1].inp_db[inp].inp_dac[con];
					thrh=THRH_MIN+THRH_SLOPE*dac;
					printf("unit %2d/%-2u con %u: threshold (%5.*f bis %4.*f) %*.*f ",
									 sc, inp, con,
									 THRHW, THRH_MIN, THRHW, THRH_MAX, THRHW+3, THRHW, thrh);
					thrh=Read_Float(thrh);
					if (errno) return;

					dac=(u_int)((thrh-THRH_MIN)/THRH_SLOPE +0.5);
				}

				if (dac < 0) dac=0;
				if (dac > 0xFF) dac=0xFF;
				last_thr_dac=dac;
				config.sc_db[sc+1].inp_db[inp].inp_dac[con]=dac;
				if (set_dac(sc, inp, con, dac)) return;
			}
	}
}
//
//--------------------------- logging ---------------------------------------
//
static void logging(void)
{
	struct time	timerec;
	struct date	daterec;

	int		sc;
	u_int		inp,con;
	u_int		xcon;

	gettime(&timerec);
	getdate(&daterec);
	if (datafile >= 0) {
		dat_info.sec=timerec.ti_sec;
		dat_info.min=timerec.ti_min;
		dat_info.hour=timerec.ti_hour;
		dat_info.day=daterec.da_day;
		dat_info.mon=daterec.da_mon;
		dat_info.year=daterec.da_year;
		dat_info.edge=config.daq_edge;
		dat_info.trig=config.trig_mode;
		dat_info.hsdiv=config.hsdiv;
		dat_info.lat=0;
		dat_info.win=0;
		if (config.trig_mode == 0) {
			dat_info.lat=(short)tlat;
			dat_info.win=(u_short)twin;
		}

		dat_info.tdac=-1;
		if (config.daq_mode &EVF_ON_TP) dat_info.tdac=config.test_dac;

		xcon=0;
		for (sc=-1; sc < config.nr_fbsc; sc++) {
			for (inp=0; inp < config.sc_db[sc+1].nr_f1units; inp++)
				for (con=0; con < 4; con++)
					dat_info.dac[xcon++]=config.sc_db[sc+1].inp_db[inp].inp_dac[con];
		}

		dat_info.hlen=0x80010000L+OFFSET(DAT_INFO, dac)+xcon;
		if (write(datafile, &dat_info, (u_int)dat_info.hlen) != (u_int)dat_info.hlen) {
			perror("error writing config file");
			printf("name =%s", config.dataname);
			close_datafile();
			return;
		}
	}
}
//
//--------------------------- loginfo ---------------------------------------
//
static void loginfo(void)
{
	char	*str;
	char	*dst=(char*)&dat_info +4;
	u_int	i;

	if (datafile < 0) return;

	printf("enter text\n");
	str=Read_String(0, 80);
	if (errno) return;

	i=0;
	while ((dst[i++]=*str++) != 0);
	while (i %4) dst[i++]=0;
	dat_info.hlen=0x80020000L+4+i;
	if (write(datafile, &dat_info, (u_int)dat_info.hlen) != (u_int)dat_info.hlen) {
		perror("error writing config file");
		printf("name =%s", config.dataname);
		close_datafile();
	}
}
//
//--------------------------- daq ---------------------------------------
//
static void daq(u_int test)
{
	u_long	lp;
	u_long	tc;
	u_long	dlen;
	int		dac;
	float		thrh;

	if (datafile < 0) {
		printf("open data file\n"); return;
	}

	if (test) {
		dac=config.test_dac;
		thrh=THRH_MIN+THRH_SLOPE*dac;
		printf("test pulse threshold (%5.*f bis %4.*f) %*.*f ",
						 THRHW, THRH_MIN, THRHW, THRH_MAX, THRHW+3, THRHW, thrh);
		thrh=Read_Float(thrh);
		if (errno) return;

		dac=(u_int)((thrh-THRH_MIN)/THRH_SLOPE +0.5);
		if (dac < 0) dac=0;
		if (dac > 0xFF) dac=0xFF;
		config.test_dac=dac;
		if (set_dac(SC_ALL, -1, -1, dac)) return;

		config.daq_mode=EVF_ON_TP|MCR_DAQRAW|MCR_DAQTRG;
		config.rout_delay=255;		//*25ns => 6.4 us
	} else {
		if (config.trig_mode) {
			config.daq_mode=MCR_DAQRAW;
			config.rout_delay=40;		//*25ns => 1 us
		} else {
			config.daq_mode=MCR_DAQTRG;
		}
	}

	printf("number of trigger (0=forever)       %10lu ", config.tcount);
	config.tcount=Read_Deci(config.tcount, -1);
	if (errno) return;

	use_default=1;
	if (inp_mode()) return;

	logging();
	use_default=0; dlen=0; lp=kbatt;
	kbatt_msk=0xFF; tc=0;
	while (1) {
		if (test) test_pulse();

		if (read_event()) {
			printf("\r%08lX\n", dlen);
			eventcount=event_counter(-1); 
			return;
		}

		if (write(datafile, robf, rout_len) != rout_len) {
			perror("error writing config file");
			printf("name =%s", config.dataname);
			close_datafile();
			return;
		}

		dlen+=rout_len;
		if (config.tcount && (++tc >= config.tcount)) {
			printf("\r%08lX\n", dlen);
			eventcount=event_counter(-1); 
			return;
		}

		if ((kbatt-lp) >= 0x1000) {
			lp=kbatt;
			printf("\r%08lX", dlen);
			if (inp_overrun() < 0) return;
		}
	}
}
//
//--------------------------- data_acquisition ------------------------------
//
void data_acquisition(void)
{
	char	key;

	while (1) {
		Writeln();
		printf("   data file ................0\n");
		printf("   setup ....................2/s\n");
		printf("   threshold ................3/d\n");
	if (datafile >= 0) {
		printf("   log info .................4/i\n");
		printf("   data acquisition .........5/a\n");
		printf("   daq with test pulse ......6/t\n");
	}
		printf("   Ende ...................ESC\n");
		printf("                             %c ",
		config.tmp_menu);

		key=toupper(getch());
		if ((key == CTL_C) || (key == ESC)) {
			close_datafile();
			return;
		}

		use_default=(key == TAB);
		if ((key == TAB) || (key == CR)) key=config.tmp_menu;

		printf("\n");
		errno=0; last_key=0;
		switch (key) {

	case '0':
			printf("datafile: ");
			open_datafile();
			Writeln();
			break;

	case '2':
	case 'S':
			setup();
			break;

	case '3':
	case 'D':
			setdac();
			break;

	case '4':
	case 'I':
			loginfo();
			break;

	case '5':
	case 'A':
	case 'Q':
			daq(0);
			config.tmp_menu=key;
			break;

	case '6':
	case 'T':
			daq(1);
			config.tmp_menu=key;
			break;

		}

		while (kbhit()) getch();
	}
}
