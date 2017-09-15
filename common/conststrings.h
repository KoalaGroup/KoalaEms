/******************************************************************************
*                                                                             *
* conststrings.h                                                              *
*                                                                             *
* ULTRIX/OSF1                                                                 *
*                                                                             *
* 22.03.95                                                                    *
*                                                                             *
******************************************************************************/

#ifndef _conststrings_h_
#define _conststrings_h_

#include <cdefs.h>
#include <ems_err.h>
#include <errorcodes.h>
#include <requesttypes.h>
#include <unsoltypes.h>
#include <intmsgtypes.h>
#include <objecttypes.h>

/*****************************************************************************/

__BEGIN_DECLS
const char* EMS_errstr(EMSerrcode error);
const char* R_errstr(errcode error);
const char* PL_errstr(plerrcode error);
const char* RT_errstr(rterrcode error);
const char* Req_str(Request nr);
const char* Unsol_str(UnsolMsg nr);
const char* Int_str(IntMsg nr);
const char* Object_str(Object nr);
const char* Domain_str(Domain nr);
const char* Invocation_str(Invocation nr);
const char* Capabtyp_str(Capabtyp nr);
const char* InOutTyp_str(InOutTyp nr);
const char* IOAddr_str(IOAddr nr);
const char* DataoutStatus_str(int nr);
__END_DECLS

#endif

/*****************************************************************************/
/*****************************************************************************/
