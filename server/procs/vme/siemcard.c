
/***********************************************************************/
/*                                                                     */
/* Die Prozeduren setzten eine Instrumentierungslist voraus.           */
/* dh.: 1) downloaddomain adresse1 typ1 adresse2 typ2 ...              */
/*      2) createis nr. dummy                                          */
/*      3) is nr.                                                      */
/*      4) downloadismodullist adresse1 adresse2 ...                   */
/*      5) zudem muss eine den Anforderungen entsprechende Variabelen- */
/*         liste existieren.                                           */
/*                                                                     */
/* Bem: Die ersten vier Prozeduren bearbeiten jeweils nur ein Element  */
/*      der Instrumentierungsliste. Die folgenden dagegen alle Module  */
/*      der jeweilig aktuellen Instrumentierungsliste. Weiterhin muss  */
/*      eine den Anfordeungen entsprechende Variablenliste existieren. */
/*      Naehere Angabe kann man den Prozedurkoepfen entnehmen.         */
/*                                                                     */
/* Erstellungsdatum: 04.06. '93                                        */
/*                                                                     */
/* letzte Aenderung:                                                   */
/*                                                                     */
/* Autor: Twardo (umgeschrieben fuer EMS)                              */
/*                                                                     */
/***********************************************************************/

#include <errorcodes.h>
#include <modultypes.h>
#include <debug.h>
#include <sconf.h>
#include <types.h>
#include "../../LOWLEVEL/VME/vme.h"
#include "../../OBJECTS/VAR/variables.h"

extern ems_u32* outptr;
extern int* memberlist;

/***********************************************************************/
/*                                                                     */
/* Procedure DBLRUN(adresse)                                           */
/*                  adresse = Basisadresse (input)                     */
/*                                                                     */
/***********************************************************************/

plerrcode proc_DBLRUN(p)
int *p; 

{ 

int  welcher,adresse;
char COUNT, CO0, DA0;
char CO1, DA1;
    
welcher=p[1];
adresse=memberlist[welcher];

COUNT = 4; 
/* ---------------------------------------------- */ 
/* --- CO0 := COMMAND-REGISTER COUNTER-CHIP 0 --- */ 

CO0 = COUNT + 1;  
/* ---------------------------------------------- */ 

/* --- CO1 := COMMAND-REGISTER COUNTER-CHIP 1 --- */ 

CO1 = COUNT + 3; DA1 = COUNT + 2; 
/* ---------------------------------------------- */ 


*A_Addr(adresse,CO0) = (96 + 31);        /* CHIP 0 */ 

*A_Addr(adresse,CO1) = (96 + 31);        /* CHIP 1 */ 


return(plOK);
} 

plerrcode test_proc_DBLRUN(p)
int  *p;

{
if ( *p!=1 )                                      return(plErr_ArgNum);
if ( !memberlist )                                return(plErr_NoISModulList);
if ( get_mod_type(memberlist[p[1]])!=SIEMENS ) { 
   *outptr++=memberlist[p[1]];
                                                  return(plErr_BadModTyp);
   }

return(plOK);
}
char name_proc_DBLRUN[]="DBLRUN";
int ver_proc_DBLRUN=1;

/*****************************************************************************/
/*                                                                           */
/* Procedure DBLINI(adresse,variable1,...,variable10)                        */
/*                  adresse   = Basisadresse (input)                         */
/*                                                                           */
/*                  variable1 = 1mod1,1mod2,1load1,1load2,1hold1,1hold2      */
/*                      .     =   .     .      .      .     .      .         */
/*                      .     =   .     .      .      .     .      .         */
/*                      .     =   .     .      .      .     .      .         */
/*                  variable10= 10mod1,10mod2,10load1,10load2,10hold1,10hold2*/
/*                                                                           */
/* Bem: Es muessen also zu der Adresse 10 Variablen der Laenge 6 uebergeben  */
/*      werden.                                                              */
/*                                                                           */
/*****************************************************************************/

plerrcode proc_DBLINI(p)
int  *p;
 
