/*
 * debug.hxx
 * 
 * $ZEL: debug.hxx,v 2.6 2004/11/18 19:31:45 wuestner Exp $
 * 
 * created before 29.11.93 PW
 */

#ifndef _debug_hxx_
#define _debug_hxx_

//#include <stringhack.hxx>

#ifdef DEBUG
#include <cdefs.h>
#include <unistd.h>

#ifdef _main_c_
//#ifdef TRACE
//int debug=0x80000000;
//#else
int debug=0;
//#endif
int verbose=0;
#else /* _main_c_ */
extern int debug, verbose;
#endif /* _main_c_ */

#ifndef EOF
#include <stdio.h>
#endif /* EOF */

#define eins (unsigned int)1
#define D_TRACE (eins<<31)
#define D_COMM  (eins<<0)  /* Kommunikation */
#define D_TR    (eins<<1)  /* Transport */
#define D_DUMP  (eins<<2)  /* Dump */
#define D_MEM   (eins<<3)  /* Speicherverwaltung */
#define D_EV    (eins<<4)  /* Eventgroessen und -Header */
#define D_PL    (eins<<5)  /* Prozedurlisten */
#define D_USER  (eins<<6)  /* Userprozeduren */
#define D_REQ   (eins<<7)  /* Requestbearbeitung */
#define D_TRIG  (eins<<8)  /* Readout ... */
#define D_TIME  (eins<<9)  /* Timing */

#define D(m,x) {if(debug & (m)) {x}}
#define DV(m,x) {if(verbose&&(debug&(m))) {x}}
#define DL(x) {x}

#ifdef TRACE
#define TR(x) {if (debug & D_TRACE) cerr << __GNIRTS(x) << " entered" << endl;}
#else
#define TR(x)
#endif

#else /* DEBUG */

#define D(m,x)
#define DV(m,x)
#define DL(x)
#define TR(x)

#endif /* DEBUG */

#endif /* _debug_h_ */

/*****************************************************************************/
/*****************************************************************************/
