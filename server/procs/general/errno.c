/*
 * server/procs/general/errno.c
 * 
 * created: 11. Aug 2000 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: errno.c,v 1.5 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
#include <errno.h>
#include <string.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../procs.h"
#include <xdrstring.h>

extern ems_u32* outptr;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/general")

/*****************************************************************************/

plerrcode proc_Strerror(ems_u32* p)
{
if (p[0]==1)
  {
  char* s;
  s=strerror((int)p[1]);
  outptr=outstring(outptr, s);
  }
else
  {
  int a, b, i;
  if (p[0]==0)
    {a=0; b=100;}
  else
    {a=p[1]; b=p[2];}
  *outptr++=b-a+1;
  for (i=a; i<=b; i++)
    {
    char* s;
    s=strerror(i);
    outptr=outstring(outptr, s);
    }
  }
return plOK;
}

plerrcode test_proc_Strerror(ems_u32* p)
{
if ((p[0]<0) || (p[0]>2)) return(plErr_ArgNum);
wirbrauchen=-1;
return plOK;
}

char name_proc_Strerror[]="Strerror";
int ver_proc_Strerror=1;

/*****************************************************************************/
/*****************************************************************************/