{
int welcher,adresse,i;
u_char *buff;
register int *var1 =(int *)var_list[p[2]].var,
             *var2 =(int *)var_list[p[3]].var,
             *var3 =(int *)var_list[p[4]].var,
             *var4 =(int *)var_list[p[5]].var,
             *var5 =(int *)var_list[p[6]].var,
             *var6 =(int *)var_list[p[7]].var,
             *var7 =(int *)var_list[p[8]].var,
             *var8 =(int *)var_list[p[9]].var,
             *var9 =(int *)var_list[p[10]].var,
             *var10=(int *)var_list[p[11]].var;

welcher=p[1];
adresse=memberlist[welcher];    

SIEM_303SETCTR5(adresse); 
SIEM_303SETMLH5(adresse); 
buff=SIEM_303GETMLH5(adresse); 

for (i=1;i<=6;i++)   var1[i] =*++buff;
for (i=6;i<=12;i++)  var2[i] =*++buff;
for (i=12;i<=18;i++) var3[i] =*++buff;
for (i=18;i<=24;i++) var4[i] =*++buff;
for (i=24;i<=30;i++) var5[i] =*++buff;
for (i=30;i<=36;i++) var6[i] =*++buff;
for (i=36;i<=42;i++) var7[i] =*++buff;
for (i=42;i<=48;i++) var8[i] =*++buff;
for (i=48;i<=54;i++) var9[i] =*++buff;
for (i=54;i<=60;i++) var10[i]=*++buff;

return(plOK);
} 

plerrcode test_proc_DBLINI(p)
int  *p;

{
int i;

if ( *p!=11 )                                      return(plErr_ArgNum);
if ( !memberlist )                                 return(plErr_NoISModulList);
if ( get_mod_type(memberlist[p[1]])!=SIEMENS ) { 
   *outptr++=memberlist[p[1]];
                                                   return(plErr_BadModTyp);
   }

for (i=2;i<=11;i++) { if ( p[i]>MAX_VAR )          return(plErr_IllVar);
}
for (i=2;i<=11;i++) { if ( !var_list[p[i]].len )   return(plErr_IllVar);
}
for (i=2;i<=11;i++) { if ( var_list[p[i]].len!=6 ) return(plErr_IllVar);
}

return(plOK);
}
char name_proc_DBLINI[]="DBLINI";
int ver_proc_DBLINI=1;



/*****************************************************************************/
/*                                                                           */
/* Procedure DBLSET(adresse,variable1,...,variable10)                        */
/*                  adresse   = Basisadresse (input)                         */
/*                                                                           */
/*                  variable1 = 1mod1,1mod2,1load1,1load2,1hold1,1hold2      */
/*                      .     =   .     .      .      .     .      .         */
/*                      .     =   .     .      .      .     .      .         */
/*                      .     =   .     .      .      .     .      .         */
/*                  variable10= 10mod1,10mod2,10load1,10load2,10hold1,10hold2*/
/*                                                                           */
/* Bem: Es muessen also zu der Adresse 10 Variablen der Laenge 6 uebergeben  */
/*      werden.                                                              */
/*                                                                           */
/*****************************************************************************/

plerrcode proc_DBLSET(p)
int  *p;
 
{
int welcher,adresse,i;
u_char *buff;
register int *var1 =(int *)var_list[p[2]].var,
             *var2 =(int *)var_list[p[3]].var,
             *var3 =(int *)var_list[p[4]].var,
             *var4 =(int *)var_list[p[5]].var,
             *var5 =(int *)var_list[p[6]].var,
             *var6 =(int *)var_list[p[7]].var,
             *var7 =(int *)var_list[p[8]].var,
             *var8 =(int *)var_list[p[9]].var,
             *var9 =(int *)var_list[p[10]].var,
             *var10=(int *)var_list[p[11]].var;

welcher=p[1];
adresse=memberlist[welcher];    

SIEM_303SETCTR5(adresse); 
SIEM_303SETMLH5(adresse); 
buff=SIEM_303GETMLH5(adresse); 

for (i=1;i<=6;i++)   var1[i] =*++buff;
for (i=6;i<=12;i++)  var2[i] =*++buff;
for (i=12;i<=18;i++) var3[i] =*++buff;
for (i=18;i<=24;i++) var4[i] =*++buff;
for (i=24;i<=30;i++) var5[i] =*++buff;
for (i=30;i<=36;i++) var6[i] =*++buff;
for (i=36;i<=42;i++) var7[i] =*++buff;
for (i=42;i<=48;i++) var8[i] =*++buff;
for (i=48;i<=54;i++) var9[i] =*++buff;
for (i=54;i<=60;i++) var10[i]=*++buff;

return(plOK);
} 

