/*
 * lvd_vertex_map.h
 * $ZEL: lvd_vertex_map.h,v 1.19 2011/07/11 11:20:35 trusov Exp $
 * created 2005-Feb-26
 */

#ifndef _lvd_vertex_map_h_
#define _lvd_vertex_map_h_

#include <sys/types.h>
#include <emsctypes.h>
/*#include <dev/pci/sis1100_var.h>
#include <dev/pci/zellvd_map.h>*/

enum vtx_chiptype {VTX_CHIP_NONE=0, VTX_CHIP_VA32TA2, VTX_CHIP_MATE3};
#define VATA_FIFO_SIZE 128 /* size of vtx_grp.reg_data */


/* 128 Byte */
#define VTX_SEQ_CSR_LV_HALT    0x0001
#define VTX_SEQ_CSR_LV_IDLE    0x0002
#define VTX_SEQ_CSR_LV_OVFL    0x0004
#define VTX_SEQ_CSR_LV_UNFL    0x0008
#define VTX_SEQ_CSR_LV_OPERR   0x0010
#define VTX_SEQ_CSR_LV_FEMPTY  0x0020
#define VTX_SEQ_CSR_LV_BUSY    0x0080
#define VTX_SEQ_CSR_HV_HALT    0x0100
#define VTX_SEQ_CSR_HV_IDLE    0x0200
#define VTX_SEQ_CSR_HV_OVFL    0x0400
#define VTX_SEQ_CSR_HV_UNFL    0x0800
#define VTX_SEQ_CSR_HV_OPERR   0x1000
#define VTX_SEQ_CSR_HV_FEMPTY  0x2000
#define VTX_SEQ_CSR_HV_BUSY    0x8000

#define	VTX_CR_RUN             0x0001
#define	VTX_CR_RAW             0x0002
#define	VTX_CR_VERB            0x0004
#define	VTX_CR_TM_MODE         0x0008
#define	VTX_CR_IH_LV           0x0010
#define	VTX_CR_IH_HV           0x0020
#define	VTX_CR_IH_TRG          0x0040
#define	VTX_CR_VA_TRG          0x0080
#define	VTX_CR_MON             0x0700
#define VTX_CR_MON_BUSY        0x0100
#define VTX_CR_UDW             0x0800
#define VTX_CR_NOCOM           0x1000
#define VTX_CR_LV_E            0x4000
#define VTX_CR_HV_E            0x8000


#define VTX_CR_LV_E            0x4000
#define VTX_CR_HV_E            0x8000


#define	VTX_SR_FAT_ERR         0x0001
#define	VTX_SR_RO_ERR          0x0002
#define	VTX_SR_LV_DAQ_ERR      0x0030
#define	VTX_SR_HV_DAQ_ERR      0x00C0
#define	VTX_SR_LV_POTY_BUSY    0x0100
#define	VTX_SR_HV_POTY_BUSY    0x0200
#define	VTX_SR_POTY_BUSY       (VTX_SR_LV_POTY_BUSY|VTX_SR_HV_POTY_BUSY)
#define	VTX_SR_LV_VADAC_BUSY   0x0400
#define	VTX_SR_HV_VADAC_BUSY   0x0800
#define	VTX_SR_VADAC_BUSY      (VTX_SR_LV_VADAC_BUSY|VTX_SR_HV_VADAC_BUSY)
#define	VTX_SR_LV_VAREGOUT_BSY 0x1000
#define	VTX_SR_HV_VAREGOUT_BSY 0x2000
#define	VTX_SR_VAREGOUT_BSY    (VTX_SR_LV_VAREGOUT_BSY|VTX_SR_HV_VAREGOUT_BSY)
#define	VTX_SR_SERIAL_BUSY     (VTX_SR_POTY_BUSY|VTX_SR_VADAC_BUSY|VTX_SR_VAREGOUT_BSY)
#define	VTX_SR_LV_VAREGIN_DVAL 0x4000
#define	VTX_SR_HV_VAREGIN_DVAL 0x8000
#define	VTX_SR_VAREGIN_DVAL    (VTX_SR_LV_VAREGIN_DVAL|VTX_SR_HV_VAREGIN_DVAL)

#define VTX_C_HV_SRAM_OFFSET   0x080000L
#define VTX_C_HV_TRAM_OFFSET   0x100000L

/* #define BLOCK1_HV 0x100000L */


#define VTX_I2C_CHIPS     0x001F /* number of chips mask */
#define VTX_I2R_BSY_MRST  0x0100 /* RD:busy; WR:master reset */
#define VTX_I2R_DAV_IFRST 0x0200 /* RD:data available; WR:input FIFO reset */
#define VTX_I2R_ERR_RST   0x0400 /* RD:I2C error; WR:error reset */
#define VTX_I2R_SETUP     0x8000 /* WO:setup I2C bus, determine number of chips */


#define VTX_JT_TDI             0x001
#define VTX_JT_TMS             0x002
#define VTX_JT_TDO             0x008
#define VTX_JT_ENABLE          0x100
#define VTX_JT_AUTO_CLOCK      0x200
#define VTX_JT_SLOW_CLOCK      0x400
#define VTX_JT_C            (VTX_JT_ENABLE|VTX_JT_AUTO_CLOCK)

