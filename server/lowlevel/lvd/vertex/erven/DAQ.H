// Borland C++ - (C) Copyright 1991, 1992 by Borland International

// Header File: daq.h	general DAQ system

#include "jtag.h"
#include "regs.h"

//	======================================================================= --
//							access via SIS1100 and Master System Controller       --
//	======================================================================= --

#ifdef SC_M
#include "sis1100.h"
//
#define CLR_PROT_ERROR()	clr_prot_error()
#define BUS_ERROR				protocol_error(-1, 0)
#define TST_BUS_ERROR		(gigal_regs->sr &SR_PROT_ERROR)
#define CLR_BUS_ERROR		clr_prot_error()

#define SC_UNIT(i)			bus1100->coupler +i
//
// ========================== CMASTER Card ==================================
//
typedef struct {
	u_long	version;			//00
	u_long	sr;				//04 Status Register
#define DBELL_F1_ERR		0xFF000000L
#define DBELL_EVENTPF	0x00FF0000L
#define DBELL_EVENT		0x0000FF00L
#define DBELL_BLOCK		0x00000001L

	u_long	cr;				//08 Control Register, 8 bit
#define SCR_DDTX		0x01	// direct data transfer
#define SCR_SCAN		0x02	// scan LVD front bus
#define SCR_EV_DBELL	0x04	// single event doorbell
#define SCR_LED0		0x08	// untere linke LED
#define SCR_LED1		0x10	// untere rechte LED

	u_long	res0C;			//0C ist in SIS1100 fuer semaphore reserviert
	u_long	dd_counter;		//10
	u_long	dd_blocksz;		//14
	u_long	jtag_csr;		//18 JTAG control/status
	u_long	jtag_data;		//1C JTAG data
	u_long	timer;			//20
} CM_REGS;
//
// ========================== BUS Struktur ==================================
//
// Adressmapping eine optical Koppelkarte
//
typedef struct {
	TDCF1_RG		in_card[16];	// 0000     eigene Eingabekarten (backplane)
	SYSCONF1_RG	res0800[16];	// 0800     angeschlossene Koppelkarten (80MB LVD BUS)
	TDCF1_RG		res1000[16];	// 1000     Eingabekarten einer selektierten Koppelkarte
	SYSCONF1_RG	l_coupler;		// 1800     lokale Koppelkarte
	TDCF1_BC		in_card_bc;		// 1880     broadcast Bereich der eigenen Eingabekarten
	COUPLER_BC  res1900;			// 1900     broadcast Bereich der angeschlossenen Koppelkarten
	TDCF1_BC		res1980;			// 1980     broadcast Bereich der Eingabekarten
										//             einer selektierten Koppelkarte
} BUS1100;							// 1A00
//
// ========================== SIS1100 extendet protocol errors ==============
//
#define PE_BLK_ND		0x09	// block read, no data at all
#define PE_BLK_BERR	0x0A	// block read, bus error
//
// ==========================================================================
//                            Externals
// ==========================================================================
//
// -------------------------- daq.c -----------------------------------------
//
extern char				gigal_ok;
extern u_short			gtmp;				// wegen Bit Test im high byte
extern SIS1100_REGS	*gigal_regs;	// local register
extern CM_REGS			*cm_reg;			// CMASTER register
extern BUS1100			*bus1100;

void clr_prot_error(void);
int protocol_error(u_long lp, char *id);
int send_dmd_eot(void);
//
#define DMA_64K		16					// 64k pages for DMA buffer => 1 Mb

#else
//	======================================================================= --
//							access via PCI LVDBUS                                 --
//	======================================================================= --

#include "lvdbus.h"
//
#define DMA_64K		1					// 64k pages for DMA buffer => 1 Mb

#define CLR_PROT_ERROR()		lvd_regs->sr=lvd_regs->sr
#define BUS_ERROR					bus_error()
#define TST_BUS_ERROR			(lvd_regs->sr &SR_LB_TOUT)
#define CLR_BUS_ERROR			lvd_regs->sr =SR_LB_TOUT

// ==========================================================================
//										input LVD process bus
// ==========================================================================

