// Borland C++ - (C) Copyright 1991, 1992 by Borland International

//	jtag.c -- JTAG control

#include <stddef.h>		// offsetof()
#include <stdlib.h>		// offsetof()
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
#include "jtag.h"
//
//-------------------------------
//			JTAG definitions
//-------------------------------
//
// instruction opcodes und register length stehen in
//		\Xilinx\xc18v00\data\xc18v0?.bsd
//		\Xilinx\spartan2\data\xc2s*.bsd
//		\Xilinx\virtex\data\xcv400.bsd
//		\Xilinx\virtex2\data\xc2v?000.bsd
//
#define C_JTG_DEVS	8

#define TDI		0x001
#define TMS		0x002
#define JT_C	0x300			// autoclock, enable

//
// folgende Instruction gelten nur fuer xc18v00
//
#define SERASE		0x0A	// Globally refines the programmed values in the array
#define FVFY3		0xE2
#define FVFY6		0xE6
#define ISPEN		0xE8
#define FPGM		0xEA	// Programs specific bit values at specified addresses
#define FADDR		0xEB	// Sets the PROM array address register
#define FERASE		0xEC	// Erases a specified program memory block
#define FDATA0		0xED	// Accesses the array word-line register ?
#define CONFIG		0xEE
#define NORMRST	0xF0	// CONLD, Exits ISP Mode ?
#define FDATA3		0xF3	// 6
#define FVFY1		0xF8	// Reads the fuse values at specified addresses
#define USERCODE	0xFD
#define IDCODE		0xFE

#define XSC_DATA_RDPT		0x0004
#define XSC_DATA_UC			0x0006
#define XSC_DATA_DONE		0x0009
#define XSC_DATA_CCB			0x000C
#define XSC_BLANK_CHECK		0x000D
#define XSC_DATA_SUCR		0x000E
#define XSC_OP_STATUS		0x00E3
#define ISC_PROGRAM			0x00EA	// FPGM
#define ISC_ADDRESS_SHIFT	0x00EB	// FADDR
#define ISC_ERASE				0x00EC	// FERASE
#define ISC_DATA_SHIFT		0x00ED	// FDATA0
#define XSC_READ				0x00EF
#define CONLD					0x00F0	// NORMST
#define XSC_DATA_BTC			0x00F2
#define XSC_CLEAR_STATUS	0x00F4
#define XSC_DATA_WRPT		0x00F7
#define XSC_UNLOCK			0xAA55

#define BYPASS		0xFFFF

typedef struct {
	u_long	id;
	char		*name;
	u_int		fam;
#define FM_18V	0x1
#define FM_F_S	0x2
#define FM_F_P	0x4

	u_int		ir_len;
	u_int		data0;		// beachte Groesse dr_tdi[]
	u_long	size0;
	u_long	erasetest;
} DEV_DESC;

static DEV_DESC	dev_desc[]={
	{0x05024093L, "XC18V01 ", FM_18V, 8, 0x100, 0x020000L, 15000000L},
	{0x05034093L, "XC18V01 ", FM_18V, 8, 0x100, 0x020000L, 15000000L},
	{0x05025093L, "XC18V02 ", FM_18V, 8, 0x200, 0x040000L, 1500000L},
	{0x05026093L, "XC18V04 ", FM_18V, 8, 0x200, 0x080000L, 1500000L},
	{0x05036093L, "XC18V04 ", FM_18V, 8, 0x200, 0x080000L, 15000000L},
	{0x05044093L, "XCF01S  ", FM_F_S, 8, 0x100, 0x020000L, 15000000L},
	{0x05046093L, "XCF04S  ", FM_F_S, 8, 0x200, 0x080000L, 15000000L},
	{0x05057093L, "XCF08P  ", FM_F_P,16, 0x020, 0x100000L, 140000000L},
	{0x00618093L, "XC2S150 ", 0, 5, 0, 0, 0},
	{0x00620093L, "XCV300  ", 0, 5, 0, 0, 0},
	{0x00628093L, "XCV400  ", 0, 5, 0, 0, 0},
	{0x01038093L, "XC2V2000", 0, 6, 0, 0, 0},
	{0x01414093L, "XC3S200 ", 0, 6, 0, 0, 0},
	{0x0141C093L, "XC3S400 ", 0, 6, 0, 0, 0},
	{0x01428093L, "XC3S1000", 0, 6, 0, 0, 0},
	{0}
};

typedef struct {
	u_int		num_devs;
	u_int		select;
	int		state;
 #define JT_UNKNOWN	-1		// unknown device (corrupted JTAG chain)
 #define JT_RESET		0		// state is RESET
 #define JT_IDLE		1		// state is RTI (RUN-TEST/IDLE)
 #define JT_DR_SHIFT	2		// state is SHIFT-DR => jtag_rd_data()

	struct {
		u_long	idcode;
		DEV_DESC	*ddesc;	// device descriptor
	} devs[C_JTG_DEVS];
} JTAG_TAB;

#pragma option -a1
typedef struct {
	u_char	bc;
	u_short	addr;
	u_char	type;
	u_char	data[0x20+1];
} MCS_RECORD;
#pragma option -a

static u_int	(*jt_getcsr)(void);	// get control/status
static void		(*jt_putcsr)(u_int);	// put control/status

static u_long	(*jt_data)(void);	// data in
static JT_CONF	*jt_conf;
static u_char	jt_mode;

static JTAG_TAB	jtag_tab;
static int			mcs_hdl=-1;
static int			mcs_ix;
static int			mcs_blen;
static FILE			*svfdat=0;
static char			svf_line[300];
static u_int		svf_ix;
static u_long		svf_value;

typedef struct {
	u_char	mcs_buf[0x1000];
	u_char	dr_tdi[0x1000];	// DR values
	u_char	dr_itdo[0x1000];
	u_char	dr_tdo[0x1000];
	u_char	dr_mask[0x1000];
} JTBUF;

static JTBUF		*jtbuf=0;

static u_int		sc_line;				// SVF file, line number
static u_int		sc_command;
static u_int		sc_length;			// number of bits for svf command
static u_long		lvdr_tdi;			// DR long value
static u_long		lvdr_tdo;
static u_long		lvdr_mask;
static u_int		lvir_tdi;			// IR long value
static u_int		lvir_tdo;
static u_int		lvir_mask;
static u_char		tdo_val;
typedef enum {
	SC_UNDEF,
	SC_COMMENT,

	SC_TRST,
	SC_ENDIR,
	SC_ENDDR,
	SC_STATE,
	SC_FREQUENCY,

	SC_HIR,		// Header Instruction Register
	SC_TIR,		// Trailer Instruction Register
	SC_HDR,		// Header Data Register
	SC_TDR,		// Trailer Data Register
	SC_SIR,		// Scan Instruction Register;		ab hier Interpretation
	SC_SDR,		// Scan Data Register
	SC_RUNTEST,	// den folgenden geht SC_RUNTEST voraus

	SC_TDI,
	SC_SMASK,
	SC_TDO,
	SC_MASK,
	SC_TCK,

	SC_BRACKET_O,
	SC_BRACKET_C,
	SC_SEMICOLON
};

static char *const cmd_txt[] ={
	"TRST", "ENDIR", "ENDDR", "STATE", "FREQUENCY",
	"HIR",  "TIR",   "HDR",   "TDR",   "SIR", "SDR", "RUNTEST",
	"TDI",  "SMASK", "TDO",   "MASK",  "TCK", 0
};

static MCS_RECORD	mcs_record;
static u_long		jt_usercode=0xFFFFFFFFL;
static u_char		pnt;
//
//--------------------------- display_errcode --------------------------------
//
#define CEN_PROMPT		-2		// display no command menu, only prompt
#define CE_MENU			1
#define CE_PROMPT			2		// display no command menu, only prompt
#define CE_FORMAT			4
#define CE_FATAL			5

static void display_errcode(int err)
{
	if (err == CE_PROMPT) return;

	printf("err# %u, ", err);
	switch (err) {

case CE_FORMAT:
		printf("MCS format error\n"); break;

default:
		printf("\n"); break;
	}
}
//
//===========================================================================
//
//										JTAG Interface
//
//===========================================================================
//
static int svf_a2bin(	// count of valid characters
								// value in svf_value
	char	hex,				// default base
	char	*str)
{
	u_int	t,i;
	char	base;
	u_int	dg;
	u_int	len;

	base=-1; t=0;
	while ((str[t] == ' ') || (str[t] == TAB)) t++;
	for (i=t;; i++) {
		dg=toupper(str[i]);
		if ((dg >= '0') && (dg <= '9')) continue;

		if (   (dg >= 'A') && (dg <= 'F')
			 && (hex || (base == 1))) { base=1; continue; }

		if ((i == 1) && (dg == 'X') && (str[0] == '0')) { base=1; continue; }

		if ((dg == '.') && (base < 0)) { base=0; len=i+1; break; }

		len=i; break;
	}

	if (base < 0) base=hex;

	svf_value=0;
	if (base) {							// Hexa
		for (i=t; i < len; i++) {
			dg=toupper(str[i]);
			if (dg == 'X') continue;

			if (svf_value >= 0x10000000l) return i;
			if (dg >= 'A') dg -=7;

			svf_value =(svf_value <<4) +(dg-'0');
		}

		return len;
	}

	for (i=t; (i < len) && (str[i] != '.'); i++) {
		dg=str[i]-'0';
		if ((svf_value > 429496729l) || ((svf_value == 429496729l) && (dg >= 6)))
			return i;

		svf_value=10*svf_value +dg;
	}

	return len;
}

