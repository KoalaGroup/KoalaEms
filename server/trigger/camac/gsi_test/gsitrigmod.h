/******************************************************************************
*                                                                             *
* gsitrigmod.h for camac                                                      *
*                                                                             *
* OS9/UNIX                                                                    *
*                                                                             *
* created 05.12.94                                                            *
* last changed 05.12.94                                                       *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#ifndef _gsitrigmod_h_
#define _gsitrigmod_h_

#include <errorcodes.h>

plerrcode trigmod_init(int slot, int master, int ctime, int fcatime);
plerrcode trigmod_done();
int trigmod_get();
void trigmod_reset();
plerrcode trigmod_soft_init(int slot, int id, int ctime, int fcatime);
plerrcode trigmod_soft_done();
int trigmod_soft_get();
void trigmod_soft_reset();

#endif

/*****************************************************************************/
/*****************************************************************************/
