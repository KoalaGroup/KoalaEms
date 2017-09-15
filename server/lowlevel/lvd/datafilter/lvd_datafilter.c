/*
 * lowlevel/lvd/datafilter/lvd_datafilter.c
 * created 2010-Feb-03 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_datafilter.c,v 1.2 2011/04/06 20:30:26 wuestner Exp $";

#include <sconf.h>
#include <rcs_ids.h>
#include "lvd_datafilter.h"

RCS_REGISTER(cvsid, "lowlevel/lvd/datafilter")

/*****************************************************************************/
int lvd_datafilter_low_printuse(FILE* outfilepath)
{
    return 0;
}
/*****************************************************************************/
errcode lvd_datafilter_low_init(char* arg)
{
    return OK;
}
/*****************************************************************************/
errcode lvd_datafilter_low_done()
{
    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