//--------------------------- svf_readln ------------------------------------
//
static int svf_readln(void)
{
	u_int	i;
	int	c;

	sc_line++; svf_ix=0; i=0;
	while (1) {
		c=fgetc(svfdat);
		if (c == EOF) { svf_line[i]=0; return -1; }

		if ((c == '\r') || (c == '\n')) {
			if (i == 0) continue;

			svf_line[i]=0; return 0;
		}

		if (i >= sizeof(svf_line)-1) {
			printf("line too long\n");
			svf_line[i]=0; return -2;
		}

		svf_line[i++]=c;
	}
}
//
//--------------------------- svf_token -------------------------------------
//
static int svf_token(void)
{
	int	ret;
	int	i;
	char	*const *tkx;
	char	*tk;

	i=0;
	while (1) {
		while ((svf_line[svf_ix] == ' ') || (svf_line[svf_ix] == TAB)) svf_ix++;

		if (!svf_line[svf_ix]) {
			if ((ret=svf_readln()) == 0) continue;

			return ret;
		}

		switch (svf_line[svf_ix]) {
	case '/':
	case '!':
			svf_ix++; return SC_COMMENT;

	case '(':
			svf_ix++; return SC_BRACKET_O;

	case ')':
			svf_ix++; return SC_BRACKET_C;

	case ';':
			svf_ix++; return SC_SEMICOLON;
		}

		ret=SC_TRST; tkx=cmd_txt;
		while ((tk= *tkx++) != 0) {
			i=0;
			while (tk[i] && (tk[i] == svf_line[svf_ix+i])) i++;

			if (!tk[i]) { svf_ix +=i; return ret; }

			ret++; tk++;
		}

		return SC_UNDEF;
	}
}
//
//--------------------------- svf_byte --------------------------------------
//
static int svf_byte(void)
{
	int	ret;
	u_int	i,dg,val;

	val=0;
	for (i=0; i < 2; i++, svf_ix++) {
		if (!svf_line[svf_ix])
			if ((ret=svf_readln()) != 0) return ret;

		dg=toupper(svf_line[svf_ix]);
		if (((dg < '0') || (dg > '9')) && ((dg < 'A') || (dg > 'F'))) return -3;

		if (dg >= 'A') dg -=7;

		val =(val <<4) +(dg-'0');
	}

	return val;
}
//
//--------------------------- MCS_byte --------------------------------------
//
static int MCS_byte(void)	// -1: error
{

	if (mcs_ix >= mcs_blen) {
		if ((mcs_blen == 0) || (mcs_hdl == -1)) return -1;

      mcs_ix=0;
		if ((mcs_blen=read(mcs_hdl, jtbuf->mcs_buf, sizeof(jtbuf->mcs_buf)))
																!= sizeof(jtbuf->mcs_buf)) {
			if (mcs_blen < 0) { perror("error reading config file"); mcs_blen=0; }

			if (mcs_blen == 0) return -1;
		}
	}

	return jtbuf->mcs_buf[mcs_ix++];
}
//
//--------------------------- MCS_record ------------------------------------
//
// lese naechsten MCS Record nach
//		mcs_record
//
static int MCS_record(void)	// != 0: error
{
	int		bx,ix,dx;
	u_char	*bf;

	while ((bx=MCS_byte()) != ':') {
		if (bx < 0) return -1;

		if ((bx != '\r') && (bx != '\n')) {
			printf("illegal Format\n"); return -1;
		}
	}

	ix=0; bf=(u_char*)&mcs_record;
	do {
		bx=MCS_byte();
		if (bx < ' ') break;

		dx =bx -'0';
		if (dx >= 10) dx -=('A'-'9'-1);

		bx=MCS_byte() -'0';
		if (bx >= 10) bx -=('A'-'9'-1);

		bf[ix++]=(dx <<4) |bx;
	} while (ix < sizeof(mcs_record));

	dx=ix-1; bx=0; ix=0;
	do bx +=bf[ix++]; while (ix < dx);
	if (   (mcs_record.bc != (dx-4))
		 || (bf[dx] != (u_char)(-bx))) {
		return CE_FORMAT;
	}

	return 0;
}
//
//--------------------------- jtag_enter_reset ------------------------------
//
static void jtag_enter_reset(void)
{
	u_int		i;

	if (jtag_tab.state == JT_UNKNOWN) return;

	for (i=0; i < 8; i++) (*jt_putcsr)(JT_C|TMS);
	jtag_tab.state=JT_RESET;
}
//
//--------------------------- jtag_enter_rti ------------------------------
//
static void jtag_enter_rti(void)
{
	if (jtag_tab.state == JT_RESET) {
		(*jt_putcsr)(JT_C);
		jtag_tab.state=JT_IDLE;
	}
}
//
//--------------------------- jtag_reset ------------------------------------
//
static int jtag_reset(void)
{
	u_int		ux,uy;
	u_long	tmp;
	u_long	eid=(jt_mode) ? 0x55555555L : 0xAAAAAAAAL;	// end ID

	ux=0;
	do {
		uy=0;
		do (*jt_putcsr)(JT_C|TMS); while (++uy < 5);	// TLR state, ID DR wird selectiert
		(*jt_putcsr)(JT_C);									// RTI state
	} while (++ux < 3);						// einmal sollte auch genuegen
//
//--- RTI (Run-Test/Idle)
//
	(*jt_putcsr)(JT_C|TMS);	// Select DR
	(*jt_putcsr)(JT_C);		// Capture DR
	if (jt_mode) (*jt_putcsr)(JT_C);

	ux=C_JTG_DEVS;								// die letzte device ID wird zuerst gelesen
	while (1) {
		for (uy=0; uy < 32; uy++)
			(*jt_putcsr)((uy&1) ? JT_C : JT_C|TDI);	// shift DR

		tmp=(*jt_data)();
		if (   (tmp == eid)		// wurde vorne (TDI) hineingeschoben
			 || (tmp == 0)
			 || (tmp == 0xFFFFFFFFl)
			 || (ux == 0)) break;			// maximale Anzahl erreicht

		jtag_tab.devs[--ux].idcode=tmp;
	}

	(*jt_putcsr)(JT_C|TMS);		// Exit1 DR
	(*jt_putcsr)(JT_C|TMS);		// Update DR
	(*jt_putcsr)(JT_C);			// RTI state
	jtag_tab.num_devs=0; jtag_tab.select=0; jtag_tab.state=JT_UNKNOWN;
	if (   (tmp	!= eid)			// Ende nicht erkannt
		 || (ux == C_JTG_DEVS)) {		 	// kein device vorhanden
		uy= C_JTG_DEVS;
		while (uy > ux) printf("%08lX, ", jtag_tab.devs[--uy].idcode);
		printf("%08lX\n", tmp);
		return -1;
	}

	jtag_tab.num_devs=C_JTG_DEVS-ux;
	uy=0;
	do {												// nach vorne schieben
		jtag_tab.devs[uy].idcode=jtag_tab.devs[ux++].idcode;
		jtag_tab.devs[uy++].ddesc=0;			// wird spaeter bestimmt
	} while (ux < C_JTG_DEVS);

	jtag_tab.state=JT_IDLE;
	return 0;
}
//
//--------------------------- jtag_select -----------------------------------
//
static int jtag_select(u_int dev)
{
	if (dev >= jtag_tab.num_devs) return CE_PROMPT;

	jtag_tab.select=dev;
	return 0;
}
//
//--------------------------- jtag_runtest ----------------------------------
//
#define MS	10000L

