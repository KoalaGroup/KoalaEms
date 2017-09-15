/*
 * common/reqstrings.c.m4
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: reqstrings.c.m4,v 2.6 2011/04/07 14:07:08 wuestner Exp $";

#include "rcs_ids.h"
RCS_REGISTER(cvsid, "common")

define(`version', `dnl')
define(`Req', `"$1",')
char *reqstrs[]={
include(EMSDEFS/requesttypes.inc)
0,
};

define(`Unsol', `"$1",')
char *unsolstrs[]={
include(EMSDEFS/unsoltypes.inc)
0,
};

define(`Intmsg', `"$1",')
char *intmsgstrs[]={
include(EMSDEFS/intmsgtypes.inc)
0,
};

