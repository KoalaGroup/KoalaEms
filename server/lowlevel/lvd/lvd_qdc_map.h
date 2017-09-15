/*
 * lvd_qdc_map.h
 * $ZEL: lvd_qdc_map.h,v 1.18 2012/09/12 15:02:14 wuestner Exp $
 * created 2006-Feb-07
 */

#ifndef _lvd_qdc_map_h_
#define _lvd_qdc_map_h_

#include <sys/types.h>

/* registers of a QDC card with FW version < 3.0 */
/* 128 Byte */
struct lvd_qdc1_card {
        u_int16_t           ident;      /*   0 */
        u_int16_t           serial;     /*   2 */
        int16_t             latency;    /*   4 */
        u_int16_t           window;     /*   6 */
        u_int16_t           ro_data;    /*   8 */
        u_int16_t           res0a;      /*  10 */
        u_int16_t	    cr;         /*  12 */
#define QDC_CR_ENA          0x001   /* gen. enable */
#define QDC_CR_LTRIG        0x002   /* local on-board trigger (NIM input) */
#define QDC_CR_LEVTRIG      0x004   /* triggered by data level */
#define QDC_CR_VERBOSE      0x008   /* send all analysis data */
#define QDC_CR_ADCPOL       0x010   /* inverse ADC data */
#define QDC_CR_SGRD         0x020   /* single gradient  */
#define QDC_CR_F1MODE       0x100   /* use F1 controller instead of GPX */

        u_int16_t           sr;         /*  14 */
#define QDC_SR_FIFO        0x0001   /* FIFO read eror; WR: selective clear */

        u_int16_t           trig_level; /*  16 */
        u_int16_t           anal_ctrl;  /*  18 */
        u_int16_t           res14;      /*  20 */
        u_int16_t           aclk_shift; /*  22 */
        u_int16_t           cha_inh;    /*  24 */
        u_int16_t           cha_raw;    /*  26 */
        u_int16_t           iw_start;   /*  28 */
        u_int16_t           iw_length;  /*  30 */
        union {                         /*  32 */
            u_int16_t       noise_level[16]; /* read */
            u_int16_t       q_threshold[16]; /* write */
        } nq;
        u_int16_t	    jtag_csr;   /*  64 */
#define QDC_JT_TDI          0x001
#define QDC_JT_TMS          0x002
#define QDC_JT_TDO          0x008
#define QDC_JT_ENABLE       0x100
#define QDC_JT_AUTO_CLOCK   0x200
#define QDC_JT_SLOW_CLOCK   0x400
#define QDC_JT_C            (QDC_JT_ENABLE|QDC_JT_AUTO_CLOCK)

        u_int16_t           res42;      /*  66 */
        u_int32_t           jtag_data;  /*  68 */

        u_int16_t           res48[2];   /*  72 */
        u_int16_t           int_length; /*  76 */
        u_int16_t           res4e;      /*  78 */
        u_int32_t           tdc_range;  /*  80 */
        u_int16_t           res54[6];   /*  84 */
        union {                         /*  96 */
            u_int16_t       mean_level[16]; /* read */
            u_int16_t       ofs_dac[4];     /* write */
        } mo;
};

/* registers of a QDC card with FW version >= 3.0 */
/* 128 Byte */
struct lvd_qdc3_card {
        u_int16_t           ident;      /*  0 */
        u_int16_t           serial;     /*  2 */
        u_int16_t           trig_level; /*  4 */
        u_int16_t           aclk_shift; /*  6 */
        u_int16_t           ro_data;    /*  8 */
        u_int16_t           res0a;      /*  a */
        u_int16_t	    cr;         /*  c */
/*#define QDC_CR_ENA        0x001*/ /* gen. enable */
/*#define QDC_CR_LTRIG      0x002*/ /* local on-board trigger (NIM input) */
/*#define QDC_CR_LEVTRIG    0x004*/ /* triggered by data level */
/*#define QDC_CR_VERBOSE    0x008*/ /* send all analysis data */
/*#define QDC_CR_ADCPOL     0x010*/ /* inverse ADC data */
/*#define QDC_CR_SGRD       0x020*/ /* single gradient  */
#define QDC_CR_TST_SIG      0x040   /* activate test signal (FQDC only) */
#define QDC_CR_ADC_PWR      0x080   /* switch off ADC power (FQDC only) */
/*#define QDC_CR_F1MODE     0x100*/ /* use F1 controller instead of GPX */
#define QDC_CR_LOCBASE      0x200   /* use automatic baseline */
#define QDC_CR_NOPS         0x400   /* no pulse search */
/* version>3: */
#define QDC_CR_LMODE        0x800   /* enable time over threshold data */
#define QDC_CR_TRAW        0x1000   /* send raw data if threshold reached */
/* version 5: */
#define QDC_CR_LTHRES      0x2000   /* selector for bline_thlevel */
#define QDC_CR_BL_QTH      0x4000   /* selector for noise_qthr and bline_thlevel */
#define QDC_CR_SEL         (QDC_CR_LTHRES|QDC_CR_BL_QTH)

