/******************************************************************************
*                                                                             *
* xdrstring.v.c                                                               *
*                                                                             *
* OS9/ULTRIX                                                                  *
*                                                                             *
* created: 11.09.94                                                           *
* last changed: 23.09.94                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include <swap.h>
#include <xdrstring.h>

/*****************************************************************************/

VOID doxdrs()
{
char* s;

s=(char*)malloc(xdrstrclen(&sp->INT32)+1);
sp=(PTR)extractstring(s, &sp->INT32);
spush(s, CSTR);
}

NORMAL_CODE(xdrs, forth, "xdr$", doxdrs)

VOID doxdrstrdot()
{
char* s;

s=(char*)malloc(xdrstrclen(&sp->INT32)+1);
sp=(PTR)extractstring(s, &sp->INT32);
emit_s(s);
free(s);
}

NORMAL_CODE(xdrstrdot, xdrs, "xdrstr.", doxdrstrdot)

VOID dosxdr()
{
char* s;
int l;

s=spop(CSTR);
input_scan(thetib, '"');
l=strxdrlen(thetib);
outstring((&sp->INT32)-l, thetib);
sp=(PTR)((&sp->INT32)-l);
}

NORMAL_CODE(sxdr, xdrstrdot, "$xdr", dosxdr)

VOID doxdrstring()
{
int l;

input_scan(thetib, '"');
l=strxdrlen(thetib);
outstring((&sp->INT32)-l, thetib);
sp-=l;
}

NORMAL_CODE(xdrstring, sxdr, "xdr\"", doxdrstring)

/*****************************************************************************/
/*****************************************************************************/
