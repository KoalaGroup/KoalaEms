
/****************************************************************************/
/*                                                                          */
/*                                                                          */
/* Erstellungsdatum : 12.07. '93                                            */
/*                                                                          */
/* letzte Aenderung:  18.08. '93                                            */
/*                                                                          */
/* Autor : Twardo (verifiziert fuer EMS)                                    */
/*                                                                          */
/****************************************************************************/


#include <math.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <sconf.h>
#include <debug.h>
#include <types.h>
#include "../../MAIN/signals.h"
#include "../../LOWLEVEL/VME/vme.h"
#include "../../OBJECTS/VAR/variables.h"
#include "M_E362.h"
#include "list.h"

#define TIME(look) ( (look<<8) | 0x80000000 )
#define CM 25016
#define MM 2502

extern ems_u32* outptr;
extern int* memberlist;

/* Variablen fuer die verkette Liste */
listpointer *ptr,*head,*c_n;
stdelement element;
int fail;

/* statische Variable, die anzeigt, ob eine Liste besteht */
static int list_exist=0;

/* Es folgen globale Variablen fuer m_e362 */
u_char CNTR_r[]={0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87};
int    R1_[]={  10,  20, 50,100,200, 400,1000};
int    R2_[]={   0,   0,  0,  0,  0,1000,1000};
int    R4_[]={   0,   0,  0,  0,  0,2000,4000};
int    R5_[]={   0,   0,  0,  0,  0,2000,4000};
int    R6_[]={   0,   0,  0,  0,  0,2200,2000};
int    R7_[]={2000,1000,500,100,200, 400,1000};

u_char motor_r[8];

/****************************************************************************/
/* Procedure M_E362_goslowperm(Adresse,CONST,CONST2)                        */
/*                                                                          */
/*           Adresse  = Basisadresse (input)                                */
/*           CONST1   = R1 Startsequenz (input)                             */
/*           CONST2   = R7 Stoppsequenz (input)                             */
/*                                                                          */
/* Confirmation: Encoderwert (im moment nicht)                              */
/*                                                                          */
/****************************************************************************/

plerrcode proc_M_E362_goslowperm(p)

int *p;
{
int    welcher,adresse;

welcher=p[1];
adresse=memberlist[welcher];
*outptr++=p[1];
D(D_REQ,printf("M_E362_goslowperm Modul %d\n",welcher);)

motor_r[1] = p[2];
motor_r[7] = p[3];

*A_Addr(adresse,0)=CNTR_reset;
*A_Addr(adresse,0)=CNTR_master;

loadr(adresse,1,motor_r[1]);
loadr(adresse,7,motor_r[7]);

*A_Addr(adresse,0)=CNTR_slowneg;
*A_Addr(adresse,0)=GO_slow;

return(plOK);
}

plerrcode test_proc_M_E362_goslowperm(p)

int *p;
{
   if ( *p++ !=3 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != E362) {
      *outptr++=memberlist[*p];
                                   return(plErr_BadModTyp);
   }

   return(plOK);
}
char name_proc_M_E362_goslowperm[]="M_E362_goslowperm";
int ver_proc_M_E362_goslowperm=1;

/****************************************************************************/
/*                                                                          */
/* Procedure M_E362_goslowpul(Adresse,CONST1,CONST2,CONST3)                 */
/*                                                                          */
/*           Adresse  = Basisadresse (input)                                */
/*           CONST1   = R1 Startsequenz (input)                             */
/*           CONST2   = R7 Stoppsequenz (input)                             */
/*           CONST3   = Anzahl der Pulse                                    */
/*                                                                          */
/* Confirmation: keine Daten                                                */
/*                                                                          */
/****************************************************************************/

plerrcode proc_M_E362_goslowpul(p)

int *p;
{
int    welcher,adresse,pulse;

welcher=p[1];
adresse=memberlist[welcher];
*outptr++=p[1];
D(D_REQ,printf("M_E362_goslowpuls Modul %d\n",welcher);)

motor_r[1] = p[2];
motor_r[7] = p[3];
pulse      = p[4];

*A_Addr(adresse,0)=CNTR_reset;
*A_Addr(adresse,0)=CNTR_master;

loadr(adresse,1,motor_r[1]);
loadr(adresse,7,motor_r[7]);

loadpulse(adresse,pulse);

*A_Addr(adresse,0)=CNTR_step;
*A_Addr(adresse,0)=GO_slow;

return(plOK);
}

plerrcode test_proc_M_E362_goslowpul(p)