static int jtag_runtest(u_long cnt)
{
#if 0
// 10000 => 1ms
	cnt /=(u_long)1000L;
	if (cnt <= 0xFFFF) { delay((u_int)cnt +1); return 0; }

	cnt /=1000L;
	while (kbhit()) getch();
	while (1) {
		sleep(1);
		if (!cnt || kbhit()) {
			while (kbhit()) getch();
			return 0;
		}

		cnt--;
	}
#else
// 40.000.000 => 5-7 Sec
	cnt=(cnt+4)/5L;
	while (cnt) { (*jt_putcsr)(JT_C); cnt--; }
	return 0;
#endif
}
//
//--------------------------- jtag_instruction ------------------------------
//
static u_int jtag_instruction(u_int icode)
{
	u_int		ic=icode;
	u_int		ux,uy;
	u_int		tdo;
	u_char	ms;		// TMS (bit 1)

	jtag_enter_rti();
	if (jtag_tab.state != JT_IDLE) {
		printf("JTAG not initialized\n");
		return 0;
	}

	(*jt_putcsr)(JT_C|TMS|TDI);	// DR Select
	(*jt_putcsr)(JT_C|TMS|TDI);	// IR Select
	(*jt_putcsr)(JT_C|TDI);			// IR Capture
	(*jt_putcsr)(JT_C|TDI);			// IR Shift
	ux=jtag_tab.num_devs;
	tdo=0; ms=0;
	while (ux) {
		ux--; uy=0;
		do {
			if (   (ux == 0)										// last device
				 && (uy == (jtag_tab.devs[ux].ddesc->ir_len -1))	// last bit
				) ms=TMS;											// IR Exit1

			if (ux != jtag_tab.select) {
				(*jt_putcsr)(JT_C|ms|TDI);	// BYPASS
//				printf(" %02X", ms|TDI);
			} else {
				tdo |=(((*jt_getcsr)() >>3) &1) <<uy;
//				printf("%02X ", ms|(ic &TDI));
				(*jt_putcsr)(JT_C|ms|(ic &TDI)); ic >>=1;
			}
		} while (++uy < jtag_tab.devs[ux].ddesc->ir_len);
//		printf(" => %u\n", ux);
	}

	(*jt_putcsr)(JT_C|TMS|TDI);	// IR Update
	(*jt_putcsr)(JT_C|TDI);			// Idle
	if (icode != CONFIG) return tdo;

	jtag_instruction(BYPASS);
	for (ux=0; ux < 3; ux++) {
		sleep(1);
		if (kbhit()) break;
	}

	while (kbhit()) getch();
	return 0;

}
//
//--------------------------- jtag_data -------------------------------------
//
static u_long jtag_data(u_long din, u_int len)
{
	u_int		dev,uy;
	u_long	tdo;

	errno=0;
	jtag_enter_rti();
	if (jtag_tab.state != JT_IDLE) { errno=1; return 0; }

	(*jt_putcsr)(JT_C|TMS|TDI);	// DR Scan
	(*jt_putcsr)(JT_C|TDI);			// CR Capture
	(*jt_putcsr)(JT_C|TDI);			// DR Shift
	dev=jtag_tab.num_devs; tdo=0;
	while (dev) {
		dev--;
		if (dev != jtag_tab.select) {	// TDI/TDO header and tail
			if (dev == 0) {
				(*jt_putcsr)(JT_C|TMS|TDI);	// letztes device, EXIT1-DR
				break;
			}

			(*jt_putcsr)(JT_C|TDI);
			continue;
		}

		uy=0;
		while (len) {
			len--;
			tdo |=(u_long)(((*jt_getcsr)() >>3) &1) <<uy; uy++;
			if (   (dev == 0)							// letztes device
				 && (len == 0)) {						// letztes bit
				(*jt_putcsr)(JT_C|TMS |((u_char)din &TDI));	// EXIT1-DR
				break;
			}

			(*jt_putcsr)(JT_C|((u_char)din &TDI)); din >>=1;
		}
	}

	(*jt_putcsr)(JT_C|TMS|TDI);	// Update DR
	(*jt_putcsr)(JT_C|TDI);			// state RTI
	return tdo;
}
//
//--------------------------- jtag_rd_data ----------------------------------
//
static int jtag_rd_data(
	void	*buf,
	u_int	len)		//  0: end, set state to RTI
{
	u_long	*bf;
	u_int		ux;

	jtag_enter_rti();
	if (jtag_tab.state == JT_UNKNOWN) return CE_PROMPT;

	if (len == 0) {
		if (jtag_tab.state == JT_DR_SHIFT) {
			(*jt_putcsr)(JT_C|TMS|TDI);
			(*jt_putcsr)(JT_C|TMS|TDI);
			(*jt_putcsr)(JT_C|TDI);
			jtag_tab.state=JT_IDLE;		// state RTI
		}

		return 0;
	}

	if (jtag_tab.state == JT_IDLE) {
		(*jt_putcsr)(JT_C|TMS|TDI);
		(*jt_putcsr)(JT_C|TDI);			// CAPTURE-DR
		if (jt_mode) (*jt_putcsr)(JT_C|TDI);	// Shift DR

		ux=jtag_tab.num_devs;
		while (--ux != jtag_tab.select) (*jt_putcsr)(JT_C|TDI);	// header bits
		jtag_tab.state=JT_DR_SHIFT;
	}

	bf=(u_long*)buf;
	while (len > 4) {
		ux=0;
		do (*jt_putcsr)(JT_C|TDI); while (++ux < 32);		// erste TCK ist noch in CAPTURE-DR

		*bf++ =(*jt_data)(); len -=4;
	}

	ux=0;
	do (*jt_putcsr)(JT_C|TDI); while (++ux < 32);

	*bf++ =(*jt_data)();
	return 0;
}
//
//--------------------------- jtag_wr_data ----------------------------------
//
static int jtag_wr_data(
	void	*buf,
	u_int	len,		// Anzahl Byte
	int	state)	// -1: end
{
	u_long	*bf;
	u_long	din;
	u_int		dev;
	u_int		ux;
	u_char	ms;

	jtag_enter_rti();
	if (jtag_tab.state != JT_IDLE) return CE_PROMPT;

	bf=(u_long*)buf; len >>=2;
	if (len == 0) return 0;

	(*jt_putcsr)(JT_C|TMS|TDI);
	(*jt_putcsr)(JT_C|TDI); (*jt_putcsr)(JT_C|TDI); dev=jtag_tab.num_devs; ms=0;
	while (dev) {
		dev--;
		if (dev != jtag_tab.select) {
			(*jt_putcsr)((dev) ? (JT_C|TDI) : (JT_C|TMS|TDI));
		} else {
			while (len) {
				len--;
				din=*bf++; ux=32;
				while (ux) {
					ux--;
					if ((dev == 0) && (len == 0) && (ux == 0)) ms=TMS;

					(*jt_putcsr)(JT_C|ms|((u_char)din &TDI)); din >>=1;
				}
			}
		}
	}

	if (state < 0) {
		(*jt_putcsr)(JT_C|TMS|TDI); (*jt_putcsr)(JT_C|TDI); jtag_tab.state=JT_IDLE;
	}

	return 0;
}
//
//--------------------------- jtag_data_exchange ----------------------------
//
// TDI buffer: dr_tdi[]
// TDO buffer: dr_itdo[]
// # of bits:  sc_length
//
static int jtag_data_exchange(void)	// 0: ok
{
	u_long	*bfi,*bfo;
	u_long	din;
	u_int		dev;
	u_int		len;
	u_int		ux;

	jtag_enter_rti();
	if (jtag_tab.state != JT_IDLE) return CE_PROMPT;

	if (!sc_length) return 0;

	bfi=(u_long*)jtbuf->dr_tdi;
	bfo=(u_long*)jtbuf->dr_itdo;
	len =sc_length >>5;

	(*jt_putcsr)(JT_C|TMS|TDI);
	(*jt_putcsr)(JT_C|TDI);
	(*jt_putcsr)(JT_C|TDI);		// SHIFT-DR
	dev=jtag_tab.num_devs;
	while (dev) {
		dev--;
		if (dev != jtag_tab.select) {		// TDI/TDO header and tail
			if (dev == 0) {
				(*jt_putcsr)(JT_C|TMS|TDI);		// letztes device, EXIT1-DR
				break;
			}

			(*jt_putcsr)(JT_C|TDI);
			continue;
		}
//
// --- selected device
//
		while (len) {
			len--;
			din=*bfi++; ux=32;
			while (ux) {
				ux--;
				if (ux == 0) {
					*bfo++ =(*jt_data)();
					if (   (len == 0) && (dev == 0)
						 && !(sc_length &0x1F)) {			// absolut letztes Bit
						(*jt_putcsr)(JT_C|TMS |((u_char)din &TDI));	// EXIT1-DR
						break;
					}
				}

				(*jt_putcsr)(JT_C|((u_char)din &TDI)); din >>=1;
			}

		}

		if ((ux=sc_length &0x1F) != 0) {
			din=*bfi++;
			while (ux) {
				ux--;
				if (ux == 0) {
					*bfo++ =(*jt_data)() >>(32-(sc_length &0x1F));
					if (dev == 0) {
						(*jt_putcsr)(JT_C|TMS|((u_char)din &TDI));	// EXIT1-DR
						break;
					}
				}

				(*jt_putcsr)(JT_C|((u_char)din &TDI)); din >>=1;
			}
		}
	}

	(*jt_putcsr)(JT_C|TMS|TDI); (*jt_putcsr)(JT_C|TDI);	// state RTI
	return 0;
}
//
//--------------------------- read_idcodes ----------------------------------
//
static int read_idcodes(void)
{
	u_int		ux,j;
	u_long	tmp,tmp0;;

	printf("\nread JTAG chain\n\n");
	if (jtag_reset()) {
		printf("no JTAG chain found\n");
		return CE_PROMPT;
	}

	printf("JTAG chain\n");
	printf("-----------\n"); ux=0;
	for (ux=0; ux < jtag_tab.num_devs; ux++) {
		printf(" %u: %08lX ", ux, tmp=jtag_tab.devs[ux].idcode);
		j=0;
		while ((tmp0=dev_desc[j].id) != 0) {
			if ((tmp &0x0FFFFFFFl) == tmp0) {
				printf("%s Ver: %u\n", dev_desc[j].name, (u_char)(tmp >>28));
				jtag_tab.devs[ux].ddesc=dev_desc +j;
				break;
			}

			j++;
		}

		if (tmp0) continue;

		jtag_tab.state=JT_UNKNOWN;
		printf("unknown device\n");
	}

	Writeln();
	return CE_PROMPT;
}
//
//--------------------------- jtag_usercode ---------------------------------
//
static int jtag_usercode(void)
{
	int		ret;
	DEV_DESC	*ddesc;
	u_long	ltmp;

	read_idcodes();
	if (jtag_tab.state != JT_IDLE) return CE_PROMPT;

	printf("device  %4u ", jt_conf->jtag_dev);
	jt_conf->jtag_dev=(u_int)Read_Deci(jt_conf->jtag_dev, -1);
	if (errno) return 0;

	if ((ret=jtag_select(jt_conf->jtag_dev)) != 0) return ret;

	ddesc=jtag_tab.devs[jtag_tab.select].ddesc;
	if (!ddesc) return CE_FATAL;

	if (ddesc->fam &(FM_F_S|FM_18V)) {
		jtag_instruction(USERCODE);
	} else {
		if (ddesc->fam &FM_F_P) {
			jtag_instruction(XSC_DATA_UC);
		} else {
			printf("device not supported\n");
			return CE_FATAL;
		}
	}

	ltmp=jtag_data(0, 32);
	printf("User code =0x%08lX = %ld\n", ltmp, ltmp);
	return CE_PROMPT;
}
//
//--------------------------- jtag_instr ------------------------------------
//
static int jtag_instr(void)
{
	int	ret;

	printf("device      %4u ", jt_conf->jtag_dev);
	jt_conf->jtag_dev=(u_int)Read_Deci(jt_conf->jtag_dev, -1);
	if (errno) return 0;

	printf("instruction 0x%04X ", jt_conf->jtag_instr);
	jt_conf->jtag_instr=(u_int)Read_Hexa(jt_conf->jtag_instr, -1);
	if (errno) return 0;

	if ((ret=jtag_select(jt_conf->jtag_dev)) != 0) return ret;

	if (jtag_tab.state == JT_UNKNOWN) {
		printf("JTAG not initialized\n");
		return CE_PROMPT;
	}

	printf("instructin reg =0x%04X\n", jtag_instruction(jt_conf->jtag_instr));
	return CE_PROMPT;
}
//
//--------------------------- jtag_exchange ---------------------------------
//
static int jtag_exchange(void)
{
	u_long	dout;

	printf("data length    %8u ", jt_conf->jtag_dlen);
	jt_conf->jtag_dlen=(u_char)Read_Deci(jt_conf->jtag_dlen, 32);
	if (errno) return 0;

	printf("JTAG data in 0x%08lX ", jt_conf->jtag_data);
	jt_conf->jtag_data=Read_Hexa(jt_conf->jtag_data, -1);
	if (errno) return 0;

	dout=jtag_data(jt_conf->jtag_data, jt_conf->jtag_dlen);
	if (errno)
		printf("JTAG not initialized\n");
	else
		printf("data reg =0x%08lX\n", dout);

	return CE_PROMPT;
}
//
//--------------------------- jtag_xc18_prom --------------------------------
//
static int jtag_xc18_prom(
	DEV_DESC	*ddesc)
{
	int		ret;
	u_long	lx;
	u_int		ux,uy;
	u_int		tmp,cnt;

	printf("loading with 'ispen' instruction (6/0x34)\n");
	jtag_instruction(ISPEN);
	jtag_data(0x34, 6);

	printf("loading with 'faddr' instruction (16/0x0001)\n");
	jtag_instruction(FADDR);
	jtag_data(1, 16);
	jtag_runtest(2);

	printf("loading with 'ferase' instruction\n");
	jtag_instruction(FERASE);
	jtag_runtest(ddesc->erasetest);

	printf("loading with 'conld/normst' instruction\n");
	jtag_instruction(NORMRST);
	jtag_runtest(110000L);

	printf("loading with 'ispen' instruction (6/0x34)\n");
	jtag_instruction(ISPEN);
	jtag_data(0x34, 6);

	printf("fdata0 len: %04X\n", ddesc->data0); tmp=0;
	lx=0; cnt=0;
	do {
		ux=0;
		do {
			do {
				if ((ret=MCS_record()) != 0) {
					Writeln();
					if (ret == -1) ret=CE_FORMAT;

					return ret;
				}

			} while ((mcs_record.type != 0) && (mcs_record.type != 1));

			if (mcs_record.type == 1) {
				Writeln();
				printf("END record\n");
				break;
			}

			uy=0;
			do jtbuf->dr_tdi[ux++] =mcs_record.data[uy++];
			while (uy < mcs_record.bc);

		} while (ux < ddesc->data0);

		lx +=ux;
		while (ux < ddesc->data0) jtbuf->dr_tdi[ux++] =0xFF;

		jtag_instruction(FDATA0);
		if ((ret=jtag_wr_data(jtbuf->dr_tdi, ddesc->data0, -1)) != 0) return ret;

		jtag_runtest(2);
		if (lx <= ddesc->data0) {	// 1. mal
			jtag_instruction(FADDR);
			jtag_data(tmp, 16); tmp+=0x20;	// (ddesc->data0 >>4)
			jtag_runtest(2);
		}

		jtag_instruction(FPGM);
		jtag_runtest(140000L);	// svf*10
		if (!(lx &0xFFF)) { printf("."); pnt=1; }

		cnt++;
	} while (mcs_record.type == 0);

	printf("last FADDR: %04X\n", tmp-0x20);
	printf("loading with 'faddr' instruction (16/0x0001)\n");
	jtag_instruction(FADDR);
	jtag_data(0x01, 16);
	jtag_runtest(2);

	printf("loading with 'serase' instruction\n");
	jtag_instruction(SERASE);
	jtag_runtest(37000L);

	printf("loading with 'conld/normst' instruction\n");
	jtag_instruction(CONLD);
	jtag_runtest(110000L);

	if (ddesc->fam &FM_18V) {
		if (jt_usercode != 0xFFFFFFFFL) {
			printf("write user code\n");
			jtag_instruction(ISPEN);
			jtag_data(0x34, 6);

			jtag_instruction(FADDR);
			jtag_data(0x4000, 16);
			jtag_runtest(2);

			jtag_instruction(USERCODE);
			jtag_data(jt_usercode, 32);

			jtag_instruction(FPGM);
			jtag_runtest(14000);

			jtag_instruction(FVFY6);
			jtag_runtest(50);
			if (jtag_data(0, 32) != jt_usercode) {
				printf("error writing user code\n");
			}

			jtag_instruction(CONLD);
			jtag_runtest(110000L);

			jtag_instruction(ISPEN);
			jtag_data(0x34, 6);

			jtag_instruction(ISPEN);
			jtag_data(0x34, 6);

			jtag_instruction(FADDR);
			jtag_data(0x4000, 16);
			jtag_runtest(2);

			jtag_instruction(FDATA3	);
			jtag_data(0x3F, 6);

			printf("loading with 'fpgm' instruction\n");
			jtag_instruction(FPGM);
			jtag_runtest(14000);

			jtag_instruction(FVFY3);
			jtag_runtest(50);
			jtag_data(0x3F, 6);

			jtag_instruction(CONLD);
			jtag_runtest(110000L);

			jtag_instruction(ISPEN);
			jtag_data(0x34, 6);

			jtag_instruction(CONLD);
			jtag_runtest(110000L);
		}

		printf("loading with 'bypass' instruction\n");
		jtag_instruction(BYPASS);
	}

	printf("0x%06lX byte written\n", lx);
	return 0;
}
//
//--------------------------- jtag_xcfs_status ------------------------------
//
static int jtag_xcfs_status(char *msg)
{
	u_int	tmo;
	u_int	wout;

	jtag_runtest(1*MS);
	jtag_instruction(XSC_OP_STATUS);
	tmo=0;
	while (1) {
		jtag_runtest(1*MS);
		wout=(u_int)jtag_data(0, 8);
		if (wout &4) return 0;

		if (++tmo >= 5000) {
			if (pnt) { pnt=0; Writeln(); }
			printf("timeout %s, status=%02X\n", msg, wout);
			jtag_instruction(CONLD);
			jtag_runtest(110*MS);

			jtag_instruction(ISPEN);
			jtag_data(0x03, 6);
			jtag_runtest(12*MS);

			jtag_instruction(CONLD);
			jtag_runtest(110*MS);
			jtag_instruction(BYPASS);
			return CE_PROMPT;
		}
	}
}
//
//--------------------------- jtag_xcfs_prom --------------------------------
//
static int jtag_xcfs_prom(
	DEV_DESC	*ddesc)
{
	int		ret;
	u_long	dout;
	u_long	lx;
	u_int		ux,uy;
	u_int		blk;

	jtag_instruction(IDCODE);
	dout=jtag_data(-1, 32);
	if ((dout &0x0FFFFFFFL) != ddesc->id) {
		printf("wrong IDCODE: %08lX (exp: %08lX)\n", dout, ddesc->id);
		return CE_PROMPT;
	}

	printf("Check for Read/Write Protect\n");
	ret=jtag_instruction(BYPASS);
	if ((ret &0x7) != 1) {
		printf("protected %02X\n", ret);
		return CE_PROMPT;
	}

	jtag_instruction(ISPEN);
	jtag_data(0x37, 6);

	jtag_instruction(FADDR);
	jtag_data(1, 16);

	printf("loading with 'ferase' instruction\n");
	jtag_instruction(FERASE);
	jtag_runtest(5000*MS);

	if (jtag_xcfs_status("erasing")) return CE_PROMPT;

	jtag_instruction(NORMRST);
	jtag_runtest(130*MS);

	jtag_instruction(ISPEN);
	jtag_data(0x37, 6);

	printf("fdata0 len: %04X\n", ddesc->data0); blk=0;
	lx=0;
	do {
		ux=0;
		do {
			do {
				if ((ret=MCS_record()) != 0) {
					Writeln();
					if (ret == -1) ret=CE_FORMAT;

					return ret;
				}

			} while ((mcs_record.type != 0) && (mcs_record.type != 1));

			if (mcs_record.type == 1) {
				Writeln();
				printf("END record\n");
				break;
			}

			uy=0;
			do jtbuf->dr_tdi[ux++] =mcs_record.data[uy++];
			while (uy < mcs_record.bc);

		} while (ux < ddesc->data0);

		lx +=ux;
		while (ux < ddesc->data0) jtbuf->dr_tdi[ux++] =0xFF;

		jtag_instruction(FDATA0);
		if ((ret=jtag_wr_data(jtbuf->dr_tdi, ddesc->data0, -1)) != 0) return ret;

		jtag_instruction(FADDR);
		jtag_data(blk, 16); blk+=0x20;	// (ddesc->data0 >>4)

		jtag_instruction(FPGM);
		jtag_runtest(3*MS);

		jtag_instruction(BYPASS);
		if (jtag_xcfs_status("writing data")) return CE_PROMPT;

		if (!(lx &0xFFF)) { printf("."); pnt=1; }

	} while (mcs_record.type == 0);

	if (pnt) { pnt=0; Writeln(); }
	printf("last FADDR: %04X\n", blk-0x20);

	jtag_runtest(3*MS);
	jtag_instruction(FADDR);
	jtag_data(0x01, 16);

	jtag_instruction(SERASE);
	jtag_runtest(37*MS);

	jtag_instruction(CONLD);
	jtag_runtest(110*MS);

	jtag_instruction(ISPEN);
	jtag_data(0x03, 6);
	jtag_runtest(10*MS);

	jtag_instruction(FADDR);
	jtag_data(0x8000, 16);

	blk=0;
	while (jt_usercode != 0xFFFFFFFFL) {
		jtag_instruction(USERCODE);
		jtag_data(jt_usercode, 32);

		jtag_instruction(FPGM);
		jtag_runtest(3*MS);

		jtag_instruction(BYPASS);
		if (jtag_xcfs_status("writing user code")) return CE_PROMPT;

		jtag_instruction(FVFY6);
		jtag_runtest(50*MS);
		dout=jtag_data(0, 32);

		jtag_instruction(CONLD);
		jtag_runtest(110*MS);

		jtag_instruction(ISPEN);
		jtag_data(0x03, 6);
		jtag_runtest(7*MS);

		jtag_instruction(ISPEN);
		jtag_data(0x03, 6);
		jtag_runtest(10*MS);

		jtag_instruction(FADDR);
		jtag_data(0x8000, 16);
		jtag_runtest(1*MS);

		if (dout == jt_usercode) {
			printf("user code ok\n");
			break;
		}

		printf("error writing user code is: %ld\n", dout);
		if (++blk > 3) break;
	}

	jtag_instruction(FDATA3);
	jtag_data(0x07, 3);

	jtag_instruction(FPGM);
	jtag_runtest(3*MS);

	jtag_instruction(BYPASS);
	if (jtag_xcfs_status("ending programming")) return CE_PROMPT;

	jtag_instruction(FVFY3);
	jtag_data(0x07, 3);

	jtag_instruction(CONLD);
	jtag_runtest(110*MS);

	jtag_instruction(ISPEN);
	jtag_data(0x03, 6);
	jtag_runtest(12*MS);

	jtag_instruction(CONLD);
	jtag_runtest(110*MS);

	jtag_instruction(BYPASS);
	printf("0x%06lX byte written\n", lx);
	return 0;
}
//
//--------------------------- jtag_prom -------------------------------------
//
static int jtag_prom(
	int	mode)	// 0: program
					// 1: verify
					// 2: read and save
{
	int		ret;
	u_long	dout;
	u_int		ux,uy,ex;
	u_long	lx;
	char		*fname;
	u_int		date, time;
	DEV_DESC	*ddesc;
	u_int		tmp;

   pnt=0;
	read_idcodes();
	if (jtag_tab.state != JT_IDLE) return CE_PROMPT;

	printf("device    %3u ", jt_conf->jtag_dev);
	jt_conf->jtag_dev=(u_int)Read_Deci(jt_conf->jtag_dev, -1);
	if (errno) return 0;

	if ((ret=jtag_select(jt_conf->jtag_dev)) != 0) return ret;

	ddesc=jtag_tab.devs[jtag_tab.select].ddesc;
	if (!ddesc) return CE_FATAL;

	if (mode == 2) {
		fname =(jt_conf->jtag_dev < C_JT_NM) ? jt_conf->jtag_file[jt_conf->jtag_dev] :
														jt_conf->jtag_file[C_JT_NM-1];
		printf("bin file name ");
		strcpy(fname,
				 Read_String(fname, sizeof(jt_conf->jtag_file[0])));
		if (errno) return 0;

		if ((mcs_hdl=_rtl_creat(fname, 0)) == -1) {
			perror("error creating file");
			return CEN_PROMPT;
		}
	} else {
		fname =(jt_conf->jtag_dev < C_JT_NM) ? jt_conf->jtag_file[jt_conf->jtag_dev] :
														jt_conf->jtag_file[C_JT_NM-1];
		printf("MCS file name ");
		strcpy(fname,
				 Read_String(fname, sizeof(jt_conf->jtag_file[0])));
		if (errno) return 0;

		if ((mcs_hdl=_rtl_open(fname, O_RDONLY)) == -1) {
			perror("error opening MCS file");
			return CE_PROMPT;
		}

		_dos_getftime(mcs_hdl, &date, &time);
		printf("time stamp: %02u.%02u.%02u  %2u:%02u:%02u\n",
				 date&0x1F, (date>>5)&0x0F, (date>>9)-20,
				 (time>>11)&0x1F, (time>>5)&0x3F, (time<<1)&0x3F);
		if ((mode == 0) && (ddesc->fam &(FM_F_P|FM_F_S|FM_18V))) {
			if (ddesc->fam &(FM_F_S|FM_18V)) {
				jtag_instruction(USERCODE);
			} else {
				if (ddesc->fam &FM_F_P) {
					jtag_instruction(XSC_DATA_UC);
				}
			}

			dout=jtag_data(0, 32);
			if (dout != 0xFFFFFFFFL) jt_usercode=dout;

			printf("user code %5ld ", jt_usercode);
			jt_usercode=Read_Deci(jt_usercode, -1);
			if (errno) return 0;
		}

		mcs_ix  =1;
		mcs_blen=1;
		if ((ret=MCS_record()) != 0) return ret;
		if ((mcs_record.type != 2) && (mcs_record.type != 4)) return CE_FORMAT;
	}

	if (jtag_tab.state == JT_UNKNOWN) {
		printf("JTAG not initialized\n");
		return CE_PROMPT;
	}

	jtag_instruction(IDCODE);
//
//--- ueberpruefe ID Code, siehe xc18v0?.bsd File
//
	dout=jtag_data(-1, 32);
	if ((dout &0x0FFFFFFFL) != ddesc->id) {
		printf("wrong IDCODE: %08lX (exp: %08lX)\n", dout, ddesc->id);
		return CE_PROMPT;
	}

	if (!ddesc->data0) {
		printf("no FLASH device\n");
		return CE_PROMPT;
	}

	if (mode == 2) {
//
//--- ISP PROM auslesen und binaer speichern
//
		printf("loading with 'ispen'\n");
		jtag_instruction(ISPEN);
		jtag_data(0x34, 6);
		ret=jtag_instruction(FADDR);
		if (ret != 0x11) {
			printf("FADDR 0x%02X, exp 0x11\n", ret);
			return CE_PROMPT;
		}

		jtag_data(0, 16);
		jtag_instruction(FVFY1);
		jtag_runtest(100);

		ex=0; lx=0;
		do {
			printf(".");
			if ((ret=jtag_rd_data(jtbuf->dr_tdi, sizeof(jtbuf->dr_tdi))) != 0) {
				Writeln(); return ret;
			}

			if (write(mcs_hdl, jtbuf->dr_tdi, sizeof(jtbuf->dr_tdi))
																	!= sizeof(jtbuf->dr_tdi)) {
				Writeln();
				perror("error writing file");
				return CEN_PROMPT;
			}

			lx +=sizeof(jtbuf->dr_tdi);
		} while (lx < ddesc->size0);

		Writeln();
		return CE_PROMPT;

	}

	if (mode == 0) {
//
//--- ISP PROM programmieren
//
		if (ddesc->fam &FM_F_S) {
			if ((ret=jtag_xcfs_prom(ddesc)) != 0) return ret;
		} else {
			if (ddesc->fam &FM_18V) {
				if ((ret=jtag_xc18_prom(ddesc)) != 0) return ret;
			} else {
				if (!(ddesc->fam &FM_F_P)) return CE_FATAL;

				// XCFP family
				printf("check ID and check for Read/Write protect\n");
				jtag_enter_reset();
	//			printf("loading with 'ISPEN/03' instruction\n");
				jtag_instruction(ISPEN);
				jtag_data(0x03, 8);

				jtag_instruction(IDCODE);
				dout=jtag_data(-1, 32);
				if ((dout &0x0FFFFFFFL) != ddesc->id) {
					printf("wrong IDCODE: %08lX (exp: %08lX)\n", dout, ddesc->id);
					return CE_PROMPT;
				}

				jtag_instruction(XSC_DATA_RDPT);
				tmp=(u_int)jtag_data(0, 16);
				if (tmp != 0xFFFF) {
					printf("read protect %04X\n", tmp);
	//				return CE_PROMPT;
				}

				jtag_instruction(XSC_DATA_WRPT);
				tmp=(u_int)jtag_data(0, 16);
				if (tmp != 0xFFFF) {
					printf("write protect %04X\n", tmp);
	//				return CE_PROMPT;
				}

	//--- erase

				printf("start erasing\n");
				jtag_enter_reset();
	//			printf("loading with 'ISPEN/D0' instruction\n");
				jtag_instruction(ISPEN);
				jtag_data(0xD0, 8);

	//			printf("loading device with XSC_UNLOCK instruction\n");
				jtag_instruction(XSC_UNLOCK);
				jtag_data(0x00003F, 24);

	//			printf("loading device with ISC_ERASE instruction\n");
				jtag_instruction(ISC_ERASE);
				jtag_data(0x00003F, 24);
	//			jtag_runtest(ddesc->erasetest);

	//			printf("loading device with XSC_OP_STATUS instruction\n");
				jtag_instruction(XSC_OP_STATUS);

				tmp=(u_int)jtag_data(0, 8);
	//			printf("erasing %02X\n", tmp);
				while (1) {
					jtag_runtest(1000);
					tmp=(u_int)jtag_data(0, 8);
					if (tmp &4) break;

					if (kbhit()) {
						jtag_instruction(CONLD);
						return CE_PROMPT;
					}
				}

	//			printf("erased  %02X\n", tmp);
				jtag_instruction(CONLD);
				jtag_runtest(50);

	//---

				jtag_enter_reset();
	#if 0
				jtag_instruction(ISPEN);
				jtag_data(0xD0, 8);
	#endif
	//			printf("loading with 'ISPEN/03' instruction\n");
				jtag_instruction(ISPEN);
				jtag_data(0x03, 8);

	//			printf("loading device with XSC_DATA_BTC/FFFFFFE0 instruction\n");
				jtag_instruction(XSC_DATA_BTC);
				jtag_data(0xFFFFFFE0L, 32);

	//			printf("loading device with ISC_PROGRAM instruction\n");
				jtag_instruction(ISC_PROGRAM);
				jtag_runtest(120);

				printf("start prgramming\n");
	//			printf("loading with 'ISPEN/D0' instruction\n");
				jtag_enter_reset();
				jtag_instruction(ISPEN);
				jtag_data(0xD0, 8);

				printf("fdata0 len: %04X\n", ddesc->data0);
				lx=0;
				do {
					ux=0;
					do {
						do {
							if ((ret=MCS_record()) != 0) {
								Writeln();
								if (ret == -1) ret=CE_FORMAT;

								return ret;
							}

						} while ((mcs_record.type != 0) && (mcs_record.type != 1));

						if (mcs_record.type == 1) {
							Writeln();
							printf("END record\n");
							break;
						}

						uy=0;
						do jtbuf->dr_tdi[ux++] =mcs_record.data[uy++];
						while (uy < mcs_record.bc);

					} while (ux < ddesc->data0);

					lx +=ux;
					while (ux < ddesc->data0) jtbuf->dr_tdi[ux++] =0xFF;

					jtag_instruction(ISC_DATA_SHIFT);

					if ((ret=jtag_wr_data(jtbuf->dr_tdi, ddesc->data0, -1)) != 0) return ret;

					if (lx <= ddesc->data0) {	// 1. mal
	//					printf("loading device with ISC_ADDRESS_SHIFT instruction\n");
						ret=jtag_instruction(ISC_ADDRESS_SHIFT);
						jtag_data(0x000000, 24);
	//					printf("ISC_ADDRESS_SHIFT =%04X\n", ret);
					}

					jtag_instruction(ISC_PROGRAM);
					jtag_runtest(1000);

					if (!(lx &0xFFF)) { printf("."); pnt=1; }

				} while (mcs_record.type == 0);

				jtag_enter_reset();
	//			printf("loading with 'ISPEN/03' instruction\n");
				jtag_instruction(ISPEN);
				jtag_data(0x03, 8);

	//			printf("loading device with XSC_DATA_SUCR instruction\n");
				jtag_instruction(XSC_DATA_SUCR);
				jtag_data(0xFFFC, 16);

				jtag_instruction(ISC_PROGRAM);
				jtag_runtest(60);

	//			printf("loading with 'ISPEN/03' instruction\n");
				jtag_instruction(ISPEN);
				jtag_data(0x03, 8);
	#if 0
				printf("loading with 'ISPEN/03' instruction\n");
				jtag_instruction(ISPEN);
				jtag_data(0x03, 8);
	#endif
				if (jt_usercode != 0xFFFFFFFFL) {
					printf("write user code\n");
					jtag_instruction(XSC_DATA_UC);
					jtag_data(jt_usercode, 32);

					jtag_instruction(ISC_PROGRAM);
					jtag_runtest(120);

					jtag_instruction(XSC_DATA_UC);
					if (jtag_data(0, 32) != jt_usercode) {
						printf("error writing user code\n");
					}

					jtag_instruction(ISPEN);
					jtag_data(0x03, 8);
				}

	//			printf("loading device with XSC_DATA_CCB instruction\n");
				jtag_instruction(XSC_DATA_CCB);
				jtag_data(0xFFFF, 16);

				jtag_instruction(ISC_PROGRAM);
				jtag_runtest(60);

				jtag_instruction(CONLD);
				jtag_runtest(50);

				jtag_enter_reset();
				jtag_instruction(ISPEN);
				jtag_data(0x03, 8);

	//			printf("loading device with XSC_DATA_DONE instruction\n");
				jtag_instruction(XSC_DATA_DONE);
				jtag_data(0xCE, 8);

				jtag_instruction(ISC_PROGRAM);
				jtag_runtest(60);

				jtag_enter_reset();
				jtag_instruction(BYPASS);

				printf("0x%06lX byte written\n", lx);
			}
		}

		mcs_ix  =1;
		mcs_blen=1;
		lseek(mcs_hdl, 0, SEEK_SET);
		Writeln(); pnt=0;
		printf("verify\n");
	}
//
//--- ISP PROM ruecklesen und vergleichen
//
	if (ddesc->fam &(FM_F_S|FM_18V)) {
		jtag_instruction(ISPEN);
		jtag_data(0x34, 6);
		ret=jtag_instruction(FADDR);
		if (ret != 0x11) {
			printf("FADDR 0x%02X, exp 0x11\n", ret);
			return CE_PROMPT;
		}

		jtag_data(0, 16);
		jtag_instruction(FVFY1);
		jtag_runtest(1000);		// 51 in svf
	} else {
		if (!(ddesc->fam &FM_F_P)) return CE_FATAL;

		jtag_instruction(ISPEN);
		jtag_data(0x03, 8);
#if 0
		jtag_instruction(XSC_DATA_RDPT);
		tmp=(u_int)jtag_data(0, 16);
		if (tmp != 0xFFFF) {
			printf("read protect %04X\n", tmp);
			return CE_PROMPT;
		}

		jtag_instruction(XSC_DATA_WRPT);
		tmp=(u_int)jtag_data(0, 16);
		if (tmp != 0xFFFF) {
			printf("write protect %04X\n", tmp);
			return CE_PROMPT;
		}

//		jtag_instruction(0xFFFF);
//		jtag_instruction(ISPEN);
//		jtag_data(0x03, 8);
		jtag_instruction(ISPEN);
		jtag_data(0x03, 8);
#endif

		jtag_instruction(ISC_ADDRESS_SHIFT);
		jtag_data(0x00, 24);

		jtag_instruction(XSC_READ);
		jtag_runtest(15);
	}

	ex=0; lx=0;
	do {
		if ((ret=jtag_rd_data(jtbuf->dr_tdi, sizeof(jtbuf->dr_tdi))) != 0)
			return ret;

		printf("."); pnt=1;
		ux=0;
		do {
			do {
				if ((ret=MCS_record()) != 0) {
					Writeln();
					if (ret == -1) ret=CE_FORMAT;

					break;
				}

				if (mcs_record.type == 1) {
					if (pnt) { Writeln(); pnt=0; }
					printf("0x%06lX byte verified\n", lx);
					ret=CE_PROMPT;
					break;
				}

			} while (mcs_record.type != 0);

			if (ret) break;

			uy=0;
			do {
				if (jtbuf->dr_tdi[ux] != mcs_record.data[uy]) {
					if (pnt) { Writeln(); pnt=0; }
					printf("%06lX: error, exp %02X, is %02X\n",
							 lx, mcs_record.data[uy], jtbuf->dr_tdi[ux]);
					if (++ex >= 20) {
						ret=CE_PROMPT;
						break;
					}
				}

				ux++; uy++; lx++;
			} while (uy < mcs_record.bc);

		} while (!ret && (ux < sizeof(jtbuf->dr_tdi)));

	} while (!ret);
#if 0
	if (ddesc->fam FM_F_P) {		// XCFP family
		printf("check device status\n");
		jtag_instruction(ISPEN);
		tmp=(u_int)jtag_data(0x00, 8);
		if ((tmp &0x7F) != 0x36) printf("failed, ret=%02X\n", tmp);

		jtag_instruction(CONLD);
		jtag_runtest(50);

		printf("loading with 'bypass' instruction\n");
		jtag_instruction(BYPASS);
		jtag_data(0x0, 1);
	}
#endif
	return ret;
}
//
//--------------------------- jtag_svf --------------------------------------
//
static int jtag_svf(void)
{
	int		ret;
	u_long	dx;
	int		arg;
	u_int		i;
	u_int		ix,lx;
	char		*fname;
	int		hd;
	u_int		date, time;
	u_char	*bf;
	u_int		pt,ex;
	u_long	tlen;
	u_char	irlen_err=0;

	read_idcodes();
	if (jtag_tab.state != JT_IDLE) return CE_PROMPT;

	printf("device    %3u ", jt_conf->jtag_dev);
	jt_conf->jtag_dev=(u_int)Read_Deci(jt_conf->jtag_dev, -1);
	if (errno) return 0;

	if ((ret=jtag_select(jt_conf->jtag_dev)) != 0) return ret;

	fname =(jt_conf->jtag_dev < C_JT_NM) ? jt_conf->jtag_svf_file[jt_conf->jtag_dev] :
													jt_conf->jtag_svf_file[C_JT_NM-1];
	printf("SVF file name ");
	strcpy(fname,
			 Read_String(fname, sizeof(jt_conf->jtag_svf_file[0])));
	if (errno) return 0;

	if ((hd=_rtl_open(fname, O_RDONLY)) == -1) {
		perror("error opening MCS file");
		return CE_PROMPT;
	}

	_dos_getftime(hd, &date, &time);
	close(hd);
	printf("time stamp: %02u.%02u.%02u  %2u:%02u:%02u\n",
			 date&0x1F, (date>>5)&0x0F, (date>>9)-20,
			 (time>>11)&0x1F, (time>>5)&0x3F, (time<<1)&0x3F);
	if ((svfdat=fopen(fname, "r")) == 0) {
		perror("error opening file");
		return CE_PROMPT;
	}

	i=0; sc_line=0;
	pt=0; ex=0;
	tlen=0;
	while (1) {
		svf_line[svf_ix=0]=0;
		if ((ret=svf_token()) <= SC_UNDEF) {
			if (ret == SC_UNDEF) { i=0; break; }

			return (ret == -1) ? CE_PROMPT : CEN_PROMPT;
		}

		if (ret > SC_RUNTEST) {
			if (pt) { Writeln(); pt=0; }
			printf("illegal SVF command %d in line %u\n", ret, sc_line);
			svf_line[80]=0;
			printf("%s\n", svf_line);
			return CEN_PROMPT;
		}

		if (ret < SC_SIR) {
			if (   (ret == SC_FREQUENCY)
				 || (ret == SC_STATE)
//				 || (ret == SC_COMMENT)
				) printf("%s\n", svf_line);

			continue;
		}

		sc_command=ret;
		svf_ix +=svf_a2bin(0, svf_line+svf_ix);

		if ((svf_line[svf_ix] != ' ') && (svf_line[svf_ix] != TAB)) { i=1; break; }

		if (sc_command == SC_RUNTEST) {
			if (svf_token() != SC_TCK) { i=2; break; }

			if (svf_token() != SC_SEMICOLON) { i=3; break; }

//	printf("runtest %lu\n", svf_value);
			jtag_runtest(svf_value);
			continue;
		}

		if (!(sc_length=(u_int)svf_value)) { i=4; break; }

		lx=(sc_length+7) /8;
		if (lx > sizeof(jtbuf->dr_tdi)) {
			if (pt) { Writeln(); pt=0; }
			printf("date to long %u, in line\n", lx, sc_line);
			return CEN_PROMPT;
		}

		tdo_val=0;
		if (lx <= 4) {		// passt in u_long
			while (1) {
				ret=svf_token();
				if ((ret < SC_TDI) || (ret > SC_MASK)) break;

				arg=ret;
				if ((ret=svf_token()) != SC_BRACKET_O) break;

				svf_ix +=svf_a2bin(1, svf_line+svf_ix);
				if ((ret=svf_token()) != SC_BRACKET_C) break;

				if (sc_command == SC_SIR)
					switch (arg) {
			case SC_TDI:
						lvir_tdi=(u_int)svf_value; break;

			case SC_TDO:
						tdo_val=1;
						lvir_tdo=(u_int)svf_value; break;

			case SC_SMASK:
						break;

			case SC_MASK:
						lvir_mask=(u_int)svf_value; break;
					}
				else
					switch (arg) {
			case SC_TDI:
						lvdr_tdi=svf_value; break;

			case SC_TDO:
						tdo_val=1;
						lvdr_tdo=svf_value; break;

			case SC_SMASK:
						break;

			case SC_MASK:
						lvdr_mask=svf_value; break;
					}
			}

			if (ret != SC_SEMICOLON) { i=5; break; }

			if (sc_command == SC_SIR) {				// Scan Instruction Register
				if (lvir_tdi == IDCODE) {
					if (pt) { Writeln(); pt=0; }
					printf(" read ID code\n");
				}

				if (lvir_tdi == ISPEN) {
					if (pt) { Writeln(); pt=0; }
					printf(" programming enable\n");
				}

				if (lvir_tdi == NORMRST) {
					if (pt) { Writeln(); pt=0; }
					printf(" programming disable\n");
				}

				if (lvir_tdi == FADDR) {
					if (!(pt &0x3)) printf(".");
					if (++pt >= 4*70) { Writeln(); pt=0; }
				}

// printf("%d %5u %8lX %8lX %8lX\n",
// sc_command, sc_length, lvir_tdi, lvir_tdo, lvir_mask);
				if (   (jtag_tab.devs[jtag_tab.select].ddesc->ir_len != sc_length)
					 && (++irlen_err > 2)) {
					if (pt) { Writeln(); pt=0; }
					printf("illegal length of instruction register\n");
					return CEN_PROMPT;
				}

				if (jtag_tab.state == JT_UNKNOWN) {
					printf("JTAG not initialized\n");
					return CE_PROMPT;
				}

				ret=jtag_instruction(lvir_tdi);
				if (tdo_val && ((ret ^lvir_tdo) &lvir_mask)) {
					if (!(~((1 <<jtag_tab.devs[jtag_tab.select].ddesc->ir_len) -1)
							&lvir_mask)) {
						if (pt) { Writeln(); pt=0; }
						printf("line %d: IR %02X, exp: %02X/%02X, is: %02X\n",
								 sc_line, (u_int)lvir_tdi, (u_int)lvir_tdo, (u_int)lvir_mask, (u_int)ret);
//						return CEN_PROMPT;
						if (WaitKey(0)) return CEN_PROMPT;
					}
				}

				continue;
			}

// printf("%d %5u %8lX %8lX %8lX\n",
// sc_command, sc_length, lvdr_tdi, lvdr_tdo, lvdr_mask);
			dx=jtag_data(lvdr_tdi, sc_length);
			if (tdo_val && ((dx ^lvdr_tdo) &lvdr_mask)) {
				if (pt) { Writeln(); pt=0; }
				printf("illegal content of dr\n");
				return CEN_PROMPT;
			}

			continue;
		}
//
//--- DR ist Laenger als 32 Bit ---
//
		while (1) {
			ret=svf_token();
			if ((ret < SC_TDI) || (ret > SC_MASK)) break;

			switch (arg=ret) {
	case SC_TDI:
				bf=jtbuf->dr_tdi; break;

	case SC_TDO:
				tdo_val=1;
				bf=jtbuf->dr_tdo; break;

	case SC_SMASK:
				bf=0; break;

	case SC_MASK:
				bf=jtbuf->dr_mask; break;
			}

			if ((ret=svf_token()) != SC_BRACKET_O) break;

			ix=lx;
			while (ix) {
				if ((ret=svf_byte()) < 0) break;

				ix--;
				if (bf) bf[ix] =ret;
			}

			if (ret < 0) break;

			if ((ret=svf_token()) != SC_BRACKET_C) break;
		}

		if (ret != SC_SEMICOLON) { i=5; break; }

#if 0
printf("%d %5u\n",
sc_command, sc_length);
for (ix=0; ix < 8; ix++) printf(" %02X/%02X/%02X",
										  jtbuf->dr_tdi[ix], jtbuf->dr_tdo[ix],
										  jtbuf->dr_mask[ix]);
Writeln();
#endif
		if (jtag_data_exchange()) return CEN_PROMPT;

		if (!tdo_val) continue;

		for (ix=0; ix < lx; ix++) {
			if ((jtbuf->dr_tdo[ix] ^jtbuf->dr_itdo[ix]) &jtbuf->dr_mask[ix]) {
				if (pt) { Writeln(); pt=0; }
				printf("%06lX: error, exp %02X, is %02X\n",
						 tlen+ix, jtbuf->dr_tdo[ix] &jtbuf->dr_mask[ix],
						 jtbuf->dr_itdo[ix] &jtbuf->dr_mask[ix]);
				if (++ex >= 20) {
					return CE_PROMPT;
				}
			}
		}

		tlen +=lx;
	}

	if (pt) { Writeln(); pt=0; }
	printf("%u: illegal character in line %u, column %u\n", i, sc_line, svf_ix);
	svf_line[80]=0;
	printf("%s\n", svf_line);
	return CEN_PROMPT;
}
//
//--------------------------- jtag_load -------------------------------------
//
static int jtag_load(void)
{
	int		ret;
	DEV_DESC	*ddesc;
	u_long	dout;

#define _AY3	0x1F			// pc20=>0x1F; vq44=>0x3F

	read_idcodes();
	if (jtag_tab.state != JT_IDLE) return CE_PROMPT;

	printf("device    %3u ", jt_conf->jtag_dev);
	jt_conf->jtag_dev=(u_int)Read_Deci(jt_conf->jtag_dev, -1);
	if (errno) return 0;

	if ((ret=jtag_select(jt_conf->jtag_dev)) != 0) return ret;

	if (jtag_tab.state == JT_UNKNOWN) {
		printf("JTAG not initialized\n");
		return CE_PROMPT;
	}

	ddesc=jtag_tab.devs[jtag_tab.select].ddesc;
	if (!ddesc) return CE_FATAL;
//
//--- ueberpruefe ID Code, siehe xc18v0?.bsd File
//
	jtag_instruction(IDCODE);
	dout=jtag_data(-1, 32);
	if ((dout &0x0FFFFFFFL) != ddesc->id) {
		printf("wrong IDCODE: %08lX (exp: %08lX)\n", dout, ddesc->id);
		return CE_PROMPT;
	}

	if (!ddesc->fam) {
		printf("no FLASH device\n");
		return CE_PROMPT;
	}

	if (ddesc->fam &FM_18V) {		// XC18 family
		jtag_instruction(ISPEN);
		jtag_data(0x34, 6);

		jtag_instruction(FDATA3);
		jtag_data(_AY3, 6);

		jtag_instruction(FPGM);
		jtag_runtest(14000);

		jtag_instruction(FVFY3);
		jtag_runtest(50);
		jtag_data(_AY3, 6);

		jtag_instruction(CONLD);
		jtag_runtest(110000L);

		jtag_instruction(ISPEN);
		jtag_data(0x34, 6);

		jtag_instruction(CONLD);
		jtag_runtest(110000L);

		jtag_instruction(CONFIG);	// was jetzt CONLD?
	} else {
		if (!(ddesc->fam &(FM_F_P|FM_F_S))) return CE_FATAL;

		// XCF family

		printf("loading with 'CONFIG' instruction\n");
		jtag_instruction(CONFIG);
	}

	return CEN_PROMPT;
}
//
//--------------------------- JTAG_menu -------------------------------------
//
static char *const jtag_txt[] ={
	"TDI", "TMS",  "TCK",  "TDO", "?", "?", "?", "?",
	"ena", "auto", "slow", "?",   "?", "?", "?", "?"
};