plerrcode test_proc_DBLSET(p)
int  *p;

{
int i;

if ( *p!=11 )                                      return(plErr_ArgNum);
if ( !memberlist )                                 return(plErr_NoISModulList);
if ( get_mod_type(memberlist[p[1]])!=SIEMENS ) { 
   *outptr++=memberlist[p[1]];
                                                   return(plErr_BadModTyp);
   }

for (i=2;i<=11;i++) { if ( p[i]>MAX_VAR )          return(plErr_IllVar);
}
for (i=2;i<=11;i++) { if ( !var_list[p[i]].len )   return(plErr_IllVar);
}
for (i=2;i<=11;i++) { if ( var_list[p[i]].len!=6 ) return(plErr_IllVar);
}

return(plOK);
}
char name_proc_DBLSET[]="DBLSET";
int ver_proc_DBLSET=1;


/*****************************************************************************/
/*                                                                           */
/* Procedure DBLGET(adresse,variable1,...,variable10)                        */
/*                  adresse   = Basisadresse (input)                         */
/*                                                                           */
/*                  variable1 = 1mod1,1mod2,1load1,1load2,1hold1,1hold2      */
/*                      .     =   .     .      .      .     .      .         */
/*                      .     =   .     .      .      .     .      .         */
/*                      .     =   .     .      .      .     .      .         */
/*                  variable10= 10mod1,10mod2,10load1,10load2,10hold1,10hold2*/
/*                                                                           */
/* Bem: Es muessen also zu der Adresse 10 Variablen der Laenge 6 uebergeben  */
/*      werden.                                                              */
/*                                                                           */
/*****************************************************************************/

plerrcode proc_DBLGET(p)
int  *p; 

{ 
double ASK;

/*

GETCOU5(C_303_ACTNUM); 
GETMLH5(C_303_ACTNUM); 


T = C_DBL_INTERN[i] * 4 - 3; 
CN = C_DBL_CARDNUM[i]; 
C_DBL_ACTBYT[i][1] = C_303_CARD[CN].COUNTERS[T]; 
PART[1] = C_DBL_ACTBYT[i][1]; 


C_DBL_ACTBYT[i][2] = C_303_CARD[CN].COUNTERS[T + 1]; 
PART[2] = C_DBL_ACTBYT[i][2]; 

C_DBL_ACTBYT[i][3] = C_303_CARD[CN].COUNTERS[T + 2]; 
PART[3] = C_DBL_ACTBYT[i][3]; 

C_DBL_ACTBYT[i][4] = C_303_CARD[CN].COUNTERS[T + 3]; 
PART[4] = C_DBL_ACTBYT[i][4]; 

S_MAKDOU(C_DBL_GETCOUNT[i], 4, PART); 
*/


return(plOK);
}
plerrcode test_proc_DBLGET(p)
int  *p;

{
int i;

if ( *p!=11 )                                      return(plErr_ArgNum);
if ( !memberlist )                                 return(plErr_NoISModulList);
if ( get_mod_type(memberlist[p[1]])!=SIEMENS ) { 
   *outptr++=memberlist[p[1]];
                                                   return(plErr_BadModTyp);
   }

for (i=2;i<=11;i++) { if ( p[i]>MAX_VAR )          return(plErr_IllVar);
}
for (i=2;i<=11;i++) { if ( !var_list[p[i]].len )   return(plErr_IllVar);
}
for (i=2;i<=11;i++) { if ( var_list[p[i]].len!=6 ) return(plErr_IllVar);
}

return(plOK);
}
char name_proc_DBLGET[]="DBLGET";
int ver_proc_DBLGET=1;


/***************************************************************************/
