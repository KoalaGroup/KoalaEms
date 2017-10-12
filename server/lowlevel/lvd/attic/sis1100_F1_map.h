/*
 * straw_map.h
 * $ZEL: sis1100_F1_map.h,v 1.1 2003/09/08 11:45:23 wuestner Exp $
 * created 21.Aug.2003
 */

#ifndef _f1straw_map_h_
#define _f1straw_map_h_

#include <sys/types.h>
#include <dev/pci/sis1100_var.h>

/*
 * F1 definitions
 */

#define T_REF			25.0		/* 40 MHz */
#define REFCLKDIV		7
#define HSDIV			(T_REF*(1<<REFCLKDIV)/0.152)
#define REFCNT			0x100		/* used for synchron mode */

#define R1_HITM			0x4000
#define R1_LETRA		0x2000
#define R1_FAKE			0x0400
#define R1_OVLAP		0x0200
#define R1_DACSTRT		0x0001

#define R7_BEINIT		0x8000

#define R15_ROSTA		0x0008	/* ring oszillator start */
#define R15_SYNC		0x0004
#define R15_COM			0x0002
#define R15_8BIT		0x0001

/* ========================== Input Card ==================================== */
/* -------------------------- threshold limits ------------------------------ */

#define THRH_MIN		-2.1
#define THRH_MAX		2.03
#define THRH_SLOPE	((THRH_MAX-THRH_MIN)/255.0)

/* ========================== CMASTER Card ================================== */
struct cm_regs {
        u_int32_t   version;
        u_int32_t   sr;                     /* Status Register */
#define SR_EVENT        0x0010L
#define SR_DATA_AVAIL   0x0020L
#define SR_ATTENT       0x0040L

        u_int32_t   cr;                     /* Control Register, 4 bit */
/* noch unbenutzt */

        u_int32_t   res1;
        u_int32_t   jtag_csr;               /* JTAG control/status */
        u_int32_t   jtag_data;              /* JTAG data */
};

/* ========================== BUS Struktur ================================== */

/* -- register einer Eingabekarte */

struct in_card {
        u_int16_t       ident;
        u_int16_t       res0;
        u_int16_t       trg_lat;
        u_int16_t       trg_win;
        u_int16_t       ro_data;
        u_int16_t       f1_addr;
#define F1_ALL          8

	u_int16_t	cr;             /* 2 bit */
#define ICR_RAW         0x01            /* ohne Zeitlimit */

	u_int16_t       sr;
#define IN_FIFO_ERR     0x1

	u_int16_t       f1_state[8];
#define WRBUSY          0x01
#define NOINIT          0x02
#define NOLOCK          0x04
/*                      0x10 F1 Hit FIFO Overflow    */
/*                      0x20 F1 Output FIFO Overflow */

	u_int16_t       f1_reg[16];
	u_int16_t	jtag_csr;
#define JT_TDI          0x001
#define JT_TMS          0x002
#define JT_ENABLE       0x100
#define JT_AUTO_CLOCK	0x200

        u_int16_t       res2;
        u_int32_t       jtag_data;

        u_int16_t       res3[4];
        u_int16_t       res4[16];
        u_int16_t       res5[7];
        u_int16_t       action;
#define MRESET          0x01            /* nur broadcast */
#define PURES           0x02
#define SYNCRES         0x04
#define TESTPULSE       0x08
#define F1START         0x10
};

/*
 * -- broadcast register der Eingabekarten einer backplane
 */

struct in_card_bc {
        u_int16_t       card_onl;
        u_int16_t       card_offl;
        u_int16_t       trg_lat;
        u_int16_t       trg_win;
        u_int16_t       ro_data;
        u_int16_t       f1_addr;
        u_int16_t       cr;             /* siehe IN_CARD */
        u_int16_t       f1_error;       /* wr: wie IN_CARD->sr */
/*?	u_int16_t       f1_state[8]; */
        u_int16_t       event_time;
        u_int16_t       res1[7];
        u_int16_t       f1_reg[16];
        u_int16_t       jtag_csr;

