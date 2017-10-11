/*
 * lvd_map.h
 * $ZEL: lvd_map.h,v 1.36 2013/09/24 14:08:03 wuestner Exp $
 * created 21.Aug.2003 (as f1_map.h)
 */

#ifndef _lvd_map_h_
#define _lvd_map_h_

#include <sys/types.h>
#include "dev/pci/sis1100_var.h"
#include "dev/pci/zellvd_map.h"
#include "lvd_f1_map.h"
#include "lvd_vertex_map.h"
#include "lvd_gpx_map.h"
#include "lvd_qdc_map.h"
#include "lvd_sync_master_map.h"
#include "lvd_sync_output_map.h"
#include "lvd_trig_map.h"


/* to escape uncertainty: VERTEX univ.= 0x39 and vertex proto=0x38  types:
 * instead of  #define LVD_HWtyp(id) (((id))&0xf8)
*/
#if 0
#define LVD_HWtyp(id) (((((id))&0xf8) != 0x38) ? (((id))&0xf8) : (((id))&0xf9))
#else
#define LVD_HWtyp(id) \
    ((((id)&0xf8)==0x38)||(((id)&0xf8)==0x78) ? ((id)&0xf9) : ((id)&0xf8))
#endif
#define LVD_HWver(id) ((id)&0x7)
#define LVD_FWver(id) (((id)>>8)&0xff)
#define LVD_FWverH(id) (((id)>>12)&0xf)
#define LVD_FWverL(id) (((id)>>8)&0xf)

enum lvd_cardid {
    LVD_CARDID_CONTROLLER_GPX   = 0x00,
    LVD_CARDID_CONTROLLER_F1GPX = 0x08,
    LVD_CARDID_CONTROLLER_F1    = 0x10,
#if 0
    LVD_CARDID_SLAVE_CONTROLLER = 0x20,
#endif
    LVD_CARDID_F1               = 0x30,
    LVD_CARDID_VERTEX           = 0x40,
    LVD_CARDID_VERTEXM3         = 0x48,
    LVD_CARDID_VERTEXUN         = 0x39, /* hw provides this! */
    LVD_CARDID_SYNCH_MASTER     = 0x50,
    LVD_CARDID_SYNCH_MASTER2    = 0x58,
    LVD_CARDID_SYNCH_OUTPUT     = 0x60,
    LVD_CARDID_TRIGGER          = 0x68,
    LVD_CARDID_FQDC             = 0x70, /* 160 MHz */
    LVD_CARDID_VFQDC            = 0x78, /* 240 MHz */
    LVD_CARDID_VFQDC_200        = 0x79, /* 200 MHz */
    LVD_CARDID_GPX              = 0x80,
    LVD_CARDID_SQDC             = 0x90, /*  80 MHz */
};

/* ========================== CMASTER Card ================================== */

/* maximum size of the data FIFO of all controllers */
#define LVD_FIFO_SIZE  0x00400000
#define LVD_FIFO_MASK (LVD_FIFO_SIZE-1)
#define LVD_FRAGMENTED 0x40000000
#define LVD_TRIGGERED  0x20000000
#define LVD_HEADBIT    0x80000000
#if 0
#define LVD_HEADMASK  (~(LVD_FIFO_MASK|LVD_FRAGMENTED|LVD_TRIGGERED)|LVD_HEADBIT)
#endif
#if (LVD_FIFO_SIZE&LVD_FIFO_MASK)
#error LVD FIFO SIZE is not a power of 2
#endif

#if 0 /* defined in dev/pci/zellvd_map.h */
struct lvd_reg {
        u_int32_t           ident;
        u_int32_t           sr;             /* Status Register */
#define LVD_SR_SPEED_0      0x80000000
#define LVD_SR_SPEED_1      0x40000000
#define LVD_SR_TX_SYNCH_0   0x20000000
#define LVD_SR_TX_SYNCH_1   0x10000000
#define LVD_SR_RX_SYNCH_0   0x08000000
#define LVD_SR_RX_SYNCH_1   0x04000000
#define LVD_SR_LINK_B       0x02000000
#define LVD_DBELL_LVD_ERR   0x01000000
#define LVD_DBELL_EVENTPF   0x00010000
#define LVD_DBELL_EVENT     0x00000100
#define LVD_DBELL_INT       0x00000001

        u_int32_t           cr;             /* Control Register */
#define LVD_CR_DDTX        0x01    /* direct data transfer */
#define LVD_CR_SCAN        0x02    /* scan LVD front bus */
#define LVD_CR_SINGLE      0x04    /* single event dorbell */
#define LVD_CR_LED0        0x08    /* SIS1100opt front LED (upper) */
#define LVD_CR_LED1        0x10    /* SIS1100opt front LED (lower) */
#define LVD_CR_LEDs        (LVD_CR_LED0|LVD_CR_LED1)

