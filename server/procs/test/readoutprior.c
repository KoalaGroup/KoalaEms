/******************************************************************************
*                                                                             *
* debuglevel.c                                                                *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 06.01.97                                                                    *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: readoutprior.c,v 1.5 2011/04/06 20:30:34 wuestner Exp $";

#include <errorcodes.h>
#include <config.h>
#include <debug.h>
#include <rcs_ids.h>
#include "../procprops.h"
#include "../procs.h"

extern ems_u32* outptr;
#ifdef VAR_READOUT_PRIOR
extern int readout_prior;
#endif

RCS_REGISTER(cvsid, "procs/test")

/*****************************************************************************/
/*
ReadoutPrior
*/
plerrcode proc_ReadoutPrior(ems_u32* p)
{
#ifdef VAR_READOUT_PRIOR
*outptr++=readout_prior;
if (p[0]>0) readout_prior=p[1];
#endif
return(plOK);
}

plerrcode test_proc_ReadoutPrior(ems_u32* p)
{
plerrcode res=plOK;
#ifdef VAR_READOUT_PRIOR
switch (p[0])
  {
  case 0: break;
  case 1: if (p[1]<1) res=plErr_ArgRange; break;
  default: res=plErr_ArgNum; break;
  }
#else
res= plErr_NoSuchProc;
#endif
return res;
}
#ifdef PROCPROPS
static procprop ReadoutPrior_prop={0, 1, "void", 0};

procprop* prop_proc_ReadoutPrior(void)
{
return(&ReadoutPrior_prop);
}
#endif
char name_proc_ReadoutPrior[]="ReadoutPrior";
int ver_proc_ReadoutPrior=1;

/*****************************************************************************/
/*****************************************************************************/
