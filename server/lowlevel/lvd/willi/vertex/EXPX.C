// Borland C++ - (C) Copyright 1991, 1992 by Borland International

/*	expx.c -- experiment functions  */

//
#ifdef LOGFILE
static FILE	*logfile=0;
//
//--------------------------- close_logfile ---------------------------------
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
//--------------------------- open_logfile ----------------------------------
//
static int open_logfile(void)
{
	char	*str;

	str=Read_String(config.logname, sizeof(config.logname));
	if (errno) return -1;

	if (logfile && !strcmp(str, config.logname)) return 0;

	if (close_logfile()) return CEN_MENU;

	if (!*str) return 0;

	logfile=fopen(str, "w");
	if (!logfile) {
		perror("can't open logfile");
		printf("name =%s", str);
		return CEN_MENU;
	}

	strcpy(config.logname, str);
	return 0;
}
//
#endif
#if defined(MWPC) || defined(TOF)
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
static int open_datafile(void)
{
	char	*str;

	str=Read_String(mwconf.dataname, sizeof(mwconf.dataname));
	if (errno) return CE_MENU;

	if ((datafile >= 0) && !strcmp(str, mwconf.dataname)) return 0;

	close_datafile();
	if (!*str) return 0;

	if ((datafile=_rtl_creat(str, 0)) == -1) {
		perror("error creating data file");
		printf("name =%s", str);
		return CE_PROMPT;
	}

	strcpy(mwconf.dataname, str);
	return 0;
}
//
#endif
//===========================================================================
//
//------------------------------ raw_input ----------------------------------
//
int raw_input(
	int	sel)	// selected channels
{
	int		ret;
	u_int		i;
	u_int		lx;
	u_long	cmask[2*C_UNIT];
	u_int		un;
	u_int		cha;
	u_int		col;

	if (sel) {
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
					if (config.sel_cha[i] == -2) {
					for (i=0; i < 2*C_UNIT; cmask[i++]=0xFFFFFFFFL);
					break;
				}

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
	}

	if ((ret=inp_mode(0)) != 0) return ret;

	use_default=0;
	while (1) {
		if ((ret=read_event(0, RO_RAW)) != 0) {
			if (ret <= CEN_PPRINT) continue;

			return ret;
		}

		if (!sel) for (i=0; i < 2*C_UNIT; cmask[i++]=0);

		i=0; lx =rout_len >>2; col=0;
		while (i < lx) {
			cha=EV_CHAN(robf[i]);
			if (rout_hdr && (i < HDR_LEN)) {
				if (col) printf(" ");
				printf(" %08lX", robf[i]); col++;
			} else {
				un=(u_int)(robf[i] >>28);
				if (sel) {
					if (!inp_type[un] && (cmask[cha >>5] &(1L <<(cha &0x1F)))) {
						if (col) printf(" ");
						printf(" %02X:%X%04X", cha,
								 EV_MTIME(robf[i]) &0xF, EV_FTIME(robf[i]));
						col++;
					}
				} else {
					if (col) printf(" ");
					if (inp_type[un]) printf(" %08lX", robf[i]);
					else
						if (cmask[cha >>5] &(1L <<(cha &0x1F))) {
							printf(".%02X:%X%04X", cha,
									 EV_MTIME(robf[i]) &0xF, EV_FTIME(robf[i]));
						} else {
							cmask[cha >>5] |=(1L <<(cha &0x1F));
							printf(" %02X:%X%04X", cha,
									 EV_MTIME(robf[i]) &0xF, EV_FTIME(robf[i]));
						}

					col++;
				}
			}

			i++;
			if (col >= 8) { col=0; Writeln(); }
		}

#if 0
		if (rout_hdr &MCR_DAQTRG) {
			if (col) printf(" ");
			printf(" MT:%05lX", rout_trg_time &0xFFFFFL);
			Writeln();
		} else
#endif
		{
			if (col) Writeln();
		}

	}
}
//
//--------------------------- triggered_input -------------------------------
//
int triggered_input(
	int	sel)	// selected channels
{
	int		ret;
	u_long	cmask[2*C_UNIT];	// anzuzeigende Kanaele
	u_int		i,j,sp;
	int		cha;
	long		diff;
	int		l_cha;				// letzter angezeigter Kanal
	int		k,v_cha;				// Werte je Kanal
	u_int		ln=0;
	char		err=0;
	float		resol=RESOLUTION/1000.;

	for (i=0; i < 2*C_UNIT; cmask[i++]=0);

	if (sel) {
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

		ln=100;
	}

	if ((ret=inp_mode(IM_TREF)) != 0) return ret;

	v_cha=1;
	while (1) {
		if ((ret=read_event(0, config.inp_trigger)) != 0) {
			if (ret <= CEN_PPRINT) continue;

			return ret;
		}

//printf("%04X %04X\n", tm_cnt, 64*C_UNIT);
//for (i=0; i < tm_cnt; i++) printf("  %02X", (*tm_hit)[i].tm_cha);
//Writeln();
		if (ln >= 40) {
			ln=0; Writeln();
			for (cha=0, j=0, sp=0; (cha < 64*C_UNIT); cha++) {
				if (!(cmask[cha >>5] &(1L <<(cha &0x1F)))) continue;

				if (sp) printf(" ");

				sp++; j++;
				printf("C%03X", cha);
				for (k=1; k < v_cha; k++, sp++, j++, printf("     "));

				if (sp+v_cha > 16) { Writeln(); sp=0; }
			}

			if (sp) Writeln();
		}

		l_cha=-1; j=0; sp=0; i=0;
		while (i < tm_cnt) {
			cha=(*tm_hit)[i].tm_cha;
			if (inp_type[cha>>6]) { i++; continue; }

			if ((cha=(*tm_hit)[i].tm_cha) >= 64*C_UNIT) break;

			if (!(cmask[cha >>5] &(1L <<(cha &0x1F)))) {		// neuer Kanal
				if (sel) { i++; continue; }

				cmask[cha >>5] |=(1L <<(cha &0x1F));
				ln=100;
			}
//
//--- Platzhalter fuer nicht vorhandene Kanaele
//
			while (++l_cha < cha) {
				if (!(cmask[l_cha >>5] &(1L <<(l_cha &0x1F)))) continue;

				if (sp) printf(" ");

				sp++; j++; printf("*%03X", l_cha); err=1;
				for (k=1; k < v_cha; k++, sp++, j++, printf("     "));

				if (sp+v_cha > 16) { Writeln(); ln++; sp=0; }
			}

			k=0;
			do {
				if (cha != (*tm_hit)[i].tm_cha) {
					printf("     ");
					sp++; j++; k++; err=1;
					continue;
				}

				diff=(*tm_hit)[i].tm_diff;
				if (sp) printf(" ");

				printf("%4.0f", diff*resol);
				sp++; j++; i++; k++;
				if (   (config.f1_letra &0x04)				// letra mode
					 && (i < tm_cnt) && (cha == (*tm_hit)[i].tm_cha)	// next cha identisch
					 && ((diff &1) != ((*tm_hit)[i].tm_diff &1))) {	// letra paar
					printf("/%-4.0f", ((diff&~1)-((*tm_hit)[i].tm_diff&~1)) *resol);
					sp++; j++; i++; k++;
				}
			} while ((i < tm_cnt) && ((k < v_cha) || (cha == (*tm_hit)[i].tm_cha)));

			if (k > v_cha) v_cha=k;

			if (sp+v_cha > 16) { Writeln(); ln++; sp=0; }
		}

		if (sp) { Writeln(); ln++; }

//		printf(" MT:%05lX", rout_trg_time &0xFFFFFL);
		if (err) {
#if 0
			lx =rout_len >>1;
			i=0;
			while (i < lx) {
				if (i %16) printf(" ");
				printf(" %02X:%X%04X", EV_CHAN(bf[i]),
											  bf[i] &0xF, bf[i+1]);
				i +=2;
				if (!(i %16)) Writeln();
			}

#if 0
			if (rout_hdr &MCR_DAQTRG) {
				if (i %16) printf(" ");
				printf(" MT:%05lX", rout_trg_time &0xFFFFFL);
				Writeln();
			} else
#endif
			{
				if (i %16) Writeln();
			}

			return CE_PROMPT;
#endif
		}
	}
}
//
//------------------------------ pulse_width --------------------------------
//
static u_int	ri_markl[64];
static u_int	fa_markl[64];
static u_char	ri_markh[64];
static u_char	fa_markh[64];

