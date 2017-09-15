/*
 * lowlevel/lvd/datafilter/filter_sync_statist.c
 * created 2010-Feb-11 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: filter_sync_statist.c,v 1.2 2011/04/06 20:30:26 wuestner Exp $";

#include <sconf.h>
#include <stdlib.h>
#include <rcs_ids.h>
#include "datafilter.h"

RCS_REGISTER(cvsid, "lowlevel/lvd/datafilter")

/*****************************************************************************/
plerrcode
filter_sync_statist_init(struct datafilter *filter)
{
    return plOK;
}
/*****************************************************************************/
plerrcode
filter_sync_statist_filter(struct datafilter *filter, ems_u32 *data, int *len)
{
    return plOK;
}
/*****************************************************************************/
/*****************************************************************************/
