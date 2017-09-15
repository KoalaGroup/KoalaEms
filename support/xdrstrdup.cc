/*
 * xdrstrdup.cc
 * 
 * created 19.11.1994 PW
 * 05.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#include <errno.h>

#include <xdrstring.h>
#include "xdrstrdup.hxx"
#include "errors.hxx"
#include "versions.hxx"

VERSION("Jun 06 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: xdrstrdup.cc,v 2.9 2004/11/26 14:45:43 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

ems_u32* xdrstrdup(char*& s, const ems_u32* ptr)
{
int l, i;
ems_u32* ptr1;

l=xdrstrclen(ptr);

if ((s=new char[l+1])==0)
  throw new C_unix_error(errno, "xdrstrdup");

ptr1=extractstring(s, ptr);
for (i=0; i<l; i++) if (s[i]==0xd) s[i]='\n';
return(ptr1);
}

/*****************************************************************************/

char* xdrstrdup(const ems_u32* ptr)
{
char* s;
int l, i;

l=xdrstrclen(ptr);
if ((s=new char[l+1])==0)
  throw new C_unix_error(errno, "xdrstrdup");

extractstring(s, ptr);
for (i=0; i<l; i++) if (s[i]==0xd) s[i]='\n';
return(s);
}

/*****************************************************************************/
/*****************************************************************************/
