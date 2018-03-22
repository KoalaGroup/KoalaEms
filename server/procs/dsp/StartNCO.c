/************************************************************************/
/*									*/
/*		Programm:	StartNCO.c	/ OS9			*/
/*									*/
/*		Erstellt:	G. Heinrichs				*/
/*									*/
/*		Datum:		03.08.1994				*/
/*									*/
/************************************************************************/


#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <errorcodes.h>
#include <xdrstring.h>

#include "nco_drv.h"

extern ems_u32* outptr;

/*----------------------------------------------------------------------*/

plerrcode proc_StartNCO1( p )
int *p;
{
  char buf[256];
  double frequenz;

  /*----- Starten der Sequenzer --------*/

  *Start_Stop = 0xfffe;
  *Start_Stop = 0xffff;
  
  /*----- Laden und Starten der NCO ----*/

  extractstring( buf, p );
  frequenz = (double) atof( *buf );
  setF( 0, frequenz );
  *FHi[0] = 0x0016;
  return( plOK );
}


plerrcode test_proc_StartNCO1( p )
int *p;
{
  if( *p > 63 )
    return( plErr_NoMem );
  wirbrauchen = 0;
  return( plOK );
}


char name_proc_StartNCO1[] = "StartNCO1";
int ver_proc_StartNCO1 = 1;

/*----------------------------------------------------------------------*/

plerrcode proc_StartNCO2( p )
int *p;
{
  char buf[256];
  double frequenz;

  /*----- Starten der Sequenzer --------*/

  *Start_Stop = 0xfffe;
  *Start_Stop = 0xffff;
  
  /*----- Laden und Starten der NCO ----*/

  extractstring( buf, p );
  frequenz = (double) atof( *buf );
  setF( 0, frequenz );
  *FHi[0] = 0x0016;
  return( plOK );
}

plerrcode test_proc_StartNCO2( p )
int *p;
{
  if( *p > 63 )
    return( plErr_NoMem );
  wirbrauchen = 0;
  return( plOK );
}

char name_proc_StartNCO2[] = "StartNCO2";
int ver_proc_StartNCO2 = 1;

/*----------------------------------------------------------------------*/













