/*
 * procs/unixvme/sis4100/sis4100_map.h
 * $ZEL: sis4100_map.h,v 1.1 2003/10/27 11:46:34 wuestner Exp $
 */
#ifndef _sis4100_map_h_
#define _sis4100_map_h_

/*
 *   these definitions are identical for SIS4100/NGF and STR340/SFI
 */

#include <sys/types.h>

typedef volatile struct sfi_w
  {
  u_int32_t dummy0[0x400];
  u_int32_t write_vme_out_signal;          /* 1000 */
  u_int32_t clear_both_lca1_test_register; /* 1004 */
  u_int32_t dummy1[2];
  u_int32_t write_int_fb_ad;               /* 1010 */
  u_int32_t aux_puls_b40;                  /* 1014 */
  u_int32_t dummy2[0x3fa];
  u_int32_t fastbus_timeout;               /* 2000 */
  u_int32_t fastbus_arbitration_level;     /* 2004 */
  u_int32_t dummy3[2];
  u_int32_t vme_irq_level_and_vector;      /* 2010 */
  u_int32_t vme_irq_mask_register;         /* 2014 */
  u_int32_t next_sequencer_ram_address;    /* 2018 */
  u_int32_t reset_register_group_lca2;     /* 201c */
  u_int32_t sequencer_enable;              /* 2020 */
  u_int32_t sequencer_disable;             /* 2024 */
  u_int32_t sequencer_ram_load_enable;     /* 2028 */
  u_int32_t sequencer_ram_load_disable;    /* 202c */
  u_int32_t sequencer_reset;               /* 2030 */
  u_int32_t dummy4[1];
  u_int32_t clear_sequencer_cmd_flag;      /* 2038 */
  u_int32_t dummy5[0x37f1];
  u_int32_t seq[0x10000];                 /* 10000 */
  } *sfi_w;

typedef volatile struct sfi_r
  {
  u_int32_t dummy0[0x400];
  u_int32_t read_int_fb_ad;                /* 1000 */
  u_int32_t last_primadr;                  /* 1004 */
  u_int32_t dummy1[0x3fe];
  u_int32_t fastbus_timeout;               /* 2000 */
  u_int32_t fastbus_arbitration_level;     /* 2004 */
  u_int32_t fastbus_protocol_signal;       /* 2008 */
  u_int32_t fifo_flags;                    /* 200c */
  u_int32_t vme_irq_level_and_vector;      /* 2010 */
  u_int32_t vme_irq_mask_register;         /* 2014 */
  u_int32_t next_sequencer_ram_address;    /* 2018 */
  u_int32_t last_sequencer_protocol;       /* 201c */
  u_int32_t sequencer_status;              /* 2020 */
  u_int32_t fastbus_status_1;              /* 2024 */
  u_int32_t fastbus_status_2;              /* 2028 */
  u_int32_t dummy2[0x7f5];
  u_int32_t read_seq2vme_fifo;             /* 4000 */
  } *sfi_r;

/* sequencer commands (fastbus) */        /* A2=1 */
#define SFI_FUNC_PA                 1     /* 0 */
#define SFI_FUNC_PA_HM              5     /* 1 */
#define SFI_FUNC_DISCON             9     /* 2 */
#define SFI_FUNC_DISCON_RM         13     /* 3 */
#define SFI_FUNC_RNDM              17     /* 4 */
#define SFI_FUNC_RNDM_DIS          21     /* 5 */
#define SFI_FUNC_CLEANUP           25     /* 6 */
#define SFI_FUNC_CLEANUP_DIS       29     /* 7 */
/*#define SFI_FUNC_NOP             33*/   /* 8 */
#define SFI_FUNC_VMEADDR           37     /* 9 */
#define SFI_FUNC_DMA_CWC           41     /* a */
#define SFI_FUNC_DMA               45     /* b */
/*#define SFI_FUNC_NOP             49*/   /* c */
#define SFI_FUNC_STORE_ADDR        53     /* d */
#define SFI_FUNC_STORE_STAT_COUNT  57     /* e */
#define SFI_FUNC_STORE_COUNT       61     /* f */

