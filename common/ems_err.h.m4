/******************************************************************************
*                                                                             *
* ems_err.h.m4                                                                *
*                                                                             *
* 01.09.94                                                                    *
*                                                                             *
******************************************************************************/

#ifndef _ems_err_h_
#define _ems_err_h_

/* Fehlercodes fuer EMS_errno */
define(`emse_code', `  $1,')
typedef enum
  {
  /* EMS_errno<=EMSE_Start --> EMS_errno ist System-errno */
  EMSE_OK=0,
  EMSE_Start=1000,
include(EMSDEFS/ems_err.inc)
  NrOfEMSErrs
  } EMSerrcode;

#endif

/*****************************************************************************/
/*****************************************************************************/
