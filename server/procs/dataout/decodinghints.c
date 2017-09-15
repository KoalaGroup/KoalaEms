/*
 * server/procs/dataout/decodinghints.c
 * created: 2007-02-08 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: decodinghints.c,v 1.5 2011/04/06 20:30:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <string.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../objects/is/is.h"

extern ems_u32* outptr;
extern int wirbrauchen;
extern struct IS *current_IS;

RCS_REGISTER(cvsid, "procs/dataout")

/*****************************************************************************/
/*
 * p[0]: argnum (0|1)
 * [p[1]: flags]
 */

plerrcode proc_decoding_hints(ems_u32* p)
{
    if (p[0])
        current_IS->decoding_hints=p[1];
    else
        *outptr++=current_IS->decoding_hints;
    return OK;
}

plerrcode test_proc_decoding_hints(ems_u32* p)
{
    if (current_IS==0)
        return plErr_Verify;
    switch (p[0]) {
    case 0:
        wirbrauchen=1;
        break;
    case 1:
        wirbrauchen=0;
        break;
    default:
        return plErr_ArgNum;
    }
    return plOK;
}

char name_proc_decoding_hints[]="IS_decoding_hints";
int ver_proc_decoding_hints=1;
/*****************************************************************************/
/*
 * p[0]: argnum (0|1)
 * p[1]: flags
 */

plerrcode proc_importance(ems_u32* p)
{
    if (p[0])
        current_IS->importance=p[1];
    else
        *outptr++=current_IS->importance;
    return OK;
}

plerrcode test_proc_importance(ems_u32* p)
{
    if (current_IS==0)
        return plErr_Verify;
    switch (p[0]) {
    case 0:
        wirbrauchen=1;
        break;
    case 1:
        wirbrauchen=0;
        break;
    default:
        return plErr_ArgNum;
    }
    return plOK;
}

char name_proc_importance[]="IS_importance";
int ver_proc_importance=1;
/*****************************************************************************/
/*****************************************************************************/
