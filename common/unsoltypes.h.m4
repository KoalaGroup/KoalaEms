/******************************************************************************
*                                                                             *
* unsoltypes.h.m4                                                             *
*                                                                             *
* ULTRIX/OS9                                                                  *
*                                                                             *
* 15.02.93                                                                    *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/
#ifndef _unsoltypes_h_
#define _unsoltypes_h_

define(`Unsol', `Unsol_$1,')
define(`version', `')

typedef enum{
include(EMSDEFS/unsoltypes.inc)
NrOfUnsolmsg
} UnsolMsg;

define(`Unsol', `')
define(`version', `#define ver_unsolmsgtab $1')
include(EMSDEFS/unsoltypes.inc)

#endif

/*****************************************************************************/
/*****************************************************************************/

