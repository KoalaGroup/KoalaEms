/*
 * lowlevel/jtag/jtag_XCFP.c
 * created 2008-Feb-13 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: jtag_XCFP.c,v 1.2 2011/04/06 20:30:24 wuestner Exp $";

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
jtag_write_XCFP(struct jtag_chip* chip, ems_u8* data, ems_u32 usercode)
{
printf("NOT WRITING!\n");
return -1;
}
/****************************************************************************/
int
jtag_read_XCFP(struct jtag_chip* chip, ems_u8* data, ems_u32* usercode)
{
printf("NOT READING!\n");
return -1;
}
/****************************************************************************/
/****************************************************************************/
