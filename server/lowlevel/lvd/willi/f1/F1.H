// Borland C++ - (C) Copyright 1991, 1992 by Borland International

// Header File

//	f1.h -- optical control mit SIS1100

#include "sis1100.h"
#include "regs.h"
//
// ========================== CMASTER Card ==================================
//
typedef struct {
	u_long	version;			//00
	u_long	sr;				//04 Status Register
#define DBELL_F1_ERR		0xFF000000L
#define DBELL_EVENTPF	0x00FF0000L
#define DBELL_EVENT		0x0000FF00L

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
	IN_CARD     in_card[16];	// 0000     eigene Eingabekarten (backplane)
	COUPLER     coupler[16];	// 0800     angeschlossene Koppelkarten (80MB LVD BUS)
	IN_CARD     c_in_card[16];	// 1000     Eingabekarten einer selektierten Koppelkarte
	COUPLER     l_coupler;		// 1800     lokale Koppelkarte
	IN_CARD_BC  in_card_bc;		// 1880     broadcast Bereich der eigenen Eingabekarten
	COUPLER_BC  coupler_bc;		// 1900     broadcast Bereich der angeschlossenen Koppelkarten
	IN_CARD_BC  c_in_card_bc;	// 1980     broadcast Bereich der Eingabekarten
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
// -------------------------- f1.c -----------------------------------------
//
extern char			gigal_ok;
extern GIGAL_REGS	*gigal_regs;	// local register
extern CM_REGS		*cm_reg;			// CMASTER register
extern BUS1100		*bus1100;

void clr_prot_error(void);
int protocol_error(u_long lp, char *id);
int send_dmd_eot(void);
//
// ========================== Input Card ====================================
//
// -------------------------- threshold limits ------------------------------
//
#define THRH_VIN		0.5	// 0.256
#define THRHW			3			// Nachkommastellen

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
#define RES_121_7			0xAD
#define RES_121_0			0xAE
#define RES_120_3			0xAF		// 120.30ps <=(25*128)/(0.152*0xAF)
#define RES_119_6			0xB0
#define RES_118_9			0xB0
//
// ==========================================================================
//                            Programm and Configuration
// ==========================================================================
//
// -------------------------- Configuration Buffer --------------------------
//
typedef struct {
	u_char	inp_dac[4];
} INP_DB;

typedef struct {
	int			nr_f1units;
	INP_DB		inp_db[16];		// logisch von [slv_first] bis [15]
} SC_DB;

typedef struct {
	PCI_CONFIG	pconf;

	char		tst_menu;
	char		reg_menu;
	char		epr_menu;
	char		tmp_menu;

	u_int		phys_pg;
	u_int		dbuf_verify;

	int		nr_fbsc;		// number of front bus system controller, 0..C_SC_UNITS
#define C_SC_UNITS	3
	SC_DB		sc_db[C_SC_UNITS+1];

	u_int		hsdiv;
#define RESOLUTION	(HSDIV/config.hsdiv)
#define RESOLUTIONNS	((HSDIV/1000.0)/config.hsdiv)

	u_int		f1_slope;

	u_int		rout_delay;
	u_int		daq_mode;		// see MCR_DAQ of "regs.h"
#define EVF_HANDSH	0x04		// aknowledge each trigger
#define EVF_DBELLPF	0x10		// use door bell
#define EVF_ON_TP		0x20		// use test pulse as trigger
#define EVF_EXT_RAW	0x40		// extended raw mode (2 long word per hit)

	float		ef_trg_lat;		// latancy
	float		ef_trg_win;		// window
	u_long	dd_block;

	char		dataname[80];
	u_char	trig_mode;
	u_char	daq_edge;
	u_int		test_dac;
	u_long	tcount;

} CONFIG;
//
// ==========================================================================
//                            Externals
// ==========================================================================
//
// -------------------------- f1.c ----------------------------------------
//
//
// config is imported by pci.c with the type PCI_CONFIG (see pci.h)!
//
extern CONFIG	config;
#define pciconf	config.pconf

extern PLX_REGS	*plx_regs;
extern u_char		*eeprom;

#define SZ_PAGE		0x1000
#define SZ_PAGE_L		(SZ_PAGE/4)
#define DMA_PAGES		16				// max number of DMA pages
extern u_long	*dmabuf;				// DMA buffer

extern u_long	dmabuf_phy[];		// physical addresses
extern u_int	phypages;

extern u_long	*dmadesc;			// DMA descriptor buffer
extern u_long	dmadesc_phy;
void _delay_(u_long dx);

int dma_buffer(
	u_int		mode);	// 0: display and clear pages
							// 1: clear pages
//
// -------------------------- f1lib.c ---------------------------------------
//
extern u_long	kbatt;
extern u_long	kbatt_msk;
extern long		tlat,twin;
extern u_long	*robf;
extern u_int	rout_len;
extern u_long	eventcount;

int f1_hit_slope(	// 0: ok
	int	sc);	// -2: all
					// -1: master
					// else LVD frontbus controller
#define SC_ALL	-2

int set_dac(	// 0: ok
	int	sc,
	int	inp,
	int	con,
	u_int	dac);

void test_pulse(
	void);

u_long event_counter(
	int	sc);

int inp_overrun(void);	//   0: ok
								//   1: overflow Fehler erkannt
								//  -1: fataler Bus Error
int inp_mode(
	void);

int read_event(
	void);

int stop_ddma(void);
void application(void);
//
// -------------------------- daq.c -----------------------------------------
//
void data_acquisition(void);
