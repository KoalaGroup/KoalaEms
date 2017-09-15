/******************************************************************************
*                                                                             *
* names.c.m4                                                                  *
*                                                                             *
* OS9/ULTRIX                                                                  *
*                                                                             *
* created: 13.09.94                                                           *
* last changed: 13.09.94                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: names.c.m4,v 1.2 2011/04/06 20:30:28 wuestner Exp $";

#include "names.h"
#include "rcs_ids.h"

RCS_REGISTER(cvsid, "lowlevel/tileforth")

/*****************************************************************************/

define(`laenge', `ifelse(index($1, ` '), -1, len($1), index($1, ` '))')
define(`cut', `substr($1, 0, laenge($1))')

define(`e_code', `"cut(Err_$1)",')
define(`pl_code', `')
define(`rt_code', `')
char *errstr[]={
"OK",
include(../../../common/errorcodes.inc)
};

define(`version', `dnl')
define(`Req', `"$1",')
char *reqstrs[]={
include(../../../common/requesttypes.inc)
};

/*****************************************************************************/
/*****************************************************************************/
