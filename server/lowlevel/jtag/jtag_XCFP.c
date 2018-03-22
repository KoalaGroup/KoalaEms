/*
 * lowlevel/jtag/jtag_XCFP.c
 * created 2008-Feb-13 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: jtag_XCFP.c,v 1.3 2017/10/22 22:14:27 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <rcs_ids.h>

#include "jtag_chipdata.h"

RCS_REGISTER(cvsid, "lowlevel/jtag")

int
jtag_write_XCFP(__attribute__((unused)) struct jtag_chip* chip,
        __attribute__((unused)) ems_u8* data,
        __attribute__((unused)) ems_u32 usercode)
{
printf("NOT WRITING!\n");
return -1;
}
/****************************************************************************/
int
jtag_read_XCFP(__attribute__((unused)) struct jtag_chip* chip,
        __attribute__((unused)) ems_u8* data,
        __attribute__((unused)) ems_u32* usercode)
{
printf("NOT READING!\n");
return -1;
}
/****************************************************************************/
/****************************************************************************/