#define	R_CNTRSTLV             0x0100
#define	R_CNTRSTHV             0x0200
#define	R_CLEARERROR           0x0400
#define	R_RESETSEQ             0x0800



/* register group associated with LV/HV for VERTEX */

struct vtx_v_grp { /* for vata support*/
        ems_u16    comval;            /* 36  66 read only, last common mode value */
        ems_u16    comcnt;            /* 38  68 read only, last number of data in noise window */
        ems_u16    poti;              /* 3A  6A write only, ADC offset */
        ems_u16    dac;               /* 3C  6C write only, serial port for DAC loading */
        ems_u16    reg_cr;            /* 3E  6E control reg to load VATA */
        ems_u16    reg_data;          /* 40  70 FIFO for VATA loading */
        ems_u16    adc_value;         /* 42  72 read only, current ADC value */
        ems_u16    otr_counter;       /* 44  74 read only, out of range counter */
        ems_u16    user_data;         /* 46  76 write only, user data word, 24 Bit, high then low 16 bits */ 
};

/* register group associated with LV/HV for VERTEXM3 or VERTEXUN*/
struct vtx_m_grp {
      union {                      /* 36  66 */
        ems_u16    poti;           /*           write ADC offset */         
        ems_u16    comval;         /*           read last common mode value */
      };
      union {                      /* 38  68 */
	ems_u16    dac;            /*           write,  serial port for DAC loading */
        ems_u16    comcnt;         /*           read last number of data in noise window */
      };
      ems_u16	   i2c_csr;        /* 3A  6A    I2C control/status */
      ems_u16	   i2c_data;       /* 3C  6C    I2C FIFO read/write */
      ems_u16	   adc_value;      /* 3E  6E    read only, current ADC value */
      union {                      /* 40  70 */
        ems_u16    user_data;      /*           write, user data word, 24 Bit, high then low 16 bits */
        ems_u16    otr_counter;    /*           read,  out of range counter */
      };
      ems_u16    aclk_par;       /* 42  72    ADC clock parameter */
      ems_u16    xclk_del;       /* 44  74    delay of 2. VA clock */
      ems_u16    res46;          /* 46  76    not used */ 
};

/* register group associated with LV/HV for VERTEX or VERTEXM3 */
struct vtx_grp {
        ems_u32    swreg;             /* 20  50 */
        ems_u16    counter[2];        /* 24  54 */
        ems_u16    lp_counter[2];     /* 28  58 */
        ems_u16    dgap_len[2];       /* 2C  5C */
        ems_u16    clk_delay;         /* 30  60 */
        ems_u16    nr_chan;           /* 32  62 */
        ems_u16    noise_thr;         /* 34  64 */
    union  {
      struct vtx_m_grp m;
      struct vtx_v_grp v;
    };
};

/* registers of a vertex ADC card of VERTEX, VERTEXM3 or VERTEXUN */
struct lvd_vertex_card {
        ems_u16    ident;             /* 00 */
        ems_u16    serial;            /* 02 */
        ems_u16    seq_csr;           /* 04 */
        union {
          ems_u16    aclk_par;          /* 06, not used for VERTEXM3! res06 for it */
          ems_u16    unit_mode;         /* 06 only for VERTEXUN */
        }; 
        ems_u16    ro_data;           /* 08 */
        ems_u16    res0a;             /* 0a */
        ems_u16    cr;                /* 0c */
        ems_u16    sr;                /* 0e */
        ems_u32    seq_addr;          /* 10 */
        union seq_data {
            ems_u32 l;                /* 14 16 */
            ems_u16 s;                /* 14 */
        } seq_data;
        ems_u32    table_addr;        /* 18 */

        union table_data {
            ems_u32 l;                /* 1c 1e */
            ems_u16 s;                /* 1c */
        } table_data;
        struct vtx_grp lv;            /* 20 */

        ems_u16    res48;             /* 48 */
        ems_u16    cyclic_raw;        /* 4a */
        ems_u16    scaling;           /* 4c */
        ems_u16    dig_tst;           /* 4e */
        struct vtx_grp hv;            /* 50 */
        ems_u32    jtag_data;         /* 78 */
        ems_u16    jtag_csr;          /* 7c */
        ems_u16	   action;            /* 7e */
};

/* broadcast registers of a vertex ADC card (both VATA and MATE3) */
/* 128 Byte */
struct lvd_vertex_card_bc {
        ems_u16    card_onl;       /* 00 */
        ems_u16    card_offl;      /* 02 */
        ems_u16    res02;          /* 04 */
        ems_u16    res03;          /* 06 */
        ems_u16    ro_fifo;        /* 08 */
        ems_u16    res05;          /* 0A */
        ems_u16    res06;          /* 0C */
        ems_u16    error;          /* 0E */
        ems_u16    res08[28];      /* 10 */
        ems_u32    trigger;        /* 48 */ /* used by hardware only! */
        ems_u16    res22[25];      /* 4A */
        ems_u16    action;         /* 7E */ /* Master Reset */
};

#endif
