
/****************************************************************************/
/*                                                                          */
/* Die Prozeduren setzten eine Instrumentierungslist voraus.                */
/* dh. : 1) downloaddomain adresse1 type1 adresse2 typ2 ...                 */
/*       2) createis nr. dummy                                              */
/*       3) is nr.                                                          */
/*       4) downloadismodullist adresse1 adresse2 ...                       */
/*       5) zudem muss eine den Anforderungen entsprechende Variabelenliste */
/*          existieren.                                                     */
/*                                                                          */
/* Bem: Die ersten vier Prozeduren bearbeiten jeweils nur ein Element der   */
/*      Instrumentierungsliste. Die folgenden dagegen alle Module der je-   */
/*      weilig aktuellen Instrumentierungsliste.                            */
/*      Die Parameteruebergabe erfolgt ausschliesslich ueber Variablen. Die */
/*      Adressen ueber die Nummer des Moduls in der Instrumentierungsliste. */
/*      In dieser Version werden bei den *all-Prozeduren alle Module der    */
/*      Instrumentierungslist in ihrer Reihenfolge abgearbeitet. Die ueber- */
/*      gebenen Nummern der Module in der Instrumentierrungsliste sind hier */
/*      letztlich ohne Bedeutung.                                           */
/*                                                                          */
/* Erstellungsdatum : 24.05. '93                                            */
/*                                                                          */
/* letzte Aenderung : 11.06.                                                */
/*                    08.07. es werden Erebnisse nicht nur in Variablen     */
/*                    sondern auch ueber outptr.                            */
/*                                                                          */
/* Autor : Twardo (vertifiziert fuer EMS)                                   */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/


#include <errorcodes.h>
#include <modultypes.h>
#include <config.h>
#include <debug.h>
#include <types.h>
#include "../../../LOWLEVEL/VME/vme.h
#include "../../../OBJECTS/VAR/variables.h

extern ems_u32* outptr;
extern int* memberlist;

/****************************************************************************/
/* Procedure C_NE526_RUN(Adresse,Variable)                                  */
/*                                                                          */
/*           Adresse  = Basisadresse (input)                                */
/*           Variable = Mode (input)                                        */
/*                                                                          */
/* Confirmation: keine Daten                                                */
/*                                                                          */
/****************************************************************************/

plerrcode proc_C_NE526_RUN(p)

int *p;
{
int    welcher,adresse;
int dummy;

welcher=p[1];
adresse=memberlist[welcher];

/* --- Load CommandWord --- */
*A_Addr(adresse,5)=var_list[p[2]].var.val;

return(plOK);
}

plerrcode test_proc_C_NE526_RUN(p)

int *p;
{
   if ( *p++ !=2 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[*p]) != NE526 ){
      *outptr++=memberlist[*p];
                                   return(plErr_BadModTyp);
   }
   if ( *++p>MAX_VAR )             return(plErr_IllVar);
   if ( !var_list[*p].len )        return(plErr_NoVar);
   if ( var_list[*p].len !=1)      return(plErr_IllVarSize);


   return(plOK);
}
char name_proc_C_NE526_RUN[]="C_NE526_RUN";
int ver_proc_C_NE526_RUN=1;

/************************************************************************/
/*                                                                      */
/* Procedure C_NE526_INI(Adresse,Variable)                              */
/*                                                                      */
/*           Adresse  = Basisadresse (input)                            */
/*           Variable = Status (output)                                 */
/*                                                                      */
/* Confirmation: Status                                                 */
/*                                                                      */
/************************************************************************/

plerrcode proc_C_NE526_INI(p)
int *p;

{
int    welcher,adresse;
u_char dummy;

welcher=p[1];
adresse=memberlist[welcher];

/* ---- Stop Counter ---- */
*A_Addr(adresse,5)=0;

/* ---- Reset ----------- */
dummy=*A_Addr(adresse,4);

/* ---- Read Status ----- */

var_list[p[2]].var.val=*A_Addr(adresse,5);

/* ---- Confirmation ---- */
*outptr++=var_list[p[2]].var.val;

return(plOK);
}

plerrcode test_proc_C_NE526_INI(p)

