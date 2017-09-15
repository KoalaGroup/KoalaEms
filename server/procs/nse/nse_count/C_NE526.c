
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
/* Erstellungsdatum : 24.05. '93                                            */
/*                                                                          */
/* letzte Aenderung : 11.06.                                                */
/*                    08.07. es werden Erebnisse nicht nur in Variablen     */
/*                    sondern auch ueber outptr.                            */
/*                    09.12.                                                */
/*                                                                          */
/* Autor : Twardo (verifiziert fuer EMS)                                    */
/*                                                                          */
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

#include <stdio.h>
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
/* Procedure C_NE526_RUN(Adresse)                                         */
/*                                                                          */
/*           Adresse  = Basisadresse (input)                                */
/* Confirmation: keine Daten                                                */
/*                                                                          */
/****************************************************************************/

plerrcode proc_C_NE526_RUN(p)

int *p;
{
int    welcher,adresse;
int *varptr;
int dummy;

welcher=p[1];
varptr=var_list[p[1]].var.ptr;

/* --- gebe Modulnummer zurueck -------------------------- */
*outptr++= welcher;

adresse=memberlist[welcher];

/* --- Load CommandWord --- */
*A_Addr(adresse,5)=varptr[0];

D(D_REQ,printf("NE526_RUN Modul %d Adresse %x\n",p[1],adresse);)
D(D_REQ,printf("Mode %d\n",varptr[0]);)

return(plOK);
}

plerrcode test_proc_C_NE526_RUN(p)

int *p;
{
   if ( p[0] !=1 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != NE526 ){
      *outptr++=memberlist[p[1]];
                                   return(plErr_BadModTyp);
   }
   if ( p[1]>MAX_VAR )             return(plErr_IllVar);
   if ( !var_list[p[1]].len )      return(plErr_NoVar);
   if ( var_list[p[1]].len !=2)    return(plErr_IllVarSize);


   return(plOK);
}
char name_proc_C_NE526_RUN[]="C_NE526_RUN";
int ver_proc_C_NE526_RUN=1;

/************************************************************************/
/*                                                                      */
/* Procedure C_NE526_INI(Adresse)                                       */
/*                                                                      */
/*           Adresse  = Basisadresse (input)                            */
/*                                                                      */
/* Confirmation: keine Daten                                            */
/*                                                                      */
/************************************************************************/

plerrcode proc_C_NE526_INI(p)
int *p;

{
int    welcher,adresse;
u_char dummy;

welcher=p[1];

/* --- gebe Modulnummer zurueck -------------------------- */
*outptr++= welcher;

adresse=memberlist[welcher];

D(D_REQ,printf("NE526_INI Modul %d Adresse %x\n",p[1],adresse);)

/* ---- Stop Counter ---- */
*A_Addr(adresse,5)=0;

/* ---- Reset ----------- */
dummy=*A_Addr(adresse,4);

return(plOK);
}

plerrcode test_proc_C_NE526_INI(p)

int *p;
{
   if ( p[0] !=1 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != NE526 ){
                  D(D_REQ,printf("memberlist %d\n",memberlist[p[1]]);)
                                   *outptr++=memberlist[p[1]];
                                   return(plErr_BadModTyp);
   }

return(plOK);

}
char name_proc_C_NE526_INI[]="C_NE526_INI";
int ver_proc_C_NE526_INI=1;

/************************************************************************/
/*                                                                      */
/* Procedure C_NE526_SET(Adresse)                                       */
/*                                                                      */
/*           Adresse  = Basisadresse (input)                            */
/* Confirmation: keine Daten                                            */
/*                                                                      */
/************************************************************************/

plerrcode proc_C_NE526_SET(p)
int *p;
{
int    welcher,adresse;
int *varptr;
u_char dummy,*part;

part=(u_char *)malloc(5*sizeof(part));
welcher=p[1];

/* --- gebe Modulnummer zurueck -------------------------- */
*outptr++= welcher;
varptr=var_list[p[1]].var.ptr;

adresse=memberlist[welcher];

D(D_REQ,printf("NE526_SET Modul %d Adresse %x\n",p[1],adresse);)
D(D_REQ,printf("Set %d\n",varptr[1]);)

/* --- Stop Counter ----------------------- */
*A_Addr(adresse,5)=0;

/* --- RESET ------------------------------ */
dummy=*A_Addr(adresse,4);

/* --- 32-BitInteger => 4*8-BitInteger --- */
part=MAKBYT(4,varptr[1]);

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
   if ( p[0] !=1 )                  return(plErr_ArgNum);
   if ( !memberlist )               return(plErr_NoISModulList);

   if ( get_mod_type(memberlist[p[1]]) != NE526 ){
                                    *outptr++=memberlist[p[1]];
                                    return(plErr_BadModTyp);
   }
   if ( p[1]>MAX_VAR )              return(plErr_IllVar);
   if (!var_list[p[1]].len)         return(plErr_NoVar);
   if (var_list[p[1]].len !=2)      return(plErr_IllVarSize);

return(plOK);
}

