/****************************************************************************
*                                                                           *
* errorstring.c.m4                                                          *
*                                                                           *
* UNIX                                                                      *
*                                                                           *
* 09.08.93                                                                  *
*                                                                           *
* PW                                                                        *
*                                                                           *
****************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: errorstring.c.m4,v 2.3 2011/04/07 14:07:08 wuestner Exp $";

#include "rcs_ids.h"
RCS_REGISTER(cvsid, "common")

define(`laenge', `ifelse(index($1, ` '), -1, len($1), index($1, ` '))')
define(`cut', `substr($1, 0, laenge($1))')

dnl define(`g_code', `"cut(Err_$1)",')
define(`e_code', `"cut(Err_$1)",')
define(`pl_code', `')
define(`rt_code', `')
char *errstr[]={
"OK",
include(EMSDEFS/errorcodes.inc)
};

dnl define(`g_code', `"$2",')
define(`e_code', `"$2",')
define(`pl_code', `')
define(`rt_code', `')
char *errlstr[]={
"kein Fehler",
include(EMSDEFS/errorcodes.inc)
};

dnl define(`g_code', `"cut(plErr_$1)",')
define(`e_code', `')
define(`pl_code', `"cut(plErr_$1)",')
define(`rt_code', `')
char *plerrstr[]={
"OK",
include(EMSDEFS/errorcodes.inc)
};

dnl define(`g_code', `"$2",')
define(`e_code', `')
define(`pl_code', `"$2",')
define(`rt_code', `')
char *plerrlstr[]={
"kein Fehler",
include(EMSDEFS/errorcodes.inc)
};

define(`e_code', `')
define(`pl_code', `')
define(`rt_code', `"cut(rtErr_$1)",')
char *rterrstr[]={
"OK, kann aber nicht auftreten",
include(EMSDEFS/errorcodes.inc)
};

define(`e_code', `')
define(`pl_code', `')
define(`rt_code', `"$2",')
char *rterrlstr[]={
"kein Fehler, kann aber nicht auftreten",
include(EMSDEFS/errorcodes.inc)
};

/***************************************************************************/
/***************************************************************************/

