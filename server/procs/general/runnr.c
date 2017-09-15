/*
 * server/procs/general/runnr.c
 * created: 2006-Nov-19 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: runnr.c,v 1.4 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../objects/ved/ved.h"

extern ems_u32* outptr;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/general")

/*****************************************************************************/
/*
 * p[0]: #args (==1)
 * p[1]: runnr
 */
plerrcode proc_set_runnr(ems_u32* p)
{
    ved_globals.runnr=p[1];
    return plOK;
}

plerrcode test_proc_set_runnr(ems_u32* p)
{
    if (p[0]!=1)
        return plErr_ArgNum;
    wirbrauchen=0;
    return plOK;
}

char name_proc_set_runnr[]="set_runnr";
int ver_proc_set_runnr=1;
/*****************************************************************************/
plerrcode proc_get_runnr(ems_u32* p)
{
    *outptr++=ved_globals.runnr;
    return plOK;
}

plerrcode test_proc_get_runnr(ems_u32* p)
{
    if (p[0]!=0)
        return plErr_ArgNum;
    wirbrauchen=1;
    return plOK;
}

char name_proc_get_runnr[]="get_runnr";
int ver_proc_get_runnr=1;
/*****************************************************************************/
/*****************************************************************************/
