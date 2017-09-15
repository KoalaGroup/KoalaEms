/******************************************************************************
*                                                                             *
* c219_help.h                                                                 *
*                                                                             *
* OS9/UNIX                                                                    *
*                                                                             *
* created 05.12.94                                                            *
* last changed 20.10.95                                                       *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#ifndef _c219_help_h_
#define _c219_help_h_

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include "../../../objects/domain/dom_ml.h"

plerrcode c219_init(ml_entry*, int, int*,
    int (**)(void),
    void (**)(void),
    plerrcode (**)(void),
    int (**)(int));

#endif

/*****************************************************************************/
/*****************************************************************************/