typedef struct {
	TDCF1_RG    in_card;			//  0
	TDCF1_BC		in_card_bc;		// 80
	u_short		res100[128];	//100
} LVD_BUS;							//200

// ==========================================================================
//                            Externals
// ==========================================================================
//
//--------------------------- daqinp.c --------------------------------------
//
extern char			lvd_ok;
extern u_short		gtmp;		// wegen Bit Test im high byte
extern LVD_REGS	*lvd_regs;
extern void			*lvd_bus;
#endif

//
// ========================== Input Card ====================================
//
// -------------------------- threshold limits ------------------------------
//
#if 0
#ifdef MWPC
#define THRH_VIN		0.5	// 0.256
#define THRHW			3			// Nachkommastellen
#else
#define THRH_VIN		2.04
#define THRHW			2			// Nachkommastellen
#endif
#endif

#define THRH_MIN		-THRH_VIN
#define THRH_MAX		((255./128.-1.)*THRH_VIN)
#define THRH_SLOPE	((THRH_MAX-THRH_MIN)/255.0)
//
//--------- F1 definitions
//
#define R1_HITM			0x4000
#define R1_LETRA			0x2000
#define R1_FAKE			0x0400
#define R1_OVLAP			0x0200
#define R1_DACSTRT		0x0001

#define R7_BEINIT			0x8000

#define R15_ROSTA			0x0008	// ring oszillator start
#define R15_SYNC			0x0004
#define R15_COM			0x0002
#define R15_8BIT			0x0001

#define T_REF				25.0		// 40 MHz
#define REFCLKDIV			7
#define HSDIV				(T_REF*(1<<REFCLKDIV)/0.152)
#define REFCNT				0x100		// used for synchron mode
//
//--------- GPX definitions
//
#define G_HSDIV			(T_REF*(1<<REFCLKDIV)/0.216)
#define G_STARTOFF1		0x20000L
#define G_ST_TMR			0x100		// Start Timer (6.4 us)
//
//--------- Readout Data Header
//
#define HDR_LEN	3
//
// ==========================================================================
//                            Programm and Configuration
// ==========================================================================
//
#define SP_			20		// Menuausgabe Spalte
#define ZL_			5		// Zeile
#define CB_UNIT	4					// Bitanzahl fuer Input Units fuer DAQ
											// sonst > 64k
#define C_UNIT		(1<<CB_UNIT)	// Anzahl Input Units fuer DAQ

#define C_MTIME		0x3F
#define EV_CHAN(w)	( (u_int)((u_long)(w) >>22) &((1 <<(CB_UNIT+6))-1))
#define EV_MTIME(w) 	( (u_int)((u_long)(w) >>16) &C_MTIME)	// main time
#define EV_FTIME(w) 	( (u_int)(w))									// F1 time
//
// -------------------------- error code ------------------------------------
//
#define CEN_DAC_CHG		-5		// DAC wurde geaendert
#define CEN_FPRINT		-4		// max 2 lines
#define CEN_PPRINT		-3		// more than 2 lines
#define CEN_PROMPT		-2		// display no command menu, only prompt
#define CEN_MENU			-1
#define CE_MENU			1
#define CE_PROMPT			2		// display no command menu, only prompt
#define CE_PARAM_ERR		3
#define CE_NO_DMA_BUF	4
#define CE_FATAL			5
#define CE_NO_SC			6		// no system controller
#define CE_NO_INPUT		7
#define CE_F1_NOLOCK		8
#define CE_NOT_IMPL		9		// function not implemented

#define CE_KEY_SPECIAL	0x100
#define CE_KEY_PRESSED	0x200

// -------------------------- Configuration Buffer --------------------------
//
#define C_WSITEMS		8

typedef struct {
	u_int		unit;
	u_char	slv_addr;
	u_char	type;
#define TDCF1		0x30
#define VERTEX		0x40
#define SYMASTER	0x50
#define SYSLAVE	0x60
#define FQDC		0x70
#define FQDCTST	0x78
#define TDCGPX		0x80
#define TDCGPXTST	0x88
#define SQDC		0x90
#define SQDCTST	0x98
#define JTEST		0xF8

	void		*inp_regs;
	union {
		u_long	l[12];
		u_int		s[24];
	} slv_data;

	int		f1_chip;
	u_int		inp_con;
	u_char	inp_dac[4];
} INP_DB;

