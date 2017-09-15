/*
 * errorcodes.h.m4
 * 
 * created (before?) 1993-Nov-07
 * 
 * $ZEL: errorcodes.h.m4,v 2.3 2007/04/09 23:55:07 wuestner Exp $
 */

#ifndef _errorcodes_h_
#define _errorcodes_h_

define(`e_code', `Err_$1,')
define(`pl_code', `')
define(`rt_code',`')
typedef enum
  {
  OK,
include(EMSDEFS/errorcodes.inc)
  NrOfErrs
  } errcode;

define(`e_code', `')
define(`pl_code', `plErr_$1,')
define(`rt_code',`')
typedef enum
  {
  plOK,
include(EMSDEFS/errorcodes.inc)
  NrOfplErrs
  } plerrcode;

define(`e_code', `')
define(`pl_code',`')
define(`rt_code', `rtErr_$1,')
typedef enum
  {
  rtOK,
include(EMSDEFS/errorcodes.inc)
  NrOfrtErrs
  } rterrcode;

#endif

/*****************************************************************************/
/*****************************************************************************/

