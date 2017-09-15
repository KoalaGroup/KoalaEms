/*
 * lowlevel/asynch_io/asynch_io.c
 * created 2007-Oct-24 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: asynch_io.c,v 1.2 2011/04/06 20:30:22 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "asynch_io.h"

RCS_REGISTER(cvsid, "lowlevel/asynch_io")

int asynch_io_low_printuse(FILE* outfilepath)
{
/* nothing to configure */
    return 0;
}

errcode asynch_io_low_init(char* arg)
{
    return OK;
}

errcode asynch_io_low_done()
{
    return OK;
}