int *p;
{
   if ( p[0] !=2 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != NE526 ){
                                   *outptr++=memberlist[*p];
                                   return(plErr_BadModTyp);
   }
   if ( p[2]>MAX_VAR )             return(plErr_IllVar);
   if ( !var_list[p[2]].len )        return(plErr_NoVar);
   if ( var_list[p[2]].len !=1)      return(plErr_IllVarSize);

return(plOK);

}
char name_proc_C_NE526_INI[]="C_NE526_INI";
int ver_proc_C_NE526_INI=1;

/************************************************************************/
/*                                                                      */
/* Procedure C_NE526_SET(Adresse,Variable)                              */
/*                                                                      */
/*           Adresse  = Basisadresse (input)                            */
/*           Variable = SET (input)                                     */
/*                                                                      */
/* Confirmation: keine Daten                                            */
/*                                                                      */
/************************************************************************/

plerrcode proc_C_NE526_SET(p)
int *p;
{
int    welcher,adresse;
u_char dummy,*part;

part=(u_char *)malloc(5*sizeof(part));
welcher=p[1];
adresse=memberlist[welcher];

/* --- Stop Counter ----------------------- */
*A_Addr(adresse,5)=0;

/* --- RESET ------------------------------ */
dummy=*A_Addr(adresse,4);

/* --- 32-BitInteger => 4*8-BitInteger --- */
part=MAKBYT(4,var_list[p[2]].var.val);

/* --- SET Zaehlregister------------------ */
*A_Addr(adresse,0)=part[2];

*A_Addr(adresse,1)=part[1];

*A_Addr(adresse,2)=part[4];

*A_Addr(adresse,3)=part[3];

return(plOK);
}

plerrcode test_proc_C_NE526_SET(p)

int *p;
{
   if ( p[0] !=2 )                  return(plErr_ArgNum);
   if ( !memberlist )               return(plErr_NoISModulList);

   if ( get_mod_type(memberlist[p[1]]) != NE526 ){
                                    *outptr++=memberlist[p[1]];
                                    return(plErr_BadModTyp);
   }
   if ( p[2]>MAX_VAR )              return(plErr_IllVar);
   if (!var_list[p[2]].len)         return(plErr_NoVar);
   if (var_list[p[2]].len !=1)      return(plErr_IllVarSize);

return(plOK);
}

char name_proc_C_NE526_SET[]="C_NE526_SET";
int ver_proc_C_NE526_SET=1;

/************************************************************************/
/*                                                                      */
/* Procedure C_NE526_GET(Adresse,Variable)                              */
/*                                                                      */
/*           Adresse  = Basisadresse (input)                            */
/*           Variable = Status (output)                                 */
/*           Variable = Inhalt der Zaehlregister (output)               */
/*                                                                      */
/* Confirmation: Status, Counter                                        */
/*                                                                      */
/************************************************************************/

plerrcode proc_C_NE526_GET(p)
int *p;
{
int    welcher,adresse;
u_char dummy,part[4];

welcher=p[1];
adresse=memberlist[welcher];

/* --- Read Status ----------------------- */
 var_list[p[2]].var.val=*A_Addr(adresse,5);

/* --- GET Zaehlregister ----------------- */
 part[2]=*A_Addr(adresse,0);

 part[1]=*A_Addr(adresse,1);

 part[4]=*A_Addr(adresse,2);

 part[3]=*A_Addr(adresse,3);


/* --- 4*8-BitInteger => 32-BitInteger --- */
 var_list[p[3]].var.val=MAKDOU( 4, part );

/* --- Confirmation: Status, Counter ----- */
*outptr++ = var_list[p[2]].var.val;
*outptr++ = var_list[p[3]].var.val;

return(plOK);
}

plerrcode test_proc_C_NE526_GET(p)

int *p;
{
   if ( p[0] !=3 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != NE526 ){
      *outptr++=memberlist[p[1]];
                                   return(plErr_BadModTyp);
   }
   if ( p[2]>MAX_VAR )             return(plErr_IllVar);
   if (!var_list[p[2]].len)        return(plErr_NoVar);
   if (var_list[p[2]].len !=1)     return(plErr_IllVarSize);
   if ( p[3]>MAX_VAR )             return(plErr_IllVar);
   if (!var_list[p[3]].len)        return(plErr_NoVar);
   if (var_list[p[3]].len !=1)     return(plErr_IllVarSize);

return(plOK);
}
char name_proc_C_NE526_GET[]="C_NE526_GET";
int ver_proc_C_NE526_GET=1;

/************************************************************************/
/************************************************************************/
/************************************************************************/

