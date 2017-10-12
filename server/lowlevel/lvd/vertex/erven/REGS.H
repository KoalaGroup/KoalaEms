
// Borland C++ - (C) Copyright 1991, 1992 by Borland International

// Header File

//	regs.h -- Register Definitionen

// ======================================================================= ==
//										register der F1 TDC Karte                    --
// ======================================================================= ==
//
typedef struct {
	u_short	ident;			// 00
	u_short	serial;			// 02
	short		trg_lat;			// 04
	u_short	trg_win;
	u_short	ro_data;			// 08
	u_short	f1_addr;			// 0A
#define F1_ALL		8

	u_short	cr;				//	0C	2 bit
#define ICR_TMATCHING	0x01	// Trigger Matching (with time window)
#define ICR_RAW			0x02	// raw F1 data with min 19.2 us hold time
#define ICR_LETRA			0x04	// F1 b0 is edge indicator
#define ICR_EXT_RAW		0x08	// extended raw mode (2 long word per hit)

	u_short	sr;				// 0E
#define IN_FIFO_ERR	0x1	// no double word

	u_short	f1_state[8];	// 10
#define WRBUSY		0x01
#define NOINIT		0x02
#define NOLOCK		0x04
#define PERM_HOFL	0x08	// permanent Hit Overflow
#define HOFL		0x10	// F1 Hit FIFO Overflow
#define OOFL		0x20	// F1 Output FIFO Overflow
#define SEQERR		0x40	// F1 Daten falsch (Reihenfolge)
#define IOFL		0x80	// FPGA Input FIFO Overflow
#define F1_INPERR	(IOFL|SEQERR|OOFL|HOFL)

	u_short	f1_reg[16];		// 20
	u_short	jtag_csr;		// 40
#define JT_TDI				0x001
#define JT_TMS				0x002
#define JT_TCK				0x004
#define JT_TDO				0x008
#define JT_ENABLE			0x100
#define JT_AUTO_CLOCK	0x200
#define JT_SLOW_CLOCK	0x400

	u_short	res42;
	u_long	jtag_data;		// 44
	u_long	res48;
	u_short	f1_range;		// 4C
	u_short	res4E;
	u_short	hitofl[4];		// 50
	u_short	res58[12];
	u_short	res70[7];
	u_short	ctrl;				// 7E
#define PURES		0x02
#define SYNCRES	0x04
#define TESTPULSE	0x08

} TDCF1_RG;						// 80
//
// -------------------------- F1 TDC broadcast register ------------------ --
//
typedef struct {				//		write			read
	u_short	card_onl;		// 00 address		online units
	u_short	card_offl;		//						any offline unit
	short		trg_lat;			// 04	value
	u_short	trg_win;			//		value
	u_short	ro_data;			// 08					data available
	u_short	f1_addr;			//		value
	u_short	cr;				//	0C	value
	u_short	f1_error;		//	0E	sel clr		F1 error mask
	u_short	f1_state[8];	// 10	sel clr
	u_short	f1_reg[16];		// 20	value
	u_short	jtag_csr;		// 40	value

	u_short	res42[3];
	u_long	trigger;			// 48 fuer die Software ohne Bedeutung
	u_short	f1_range;		// 4C	value
	u_short	res4E;
	u_short	hitofl;			// 50	clear
	u_short	res52[15];
	u_short	res70[7];
	u_short	ctrl;				// 7E	value
#define MRESET		0x01
//	PURES				see TDCF1_RG.ctrl plus
// SYNCRES
// TESTPULSE
#define TSTART		0x80

} TDCF1_BC;						// 80
//
// ======================================================================= ==
//										register structure of the GPX TDC card       --
//										                                             --
// ======================================================================= ==
//
typedef struct {
	u_short	ident;			// 00
	u_short	serial;			// 02
	short		trg_lat;			// 04
	u_short	trg_win;
	u_short	ro_data;			// 08
	u_short	res0A;
	u_short	cr;				//	0C	4 bit RW
//define ICR_TMATCHING	0x01	// Trigger Matching (with time window)
//define ICR_RAW			0x02	// raw F1 data with min 19.2 us hold time
//define ICR_LETRA		0x04	// F1 b0 is edge indicator
//define ICR_EXT_RAW		0x08	// extended raw mode (2 long word per hit)
#define ICR_NO_EIND		0x10	// no error indication

	u_short	sr;				// 0E
// b0:

	u_short	gpx_int_err;	// 10
#define GPX_ERRFL		0xFF

	u_short	gpx_emptfl;		// 12
	u_short	gpx_dcm_shift;	// 14
	u_short	res12[5];		// 16
	u_long	gpx_data;		// 20
	u_short	gpx_seladdr;	// 24
	u_short	gpx_dac;			// 26
	u_short	res26[4+1*8];	// 28
	u_short	jtag_csr;		// 40
	u_short	res42;
	u_long	jtag_data;		// 44
	u_short	res48[2];
	u_long	gpx_range;		// 4C
	u_short	res50[2*8+7];
	u_short	ctrl;				// 7E not used

} TDCGPX_RG;					// 80
//
// -------------------------- GPX broadcast register --------------------- --
//
typedef struct {				//		write			read
	u_short	card_onl;		// 00 address		online units
	u_short	card_offl;		//						b0: any offline unit
	u_short	res04[2];
	u_short	ro_data;			// 08					data available
	u_short	res0A[2];
	u_short	any_error;		//	0E					gen error
	u_short	res10[3*8+4];	// 10
	u_short	busy;				// 48 trigger		=> used by system controller
	u_short	res4A[3+2*8+7];// 4A
	u_short	ctrl;				// 7E
// b0: sgl shot, master reset

} TDCGPX_BC;					// 80
//
// ======================================================================= ==
//										register der synch Master Karte              --
// ======================================================================= ==
//
typedef struct {
	u_short	ident;			// 00
	u_short	serial;			// 02
	u_short	sr2;				// 04
	u_short	res06;
	u_short	ro_data;			// 08
	u_short	res0A;			// 0A
	u_short	sm_cr;			//	0C
#define ST_RING		0x0001
#define ST_GO			0x0002
#define ST_TAW_ENA	0x0004
#define B_ST_AUX0		4
#define ST_AUX_MASK	0x00F0
#define ST_T4_UTIL	0x0100

	u_short	sm_sr;			// 0E
#define CS_GO_RING	0x0010
#define CS_INH			0x0080
#define CS_EOC			0x1000
#define CS_SI_RING	0x2000
#define CS_TDT_RING	0x8000

	u_short	trg_inh;			// 10
	u_short	tlg_inh;			// 12
	u_short	trg_inp;			// 14
	u_short	trg_acc;			// 16
#define CS_TRIGGER	0x000F

	u_short	res18;			// 18
	u_short	sm_fclr;			// 1A
	u_short	res1C[2];		// 1C
	u_short	pwidth[9];		// 20
	u_short	setpoti;			// 32
	u_long	sm_tmc;			// 34
	u_short	res38[2];		// 38
	u_long	sm_tdt;			// 3C
	u_short	jtag_csr;		// 40
	u_short	res42;
	u_long	jtag_data;		// 44
	u_short	res48;
	u_short	sm_fcat;			// 4A
	u_long	sm_evc;			// 4C
	u_short	res50[2*8+7];
	u_short	sm_ctrl;			// 7E
#define RST_TRG	0x02

} SYMST_RG;						// 80
//
// -------------------------- sync master broadcast register ------------- --
//
typedef struct {				//		write			read
	u_short	card_onl;		// 00 address		online units
	u_short	card_offl;		//						any offline unit
	u_short	res04[2];
	u_short	ro_data;			// 08					data available
	u_short	res0A[2];
	u_short	f1_error;		//	0E
	u_short	res10[3*8+4];	// 10
	u_short	busy;				// 48	trigger		=> used by system controller
	u_short	res4A[3+2*8+7];// 4A
	u_short	ctrl;				// 7E
//#define TSTART		0x80

} SYMST_BC;						// 80
//
// ======================================================================= ==
//										register der synch slave Karte               --
// ======================================================================= ==
//
typedef struct {
	u_short	ident;			// 00
	u_short	serial;			// 02
	u_short	bsy_tmo;			// 04
	u_short	bsy_state;		// 06
	u_short	ro_data;			// 08
	u_short	res0A;			// 0A
	u_short	cr;				//	0C
#define TS_RUN		0x100

	u_short	sr;				// 0E

	u_short	trg_in;			// 10
	u_short	sram_addr;		// 12
	u_short	sram_data;		// 14
	u_short	res16[5+2*8];	// 16
	u_short	jtag_csr;		// 40
	u_short	res42;
	u_long	jtag_data;		// 44
	u_short	res48[4+2*8+7];
	u_short	ctrl;				// 7E

} SYSLV_RG;						// 80
//
// -------------------------- sync slave broadcast register -------------- --
//
typedef struct {				//		write			read
	u_short	card_onl;		// 00 address		online units
	u_short	card_offl;		//						any offline unit
	u_short	res04[2];
	u_short	ro_data;			// 08					data available
	u_short	res0A[2];
	u_short	f1_error;		//	0E
	u_short	res10[3*8+4];	// 10
	u_short	busy;				// 48	trigger		=> used by system controller
	u_short	res4A[3+2*8+7];// 4A
	u_short	ctrl;				// 7E

} SYSLV_BC;						// 80
//
// ======================================================================= ==
//										register der VERTEX Karte                    --
// ======================================================================= ==
#define	SEQ_VACLK				0
#define	SEQ_FRAME				1
#define	SEQ_HOLD					2
#define	SEQ_TSTON				3
#define	SEQ_CS					4
#define	SEQ_HOLD_B				5
#define	SEQ_RESET				6
#define	SEQ_DTP 					7
#define	SEQ_BUSY					8
//
//-- LV/HV register
//
typedef struct {
	u_long	ADR_SWREG;				// 20
	u_short	ADR_COUNTER[2];		// 24
	u_short	ADR_LP_COUNTER[2];	// 28
	u_short	ADR_CLK_DELAY;			// 2C
	u_short	res2E;					// 2E
	u_short	ADR_NR_CHAN;			//	30
	u_short	ADR_NOISE_THR;			//	32
	u_short	ADR_COMVAL;				//	34
	u_short	ADR_COMCNT;				//	36
	u_short	ADR_POTI;				//	38
	u_short	ADR_DAC;					//	3A
	u_short	ADR_REG_CR;				//	3C
	u_short	ADR_REG;					//	3E
	u_short	ADR_ADC_VALUE;			//	40
} VTX_GRP;								// 42