char name_proc_C_NE526_SET[]="C_NE526_SET";
int ver_proc_C_NE526_SET=1;

/************************************************************************/
/*                                                                      */
/* Procedure C_NE526_GET(Adresse)                                       */
/*                                                                      */
/*           Adresse  = Basisadresse (input)                            */
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

/* --- gebe Modulnummer zurueck -------------------------- */
*outptr++= welcher;

adresse=memberlist[welcher];

D(D_REQ,printf("NE526_GET Modul %d Adresse %x\n",p[1],adresse);)

/* --- Read Status ----------------------- */
 *outptr++ =*A_Addr(adresse,5);

/* --- GET Zaehlregister ----------------- */
 part[2]=*A_Addr(adresse,0);

 part[1]=*A_Addr(adresse,1);

 part[4]=*A_Addr(adresse,2);

 part[3]=*A_Addr(adresse,3);


/* --- 4*8-BitInteger => 32-BitInteger --- */
 *outptr++ =MAKDOU( 4, part );

return(plOK);
}

plerrcode test_proc_C_NE526_GET(p)

int *p;
{
   if ( p[0] !=1 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != NE526 ){
      *outptr++ =memberlist[p[1]];
                                   return(plErr_BadModTyp);
   }

return(plOK);
}
char name_proc_C_NE526_GET[]="C_NE526_GET";
int ver_proc_C_NE526_GET=1;

/************************************************************************/
/************************************************************************/
/************************************************************************/

/************************************************************************/
/*                                                                      */
/* Procedure C_NE526_allRUN()                     */
/*                                                                      */
/* Confirmation: keine Daten                                            */
/*                                                                      */
/************************************************************************/

plerrcode proc_C_NE526_allRUN(p)
int *p;

{
int    i,adresse;
int *varptr;

for (i=1;i<=memberlist[0];i++){
/* --- gebe Modulnummer zurueck -------------------------- */
   *outptr++= i;

   adresse=memberlist[i];
   varptr=var_list[i].var.ptr;
   D(D_REQ,printf("NE526_RUN Modul %d Adresse %x\n",i,adresse);)
   D(D_REQ,printf("Mode %d\n",varptr[0]);)

/* --- Load CommandWord --- */
   *A_Addr(adresse,5)=varptr[0];

}/*for*/

return(plOK);
}/*end*/

plerrcode test_proc_C_NE526_allRUN(p)
int *p;

{
int i;

   if ( p[0] )                     return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   for ( i=1;i<=memberlist[0];i++ ){
      if ( get_mod_type(memberlist[i]) != NE526 ){
         *outptr++=memberlist[i];
                                   return(plErr_BadModTyp);
      }/*if*/
   }/*for*/

   for ( i=1;i<=memberlist[0];i++ ){
      if ( i>MAX_VAR )        return(plErr_IllVar);
      if (!var_list[i].len)   return(plErr_NoVar);
      if (var_list[i].len !=2) return(plErr_IllVarSize);
   }/*for*/

   return(plOK);
}/*end*/

char name_proc_C_NE526_allRUN[]="C_NE526_allRUN";
int ver_proc_C_NE526_allRUN=1;

/************************************************************************/
/*                                                                      */
/* Procedure C_NE526_allINI()                                           */
/*                                                                      */
/* Confirmation: keine Daten                                            */
/*                                                                      */
/************************************************************************/

plerrcode proc_C_NE526_allINI(p)
int *p;

{
int    i,adresse;
u_char dummy;

for (i=1;i<=memberlist[0];i++){

/* --- gebe Modulnummer zurueck -------------------------- */
   *outptr++= i;

   adresse=memberlist[i];

   D(D_REQ,printf("NE526_INI Modul %d Adresse %x\n",i,adresse);)

/* --- Counter Stop --- */
   *A_Addr(adresse,5)=0;

/* --- Reset ---------- */
   dummy=*A_Addr(adresse,4);

}/*for*/

return(plOK);
}/*end*/

