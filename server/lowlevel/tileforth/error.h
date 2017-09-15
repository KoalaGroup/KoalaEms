/*
  C BASED FORTH-83 MULTI-TASKING KERNEL ERROR MANAGEMENT DEFINITIONS

  Copyright (C) 1989-1990 by Mikael R.K. Patel

*/

#ifndef _error_h_
#define _error_h_

/* INCLUDE FILES: SETJUMP */

#include <setjmp.h>

/* WARM RESTART ENVIRONMENT FOR LONGJMP */

extern jmp_buf restart;

/* EXPORTED FUNCTIONS AND PROCEDURES */

VOID error_initiate();
VOID error_restart();
VOID error_fatal();
VOID error_signal();
VOID error_finish();

#endif

/*****************************************************************************/
/*****************************************************************************/
