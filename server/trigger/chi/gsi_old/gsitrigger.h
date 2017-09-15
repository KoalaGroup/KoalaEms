/******************************************************************************
*                                                                             *
* gsitrigger.h for chi                                                        *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 17.03.94                                                                    *
*                                                                             *
******************************************************************************/

#ifndef _gsitrigger_h_
#define _gsitrigger_h_
#include <sconf.h>

/* register */

#define GSI_BASE ((char*)0x60000000)
#define STATUS  0x0
#define CONTROL 0x4
#define FCATIME 0x8
#define CTIME   0xc

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

