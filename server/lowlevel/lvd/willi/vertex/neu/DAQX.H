//
// ========================== Input Card ====================================
//
// -------------------------- threshold limits ------------------------------
//
#ifdef MWPC
#define THRH_VIN		0.5	// 0.256
#define THRHW			3			// Nachkommastellen
#else
#define THRH_VIN		2.04
#define THRHW			2			// Nachkommastellen
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
#define HDR_LEN	3
//
// ==========================================================================
//                            Programm and Configuration
// ==========================================================================
//
#define SP_			20		// Menuausgabe Spalte
#define ZL_			5		// Zeile
#define CB_UNIT	3					// Bitanzahl fuer Input Units fuer DAQ
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
	u_char	f1tdc;
	void		*inp_regs;
	u_int		slv_data[8];

	int		f1_chip;
	u_int		inp_con;
	u_char	inp_dac[4];
} INP_DB;

typedef struct {
	int		unit;
	int		sc_addr;
	COUPLER	*sc_unit;
	F1TDC_BC	*inp_bc;
	u_long	cmst_data[8];
	u_int		tst_cnt;
	int		tst_delay;

	u_int		slv_sel;
	u_int		slv_cnt;
	INP_DB	inp_db[16];		// logisch von [slv_first] bis [15]
	int		dac_iunit;		// input unit
	INP_DB	*dac_iconf;
	F1TDC_RG	*dac_iregs;
	u_int		dac_con;			// connecter for threshold
} SC_DB;

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
	char		epr_menu;
	char		tmp_menu;
	char		dma_menu;
	char		c0_menu;
	char		c1_menu;
	char		c2_menu;
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

	JT_CONF	jtag_scm0;
	JT_CONF	jtag_scm1;
	JT_CONF	jtag_scm;
	u_long	dma_regs[16];	// [0..4] d_hdr, d_am, d_ad, d_ad_h, d_bc
									// [5..6] rd_pipe_adr, rd_pipe_len
									// [7]: DMA remote address
									// [8]: DMA remote length
									// [9]: dmd DMA, EOT
									// [10..11] d0_bc_adr, d0_bc_len
#endif

	JT_CONF	jtag_slave_sc0;
	JT_CONF	jtag_slave_sc;
	JT_CONF	jtag_input0;
	JT_CONF	jtag_input;
	JT_CONF	jtag_vertex;

	int		sc_units;	// 0 .. C_SC_UNITS
	int		sc_unitnr;	// -1 .. (C_SC_UNITS-1)
	int		dac_sc_nr;	// ./.
#define C_SC_UNITS	3
	SC_DB		sc_db[C_SC_UNITS+1];

	u_int		sc_f1_reg[16];
	u_int		inp_f1_reg[16];

	u_int		hsdiv;
#define RESOLUTION	(HSDIV/config.hsdiv)
#define RESOLUTIONNS	((HSDIV/1000.0)/config.hsdiv)

	char		f1_sync;			// 1: F1 SYNCHRON mode
									// 2: F1 COMMON mit F1 Start
	u_char	f1_letra;

	u_int		rout_delay;
	u_int		daq_mode;
#define EVF_HANDSH	0x04
#ifdef SC_M
#define EVF_DBELL		0x08
#define EVF_DBELLPF	0x10
#define EVF_DDATA		0x20		// direct event data transfer
#define EVF_NOBCBUF	0x40
#define EVF_IGNOVR	0x80
#endif
#define EVF_ON_TP		0x100		// only test pulse
#define EVF_SORT		0x200
#define EVF_IGNNUL	0x400
#define EVF_VTDUM		0x800

	int		daq_sc;
	int		inp_trigger;	// Trigger Channel
	float		ef_trg_lat;		// latancy
	float		ef_trg_win;		// window
#ifdef SC_M
	u_long	dd_block;
#endif

	char		logname[80];
#define C_SEL_CHA	16
	int		sel_cha[C_SEL_CHA];

	u_char	trig_mode;
	u_char	daq_edge;
	u_int		test_dac;
	u_long	tcount;