/************************************************************************/
/*                                                                      */
/* Procedure C_NE526_allRUN(Adresse1,Variable1,...)                     */
/*                                                                      */
/*           Adresse  = Basisadresse (input)                            */
/*           Variable = Mode (input)                                    */
/*              .         .                                             */
/*              .         .                                             */
/*              .         .                                             */
/*                                                                      */
/* Confirmation: keine Daten                                            */
/*                                                                      */
/************************************************************************/

plerrcode proc_C_NE526_allRUN(p)

int *p;
{
int    i,adresse;

for (i=1;i<=memberlist[0];i++){
   adresse=memberlist[i];

/* --- Load CommandWord --- */
   *A_Addr(adresse,5)=var_list[p[2*i]].var.val;

}/*for*/

return(plOK);
}/*end*/

plerrcode test_proc_C_NE526_allRUN(p)
int *p;

{
int i;

   if ( p[0]==0 )                  return(plErr_ArgNum);
   if ( p[0]%2!=0 )                return(plErr_ArgNum);

   if ( !memberlist )              return(plErr_NoISModulList);
   for ( i=1;i<=memberlist[0];i++ ){
      if ( get_mod_type(memberlist[i]) != NE526 ){
         *outptr++=memberlist[i];
                                   return(plErr_BadModTyp);
      }/*if*/
   }/*for*/

   for ( i=1;i<=memberlist[0];i++ ){
      if ( p[2*i]>MAX_VAR )        return(plErr_IllVar);
      if (!var_list[p[2*i]].len)   return(plErr_NoVar);
      if (var_list[p[2*i]].len !=1) return(plErr_IllVarSize);
   }/*for*/

   return(plOK);
}/*end*/

char name_proc_C_NE526_allRUN[]="C_NE526_allRUN";
int ver_proc_C_NE526_allRUN=1;

/************************************************************************/
/*                                                                      */
/* Procedure C_NE526_allINI(Adresse1,Variable1,...)                     */
/*                                                                      */
/*           Adresse  = Basisadresse (input)                            */
/*           Variable = Status (output)                                 */
/*              .         .                                             */
/*              .         .                                             */
/*              .         .                                             */
/*                                                                      */
/* Confirmation: Status                                                 */
/*                                                                      */
/************************************************************************/

plerrcode proc_C_NE526_allINI(p)
int *p;

{
int    i,adresse;
u_char dummy;

for (i=1;i<=memberlist[0];i++){

   adresse=memberlist[i];

/* --- Counter Stop --- */
   *A_Addr(adresse,5)=0;

/* --- Reset ---------- */
   dummy=*A_Addr(adresse,4);

/* --- Read Status ---- */
   var_list[p[2*i]].var.val=*A_Addr(adresse,5);

/* --- Confirmation --- */
   *outptr++ = var_list[p[2*i]].var.val;

}/*for*/

return(plOK);
}/*end*/

plerrcode test_proc_C_NE526_allINI(p)
int *p;

{
int i;

   if ( p[0]==0 )                  return(plErr_ArgNum);
   if ( p[0]%2!=0 )                return(plErr_ArgNum);

   if ( !memberlist )              return(plErr_NoISModulList);
   for ( i=1;i<=memberlist[0];i++ ){
      if ( get_mod_type(memberlist[i]) != NE526 ){
         *outptr++=memberlist[i];
                                   return(plErr_BadModTyp);
      }/*if*/
   }/*for*/

   for ( i=1;i<=memberlist[0];i++ ){
      if ( p[2*i]>MAX_VAR )        return(plErr_IllVar);
      if (!var_list[p[2*i]].len)   return(plErr_NoVar);
      if (var_list[p[2*i]].len !=1) return(plErr_IllVarSize);
   }/*for*/

   return(plOK);

}/*end*/

char name_proc_C_NE526_allINI[]="C_NE526_allINI";
int ver_proc_C_NE526_allINI=1;

/************************************************************************/
/*                                                                      */
/* Procedure C_NE526_allSET(Adresse1,Variable1,...)                     */
/*                                                                      */
/*           Adresse  = Basisadresse (input)                            */
/*           Variable = Set (input)                                     */
/*              .         .                                             */
/*              .         .                                             */
/*              .         .                                             */
/*                                                                      */
/* Confirmation: keine Daten                                            */
/*                                                                      */
/************************************************************************/

plerrcode proc_C_NE526_allSET(p)
int *p;

