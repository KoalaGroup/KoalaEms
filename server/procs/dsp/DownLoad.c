/************************************************************************/
/*									*/
/*		Programm:	DownLoad.c	/ OS9			*/
/*									*/
/*		Erstellt:	G. Heinrichs				*/
/*									*/
/*		Datum:		29.07.1994				*/
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
#include "nco_drv.h"

extern ems_u32* outptr;

#define  BUFLEN  1048576

/*----------------------------------------------------------------------*/

plerrcode proc_DownLoad1( p )
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
    fp1 = open( datei, S_IREAD );
    extractstring( datei, p );
    fp2 = open( datei, S_IREAD );

    if( (fp1 != -1) && (fp2 != -1) )
    {
      /*----- Schreiben der Daten in den Speicher -----*/

      set_led( 0, 0 );
      init_dsp_adr( 0, MEMB );
     
      while( n = read( fp1, buf, BUFLEN ) )
      {
	n = (int) (n / 2);
	for( i=0; i<n; i++ )
	  *pdr[0] = *(buf + i);
	laenge += n;
      }
      close( fp1 );

      set_led( 1, 0 );
      init_dsp_adr( 1, MEMB );

      while( n = read( fp2, buf, BUFLEN ) )
      {
	n = (int) (n / 2);
	for( i=0; i<n; i++ )
	  *pdr[1] = *(buf +i);
      }
      close( fp2 );
      free( buf );

      printf("\n%ld  Werte eingelesen!\n\n", laenge);

      /*------------- Laden der Sequenzer -------------*/

      *Start_Stop = 0xffff;

      load_seq( 0, laenge );
      load_seq( 1, laenge );

      dsp_ready( 0 );
      dsp_ready( 1 );

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


plerrcode test_proc_DownLoad1( p )
int *p;
{
  if( *p > 63 )
    return( plErr_NoMem );
  wirbrauchen = 0;
  return( plOK );
}


char name_proc_DownLoad1[] = "DownLoad1";
int ver_proc_DownLoad1 = 1;

/*----------------------------------------------------------------------*/

plerrcode proc_DownLoad2( p )
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
    fp1 = open( datei, S_IREAD );
    extractstring( datei, p );
    fp2 = open( datei, S_IREAD );

    if( (fp1 != -1) && (fp2 != -1) )
    {
      /*----- Schreiben der Daten in den Speicher -----*/

      set_led( 2, 0 );
      init_dsp_adr( 2, MEMB );
      
      while( n = read( fp1, buf, BUFLEN ) )
      {
	n = (int) (n / 2);
	for( i=0; i<n; i++ )
	  *pdr[2] = *(buf + i);
	laenge += n;
      }
      close( fp1 );
      
      set_led( 3, 0 );
      init_dsp_adr( 3, MEMB );

      while( n = read( fp2, buf, BUFLEN ) )
      {
	n = (int) (n / 2);
	for( i=0; i<n; i++ )
	  *pdr[3] = *(buf +i);
      }
      close( fp2 );
      free( buf );

      printf("\n%ld  Werte eingelesen!\n\n", laenge);

      /*------------- Laden der Sequenzer -------------*/

      *Start_Stop = 0xffff;

      load_seq( 2, laenge );
      load_seq( 3, laenge );

      dsp_ready( 2 );
      dsp_ready( 3 );

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


plerrcode test_proc_DownLoad2( p )
int *p;
{
  if( *p > 63 )
    return( plErr_NoMem );
  wirbrauchen = 0;
  return( plOK );
}


char name_proc_DownLoad2[] = "DownLoad2";
int ver_proc_DownLoad2 = 1;

/*----------------------------------------------------------------------*/













