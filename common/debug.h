/******************************************************************************
*                                                                             *
* debug.h                                                                     *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 13.10.94                                                                    *
*                                                                             *
******************************************************************************/

#ifndef _debug_h_
#define _debug_h_

#ifdef DEBUG

#include <cdefs.h>

#ifdef _main_c_
#ifdef TRACE
int debug=0x80000000;
#else
int debug=0;
#endif
int verbose=0;
#else /* _main_c_ */
extern int debug, verbose;
#endif /* _main_c_ */

#ifndef EOF
#include <stdio.h>
#endif /* EOF */

#define D_TRACE (1<<31)
#define D_COMM  (1<<0)  /* Kommunikation */
#define D_TR    (1<<1)  /* Transport */
#define D_DUMP  (1<<2)  /* Dump */
#define D_MEM   (1<<3)  /* Speicherverwaltung */
#define D_EV    (1<<4)  /* Eventgroessen und -Header */
#define D_PL    (1<<5)  /* Prozedurlisten */
#define D_USER  (1<<6)  /* Userprozeduren */
#define D_REQ   (1<<7)  /* Requestbearbeitung */
#define D_TRIG  (1<<8)  /* Readout ... */
#define D_TIME  (1<<9)  /* Timing */
#define D_FORK  (1<<10) /* extra processes (tapehandler...) */
#define D_LOW   (1<<11) /* Lowlevel (Initalisierung) */

#define D(m,x) if(debug & (m)) {x}
#define DV(m,x) if(verbose&&(debug&(m))) {x}

#define PRNAME(x) __STRING(trace: x\n)

#define T(x) if (debug & D_TRACE) fprintf(stderr, PRNAME(x));

#else /* DEBUG */
#define D(m,x)
#define DV(m,x)
#define T(x)
#endif /* DEBUG */

#endif /* _debug_h_ */

/*****************************************************************************/
/*****************************************************************************/
