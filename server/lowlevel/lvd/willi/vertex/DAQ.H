// Borland C++ - (C) Copyright 1991, 1992 by Borland International

// Header File

//	daq.h -- optical control mit SIS1100

#include "sis1100.h"
#include "jtag.h"
#include "regs.h"
//
#define SC_M
#define LOW_SC_UNIT			-1
#define CLR_PROT_ERROR()	clr_prot_error()
#define BUS_ERROR				protocol_error(-1, 0)
#define TST_BUS_ERROR		(gigal_regs->sr &SR_PROT_ERROR)
#define CLR_BUS_ERROR		clr_prot_error()
#define INP_UNIT(i)	(sc_conf->unit == -1) ? bus1100->in_card +i		\
														 : bus1100->c_in_card +i;
#define SC_UNIT(i)				bus1100->coupler +i
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
	F1TDC_RG		in_card[16];	// 0000     eigene Eingabekarten (backplane)
	COUPLER     coupler[16];	// 0800     angeschlossene Koppelkarten (80MB LVD BUS)
	F1TDC_RG		c_in_card[16];	// 1000     Eingabekarten einer selektierten Koppelkarte
	COUPLER     l_coupler;		// 1800     lokale Koppelkarte
	F1TDC_BC		in_card_bc;		// 1880     broadcast Bereich der eigenen Eingabekarten
	COUPLER_BC  coupler_bc;		// 1900     broadcast Bereich der angeschlossenen Koppelkarten
	F1TDC_BC		c_in_card_bc;	// 1980     broadcast Bereich der Eingabekarten
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
extern char			gigal_ok;
extern GIGAL_REGS	*gigal_regs;	// local register
extern CM_REGS		*cm_reg;			// CMASTER register
extern BUS1100		*bus1100;

void clr_prot_error(void);
int protocol_error(u_long lp, char *id);
int send_dmd_eot(void);
//
// -------------------------- ggladdon.c ------------------------------------
//
int addon_dial(void);
//
#include "daqx.h"
