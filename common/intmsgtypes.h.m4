/******************************************************************************
*                                                                             *
* intmsgtypes.h.m4                                                            *
*                                                                             *
* ULTRIX/OSF1                                                                 *
*                                                                             *
* 02.08.94                                                                    *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/
#ifndef _intmsgtypes_h_
#define _intmsgtypes_h_

define(`Intmsg', `Intmsg_$1,')
define(`version', `')

typedef enum{
include(EMSDEFS/intmsgtypes.inc)
NrOfIntmsg
} IntMsg;

define(`Intmsg', `')
define(`version', `#define ver_intmsgtab $1')
include(EMSDEFS/intmsgtypes.inc)

#endif

/*****************************************************************************/
/*****************************************************************************/
