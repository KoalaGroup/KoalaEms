/*
 * lowlevel/ipmod/ipmod.c
 * created 2015-Dec-19 PeWue
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: ipmod.c,v 1.4 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "ipmod.h"
#include "ipmodules.h"

RCS_REGISTER(cvsid, "lowlevel/ipmod")

/*****************************************************************************/
int ipmod_low_printuse(__attribute__((unused)) FILE* outfilepath)
{
    return 0;
}
/*****************************************************************************/
errcode ipmod_low_init(__attribute__((unused)) char* arg)
{
    T(ipmod_low_init)

    ipmodules_init();

    return OK;
}
/*****************************************************************************/
errcode ipmod_low_done()
{
    printf("== lowlevel/ipmod/ipmod.c:ipmod_low_done ==\n");

    ipmodules_done();

    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
