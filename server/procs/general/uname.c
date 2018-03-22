/*
 * server/procs/general/uname.c
 * 
 * created: 14. Aug 2000 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: uname.c,v 1.8 2017/10/20 23:20:52 wuestner Exp $";

#include <sconf.h>
#include <errno.h>
#include <string.h>
#include <sys/utsname.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../procs.h"
#include <xdrstring.h>

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/general")

/*****************************************************************************/

plerrcode proc_Uname(__attribute__((unused)) ems_u32* p)
{
struct utsname name;

if (uname(&name)<0) return plErr_System;
*outptr++=5;
outptr=outstring(outptr, name.sysname);
outptr=outstring(outptr, name.nodename);
outptr=outstring(outptr, name.release);
outptr=outstring(outptr, name.version);
outptr=outstring(outptr, name.machine);
return plOK;
}

plerrcode test_proc_Uname(ems_u32* p)
{
if (p[0]!=0) return(plErr_ArgNum);
wirbrauchen=-1;
return plOK;
}

char name_proc_Uname[]="Uname";
int ver_proc_Uname=1;

/*****************************************************************************/
/*****************************************************************************/
