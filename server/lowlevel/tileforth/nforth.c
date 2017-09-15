/******************************************************************************
*                                                                             *
* nforth.c                                                                    *
*                                                                             *
* created 07.09.94                                                            *
* last changed 23.09.94                                                       *
*                                                                             *
******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "kernel.h"
#include "error.h"
#include "memory.h"
#include "out.h"
#include "in.h"
#include "nforth.h"
#include "startup.h"

/* STRUCTURE SIZES */

#define DICTIONARYSIZE 512 * 1024
#define USERSIZE 1024
#define PARAMSIZE 256
#define RETURNSIZE 256

#define BANNER "Das ist forth"

/*****************************************************************************/

void io_dispatch()
{
/* Any application action which requires periodical attention */
}

/*****************************************************************************/

static char* line;

char* zeile(char* old)
{
if (line)
  {
  char* s;
  s=line;
  line=0;
  return(s);
  }
else
  {
  return(0);
  }
}

/*****************************************************************************/

int fort_initiate(char *arg)
{
int dictionarysize, usersize, paramsize, returnsize;
int val;

/* Initiate default size values */
dictionarysize = DICTIONARYSIZE;
usersize = USERSIZE;
paramsize = PARAMSIZE;
returnsize = RETURNSIZE;

if (arg)
  {
  val=0;
  cgetnum(arg, "forth_dict", &val);
  if (val) dictionarysize=val;
  val=0;
  cgetnum(arg, "forth_user", &val);
  if (val) usersize=val;
  val=0;
  cgetnum(arg, "forth_param", &val);
  if (val) paramsize=val;
  val=0;
  cgetnum(arg, "forth_return", &val);
  if (val) returnsize=val;
  }

/* Initiate memory, error, io, and kernel */
out_initiate(BANNER);
in_initiate();
error_initiate();
memory_initiate(dictionarysize);
kernel_initiate(0, 0, usersize, paramsize, returnsize);

verbose = TRUE;
rinit();
doleftbracket();

forth_startup();
return(0);
}

/*****************************************************************************/

void forthiere(char* s)
{
char* ss;

ss=(char*)malloc(strlen(s)+2);
strcpy(ss, s);
strcat(ss, "\n");
line=ss;
dointerpret();
free(ss);
}

/*****************************************************************************/

void _forthiere(char* s)
{
line=s;
dointerpret();
}

/*****************************************************************************/

int forth_finish()
{
/* Clean up the kernel, io, error and memory package before exit */
kernel_finish();
memory_finish();
error_finish();
in_finish();
out_finish();
return(0);
}

/*****************************************************************************/
/*****************************************************************************/