/* sequencer modifier */
#define SFI_MS0  0x40
#define SFI_MS1  0x80
#define SFI_MS2 0x100
#define SFI_RD  0x200
#define SFI_EG  0x400

/* sequencer commands (control) */        /* A3=1 */
#define SFI_CFUNC_OUT               2     /* 0 */
#define SFI_CFUNC_DISABLE           6     /* 1 */
#define SFI_CFUNC_ENABLE_RAM       10     /* 2 */
#define SFI_CFUNC_DISABLE_RAM      14     /* 3 */
#define SFI_CFUNC_WAIT_GO_FLAG     18     /* 4 */
#define SFI_CFUNC_CLEAR_GO_FLAG    22     /* 5 */
#define SFI_CFUNC_SET_CMD_FLAG     26     /* 6 */

#define FB_HOLD_MASTER

/* fastbus commands */
#define seq_prim_hm_dsr   (        SFI_FUNC_PA_HM)
#define seq_prim_hm_csr   (SFI_MS0|SFI_FUNC_PA_HM)
#define seq_prim_hm_dsrm  (SFI_MS1|SFI_FUNC_PA_HM)
#define seq_prim_hm_csrm  (SFI_MS1|SFI_MS0|SFI_FUNC_PA_HM)
#ifndef FB_HOLD_MASTER
#define seq_prim_dsr      (        SFI_FUNC_PA)
#define seq_prim_csr      (SFI_MS0|SFI_FUNC_PA)
#define seq_prim_dsrm     (SFI_MS1|SFI_FUNC_PA)
#define seq_prim_csrm     (SFI_MS1|SFI_MS0|SFI_FUNC_PA)
#else
#define seq_prim_dsr      seq_prim_hm_dsr
#define seq_prim_csr      seq_prim_hm_csr
#define seq_prim_dsrm     seq_prim_hm_dsrm
#define seq_prim_csrm     seq_prim_hm_csrm
#endif
#define seq_secad_w       (SFI_MS1|SFI_FUNC_RNDM)
#define seq_secad_r       (SFI_RD|SFI_MS1|SFI_FUNC_RNDM)
#define seq_rndm_w        (        SFI_FUNC_RNDM)
#define seq_rndm_r        (SFI_RD| SFI_FUNC_RNDM)
#define seq_rndm_w_dis    (        SFI_FUNC_RNDM_DIS)
#define seq_rndm_r_dis    (SFI_RD| SFI_FUNC_RNDM_DIS)
#define seq_discon        (SFI_FUNC_DISCON)
#define seq_discon_rm     (SFI_FUNC_DISCON_RM)
#define seq_cleanup       (SFI_FUNC_CLEANUP)
#define seq_cleanup_dis   (SFI_FUNC_CLEANUP_DIS)
#define seq_store_wc      (SFI_FUNC_STORE_COUNT)
#define seq_store_statwc  (SFI_FUNC_STORE_STAT_COUNT)
#define seq_store_ap      (SFI_FUNC_STORE_ADDR)
#define seq_dma_w_clearwc (SFI_FUNC_DMA_CWC)
#define seq_dma_r_clearwc (SFI_RD|SFI_FUNC_DMA_CWC)
#define seq_dma_w         (SFI_FUNC_DMA)
#define seq_dma_r         (SFI_RD|SFI_FUNC_DMA)
#define seq_load_address  (SFI_FUNC_VMEADDR)

/* control commands */
#define seq_out           (SFI_CFUNC_OUT)
#define seq_disable       (SFI_CFUNC_DISABLE)
#define seq_enable_ram    (SFI_CFUNC_ENABLE_RAM)
#define seq_disable_ram   (SFI_CFUNC_DISABLE_RAM)
#define seq_set_cmd_flag  (SFI_CFUNC_SET_CMD_FLAG)
#define seq_wait_go_flag  (SFI_CFUNC_WAIT_GO_FLAG)
#define seq_clear_go_flag (SFI_CFUNC_CLEAR_GO_FLAG)

#endif
