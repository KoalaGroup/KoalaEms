/******************************************************************************
*                                                                             *
* ems_errstr.c.m4                                                             *
*                                                                             *
* 09.08.93                                                                    *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: ems_errstr.c.m4,v 2.4 2011/04/07 14:07:08 wuestner Exp $";

#include "rcs_ids.h"
RCS_REGISTER(cvsid, "common")

define(`laenge', `ifelse(index($1, ` '), -1, len($1), index($1, ` '))')
define(`cut', `substr($1, 0, laenge($1))')

define(`emse_code', `"cut($1)",')
char *emserrstr[]={
"EMS_OK",
include(EMSDEFS/ems_err.inc)
};

define(`emse_code', `"$2",')
char *emserrlstr[]={
"no EMS Error",
include(EMSDEFS/ems_err.inc)
};

/*****************************************************************************/
/*****************************************************************************/
