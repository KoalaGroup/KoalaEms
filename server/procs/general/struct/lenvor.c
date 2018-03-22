/*
 * procs/general/struct/lenvor.c
 * created before 17.06.94
 */

static const char* cvsid __attribute__((unused))=
    "$ZEL: lenvor.c,v 1.7 2017/10/09 21:25:37 wuestner Exp $";

#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../procprops.h"
#include "../../proclist.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/general/struct")

plerrcode proc_LenVor(ems_u32* p)
{
    ems_u32* help;
    help=outptr++;
    scan_proclist(++p);
    *help=outptr-help-1;
    return(plOK);
}

plerrcode test_proc_LenVor(ems_u32* p)
{
    ssize_t limit;
    if (*p<1) return(plErr_ArgNum);
    if (test_proclist(p+1,*p,&limit))
            return(plErr_RecursiveCall);
    if (limit>=0) wirbrauchen=limit+1;
    return(plOK);
}
#ifdef PROCPROPS
static procprop LenVor_prop={1, -1, 0, 0};

procprop* prop_proc_LenVor()
{
    return(&LenVor_prop);
}
#endif
char name_proc_LenVor[]="LenVor";
int ver_proc_LenVor=1;

/*****************************************************************************/
/*****************************************************************************/
