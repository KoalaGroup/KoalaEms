/******************************************************************************
*                                                                             *
* gsitrigger.h                                                                *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 11.10.93                                                                    *
*                                                                             *
******************************************************************************/

#ifndef _gsitrigger_h_
#define _gsitrigger_h_

/* register */

#define STATUS 0
#define CONTROL 1
#define FCATIME 2
#define CTIME 3

/*--- control register bits --------*/
#define MASTER     0x00000004
#define SLAVE      0x00000020
#define HALT       0x00000010
#define GO         0x00000002
#define EN_IRQ     0x00000001
#define DIS_IRQ    0x00000008
#define CLEAR      0x00000040
#define BUS_ENABLE 0x00000800
#define BUS_DISABLE 0x00001000

/*--- status register bits ---------*/
#define DT_CLEAR     0x00000020
#define IRQ_CLEAR    0x00001000
#define DI_CLEAR     0x00002000
#define EV_IRQ_CLEAR 0x00008000
#define EON          0x00008000
#define FC_PULSE     0x00000010
#define REJECT       0x00000080
#define SUBEV_INV    0x00000080
#define MISMATCH     0x00000040

#endif

/*****************************************************************************/
/*****************************************************************************/

