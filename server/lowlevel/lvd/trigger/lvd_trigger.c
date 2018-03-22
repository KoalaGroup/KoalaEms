/*
 * lowlevel/lvd/trig/lvd_trig.c
 * created 2009-Nov-10 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_trigger.c,v 1.3 2017/10/20 23:21:31 wuestner Exp $";

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
#include <modultypes.h>
#include <rcs_ids.h>
#include "lvd_trigger.h"
#include "../lvdbus.h"
#include "../lvd_initfuncs.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

RCS_REGISTER(cvsid, "lowlevel/lvd/trigger")

/*****************************************************************************/
int lvd_trigger_low_printuse(__attribute__((unused)) FILE* outfilepath)
{
    return 0;
}
/*****************************************************************************/
errcode lvd_trigger_low_init(__attribute__((unused)) char* arg)
{
    lvdbus_register_cardtype(ZEL_LVD_TRIGGER, lvd_trig_acard_init);
    return OK;
}
/*****************************************************************************/
errcode lvd_trigger_low_done()
{
    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
