/******************************************************************************
*                                                                             *
* getevent.h                                                                  *
*                                                                             *
* ULTRIX/OSF1                                                                 *
*                                                                             *
* created before 23.03.94                                                     *
* last changed 21.01.95                                                       *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#ifndef _getevent_h_
#define _getevent_h_

#include <cdefs.h>

extern char grund[];

#define MAXEVENT (0x3fff)   /* 64 kByte -1 word */
/*#define MAXEVENT (0xF000) /* 240 kByte */

__BEGIN_DECLS

int* getevent(int path);
int* getevent_s(int path);

__END_DECLS

#endif

/*****************************************************************************/
/*****************************************************************************/
