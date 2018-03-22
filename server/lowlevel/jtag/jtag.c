/*
 * lowlevel/jtag.c
 * created 12.Aug.2005 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: jtag.c,v 1.3 2017/10/22 21:29:37 wuestner Exp $";

#include <sconf.h>
#include <rcs_ids.h>
#include "jtag.h"

RCS_REGISTER(cvsid, "lowlevel/jtag")

/*****************************************************************************/
int
jtag_low_printuse(__attribute__((unused)) FILE* outfilepath)
{
    return 0;
}
/*****************************************************************************/
errcode
jtag_low_init(__attribute__((unused)) char* arg)
{
    return OK;
}
/*****************************************************************************/
errcode jtag_low_done()
{
    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