int pulse_width(void)
{
	int		ret;
	u_int		i,s;
	u_int		lx;
	u_long	ri_msk[2*C_UNIT];
	u_long	fa_msk[2*C_UNIT];
	u_int		cha;
	u_int		pw;
	char		err=0;
	char		mx;
	float		resol=RESOLUTION/1000.;

	if ((config.f1_letra &0x07) != 0x07) {
		printf("setup with letra mode\n");
		return  CEN_PROMPT;
	}

	if ((ret=inp_mode(0)) != 0) return ret;

	while ((ret=read_event(EVF_NODAC, RO_RAW)) == 0) {
		lx =rout_len >>2;
		s=(rout_hdr) ? HDR_LEN : 0;
		if (lx <= s) continue;
		lx -=s;
		for (i=0; i < 2*C_UNIT; i++) { ri_msk[i]=0; fa_msk[i]=0; }

		for (i=0; i < lx; i++) {
			cha=EV_CHAN(robf[i+s]);
			if (EV_FTIME(robf[i+s]) &1) {
				ri_msk[cha>>5] |=(1L <<(cha &0x1F));
				ri_markl[cha]=(EV_FTIME(robf[i+s]) &~1);
				ri_markh[cha]=(u_char)EV_MTIME(robf[i+s]);
				continue;
			}

			fa_msk[cha>>5] |=(1L <<(cha &0x1F));
			fa_markl[cha]=EV_FTIME(robf[i+s]);
			fa_markh[cha]=(u_char)EV_MTIME(robf[i+s]);
		}


		for (mx=0, i=0; i < 2*C_UNIT; i++)
			if (ri_msk[i] &=fa_msk[i]) mx=1;

		if (!mx) {
			printf("no double time marks\n");
			continue;
		}

		for (i=0, cha=0; cha < 64*C_UNIT; cha++) {
			if (!(ri_msk[cha>>5] &(1L <<(cha &0x1F)))) continue;

			pw=fa_markl[cha]-ri_markl[cha];
			if (fa_markh[cha] != ri_markh[cha]) {
				if (fa_markh[cha] != (u_char)(ri_markh[cha]+1)) err=1;
				else pw +=(u_int)f1_range;	// pw=(fa_markl+f1_range)-ri_markl;
			}

			if (pw > 0x2000) err=2;

			if (i %8) printf("  ");
			printf("%02X:%5.1f", cha, pw*resol);
			i++;
			if (!(i %8)) Writeln();
		}

if (err) printf("err %d\n", err);
err=0;
//		if (i %8) Writeln();
		printf(" MT:%05lX", rout_trg_time &0xFFFFFL);
		Writeln();
		if (err) {
			i=0;
			while (i < lx+s) {
				if (i %8) printf(" ");
				if (i < s) printf(" %08lX", robf[i]);
				else
					printf(" %02X:%X%04X", EV_CHAN(robf[i]),
							 EV_MTIME(robf[i]) &0xF, EV_FTIME(robf[i]));

				i++;
				if (!(i %8)) Writeln();
			}

			if (rout_hdr &MCR_DAQTRG) {
				if (i %8) printf(" ");
				printf(" MT:%05lX", rout_trg_time &0xFFFFFL);
				Writeln();
			} else {
				if (i %8) Writeln();
			}

			return CE_PROMPT;
		}
	}

	return ret;
}
//
#ifdef MWPC
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

