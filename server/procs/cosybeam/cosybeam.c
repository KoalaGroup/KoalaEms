/*
 * procs/cosybeam/cosybeam.c
 *
 * created 06.Aug.2001 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: cosybeam.c,v 1.6 2017/10/09 21:25:37 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../lowlevel/cosybeam/cosybeam.h"
#include "../procs.h"
#include "../procprops.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/cosybeam")

/*****************************************************************************/
/*
 * p[0]: number of arguments (==3)
 * p[1]: number of requested data
 * p[2]: don't suppress old data
 * p[3]: wait for semaphore
 */
plerrcode proc_cosybeam(ems_u32* p)
{
    return cosybeam_get_data(&outptr, p[1], p[2], p[3]);
}
/*****************************************************************************/
plerrcode test_proc_cosybeam(ems_u32* p)
{
    if (p[0]!=3) return(plErr_ArgNum);
    wirbrauchen=p[1]+3;
    return plOK;
}
/*****************************************************************************/
char name_proc_cosybeam[]="CosyBeam";
int ver_proc_cosybeam=1;
/*****************************************************************************/
/*****************************************************************************/