        u_int16_t           sr;         /*  e */
/*#define QDC_SR_FIFO      0x0001*/   /* FIFO read eror; WR: selective clear */
        u_int16_t           cha_inh;    /* 10 */
        u_int16_t           cha_raw;    /* 12 */
        u_int16_t           iw_start;   /* 14 */
        u_int16_t           iw_length;  /* 16 */
        u_int16_t           sw_start;   /* 18 */
        u_int16_t           sw_length;  /* 1a */
        u_int16_t           sw_ilength; /* 1c */
        u_int16_t           anal_ctrl;  /* 1e */
        u_int16_t           noise_qthr[16];/* 20..3e */
/*
            version 3:  write: Q-threshold
                        read : peak to peak noise level
            version 4:  write: Q-threshold
                        read : peak to peak noise level
            version 5:  write: Q-threshold
                        read : peak to peak noise level (cr<14>=0)
                        read : Q-threshold              (cr<14>=1)
*/
//         union {                         /* 20..3e */
//             u_int16_t       noise_level[16]; /* read */
//             u_int16_t       q_threshold[16]; /* write */
//         } nq;
        u_int16_t           bline_thlevel[16];/* 40..5e */
/*
            version 3:  write: fixed base line 12.4 bit
                        read : auto base line 12.4 bit
            version 4:  write: threshold level 12 bit
                        read : auto base line 12.4 bit
            version 5:  write: fixed base line 12.4 bit (cr<13>=0)
                        write: threshold level 12 bit   (cr<13>=1)
                        read : auto base line 12.4 bit  (cr<13,14>=0)
                        read : threshold level 12 bit   (cr<13>=1)
                        read : fixed base line 12.4 bit (cr<14>=1)
*/
//         union {                         /* 40..5e */
//             u_int16_t       mean_level[16]; /* read */
//             u_int16_t       base_line[16];  /* write */
//             u_int16_t       thrh_level[16];  /* r/w with bit 13 in cr */
//         } mb;
        u_int32_t           tdc_range;  /* 60 */
        u_int16_t           traw_cycle; /* 64 */
        u_int16_t           ovr;        /* 66 */
        u_int16_t           bline_adjust; /* 68 */
        u_int16_t           res6a[3];   /* 6a..6e */
        u_int16_t           ofs_dac[4]; /* 70..76 */
        u_int32_t           jtag_data;  /* 78..7a */
        u_int16_t	    jtag_csr;   /* 7c */
#define QDC_JT_TDI          0x001
#define QDC_JT_TMS          0x002
#define QDC_JT_TDO          0x008
#define QDC_JT_ENABLE       0x100
#define QDC_JT_AUTO_CLOCK   0x200
#define QDC_JT_SLOW_CLOCK   0x400
#define QDC_JT_C            (QDC_JT_ENABLE|QDC_JT_AUTO_CLOCK)

        u_int16_t           res7e;      /* 7e */
};

