/******************************************************************************
*                                                                             *
* isvar.h                                                                     *
*                                                                             *
* 20.03.97                                                                    *
*                                                                             *
******************************************************************************/

#ifndef _isvar_h_
#define _isvar_h_

#include <errorcodes.h>
#include <sconf.h>

#ifdef ISVARS

plerrcode allocisvar(int size);
plerrcode reallocisvar(int size);
plerrcode freeisvar(void);
int isvarsize(void);
  
#endif /* ISVARS */

#endif

/*****************************************************************************/
/*****************************************************************************/