//--- sequencer

	char		sq_save_file[80];
	char		sq_load_file[80];
	u_short	srf_lw;

	u_long	srtst_strt;
	u_long	srtst_end;
	u_int		srtst_mode;

	u_long	trtst_strt;
	u_long	trtst_end;
	u_int		trtst_mode;

	u_long	save_end;

#ifdef MWPC
	MWPC_CONF	mwpc;
#define mwconf		config.mwpc
#endif

#ifdef MEASURE
	MEAS		meas;
#define measconf	config.meas
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

#define DMA_64K		16					// 64k pages for DMA buffer => 1 Mb
#define DMA_PAGES		(16*DMA_64K)	// max number of DMA pages

extern u_long	*dmabuf[];			// DMA buffer
extern u_int	dmapages;

extern u_long	dmabuf_phy[];		// physical addresses

extern u_long	*dmadesc[];			// DMA descriptor buffer
extern u_long	dmadesc_phy[];

typedef u_int *(BIGBUF_S)[];
typedef u_long *(BIGBUF_L)[];
#define DBUF_L(i)	(dmabuf[(u_int)((i) >> 14)])[(u_int)(i) &0x3FFF]
#define DBUF_S(i)	((*(BIGBUF_S*)&dmabuf)[(u_int)((i) >> 15)])[(u_int)(i) &0x7FFF]

//--- work buffer ---

#define C_WBUF		64		// 64k pages => 4 Mb

extern u_int	*abuf[64];

//#define WBUF_L(i)	((*((u_int *[])*)&abuf)[(u_int)((i) >> 14)])[(u_int)(i) &0x3FFF]
#define WBUF_L(i)	((*(BIGBUF_L*)&abuf)[(u_int)((i) >> 14)])[(u_int)(i) &0x3FFF]
#define WBUF_S(i)	(abuf[(u_int)((i) >> 15)])[(u_int)(i) &0x7FFF]

//--- external functions

void display_errcode(int err);
int err_logging(void);
void _delay_(u_long dx);
int dma_buffer(
	u_int		mode);	// 0: display and clear pages
							// 1: clear pages
//
// -------------------------- inputx.c --------------------------------------
//
extern SC_DB		*sc_conf;
extern COUPLER_BC	*sc_bc;		// front bus broad cast
extern INP_DB		*inp_conf;
extern u_char		inp_type[];		// ix is LVD address, T: VERTEX

extern int		last_thr_dac;
extern u_long	kbatt;
extern u_long	kbatt_msk;
extern u_long	*robf;
extern u_int	rout_len;
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

extern long		tm_win_le,tm_win_he;	// trigger window, low/high edge
extern long		tlat,twin;
extern long		f1_range;		// max value of time low word (6.4us)
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
#define DMA_CIRCULAR	0x10

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
#define DAC_ADD1	-2
#define DAC_SUB1	-3

int general_setup(		// 0: no error
	int	sc,	// system controller
					//  -2: alle
					//  -1: master
					// >=0: slave
	int	inp);	//  -2: keine (nur system controller)
					//  -1: alle
int read_event(
	char	mode,		// EVF_NODAC: ohne DAC Einstellung
#define EVF_NODAC	0x01

	int	trig);	// -3: TO_XRAQ, multi events too
						// -2: RO_RAW, raw
						// -1: RO_TRG, passend zu rout_trg_time
						//  c: trigger channel
#define RO_XRAW	-3
#define RO_RAW		-2
#define RO_TRG		-1

int inp_mode(
	u_char	mode);	// 0x01: mit Trigger Reference
							// 0x02: mit SC Trigger als Reference
							// 0x04: MCR_DAQTRG nicht erlaubt
							// 0x10: nobcbuff when ddate
#define IM_TREF		0x01
#define IM_STRIG		0x02
#define IM_NODAQTRG	0x04
#define IM_VERTEX		0x08
#define IM_NOBCBUF	0x10

int stop_ddma(void);
int application_dial(u_char pup);
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
// -------------------------- daq.c -----------------------------------------
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
int measurement(void);
#endif