//--------------------------- setup -----------------------------------------
//
static int setup(void)
{

	printf("falling/rising/both(LeTra) edge, 0/1/2: %u ", config.daq_edge);
	config.daq_edge=(u_char)Read_Deci(config.daq_edge, 2);
	if (errno) return CE_MENU;

	printf("triggered/raw hits                 0/1: %u ", config.trig_mode);
	config.trig_mode=(u_char)Read_Deci(config.trig_mode, 1);
	if (errno) return CE_MENU;

#if 0
	printf("event counter                  0x%08lX ", eventcount);
	eventcount=Read_Hexa(eventcount, -1);
	if (errno) return CE_MENU;
#endif

	if (config.trig_mode) {
		config.daq_mode=MCR_DAQRAW;
		config.rout_delay=40;		//*25ns => 1 us
	} else {
		config.daq_mode=MCR_DAQTRG;
		printf("trigger latency (ns)               %6.1f ", config.ef_trg_lat);
		config.ef_trg_lat=Read_Float(config.ef_trg_lat);
		if (errno) return CE_MENU;

		printf("trigger window  (ns)               %6.1f ", config.ef_trg_win);
		config.ef_trg_win=Read_Float(config.ef_trg_win);
		if (errno) return CE_MENU;
	}

	use_default=1;
	config.hsdiv=(u_int)(HSDIV/120.0);
	config.f1_sync=2;
	switch (config.daq_edge) {
case 0:
		config.f1_letra=0x12; break;

case 1:
		config.f1_letra=0x11; break;

default:
		config.f1_letra=0x17; break;
	}

	return general_setup(-2, -1);
}
//
#ifdef LOGFILE
//--------------------------- logging ---------------------------------------
//
static void log_dac(void)
{
	int		sc;
	SC_DB		*sconf;
	u_int		ip;
	INP_DB	*iconf;
	u_int		con;
	u_int		cha;
	u_int		xcon;

	for (sc=LOW_SC_UNIT, sconf=config.sc_db+LOW_SC_UNIT+1;
		  sc < config.sc_units; sc++, sconf++) {
		fprintf(logfile, "DACs: ", sconf->unit); cha=0; xcon=0;
		for (ip=0, iconf=sconf->inp_db; ip < sconf->slv_cnt; ip++, iconf++) {
			if (xcon) fprintf(logfile, "\n      ");

			for (con=0; con < 4; con++) {
				fprintf(logfile, " %03X/%1.*f;", cha,
					 THRHW, THRH_MIN+THRH_SLOPE*iconf->inp_dac[con]);
				cha +=16;
			}
		}

		fprintf(logfile, "\n");
	}
}
#endif

