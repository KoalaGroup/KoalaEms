/*
 * lowlevel/lvd/vertex/lvd_vertex.c
 * created 2005-Feb-25 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_vertex.c,v 1.6 2017/10/20 23:21:31 wuestner Exp $";

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
#include "lvd_vertex.h"
#include "../lvdbus.h"
#include "../lvd_initfuncs.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

RCS_REGISTER(cvsid, "lowlevel/lvd/vertex")

/*****************************************************************************/
int lvd_vertex_low_printuse(__attribute__((unused)) FILE* outfilepath)
{
    return 0;
}
/*****************************************************************************/
errcode lvd_vertex_low_init(__attribute__((unused)) char* arg)
{
    lvdbus_register_cardtype(ZEL_LVD_ADC_VERTEX, lvd_vertex_acard_init);
    lvdbus_register_cardtype(ZEL_LVD_ADC_VERTEXM3, lvd_vertex_acard_init);
    lvdbus_register_cardtype(ZEL_LVD_ADC_VERTEXUN, lvd_vertex_acard_init);
    return OK;
}
/*****************************************************************************/
errcode lvd_vertex_low_done()
{
    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