/* registers of a QDC 240 Mhz card with FW version >= 6.0 */
/* 128 Byte */
struct lvd_qdc6_card {
        u_int16_t           ident;      /*  0 */
        u_int16_t           serial;     /*  2 */
        u_int16_t           grp_coinc;  /*  4 */
        u_int16_t           aclk_shift; /*  6 */
        u_int16_t           ro_data;    /*  8 */
        u_int16_t           res0a;      /*  a */
        u_int16_t           cr;         /*  c */
//#define QDC_CR_ENA        0x001 /* gen. enable */
#define QDC_CR_TRG_MODE_BIT1       0x002
#define QDC_CR_TRG_MODE_BIT2       0x004
/* TRG_MODE selection
0 - Trigger with system controller
1 - Trigger with LEMO of QDC module -System Controler has to be in RAW MODE (2)
2 - Trigger with coincidence matrix of QDC Module - trigger output also on
            debug conector
3 - Trigger with coincidence from first ADC FPGA 
*/
#define QDC_CR_TRG_MODE_SEL   (QDC_CR_TRG_MODE_BIT1|QDC_CR_TRG_MODE_BIT2) /* TRG_MODE*/
//#define QDC_CR_VERBOSE     0x008  /* send all analysis data */
//#define QDC_CR_ADCPOL      0x010  /* inverse ADC data */
//#define QDC_CR_SGRD        0x020  /* single gradient  */
//#define QDC_CR_TST_SIG     0x040  /* activate test signal (FQDC only) */
//#define QDC_CR_ADC_PWR     0x080  /* switch off ADC power (FQDC only) */
//#define QDC_CR_F1MODE      0x100  /* use F1 controller instead of GPX */
//#define QDC_CR_LOCBASE     0x200  /* use automatic baseline */
#define QDC_CR_SI_PS       0x400  /* analyse only first  pulse  */
//#define QDC_CR_LMODE       0x800  /* enable time over threshold data */
//#define QDC_CR_LTHRES      0x2000 /* selector for bline_thlevel */
//#define QDC_CR_BL_QTH      0x4000 /* selector for noise_qthr and bline_thlevel */
//#define QDC_CR_SEL         (QDC_CR_LTHRES|QDC_CR_BL_QTH)

        u_int16_t           sr;         /*  e */
/*#define QDC_SR_FIFO      0x0001*/   /* FIFO read eror; WR: selective clear */
        u_int16_t           cha_inh;    /* 10 */
        u_int16_t           cha_raw;    /* 12 */
        u_int16_t           iw_start;   /* 14 */
        u_int16_t           iw_length;  /* 16 */
        u_int16_t           sw_start;   /* 18 */
        u_int16_t           sw_length;  /* 1a */
        u_int16_t           sw_ilength; /* 1c */
        u_int16_t           anal_ctrl;  /* 1e */
        u_int16_t           noise_qthr_coi[16];/* 20..3e */
/*
            version 6:
                        write: Q-threshold              (cr<14>=0)
                        write: Coincidence parameter    (cr<14>=1)
                        read : peak to peak noise level (cr<14>=0)
                        read : Q-threshold              (cr<14>=1,cr<13>=0)
                        read : Coincidence parameter    (cr<14>=1,cr<13>=1)
*/
        u_int16_t           bline_thlevel[16];/* 40..5e */
/*
            version 6:  
                        write: fixed base line 12.4 bit (cr<13>=0)
                        write: threshold level 12 bit   (cr<13>=1)
                        read : auto base line 12.4 bit  (cr<14>=0)
                        read : fixed base line 12.4 bit (cr<14>=1,cr<13>=0)
                        read : threshold level 12 bit   (cr<14>=1,cr<13>=1)
*/
        u_int32_t           tdc_range;  /* 60 */
        u_int16_t           traw_cycle; /* 64 */
        u_int16_t           ovr;        /* 66 */
        u_int16_t           bline_adjust; /* 68 */
        u_int16_t           res6a[1];   /* 6a */
        u_int16_t           trigger_time; /* 6c */
        u_int16_t           res6e[1];   /* 6e */
        u_int16_t           dac_coinc[4]; /* 70..76 */
/*
            version 6:  
                        write: test signal amplitude nur dac_coinc[0] (cr<13>=0)
                        write: lookup table for coincidence  (cr<13>=1)
*/
        u_int32_t           jtag_data;  /* 78..7a */
        u_int16_t	    jtag_csr;   /* 7c */
//#define QDC_JT_TDI          0x001
//#define QDC_JT_TMS          0x002
//#define QDC_JT_TDO          0x008
//#define QDC_JT_ENABLE       0x100
//#define QDC_JT_AUTO_CLOCK   0x200
//#define QDC_JT_SLOW_CLOCK   0x400
//#define QDC_JT_C            (QDC_JT_ENABLE|QDC_JT_AUTO_CLOCK)

        u_int16_t           ctrl;       /* 7e */
};