int *p;
{
   if ( *p++ !=4 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != E362) {
      *outptr++=memberlist[*p];
                                   return(plErr_BadModTyp);
   }

   return(plOK);
}
char name_proc_M_E362_goslowpul[]="M_E362_goslowpul";
int ver_proc_M_E362_goslowpul=1;


/****************************************************************************/
/*                                                                          */
/* Procedure M_E362_stop(adresse,variable,stopmode)                         */
/*                                                                          */
/*           Adresse  = Basisadresse (input)                                */
/*           variable = zum Motor zugehoerige Variable                      */
/*                      wird benoetigt, um an die ID's fuer die Alarme be-  */
/*                      reitzustellen                                       */
/*           stopmode = 0     stop immediatelly (input)                     */
/*                      sonst gently                                        */
/*                                                                          */
/* Confirmation: keine Daten                                                */
/*                                                                          */
/****************************************************************************/

plerrcode proc_M_E362_stop(p)

int *p;
{
int    welcher,adresse;
int    *var_ptr;

welcher=p[1];
adresse=memberlist[welcher];
*outptr++=p[1];
D(D_REQ,printf("M_E362_stop Modul %d\n",welcher);)

var_ptr=(int *)var_list[p[2]].var.ptr;

/* ___ Motormechanik stoppen ----------------------------------- */
if (p[3]) *A_Addr(adresse,0)=CNTR_reset;
else *A_Addr(adresse,0)=CNTR_stop;

/* --- zyklischen Alarm und Timeout loeschen ------------------- */
alm_delete(var_ptr[7]);
alm_delete(var_ptr[8]);

/* --- Datenbereich und Signale im Signalhandler freigeben ----- */
remove_signalhandler(var_ptr[9]);
remove_signalhandler(var_ptr[10]);

return(plOK);
}

plerrcode test_proc_M_E362_stop(p)

int *p;
{
   if ( p[0] !=3 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]])!= E362) {
      *outptr++=memberlist[p[1]];
                                   return(plErr_BadModTyp);
   }
   if ( p[2] > MAX_VAR )           return(plErr_IllVar);
   if ( !var_list[p[2]].len )      return(plErr_NoVar);
   if (  var_list[p[2]].len != 11 ) return(plErr_IllVarSize); 
   
   return(plOK);
}
char name_proc_M_E362_stop[]="M_E362_stop";
int ver_proc_M_E362_stop=1;


/****************************************************************************/
/*                                                                          */
/* Procedure M_E362_status(motoradresse,encoderadresse)                     */
/*                                                                          */
/*           MotorAdresse  = Basisadresse Nr. aus der Instrumentierungsliste*/
/*           EncoderAdresse= Basisadresse Nr. aus der Instrumentierungsliste*/
/*                                                                          */
/* Confirmation: Statusregister, Pulsregister, Encoderwert in 1/10 mm       */
/*                                                                          */
/****************************************************************************/

plerrcode proc_M_E362_status(p)

int *p;
{
int m_adresse,e_adresse,channel;
float value,help;

*outptr++=p[1];
D(D_REQ,printf("M_E362_status Modul %d\n",p[1]);)

m_adresse=memberlist[p[1]];
e_adresse=memberlist[p[2]];

*outptr++ = *A_Addr(m_adresse,0);
*outptr++ = getpulse(m_adresse);

switch ( get_mod_type(memberlist[p[2]]) ) {
   case ROC717 : *outptr++ = encoderwert_ROC(e_adresse);break;   
   case E233_0 : channel=0;
                 value = encoderwert_E233(e_adresse,channel);
                 help = ( (VOLT_max-value)*5.3625377+CM_min )*100;
D(D_REQ,printf("STATUS SCHUBSTANGE: mm=%f Volt=%f   \n",help,value);)

/* --- Rueckgabewert in Milimeter --------------------------------------- */
                 *outptr++ = (int) ( (floor(help)+ceil(help))/2.0 );
                 break;
   case E233_1 : channel=1;
                 value = encoderwert_E233(e_adresse,channel);
                 help = ( (VOLT_max-value)*5.3625377+CM_min )*10;

/* --- Rueckgabewert in Milimeter --------------------------------------- */
                 *outptr++ = (int) ( (floor(help)+ceil(help))/2.0 );
                 break;
}
return(plOK);
}

plerrcode test_proc_M_E362_status(p)