static int logging(void)
{
	struct time	timerec;
	struct date	daterec;

	SC_DB		*sconf=config.sc_db+LOW_SC_UNIT+1;
	u_int		ip;
	INP_DB	*iconf;
	u_int		con,xcon;

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
		for (ip=0, iconf=sconf->inp_db; ip < sconf->slv_cnt; ip++, iconf++) {
			for (con=0; con < 4; con++) {
				dat_info.dac[xcon++]=iconf->inp_dac[con];
			}
		}

		dat_info.hlen=0x80010000L+OFFSET(DAT_INFO, dac)+xcon;
		if (write(datafile, &dat_info, (u_int)dat_info.hlen) != (u_int)dat_info.hlen) {
			perror("error writing config file");
			printf("name =%s", mwconf.dataname);
			close_datafile();
			return CEN_PROMPT;
		}
	}

#ifdef LOGFILE
	if (!logfile) return 0;

	fprintf(logfile, "%2d:%02d:%02d", timerec.ti_hour, timerec.ti_min, timerec.ti_sec);
	switch (config.daq_edge) {
case 0:
		fprintf(logfile, ", falling edge"); break;

case 1:
		fprintf(logfile, ", rising edge"); break;

default:
		fprintf(logfile, ", LeTra mode"); break;
	}

	if (config.trig_mode) fprintf(logfile, ", raw mode");
	else
		fprintf(logfile, ", latency/window %1.1f/%1.1fns",
				  config.ef_trg_lat, config.ef_trg_win);
	fprintf(logfile, ", resolution %1.2fps\n", RESOLUTION);
	fprintf(logfile, "event counter 0x%08lX\n", eventcount);
	log_dac();
