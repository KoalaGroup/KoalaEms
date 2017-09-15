
/****************************************************************************/
/*                                                                          */
/* ENCODER-ROC717                                                           */
/*                                                                          */
/*                                                                          */
/* Erstellungsdatum : 07.07. '93                                            */
/*                                                                          */
/* letzte Aenderung : 08.07. Der Wert des Encoders wird auch ueber die      */
/*                    Confirmation zurueckgegeben.                          */
/*                                                                          */
/* Autor : Twardo (verifiziert fuer EMS)                                   */
/*                                                                          */
/* Bemerkung : Wert und Status des Encoders wird in eine Variable.          */
/*             geschrieben. Das bietet die Moeglichkeit mit anderen Klients */
/*             Werte auszulesen, ohne auf den SMP-Rahmen zuzugreifen. Zudem */
/*             werden die Ergebnisse direkt ueber die Socket an den Klient  */
/*             geschickt. In dieser Version werden bei der Prozedure        */
/*             E_ROCSMPall die Module in der Reihenfolge bearbeitet in der  */
/*             sie angegeben werden. Wird eine Nummer der Modulliste ueber- */
/*             geben kommt es zu einem Fehler, der nicht abgefangen wird.   */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/


#include <errorcodes.h>
#include <modultypes.h>
#include <sconf.h>
#include <debug.h>
#include <types.h>
#include "../../../LOWLEVEL/VME/vme.h"
#include "../../../OBJECTS/VAR/variables.h"

extern ems_u32* outptr;
extern int* memberlist;

/****************************************************************************/
/* Procedure E_ROCSMP(Adresse,Variable)                                     */
/*                                                                          */
/*           Adresse  = Basisadresse (input)                                */
/*           Variable = Wert des Encoders (input)                           */
/*                                                                          */
/* Confirmation: Wert des Encoders                                          */
/*                                                                          */
/****************************************************************************/

plerrcode proc_E_ROCSMP(p)

int *p;
{
int    welcher,adresse;
u_char l_byte,m_byte,h_byte;
u_char rep=0;
int    wert,altwert,status;

D(D_REQ,printf("E_ROCSMP erfolgreich\n");)
*outptr++=p[1];

welcher=p[1];
adresse=memberlist[welcher];

var_list[p[2]].var.val=(-1);

/* --- lese bis Statusregister = 255 -------------------------- */
do {
   status = *A_Addr(adresse,3);
} while( status!=255 );

/* --- lese drei Byte aus ------------------------------------- */
l_byte = *A_Addr(adresse,0);
m_byte = *A_Addr(adresse,1);
h_byte = *A_Addr(adresse,2);

/* --- nur das erste Byte zaehlt ------------------------------ */
h_byte = h_byte & 1;

/* --- die drei Byte werden zu einem Integer zusammengesetzt ---*/
altwert = h_byte*256*256+m_byte*256+l_byte;

/* --- dies wird wiederholt bis die Werte die nun zurueckgelesen*/
/* --- werden gleich sind. Wird dies nicht erreicht erhaelt, die*/
/* --- Variable den Wert -1. ---------------------------------- */
do {
   do {
      status = *A_Addr(adresse,3);
   } while( status!=255 );

   l_byte = *A_Addr(adresse,0);
   m_byte = *A_Addr(adresse,1);
   h_byte = *A_Addr(adresse,2);

   h_byte = h_byte & 1;

   wert = h_byte*256*256+m_byte*256+l_byte;

   if(wert==altwert) {
      var_list[p[2]].var.val=wert;
      rep=10;
/* --- Confirmation Wert -------------------------------------- */
*outptr++ = wert;
   }
   else {
      rep=rep+1;
      altwert=wert;
   }
} while(rep<10);

return(plOK);
}

plerrcode test_proc_E_ROCSMP(p)

int *p;
{
   if ( p[0] !=2 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != ROC717 ){
      *outptr++=memberlist[p[1]];
                                   return(plErr_BadModTyp);
   }
   if ( p[2]>MAX_VAR )             return(plErr_IllVar);
   if ( !var_list[p[2]].len )      return(plErr_NoVar);
   if ( var_list[p[2]].len !=1)    return(plErr_IllVarSize);


   return(plOK);
}
char name_proc_E_ROCSMP[]="E_ROCSMP";
int ver_proc_E_ROCSMP=1;

/****************************************************************************/
/* Procedure E_ROCSMPall(Adresse,Variable,...)                              */
/*                                                                          */
/*           Adresse  = Basisadresse 1 (input)                              */
/*           Variable = Wert des Encoders 2 (input)                         */
/*              .     =       .                                             */
/*              .     =       .                                             */
/*              .     =       .                                             */
/*                                                                          */
/* Confirmation: Werte de Encoder                                           */
/*                                                                          */
/****************************************************************************/

plerrcode proc_E_ROCSMPall(p)

int *p;
{
int    welcher,adresse;
u_char l_byte,m_byte,h_byte;
u_char rep=0;
int    wert,altwert,status;
int    i;

for(i=1;i<=memberlist[0];i++) {

   welcher=p[2*i-1];
   adresse=memberlist[welcher];

   var_list[p[2*i]].var.val=(-1);

/* --- lese bis Statusregister = 255 -------------------------- */
   do {
      status = *A_Addr(adresse,3);
   } while( status!=255 );

/* --- lese drei Byte aus ------------------------------------- */
   l_byte = *A_Addr(adresse,0);
   m_byte = *A_Addr(adresse,1);
   h_byte = *A_Addr(adresse,2);

/* --- nur das erste Byte zaehlt ------------------------------ */
   h_byte = h_byte & 1;

/* --- die drei Byte werden zu einem Integer zusammengesetzt ---*/
   altwert = h_byte*256*256+m_byte*256+l_byte;

/* --- dies wird wiederholt bis die Werte die nun zurueckgelesen*/
/* --- werden gleich sind. Wird dies nicht erreicht erhaelt, die*/
/* --- Variable den Wert -1. ---------------------------------- */
   do {
      do {
         status = *A_Addr(adresse,3);
      } while( status!=255 );

      l_byte = *A_Addr(adresse,0);
      m_byte = *A_Addr(adresse,1);
      h_byte = *A_Addr(adresse,2);

      h_byte = h_byte & 1;

      wert = h_byte*256*256+m_byte*256+l_byte;

      if(wert==altwert) {
         var_list[p[2*i]].var.val=wert;
         rep=10;
/* --- Confirmation Wert -------------------------------------- */
   *outptr++ = wert;
      }
      else {
         rep=rep+1;
         altwert=wert;
      }
   } while(rep<10);

} /*for*/
return(plOK);
}

plerrcode test_proc_E_ROCSMPall(p)

int *p;
{
int i;

   if ( p[0] == 0 )                return(plErr_ArgNum);
   if ( p[0]%2 != 0 )              return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);

   for(i=1;i<=memberlist[0];i++) {
      if ( get_mod_type(memberlist[p[i]]) != ROC717 ){
         *outptr++=memberlist[p[i]];
                                   return(plErr_BadModTyp);
      }
   } /*for*/
   for(i=1;i<=memberlist[0];i++) {
      if ( p[2*i]>MAX_VAR )        return(plErr_IllVar);
      if ( !var_list[p[2*i]].len ) return(plErr_NoVar);
      if ( var_list[p[2*i]].len !=1) return(plErr_IllVarSize);
   }

   return(plOK);
}
char name_proc_E_ROCSMPall[]="E_ROCSMPall";
int ver_proc_E_ROCSMPall=1;

/********************************************************************/

