/*
 * server/procs/dataout/fadvise.c
 * created: 2008-Aug-26 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fadvise.c,v 1.3 2011/04/06 20:30:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <string.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../objects/domain/dom_dataout.h"
#include "../../objects/do/dataout.h"

extern ems_u32* outptr;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/dataout")

/*****************************************************************************/
/*
 * p[0]: argnum (>=1)
 * p[1]: do_ID
 * p[2]: fadvise flag (
 *          0: don't use fadvise
 *          1: use POSIX_FADV_DONTNEED
 *          2: use fdatasync (not recommended)
 *          4: to be defined ...
 *       )
 * p[3]: data
 */

plerrcode proc_fadvise(ems_u32* p)
{
    if (dataout[p[1]].bufftyp==-1)
        return plErr_ArgRange;
    if (dataout[p[1]].addrtyp!=Addr_File)
        return plErr_ArgRange;

    if (p[0]==1) {
        *outptr++=dataout[p[1]].fadvise_flag;
        *outptr++=dataout[p[1]].fadvise_data;
    } else {
        dataout[p[1]].fadvise_flag=p[2];
        if (p[0]>2)
            dataout[p[1]].fadvise_data=p[3];
        else
            dataout[p[1]].fadvise_data=0;
    }
    return OK;
}

plerrcode test_proc_fadvise(ems_u32* p)
{
    if (p[0]<1)
        return plErr_ArgNum;
    if (p[0]==1)
        wirbrauchen=2;
    else
        wirbrauchen=0;
    return plOK;
}

char name_proc_fadvise[]="fadvise";
int ver_proc_fadvise=1;
/*****************************************************************************/
/*****************************************************************************/
