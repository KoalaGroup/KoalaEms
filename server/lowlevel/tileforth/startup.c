/******************************************************************************
*                                                                             *
* startup.c                                                                   *
*                                                                             *
* created 13.09.94                                                            *
* last changed 23.09.94                                                       *
*                                                                             *
******************************************************************************/

#include <errno.h>
#include "nforth.h"
#include <stdio.h>
#include <stdlib.h>
#include "out.h"
#include "names.h"
#include "../../../common/requesttypes.h"
#include "../../procs/listprocs.h"

/*****************************************************************************/

static char startup0[]=
"\
vocabulary v_requests \
vocabulary v_procedures \
v_server definitions \
: Requests create requestidx 1 < if .\" not found\" then , does> \
@ request ; \
: Requests_ create , does> @ request ; \
: Procedures create , does> @ localprocedure ; \
forth definitions \
v_requests definitions \
\n";

static char startup1[]=
"\
v_procedures definitions \
\n";

static char startup2[]=
"\
forth only \
vocabulary v_user \
v_user definitions \
\n";

/*****************************************************************************/

static void printres()
{
char* s;

if (s=get_outstring())
  {
  printf("forth:\n  %s\n", s);
  free(s);
  }
}

/*****************************************************************************/

void forth_startup()
{
char s[1024];
char last[64];
char first[64];
int i;

_forthiere(startup0);
printres();

for (i=0; i<NrOfReqs; i++)
  {
  sprintf(s, "Requests %s %s \n", reqstrs[i], reqstrs[i]);
  _forthiere(s);
  printres();
  }

_forthiere(startup1);
printres();

for (i=0; i<NrOfProcs; i++)
  {
  sprintf(s, "%d Procedures %s.%d \n", i, Proc[i].name_proc, *Proc[i].ver_proc);
  _forthiere(s);
  printres();
  }
_forthiere(startup2);
printres();
}

/*****************************************************************************/
/*****************************************************************************/