typedef struct {
	int			x_unit;
	int			x_sc_addr;
	SYSCONF1_RG	*sc_unit;
	TDCF1_BC		*inp_bc;
	u_long		cmst_data[10];
	u_int			tst_cnt;
	int			tst_delay;

	u_int			slv_sel;
	u_int			slv_cnt;
	INP_DB		inp_db[16];		// logisch von [slv_first] bis [15]
	int			dac_iunit;		// input unit
	INP_DB		*dac_iconf;
	TDCF1_RG		*dac_iregs;
	u_int			dac_con;			// connecter for threshold
} SC_DB;

typedef struct {
	u_long	hd[3];
	u_short	dac0,dac1,lat,win,ntol,tlev,cr,mean,noise;
} DAT_HDR;

typedef struct {
	u_int		dac0,dac1;
	u_int		a_min,a_max;
	u_char	comp;
	u_char	bool;
#define _POL	0x01		// signal polarity
#define _ITG	0x02		// internal (board) trigger
#define _RAW	0x04		// all ADC data of the window
#define _GRD	0x08		// single gradient
#define _MEA	0x10		// display mean line
#define _TP		0x20		// test pulse
#define _VEB	0x40		// verbose analysing data
#define _ADA	0x80		// analyzing data

	u_int		shift;
	u_int		anal_ctrl;
	u_int		threshold;
	u_int		tlevel;
	int		latency;
	int		window;
	int		i_length;
	int		iw_start;
	int		iw_length;
} SQ_DATA;

typedef struct {
	u_int		dac0,dac1;
	u_int		a_min,a_max;
	u_char	comp;
	u_char	bool;
#define _XALL	0x01		// all ADC data of the window
#define _XRAW	0x04		// all ADC data of the window
#define _XCDA	0x08		// display common mode data
#define _XZRO	0x10		// display zero line
#define _XTP	0x20		// test pulse
#define _XVEB	0x40		// verbose analysing data

	u_int		shift;
	u_int		clkdelay;
	u_int		noisethr;
	u_int		threshold;
	u_int		pedestal;
} VTX_DATA;

typedef struct {			// VATA register parameter
	u_char	glthr;
	u_char	locthr;
	u_long	disable[5];
	u_char	glref;
	u_char	ctrl;
} VTX_REGP;

#ifdef MWPC
#define MAX_ROWS		64			// # of rows (# of CMP16) per column
#define MAX_COLUMNS	3

typedef struct {
	u_short	columns;
	u_short	act_col;
	struct {
		u_short	rows;						/* # of CMP16 chips */
		u_char	dac[MAX_ROWS];
	} column[MAX_COLUMNS];

	u_short	mrout_start;				// total_readout(), first channel
	u_short	mrout_chan;					//    # of channels

	u_short	ef_start[3];				// 3 columnes
	u_short	ef_chan;						// # of channels
	u_short	ef_ped;						// efficiency pedestals
	u_short	ef_time_iv;					// time interval
	u_short	ef_trg_iv;
	u_short	ef_cntmx;

	short		mr_dlay;					// delay after displaying data (readout)
	char		dataname[80];

} MWPC_CONF;
#endif

#ifdef MEASURE
typedef struct {
	char		menu_0;
	char		menu_1;
	int		chan;
	int		chan1;
} MEAS;
#endif

