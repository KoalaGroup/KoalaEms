/******************************************************************************
*                                                                             *
* requesttypes.h.m4                                                           *
*                                                                             *
* OS9, UNIX                                                                   *
*                                                                             *
* created before: 02.08.94                                                    *
* last changed: 24.03.95                                                      *
*                                                                             *
******************************************************************************/

#ifndef _requesttypes_h_
#define _requesttypes_h_

define(`version', `dnl')
define(`Req', `Req_$1,')
typedef enum{
include(EMSDEFS/requesttypes.inc)
NrOfReqs
} Request;

define(`Req',`dnl')
define(`version', `#define ver_requesttab $1')
include(EMSDEFS/requesttypes.inc)

#endif

/*****************************************************************************/
/*****************************************************************************/
