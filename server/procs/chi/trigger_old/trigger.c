/******************************************************************************
*                                                                             *
* trigger.c                                                                   *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 20.09.94                                                                    *
*                                                                             *
******************************************************************************/

#include <errorcodes.h>
#include "../../../trigger/chi/gsi/gsitrigger.h"
#include <ssm.h>

#ifdef NOVOLATILE
#define VOLATILE
#else
#define VOLATILE volatile
#endif

/*****************************************************************************/

plerrcode proc_ResetTrigger(p)
int *p;
{
VOLATILE int* addr;

addr=(int*)(GSI_BASE+0x100*p[1]+CONTROL);
*addr=HALT;
*addr=SLAVE;
*addr=BUS_ENABLE;
*addr=CLEAR;
*addr=BUS_DISABLE;
return(plOK);
}

plerrcode test_proc_ResetTrigger(p)
int *p;
{
if (p[0]!=1) return(plErr_ArgNum);
if ((p[1]<1) || (p[1]>3)) return(plErr_ArgRange);
permit((int*)(GSI_BASE+0x100*p[1]), 16, ssm_RW);
return(plOK);
}

char name_proc_ResetTrigger[]="ResetTrigger";
int ver_proc_ResetTrigger=2;

/*****************************************************************************/
/*****************************************************************************/
