/*********************************************************************/
/*                                                                   */
/* Erstellungsdatum : 03.06. '93                                     */
/*                                                                   */
/* Autor : Twardo ( umgeschrieben fuer EMS )                         */
/*                                                                   */
/*********************************************************************/
#include <types.h>
#include "../../LOWLEVEL/VME/vme.h"


void SIEM_303SETCTR5(adresse) 
int  adresse; 

{ 
char COUNT, CO0, DA0;
char CO1, DA1;
char COMX, DAX;


/* ---------- SET CONTROL GROUP ----------------- */ 
COUNT = 4; 

/* --- CO0 := COMMAND-REGISTER COUNTER-CHIP 0 --- */ 
CO0 = COUNT + 1; DA0 = COUNT + 0; 

/* --- CO1 := COMMAND-REGISTER COUNTER-CHIP 1 --- */ 
CO1 = COUNT + 3; DA1 = COUNT + 2; 

/* --- INIT CHIP 0 AND 1, CLEAR ALL TC'S -------- */ 

*A_Addr(adresse, CO0) = 255;              /* REST */ 

*A_Addr(adresse, CO0) = (64 + 31);  /*LOLO OR ALL */ 

*A_Addr(adresse, CO1) = 255;              /* REST */ 

*A_Addr(adresse, CO1) = 64 + 31;    /*LOLO OR ALL */
/* ---------------------------------------------- */

COMX = CO0; DAX = DA0; 

*A_Addr(adresse, COMX) = 7;             /* AL1REG */ 

*A_Addr(adresse, DAX)  = 255;           /* AL1L   */ 

*A_Addr(adresse, DAX)  = 255;           /* AL1H   */ 

*A_Addr(adresse, DAX)  = 255;           /* AL2L   */ 

*A_Addr(adresse, DAX)  = 255;           /* AL2H   */ 

*A_Addr(adresse, DAX)  = 0;             /* MMXL   */ 

*A_Addr(adresse, DAX)  = 1;             /* MMXH   */


COMX = CO1; DAX = DA1; 

*A_Addr(adresse, COMX) = 7;             /* AL1REG */ 

*A_Addr(adresse, DAX)  = 255;           /* AL1L   */ 

*A_Addr(adresse, DAX)  = 255;           /* AL1H   */ 

*A_Addr(adresse, DAX)  = 255;           /* AL2L   */ 

*A_Addr(adresse, DAX)  = 255;           /* AL2H   */ 

*A_Addr(adresse, DAX)  = 0;             /* MMXL   */ 

*A_Addr(adresse, DAX)  = 1;             /* MMXH   */

} 

/*********************************************************************/
/*
*/


void SIEM_303SETMLH5(adresse) 
int  adresse;
{ 
char COUNT, CO0, DA0;
char CO1, DA1;
char COMX, DAX;

/* -------- FOR COUNTER CHIP 0  ----------------------- */ 

/* --- LOAD MODE-,LOAD-,HOLD-REG. FOR COUNTER 1 - 5 --- */ 

/* --- SET DATA-POINTER FOR COUNTER 1 MODE-REG. ------- */ 


COUNT = 4; 
/* ---------------------------------------------------- */ 

/* --- CO0 := COMMAND-REGISTER COUNTER-CHIP 0 --------- */ 

CO0 = COUNT + 1; DA0 = COUNT + 0; 
/* ---------------------------------------------------- */ 

/* --- CO1 := COMMAND-REGISTER COUNTER-CHIP 1 --------- */ 

CO1 = COUNT + 3; DA1 = COUNT + 2; 
/* ---------------------------------------------------- */ 


COMX = CO0; DAX = DA0; 

*A_Addr(adresse, COMX) = 1; 
/* --------------  1  --------------------------------- */ 

*A_Addr(adresse, DAX)  = 41; 
*A_Addr(adresse, DAX)  = 161; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
/* --------------  2  --------------------------------- */ 

*A_Addr(adresse, DAX)  = 8; 
*A_Addr(adresse, DAX)  = 160; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 

/* --------------  3  --------------------------------- */ 

*A_Addr(adresse, DAX)  = 41;
*A_Addr(adresse, DAX)  = 163; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 

/* --------------  4  --------------------------------- */ 

*A_Addr(adresse, DAX)  = 8;
*A_Addr(adresse, DAX)  = 160; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 

/* --------------  5  --------------------------------- */ 

*A_Addr(adresse, DAX)  = 41;
*A_Addr(adresse, DAX)  = 165; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 


/* -------- FOR COUNTER CHIP 1 ------------------------ */ 

/* --- LOAD MODE-,LOAD-,HOLD-REG. FOR COUNTER 1 - 5 --- */ 

/* --- SET DATA-POINTER FOR COUNTER 1 MODE-REG. ------- */ 

COMX = CO1; DAX = DA1; 
*A_Addr(adresse, COMX) = 1; 

/* --------------  1  --------------------------------- */ 

*A_Addr(adresse, DAX)  = 8; 
*A_Addr(adresse, DAX)  = 161; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
/* --------------  2  --------------------------------- */ 

*A_Addr(adresse, DAX)  = 41; 
*A_Addr(adresse, DAX)  = 162; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 

/* --------------  3  --------------------------------- */ 

*A_Addr(adresse, DAX)  = 8;
*A_Addr(adresse, DAX)  = 160; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 

/* --------------  4  --------------------------------- */ 

*A_Addr(adresse, DAX)  = 41;
*A_Addr(adresse, DAX)  = 164; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 

/* --------------  5  --------------------------------- */ 

*A_Addr(adresse, DAX)  = 8;
*A_Addr(adresse, DAX)  = 160; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 
*A_Addr(adresse, DAX)  = 0; 

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
char COUNT, CO0, DA0;
char CO1, DA1;
char COMX, DAX;
char I;
u_char *ptr;

ptr=(u_char *)malloc( 70*sizeof(ptr) );

COUNT = 4; 
/* ------------------------------------------------------ */ 

/* --- CO0 := COMMAND-REGISTER COUNTER-CHIP 0 ----------- */ 

CO0 = COUNT + 1; DA0 = COUNT + 0; 
/* ------------------------------------------------------ */ 

/* --- CO1 := COMMAND-REGISTER COUNTER-CHIP 1 ----------- */ 

CO1 = COUNT + 3; DA1 = COUNT + 2; 
/* ------------------------------------------------------ */ 


/* --- FOR COUNTER-CHIP   0  and  1   ------------------- */ 

/* --- READ ALL REGISTER FROM COUNTER 1 - 5 ------------- */ 

/* --- IN DER REIHENFOLGE MODE-, LOAD-, HOLD-REGISTER --- */ 

COMX = CO0; DAX = DA0;  

*A_Addr(adresse,COMX)  = (160 + 31);/* SAVE COUNTER 1 - 5 */ 

*A_Addr(adresse, COMX) = 1;         /* -SET DATA-POINTER- */ 

/* ------------------------------------------------------ */ 


I = 1; 
do { 
   ptr[I] = *A_Addr(adresse, DAX); 
   I = I + 1; 
}  while ( !(I > 30) ) ; 

COMX = CO1; DAX = DA1; 

*A_Addr(adresse, COMX) = (160 + 31); /*SAVE COUNTER 1 - 5*/ 

*A_Addr(adresse, COMX) = 1;          /*  SET DATA-POINTER */ 

/* ----------------------------------------------------- */ 

do { 
   ptr[I] = *A_Addr(adresse, DAX); 
   I = I + 1; 
}  while ( !(I > 60) ) ; 

ptr[0]=I;

return( ptr );
} 
/******************************************************************/
/*                                                                */
/* gibt (u_char *) der Laenge 11 auf Zaehlregister zurueck        */
/*                                                                */
/******************************************************************/


