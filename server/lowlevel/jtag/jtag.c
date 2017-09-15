/*
 * lowlevel/jtag.c
 * created 12.Aug.2005 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: jtag.c,v 1.2 2011/04/06 20:30:24 wuestner Exp $";

#include <sconf.h>
#include <rcs_ids.h>
#include "jtag.h"

RCS_REGISTER(cvsid, "lowlevel/jtag")

/*****************************************************************************/
int
jtag_low_printuse(FILE* outfilepath)
{
    return 0;
}
/*****************************************************************************/
errcode
jtag_low_init(char* arg)
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