        u_int32_t           res0C;
        u_int32_t           ddt_counter;
        u_int32_t           ddt_blocksz;
        u_int32_t           jtag_csr;       /* JTAG control/status */
        u_int32_t           jtag_data;      /* JTAG data */
        u_int32_t           timer;          /* 0.1 ms */
};
#endif

/* broadcast registers of a LVD controller card */
/* (hardware version 1 version only (F1)) */
/* 128 Byte */
struct lvd_controller_bc_1 {
        u_int16_t           card_onl;  /* RW */
        u_int16_t           card_offl; /* RO (sparse scan) */
        u_int16_t           transp;    /* WO */
	u_int16_t           card_id;   /* WO */
	u_int16_t           cr;        /* WO */
	u_int16_t           sr;        /* WR, RD:f1_err -> any DAQ F1 error */
        /* sr was f1_err */
        u_int32_t           event_nr;  /* WO */
        u_int16_t           ro_data;   /* RO data available (sparse scan) */
        u_int16_t           fifo_pf;   /*    event FIFO partially full */
        u_int16_t           ro_delay;  /* WO delay after event (for readout of input cards) */
        u_int16_t           res16[5];
        u_int16_t           f1_reg[16];
        u_int16_t           jtag_csr;
        u_int16_t           res42[30];
        u_int16_t           action; /* alle Koppelkarten und alle Eingabekarten */
#define F1_DAQRESET         0x20  /* Event FIFO */
};

/* registers of a LVD controller card (hardware version 1 (F1)) */
/* 128 Byte */
struct lvd_controller_1 {
        u_int16_t           ident;
        u_int16_t           serial;
        u_int16_t           res04[2];
        u_int16_t           cr;
#define LVD_MCR_DAQ          0x3  /* mask for DAQ mode */
#define LVD_MCR_DAQIDLE        0  /* inactive */
#define LVD_MCR_DAQWINDOW      1  /* automatische Erfassung der Input Daten */
#define LVD_MCR_DAQRAWINP      2  /* raw input, trigger not used */
#define LVD_MCR_DAQRAWTRIG     3  /* raw input, triggered, but no window */
#define LVD_MCR_DAQ_TEST    0x04  /* enable test pulse only */
#define LVD_MCR_DAQ_HANDSH  0x08  /* event synchronisation via handshake */
#define LVD_MCR_DAQ_LOCEVNR 0x10  /* local generation of event number */
#define LVD_MCR_TRG_BUSY    0x40  /* set busy signal for trigger */
#define LVD_MCR_NOSTART     0x80  /* no 6.4 us Start Signal */

        u_int16_t           sr;
#define LVD_MSR1_WRBUSY     0x0001
#define LVD_MSR1_NOINIT     0x0002
#define LVD_MSR1_NOLOCK     0x0004 /* PLL not locked */
#define LVD_MSR1_T_OVL      0x0008 /* F1 Trigger FIFO Overflow (not used) */
/*#define LVD_MSR_SYNC_ERR    0x0010*/ /* no valid event number received */
#define LVD_MSR1_H_OVL      0x0010 /* F1 hit FIFO overflow */
#define LVD_MSR1_O_OVL      0x0020 /* F1 output FIFO overflow */
#define LVD_MSR1_FATAL      0x0040 /* fatal error (reg. 0xE or local) */
#define LVD_MSR1_TRG_LOST   0x0080 /* trigger overrun */
#define LVD_MSR1_DATA_AV    0x0100 /* FIFO Data available */
#define LVD_MSR1_READY      0x0200 /* trigger time available / event ready */
#define LVD_MSR1_EVC_ERR    0x1000 /* error receiving event number */
#define LVD_MSR1_FIFO_FULL  0x4000 /* event FIFO full */
#define LVD_MSR1_F1_SEQERR  0x8000 /* F1 sequence error (wrong data) */

        u_int32_t           event_nr;
        u_int16_t           ro_data;    /* Blocktransfer aus Event FIFO */
        u_int16_t           res12;
        u_int16_t           ro_delay;/* Delay nach Event fuer Readout input Cards */
        u_int16_t           res16;
        u_int32_t           trigger_time; /* Trigger Timestamp */
        u_int16_t           res1c[2];/* was: address and length of last event */
        u_int16_t           f1_reg[16];
        u_int32_t           jtag_csr;
        u_int32_t           jtag_data;
        u_int16_t           res48[27];
        u_int16_t           ctrl;
};