u_char *SIEM_303GETCOU5(adresse) 
int  adresse; 

{ 
char COUNT, CO0, DA0;
char CO1, DA1;
char COMX, DAX;

char I, NA, NE;
u_char *ptr;

ptr=(u_char *)malloc(21*sizeof(ptr));

COUNT = 4; 
/* ---------------------------------------------- */ 

/* --- CO0 := COMMAND-REGISTER COUNTER-CHIP 0 --- */

CO0 = COUNT + 1; DA0 = COUNT + 0; 
/* ---------------------------------------------- */ 

/* --- CO1 := COMMAND-REGISTER COUNTER-CHIP 1 --- */ 

CO1 = COUNT + 3; DA1 = COUNT + 2; 
/* ---------------------------------------------- */ 
  
/* ----------- READ COUNTER-PAIR 1-5 ------------ */ 

COMX = CO0; DAX = DA0;        /* SET COUNTER-ADR. */ 


NA = 1; NE = 10; 

/* --- SAVE COUNTER 1-5 ------------------------- */
*A_Addr(adresse, COMX) = (160 + 31); 
 
/* --- SET DATA-POINTER HOLD-CYCLE TO COUNTER 1 - */ 
*A_Addr(adresse, COMX) = 25;  


I = NA; 
do { 
   ptr[I] = *A_Addr(adresse, DAX); 
   I = I + 1; 
}  while ( !(I > NE) ) ; 

COMX = CO1; DAX = DA1; 
NA = 11; NE = 20;
 
/* --- SAVE COUNTER 1-5 ------------------------- */
*A_Addr(adresse, COMX) = (160 + 31);  

/* --- SET DATA-POINTER HOLD-CYCLE TO COUNTER 1 - */ 
*A_Addr(adresse, COMX) = 25;  


I = NA; 
do { 
   ptr[I] = *A_Addr(adresse, DAX); 
   I = I + 1; 
}  while ( !(I > NE) ) ; 

ptr[0] = I-1;

return(ptr);
} 

/******************************************************************/
/*                                                                */
/* gibt (u_char *) der Laenge 7 auf Zaehlregister zurueck        */
/*                                                                */
/******************************************************************/

u_char *SIEM_303GETCTR5(adresse) 
int  adresse; 

{ 

char COUNT, CO0, DA0;
char CO1, DA1;
char COMX, DAX;
u_char *ptr;

ptr=(u_char *)malloc( 7*sizeof(ptr) );

COUNT = 4; 
/* ---------------------------------------------- */ 

/* --- CO0 := COMMAND-REGISTER COUNTER-CHIP 0 --- */ 

CO0 = COUNT + 1; DA0 = COUNT + 0; 
/* ------------------------------------- */ 

/* --- CO1 := COMMAND-REGISTER COUNTER-CHIP 1 --- */ 

CO1 = COUNT + 3; DA1 = COUNT + 2; 
/* ---------------------------------------------- */ 


/* ---------- READ CONTROL-GROUP -----------------*/ 

*A_Addr(adresse, COMX) = 7;             /* AL1REG */

ptr[1] = *A_Addr(adresse, DAX);            /* ALY1L */ 

ptr[2] = *A_Addr(adresse, DAX);            /* ALY1H */ 

ptr[3] = *A_Addr(adresse, DAX);            /* ALY2L */ 

ptr[4] = *A_Addr(adresse, DAX);            /* ALY2H */ 

ptr[5] = *A_Addr(adresse, DAX);             /* MMYL */ 

ptr[6] = *A_Addr(adresse, DAX);             /* MMYH */ 

ptr[0] = 6;

return(ptr);
} 
/*************************************************************/
