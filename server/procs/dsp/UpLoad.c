/************************************************************************/
/*									*/
/*		Programm:	UpLoad.c	/ OS9			*/
/*									*/
/*		Erstellt:	G. Heinrichs				*/
/*									*/
/*		Datum:		29.08.1994				*/
/*									*/
/************************************************************************/


#include <stdio.h>
#include <string.h>
#include <math.h>
#include <modes.h>
#include <errno.h>
#include <errorcodes.h>
#include <xdrstring.h>

#include "dsp_drv.h"

extern ems_u32* outptr;
extern int wirbrauchen;

#define  BUFLEN  1048576

/*----------------------------------------------------------------------*/

plerrcode proc_UpLoad1( p )
int *p;
{
  long laenge = 0;
  word *buf;
  char datei[256];
  int fp1, fp2, i, n;

  /*----- Ruecksetzen der DSPs ---------*/

  reset_dsp( 0 );
  reset_dsp( 1 );
  
  /*----- Allokieren des Buffers -------*/

  buf = (word *) malloc( BUFLEN * sizeof( word ) );

  /*----- Oeffnen der Binaer-Files -----*/

  if( buf )
  {
    p = extractstring( datei, ++p );
    fp1 = creat( datei, S_IWRITE );
    extractstring( datei, p );
    fp2 = creat( datei, S_IWRITE );

    if( (fp1 != -1) && (fp2 != -1) )
    {
      /*----- Schreiben der Daten in den Speicher -----*/

      set_led( 0, 0 );
      set_led( 1, 0 );

      init_dsp_adr( 0, MEMB );
      init_dsp_adr( 1, MEMB );
     
      for( i=0; i<2; i++ )
      {
	for( n=0; n<BUFLEN; n++ )
       	  *(buf + n) = *pdr[0];
	laenge += write( fp1, buf, 2*BUFLEN );

	for( n=0; n<BUFLEN; n++ )
	  *(buf + n) = *pdr[1];
	write( fp2, buf, 2*BUFLEN );
      }
      close( fp1 );
      close( fp2 );
      free( buf );

      printf("\n%ld  Werte ausgelesen!\n\n", laenge/2);

      return( plOK );
    }
    else
    {
      free( buf );
      *outptr++ = errno;
      return( plErr_System );
    }
  }
  else
    return( plErr_NoMem );
}


plerrcode test_proc_UpLoad1( p )
int *p;
{
  if( *p > 63 )
    return( plErr_NoMem );
  wirbrauchen = 0;
  return( plOK );
}


char name_proc_UpLoad1[] = "UpLoad1";
int ver_proc_UpLoad1 = 1;

/*----------------------------------------------------------------------*/

plerrcode proc_UpLoad2( p )
int *p;
{
  long laenge = 0;
  word *buf;
  char datei[256];
  int fp1, fp2, i, n;

  /*----- Ruecksetzen der DSPs ---------*/

  reset_dsp( 2 );
  reset_dsp( 3 );
  
  /*----- Allokieren des Buffers -------*/

  buf = (word *) malloc( BUFLEN * sizeof( word ) );

  /*----- Oeffnen der Binaer-Files -----*/

  if( buf )
  {
    p = extractstring( datei, ++p );
    fp1 = creat( datei, S_IWRITE );
    extractstring( datei, p );
    fp2 = creat( datei, S_IWRITE );

    if( (fp1 != -1) && (fp2 != -1) )
    {
      /*----- Schreiben der Daten in den Speicher -----*/

      set_led( 2, 0 );
      set_led( 3, 0 );

      init_dsp_adr( 2, MEMB );
      init_dsp_adr( 3, MEMB );
      
      for( i=0; i<2; i++ )
      {
	for( n=0; n<BUFLEN; n++ )
	  *(buf + n) = *pdr[2];
	laenge += write( fp1, buf, 2*BUFLEN );

	for( n=0; n<BUFLEN; n++ )
	  *(buf + n) = *pdr[3];
	write( fp2, buf, 2*BUFLEN );
      }
      close( fp1 );
      close( fp2 );
      free( buf );

      printf("\n%ld  Werte ausgelesen!\n\n", laenge/2);

      return( plOK );
    }
    else
    {
      free( buf );
      *outptr++ = errno;
      return( plErr_System );
    }
  }
  else
    return( plErr_NoMem );
}


plerrcode test_proc_UpLoad2( p )
int *p;
{
  if( *p > 63 )
    return( plErr_NoMem );
  wirbrauchen = 0;
  return( plOK );
}


char name_proc_UpLoad2[] = "UpLoad2";
int ver_proc_UpLoad2 = 1;

/*----------------------------------------------------------------------*/