/* registers of a LVD controller card (hardware version 2 (GPX)) */
/* 128 Byte */
struct lvd_controller_2 {
        u_int16_t           ident;
        u_int16_t           serial;
        u_int16_t           res04[2];
        u_int16_t           cr;
#if 0 /* identical to version 1 */
#define LVD_MCR_DAQ         0x03  /* mask for DAQ mode */
#define LVD_MCR_DAQIDLE        0  /* inactive */
#define LVD_MCR_DAQWINDOW      1  /* automatische Erfassung der Input Daten */
#define LVD_MCR_DAQRAWINP      2  /* raw input, trigger not used */
#define LVD_MCR_DAQRAWTRIG     3  /* raw input, triggered, but no window */
#define LVD_MCR_DAQ_TEST    0x04  /* enable test pulse only */
#define LVD_MCR_DAQ_HANDSH  0x08  /* event synchronisation via handshake */
#define LVD_MCR_DAQ_LOCEVNR 0x10  /* local generation of event number */
#define LVD_MCR_TRG_BUSY    0x40  /* set busy signal for trigger */
#define LVD_MCR_NOSTART     0x80  /* no 6.4 us Start Signal */
#endif
#define LVD_MCR_F1_MODE     0x20  /* trigger time in F1 format */

        u_int16_t           sr;
#define LVD_MSR2_EF0        0x0001 /* empty flag of GPX FIFO 0 */
#define LVD_MSR2_EF1        0x0002 /* empty flag of GPX FIFO 1 (not used) */
#define LVD_MSR2_NOLOCK     0x0004 /* PLL not locked */
#define LVD_MSR2_INTFLAG    0x0008 /* high bit of the GPX start counter */
#define LVD_MSR2_FATAL      0x0040 /* fatal error (reg. 0xE or local) */
#define LVD_MSR2_TRG_LOST   0x0080 /* trigger overrun */
#define LVD_MSR2_DATA_AV    0x0100 /* FIFO Data available */
#define LVD_MSR2_READY      0x0200 /* trigger time available / event ready */
#define LVD_MSR2_EVC_ERR    0x1000 /* error receiving event number */
#define LVD_MSR2_FIFO_FULL  0x4000 /* event FIFO full */
#if 0 /* identical to version 1 */
#endif

        u_int32_t           event_nr;
        u_int16_t           ro_data;    /* Blocktransfer aus Event FIFO */
        u_int16_t           res12;
        u_int16_t           ro_delay;/* Delay nach Event fuer Readout input Cards */
        u_int16_t           res16;
        u_int32_t           trigger_time; /* Trigger Timestamp */
        u_int32_t           XXXevent_info; /* address and length of last event */
        u_int16_t           gpx_reg;
        u_int16_t           res22;
        u_int32_t           gpx_data;
        u_int32_t           gpx_range;
        u_int16_t           res2c[41];
        u_int16_t           ctrl;
};

struct i2c {
        u_int16_t           csr;        /* 60 68 */
#define LVD_I2C_BUSY        0x0001
#define LVD_I2C_ERROR       0x0002
#define LVD_I2C_DREADY      0x0004
#define LVD_I2C_RESET       0x8000
        u_int16_t           data;       /* 62 6a */
        u_int16_t           wr;         /* 64 6c */
        u_int16_t           rd;         /* 66 6e */
};

/* registers of a LVD controller card (hardware version 3 (GPX+F1)) */
/* 128 Byte */
struct lvd_controller_3 {
        u_int16_t           ident;      /*  0 */
        u_int16_t           serial;     /*  2 */
        u_int16_t           res04[2];   /*  4 */

        u_int16_t           cr;         /*  8 */
#define LVD_MCR3_DAQ        0x83  /* mask for DAQ mode */
#if 0 /* identical to version 2 */
#define LVD_MCR_DAQIDLE        0  /* inactive */
#define LVD_MCR_DAQWINDOW      1  /* automatische Erfassung der Input Daten */
#define LVD_MCR_DAQRAWINP      2  /* raw input, trigger not used */
#define LVD_MCR_DAQRAWTRIG     3  /* raw input, triggered, but no window */
#define LVD_MCR_DAQ_TEST    0x04  /* enable test pulse only */
#define LVD_MCR_DAQ_HANDSH  0x08  /* event synchronisation via handshake */
#define LVD_MCR_DAQ_LOCEVNR 0x10  /* local generation of event number */
#define LVD_MCR_F1_MODE     0x20  /* trigger time in F1 format */
#define LVD_MCR_TRG_BUSY    0x40  /* set busy signal for trigger */
#endif
#define LVD_MCR3_QDCTRIG       0x81 /* triggered by QDCs */
#define LVD_MCR3_DAQTRIGANDRAW 0x82 /* RAW and TRIGGERED combined */
#define LVD_MCR3_EXTRA_TRIG   0x100 /* Add trigger times to data */
#define LVD_MCR3_RATE_TRIG    0x200 /* Enable rate_trigger */

