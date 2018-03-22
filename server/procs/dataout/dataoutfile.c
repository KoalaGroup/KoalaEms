/*
 * server/procs/dataout/dataoutfile.c
 * 
 * created: 24.Jul.2004 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: dataoutfile.c,v 1.6 2017/10/09 21:25:37 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <string.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../dataout/dataout.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/dataout")

/*****************************************************************************/
/*
 * p[0]: argnum
 * p[1]: do_ID
 */

plerrcode proc_get_dataoutfile(ems_u32* p)
{
    const char *filename;
    plerrcode pres;

    if ((pres=current_filename(p[1], &filename))!=plOK)
        return pres;

    printf("get_dataoutfile: name=%s\n", filename);

    outptr=outstring(outptr, filename);

    return OK;
}

plerrcode test_proc_get_dataoutfile(ems_u32* p)
{
    if (p[0]!=1) return plErr_ArgNum;
    if (p[1]>=MAX_DATAOUT) return plErr_ArgRange;
    wirbrauchen=-1;
    return plOK;
}

char name_proc_get_dataoutfile[]="get_dataoutfile";
int ver_proc_get_dataoutfile=1;

/*****************************************************************************/
/*****************************************************************************/