/* registers of a QDC 240 Mhz card with FW version >= 6.7 */
/* 128 Byte */
struct lvd_qdc6s_card {
        u_int16_t           ident;      /*  0 */
        u_int16_t           serial;     /*  2 */
        u_int16_t           grp_coinc;  /*  4 */
        u_int16_t           aclk_shift; /*  6 */
        u_int16_t           ro_data;    /*  8 */
        u_int16_t           res0a;      /*  a */
        u_int16_t           cr;         /*  c */
        u_int16_t           sr;         /*  e */
        u_int16_t           cha_inh;    /* 10 */
        u_int16_t           cha_raw;    /* 12 */
        u_int16_t           iw_start;   /* 14 */
        u_int16_t           iw_length;  /* 16 */
        u_int16_t           sw_start;   /* 18 */
        u_int16_t           sw_length;  /* 1a */
        u_int16_t           sw_ilength; /* 1c */
        u_int16_t           anal_ctrl;  /* 1e */
        u_int16_t           noise_qthr_coi[16];/* 20..3e */
        u_int16_t           bline_thlevel[16];/* 40..5e */
        u_int32_t           tdc_range;  /* 60 */
        u_int16_t           traw_cycle; /* 64 */
        u_int16_t           ovr;        /* 66 */
        u_int32_t           scaler;     /* 68 */ /* auto increment for read */
        u_int16_t           trigger_time; /* 6c */
        u_int16_t           scaler_rate;  /* 6e */
        u_int16_t           dac_coinc[4]; /* 70..76 */
        u_int32_t           jtag_data;  /* 78..7a */
        u_int16_t           jtag_csr;   /* 7c */
        u_int16_t           ctrl_bline_adjust; /* 7e */
};

/* registers of a QDC 240 Mhz card with FW version >= 8.0 */
/* 128 Byte */
struct lvd_qdc80_card {
        u_int16_t           ident;           /*  0 */
        u_int16_t           serial;          /*  2 */
        u_int16_t           channel;         /*  4 */
        u_int16_t           aclk_shift;      /*  6 */
        u_int32_t           ro_data;         /*  8, a */
        u_int16_t           cr;              /*  c */
/*
#define QDC_CR_ENA                 0x001  gen. enable
#define QDC_CR_TRG_MODE_BIT1       0x002
#define QDC_CR_TRG_MODE_BIT2       0x004
 TRG_MODE selection
0 - Trigger with system controller
1 - Trigger with LEMO of QDC module -System Controler has to be in RAW MODE (2) ?????????? 
2 - Trigger with coincidence matrix of QDC Module - trigger output also on
            debug conector
3 - Trigger with coincidence from first ADC FPGA 
*/
#define QDC_CR_TDC_ENA            0x008
#define QDC_CR_FIX_BASELN         0x010
//#define QDC_CR_ADC_PWR     0x080  /* switch off ADC power (FQDC only) */
//#define QDC_CR_F1MODE      0x100  /* use F1 controller instead of GPX */
        u_int16_t           sr;              /*  e */
        u_int16_t           sw_start;        /* 10 */
        u_int16_t           sw_len;          /* 12 */
        u_int16_t           iw_start;        /* 14 */
        u_int16_t           iw_len;          /* 16 */
        u_int16_t           coinc_tab;       /* 18*/
        u_int16_t           res1a[2];        /* 1a..1d*/
        u_int16_t           ch_ctrl;         /* 1e */
        u_int16_t           outp;            /* 20 */
        u_int16_t           cf_qrise;        /* 22 */
        u_int16_t           tot_level;       /* 24 */
        u_int16_t           logic_level;     /* 26 */
        u_int16_t           iw_q_thr;        /* 28 */
        u_int16_t           sw_q_thr;        /* 2a */
        u_int16_t           sw_ilen;         /* 2c */
        u_int16_t           cl_q_st_len;     /* 2e */
        u_int16_t           cl_q_del;        /* 30 */
        u_int16_t           cl_len;          /* 32 */
        u_int16_t           coinc_par;       /* 34 */
        u_int16_t           dttau;           /* 36 */
        u_int16_t           raw_table;       /* 38 */
        u_int16_t           baseline;        /* 3a */
        u_int16_t           tcor_noise;      /* 3c */
        u_int16_t           dttau_quality;   /* 3e */
        u_int16_t           res40[16];       /* 40..5e */
        u_int32_t           tdc_range;       /* 60, 62 */
        u_int32_t           scaler;          /* 64, 66 */ /* auto increment */
        u_int16_t           scaler_rout;     /* 68 */
        u_int16_t           coinmin_traw;    /* 6a */
        u_int16_t           trigger_time;    /* 6c */
        u_int16_t           res6e;           /* 6e */
        u_int16_t           grp_coinc;       /* 70 */
        u_int16_t           tp_dac;          /* 72 */
        u_int16_t           bl_cor_cycle;    /* 74 */
        u_int16_t           res76;           /* 76 */
        u_int32_t           jtag_data;       /* 78..7a */
        u_int16_t           jtag_csr;        /* 7c */
        u_int16_t           ctrl_ovr;        /* 7e */
};

#endif