{
int    i,adresse;
u_char dummy,*part;

part=(u_char *)malloc(5*sizeof(part));

for ( i=1;i<=memberlist[0];i++ ){
   adresse=memberlist[i];

/* --- Counter Stop ---------------------- */
   *A_Addr(adresse,5)=0;

/* --- Reset ----------------------------- */
   dummy=*A_Addr(adresse,4);

/* --- 32-BitInteger => 4*8-BitInteger --- */
   part=MAKBYT(4,var_list[p[2*i]].var.val);

/* --- Set Zaehlregister------------------ */
  *A_Addr(adresse,0)=part[2];

  *A_Addr(adresse,1)=part[1];

  *A_Addr(adresse,2)=part[4];

  *A_Addr(adresse,3)=part[3];

}/*for*/

return(plOK);
}/*end*/

plerrcode test_proc_C_NE526_allSET(p)
int *p;

{
int i;

   if ( p[0]==0 )                  return(plErr_ArgNum);
   if ( p[0]%2!=0 )                return(plErr_ArgNum);

   if ( !memberlist )              return(plErr_NoISModulList);
   for ( i=1;i<=memberlist[0];i++ ){
      if ( get_mod_type(memberlist[i]) != NE526 ){
         *outptr++=memberlist[i];
                                   return(plErr_BadModTyp);
      }/*if*/
   }/*for*/

   for ( i=1;i<=memberlist[0];i++ ){
      if ( p[2*i]>MAX_VAR )        return(plErr_IllVar);
      if (!var_list[p[2*i]].len)   return(plErr_NoVar);
      if (var_list[p[2*i]].len !=1) return(plErr_IllVarSize);
   }/*for*/

   return(plOK);

}/*end*/

char name_proc_C_NE526_allSET[]="C_NE526_allSET";
int ver_proc_C_NE526_allSET=1;

/************************************************************************/
/*                                                                      */
/* Procedure C_NE526_allGET(Adresse1,Variable11,Variable12,...)         */
/*                                                                      */
/*           Adresse  = Basisadresse (input)                            */
/*           Variable = Status (output)                                 */
/*           Variable = Inhalt der Zaehlregister (output)               */
/*              .         .                                             */
/*              .         .                                             */
/*              .         .                                             */
/*                                                                      */
/* Confirmation: Status, Counter, ...                                   */
/*                                                                      */
/************************************************************************/

plerrcode proc_C_NE526_allGET(p)
int *p;

{

int    i,adresse;
u_char dummy,part[4];

for ( i=1;i<=memberlist[0];i++ ){
   adresse=memberlist[i];

/* --- Read Status ----------------------- */
   var_list[p[3*i-1]].var.val=*A_Addr(adresse,5);

/* --- Get Zaehlregister ----------------- */

   part[2]=*A_Addr(adresse,0);

   part[1]=*A_Addr(adresse,1);

   part[4]=*A_Addr(adresse,2);

   part[3]=*A_Addr(adresse,3);


/* --- 4*8-BitInteger => 32-BitInteger --- */
   var_list[p[3*i]].var.val=MAKDOU( 4,part );

/* --- Confirmation Status, Counter ------ */
*outptr++ = var_list[p[3*i-1]].var.val;
*outptr++ = var_list[p[3*i]].var.val;

}/*for*/

return(plOK);
}/*end*/

plerrcode test_proc_C_NE526_allGET(p)
int *p;

{
int i;

   if ( p[0]==0 )                  return(plErr_ArgNum);
   if ( p[0]%3!=0 )                return(plErr_ArgNum);

   if ( !memberlist )              return(plErr_NoISModulList);
   for ( i=1;i<=memberlist[0];i++ ){
      if ( get_mod_type(memberlist[i]) != NE526 ){
         *outptr++=memberlist[i];
                                   return(plErr_BadModTyp);
      }/*if*/
   }/*for*/

   for ( i=1;i<=memberlist[0];i++ ){
      if ( p[2*i]>MAX_VAR )        return(plErr_IllVar);
      if (!var_list[p[2*i]].len)   return(plErr_NoVar);
      if (var_list[p[2*i]].len !=1) return(plErr_IllVarSize);
   }/*for*/

   return(plOK);

}/*end*/

char name_proc_C_NE526_allGET[]="C_NE526_allGET";
int ver_proc_C_NE526_allGET=1;

/***********************************************************************/
