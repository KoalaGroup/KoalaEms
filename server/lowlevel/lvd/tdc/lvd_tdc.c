/*
 * lowlevel/lvd/tdc/lvd_tdc.c
 * created 13.Dec.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_tdc.c,v 1.4 2017/10/20 23:21:31 wuestner Exp $";

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
#include "lvd_tdc.h"
#include "../lvdbus.h"
#include "../lvd_initfuncs.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

RCS_REGISTER(cvsid, "lowlevel/lvd/tdc")

/*****************************************************************************/
int lvd_tdc_low_printuse(__attribute__((unused)) FILE* outfilepath)
{
    return 0;
}
/*****************************************************************************/
errcode lvd_tdc_low_init(__attribute__((unused)) char* arg)
{
    lvdbus_register_cardtype(ZEL_LVD_TDC_F1,  lvd_f1_acard_init);
    lvdbus_register_cardtype(ZEL_LVD_TDC_GPX, lvd_gpx_acard_init);
    return OK;
}
/*****************************************************************************/
errcode lvd_tdc_low_done()
{
    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
