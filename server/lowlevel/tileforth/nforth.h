/******************************************************************************
*                                                                             *
* nforth.h                                                                    *
*                                                                             *
* created 07.09.94                                                            *
* last changed 23.09.94                                                       *
*                                                                             *
******************************************************************************/

#ifndef _nforth_h_
#define _nforth_h_

extern void io_dispatch();

extern char* zeile(char*);

extern int fort_initiate();
extern int forth_finish();

extern void forthiere(char*);
extern void _forthiere(char*);

#endif

/*****************************************************************************/
/*****************************************************************************/