int *p;
{
   if ( p[0] !=2 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( (get_mod_type(memberlist[p[1]])!= E362)||
        ((get_mod_type(memberlist[p[2]])!= E233_0)&&
        (get_mod_type(memberlist[p[2]])!= E233_1)&&
        (get_mod_type(memberlist[p[2]])!= ROC717)) ){
      *outptr++=memberlist[p[1]];
                                   return(plErr_BadModTyp);
   }
   
   return(plOK);
}
char name_proc_M_E362_status[]="M_E362_status";
int ver_proc_M_E362_status=1;



/****************************************************************************/
/*                                                                          */
/* Procedure M_E362(Adresse,CONST1,CONST2,CONST3,CONST4,CONST5,Adresse)     */
/*                                                                          */
/*           Adresse  = Basisadresse der Motorkarte                         */
/*           CONST1   = Richtung negativ=0                                  */
/*           CONST2   = Geschindigkeit 1..8                                 */
/*           CONST3   = Winkel=0, Zentimeter=1, sonst Pulse=2               */
/*           CONST4   = Grad, Zentimeter oder Pulse                         */
/*           CONST5   = Minuten                                             */
/*           CONST6   = Sekunden                                            */
/*           CONST7   = justieren nein=0                                    */
/*           Adresse  = Basisadresse des Encoders                           */
/*           Variable = Variablennummer fuer Status                         */
/*           try      = Anzahl der Versuche zum Positionieren               */
/*           t_out    = Timeout in Sekunden                                 */
/*                                                                          */
/* Confirmation: keine                                                      */
/*                                                                          */
/* Bemerkung: Es wird nocht nicht beruecksichtigt, dass der Motor nach      */
/*            0x1ffff Pulsen eine ganze Umdrehung gemacht hat. Dies wird in */
/*            dem Programm nicht abgefangen, so dass das Programm in einen  */
/*            "Endlosloop" geraet.                                          */
/*            Die hier angebotenen Geschwindigkeiten muessen auch noch      */
/*            ueberdacht werden. Sie muessen wohl an dem tatsaechlichen     */
/*            Experiment erstellt werden.                                   */
/*                                                                          */
/****************************************************************************/


plerrcode proc_M_E362(p)

int *p;
{

ClientData daten;
int *var_ptr,channel;
int look=1,out=120; /* als Parameter!!! */
float hilf;

*outptr++=p[1];
D(D_REQ,printf("M_E362 Modul %d\n",p[1]);)

/* --- Pointer auf die Variable setzten -------------------------------- */
var_ptr=(int *)var_list[p[10]].var.ptr;

/* --- Adresse des Motors ---------------------------------------------- */
element.motoradresse=memberlist[p[1]];

/* --- Typ des Motors -------------------------------------------------- */
element.motortyp=get_mod_type(memberlist[p[1]]);

/* --- Adresse des Encoders -------------------------------------------- */
element.encoderadresse=memberlist[p[9]];

/* --- Typ des Encoders ------------------------------------------------ */
element.encodertyp=get_mod_type(memberlist[p[9]]);

/* --- Nummer der Variablen fuer den Status in die Struktur schreiben -- */
element.var=p[10];

/* --- Reset- und MasterMode ------------------------------------------- */
*A_Addr(element.motoradresse,0)=CNTR_reset;
*A_Addr(element.motoradresse,0)=CNTR_master;

/* --- Encoder auslesen (fuer den Encoder E233 nicht noetig, da in   --- */
/* --- dem Fall nicht justiert wird) ----------------------------------- */
switch ( element.encodertyp ) {
   case ROC717 : element.encoderalt=encoderwert_ROC(element.encoderadresse);
                 break;
   case E233_0 : channel=0;
                 break;
   case E233_1 : channel=1;
                 break;
}

/* --- Register 1,2,4,5,6,7 mit der gewaehlten Geschwindigkeit laden --- */
loadr(element.motoradresse,1,R1_[p[3]]);
loadr(element.motoradresse,2,R2_[p[3]]);
loadr(element.motoradresse,4,R4_[p[3]]);
loadr(element.motoradresse,5,R5_[p[3]]);
loadr(element.motoradresse,6,R6_[p[3]]);
loadr(element.motoradresse,7,R7_[p[3]]);

/* --- Winkel, Zentimeter oder Pulse ----------------------------------- */
switch ( p[4] ) {
   case 0 : element.pulse=(60*60*p[5]+60*p[6]+p[7])/9;
            break;
   case 1 : element.pulse=CM*p[5]+MM*p[6];
            break;
   case 2 : element.pulse=p[5];
            break;
}

/* --- die Anzahl der Pulse laden -------------------------------------- */
loadpulse(element.motoradresse,element.pulse);

/* --- Ziel feststellen (nicht fuer den Encoder E233) ------------------ */
if ( element.encodertyp==ROC717 ) {
   if ( p[2] ) element.ziel=element.encoderalt+p[5];
   else        element.ziel=element.encoderalt-p[5];
}

/* --- alle fuer die Interceptroutine benoetigten Parameter in die ----- */
/* --- Struktur laden -------------------------------------------------- */
element.encoder=0;
element.dif=0;
element.justieren=p[8];
element.direction=p[2];
element.again=0;
element.try=p[11];
element.rep=0;

daten.ptr=(int *)malloc(22*sizeof(int));
/* muss aber auch wieder freigegeben werden */

daten.ptr[1]=element.motoradresse;
daten.ptr[2]=element.motortyp;
daten.ptr[3]=element.encoderadresse;
daten.ptr[4]=element.encodertyp;
daten.ptr[5]=element.encoder;
daten.ptr[6]=element.encoderalt;
daten.ptr[7]=element.justieren;
daten.ptr[8]=element.pulse;
daten.ptr[9]=element.direction;
daten.ptr[10]=element.ziel;
daten.ptr[11]=element.dif;
daten.ptr[12]=element.again;
daten.ptr[17]=element.var;
daten.ptr[18]=element.try;
element.t_out=( (p[12]<<8) | 0x80000000 );
daten.ptr[19]=element.t_out;
daten.ptr[20]=element.rep;


if ( (element.signal_look=install_signalhandler(sighand,daten))==(-1) ){
   printf("ERROR can't get Signal.\n");
   exit(1);
}
daten.ptr[14]=element.signal_look;
var_ptr[10]  =element.signal_look;

/* --- cyklischen Alarm setzen ----------------------------------------- */
if ( (element.alm_cycle_id =
     alm_cycle(element.signal_look,TIME(look)))==(-1) ) {
   printf("ERROR can't set ALARM\n");
   *outptr++=-1;
   exit();
}
daten.ptr[16]=element.alm_cycle_id;
var_ptr[7]   =element.alm_cycle_id;

if ( (element.signal_timeout=install_signalhandler(sighand,daten))==(-1) ){
   printf("ERROR can't get Signal.\n");
   exit(1);
}
daten.ptr[13]=element.signal_timeout;
var_ptr[9]   =element.signal_timeout;

/* --- Alarm fuer Timeout setzen --------------------------------------- */
if ( (element.alm_set_id =
      alm_set(element.signal_timeout,element.t_out))==(-1) ) {
   printf("ERROR can't set ALARM\n");
   *outptr++=-1;
   exit();
}
daten.ptr[15]=element.alm_set_id;
var_ptr[8]   =element.alm_set_id;

/* --- Motor einmal anfahren fahren, Rest mit Interceptroutine ---- */
if ( p[3]<=4 ) {
   if ( p[2] ) {
      *A_Addr(element.motoradresse,0)=CNTR_steppos;
   }
   else {
      *A_Addr(element.motoradresse,0)=CNTR_stepneg;
   }
   *A_Addr(element.motoradresse,0)=GO_slow;
}
else {
   if ( p[2] ) {
      *A_Addr(element.motoradresse,0)=CNTR_stepposAv;
   }
   else {
      *A_Addr(element.motoradresse,0)=CNTR_stepnegAv;
   }
   *A_Addr(element.motoradresse,0)=GO_quick;
}
free(var_ptr);
return(plOK);
}

plerrcode test_proc_M_E362(p)

int *p;
{
   if ( p[0] !=12 )                 return(plErr_ArgNum);
   if ( !memberlist )               return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != E362 ){
      *outptr++=memberlist[p[1]];
                                    return(plErr_BadModTyp);
   }
   if ( (get_mod_type(memberlist[p[1]])!= E362)||
        ((get_mod_type(memberlist[p[9]])!= E233_0)&&
         (get_mod_type(memberlist[p[9]])!= E233_1)&&
         (get_mod_type(memberlist[p[9]])!= ROC717)) ) {
      *outptr++=memberlist[p[1]];
                                   return(plErr_BadModTyp);
     
   }
   if ( p[10] > MAX_VAR )           return( plErr_IllVar);
   if ( !var_list[p[10]].len )      return(plErr_NoVar);
   if (  var_list[p[10]].len != 11 ) return(plErr_IllVarSize);

   return(plOK);
}

char name_proc_M_E362[]="M_E362";
int ver_proc_M_E362=1;