        u_int16_t           sr;         /*  a */
#define LVD_MSR3_NOINIT     0x0001
#define LVD_MSR3_NOLOCKF1   0x0002 /* F1 PLL not locked */
#define LVD_MSR3_NOLOCKGPX  0x0004 /* GPX PLL not locked */
#define LVD_MSR3_PHASE_ERR  0x0008 /* GPX start counter not in phase */
#define LVD_MSR3_EVC_ERR    0x0010 /* error receiving event number */
#define LVD_MSR3_TRG_LOST   0x0020 /* trigger overrun */
#define LVD_MSR3_GLB        0x0040 /* fatal error (reg. 0xE) */
#define LVD_MSR3_FATAL      0x0080 /* fatal error (reg. 0xE or local) */
#define LVD_MSR3_DATA_AV    0x0100 /* FIFO Data available */
#define LVD_MSR3_READY      0x0200 /* trigger time available / event ready */
#define LVD_MSR3_FIFO_FULL  0x8000 /* event FIFO full */

        u_int32_t           event_nr;   /*  c */
        u_int32_t           ro_data;    /* Blocktransfer aus Event FIFO */
        u_int16_t           ro_delay;/* Delay nach Event fuer Readout input Cards */
        u_int16_t           res16;      /* 16 */
        u_int32_t           trigger_time; /* Trigger Timestamp */
        u_int16_t           res1c[2];   /* 1c */
        u_int16_t           gpx_reg;    /* 20 */
        u_int16_t           res22;      /* 22 */
        u_int32_t           gpx_data;   /* 24 */
        u_int32_t           gpx_range;  /*28/2a*/
        u_int16_t           rate_trigger; /* 2c */
        u_int16_t           res2c[9];   /* 2e */
        u_int16_t           f1_reg[16]; /*40/5e*/

struct i2c i2c[2];
        u_int16_t           res70[7];
        u_int16_t           ctrl;
};

/* common registers of all LVD controller cards */
/* 128 Byte */
struct lvd_controller_common {
        u_int16_t           ident;
        u_int16_t           serial;
        u_int16_t           res04[2];
        u_int16_t           cr;
#if 0 /* identical to version 1 */
#define LVD_MCR_DAQ         0x03  /* mask for DAQ mode */
#define LVD_MCR_DAQIDLE        0  /* inactive */
#define LVD_MCR_DAQWINDOW      1  /* automatische Erfassung der Input Daten */
#define LVD_MCR_DAQRAWINP      2  /* raw input, trigger not used */
#define LVD_MCR_DAQRAWTRIG     3  /* raw input, triggered, but no window */
#define LVD_MCR_DAQ_TEST    0x04  /* enable test pulse only */
#define LVD_MCR_DAQ_HANDSH  0x08  /* event synchronisation via handshake */
#define LVD_MCR_DAQ_LOCEVNR 0x10  /* local generation of event number */
#define LVD_MCR_TRG_BUSY    0x40  /* set busy signal for trigger */
#endif

        u_int16_t           sr;
#define LVD_MSR_DATA_AV    0x0100 /* FIFO Data available */
#define LVD_MSR_READY      0x0200 /* trigger time available / event ready */

        u_int32_t           event_nr;
        u_int16_t           ro_data;    /* Blocktransfer aus Event FIFO */
        u_int16_t           res12;
        u_int16_t           ro_delay;/* Delay nach Event fuer Readout input Cards */
        u_int16_t           res16;
        u_int32_t           trigger_time; /* Trigger Timestamp */
        u_int16_t           res1c[49];
        u_int16_t           ctrl;
};

/* common registers of all acquisition cards */
/* 128 Byte */
struct lvd_card {
        u_int16_t           ident;    /*   0 */
        u_int16_t           serial;   /*   2 */
        u_int16_t           res04[2]; /*   4 */
        u_int16_t           ro_data;  /*   8 */
        u_int16_t           res10;    /*  10 */
	u_int16_t	    cr;       /*  12 */
	u_int16_t           sr;       /*  14 */
	u_int16_t           res16[55];/*  16 */
        u_int16_t           ctrl;     /* 126 */
};

