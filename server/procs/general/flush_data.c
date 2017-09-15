/*
 * server/procs/general/flush_data.c
 * 
 * created: 18.Apr.2004 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: flush_data.c,v 1.2 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
#include <errno.h>
#include <string.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../objects/do/dataout.h"

extern int *outptr;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/general")

/*****************************************************************************/
/*
 * p[0]: argnum
 * p[1...]: list of dataout indizes to be waited for (not yet implemented)
 */

plerrcode proc_flush_data(ems_u32* p)
{
    plerrcode res;


    res=flush_dataout(p+1, p[0]);
    return res;
}

plerrcode test_proc_flush_data(ems_u32* p)
{
    if (p[0]) return(plErr_ArgNum);
    wirbrauchen=0;
    return plOK;
}

char name_proc_flush_data[]="flush_data";
int ver_proc_flush_data=1;

/*****************************************************************************/
/*****************************************************************************/