static u_long	rxxx;

int JTAG_menu(
	void		(*jtag_putcsr)(u_int),
	u_int		(*jtag_getcsr)(void),
	u_long	(*jtag_getdata)(void),
	JT_CONF	*jtag_conf,
	u_char	jtag_mode)
{
	u_int		ux;
	char		hgh;
	char		key;
	int		ret;
	u_long	tmp;

	if (!jtbuf) {
		if ((jtbuf=malloc(sizeof(JTBUF))) == 0) {
			printf("can't allocate memory\n");
			return 1;
		}
	}

	jt_putcsr=jtag_putcsr;
	jt_getcsr=jtag_getcsr;
	jt_data=jtag_getdata;
	jt_conf=jtag_conf;
	jt_mode=jtag_mode;
	jtag_tab.state=JT_UNKNOWN;

	while (1) {
		Writeln();
		printf("JTAG csr            $%04X ..0 ", tmp=(*jtag_getcsr)());
		ux=16; hgh=0;
		while (ux--)
			if (tmp &((u_long)1L <<ux)) {
				if (hgh) highvideo(); else lowvideo();
				hgh=!hgh; cprintf("%s", jtag_txt[ux]);
			}
		lowvideo(); cprintf("\r\n");

		printf("JTAG data       $%08lX\n", (*jtag_getdata)());

		printf("read IDCODEs (reset)      ..1\n");
		printf("JTAG instruction          ..2\n");
		printf("JTAG data exchange        ..3\n");
		printf("JTAG program PROM         ..4\n");
		printf("JTAG verify PROM          ..5\n");
		printf("JTAG read and save binary ..6\n");
		printf("JTAG load FPGA            ..7\n");
		printf("JTAG execute SVF file     ..8\n");
		printf("JTAG read user code       ..9\n");
		printf("print known devices       ..d\n");
		printf("                            %c ",
					 jt_conf->jtag_menu);
		key=toupper(getch());
		if ((key == CTL_C) || (key == ESC)) {
			Writeln();
			(*jtag_putcsr)(0);
			return 0;
		}

		while (1) {
			use_default=(key == TAB);
			if (use_default || (key == CR)) key=jt_conf->jtag_menu;

			if (key >= ' ') printf("%c", key);
			Writeln();
			errno=0; ret=CEN_PROMPT;
			switch (key) {

		case '0':
				printf("JTAG ctrl   $%04X ", jt_conf->jtag_ctrl);
				jt_conf->jtag_ctrl=(u_int)Read_Hexa(jt_conf->jtag_ctrl, -1);
				if (errno) break;

				(*jtag_putcsr)(jt_conf->jtag_ctrl);
				ret=0;
				break;

		case '1':
				ret=read_idcodes();
				break;

		case '2':
				ret=jtag_instr();
				break;

		case '3':
				ret=jtag_exchange();
				break;

		case '4':
		case '5':
		case '6':
				ret=jtag_prom(key-'4');
				Writeln();
				if (mcs_hdl != -1) { close(mcs_hdl); mcs_hdl=-1; }
				if (jtag_tab.state == JT_DR_SHIFT) {
					jtag_rd_data(0, 0);
					printf("%02X\n", jtag_instruction(NORMRST));
					jtag_runtest(110000L);
					printf("%02X\n", jtag_instruction(BYPASS));
				}
				break;

		case '7':
				ret=jtag_load();
				break;

		case '8':
				ret=jtag_svf();
				if (svfdat) { fclose(svfdat); svfdat=0; jt_conf->jtag_menu=key; }

				break;

		case '9':
				ret=jtag_usercode();
				break;

		case 'D':
				ux=0;
				while (dev_desc[ux].id != 0) {
					printf("0x%08lX %s\n", dev_desc[ux].id, dev_desc[ux].name);
					ux++;
				}
				break;

		case 'X':
				printf("runtest %8lu ", rxxx);
				rxxx=Read_Deci(rxxx, -1);
				if (errno) break;

				timemark();
				jtag_runtest(rxxx);
				printf("%lu\n", ZeitBin());
				break;
			}

			while (kbhit()) getch();
			if ((ret == CE_MENU) || (ret == CE_PROMPT)) jt_conf->jtag_menu=key;

			if (ret <= 0) break;

			if (ret > CE_PROMPT) display_errcode(ret);

			printf("select> ");
			key=toupper(getch());
			if (key == TAB) continue;

			if (key < ' ') { Writeln(); break; }

			if (key == ' ') key=CR;
		}
	}
}
