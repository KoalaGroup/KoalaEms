/******************************************************************************
*                                                                             *
* fbaccmacro.h                                                                *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* created: 21.10.94                                                           *
* last changed: 21.10.94                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#ifndef _fbaccmacro_h_
#define _fbaccmacro_h_

#include "globals.h"

/* int sscode()                                                      */
/* void FCPD_(int pa)           primary address cycle; data          */
/* void FCPC_(int pa)           primary address cycle; control       */
/* void FCPDM(int pa)          primary address cycle; multi; data    */
/* void FCPCM(int pa)          primary address cycle; multi; control */
/* void FCDISC()               disconnect                            */
/* int FCRW()                  read word                             */
/* int FCWW(int dat)           write word                            */
/* int FCRSA()                 read secondary address                */
/* int FCWSA(int sa)           write secondary address               */

#define sscode()  (*SSREGptr&0x7)
#define FCPD_(pa)  *DGEOptr=(pa)
#define FCPC_(pa)  *CGEOptr=(pa)
#define FCPDM(pa) *DBREGptr=(pa)
#define FCPCM(pa) *CBREGptr=(pa)
#define FCDISC()  *DISCONptr=0
#define FCRW()    (*RNDMptr)
#define FCWW(dat) *RNDMptr=dat
#define FCRSA()   (*SECADptr)
#define FCWSA(sa) *SECADptr=(sa)

#endif

/*****************************************************************************/
/*****************************************************************************/
