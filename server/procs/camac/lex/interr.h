/* CNAF-Befehlsfunktionsnummern fuer JIR 10 (Interrupt-Modul (IM)) */

#define IM_READ_IR      0       /* nAFread(5,0,0) */
#define IM_READ_MR      1       /* nAFread(5,0,1) */
#define IM_READ_IR_CM   2       /* nAFread(5,0,2) */
#define IM_TEST_LAM     8       /* nAFread(5,0,8) */
#define IM_CLEAR_IR     9       /* nAFcntl(5,0,9) */
#define IM_OWRITE_MR    17      /* nAFwrite(5,0,17,0xFFFF) */
#define IM_SCIR_SSMR    19      /* nAFwrite(5,0,19,0x?) */
#define IM_DIS_LAM      24      /* nAFcntl(5,0,24) */
#define IM_ENA_LAM      26      /* nAFcntl(5,0,26) */
#define IM_INIT         28      /* nAFcntl(5,0,28) */

#define IM_SUB_ADR      0       /* Subadresse (nicht benoetigt) */
