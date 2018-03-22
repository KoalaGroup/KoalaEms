/*
 * lowlevel/lvd/datafilter/lvd_datafilter.c
 * created 2010-Feb-03 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_datafilter.c,v 1.3 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <rcs_ids.h>
#include "lvd_datafilter.h"

RCS_REGISTER(cvsid, "lowlevel/lvd/datafilter")

/*****************************************************************************/
int lvd_datafilter_low_printuse(__attribute__((unused)) FILE* outfilepath)
{
    return 0;
}
/*****************************************************************************/
errcode lvd_datafilter_low_init(__attribute__((unused)) char* arg)
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
