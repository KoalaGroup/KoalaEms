/******************************************************************************
*                                                                             *
* trigger.c                                                                   *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* created: before 11.10.94                                                    *
* last changed: 04.11.94                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include <errorcodes.h>
#include "../../../trigger/chi/gsi/gsitrigger.h"
#include "../../procprops.h"

#ifdef NOVOLATILE
#define VOLATILE
#else
#define VOLATILE volatile
#endif

/*****************************************************************************/
/*
p[0]: num
p[1]: space
*/
plerrcode proc_ResetTrigger(p)
int *p;
{
VOLATILE int* addr;

addr=(int*)(GSI_BASE+0x100*p[1]+CONTROL);
*addr=HALT;
*addr=SLAVE_TRIG;
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
if (!is_gsi_space(p[1])) return(plErr_HWTestFailed);
return(plOK);
}

static procprop ResetTrigger_prop={0, 0, "<address space>", 0};

procprop* prop_proc_ResetTrigger()
{
return(&ResetTrigger_prop);
}

char name_proc_ResetTrigger[]="ResetTrigger";
int ver_proc_ResetTrigger=2;

/*****************************************************************************/
/*****************************************************************************/
