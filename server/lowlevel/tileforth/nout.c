/******************************************************************************
*                                                                             *
* nout.c                                                                      *
*                                                                             *
* created 02.09.94                                                            *
* last changed 23.09.94                                                       *
*                                                                             *
******************************************************************************/

#include <string.h>
#include <stdlib.h>
#include "out.h"

/*****************************************************************************/

#define RESERVE 10

static char* outline;
static int outlen, usedlen;

/*****************************************************************************/

static void addstring(char* s)
{
int len;

len=strlen(s);
if (!outline)
  {
  outlen=len+RESERVE;
  outline=(char*)malloc(outlen+1);
  strcpy(outline, s);
  usedlen=len;
  }
else if ((usedlen+len)<=outlen)
  {
  strcpy(outline+usedlen, s);
  usedlen+=len;
  }
else
  {
  int newlen;
  char* newline;

  newlen=usedlen+len+RESERVE;
  newline=(char*)malloc(newlen+1);
  strcpy(newline, outline);
  free(outline);
  outline=newline;
  outlen=newlen;
  strcpy(outline+usedlen, s);
  usedlen+=len;
  }
}

/*****************************************************************************/

static void addchar(char c)
{
if (!outline)
  {
  outlen=RESERVE;
  outline=(char*)malloc(outlen+1);
  outline[0]=c; outline[1]=0;
  usedlen=1;
  }
else if ((usedlen+1)<=outlen)
  {
  outline[usedlen++]=c; outline[usedlen]=0;
  }
else
  {
  int newlen;
  char* newline;

  newlen=usedlen+RESERVE;
  newline=(char*)malloc(newlen+1);
  strcpy(newline, outline);
  free(outline);
  outline=newline;
  outlen=newlen;
  outline[usedlen++]=c; outline[usedlen]=0;
  }
}

/*****************************************************************************/

char* get_outstring()
{
char* s;

s=outline;
outline=0;
return(s);
}

/*****************************************************************************/

char s_err[1024];
char s_out[1024];

void emit_err()
{
addstring("\n");
addstring(s_err);
}

/*****************************************************************************/

void emit(char c)
{
addchar(c);
}

/*****************************************************************************/

void emit_s(char *s)
{
addstring(s);
}

/*****************************************************************************/

void emit_()
{
addstring(s_out);
}

/*****************************************************************************/

void out_initiate(char* banner)
{
outline=0;
}


/*****************************************************************************/

void out_finish()
{
if (outline) free(outline);
}


/*****************************************************************************/
/*****************************************************************************/
