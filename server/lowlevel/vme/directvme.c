/*****************************************************************************/
/*                                                                           */
/* Initialisiert VME-RAHMEN                                                  */    
/*                                                                           */
/* Die Prozedur initialisiert einen VME-Rahmen und gibt einen Zeiger auf     */
/* das entsprechende SMP-Modul zurueck.                                      */
/*                                                                           */
/* Parameter: CFG A24_MEM A24_IO ...                                         */
/*                                                                           */
/* 26.04.'93                                                                 */
/*                                                                           */
/* Autor Twardowski                                                          */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <types.h>
#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include "directvme.h"

#ifdef NOVOLATILE
#define VOLATILE 
#else
#define VOLATILE volatile
#endif


u_char *VMEinit(CFG,A24_MEM,A24_IO)
long int CFG,A24_MEM,A24_IO;
{
   int _CFG;

   _CFG=A16_BASE+CFG;

/* !!!!!!!!!!!!!!!!!  muss hier noch fuer andere module gemacht werden !!! */
   *((VOLATILE u_char *)(_CFG+0x6))=0x10+(A24_MEM >>20);
   *((VOLATILE u_char *)(_CFG+0x2))=A24_IO >>16;
   *((VOLATILE u_char *)(_CFG+0x4))=0xFF;
   *((VOLATILE u_char *)(_CFG+0x8))=0x08;

   return( (u_char *)(A24_BASE+A24_IO) );
}
/***************************************************************************/
/* gibt eine Adresse auf ein Smpmodul zurueck                              */
/* bei der Angabe der Adresse ist zu beachten, dass bei Adresse der Module */
/* im zweiten Rahmen MAXSMP aufaddiert werden muss.                        */ 

u_char *SMPaddr(adresse)
register int adresse;
{
   int addr,A24_IO;
   u_char *smp_io;
 
   addr=adresse;
   if ( addr/MAXSMP ){
      A24_IO=A24_IO_2;
      addr=addr-MAXSMP;
   }
   else{
      A24_IO=A24_IO_1;
   }
   smp_io=(u_char *)(A24_BASE+A24_IO);

   return( smp_io+addr );
}
/***************************************************************************/
/* gibt eine Adresse auf ein Smpmodul zurueck                              */
/* bei der Angabe der Adresse ist zu beachten, dass bei Adresse der Module */
/* im zweiten Rahmen MAXSMP aufaddiert werden muss.                        */ 
/* im Gegensatz zu SMPaddr wir hier noch ein Offset auf die Adresse  ueber-*/
/* geben.                                                                  */
u_char *A_Addr(adresse,offset)
register int adresse,offset;
{
u_char *ptr;
int bustype, cratenumber, moduladdres, io;

moduladdres= (adresse & 0xFFFF);
bustype    = ((adresse >> 16) & 0xF);
cratenumber= ((adresse >> 20) & 0xF);
io         = ((adresse >> 24) & 0xF);

if (bustype)
{  switch (cratenumber)
   {  case 0:  ptr=(u_char *)(moduladdres+offset);
               break;

      case 1:  switch (io)
               {  case 0:  ptr=(u_char *)(A24_BASE+A24_MEM_1+moduladdres+offset);
                           break;

                  case 1:  ptr=(u_char *)(A24_BASE+A24_MMIO_1+moduladdres+offset);
                           break;

                  default: ptr=(u_char *)(A24_BASE+A24_IO_1+moduladdres+offset);
                           break;
                }
               break;
      case 2:  switch (io)
               {  case 0:  ptr=(u_char *)(A24_BASE+A24_MEM_2+moduladdres+offset);
                           break;

                  case 1:  ptr=(u_char *)(A24_BASE+A24_MMIO_2+moduladdres+offset);
                           break;

                  default: ptr=(u_char *)(A24_BASE+A24_IO_2+moduladdres+offset);
                           break;
                }
               break;

      default: {}
   } /*switch*/
} /*if*/
else {printf("!!! PROFIBUS IST NICHT IMPLEMENTIERT !!!\n");ptr=NULL;}

return(ptr);
}
/***************************************************************************/
/* gibt eine VMEAdresse zurueck                                            */

