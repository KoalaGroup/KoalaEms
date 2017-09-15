#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <conio.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include "pci.h"

typedef unsigned char	u_char;
typedef unsigned short	u_short;
typedef unsigned long	u_long;
typedef unsigned int		u_int;

#define OFFSET(strct, field)	((u_int)&((strct*)0)->field)

#define Q_ENA			0x001	// gen. enable
#define Q_LTRIG		0x002	// local on board trigger, NIM input
#define Q_LEVTRG		0x004	// or data level
#define Q_CLKPOL		0x008	// DAC clock polarity
#define Q_ADCPOL		0x040	// inverse ADC data
#define Q_RAW			0x080	// insert ADC data
#define Q_SGRD			0x100	// single gradient

#define QS_DIS0		0x10	// disable channel 0
#define QS_DIS1		0x20	// disable channel 1

#define QDCADC		7
#define QDCVSL		9

#define HDR_LEN	3

#define CEN_PROMPT		-2		// display no command menu, only prompt
#define CEN_MENU			-1
#define CE_PROMPT			2		// display no command menu, only prompt

#define CE_KEY_SPECIAL	0x100
#define CE_KEY_PRESSED	0x200

typedef struct {
	u_long	hd[3];
	u_short	lat,win,dac0,dac1,tlev,cr,mean,noise;
} DAT_HDR;

typedef struct {
	u_int		dac0,dac1;
	u_int		a_min,a_max;
	u_char	comp;
	u_char	bool;
#define _POL	0x01
#define _ITG	0x02
#define _RAW	0x04
#define _GRD	0x08
#define _MEA	0x10
#define _EDG	0x20

	u_int		shift;
	u_int		noise;
	u_char	res0;
	u_char	res1;
	u_int		tlevel;
	int		latency;
	int		window;
} SQ_DATA;

#define C_BUF	0x0800
#define C_POS	0x0800

typedef struct {
	PCI_CONFIG	pconf;

} CONFIG;

CONFIG		config;		// will be imported at pci.c, pciconf.c and pcibios.c

