/******************************************************************************
*                                                                             *
* forth.c                                                                     *
*                                                                             *
* created 08.09.94                                                            *
* last changed 08.09.94                                                       *
*                                                                             *
******************************************************************************/

#include <errorcodes.h>
#include <xdrstring.h>
#include "../../lowlevel/tileforth/nforth.h"
#include "../../lowlevel/tileforth/out.h"

extern ems_u32* outptr;

/*****************************************************************************/

plerrcode proc_forth(p)
int* p;
{
char* s;

s=(char*)malloc(xdrstrclen(p+1)+2);
extractstring(s, p+1);
strcat(s, "\n");
forthiere(s);
free(s);
if (s=get_outstring())
  {
  outptr=outstring(outptr, s);
  free(s);
  }
return(plOK);
}

plerrcode test_proc_forth(p)
int* p;
{
if (p[0]<1) return(plErr_ArgNum);
if (p[0]!=xdrstrlen(p+1)) return(plErr_ArgNum);
return(plOK);
}

char name_proc_forth[]="Forth";
int ver_proc_forth=1;

/*****************************************************************************/
/*****************************************************************************/
