/******************************************************************************
*                                                                             *
*  gsi_soft.c for chi                                                         *
*                                                                             *
*  OS9                                                                        *
*                                                                             *
*  17.03.94                                                                   *
*                                                                             *
******************************************************************************/

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <ssm.h>

#include "../../../trigger/chi/gsi/gsitrigger.h"

#ifdef NOVOLATILE
#define VOLATILE
#else
#define VOLATILE volatile
#endif

static VOLATILE int *stat, *stat;
static VOLATILE int *ctrl, *fca, *cvt;
static int nummer;

/*****************************************************************************/
/*
GSI_Trig
p[1] : base (1, 2, 3)
p[2] : nummer
*/

/*****************************************************************************/

plerrcode proc_GSI_soft(p)
int *p;
{
char* basis;
VOLATILE int *stat;

T(proc_GSI_soft)
basis=GSI_BASE+0x100*p[1];
stat=(int*)(basis+STATUS);
*stat=p[2];
return(plOK);
}

plerrcode test_proc_GSI_soft(p)
int *p;
{
char* basis;

T(test_proc_GSI_soft)
if (p[0]!=2) return(plErr_ArgNum);
if ((p[1]!=1) && (p[1]!=2) && (p[1]!=3)) return(plErr_ArgRange);
if ((p[2]<1) || (p[2]>15)) return(plErr_ArgRange);

basis=GSI_BASE+0x100*p[1];

permit(basis, 16, ssm_RW);
return(plOK);
}

char name_proc_GSI_soft[]="GSI_soft";
int ver_proc_GSI_soft=1;

/*****************************************************************************/
/*****************************************************************************/