typedef struct {						// Bus FPGA
	u_short	ADR_IDENT;				// 00  00
	u_short	ADR_SERIAL;				// 02
	u_short	ADR_SEQ_CSR;			// 04
#define SEQ_CSR_LV_HALT	0x0001
#define SEQ_CSR_HV_HALT	0x0100

	u_short	ADR_ACLK_PAR;			// 06
	u_short	ADR_FIFO;				// 08  04
	u_short	res0A;					// 0A	 05
	u_short	ADR_CR;					//	0C	 06
#define	VCR_RUN			0x0001
#define	VCR_RAW			0x0002
#define	VCR_VERB			0x0004
#define	VCR_TM_MODE		0x0008

#define	VCR_IH_LV	   0x0010
#define	VCR_IH_HV		0x0020
#define	VCR_IH_TRG		0x0040
#define	VCR_VA_TRG		0x0080
#define	VCR_MON			0x0100

	u_short	ADR_SR;					// 0E  07

	u_long	ADR_SRAM_ADR;			// 10
#define C_HV_SRAM_OFFSET	0x080000L
#define C_HV_TRAM_OFFSET	0x100000L

	u_short	ADR_SRAM_DATA;			// 14
	u_short	res16;					// 16
	u_long	ADR_RAMT_ADR;			//	18
#define BLOCK1_HV				0x100000L

	u_short	ADR_RAMT_DATA;			//	1C
	u_short	res1E;					//	1E
	VTX_GRP	lv;						// 20
	u_short	res42[5];				// 42
	u_short	ADR_SCALING;			// 4C
	u_short	ADR_DIG_TST;			// 4E
	VTX_GRP	hv;						// 50
	u_short	res72[3];				// 72
	u_long	ADR_JTAG_DATA;			//	78  3C
	u_short	ADR_JTAG_CSR;			//	7C  3E
	u_short	ADR_ACTION;				// 7E	 3F
//	#define MRESET		0x01
//	#define PURES		0x02
//	#define SYNCRES	0x04
//	#define TESTPULSE	0x08

#define	R_CNTRSTLV     		0x0100
#define	R_CNTRSTHV				0x0200
#define	R_CLEARERROR   		0x0400
#define	R_RESETSEQ     		0x0800

} VTEX_RG;								// 80
//
//-- VERTEX broadcast register
//
typedef struct {						// Bus FPGA
	u_short	ADR_CARD_ONL;			// 00  00
	u_short	ADR_CARD_OFFL;			// 02  01
	u_short	res02;					// 04  02
	u_short	res03;         		// 06  03
	u_short	ADR_FIFO;				// 08  04
	u_short	res05;	        		// 0A  05
	u_short	res06;					//	0C  06
	u_short	ADR_ERROR; 				// 0E  07
	u_short	res08[28];				// 10	 08
	u_short	ADR_TRIGGER_L;			// 48  24	=> used by system controller
	u_short	ADR_TRIGGER_H;
	u_short	res25[25];				// 4A	 25
	u_short	ADR_ACTION;				// 7E  3F	Master Reset
} VTEX_BC;
//
// ======================================================================= ==
//										register structure of the fast QDC test card --
//										developed by Pawel Marciniewski              --
// ======================================================================= ==
//
typedef struct {
	u_short	ident;			// 00
	u_short	serial;			// 02
	u_short	mean_tol;		// 04
	u_short	trig_level;		// 06
	u_short	ro_data;			// 08
	u_short	res0A;			// 0A
	u_short	cr;				//	0C	4 bit RW
#define Q_ENA			0x001	// gen. enable
#define Q_LTRIG		0x002	// local on board trigger, NIM input
#define Q_LEVTRG		0x004	// or data level
#define QFT_CLKPOL	0x008	// DAC clock polarity
#define QFT_ADCPOL	0x040	// inverse ADC data
#define QFT_RAW		0x080	// insert ADC data
#define QFT_SGRD		0x100	// single gradient

	u_short	sr;				// 0E
// b0:	FIFO read error, WR: selective clear
// b1: NIM input signal

	u_short	res10[2];		// 10
	u_short	mean_level;		// 14
	u_short	res16;			// 16
	u_short	mean_noise;		// 18
	u_short	res1A;			// 1A
	short		latency;			// 1C
	u_short	window;			// 1E
	u_short	res20[2*8];		// 20
	u_short	jtag_csr;		// 40
	u_short	res42;
	u_long	jtag_data;		// 44
	u_short	res48[4+2*8+7];
	u_short	ctrl;				// 7E not used

} FQDCT_RG;						// 80
//
// -------------------------- QDC broadcast register --------------------- --
//
typedef struct {				//		write			read
	u_short	card_onl;		// 00 address		online units
	u_short	card_offl;		//						b0: any offline unit
	u_short	res04[2];
	u_short	ro_data;			// 08					data available
	u_short	res0A[2];
	u_short	any_error;		//	0E not used
	u_short	res10[3*8+4];	// 10
	u_short	busy;				// 48	trigger		=> used by system controller
	u_short	res4A[3+2*8+7];// 4A
	u_short	ctrl;				// 7E
// b0: sgl shot, master reset

} FQDCT_BC;						// 80
//
// ======================================================================= ==
//										register structure of the fast QDC card      --
//										16 channel, developed by Pawel Marciniewski  --
// ======================================================================= ==
//
typedef struct {
	u_short	ident;			// 00
	u_short	serial;			// 02
	short		latency;			// 04
	u_short	window;			// 06
	u_short	ro_data;			// 08
	u_short	res0A;			// 0A
	u_short	cr;				//	0C	4 bit RW
#define Q_ENA			0x001	// gen. enable
#define Q_LTRIG		0x002	// local on board trigger, NIM input
#define Q_LEVTRG		0x004	// or data level
#define Q_VERBOSE		0x008	// all analysed data
#define Q_ADCPOL		0x010	// inverse ADC data
#define Q_SGRD			0x020	// single gradient
#define QF_TESTON		0x040	// test pulse on
#define QF_PWROFF		0x080	// ADC power off

	u_short	sr;				// 0E
// b0: FIFO read error, WR: selective clear
// b1: ADC power fail

	u_short	trig_level;		// 10
	u_short	anal_ctrl;		// 12
	u_short	inp_ovr;			// 14
	u_short	dcm_shift;		// 16
	u_short	cha_inhibit;	// 18
	u_short	cha_raw;			// 1A
	u_short	iw_start;		// 1C
	u_short	iw_length;		// 1E
	u_short	mean_noise[16];// 20
#define q_thresh	mean_noise

	u_short	jtag_csr;		// 40
	u_short	res42;
	u_long	jtag_data;		// 44
	u_short	res48[2];
	u_short	i_length;		// 4C
	u_short	f1_range;		// 4E	old version
	u_long	gpx_range;		// 50
	u_short	res54[6];
	u_short	mean_level[15];// 60
	u_short	ctrl;				// 7E not used

} FQDC_RG;						// 80
//
// -------------------------- QDC broadcast register --------------------- --
//
typedef struct {				//		write			read
	u_short	card_onl;		// 00 address		online units
	u_short	card_offl;		//						b0: any offline unit
	u_short	res04[2];
	u_short	ro_data;			// 08					data available
	u_short	res0A[2];
	u_short	any_error;		//	0E not used
	u_short	res10[3*8+4];	// 10
	u_short	busy;				// 48	trigger		=> used by system controller
	u_short	res4A[3+2*8+7];// 4A
	u_short	ctrl;				// 7E
// b0: sgl shot, master reset

} FQDC_BC;						// 80
//
// ======================================================================= ==
//										register structure of the slow QDC card      --
//										test piggy pack with VERTEX base card        --
// ======================================================================= ==
//
typedef struct {
	u_short	ident;			// 00
	u_short	serial;			// 02
	u_short	mean_tol;		// 04
	u_short	trig_level;		// 06
	u_short	ro_data;			// 08
	u_short	res0A;			// 0A
	u_short	cr;				//	0C	8 bit RW
#define Q_ENA			0x001	// gen. enable
#define Q_LTRIG		0x002	// local on board trigger, NIM input
#define Q_LEVTRG		0x004	// or data level
#define QST_CLKPOL	0x008	// DAC clock polarity
#define QST_DIS0		0x010	// disable channel 0
#define QST_DIS1		0x020	// disable channel 1
#define QST_ADCPOL	0x040	// inverse ADC data
#define QST_RAW		0x080	// insert ADC data
#define QST_SGRD		0x100	// single gradient

	u_short	sr;				// 0E
// b0:	FIFO read error, WR: selective clear
// b1: NIM input signal

	u_short	dac[2];			// 10
	u_short	mean_level[2];	// 14
	u_short	mean_noise[2];	// 18
	short		latency;			// 1C
	u_short	window;			// 1E
	u_long	qram_addr;		// 20
	u_short	qram_data;		// 24
	u_short	res26;			// 26
	u_long	tram_addr;		// 28
	u_short	tram_data;		// 2C
	u_short	res2E[1+8];		// 2E
	u_short	jtag_csr;		// 40
	u_short	res42;
	u_long	jtag_data;		// 44
	u_short	res48[4+2*8+7];
	u_short	ctrl;				// 7E not used

} SQDCT_RG;						// 80
//
// -------------------------- QDC broadcast register --------------------- --
//
typedef struct {				//		write			read
	u_short	card_onl;		// 00 address		online units
	u_short	card_offl;		//						b0: any offline unit
	u_short	res04[2];
	u_short	ro_data;			// 08					data available
	u_short	res0A[2];
	u_short	any_error;		//	0E not used
	u_short	res10[3*8+4];	// 10
	u_short	busy;				// 48	trigger		=> used by system controller
	u_short	res4A[3+2*8+7];// 4A
	u_short	ctrl;				// 7E
// b0: sgl shot, master reset

} SQDCT_BC;						// 80
//
// ======================================================================= ==
//										register structure of the slow QDC card      --
//										16 channel                                   --
// ======================================================================= ==
//
typedef struct {
	u_short	ident;			// 00
	u_short	serial;			// 02
	short		latency;			// 04
	u_short	window;			// 06
	u_short	ro_data;			// 08
	u_short	res0A;			// 0A
	u_short	cr;				//	0C	4 bit RW
#define Q_ENA			0x001	// gen. enable
#define Q_LTRIG		0x002	// local on board trigger, NIM input
#define Q_LEVTRG		0x004	// or data level
#define Q_VERBOSE		0x008	// all analysed data
#define Q_ADCPOL		0x010	// inverse ADC data
#define Q_SGRD			0x020	// single gradient

	u_short	sr;				// 0E
// b0:	FIFO read error, WR: selective clear

	u_short	trig_level;		// 10
	u_short	anal_ctrl;		// 12
	u_short	inp_ovr;			// 14
	u_short	dcm_shift;		// 16
	u_short	cha_inhibit;	// 18
	u_short	cha_raw;			// 1A
	u_short	iw_start;		// 1C
	u_short	iw_length;		// 1E
	u_short	mean_noise[16];// 20
#define q_thresh	mean_noise

	u_short	jtag_csr;		// 40
	u_short	res42;
	u_long	jtag_data;		// 44
	u_short	res48[2];
	u_short	i_length;		// 4C
	u_short	f1_range;		// 4E	old version
	u_long	gpx_range;		// 50
	u_short	res54[6];
	u_short	mean_level[15];// 60
#define dac_data	mean_level

	u_short	ctrl;				// 7E not used

} SQDC_RG;						// 80
//
// -------------------------- QDC broadcast register --------------------- --
//
typedef struct {				//		write			read
	u_short	card_onl;		// 00 address		online units
	u_short	card_offl;		//						b0: any offline unit
	u_short	res04[2];
	u_short	ro_data;			// 08					data available
	u_short	res0A[2];
	u_short	any_error;		//	0E not used
	u_short	res10[3*8+4];	// 10
	u_short	busy;				// 48 trigger		=> used by system controller
	u_short	res4A[3+2*8+7];// 4A
	u_short	ctrl;				// 7E
// b0: sgl shot, master reset

} SQDC_BC;						// 80
//
// ======================================================================= ==
//										JTAG analysing tool                          --
// ======================================================================= ==
//
typedef struct {
	u_short	ident;			// 00
	u_short	res02;			// 02
	u_long	timer;			// 04
	u_long	ro_data;			// 08
	u_short	cr;				//	0C	4 bit RW
#define Q_ENA			0x001	// gen. enable

	u_short	sr;				// 0E
// b0:	FIFO read error, WR: selective clear

	u_short	tck_count;		// 10
	u_short	res12;			// 12
	u_short	acc_state;		// 14
	u_short	res16[5];		// 16
	u_short	res20[6*8];		// 20

} JTAG_RG;						// 80
//
// ======================================================================= ==
//										register einer Controller Karte              --
// ======================================================================= ==
//
// -------------------------- register einer Controller Karte ------------ --
//
//	Diese Struktur gilt auch fuer die lokalen Register des Master Controllers
//
typedef struct {
	u_short  ident;			// 00
	u_short	serial;			// 02
	u_short  res04;
	u_short  res06;			// 06
	u_short  cr;				// 08
//#define MCR_DAQ				0x03	// mask for DAQ mode
// #define MCR_DAQTRG		0x01	// automatische Erfassung der Input Daten
// #define MCR_DAQRAW		0x02	// Input Ueberwachung mit Interrupt Meldung
//#define MCR_DAQ_TEST		0x04	// enable test pulse only
//#define MCR_DAQ_HANDSH	0x08	// event synchronisation by handshake
//#define MCR_LOC_EVC		0x10	// use local event counter
//#define MCR_NO_F1STRT		0x40	// synchron mode durch F1 Start Signal

	u_short  sr;				//	0A	siehe auch f1_state Definitionen
//							0x0008	// F1 Trigger FIFO Overflow
#define SC_F1_ERR		0x0040	// F1 error (no lock) Summenfehler Inputkarten
#define SC_TRG_LOST	0x0080	// Trigger overrun
#define SC_DATA_AV	0x0100	// FIFO Data available
#define SC_EVENT		0x0200	// Main Trigger Time available
#define SC_F1_DERR	0x8000	// F1 data error

	u_long	event_nr;		// 0C
	u_short  ro_data;			// 10	Blocktransfer aus Event FIFO
	u_short	res12;
	u_short	ro_delay;		// 14	Delay nach Event fuer ReadOut Input Cards
	u_short	res16;
	u_long	trg_data;		// 18 Trigger Time Stamp
	u_long	ev_info;			// 1C Adresse und Laenge des letzten Trigger Events
	u_short  f1_reg[16];		// 20
	u_short	x_jtag_csr;		// 40
	u_short	res42;
	u_long	x_jtag_data;	// 44
	u_short  res48[27];
	u_short  ctrl;				// 7E
//	PURES				see TDCF1_RG.ctrl plus
// SYNCRES
// TESTPULSE
#define F1START	0x10

} SYSCONF1_RG;					// 80
//
// -------------------------- broadcast register der Controller Karten --- --
//										nur Front Bus Controller
//
//	Die scan Moeglichkeit am Front Bus ist eingeschraenkt.
//		Die erste Version hat keine scan Moeglichkeit, alle Infos erscheinen auf Bit0
//		die zweite Version kann die ersten 4 Adressen scannen, die restlichen
//			erscheinen alle auf Bit 4
//
typedef struct {
	u_short  card_onl;		// 00	RW
	u_short  card_offl;		//	02	RO
	u_short  transp;			// 04	WO
	u_short  res06;			// 06 WO
	u_short  cr;				// 08 WO
	u_short  sr;				//	0A	WR, RD:f1_err -> any DAQ F1 error
#define f1_err	sr

	u_long	event_nr;		// 0C WO
	u_short  ro_data;			// 10	RO Data available
	u_short	fifo_pf;			//	12	RO	Event FIFO partial full
	u_short	ro_delay;		// 14	WO Delay nach Event fuer ReadOut Input Cards
	u_short	res16;
	u_short  res18[4];
	u_short  f1_reg[16];		// 20	WO
	u_short	jtag_csr;		// 40	WO
	u_short	res42;
	u_short  res44[29];
	u_short  ctrl;				// 7E WO
//	MRESET
//	PURES				see TDCF1_RG.ctrl plus
// SYNCRES
// TESTPULSE
//	F1START

} COUPLER_BC;					// 80
//
// ======================================================================= ==
//										register of the system controller            --
// ======================================================================= ==
//
//	Diese Struktur gilt auch fuer die lokalen Register des System Controllers mit GPX
//
typedef struct {
	u_short  ident;			// 00
	u_short  serial;			// 02
	u_short  res04;
	u_short  res06;			// 06
	u_short  cr;				// 08
#define MCR_DAQ			0x03	// mask for DAQ mode
 #define MCR_DAQTRG		0x01	// automatische Erfassung der Input Daten
 #define MCR_DAQRAW		0x02	// Input Ueberwachung mit Interrupt Meldung
#define MCR_DAQ_TEST		0x04	// enable test pulse only
#define MCR_DAQ_HANDSH	0x08	// event synchronisation by handshake
#define MCR_LOC_EVC		0x10	// use local event counter
#define MCR_F1TIME		0x20	// F1 time format
#define MCR_NO_F1STRT	0x40	// synchron mode durch F1 Start Signal

	u_short  sr;				//	0A	siehe auch f1_state Definitionen
//							0x0008	// F1 Trigger FIFO Overflow
#define SC_F1_ERR		0x0040	// F1 error (no lock) Summenfehler Inputkarten
#define SC_TRG_LOST	0x0080	// Trigger overrun
#define SC_DATA_AV	0x0100	// FIFO Data available
#define SC_EVENT		0x0200	// Main Trigger Time available
#define SC_F1_DERR	0x8000	// F1 data error

	u_long	event_nr;		// 0C
	u_short  ro_data;			// 10	Blocktransfer aus Event FIFO
	u_short	res12;
	u_short	ro_delay;		// 14	Delay nach Event fuer ReadOut Input Cards
	u_short	res16;
	u_long	trg_data;		// 18 Trigger Time Stamp
	u_long	ev_info;			// 1C Adresse und Laenge des letzten Trigger Events
	u_short	gpx_reg;			// 20
	u_short	res22;
	u_long	gpx_data;		// 24
	u_long	gpx_range;		// 28
	u_short  res2C[10];		// 2C
	u_short	jtag_csr;		// 40
	u_short	res42;
	u_long	jtag_data;		// 44
	u_short  res48[27];
	u_short  ctrl;				// 7E
//	PURES				see TDCF1_RG.ctrl plus
// SYNCRES
// TESTPULSE
#define F1START	0x10

} SYSCON_RG;					// 80
//