typedef struct {
	PCI_CONFIG	pconf;

	char		tst_menu;
	char		reg_menu;
	char		tmp_menu;
	char		dma_menu;
	char		c0_menu;
	char		c1_menu;
	char		c2_menu;
	char		c3_menu;
	char		i0_menu;
	char		i1_menu;
	char		i2_menu;
	char		i3_menu;
	u_long	ao_regs[16];
	u_int		crd_reg;						// cyclic read
	u_int		crd_addr[C_REGION+1];
	u_int		crd_size[C_REGION+1];
	u_int		crd_cycles;

	u_int		cwr_reg,						// cyclic write
				cwr_addr[C_REGION+1];
				cwr_size[C_REGION+1];
	u_int		cwr_cycles;
	u_long	cwr_strt[C_REGION+1],
				cwr_inc[C_REGION+1];

	u_int		cwrvfy_reg,					// cyclic write/verify
				cwrvfy_addr;
	u_int		cwrvfy_size[C_REGION+1];
	u_long	cwrvfy_strt,
				cwrvfy_inc,
				cwrvfy_msk;

	u_int		cwrseq_items;
	u_int		cwrseq_size;
	struct {
		u_int		reg,
					addr;
		u_long	value;
	} cwrseq_objs[C_WSITEMS];

	u_int		cwrrdseq_items;
	struct {
		u_int		w_reg,
					w_addr;
		u_int		r_reg,
					r_addr;
		u_long	value;
	} cwrrdseq_objs[C_WSITEMS];

	u_int		mver_reg;					// memory verify region
	u_long	mver_addr[C_REGION+1];
	u_int		mver_size[C_REGION+1];

	u_int		phys_pg;
	u_int		dbuf_verify;

#ifdef SC_M
	u_int		xx_size;
	u_int		xx_am;
	u_long	xx_addr;
	u_long	xx_data;

	u_long	x_hdr;
	u_long	x_am;
	u_long	x_ad;
	u_long	x_ad_h;
	u_long	x_da;
	u_long	x_da_h;

	JT_CONF	jtag_scm0;		// SN000-SN002 1k FIFO, other layout
	JT_CONF	jtag_scm1;		// SN003-SN004 1k FIFO
	JT_CONF	jtag_scm3;		// SN004-SNxxx 8k FIFO
	JT_CONF	jtag_sc0;		// new GPX system controller, serial 001-010
	JT_CONF	jtag_sc1;		// new GPX system controller, serial >= 011
	u_long	dma_regs[16];	// [0..4] d_hdr, d_am, d_ad, d_ad_h, d_bc
									// [5..6] rd_pipe_adr, rd_pipe_len
									// [7]: DMA remote address
									// [8]: DMA remote length
									// [9]: dmd DMA, EOT
									// [10..11] d0_bc_adr, d0_bc_len
#endif

	JT_CONF	jtag_tdcf1_0;
	JT_CONF	jtag_tdcf1_1;
	JT_CONF	jtag_tdcf1_2;
	JT_CONF	jtag_tdcf1_3;
	JT_CONF	jtag_vertex_0;
	JT_CONF	jtag_vertex_1;
	JT_CONF	jtag_tmaster;
	JT_CONF	jtag_tslave_0;
	JT_CONF	jtag_tslave_1;
	JT_CONF	jtag_tslave_2;
	JT_CONF	jtag_qdcf;
	JT_CONF	jtag_qdcf_t;
	JT_CONF	jtag_tdcgpx;
	JT_CONF	jtag_qdcs;
	JT_CONF	jtag_qdcs_t;

	SC_DB		sc_db;

	u_int		sc_f1_reg[16];
	u_int		inp_f1_reg[16];
	u_long	inp_gpx_reg[16];

	u_int		hsdiv;
#define RESOLUTION	(HSDIV/config.hsdiv)
#define RESOLUTIONNS	((HSDIV/1000.0)/config.hsdiv)

	u_int		g_hsdiv;		// GPX HSDIV
#define G_RESOLUTION		(G_HSDIV/config.g_hsdiv)
#define G_RESOLUTIONNS	((G_HSDIV/1000.0)/config.g_hsdiv)

	u_long	g_startoff1;
	char		f1_sync;			// 1: F1 SYNCHRON mode
									// 2: F1 COMMON mit F1 Start
	char		res;
	u_int		f1_letra;		// 15:12 falling syscon GPX trigger source
									// 11: 8 rising  syscon GPX trigger source
									//  5: 4 falling/rising sysconF1 trigger source
									//  3: 0 TDC
	u_long	dis_cha[2];
	u_int		rout_delay;
	u_int		daq_mode;
#define EVF_HANDSH	0x0004
#ifdef SC_M
#define EVF_DBELL		0x0008
#define EVF_DBELLPF	0x0010
#define EVF_DDATA		0x0020	// direct event data transfer
#define EVF_BCBUF		0x0040
#define EVF_IGNOVR	0x0080
#endif
#define EVF_ON_TP		0x0100	// only test pulse
#define EVF_SORT		0x0200
#define EVF_TM_SORT	0x0400
#define EVF_TRIG		0x0800	// start trigger system
#define EVF_TEVC		0x1000	// use event counter from trigger system
#define EVF_NEVC_INC	0x2000	// no event counter increment
#define EVF_NEVC		0x4000	// no event counter test
#define EVF_F1TIME	0x8000	// use F1 time base

	int		daq_sc;
	int		inp_trigger;	// Trigger Channel
	float		ef_trg_lat;		// latency
	float		ef_trg_win;		// window
#ifdef SC_M
	u_long	dd_block;
#endif

	char		logname[80];
	char		dlogname[80];
#define C_SEL_CHA	16
	int		sel_cha[C_SEL_CHA];

	u_char	trig_mode;
	u_char	daq_edge;
	u_int		test_dac;
	u_long	tcount;

//--- sequencer

	char		sq_save_file[80];
	char		sq_load_file[80];
	u_long	save_end;
	u_short	srf_lw;

	u_long	srtst_strt;
	u_long	srtst_end;
	u_int		srtst_mode;

	u_long	trtst_strt;
	u_long	trtst_end;
	u_int		trtst_mode;

	u_long	sytst_strt;
	u_long	sytst_end;
	u_int		sytst_mode;

//--- synch master

	u_long	tst_cvt;
	u_long	sm_data[16];
	u_int		tst_tatw;
	u_int		tst_fcat;
	u_char	tst_t4ut;
	u_char	tst_stcs;
	u_int		tst_aux;
	u_int		tst_auxfclr;
	u_char	puwidth[6];

//--- QDC Parameter (16 = all channel)

	int		sq_cha;			// -1 find first active channel (auto_level)
									// -2 dto but use same (sq_data[16]) parameters
	int		all_cha;			// data from all channels
#define	_ALL	0x01

	SQ_DATA	sq_data[19];
	u_int		auto_level;

	int		dcm_phase;

	u_int		dac_data[5];
	u_char	inp_onl[16];	// ix is unit nr, 0: offline
	u_int		checker;

//--- VERTEX parameter

	u_int		vtx_cpar;		// clock parameter
	u_char	vtx_mon;			// monitoring
	u_char	vtx_varep;
	u_char	vtx_cha;
	u_char	vtx_trg;
	VTX_DATA	vtx_data[2];
	u_int		vtx_dac[8];
	VTX_REGP	vtx_regp;
	u_int		vtx_reg[55];
	u_int		vtx_tm[18];
	u_int		vtx_digtst;
	u_int		vtx_vtim;
	u_int		vtx_mode;

#ifdef MEASURE
	MEAS		meas;
#define measconf	config.meas
#endif

#ifdef MWPC
	MWPC_CONF	mwpc;
#define mwconf		config.mwpc
#endif

} CONFIG;

