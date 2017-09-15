/*****************************************************************
**     Treiber zur Ansteuerung der NCO's fuer die Traeger-      **
**     erzeugung und die variable Clock bei der USE             **
**     copyright 1993 by G.Heinrichs, H.Rongen  ZEL             **
*****************************************************************/


#define   byte      unsigned char
#define   word      unsigned short int
#define   longint   unsigned long int



/*------------- DIGI200 fuer die variable Clock ----------*/

word *Clk_Lo = (word*) 0xff006080;
word *Clk_Hi = (word*) 0xff006082;


/*------------- DIGI200 fuer Start/Stop und Status -------*/

word *Start_Stop = (word*) 0xff004080;
word *Status     = (word*) 0xff004082;


/*---------- 2 DIGI200 fuer die Traegerfrequenz ----------*/

word *FLo[2] = {(word*) 0xff002080, (word*) 0xff003080};
word *FHi[2] = {(word*) 0xff002082, (word*) 0xff003082};


/*--- Vorgabe der Taktfrequenz ---------------------------*/

#define NCO_CLK 40.0	/* Taktfrequenz der NCO in MHz */
#define Clock   64.0	/* Taktfrequenz der variablen Clock in MHz */

 

/*--- Ruecksetzen des NCO --------------------------------*/

reset_NCO( sys_nr )
byte sys_nr;
{
  *FLo[sys_nr] = 0xD000;	/* Data 7..0 und Reset = 0  */
  *FHi[sys_nr] = 0x0007;	/* LadeSignale */
  *FLo[sys_nr] = 0xF000;	/* Reset = 1   */
  
/*  *Clk_Hi = 0x8000; */	/* DAV = 1 */
}



/*--- Ausgabe der NCO-Frequenz auf der DIGI200 ---------------*/ 

frequenz_out( sys_nr, f )
byte sys_nr;
longint f;
{
   *FHi[sys_nr] = 0x0007;								/* Ladesignale        */

   *FLo[sys_nr] = 0xE000 | (word) (f & 255);			/* LSB in Register A0 */
   *FHi[sys_nr] = 0x000F;								/* Schreiben          */
   *FHi[sys_nr] = 0x0007;								/* Schreiben          */

   *FLo[sys_nr] = 0xE100 | (word) ((f >> 8) & 255);		/* Register A1        */
   *FHi[sys_nr] = 0x000F;								/* Schreiben          */
   *FHi[sys_nr] = 0x0007;								/* Schreiben  		  */
   
   *FLo[sys_nr] = 0xE200 | (word) ((f >> 16) & 255);	/* Register A2		  */
   *FHi[sys_nr] = 0x000F;								/* Schreiben   		  */
   *FHi[sys_nr] = 0x0007;								/* Schreiben		  */
   
   *FLo[sys_nr] = 0xE300 | (word) ((f >> 24) & 255); 	/* MSB in Register A3 */
   *FHi[sys_nr] = 0x000F;								/* Schreiben  		  */
   *FHi[sys_nr] = 0x0007;								/* Schreiben 		  */
   
   *FLo[sys_nr] = 0xB000;								/* Frequenz benutzen  */
   *FLo[sys_nr] = 0xF000;
}


/*----- Ausgabe der variablen Clock auf der DIGI200 ------------*/

clock_out( nco_clk )
longint nco_clk;
{ 
  *Clk_Lo = (word) nco_clk;						/*  LSB Schreiben 	  */
  *Clk_Hi = (word) (nco_clk >> 16);				/*  MSB Schreiben	  */
  *Clk_Hi = (word) ((nco_clk >> 16) & 0x7fff);	/*  DAV auf 0 setzen  */
  *Clk_Hi = 0x8000 | (word) (nco_clk >> 16);	/*  DAV auf 1 setzen  */
}

	

/****************************************************************
***  Hochsprachen Interface zur Ausgabe der Parameter F, Clk  ***
****************************************************************/


/*-----------------------------------------------------
  Frequenzeinstellung fuer STEL 1175 ist 32 Bit breit.
-----------------------------------------------------*/

setF( sys_nr, f )
byte sys_nr;
double f;
{
  longint x;
  
  x = (longint) floor( 4294967296.0 / NCO_CLK * f );
  frequenz_out( sys_nr, x );
}


/*-----------------------------------------------------------
  Variable Clockeinstellung fuer STEL 1175 ist 24 Bit breit.
-----------------------------------------------------------*/

set_clk( nco_clk )
double nco_clk;
{
  longint x;
  
  x = (longint) floor( 16777216.0 / Clock * nco_clk );
  clock_out( x );
}

