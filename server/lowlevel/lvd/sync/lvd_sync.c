/*
 * lowlevel/lvd/sync/lvd_sync.c
 * created 2006-Feb-07 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_sync.c,v 1.4 2011/04/06 20:30:26 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifndef HAVE_CGETCAP
#include <getcap.h>
#endif
#include <rcs_ids.h>
#include <modultypes.h>
#include "lvd_sync.h"
#include "../lvdbus.h"
#include "../lvd_initfuncs.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

RCS_REGISTER(cvsid, "lowlevel/lvd/sync")

/*****************************************************************************/
int lvd_sync_low_printuse(FILE* outfilepath)
{
    return 0;
}
/*****************************************************************************/
errcode lvd_sync_low_init(char* arg)
{
    lvdbus_register_cardtype(ZEL_LVD_MSYNCH, lvd_msync_acard_init);
    lvdbus_register_cardtype(ZEL_LVD_OSYNCH, lvd_osync_acard_init);
    return OK;
}
/*****************************************************************************/
errcode lvd_sync_low_done()
{
    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