//
// ==========================================================================
//                            Externals
// ==========================================================================
//
// -------------------------- MAIN.c ----------------------------------------
//
//
// config is imported by pci.c with the type PCI_CONFIG (see pci.h)!
//
extern CONFIG		config;
#define pciconf	config.pconf

extern char			UCLastKey[20];

extern PLX_REGS	*plx_regs;
extern u_char		*eeprom;

//--- DMA Buffer ---

#define SZ_PAGE		0x1000
#define SZ_PAGE_L		(SZ_PAGE/4)

#define DMA_PAGES		(16*DMA_64K)	// max number of DMA pages

extern u_long	*dmabuf[];			// DMA buffer
extern u_int	dmapages;

extern u_long	dmabuf_phy[];		// physical addresses

extern u_long	*dmadesc;			// DMA descriptor buffer
extern u_long	dmadesc_phy[];

typedef u_int *(BIGBUF_S)[];
typedef u_long *(BIGBUF_L)[];
#define DBUF_L(i)	(dmabuf[(u_int)((i) >> 14)])[(u_int)(i) &0x3FFF]
#define DBUF_S(i)	((*(BIGBUF_S*)&dmabuf)[(u_int)((i) >> 15)])[(u_int)(i) &0x7FFF]

//--- work buffer ---