plerrcode test_proc_C_NE526_allINI(p)
int *p;

{
int i;

   if ( p[0] )                  return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   for ( i=1;i<=memberlist[0];i++ ){
      if ( get_mod_type(memberlist[i]) != NE526 ){
         *outptr++=memberlist[i];
                                   return(plErr_BadModTyp);
      }/*if*/
   }/*for*/

   return(plOK);

}/*end*/

char name_proc_C_NE526_allINI[]="C_NE526_allINI";
int ver_proc_C_NE526_allINI=1;

/************************************************************************/
/*                                                                      */
/* Procedure C_NE526_allSET()                                             */
/*                                                                      */
/* Confirmation: keine Daten                                            */
/*                                                                      */
/************************************************************************/

plerrcode proc_C_NE526_allSET(p)
int *p;

{
int    i,adresse;
int *varptr;
u_char dummy,*part;

part=(u_char *)malloc(5*sizeof(part));

for ( i=1;i<=memberlist[0];i++ ){

/* --- gebe Modulnummer zurueck -------------------------- */
   *outptr++= i;

   adresse=memberlist[i];
   varptr=var_list[i].var.ptr;

   D(D_REQ,printf("NE526_SET Modul %d Adresse %x\n",i,adresse);)
   D(D_REQ,printf("Set %d\n",varptr[1]);)

/* --- Counter Stop ---------------------- */
   *A_Addr(adresse,5)=0;

/* --- Reset ----------------------------- */
   dummy=*A_Addr(adresse,4);

/* --- 32-BitInteger => 4*8-BitInteger --- */
   part=MAKBYT(4,varptr[1]);

/* --- Set Zaehlregister------------------ */
  *A_Addr(adresse,0)=part[2];

  *A_Addr(adresse,1)=part[1];

  *A_Addr(adresse,2)=part[4];

  *A_Addr(adresse,3)=part[3];

}/*for*/

free(part);
return(plOK);
}/*end*/

plerrcode test_proc_C_NE526_allSET(p)
int *p;

{
int i;

   if ( p[0] )                     return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   for ( i=1;i<=memberlist[0];i++ ){
      if ( get_mod_type(memberlist[i]) != NE526 ){
         *outptr++=memberlist[i];
                                   return(plErr_BadModTyp);
      }/*if*/
   }/*for*/

   for ( i=1;i<=memberlist[0];i++ ){
      if ( i>MAX_VAR )        return(plErr_IllVar);
      if (!var_list[i].len)   return(plErr_NoVar);
      if (var_list[i].len !=2) return(plErr_IllVarSize);
   }/*for*/

   return(plOK);

}/*end*/

char name_proc_C_NE526_allSET[]="C_NE526_allSET";
int ver_proc_C_NE526_allSET=1;

/************************************************************************/
/*                                                                      */
/* Procedure C_NE526_allGET()                                               */
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

   D(D_REQ,printf("NE526_GET Modul %d Adresse %x\n",i,adresse);)

/* --- gebe Modulnummer zurueck -------------------------- */
   *outptr++= i;


/* --- Read Status ----------------------- */
   *outptr++ =*A_Addr(adresse,5);

/* --- Get Zaehlregister ----------------- */

   part[2]=*A_Addr(adresse,0);

   part[1]=*A_Addr(adresse,1);

   part[4]=*A_Addr(adresse,2);

   part[3]=*A_Addr(adresse,3);


/* --- 4*8-BitInteger => 32-BitInteger --- */
   *outptr++ =MAKDOU( 4,part );

}/*for*/

return(plOK);
}/*end*/

plerrcode test_proc_C_NE526_allGET(p)
int *p;

{
int i;

   if ( p[0] )                     return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   for ( i=1;i<=memberlist[0];i++ ){
      if ( get_mod_type(memberlist[i]) != NE526 ){
         *outptr++=memberlist[i];
                                   return(plErr_BadModTyp);
      }/*if*/
   }/*for*/

   return(plOK);

}/*end*/

char name_proc_C_NE526_allGET[]="C_NE526_allGET";
int ver_proc_C_NE526_allGET=1;

/***********************************************************************/
