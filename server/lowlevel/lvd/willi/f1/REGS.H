// Borland C++ - (C) Copyright 1991, 1992 by Borland International

// Header File

//	regs.h -- Register Definitionen

// ======================================================================= ==
//										register einer Eingabekarte                  --
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
#define ICR_RAW			0x01	// ohne Zeitfenster
#define ICR_TLIM			0x03	// ohne Zeitfenster aber mit Zeitlimit
#define ICR_LETRA			0x04	// F1 b0 nicht veraendern
#define ICR_EXT_RAW		0x08	// extended raw mode (2 long word per hit)

	u_short	sr;				// 0E
#define IN_FIFO_ERR	0x1	// no double word

	u_short	f1_state[8];	// 10
#define WRBUSY		0x01
#define NOINIT		0x02
#define NOLOCK		0x04
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
#define MRESET		0x01		// nur broadcast
#define PURES		0x02
#define SYNCRES	0x04
#define TESTPULSE	0x08
#define F1START	0x10

} IN_CARD;						// 80
//
// -------------------------- broadcast register der Eingabekarten ------- --
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
	u_short	hitofl;			// 50	cleat
	u_short	res52[15];
	u_short	res70[7];
	u_short	ctrl;				// 7E	value
} IN_CARD_BC;					// 40
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
} COUPLER;						// 80
//
// -------------------------- broadcast register der Controller Karten --- --
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

} COUPLER_BC;					// 80
//