#define CB_WBUF	6		
#define C_WBUF		(1<<CB_WBUF)	// =64*64k pages => 4 Mb

extern u_int	*abuf[C_WBUF];

//#define WBUF_L(i)	((*((u_int *[])*)&abuf)[(u_int)((i) >> 14)])[(u_int)(i) &0x3FFF]
#define WBUF_L(i)	((*(BIGBUF_L*)&abuf)[(u_int)((i) >> 14)&(C_WBUF-1)])[(u_int)(i) &0x3FFF]
#define WBUF_S(i)	(abuf[(u_int)((i) >> 15)&(C_WBUF-1)])[(u_int)(i) &0x7FFF]

//--- external functions

int bus_error(void);
void display_errcode(int err);
int err_logging(void);
void _delay_(u_long dx);
int dma_buffer(
	u_int		mode);	// 0: display and clear pages
							// 1: clear pages
//
// -------------------------- input.c ---------------------------------------
//
extern SC_DB	*sc_conf;
#ifdef SC_M
extern SYSCONF1_RG	*sc_regs;	// selectierter System Controller (mit transparent)
extern SYSCON_RG		*sy_regs;	// new system controller with GPX
#endif
extern INP_DB	*inp_conf;
extern u_char	inp_type[];		// index is LVD address, see INP_DB.type
extern u_char	inp_onl[];		// ix is unit nr, 0: offline

extern u_long	glp;				// global loop counter

extern int		last_thr_dac;
extern u_long	kbatt;
extern u_long	kbatt_msk;
extern char		tstevc;			// zur Kontrolle des Eventcounters
extern float	reso;
extern u_long	*robf;
extern u_int	rout_len;		// bytes
extern u_int	lrout_len;		// lwords
extern char		rout_hdr;		// event mode
extern u_long	rout_trg_time;	// Main Trigger Time (only with handshake)
extern u_long	rout_ev_inf;
extern u_int	tm_cnt;			// Anzahl Zeitmarken, sortiert nach Kanal und Zeit
typedef struct {
	u_int		tm_cha;
	u_int		tm_width;
	long		tm_diff;
} TM_HIT[64*2*C_UNIT];		// double pulse possible
extern TM_HIT	*tm_hit;		// Zeitmarken (Kanal und Zeit)

extern long		tlat,twin;
extern long		f1_range;		// max value of time low word (6.4us)
extern long		gpx_range;		// dto.
extern long		pause;
extern u_long	eventcount;

void setup_dma(
	int		chn,	// -1: channel 0 demand mode
	u_int		dix,	// first descriptor index
	u_int		fpg,	// first page
	u_long	len,
	u_long	addr,
	u_long	ofs,	// offset
	u_int		mode);
#define DMA_L2PCI		0x01
#define DMA_NOEOT		0x02
#define DMA_FASTEOT	0x04
#define DMA_ADDRINC	0x10
#define DMA_CIRCULAR	0x20

int inp_assign(	// 0:					Ueberpruefung ok
						// CEN_MENU:		Eingabeabbruch
						// CE_NO_INPUT:	Modul nicht vorhanden
						// CE_FATAL:		Plausibilitaetsfehler
	SC_DB		*sconf,
	u_char	mode);	// 0: neu zuordnen
							// 1: ueberpruefen
							// 2: Karte waehlen

int sc_assign(	// 0:				alle zugewiesen, auch Input Units
					// CEN_MENU:	Eingabeabbruch, keine Aenderung
					// CE_NO_SC:	kein System Controller vorhanden
					// CE_NO_INPUT	inp_assign(), keine Input Unit vorhanden
	u_char	mode);	// 0: neu zuordnen
							// 1: ueberpruefen
							// 2: Karte waehlen

int f1_regdiag(
	u_int	*regs);

