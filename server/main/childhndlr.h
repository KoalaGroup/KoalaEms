/******************************************************************************
*                                                                             *
* childhndlr.h                                                                *
*                                                                             *
* created 08.10.97                                                            *
* last changed 08.10.97                                                       *
*                                                                             *
******************************************************************************/

#ifndef _childhndlr_h_
#define _childhndlr_h_

#include <sys/types.h>
#include "signals.h"
#include "callbacks.h"

typedef void (*childproc)(pid_t, int, union callbackdata);
int install_childhandler(childproc, pid_t, union callbackdata, const char*);
void child_dispatcher(int);

#endif