static u_char	typ;
static DAT_HDR	dhdr;
static u_long	dmabuf[C_BUF];
static u_long	buspos[C_POS];
static u_int	pos;
static FILE		*logfile=0;
static int   	datafile=-1;
static SQ_DATA	sq_data;
static SQ_DATA	*sqd;
static u_int	rout_len;
static u_int	ld,li;
static u_int	hdr;
static char		logname[80];
static u_int	_XF;
static float	_NS;
static float	_MV;
static u_int	_FG;
static u_int	sq_cha;
static char		_pbuf[800];
static char		*pbuf=_pbuf;
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
		printf("name =%s", logname);
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

	printf("logfile: ");
	str=Read_String(logname, sizeof(logname));
	if (errno) {
		if (close_logfile()) return CEN_PROMPT;
		return CEN_MENU;
	}

	if (logfile && !strcmp(str, logname)) return 0;

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

	strcpy(logname, str);
	return 0;
}
//
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
		int	mean=(int)dmabuf[ld+1];

		i=dhdr.cr;
		fprintf(logfile, "** len  %4u/%u\n", ld-hdr, li-ld);
		fprintf(logfile, "** lat  %04X\n", dhdr.lat);
		fprintf(logfile, "** win  %04X\n", dhdr.win);
		if (i &Q_LEVTRG)
			fprintf(logfile, "** tlv  %04X\n", dhdr.tlev);

		fprintf(logfile, "** cr   %04X\n", i &~Q_CLKPOL);
		fprintf(logfile, "** mean %04X\n", dhdr.mean);
		fprintf(logfile, "** nois %04X\n", dhdr.noise);
		for (i=ld; i < li; i++) {
			fprintf(logfile, "* 0x%08lX", dmabuf[i]);
		}
		fprintf(logfile, "\n");

		for (i=ld; i < li; i++) {
			tmp=dmabuf[i]; id=(u_int)(tmp >>16) &0x3F;
			if ((id == 0x30) || (id == 0x32) || (id == 0x34)) {
				if (id+1 != ((u_int)(dmabuf[i+1] >>16) &0x3F)) break;

				switch (id) {
		case 0x30:
					if (i == ld) fprintf(logfile, "** start%6.1fns %6.1fmV mean",
												((u_int)dmabuf[ld]-(int)tmp)*25.0/16.0,
												(int)dmabuf[i+1]*_MV);
					else         fprintf(logfile, "** end  %6.1fns %6.1fmV",
												((u_int)dmabuf[ld]-(int)tmp)*25.0/16.0,
												(int)dmabuf[i+1]*_MV);
					break;

		case 0x32:
					fprintf(logfile, "** min  %6.1fns %6.1fmV",
							  ((u_int)dmabuf[ld]-(int)tmp)*25.0/16.0,
							  (int)dmabuf[i+1]*_MV);
					break;

		case 0x34:
					fprintf(logfile, "** max  %6.1fns %6.1fmV",
							  ((u_int)dmabuf[ld]-(int)tmp)*25.0/16.0,
							  (int)dmabuf[i+1]*_MV);
					break;
				}

				i++;
				continue;
			}

			if (id == 0x36) {
				id=(u_int)tmp>>14;
				if (tmp &0x2000) tmp |=0xC000; else tmp &=~0xC000;

				fprintf(logfile, "** zcros%6.1fns    %u",
						  ((u_int)dmabuf[ld]-(int)tmp)*25.0/16.0,
						  id);
				continue;
			}

			if ((id &0x30) == 0x20) {
				fprintf(logfile, "** charg %5.2f Vns", (tmp &0xFFFFFl)*0.00625);
				continue;
			}

			break;
		}

		fprintf(logfile, "\n");

		for (i=hdr; i < ld; i++) {
			sprintf(pbuf, "%1.2f;%1.1f",
					  (i-hdr)*_NS, ((int)dmabuf[i]-mean)*_MV);
			for (id=0; pbuf[id]; id++)
				if (pbuf[id] == '.') pbuf[id]=',';

			fprintf(logfile, "%s\n", pbuf);
		}

		close_logfile();
		return 0;
	}

	i=dhdr.cr;
	fprintf(logfile, "-- len  %4u/%u\n", ld-hdr, li-ld);
	fprintf(logfile, "-- lat  %04X\n", dhdr.lat);
	fprintf(logfile, "-- win  %04X\n", dhdr.win);
	if (i &Q_LEVTRG)
		fprintf(logfile, "-- tlv  %04X\n", dhdr.tlev);

	fprintf(logfile, "-- cr   %04X\n", i &~Q_CLKPOL);
	fprintf(logfile, "-- mean %04X\n", dhdr.mean);
	fprintf(logfile, "-- nois %04X\n", dhdr.noise);
	for (i=ld; i < li; i++) {
		fprintf(logfile, "-- %08lX\n", dmabuf[i]);
	}

	if (i &Q_ADCPOL) {
		for (i=hdr; i < ld; i++)
			fprintf(logfile, "%04X %04X\n",
					  (u_int)dmabuf[i]^0xFFF,
					  (u_int)dmabuf[i]);

		fprintf(logfile, "%04X %04X\n",
				  (u_int)dmabuf[ld+1]^0xFFF,
				  (u_int)dmabuf[ld+1]);
	} else {
		for (i=hdr; i < ld; i++)
			fprintf(logfile, "%04X\n",
					  (u_int)dmabuf[i]);

		fprintf(logfile, "%04X\n",
				  (u_int)dmabuf[ld+1]);
	}

	close_logfile();
	return 0;
}
//
#if 0
//--------------------------- qdc_analyse -----------------------------------
//
static void qdc_analyse(void)
{
	u_int		i;

	u_int		e_latancy;
	u_int		e_ystrt;
	u_int		e_x;
	u_int		e_y,e_y_1,e_y_2;
	u_int		e_y_3,e_y_4;
	u_int		e_ydif1;
	u_int		e_ydif2,e_ydif4;
	u_int		e_m1,e_m2,e_m4;
	u_int		e_x1,e_x2,e_x4;
	u_int		e_y1,e_y2,e_y4;
	u_int		e_y20,e_y40;
	u_int		e_mdent,e_mdsor;
	u_int		e_quot;
	u_int		e_remd;
	u_int		e_prog;
	u_int		e_xm0;
	u_int		e_xm1,e_xm2,e_xm3;
	u_int		e_xm4;
	u_int		e_mdx;
	u_int		e_xmax,e_xmin;
	u_char	e_ovr;
	u_int		e_yboard;
	u_int		e_xboard;
	u_int		e_ymin,e_yminp;
	u_int		e_ymini;
	u_int		e_ymax;
	u_int		e_ymax1_4,e_ymax3_4;
	u_int		e_ymaxr;
	u_int		e_ymaxt;
	u_int		e_ymaxtm;
	u_int		e_gid;
	u_int		e_ymi_ma;
	u_char	e_rising,e_falling;
	u_char	e_ena0,e_ena1;
	u_int		e_enacnt;
	u_int		e_omin;
#define C_OMIN		7

	u_int		e_umax;
	u_int		e_dq,e_q0,e_q1;
	u_int		e_qs0,e_qs1;
	u_char	ep_brderx,ep_brdery;
	u_char	ep_min_x,ep_min_y;
	u_char	ep_max_x,ep_max_y;
	u_char	ep_qa,ep_q;
	u_char	ep_zero;
	u_int		ep_din,ep_dout;
	u_char	ep_wen,ep_ren;
	u_char	ep_empty;
	u_char	ep_valid;

	u_char	e_ovr_;

	ep_brderx= 1;
	ep_zero	= 0;
	ep_q		= 0;
	e_xboard	= 0;
	e_yboard	= (u_int)dmabuf[ld+1];
	e_y		= (u_int)dmabuf[hdr];
	e_ystrt	= (u_int)dmabuf[hdr];
	e_xmax	= 0;
	e_ymax	= (u_int)dmabuf[hdr];
	e_xmin	= 0;
	e_ymin	= (u_int)dmabuf[hdr];

	e_prog	= 0;
	e_ovr		= 0;
	e_rising	= 0;
	e_falling= 0;
	e_x		= 0;
	e_ena0	= 0;
	e_ena1	= 0;
	e_enacnt	= 0;
	e_umax	= 0;
	e_m1		= 0;
	e_m2		= 0;
	e_m4		= 0;
	e_q0		= 0;
	ep_min_x	= 0;

	for (i=hdr+1; i < ld; i++) {
		ep_brderx= 0;
		ep_brdery= ep_brderx;
		ep_min_y	= 0;
		if (!ep_zero && !ep_q) {
			ep_min_x	= 0;
			ep_min_y	= ep_min_x;
		}

		ep_max_y	= 0;
		if (!ep_zero) {
			ep_max_x	= 0;
			ep_max_y	= ep_max_x;
		}

		if (!ep_min_y && !ep_zero && !ep_brdery) {
			ep_q		= 0;
		}

		if (!ep_min_y && !ep_max_y) {
			ep_zero	= 0;
		}


		e_x		= e_x +1;

		e_y		= (u_int)dmabuf[hdr+i];
		e_ovr_	=  (e_y == 0xFFF)
					 | (e_ovr & !ep_max_y);
		e_y_1		= e_y;
		e_y_2		= e_y_1;
		e_y_3		= e_y_2;
		e_y_4		= e_y_3;
		e_umax	= e_umax +1;

		e_prog	= e_prog <<1;
		if (e_ena1 && (e_prog &((e_prog'high)) {
			ep_zero	= 1;
		}

		e_q0		= e_q0 +e_dq;
		e_q1		= e_q1 +e_dq;

		if    (!e_ydif1(e_ydif1'high))			-- max gradient
		  && (e_m1 < e_ydif1(T_SLV12'range)) {
			e_m1		= e_ydif1(T_SLV12'range);
			e_x1		= e_x -1;
			e_y1		= e_y_1-e_yminp;
		}

		if    (!e_ydif2(e_ydif2'high))			-- max gradient
		  && (e_m2 < e_ydif2(T_SLV12'range)) {
			e_m2		= e_ydif2(T_SLV12'range);
			e_x2		= e_x -2;
			e_y2		= e_y_2;
			e_y20		= e_y;
		}

		if    (!e_ydif4(e_ydif4'high))			-- max gradient
		  && (e_m4 < e_ydif4(T_SLV12'range)) {
			e_m4		= e_ydif4(T_SLV12'range);
			e_x4		= e_x -4;
			e_y4		= e_y_4;
			e_y40		= e_y;
		}

		if (   e_prog(16) || e_prog(12)
				  || e_prog(8) || e_prog(4)) {
			e_xm1		= e_xm0;
			e_xm2		= e_xm1;
			e_xm3		= e_xm2;
			e_xm4		= (e_latancy -EXT2SLV(e_xm3, 16-ITIME_FRAG))
							  &w_itfrag;
		}

		if   (e_y <= e_ymin)
		  || (e_y <= MEAN_LEV) {	-- calcutate min
			e_ymin	= e_y;
			e_xmin	= e_x;

			e_ymaxt	= (u_int)dmabuf[hdr+i-1];
			e_ymaxtm	= (u_int)dmabuf[hdr+i-1] <<1;

			e_q0		= (others => 0);
			e_qs0		= e_q1 +e_dq;

			e_m1		= (others => 0);
			e_m2		= (others => 0);
			e_m4		= (others => 0);
		}

		if (e_y >= e_ymax) {
			e_ymax	= e_y;
			e_xmax	= e_x;
			e_umax	= (others => 0);
		}

		if (w_wcnt(0)) {
			if (b_dout(23 downto 12) > e_ymaxt) {
				e_ymaxt	= b_dout(23 downto 12);
				e_ymaxtm	= (0&e_ymin) +b_dout(23 downto 12);
			}
		else
			if (b_dout(11 downto 0) > e_ymaxt) {
				e_ymaxt	= b_dout(11 downto 0);
				e_ymaxtm	= (0&e_ymin) +b_dout(11 downto 0);
			}
		}

		if   (e_y <= MEAN_LEV+3)		-- quiet
		  || (e_y <= e_ystrt)			-- ? falling
		  || (e_y+3 <= e_ymax) {	-- when falling, pile up
			e_enacnt	<= e_enacnt+1;
		}

		if (e_enacnt = 3) {
			e_ena0	= 1;
		}

		e_omin	= (others => 0);
		if    (e_y >= e_ymini)
		  && (e_y >= e_ymaxtm(T_SLV12'high+1 downto 1)) {
			e_omin	= e_omin+ 1;
		}

		if (!e_rising) && (e_omin = C_OMIN) {	-- a)
			e_rising	= 1;
			e_umax	= (others => 0);
			e_ena1	= e_ena0;
			e_q0		= e_q1 +e_dq;
			e_q1		= e_q0 +e_dq;
			if (e_ena0) {
				ep_min_x	= 1;
			}
		}

		ep_qa	= 0;
		if    (e_falling)
		  && (   (e_omin = C_OMIN)
				 || (e_y <= MEAN_LEV)) {	-- c)
			e_falling= 0;
			e_ymax	= e_y;
			e_xmax	= e_x;
			ep_min_x	= 1;
			if (e_ena1) {
				ep_qa	= 1;
			}
		}

		if (   ep_qa
				  || (!e_step1 && e_falling && e_ena1)) {	-- window end
			ep_q	= 1;
			e_qs1	= e_qs0;
		}

		if (e_rising) && (e_umax >= 5) && (e_y < e_ymax) {		-- b)
			e_rising	= 0;
			e_falling= 1;
			ep_max_x	= 1;

			e_omin	= (others => 0);
			e_ymin	= e_y;
			e_xmin	= e_x;

			if   (CREG(8))
			  || ((e_y2 < e_ymax1_4) || (e_y20 > e_ymax3_4)) {
				e_mdent	= e_y1;
				e_mdsor	= e_m1;
				e_xm0		= e_x1;
				e_gid		= "00";
			elsif (e_y4 < e_ymax1_4) || (e_y40 > e_ymax3_4) {
				e_mdent	= (e_y2(10 downto 0)&0)-(e_yminp(10 downto 0)&0);
				e_mdsor	= e_m2;
				e_xm0		= e_x2;
				e_gid		= "01";
			else
				e_mdent	= (e_y4(9 downto 0)&"00")-(e_yminp(9 downto 0)&"00");	-- ??
				e_mdsor	= e_m4;
				e_xm0		= e_x4;
				e_gid		= "10";
			}

			if (e_ena1) {
				e_prog	= (0 => 1, others => 0);		-- start division (e_y0/e_m)
			}
		}

		e_yminp	= e_ymin;
		if (e_ymin < MEAN_LEV) {
			e_yminp	= MEAN_LEV;
		}

		e_ymini	= e_ymin +e_ymaxr(T_SLV12'high downto 3);	-- 1/8 of e_ymax
--			if (e_ymini < MEAN_LEV+10) {
--				e_ymini	= MEAN_LEV+10;
		if (e_ymini < e_yminp+MEAN_TOL) {
			e_ymini	= e_yminp+MEAN_TOL;
		}

		if (!w_trig && e_step1 && e_falling) {
			ep_brderx= 1;
			e_xboard	= e_x;
			e_yboard	= ("0000"&e_y) -MEAN_LEV;
		}


}
//
#endif
//--------------------------- qdc_sample ------------------------------------
//
#define PRT_X		80
#define QPRT_Y		44
#define QPRT_Y0	45

static u_char	prt_dis[PRT_X];
static u_char	prt_mdis;

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

static void smp_adjust(void)
{
	u_int		i;

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

	gotoxy(1, QPRT_Y0+3);
	if (typ == QDCVSL) {
		cmd_string("_cha=");
		printf("%u", sq_cha);
	}
	cmd_string("_lo/_up limit=");
	printf("%04X/%04X", sqd->a_min, sqd->a_max);
	cmd_string(", _Shi=");
	printf("%u.", sqd->shift);
	if (typ == QDCVSL) {
		cmd_string(", _dAC0/_D1=");
		printf("%04X/%04X", sqd->dac0, sqd->dac1);
	}
	cmd_string(", c_pr=");
	printf("%3u.        ", sqd->comp);
	gotoxy(1, QPRT_Y0+4);
	if (sqd->bool &_ITG) cmd_string("InTr_ig, "); else cmd_string("ExTr_ig, ");
	cmd_string("_Lat=");
	printf("%i.", dhdr.lat);
	cmd_string(", _win=");
	printf("%i.", dhdr.win);
	cmd_string(", tle_v=");
	printf("%u.", dhdr.tlev);
	cmd_string(", p_ol=");
	printf((sqd->bool &_POL) ? "-" : "+");
	cmd_string(", s_grd=");
	printf((sqd->bool &_GRD) ? "T" : "F");
	cmd_string("   press _h for help     ");

	hdr=HDR_LEN;
	return;
}

static int rd_event(void)
{
	int	ret;

	if (datafile < 0) return CE_PROMPT;

	if (kbhit()) {
		ret=getch();
		if (ret) return CE_KEY_PRESSED|ret;

		return CE_KEY_PRESSED|CE_KEY_SPECIAL|getch();
	}

	if (rout_len) return 0;

	if (pos < C_POS) buspos[pos++]=lseek(datafile, 0, SEEK_CUR);

	if ((ret=read(datafile, dmabuf, 4*HDR_LEN)) != 4*HDR_LEN) {
		gotoxy(1, 1);
		printf("%04X  \n", ret);
		return CE_PROMPT;
	}

	if ((dmabuf[0]>>16) != 0x8000) return CE_PROMPT;

	rout_len=(u_int)dmabuf[0];
	if (rout_len <= 4*HDR_LEN) return 0;

	if (read(datafile, dmabuf+HDR_LEN, rout_len-4*HDR_LEN) != rout_len-4*HDR_LEN) return CE_PROMPT;

	return 0;
}

static int qdc_sample(void)
{
	int		ret;
	u_int		i,n,tmp;
	u_int		imx=0,imn=0;
	u_char	rkey,key;
	u_char	single=0;
	u_char	dis;
	u_char	mdis;
	u_char	con;

	smp_adjust();
	use_default=0;
	rout_len=0; ld=0; li=0;
	con=1;
	while (1) {
		if (con) { _setcursortype(_NOCURSOR); con=0; }

		if ((ret=rd_event()) != 0) {
			_setcursortype(_NORMALCURSOR); con=1;
			if (!(ret &(CE_KEY_PRESSED|CE_KEY_SPECIAL))) {
				gotoxy(1, QPRT_Y0+5);
				return ret;
			}

			use_default=0;
			if (ret &CE_KEY_SPECIAL) {
				rkey=(u_char)ret;
				if (rkey == UP) {
					rout_len=0;
					continue;
				}

				if (rkey == DOWN) {
					if (pos >= 2) {
						pos -=2;
						lseek(datafile, buspos[pos], SEEK_SET);
						rout_len=0;
					}

					continue;
				}

				if (rkey == RIGHT) {
					sqd->shift +=(64*(sqd->comp+1));
					continue;
				}

				if (rkey == LEFT) {
					if (sqd->shift < (64*(sqd->comp+1))) sqd->shift=0;
					else sqd->shift -=(64*(sqd->comp+1));

					continue;
				}

				continue;
			}

			key=toupper(rkey=(u_char)ret);
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

				printf("l   lower edge of display window\n");
				printf("u   upper edge of display window\n");
				printf("a   adjust values for lower and upper edge\n");
				printf("f   full range\n");
				printf("p   compress factor for time axis\n");
				printf("S   shift timing left\n");
				printf("m   toggle displaying mean line\n");
				WaitKey(0);
				smp_adjust();
				continue;
			}

			if (key == CR) {
				gotoxy(26, QPRT_Y0+2);
				printf("min/max(dif)/mean/noi=%03X/%03X(%u.)/%03X/%u.    ",
						 imn, imx, imx-imn,
						 dhdr.mean,
						 dhdr.noise);
				gotoxy(26, QPRT_Y0+5);
				printf("len=%03X/%X", ld, li-ld);
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

			if (rkey == 'l') {
				printf("%25s\r", "");
				printf("lower lim  %04X ", sqd->a_min);
				sqd->a_min=(u_int)Read_Hexa(sqd->a_min, -1);
				smp_adjust();
				continue;
			}

			if (key == 'U') {
				printf("%25s\r", "");
				printf("upper lim  %04X ", sqd->a_max);
				sqd->a_max=(u_int)Read_Hexa(sqd->a_max, -1);
				smp_adjust();
				continue;
			}

			if (rkey == 'S') {
				printf("%25s\r", "");
				printf("shift     %4u. ", sqd->shift);
				sqd->shift=(u_int)Read_Deci(sqd->shift, -1);
				smp_adjust();
				continue;
			}

			if (key == 'P') {
				printf("%25s\r", "");
				printf("compress  %4u. ", sqd->comp);
				sqd->comp=(u_int)Read_Deci(sqd->comp, -1);
				smp_adjust();
				continue;
			}

			if (rkey == 'm') {
				sqd->bool ^=_MEA;
				smp_adjust();
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

				smp_adjust();
				continue;
			}

			if (key == 'F') {
				sqd->a_min=0;
				sqd->a_max=0x1000;
				smp_adjust();
				continue;
			}

			if ((rkey == 'M') || (key == 'X')) {
				printf("%25s\r", "");
				logdata(key == 'X');
				continue;
			}

			continue;
		}

		while (1) {
			gotoxy(1, 1); printf("%3u", pos-1);

			li=rout_len/4; ld=li;
			while ((ld > hdr) && (dmabuf[ld-1] &0x00200000L)) ld--;

			if (single == 0) {
				u_int	id;

				if (ld != hdr+_XF*(sqd->window+1)) {
					gotoxy(1, 1);
					printf("l=%04X/%04X exp %04X ", ld, li, hdr+2*(sqd->window+1));
					_setcursortype(_NORMALCURSOR); con=1;
					if ((rkey=getch()) == ESC) {
						gotoxy(1, QPRT_Y0+5);
						return CE_PROMPT;
					}

					if (rkey == TAB) single=3;
				}

				for (i=ld; i < li; i++) {
					id=(u_int)(dmabuf[i] >>16) &0x3F;
					if ((id == 0x30) || (id == 0x32) || (id == 0x34)) {
						if (id+1 != ((u_int)(dmabuf[i+1] >>16) &0x3F)) break;
						i++; continue;
					}

					if ((id == 0x36) || ((id &0x30) == 0x20)) continue;
					break;
				}

				if (i < li) {
					smp_adjust();
					single=2;
					continue;
				}
			}

			imx=0; imn=0xFFFF;
			if (sqd->bool &_MEA) {
				tmp=(u_int)dmabuf[ld+1];
				if (tmp <= sqd->a_min) mdis=0;
				else
					if (tmp > sqd->a_max) mdis=2*QPRT_Y;
					else mdis=(u_int)(
								 ((u_long)2*QPRT_Y*(tmp-sqd->a_min)+(sqd->a_max-sqd->a_min)/2)
								 /(sqd->a_max-sqd->a_min)
										 );
			}

			for (n=0, i=sqd->shift+hdr; (n < PRT_X) && (i < ld); n++) {
				u_long	ltmp=(tmp=(u_int)dmabuf[i++]) &0x0FFF;
				u_int		j=1;
				u_int		ldis;		// last position
				u_int		ovr=0;

				if (((tmp&0xFFF) == 0) || ((tmp &0xFFF) == 0xFFF)) ovr=1;
				while ((j <= sqd->comp) && (i < ld)) {
					j++; ltmp+=(tmp=(u_int)dmabuf[i++]) &0x0FFF;
					if (((tmp&0xFFF) == 0) || ((tmp &0xFFF) == 0xFFF)) ovr=1;
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
	gotoxy(1, QPRT_Y0+5);
	printf(" %3u %2u %2u %2u ", prt_mdis, mdis, prt_dis[n], dis);
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

#if 0
			if (single == 0) {
				for (i=ld; i < li; i++) {
					if (((u_int)(dmabuf[i] >>16) &0x3F) == 0x36) {
						break;
					}
				}

				if (i == li) {
					gotoxy(1, QPRT_Y0+1);
					printf("no zero\n");
					_setcursortype(_NORMALCURSOR); con=1;
					key=toupper(rkey=getch());
					if (rkey == TAB) single=3;
					if (key == 'M') {
						logdata();
						return CE_PROMPT;
					}

					continue;
				}
			}
#endif
			if (single == 1) {
				_setcursortype(_NORMALCURSOR); con=1;
				if (toupper(getch()) != 'S') single=0;
				continue;
			}

			if (single == 2) {
				u_long	tmp;
				u_int		id;
				u_int		x=80-24, y=1;

				for (i=ld; i < li; i++) {
					tmp=dmabuf[i]; id=(u_int)(tmp >>16) &0x3F;
					gotoxy(x, y++);
					if ((id == 0x30) || (id == 0x32) || (id == 0x34)) {
						if (id+1 != ((u_int)(dmabuf[i+1] >>16) &0x3F)) break;

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
														  (int)dmabuf[i+1]*_MV,
														  ((u_int)dmabuf[ld]-(u_int)tmp) >>_FG);
						i++;
						continue;
					}

					if (id == 0x36) {
						id=(u_int)tmp>>14;
						if (tmp &0x2000) tmp |=0xC000; else tmp &=~0xC000;

						printf("zcros%5.0fns      %u   %03X", (int)tmp*25.0/16.0, id,
													((u_int)dmabuf[ld]-(u_int)tmp) >>_FG);
						continue;
					}

					if ((id &0x30) == 0x20) {
						printf("charg %lu", tmp &0xFFFFFl);
						continue;
					}

					break;
				}

				gotoxy(1, y);
				if (i < li) {
					printf("sequence error\n");
					for (i=ld; i < li; i++) printf("  %08lX", dmabuf[i]);

					if ((li-ld) %8) Writeln();
				}

				_setcursortype(_NORMALCURSOR); con=1;
				while (1) {
					key=toupper(rkey=getch());
					if (key != 'R') break;

					for (i=ld; i < li; i++) printf("  %08lX", dmabuf[i]);

					if ((li-ld) %8) Writeln();
				}

				if (rkey == 0) {
					rkey=getch();
					if (rkey == UP) {
						rout_len=0;
						break;
					}

					if (rkey == DOWN) {
						if (pos < 2) continue;

						pos -=2;
						lseek(datafile, buspos[pos], SEEK_SET);
						rout_len=0;
						break;
					}

					if (rkey == RIGHT) {
						sqd->shift +=(64*(sqd->comp+1));
						smp_adjust();
						continue;
					}

					if (rkey == LEFT) {
						if (sqd->shift < (64*(sqd->comp+1))) sqd->shift=0;
						else sqd->shift -=(64*(sqd->comp+1));

						smp_adjust();
						continue;
					}

					continue;
				}

				if (key == 'P') {
					printf("%25s\r", "");
					printf("compress  %4u. ", sqd->comp);
					sqd->comp=(u_int)Read_Deci(sqd->comp, -1);
					smp_adjust();
					continue;
				}

				if (key == 'L') {
					printf("%25s\r", "");
					printf("lower lim  %04X ", sqd->a_min);
					sqd->a_min=(u_int)Read_Hexa(sqd->a_min, -1);
					smp_adjust();
					continue;
				}

				if (key == 'U') {
					printf("%25s\r", "");
					printf("upper lim  %04X ", sqd->a_max);
					sqd->a_max=(u_int)Read_Hexa(sqd->a_max, -1);
					smp_adjust();
					continue;
				}

				if (key == 'A') {
					if (sqd->bool &_MEA) {
						u_int	tmp=(u_int)dmabuf[ld+1];

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

					smp_adjust();
					continue;
				}

				if (rkey == 'm') {
					sqd->bool ^=_MEA;
					smp_adjust();
					continue;
				}

				if ((key == 'M') || (key == 'X')) {
					printf("%25s\r", "");
					logdata(key == 'X');
					smp_adjust();
					continue;
				}

				if (rkey == ESC) {
					gotoxy(1, QPRT_Y0+5);
					return CE_PROMPT;
				}

				if (key == ' ') rout_len=0; else single=0;
				smp_adjust();
			}

			break;
		}	// while (1)
	}
}
//
//----------------------------------------------------------------------------
//										main
//----------------------------------------------------------------------------
//
int main(int argc, char *argv[])
{
	u_int	i;

	if (argc != 2) {
		printf("%u\n", argc);
		printf("usage: %s {datafile}\n", argv[0]);
		return 0;
	}

	if ((datafile=_rtl_open(argv[1], O_RDONLY)) == -1) {
		perror("error opening data file");
		printf("name =%s", argv[1]);
		return 0;
	}

	if ((i=read(datafile, &dhdr, sizeof(DAT_HDR))) != sizeof(DAT_HDR)) {
		printf("%04X  \n", i);
		close(datafile);
		return 0;
	}

	if (dhdr.hd[0] != (0x80000000L |sizeof(DAT_HDR))) {
		printf("unknown data file\n");
		close(datafile);
		return 0;
	}

	if ((((u_int)dhdr.hd[1] >>4) &0xF) == QDCADC) {
		_XF=2;
		_NS=25.0/4;
		_MV=0.2;
		_FG=2;
		typ=QDCADC;
	} else {
		if ((((u_int)dhdr.hd[1] >>4) &0xF) == QDCVSL) {
			_XF=1;
			_NS=25.0/2;
			_MV=0.5;
			_FG=3;
			typ=QDCVSL;
		} else {
			printf("\nwrong QDC type\n");
			close(datafile);
			return 0;
		}
	}

	pos=0;
	sqd=&sq_data;
	sqd->dac0=dhdr.dac0;
	sqd->dac1=dhdr.dac1;
	sqd->a_min=0;
	sqd->a_max=0x1000;
	sqd->comp=0;
	sqd->bool=0;
	if (dhdr.cr &Q_LTRIG) sqd->bool |=_ITG;
	if (dhdr.cr &Q_CLKPOL) sqd->bool |=_EDG;
	if (dhdr.cr &Q_ADCPOL) sqd->bool |=_POL;

	sq_cha=0;
	if (dhdr.cr &QS_DIS0) sq_cha=1;

	sqd->shift=0;
	sqd->tlevel=dhdr.tlev;
	sqd->latency=dhdr.lat;
	sqd->window=dhdr.win;

	qdc_sample();
	return 0;
}
