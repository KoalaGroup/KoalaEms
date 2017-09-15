/******************************************************************************
*                                                                             *
* chi_map.h                                                                   *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* created before: 09.02.93                                                    *
* last changed: 21.10.94                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#ifndef _chi_map_h_
#define _chi_map_h_

/* special memories */
#define R_BASE     0x00000000
#define M_BASE     0x20000000
#define P_BASE     0x48000000
#define F_BASE     0x58000000
#define IO_BASE    0x60000000
#define EA_BASE    0x80000000
#define X_BASE     0xC0000000
#define RTC_BASE   0xE0000000
#define DUART_BASE 0xF0000000
#define BRAM_BASE  0xE8000000
#define REG_BASE   0xF8000000

#define VIC0_BASE  0x60000000
#define VIC1_BASE  0x60000100
#define VIC2_BASE  0x60000200
#define VIC3_BASE  0x60000300

/* Basic set of primary address */
#define DGEO       0x20
#define CGEO       0x24
#define DBREG      0x28
#define CBREG      0x2c

/* Basic set of data cycles */
#define RNDM       0x4000
#define SECAD      0x4008
#define DISCON     0x4034


/* VIC_BASE */
#define CVIC_STA   0x4
#define CVIC_CTR   0xc

#define VIC_RESET  0x4c
#define C_LOCRESET 0x48

#define VIC_CMEM   0xA0
#define VIC_CMEMI  0xa4
#define VIC_CADR   0xb0

/* Bits in CVIC_STA (RS16) */
#define IOCONF2       0x8000
#define IOCONF1       0x4000
#define IOCONF0       0x2000
#define VIC_SLAVE_IRQ 0x1000
#define CHI_IRQ_CSR   0x800
#define CHI_RES_EN    0x400
#define CHI_TST_IRQ   0x200
#define CHI_TEST_EN   0x100
#define LCA_BIT1      0x80
#define LCA_BIT0      0x40
#define S_VICbus_IRQ  0x20
#define VICbus_IRQ    0x10
#define VIC_IRQ_CSR   0x8
#define VIC_RESET_EN  0x4
#define VIC_TST_IRQ   0x2
#define VIC_TEST_EN   0x1

/* Bits in CVIC_CTR */
/*#define CHI_RES_EN    0x400*/
#define CHI_TST_SIRQ  0x200
/*#define CHI_TEST_EN   0x100*/
#define CHI_RES_DI    0x2
#define CHI_TST_CIRQ  0x1
#define CHI_TEST_DI   0x0

/* M_BASE */
#define DFIM       0x000        /* relative to M_BASE   */
#define DSR        0x800        /* relative to M_BASE   */

/* REG_BASE */
#define POLLREG0   0x0          /* relative to REG_BASE */
#define POLLREG1   0x1          /* relative to REG_BASE */
#define IMASKREG   0x2          /* relative to REG_BASE */
#define LOCKREG    0x4          /* relative to REG_BASE */
#define TREG       0x6          /* relative to REG_BASE */
#define H_STATUS   0x7          /* relative to REG_BASE */
#define ASTATUS    0x8          /* relative to REG_BASE */

/* F_BASE */
#define FSR        0x000
#define RESGK      0x050
#define GAREG      0x100        /* relative to F_BASE */
#define FBCR0      0x200        /* relative to F_BASE */
#define FBCR1      0x201        /* relative to F_BASE */
#define FBCR2      0x202        /* relative to F_BASE */
#define RDM        0x404        /* relative to F_BASE */
#define RDSTA      0x41c        /* relative to F_BASE */
#define RDVC       0x428        /* relative to F_BASE */
#define LDWC       0x500        /* relative to F_BASE */
#define RDWC       0x504        /* relative to F_BASE */
#define LDAC       0x508        /* relative to F_BASE */
#define RDAC       0x50c        /* relative to F_BASE */
#define WRCR       0x510        /* relative to F_BASE */
#define RDCR       0x514        /* relative to F_BASE */
#define ENCT       0x51c        /* relative to F_BASE */
#define SSREG      0x818        /* relative to F_BASE */
#define SSENABLE   0xb00        /* relative to F_BASE */

#define EXARB      0x4000

/* FBCR0 */
#define FAS        0x80
#define FGK        0x40

/* a_prot */
#define A_PROT_NORM    0x0000
#define A_PROT_PRIOR   0x1000
#define A_PROT_ASSUR   0x2000
#define A_PROT_DISABLE 0x3000

/* masks in LOCKREG */
#define NOTIME     0x80
#define SLOW       0x40
#define LEVEL      0x20
#define ENLEVEL    0x10
#define LATCH      0x08
#define NOSLAVE    0x04
#define NOFB       0x02
#define NOIO       0x01

/* masks in POLLREG0 */
#define FIMPEND    0x80
#define SRPEND     0x40
#define MASTER     0x20
#define FDONE      0x10
#define BBUSY      0x08
#define IAS        0x04
#define BROAD      0x01
#define EXTERN     0x02

/* masks in POLLREG1 */
#define CSR        0x80
#define ASAK       0x40
#define IAK        0x20
#define PRCARRY    0x10
#define ARBUSY     0x08
#define TOP        0x04
#define IREQ       0x02
#define ARBAK      0x01

/* masks in IMASKREG */
#define IM7        0x80
#define IM6        0x40
#define IM5        0x20
#define IM4        0x10
#define IM3        0x08
#define IM2        0x04
#define IM1        0x02
#define IM0        0x01

typedef struct
  {
  char MRA;
  char CSRA;
  char CRA;
  char TBA;
  char ACR;
  char IMR;
  char CTUR;
  char CTLR;
  char MRB;
  char CSRB;
  char CRB;
  char TBB;
  char IVR;
  char OPCR;
  char OPR_set;
  char OPR_res;
  } DUART_struct;

typedef struct
  {
  char rtc_cont;
  char sec_01;
  char sec_1;
  char sec_10;
  char min_1;
  char min_10;
  char hours_1;
  char hours_10;
  char days_1;
  char days_10;
  char month_1;
  char month_10;
  char years_1;
  char years_10;
  char day_of_week;
  char cs_ir;
  } RTC_struct;

#endif

/*****************************************************************************/
/*****************************************************************************/
