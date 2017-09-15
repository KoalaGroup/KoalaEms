/******************************************************************************
*                                                                             *
* fopengz.h                                                                   *
*                                                                             *
* ULTRIX/OSF1                                                                 *
*                                                                             *
* created 21.06.95                                                            *
* last changed 21.06.95                                                       *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#ifndef _fopengz_h_
#define _fopengz_h_

#include <stdio.h>
#include <cdefs.h>

__BEGIN_DECLS

FILE* fopengz(const char* name, int* flag);

int fclosegz(FILE* f, int flag);

__END_DECLS

#endif

/*****************************************************************************/
/*****************************************************************************/