int inp_setdac(	//  0: ok
	SC_DB		*sc_cnf,		//  0 alle
	INP_DB	*inp_cnf,	//  0 alle
	int		con,			// -1 single
								// -2 alle
	int		dac);			//>=0 alle auf diesen Wert setzen
								// -1 Dialog
								// -2 um 1 erhoehen
								// -3 um 1 erniedrigen
#define DAC_DIAL	-1
#define DAC_ODIAL	-2
#define DAC_ADD1	-3
#define DAC_SUB1	-4

int general_setup(		// 0: no error
	int	sc,	// system controller
					//  -2: alle
					//  -1: master
					// >=0: slave
	int	inp);	//  -2: keine (nur system controller)
					//  -1: alle
int read_event(
	char	mode,
#define EVF_NODAC	0x01	// ohne DAC Einstellung

	int	trig);	// >=0: trigger channel
#define RO_XRAW	-3	// multi events too
#define RO_RAW		-2	// raw
#define RO_TRG		-1	// passend zu rout_trg_time

int inp_mode(
	u_char	mode);
#define IM_TREF		0x01	// mit Trigger Reference
#define IM_STRIG		0x02	// mit SC Trigger als Reference
#define IM_NODAQTRG	0x04	// MCR_DAQTRG nicht erlaubt
#define IM_VERTEX		0x08
#define IM_NOBCBUF	0x10	// nobcbuff when ddate
#define IM_QUIET		0x20

int stop_ddma(void);
int application_dial(u_char pup);
//
// -------------------------- symaster.c ------------------------------------
//
int sync_master_menu(int mode);
//
// -------------------------- syslave.c -------------------------------------
//
int sync_slave_menu(void);
//
// -------------------------- tdcf1.c ---------------------------------------
//
int xinp_f1reg(		// 0: ok
	TDCF1_RG	*f1tdc_rg,
	u_int		reg,
	u_int		value);
int tdcf1_menu(void);
//
// -------------------------- gpx.c -----------------------------------------
//
int gpxtdc_menu(void);
//
// -------------------------- qdcftst.c -------------------------------------
//
int qdcftst_menu(void);
//
// -------------------------- qdcf.c ----------------------------------------
//
int qdcf_menu(void);
//
// -------------------------- qdcs.c ----------------------------------------
//
int qdcs_menu(void);
//
// -------------------------- qdcstst.c -------------------------------------
//
int qdcstst_menu(void);
//
// -------------------------- jtest.c ----------------------------------------
//
int jtest_menu(void);
//
// -------------------------- vertex.c --------------------------------------
//
int vertex_menu(void);
//
// -------------------------- vtexutil.c --------------------------------------
//
int LoadSequencer(
	VTEX_RG	*vtex_rg,
	int		mem,
	char		*name);

int LoadTram(
	VTEX_RG	*vtex_rg,
	char		*name,
	char		hv);

int sram_copyout(
	VTEX_RG	*vtex_rg,
	u_char	mode,	// bit 0: 0 => use DMA buffer (DBUF), max len 0x100000 (1 Mb)
						//        1 => copy Work buffer (WBUF) do DMA buffer
						// bit 1: 0	=> sequencer SRAM
						//        1 => table SRAM
	u_long	len,
	u_long	sram_addr);

int sram_copyin(
	VTEX_RG	*vtex_rg,
	u_char	mode,	// bit 0: 0 => use DMA buffer (DBUF), max len 0x100000 (1 Mb)
						//        1 => copy Work buffer (WBUF) do DMA buffer
						// bit 1: 0	=> sequencer SRAM
						//        1 => table SRAM
	u_long	len,
	u_long	sram_addr);
//
// -------------------------- exp.c -----------------------------------------
//
int raw_input(int sel);
int triggered_input(int sel);
int pulse_width(void);
#ifdef MWPC
int mwpc_application(void);
#else
int tof_application(void);
#endif
//
#ifdef MEASURE
// -------------------------- measure.c -------------------------------------
//
extern u_int	prt_org;			// Null Index

void print_distribution(u_int mode);
int measurement(void);

#endif
