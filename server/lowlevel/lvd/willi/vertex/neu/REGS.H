// Borland C++ - (C) Copyright 1991, 1992 by Borland International

// Header File

//	regs.h -- Register Definitionen

// ======================================================================= ==
//										register der F1 TDC Karte                    --
// ======================================================================= ==
//
typedef struct {
	u_short	ident;			// 00
	u_short	res02;
	short		trg_lat;			// 04
	u_short	trg_win;
	u_short	ro_data;			// 08
	u_short	f1_addr;			// 0A
#define F1_ALL		8

	u_short	cr;				//	0C	2 bit
#define ICR_TMATCHING	0x01	// Trigger Matching (with time window)
#define ICR_RAW			0x02	// raw F1 data
#define ICR_TLIM			0x03	// raw F1 data with min 19.2 us hold time
#define ICR_LETRA			0x05	// F1 b0 is edge indicator
#define ICR_EXT_RAW		0x06	// extended raw mode (2 long word per hit)

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

} F1TDC_RG;						// 80
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
//	PURES				see F1TDC_RG.ctrl plus
// SYNCRES
// TESTPULSE

} F1TDC_BC;						// 40
//
// ======================================================================= ==
//										register der VERTEX Karte                    --
// ======================================================================= ==
#define	SEQ_ADCCLK				8
#define	SEQ_CS					7
#define	SEQ_REGCLK				6
#define	SEQ_REGIN				5
#define	SEQ_DTP 					4
#define	SEQ_TSTON				3
#define	SEQ_HOLD					2
#define	SEQ_RESET				1
#define	SEQ_VACLK				0
//
//-- RW register
//
typedef struct {                    // Bus FPGA
	u_short	ADR_IDENT;					// 00  00
	u_short	a1;	          			// 02  01
	u_short	ADR_ALLHIGH;				// 04  02
	u_short	ADR_ALLLOW;   				// 06  03
	u_short	ADR_FIFO;					// 08  04
	u_short	a5;			 				// 0A	 05
	u_short	ADR_CTRL;					//	0C	 06
#define	CTRL_CLEARERROR		0x0800
#define	CTRL_NIM_TRIGEN2		0x0400
#define	CTRL_NIM_TRIGEN1		0x0200
#define	CTRL_NIM_TRIGEN0	   0x0100
#define	CTRL_ADCTESTDATA		0x0080
#define	CTRL_HV_ON				0x0040
#define	CTRL_LV_ON				0x0020
#define	CTRL_HV_TRIGGER		0x0010
#define	CTRL_LV_TRIGGER		0x0008
#define	CTRL_MODEBIT_H			0x0004
#define	CTRL_MODEBIT_L			0x0001

#define	MODE_SLOW				0x0000
#define	MODE_FAST_SER			0x0004
#define	MODE_FAST_PAR			0x0005


	u_short	ADR_STAT;					// 0E  07
#define	STAT_BUSY				0x01

	u_short	ADR_SEQCTRL;				// 10	 08
#define	SEQ_CTRL_HV_HALT     0x0002
#define	SEQ_CTRL_LV_HALT     0x0001

	u_short	ADR_SEQSTATE;				// 12  09
	u_short	aa;							// 14  0A
	u_short	ADR_TRIGGEROFFSET;		// 16  0B
	u_short	ac;							// 18  0C
	u_short	ADR_CKSREG;					// 1A  0D
	u_long	ADR_SCTRL;					// 1C  0E
#define	SCTRL_DATA16			0x0000F000
#define	SCTRL_DATA15			0x0000E000
#define	SCTRL_DATA14			0x0000D000
#define	SCTRL_DATA13			0x0000C000
#define	SCTRL_DATA12			0x0000B000
#define	SCTRL_DATA11			0x0000A000
#define	SCTRL_DATA10			0x00009000
#define	SCTRL_DATA09			0x00008000
#define	SCTRL_DATA08			0x00007000
#define	SCTRL_DATA07			0x00006000
#define	SCTRL_DATA06			0x00005000
#define	SCTRL_DATA05			0x00004000
#define	SCTRL_DATA04			0x00003000
#define	SCTRL_DATA03			0x00002000
#define	SCTRL_DATA02			0x00001000
#define	SCTRL_DATA01			0x00000000

#define	SCTRL_CS_MORE			0x00000000
#define	SCTRL_CS_LAST			0x00000800

#define	SCTRL_ADC_HV			0x00000009
#define	SCTRL_ADC_LV         0x00000008
#define	SCTRL_FRONT_SD_HV		0x00000005
#define	SCTRL_FRONT_SD_LV		0x00000004
#define	SCTRL_DIGIPOTI_HV		0x00000002
#define	SCTRL_DIGIPOTI_LV		0x00000001

	u_short	ADR_SCTRLOUT;				// 20  10
	u_short	ADR_MAX_DATA_LV;			// 22  11
	u_long	ADR_SRAM_ADR;				// 24  12
#define C_HV_ADDR_OFFSET	0x80000L

	u_short	ADR_SRAM_DATA;				// 28  14
	u_short	res15;						// 2A  15
	u_short	ADR_CHIPMASK_LV;			// 2C  16
	u_short	ADR_ADCOFFSET_LV;			// 2E  17
	u_short	ADR_SEQPOL_LV;				// 30  18
	u_short	ADR_CSPOL_LV;				// 32  19
	u_long	ADR_SWREG_LV;				// 34  1A
	u_short	ADR_COUNTER_LV[4];		// 38  1C
	u_short	a20;							// 40  20
	u_short	ADR_MAX_DATA_HV;			// 42  21
	u_short	res22;						// 44  22
	u_short	res23;						// 46  23
	u_short	res24;						//	48  24
	u_short	res25;						//	4A  25
	u_short	ADR_CHIPMASK_HV;			//	4C  26
	u_short	ADR_ADCOFFSET_HV;			//	4E  27
	u_short	ADR_SEQPOL_HV;				//	50  28
	u_short	ADR_CSPOL_HV;				//	52  29
	u_long	ADR_SWREG_HV;				//	54  2A
	u_short	ADR_COUNTER_HV[4];		//	58  2C
	u_short	a30;							//	60  30
	u_short	a31;	 						//	62  31
	u_long	ADR_RAMT_ADR;				//	64  32
	u_short	ADR_RAMT_DATA;				//	68  34
	u_short	res35;						//	6A  35
	u_short	a36;							//	6C  36
	u_short	a37;							//	6E  37
	u_short	a38;							//	70  38
	u_short	a39;							//	72  39
	u_short	a3a;							//	74  3A
	u_short	a3b;							// 76  3B
	u_long	ADR_JTAG_DATA;				//	78  3C
	u_short	ADR_JTAG_CSR;				//	7C  3E
	u_short	ADR_ACTION;					// 7E	 3F
#define	R_RESETSEQ     		0x0010
#define	R_CLEARERROR   		0x0008
#define	R_CNTRSTHV				0x0004
#define	R_CNTRSTLV     		0x0002
#define	R_MRST					0x0001

} VTEX_RG;									// 80
//
//-- VERTEX broadcast register
//
typedef struct {                    // Bus FPGA
	u_short	ADR_CARD_ONL;				// 00  00
	u_short	ADR_CARD_OFFL;				// 02  01
	u_short	res02;						// 04  02
	u_short	res03;         			// 06  03
	u_short	ADR_FIFO;					// 08  04
	u_short	res05;	        			// 0A  05
	u_short	res06;						//	0C  06
	u_short	ADR_ERROR; 					// 0E  07
	u_short	res08[28];					// 10	 08
	u_long	ADR_TRIGGER;				// 48  24 fuer die Software ohne Bedeutung
	u_short	res25[25];					// 4A	 25
	u_short	ADR_ACTION;					// 7E  3F	Master Reset
} VTEX_BC;

#define	LV		1
#define	HV		0
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
	u_short  res02;
	u_short  res04;
	u_short  card_id;			// 06
	u_short  cr;				// 08
#define MCR_DAQ			0x03	// mask for DAQ mode
 #define MCR_DAQTRG		0x01	// automatische Erfassung der Input Daten
 #define MCR_DAQRAW		0x02	// Input Ueberwachung mit Interrupt Meldung
#define MCR_DAQ_TEST		0x04	// enable test pulse only
#define MCR_DAQ_HANDSH	0x08	// event synchronisation by handshake
#define MCR_NO_F1STRT	0x10	// synchron mode durch F1 Start Signal

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
	u_short	jtag_csr;		// 40
	u_short	res42;
	u_long	jtag_data;		// 44
	u_short  res48[27];
	u_short  ctrl;				// 7E
//	PURES				see F1TDC_RG.ctrl plus
// SYNCRES
// TESTPULSE
#define F1START	0x10

} COUPLER;						// 80
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
	u_short  card_id;			// 06 WO
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
//	PURES				see F1TDC_RG.ctrl plus
// SYNCRES
// TESTPULSE
//	F1START

} COUPLER_BC;					// 80
//