u_char *VME_Addr(adresse,offset)
register int adresse,offset;
{
u_char *ptr;
ptr=(u_char *)(adresse+offset);
return(ptr);
}

/***************************************************************************/
/*                                                                         */
/* wandelt ein 4-Bytefeld in eine 32_Bit Integerzahl um                    */
/* wenn MAK=4 :-)                                                          */
/*                                                                         */

int MAKDOU( MAK, B) 
u_char MAK; 
u_char *B; 

{ 
char         loki;
int          NUMVAL;

loki = MAK; 
NUMVAL = B[loki]; 
do { 
   NUMVAL = NUMVAL * 256; 
   loki = loki - 1; 
   NUMVAL = NUMVAL + B[loki]; 
}  while (!(loki == 1)) ; 

return( NUMVAL );
} 
/*********************************************************************/
/* 
 * wandelt eine 32-BitInteger in ein BytFeld der Laenge MAK um
*/
 
u_char *MAKBYT(MAK,B)
u_char MAK;
int  B;
{
int    i=0;
u_char *p;

p=(u_char *)malloc(5*sizeof(p));
do {
   p[i+1]=(B>>8*i)&255;
}while ( ++i<MAK );
return(p);
}
/**********************************************************************/
/*********************************************************************/
/*                                                                   */
/* Erstellungsdatum : 03.06. '93                                     */
/*                                                                   */
/* Autor : Twardo ( umgeschrieben fuer EMS )                         */
/*                                                                   */
/*********************************************************************/
void SIEM_303SETCTR5(adresse) 
int  adresse; 
{ 
} 
/*********************************************************************/
/*
*/
void SIEM_303SETMLH5(adresse) 
int  adresse;
{ 
} 
/******************************************************************/
/* gibt einen Pointer auf folgende Liste zurueck :                */
/*    1 mode 1, 1 mod 2, 1 load 1, 1 load 2, 1 hold 1, 1 hold 2   */
/*    2 mode 1, 2 mod 2, 2 load 1, ...                            */
/*    ...                                   10 hold 1,10 hold 2   */
/*                                                                */
/******************************************************************/
u_char *SIEM_303GETMLH5(adresse) 
int  adresse; 
{ 
} 
/******************************************************************/
/*                                                                */
/* gibt (u_char *) der Laenge 11 auf Zaehlregister zurueck        */
/*                                                                */
/******************************************************************/
u_char *SIEM_303GETCOU5(adresse) 
int  adresse; 
{ 
}
/******************************************************************/
/*                                                                */
/* gibt (u_char *) der Laenge 7 auf Zaehlregister zurueck        */
/*                                                                */
/******************************************************************/
u_char *SIEM_303GETCTR5(adresse) 
int  adresse; 

{ 
} 
/*************************************************************/
/***************************************************************************/
/*                                                                         */
/* Initialisierung des VME_Rahmens                                         */
/*                                                                         */
/* Das Programm wird mit dem Aufruf des Servers aktiviert.                 */
/*                                                                         */
/* 26.04.'93                                                               */
/*                                                                         */
/* Autor: Twardowski                                                       */
/* Bemerkung: Hier wird aus programmtechnischen Gruenden keine Initiali-   */
/*            sierung mehr vorgenommen.                                    */ 
/*                                                                         */
/***************************************************************************/
errcode vme_low_init()
{
u_char *dummy;

D(D_REQ,printf("VME init\n");)
dummy=VMEinit(CFG_1,A24_MEM_1,A24_IO_1);
dummy=VMEinit(CFG_2,A24_MEM_2,A24_IO_2);

return(OK);
}

errcode vme_low_done()
{
return(OK);
}



