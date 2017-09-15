/*
 * common/modultypeinfo.c.m4
 * created before: 2003/07/03 15:37:40 wuestner
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: modultypeinfo.c.m4,v 2.4 2011/04/07 14:07:08 wuestner Exp $";

#include "modultypeinfo.h"
#include "rcs_ids.h"

RCS_REGISTER(cvsid, "common")

define(`laenge', `ifelse(index($1, ` '), -1, len($1), index($1, ` '))')
define(`cut', `substr($1, 0, laenge($1))')
define(`module',`{"cut($1)",$2},')

modultypeinfo modultypes[]={
{"NO_KNOWN_MODULE",0},
include(EMSDEFS/modultypes.inc)
};

int anzahl_der_bekannten_module=sizeof(modultypes)/sizeof(modultypeinfo);