#endif
	return 0;
}
//
//--------------------------- loginfo ---------------------------------------
//
static int loginfo(void)
{
	char	*str;
	char	*dst=(char*)&dat_info +4;
	u_int	i;

	if (datafile < 0) return CEN_MENU;

	printf("enter text\n");
	str=Read_String(0, 80);
	if (errno) return CEN_PROMPT;

#ifdef LOGFILE
	if (logfile) fprintf(logfile, "%s\n", str);
#endif
	i=0;
	while ((dst[i++]=*str++) != 0);
	while (i %4) dst[i++]=0;
	dat_info.hlen=0x80020000L+4+i;
	if (write(datafile, &dat_info, (u_int)dat_info.hlen) != (u_int)dat_info.hlen) {
		perror("error writing config file");
		printf("name =%s", mwconf.dataname);
		close_datafile();
	}

	return CEN_PROMPT;
}
//
//--------------------------- daq ---------------------------------------
//
static int daq(u_int test)
{
	int		ret;
	u_long	lp;
	u_long	tc;
	u_long	dlen;
	int		dac;
	float		thrh;
	int		sc;
	SC_DB		*sconf;

	if (datafile < 0) {
		printf("open data file\n"); return CEN_PROMPT;
	}

	if (test) {
		dac=config.test_dac;
		thrh=THRH_MIN+THRH_SLOPE*dac;
		printf("test pulse threshold (%5.*f bis %4.*f) %*.*f ",
						 THRHW, THRH_MIN, THRHW, THRH_MAX, THRHW+3, THRHW, thrh);
		thrh=Read_Float(thrh);
		if (errno) return CE_MENU;

		dac=(u_int)((thrh-THRH_MIN)/THRH_SLOPE +0.5);
		if (dac < 0) dac=0;
		if (dac > 0xFF) dac=0xFF;
		config.test_dac=dac;
		if ((ret=inp_setdac(0, 0, -2, dac)) != 0) return ret;

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
	if (errno) return CE_MENU;

	use_default=1;
	if ((ret=inp_mode(0)) != 0) return ret;

	logging();
	use_default=0; dlen=0; lp=kbatt;
	kbatt_msk=0xFF; tc=0;
	while (1) {
		for (sc=LOW_SC_UNIT, sconf=config.sc_db+LOW_SC_UNIT+1;
			  sc < config.sc_units; sc++, sconf++) {
			sconf->sc_unit->ctrl=TESTPULSE;
		}

		if ((ret=read_event(EVF_NODAC, RO_XRAW)) != 0) {
			printf("\r%08lX\n", dlen);
			sconf=config.sc_db+LOW_SC_UNIT+1;
			eventcount=sconf->sc_unit->event_nr;
			return ret;
		}

		if (write(datafile, robf, rout_len) != rout_len) {
			perror("error writing config file");
			printf("name =%s", mwconf.dataname);
			close_datafile();
			return CEN_PROMPT;
		}

		dlen+=rout_len;
		if (config.tcount && (++tc >= config.tcount)) {
			printf("\r%08lX\n", dlen);
			sconf=config.sc_db+LOW_SC_UNIT+1;
			eventcount=sconf->sc_unit->event_nr;
			return CE_PROMPT;
		}

		if ((kbatt-lp) >= 0x1000) {
			lp=kbatt;
			printf("\r%08lX", dlen);
		}
	}
}
//
//--------------------------- mwpc_application ------------------------------
//
int mwpc_application(void)
{
	u_int	i;
	char	key;
	int	ret;

	while (1) {
		clrscr();
		i=0;
		gotoxy(SP_, i+++ZL_);
						printf("data file ................0");
#ifdef LOGFILE
		gotoxy(SP_, i+++ZL_);
						printf("log file .................1");
#endif
		gotoxy(SP_, i+++ZL_);
						printf("setup ....................2/s");
		gotoxy(SP_, i+++ZL_);
						printf("threshold ................3/d");
	if (datafile >= 0) {
		gotoxy(SP_, i+++ZL_);
						printf("log info .................4/i");
		gotoxy(SP_, i+++ZL_);
						printf("data acquisition .........5/a");
		gotoxy(SP_, i+++ZL_);
						printf("daq with test pulse ......6/t");
	}
		gotoxy(SP_, i+++ZL_);
						printf("Ende ...................ESC");
		gotoxy(SP_, i+++ZL_);
						printf("                          %c ",
									config.tmp_menu);

		key=toupper(getch());
		if ((key == CTL_C) || (key == ESC)) {
#ifdef LOGFILE
			close_logfile();
#endif
			close_datafile();
			return 0;
		}

		while (1) {
			use_default=(key == TAB);
			if ((key == TAB) || (key == CR)) key=config.tmp_menu;

			clrscr(); gotoxy(1, 10);
			errno=0; last_key=0; ret=CEN_MENU;
			switch (key) {

		case '0':
				printf("datafile: ");
				ret=open_datafile();
				Writeln();
				break;

#ifdef LOGFILE
		case '1':
				printf("logfile:  ");
				ret=open_logfile();
				break;
#endif

		case '2':
		case 'S':
				ret=setup();
				if ((ret < 0) && (ret > CE_PROMPT)) break;

				ret=CE_PROMPT;
				break;

		case '3':
		case 'D':
				ret=inp_setdac(0, 0, -2, DAC_DIAL);
				if ((ret < 0) && (ret > CE_PROMPT)) break;

				ret=CE_PROMPT;
				break;

		case '4':
		case 'I':
				loginfo();
				ret=CEN_PROMPT;
				break;

		case '5':
		case 'A':
		case 'Q':
				ret=daq(0);
				config.tmp_menu=key;
				break;

		case '6':
		case 'T':
				ret=daq(1);
				config.tmp_menu=key;
				break;

			}

			stop_ddma();
			while (kbhit()) getch();
			if ((ret &~0x1FF) == CE_KEY_PRESSED) ret =CE_PROMPT;

			if ((ret >= CEN_MENU) && (ret <= CE_MENU)) break;

			if (ret > CE_PROMPT) display_errcode(ret);

			printf("select> ");
			key=toupper(getch());
			if ((key < ' ') && (key != TAB)) break;

			if (key == ' ') key=CR;
		}
	}
}
#endif

#ifdef TOF
//===========================================================================
//
//										TOF applications
//
//===========================================================================
//
static struct time	timerec;

static void log_dac(void)
{
	int		sc;
	SC_DB		*sconf;
	u_int		ip;
	INP_DB	*iconf;
	u_int		con;
	u_int		cha;
	u_int		col;

	for (sc=LOW_SC_UNIT, sconf=config.sc_db+LOW_SC_UNIT+1;
		  sc < config.sc_units; sc++, sconf++) {
		fprintf(logfile, "DACs: ", sconf->unit); cha=0; col=0;
		for (ip=0, iconf=sconf->inp_db; ip < sconf->slv_cnt; ip++, iconf++) {
			if (++col >= 4) { col=0; fprintf(logfile, "\n   "); }

			for (con=0; con < 4; con++) {
				fprintf(logfile, " %u/%1.*f;", cha,
					 THRHW, THRH_MIN+THRH_SLOPE*iconf->inp_dac[con]);
				cha +=16;
			}
		}

		fprintf(logfile, "\n");
	}
}

// input by read_event():
//
//		tm_cha[tm_cnt], tm_diff[tm_cnt] => sorted by channel and time
//
int tof_application(void)
{
	int		ret;
	int		trig;
	u_long	cmask[2*C_UNIT];	// channels to display
	u_int		len;
	u_int		i,j;
	u_int		hit,more;
	u_int		ln;
	int		cha;
	int		l_cha;				// channel last displayed
	int		k;
	int		tm;
	u_int		col;
	char		dsp;
	u_long	lp;

	float		resol=RESOLUTION/1000.;

	printf("falling/rising/both(LeTra) edge, 0/1/2: %u ", config.daq_edge);
	config.daq_edge=(u_char)Read_Deci(config.daq_edge, 2);
	if (errno) return CE_MENU;

	printf("0: triggered hits, 1: raw hits  %u ", config.trig_mode);
	config.trig_mode=(u_char)Read_Deci(config.trig_mode, 1);
	if (errno) return CE_MENU;

	trig=RO_RAW;
	if (config.trig_mode) {
		config.daq_mode=EVF_SORT|EVF_HANDSH|MCR_DAQRAW;
		config.rout_delay=40;
	} else {
		config.daq_mode=EVF_SORT|EVF_HANDSH|MCR_DAQTRG;
		printf("trigger latency (ns)  %6.1f ", config.ef_trg_lat);
		config.ef_trg_lat=Read_Float(config.ef_trg_lat);
		if (errno) return CE_MENU;

		printf("trigger window  (ns)  %6.1f ", config.ef_trg_win);
		config.ef_trg_win=Read_Float(config.ef_trg_win);
		if (errno) return CE_MENU;
	}

	printf("logfile                      ");
	if ((ret=open_logfile()) != 0) return ret;

	for (i=0; i < 2*C_UNIT; cmask[i++]=0);

	use_default=1;
	config.hsdiv=(u_int)(HSDIV/120.0);
	config.f1_sync=2;
	switch (config.daq_edge) {
case 0:
		config.f1_letra=0x12; break;

case 1:
		config.f1_letra=0x11; break;

default:
		config.f1_letra=0x17; break;
	}

	if ((ret=general_setup(-2, -1)) <= CE_PROMPT) ret=CEN_PROMPT;

	if ((ret=inp_mode(0)) != 0) return ret;

	if (logfile) {
		gettime(&timerec);
		fprintf(logfile, "%2d:%02d:%02d", timerec.ti_hour, timerec.ti_min, timerec.ti_sec);
		switch (config.daq_edge) {
case 0:
			fprintf(logfile, ", falling edge"); break;

case 1:
			fprintf(logfile, ", rising edge"); break;

default:
			fprintf(logfile, ", LeTra mode"); break;
		}

		if (config.trig_mode) fprintf(logfile, ", raw mode");
		else
			fprintf(logfile, ", latency/window %1.1f/%1.1fns",
					  config.ef_trg_lat, config.ef_trg_win);
		fprintf(logfile, ", resolution %1.2fps\n", RESOLUTION);
		log_dac();
	}

	use_default=0; lp=0;
	ln=0; dsp=0;
	while (1) {
		if ((ret=read_event(0, trig)) != 0) {
			if (ret <= CEN_PPRINT) {
				if (logfile) log_dac();

				continue;
			}

			Writeln();
			if (!logfile || (ret != (CE_KEY_PRESSED |' '))) {
				close_logfile();
				return ret;
			}

			dsp=1;
		}

		len=rout_len/4;
		if (len <= 3) continue;

		if (logfile) {
			col=0;
#ifdef SC_M
			fprintf(logfile, "%1.1fms;", cm_reg->timer/10.);
#endif
			if (config.trig_mode)		// raw mode
				for (i=3; i < len; i++) {
					if (++col >= 10) {
						col=0; fprintf(logfile, "\n    ");
					}

					fprintf(logfile, " %u/%u;", EV_CHAN(robf[i]), EV_FTIME(robf[i]));
				}
			else
				for (i=3; i < len; i++) {
					if (++col >= 10) {
						col=0; fprintf(logfile, "\n    ");
					}

					fprintf(logfile, " %u/%i;", EV_CHAN(robf[i]), EV_FTIME(robf[i]));
				}

			fprintf(logfile, "\n");
			lp++;
			if (!(lp &0x0FF)) {
				printf("\r%9lu", lp);
			}

			if (!dsp) continue;

			dsp=0;
		}

		if (ln >= 40) {
			ln=0; Writeln();
			for (cha=0, col=0; (cha < 64*C_UNIT); cha++) {
				if (!(cmask[cha >>5] &(1L <<(cha &0x1F)))) continue;

				if (col) printf(" ");

				printf("C%03X", cha);
				if (++col >= 16) { Writeln(); col=0; }
			}

			if (col) Writeln();
		}

//		mehrfach Hits werden untereinander ausgegeben.
//			Werte stehen hintereinander.
//
//for (i=3; i < len; i++) printf("  %08lX", robf[i]);
		hit=0; more=0; l_cha=-1; col=0; i=3;
		while (1) {
			if (i >= len) {
				if (more == 0) break;

				if (col) Writeln();

				hit++; more=0; l_cha=-1; col=0; i=3;
				continue;
			}

			cha=EV_CHAN(robf[i]); k=0; j=0;
			while(1) {
				if (k == hit) j=i;

				k++; i++;
				if ((i >= len) || (cha != EV_CHAN(robf[i]))) break;

				if (k > hit) more=1;
			}

			if (j == 0) continue;

			if (!(cmask[cha >>5] &(1L <<(cha &0x1F)))) {
				cmask[cha >>5] |=(1L <<(cha &0x1F));
				ln=100;
			}
//
//--- Platzhalter fuer nicht vorhandene Kanaele
//
			while (++l_cha < cha) {
				if (!(cmask[l_cha >>5] &(1L <<(l_cha &0x1F)))) continue;

				if (col) printf(" ");

				printf("    ");
				if (++col >= 16) { Writeln(); ln++; col=0; }
			}

			tm=EV_FTIME(robf[j]);
			if (col) printf(" ");

			if (config.trig_mode) {		// raw mode
				if (   (config.f1_letra &0x04)				// letra mode
					 && (hit == 1)
					 && (EV_FTIME(robf[j-1]) &1) && !(tm &1)) {	// letra paar
					u_long tm0=((u_long)EV_MTIME(robf[j-1]) <<16) |EV_FTIME(robf[j-1]);
					u_long tm1=((u_long)EV_MTIME(robf[j]) <<16) |EV_FTIME(robf[j]);

					printf("w%3.0f", ((tm1&~1)-tm0) *resol);
				} else printf("%4.0f", (u_int)tm*resol);
			} else {
				if (   (config.f1_letra &0x04)				// letra mode
					 && (hit == 1)
					 && (EV_FTIME(robf[j-1]) &1) && !(tm &1)) {	// letra paar
					printf("w%3.0f", ((int)EV_FTIME(robf[j-1]-(tm&~1))) *resol);
				} else printf("%4.0f", tm*resol);
			}

			if (++col >= 16) { Writeln(); ln++; col=0; }
		}

		if (col) Writeln();
	}
}
#endif
