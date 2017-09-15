/******************************************************************************
*                                                                             *
* fbsingle_c.c                                                                  *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* created: 05.10.94                                                           *
* last changed: 22.10.94                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include "globals.h"
#include "fbaccmacro.h"

/*****************************************************************************/

int FRC_c(int pa, int sa)
{
register int data;
register int dummy;

*CGEOptr=pa;
*SECADptr=sa;
data=*RNDMptr;
dummy=*DISCONptr;
return(data);
}

int FRC_cm(int pa, int sa)
{
register int data;

FCPD_(pa);
FCWSA(sa);
data=FCRW();
FCDISC();
return(data);
}

int FRD_c(int pa, int sa)
{
register int data;
register int dummy;

*DGEOptr=pa;
*SECADptr=sa;
data=*RNDMptr;
dummy=*DISCONptr;
return(data);
}

void FWC_c(int pa, int sa, int data)
{
register int dummy;

*CGEOptr=pa;
*SECADptr=sa;
*RNDMptr=data;
dummy=*DISCONptr;
return;
}

void FWD_c(int pa, int sa, int data)
{
register int dummy;

*DGEOptr=pa;
*SECADptr=sa;
*RNDMptr=data;
dummy=*DISCONptr;
return;
}

/*****************************************************************************/
/*****************************************************************************/