/* broadcast registers */
/* common for all acquisition cards */
/* 128 Byte */
struct lvd_in_card_bc {
        u_int16_t           card_onl;  /*  0 */
        u_int16_t           card_offl; /*  2 */
        u_int16_t           res04[2];  /*  4 */
        u_int16_t           ro_data;   /*  8 */
        u_int16_t           res0a;     /*  a */
        u_int16_t           cr;        /*  c */ /* ??? */
        u_int16_t           error;     /*  e */
        u_int16_t           res16[28]; /* 10 */
        union trigger {
            u_int32_t       trigger;   /* 48 */ /* used by hardware only! */
            u_int16_t       busy[2];   /* 48 */
        } t;
        u_int16_t           res76[25]; /* 4c */
        u_int16_t           ctrl;      /* 7e */
};

/* bits in ctrl */
    /* valid for: C controller, G gpx, F f1, Q qdc, M sync master, O sync output */
/* controller */
#define LVD_PURES           0x0002 /* power up reset of TDC/GPX chip */
#define LVD_SYNC_RES        0x0004 /* time synchronisation of all cards */
                                   /* propagated as broadcast */
#define LVD_TESTPULSE       0x0008 /* test pulse for TDCs */
                                   /* propagated as broadcast */
#define LVD_TDC_START       0x0010 /* obsolete (test only) */

/* acq cards */
#define LVD_MRESET_         0x0001 /* G     reset */
#define LVD_PURES_          0x0002 /* GF    power up reset of GPX chip */
#define LVD_TRG_RST         0x0002 /*    M  reset trigger */
#define LVD_SYNC_RES_       0x0004 /* GF    time synchronisation of all cards */
#define LVD_TESTPULSE_      0x0008 /* GF    test pulse for TDCs */

/* acq card broadcast */
#define LVD_MRESET          0x0001 /* GFQMO reset */
#define LVD_PURES           0x0002 /* GF    power up reset of GPX chip */
#define LVD_SYNC_RES        0x0004 /* GFQ?  time synchronisation of all cards */
#define LVD_TESTPULSE       0x0008 /* GF    test pulse for TDCs */
#define LVD_TRG_MRST        0x0080 /*    MO reset trigger and event counter */

union lvd_controllers {
    struct lvd_controller_1 f1;
    struct lvd_controller_2 gpx;
    struct lvd_controller_3 f1gpx;
    struct lvd_controller_common cc;
};

union lvd_in_card {
    struct lvd_card all;
    struct lvd_f1_card f1;
    struct lvd_gpx_card gpx;
    struct lvd_vertex_card vertex;
    struct lvd_qdc1_card qdc1;
    struct lvd_qdc3_card qdc3;
    struct lvd_qdc6_card qdc6;
    struct lvd_qdc6s_card qdc6s;
    struct lvd_qdc80_card qdc80;
    struct lvd_msync_card msync;
    struct lvd_msync2_card msync2;
    struct lvd_osync_card osync;
    struct lvd_trig_card trig;
};

/* address mapping of a optical interface card */
struct lvd_bus1100 {

    /* local acquisition cards (local crate) */
    /* 0-7fe*/
    /* 000-07e 080-0fe 100-17e 180-1fe 200-27e 280-2fe 300-37e 380-3fe */
    /* 400-47e 480-4fe 500-57e 580-5fe 600-67e 680-6fe 700-77e 780-7fe */
    union lvd_in_card           in_card[16];

    /* secondary controller cards (cable bus) */
    /* 800-ffe */
    /* 800-87e 880-8fe 900-97e ...*/
    struct lvd_controller_1     Xcontroller[16]; /* not used anymore */

    /* acquisition cards in a secondary crate */
    /* the crate is selected through lvd_controller_bc.transp */
    /* 1000-17fe */
    union lvd_in_card           Xc_in_card[16]; /* not used anymore */
 
    /* primary (local) controller card */
    /* 1800-187e */
    union lvd_controllers       prim_controller;

    /* broadcast space of the local acquisition cards */
    /* 1880-18fe */
    struct lvd_in_card_bc        in_card_bc;

    /* broadcast space of the controller cards at the cable bus */
    /* 1900-197e */
    struct lvd_controller_bc_1  Xcontroller_bc; /* not used anymore */

    /* broadcast space of the acquisition cards in a secondary crate */
    /* the crate is selected through lvd_controller_bc.transp */
    /* 1980-19fe */
    struct lvd_in_card_bc        Xc_in_card_bc; /* not used anymore */
};

#endif
