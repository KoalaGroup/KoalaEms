/******************************************************************************
*                                                                             *
* onreadouterror.c                                                            *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 04.11.94                                                                    *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: onreadouterror.c,v 1.7 2011/04/06 20:30:34 wuestner Exp $";

#include <errorcodes.h>
#include <sconf.h>
#include <rcs_ids.h>
#include "../procprops.h"
#include "../procs.h"

#ifdef READOUT_CC
extern int onreadouterror;
#endif

RCS_REGISTER(cvsid, "procs/test")

/*****************************************************************************/

plerrcode proc_OnReadoutError(ems_u32* p)
{
#ifdef READOUT_CC
onreadouterror=p[1];
#endif
return(plOK);
}

plerrcode test_proc_OnReadoutError(ems_u32* p)
{
#ifdef READOUT_CC
if (p[0]==1)
  return(plOK);
else
  return(plErr_ArgNum);
#else
return(plErr_NoSuchProc);
#endif
}
#ifdef PROCPROPS
static procprop OnReadoutError_prop={0, 0, "val", 0};

procprop* prop_proc_OnReadoutError(void)
{
return(&OnReadoutError_prop);
}
#endif
char name_proc_OnReadoutError[]="OnReadoutError";
int ver_proc_OnReadoutError=1;

/*****************************************************************************/
/*****************************************************************************/