        u_int16_t       res2[7];
        u_int16_t       res3[16];
        u_int16_t       res4[7];
        u_int16_t       reset;          /* Master Reset */
};

/*
 * -- broadcast register der Koppelkarten am 80MB LVD BUS
 *    es gibt keine broadcast read Moeglichkeit ausser card_offl als
 *    Sonderloesung
 */
struct coupler_bc {
        u_int16_t       card_onl;
        u_int16_t       card_offl;
        u_int16_t       transp;
        u_int16_t       res0;
        u_int16_t       res1[2];
        u_int16_t       _ev_req;
        u_int16_t       res2;
        u_int16_t       _ro_req;
        u_int16_t       _f1_busy;
        u_int16_t       _f1_error;
        u_int16_t       res3[5];
        u_int16_t       f1_reg[16];
        u_int16_t       res4[31];
        u_int16_t       reset;  /* alle Koppelkarten und alle Eingabekarten */
};

/*
 * -- register einer Koppelkarte
 */

struct coupler {
        u_int16_t       ident;
        u_int16_t       set_addr;
        u_int16_t       res0;
        u_int16_t       card_id;
        u_int16_t       cr;     /* 4 bit */
#define MCR_F1STRT_SIG  0x01    /* synchron mode durch F1 Start Signal */
#define MCR_DAQ_MODE    0x02    /* automatische Erfassung der Input Daten */
#define MCR_SCAN_MODE   0x04    /* Input Ueberwachung mit Interrupt Meldung */

        u_int16_t  sr;          /* siehe f1_state Definitionen */
/*                      0x008   F1 Trigger FIFO Overflow */
#define SC_DATA_AV      0x040   /* FIFO Data available */
#define SC_MTRIGGER     0x080   /* Main Trigger Time available */
/*                      0x100   FIFO Data Error, no double word present */

        u_int32_t       event_nr;
        u_int16_t       ro_data;
        u_int16_t       res1;
        u_int32_t       event_data;
        u_int16_t       res2[4];
        u_int16_t       f1_reg[16];
        u_int16_t       jtag_csr;
        u_int16_t       res3;
        u_int32_t       jtag_data;
        u_int16_t       res4[27];
        u_int16_t       action;
#define TESTPULSE       0x08
};

/*
 *  Adressmapping einer optical Koppelkarte
 */

struct bus1100 {
        struct in_card     in_card[16];   /* eigene Eingabekarten (backplane) */
        struct coupler     coupler[16];   /* angeschlossene Koppelkarten (80MB LVD BUS) */
        struct in_card     c_in_card[16]; /* Eingabekarten einer selektierten Koppelkarte */
        struct coupler     l_coupler;     /* lokale Koppelkarte */
        struct in_card_bc  in_card_bc;    /* broadcast Bereich der eigenen Eingabekarten */
        struct coupler_bc  coupler_bc;    /* broadcast Bereich der angeschlossenen Koppelkarten */
        struct in_card_bc  c_in_card_bc;  /* broadcast Bereich der Eingabekarten
                                             einer selektierten Koppelkarte */
};
#ifdef DAQMODE
struct f1data {
    u_int16_t coarse:5;
    u_int16_t channel:6;
    u_int16_t board:4;
    u_int16_t marker:1;
    u_int16_t fine;
};
struct f1data_x {
    u_int16_t coarse:5;
    u_int16_t channel:10;
    u_int16_t marker:1;
    u_int16_t fine;
};
#else
/* XXX works only with little endian */
struct f1data {
    u_int16_t fine;
    u_int16_t coarse:5;
    u_int16_t channel:6;
    u_int16_t board:4;
    u_int16_t marker:1;
};
struct f1data_x {
    u_int16_t fine;
    u_int16_t coarse:5;
    u_int16_t channel:10;
    u_int16_t marker:1;
};
#endif

#endif
