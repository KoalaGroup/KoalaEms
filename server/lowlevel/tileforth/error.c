/*
  C BASED FORTH-83 MULTI-TASKING KERNEL ERROR MANAGEMENT

  Copyright (C) 1989-1990 by Mikael Patel

  Computer Aided Design Laboratory (CADLAB)
  Department of Computer and Information Science
  Linkoping University
  S-581 83 LINKOPING
  SWEDEN

  Email: mip@ida.liu.se

  Started on: 7 March 1989

  Last updated on: 20 June 1990

  Dependencies:
       (cc) signal.h, fcntl.h, kernel.h, memory.h, io.h, error.h

  Description:
       Handles low level signal to error message conversion and printing.
       Low level signals from the run-time environment are transformation
       to forth level exceptions and may be intercepted by an exception
       block.

  Copying:
       This program is free software; you can redistribute it and/or modify
       it under the terms of the GNU General Public License as published by
       the Free Software Foundation; either version 1, or (at your option)
       any later version.

       This program is distributed in the hope that it will be useful,
       but WITHOUT ANY WARRANTY; without even the implied warranty of
       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
       GNU General Public License for more details.

       You should have received a copy of the GNU General Public License
       along with this program; see the file COPYING.  If not, write to
       the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <stdio.h>
/*#include <signal.h>*/
#include "kernel.h"
#include "memory.h"
#include "error.h"
#include "out.h"
#include "in.h"

/* ENVIRONMENT FOR LONGJMP AND RESTART AFTER ERROR SIGNAL */

jmp_buf restart;

#define MAXSIGNAL 31

VOID error_signal(int sig)
{
/* Check which task received the signal */
if (tp == foreground)
  {
  sprintf(s_err, "foreground#%d: ", foreground);
  emit_err();
  }
else
  {
  sprintf(s_err, "task#%d: ", tp);
  emit_err();
  }

/* Print the signal number and a short description */
if (sig /*< SIGNALMSGSIZE*/ <=MAXSIGNAL)
  {
  sprintf(s_err, "signal#%d\n", sig);
  emit_err();
  }
else
  {
  sprintf(s_err, "exception#%d: %s\n", sig, ((ENTRY) sig) -> name);
  emit_err();
  }

/* Abort the current virtual machine call */
doabort();
}

VOID error_fatal(int sig)
{
/* Notify the error signal */
error_signal(sig);

/* Clean up the mess after all the packages */
in_finish();
out_finish();
error_finish();
kernel_finish();
memory_finish();

/* Exit and pass on the signal number */
printf("Forth wollte exit ausfuehren.\n");
}

VOID error_restart(int sig)
{
error_signal(sig);
}

VOID error_initiate()
{
}

VOID error_finish()
{
/* Future clean up function for the error package */
}
