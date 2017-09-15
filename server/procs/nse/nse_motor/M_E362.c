
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
#include <errno.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <sconf.h>
#include <debug.h>
#include <types.h>
#include "../../../MAIN/signals.h"
#include "../../../LOWLEVEL/VME/vme.h"
#include "../../../OBJECTS/VAR/variables.h"
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
/* Procedure E_SSI_Init(Adresse)                                         */
/*                                                                          */
/*           Adresse  = Basisadresse (input)                                */
/* Confirmation:                                                            */
/*                                                                          */
/****************************************************************************/

plerrcode proc_E_SSI_Init(p)

int *p;
{
int    welcher,adresse;
int *varptr;
int r_code=0;

welcher=p[1];
adresse=memberlist[welcher];

/* --- Modulnummer ----------------------------------------- */
*outptr++=p[1];
D(D_REQ,printf("Init_SSI Modul %d Adresse %x\n",welcher,adresse);)

varptr=(int *)var_list[p[1]].var.ptr;
if ( r_code=write_SSI_parameter(adresse,varptr) )
{  D(D_REQ,printf("Init_SSI Modul ERROR %d\n",r_code);)
   *outptr++ =WRITE_SSI_PARM+r_code;
   return(plErr_Other);
}

return(plOK);
}

plerrcode test_proc_E_SSI_Init(p)

int *p;
{
   if ( p[0] !=1 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != ESSI) {
      *outptr++=memberlist[p[1]];
                                   return(plErr_BadModTyp);
   }
   if ( p[1] > MAX_VAR )           return(plErr_IllVar);
   if ( !var_list[p[1]].len )      return(plErr_NoVar);
   if (  var_list[p[1]].len != 9 ) return(plErr_IllVarSize); 
   return(plOK);
}
char name_proc_E_SSI_Init[]="E_SSI_Init";
int ver_proc_E_SSI_Init=1;
/****************************************************************************/
/* Procedure E_SSI_Read(Adresse)                                            */
/*                                                                          */
/*           Adresse  = Basisadresse (input)                                */
/* Confirmation:                                                            */
/*                                                                          */
/****************************************************************************/

plerrcode proc_E_SSI_Read(p)

int *p;
{
int    welcher,adresse,channel;
int val_high,val_low;
int r_code=0;
int *var_ptr,hlp;

welcher=p[1];
adresse=memberlist[welcher];

/* --- Modulnummer ----------------------------------------- */
*outptr++=p[1];

/* --- Channelnummer --------------------------------------- */
channel= ( (adresse >> 28) & 255 ); 

D(D_REQ,printf("E_SSI Modul %d Adresse %x\n",welcher,adresse);)

var_ptr=(int *)var_list[p[1]].var.ptr;
if ( r_code=encoderwert_SSI(adresse,channel,&val_high,&val_low,var_ptr[8]) )
{  D(D_REQ,printf("E_SSI_Read Modul ERROR %d\n",r_code);)
   *outptr++ =READ_SSI_VAL+r_code;
   return(plErr_Other);
}

*outptr++ = val_high;
*outptr++ = val_low;

return(plOK);
}

plerrcode test_proc_E_SSI_Read(p)

int *p;
{
   if ( p[0] !=1 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != ESSI) {
      *outptr++=memberlist[p[1]];
                                   return(plErr_BadModTyp);
   }
   if ( p[1] > MAX_VAR )           return(plErr_IllVar);
   if ( !var_list[p[1]].len )      return(plErr_NoVar);
   if (  var_list[p[1]].len != 9 ) return(plErr_IllVarSize); 
return(plOK);
}

char name_proc_E_SSI_Read[]="E_SSI_Read";
int ver_proc_E_SSI_Read=1;

/****************************************************************************/
/* Procedure E_SSI_Status(Adresse)                                         */
/*                                                                          */
/*           Adresse  = Basisadresse (input)                                */
/* Confirmation:                                                            */
/*                                                                          */
/****************************************************************************/

plerrcode proc_E_SSI_Status(p)

int *p;
{
int    welcher,adresse,channel;
int *varptr, val_low,val_high;
int r_code=0;

welcher=p[1];
adresse=memberlist[welcher];
channel= ( (adresse >> 28) & 255 ); 

/* --- Modulnummer ----------------------------------------- */
*outptr++ =p[1];
D(D_REQ,printf("Status_SSI Modul %d Adresse %x\n",welcher,adresse);)

varptr=(int *)var_list[p[1]].var.ptr;

/* --- Genauigkeitstoleranz -------------------------------- */
*outptr++ = varptr[0];

if ( r_code=read_SSI_parameter(adresse) )
{  D(D_REQ,printf("Status_SSI Modul ERROR %d\n",r_code);)
   *outptr++ =READ_SSI_PARM+r_code;
   return(plErr_Other);
}
/* --- Umrechnungsfaktor, Umrechnungsfaktor und shift ------ */

*outptr++ = varptr[6];
*outptr++ = varptr[7];
*outptr++ = varptr[8];

return(plOK);
}

plerrcode test_proc_E_SSI_Status(p)

int *p;
{
   if ( p[0] !=1 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != ESSI) {
      *outptr++=memberlist[p[1]];
                                   return(plErr_BadModTyp);
   }
   if ( p[1] > MAX_VAR )           return(plErr_IllVar);
   if ( !var_list[p[1]].len )      return(plErr_NoVar);
   if (  var_list[p[1]].len != 9 ) return(plErr_IllVarSize); 
   return(plOK);
}
char name_proc_E_SSI_Status[]="E_SSI_Status";
int ver_proc_E_SSI_Status=1;
/****************************************************************************/
/* Procedure E_ROC_Read(Adresse)                                            */
/*                                                                          */
/*           Adresse  = Basisadresse (input)                                */
/* Confirmation:                                                            */
/*                                                                          */
/****************************************************************************/

plerrcode proc_E_ROC_Read(p)

int *p;
{
int    welcher,adresse;
int value;
int r_code=0;
int *varptr;

welcher=p[1];
adresse=memberlist[welcher];
varptr=(int *)var_list[p[1]].var.ptr;

/* --- Modulnummer ----------------------------------------- */
*outptr++=p[1];

D(D_REQ,printf("E_ROC Modul %d Adresse %x\n",welcher,adresse);)

if ( r_code=encoderwert_ROC(adresse,&value,varptr[2]) )
{  D(D_REQ,printf("E_ROC_Read Modul ERROR %d\n",r_code);)
   *outptr++ =READ_SSI_VAL+r_code;
   return(plErr_Other);
}
*outptr++ = value;

return(plOK);
}

plerrcode test_proc_E_ROC_Read(p)

int *p;
{
   if ( p[0] !=1 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != ROC717) {
      *outptr++=memberlist[p[1]];
                                   return(plErr_BadModTyp);
   }
   if ( p[1] > MAX_VAR )           return(plErr_IllVar);
   if ( !var_list[p[1]].len )      return(plErr_NoVar);
   if (  var_list[p[1]].len != 3 ) return(plErr_IllVarSize); 
return(plOK);
}

char name_proc_E_ROC_Read[]="E_ROC_Read";
int ver_proc_E_ROC_Read=1;



/****************************************************************************/
/*                                                                          */
/* Procedure M_E362_stop(adresse)                                           */
/*                                                                          */
/*           Adresse  = Basisadresse (input)                                */
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

/* --- Modulnummer ----------------------------------------- */
*outptr++=p[1];
D(D_REQ,printf("M_E362_stop Modul %d Adresse 0x%x\n",welcher,adresse);)

var_ptr=(int *)var_list[p[1]].var.ptr;

/* ___ Motormechanik stoppen ----------------------------------- */
if (var_ptr[5]) *A_Addr(adresse,0)=CNTR_reset;
else *A_Addr(adresse,0)=CNTR_stop;

/* --- zyklischen Alarm und Timeout loeschen ------------------- */
alm_delete(var_ptr[11]);
alm_delete(var_ptr[12]);

/* --- Datenbereich und Signale im Signalhandler freigeben ----- */
remove_signalhandler(var_ptr[13]);
remove_signalhandler(var_ptr[14]);
var_ptr[15]=0;

/* --- Software fertig ----------------------------------------- */
var_ptr[16]= (var_ptr[16] & 0xFFFFFFFE);
return(plOK);
}

plerrcode test_proc_M_E362_stop(p)

int *p;
{
   if ( p[0] !=1 )                  return(plErr_ArgNum);
   if ( !memberlist )               return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]])!= E362) {
      *outptr++=memberlist[p[1]];
                                    return(plErr_BadModTyp);
   }
   if ( p[1] > MAX_VAR )            return(plErr_IllVar);
   if ( !var_list[p[1]].len )       return(plErr_NoVar);
   if (  var_list[p[1]].len != 17 ) return(plErr_IllVarSize); 
   
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
/* Confirmation: Statusregister, Pulsregister, Encoderwert (in 1/10 mm)     */
/*                                                                          */
/****************************************************************************/

plerrcode proc_M_E362_status(p)

int *p;
{
int m_adresse,e_adresse,channel;
int value,help;
float val;
int *ssi_high,*ssi_low;
int r_code=0;
int *var_ptr_enc,*var_ptr_mot;

ssi_high=(int *)malloc(sizeof(int));
ssi_low =(int *)malloc(sizeof(int));
var_ptr_enc=(int *)var_list[p[2]].var.ptr;
var_ptr_mot=(int *)var_list[p[1]].var.ptr;

/* --- Modulnummer ----------------------------------------- */
D(D_REQ,printf("M_E362_status Modul %d\n",p[1]);)

m_adresse=memberlist[p[1]];
e_adresse=memberlist[p[2]];
D(D_REQ,printf("M_E362_status Modul %d und Modul %d Motoradresse %x Encoderadresse %x\n",
                                  p[1],p[2],m_adresse,e_adresse);)
channel = ( (e_adresse >> 28)&0xFF );
*outptr++=p[1];
*outptr++=p[2];

switch ( get_mod_type(memberlist[p[2]]) ) {
   case ESSI   : if ( (r_code = 
                    encoderwert_SSI(e_adresse,channel,ssi_high,ssi_low,var_ptr_enc[8]))==(-1) )
                 {  *outptr++ =READ_SSI_VAL+r_code;
                    return(plErr_Other);
                 }
                 *outptr++ = *ssi_low;
                 break;   
   case E233   : val = encoderwert_E233(e_adresse,channel);
                 help = ( (VOLT_max-val)*5.3625377+CM_min )*100;
D(D_REQ,printf("STATUS SCHUBSTANGE: mm=%f Volt=%f   \n",help,val);)

/* --- Rueckgabewert in Milimeter --------------------------------------- */
                 *outptr++ = (int) ( (floor(help)+ceil(help))/2.0 );
                 break;
}

*outptr++ = *A_Addr(m_adresse,0);
*outptr++ = getpulse(m_adresse);

/* --- Softwarestatus --------------------------------------------------- */
*outptr++ =var_ptr_mot[16];
return(plOK);
}

plerrcode test_proc_M_E362_status(p)

int *p;
{
   if ( p[0] !=2 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( (get_mod_type(memberlist[p[1]])!= E362)||
        ((get_mod_type(memberlist[p[2]])!= E233)&&
        (get_mod_type(memberlist[p[2]])!= ESSI)) ) {
      *outptr++=memberlist[p[1]];
                                   return(plErr_BadModTyp);
   }  
   return(plOK);
}
char name_proc_M_E362_status[]="M_E362_status";
int ver_proc_M_E362_status=1;

/****************************************************************************/
/*                                                                          */
/* Procedure M_E362(MotorAdresse,EncoderAdresse)                            */
/*                                                                          */
/*           MotorAdresse   = Basisadresse der Motorkarte                   */
/*           EncoderAdresse = Basisadresse der Encoderkarte                 */
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
/*  !!! SSI gibt bis zu 7 Byte Encoderwert zurueck. Wie soll dieser mit dem */
/*  Sollwert verglichen werden????????                                      */
/****************************************************************************/


plerrcode proc_M_E362(p)

int *p;
{
ClientData daten;
int *var_ptr_enc,*var_ptr_mot,channel;
int look=1;  
float hilf;
int pulse,m_pulse,e_pulse;
int t_out,alm_cycle_id,alm_set_id,signal_look,signal_timeout;
int *high,*low,ziel_high,ziel_low;
int r_code,status;
int jojo;

high=(int *)malloc(sizeof(int));
low =(int *)malloc(sizeof(int));

/* --- Modulnummer Motor, Encoder -------------------------------------- */
*outptr++=p[1];
*outptr++=p[2];

/* --- Pointer auf die Variablen setzten ------------------------------- */
var_ptr_mot=(int *)var_list[p[1]].var.ptr;
var_ptr_enc=(int *)var_list[p[2]].var.ptr;

var_ptr_mot[15]=0;

/* --- reset Softwarestatus -------------------------------------------- */
var_ptr_mot[16]=0;

/* --- Justageversuche vermerken --------------------------------------- */
var_ptr_mot[2]=var_ptr_mot[1];

/* --- Statusregister der Motorkarte ----------------------------------- */
status = *A_Addr(memberlist[p[1]],0);

/* --- ggf. Channel der Karte waehlen ---------------------------------- */
channel= ( (memberlist[p[2]] >> 28) & 255 ); 

D(D_REQ,printf("M_E362 Module %d %d Motoradresse %x Encoderadresse %x\n",
                        p[1],p[2],memberlist[p[1]],memberlist[p[2]]);)

/* --- Reset- und MasterMode ------------------------------------------- */
*A_Addr(memberlist[p[1]],0)=CNTR_reset;
*A_Addr(memberlist[p[1]],0)=CNTR_master;

/* --- Encoder auslesen (fuer den Encoder E233 nicht noetig, da in   --- */
/* --- dem Fall nicht justiert wird) ----------------------------------- */
switch ( get_mod_type(memberlist[p[2]]) ) {
   case ESSI   : if ( (r_code=
                 encoderwert_SSI(memberlist[p[2]],channel,high,low,var_ptr_enc[8]))==(-1))
                 {  D(D_REQ,printf("E_SSI_Read Modul ERROR %d\n",r_code);)
                    *outptr++ =READ_SSI_VAL+r_code;
                    return(plErr_Other);
                 }
                 break;
} 

/* --- Register 1,2,4,5,6,7 mit der gewaehlten Geschwindigkeit laden --- */
loadr(memberlist[p[1]],1,R1_[var_ptr_mot[10]]);
loadr(memberlist[p[1]],2,R2_[var_ptr_mot[10]]);
loadr(memberlist[p[1]],4,R4_[var_ptr_mot[10]]);
loadr(memberlist[p[1]],5,R5_[var_ptr_mot[10]]);
loadr(memberlist[p[1]],6,R6_[var_ptr_mot[10]]);
loadr(memberlist[p[1]],7,R7_[var_ptr_mot[10]]);

/* --- Winkel, Zentimeter, Pulse, Referenzpunkt, Limits ---------------- */
switch ( var_ptr_mot[5] ) {
   case 0 : pulse=(60*60*var_ptr_mot[6]+60*var_ptr_mot[7]+var_ptr_mot[8]);
            break;
   case 1 : pulse=CM*var_ptr_mot[6]+MM*var_ptr_mot[7];
            break;
   default : pulse=var_ptr_mot[6];
             break;
}

/* --- zu fahrende Pulse mittels Umrechnungsfaktor bestimmen ----------- */
if (var_ptr_mot[5]<=2)
   m_pulse=(int)( ((float)pulse/1296000.0)*(float)var_ptr_mot[9] ); 
else m_pulse=pulse;

/* --- Ziel feststellen (nicht fuer den Encoder E233) ------------------ */
if ( get_mod_type(memberlist[p[2]])==ESSI ) {
     e_pulse=(int)(( (float)m_pulse*((float)var_ptr_enc[6]/(float)var_ptr_mot[9])) ); 
     if ( var_ptr_mot[0] )
     {  if ( (*low-e_pulse)<0 ) ziel_low=var_ptr_enc[6]+(*low-e_pulse)+1;
        else ziel_low=*low-e_pulse;
     }
     else 
     { if ( (*low+e_pulse)>var_ptr_enc[6] ) ziel_low=*low+e_pulse-var_ptr_enc[6];
       else ziel_low=*low+e_pulse;
     }
}

daten.ptr=(int *)malloc(7*sizeof(int));
daten.ptr[1]=p[1];            /* Modulnummer Motor   */
daten.ptr[2]=p[2];            /* Modulnummer Encoder */
daten.ptr[3]=*high;           /* HighBit EncoderIst  */
daten.ptr[4]=*low;            /* LowBit EncoderIst   */
daten.ptr[5]=ziel_high;       /* HighBit EncoderSoll */
daten.ptr[6]=ziel_low;        /* LowBit EncoderSoll  */

/*************************************************************************/
/* --- Signal anfordern ------------------------------------------------ */
if ( (signal_look=install_signalhandler(sighand,daten))==(-1) ){
   D(D_REQ,printf("ERROR can't get Signal.\n");)
   *outptr++ =GETSIGNAL+CANTGETSIGNAL;
   *outptr++ =errno;  
   return(plErr_Other);
}
var_ptr_mot[13]=signal_look;

/* --- cyklischen Alarm setzen ----------------------------------------- */
if ( (alm_cycle_id =alm_cycle(signal_look,TIME(look)))==(-1) ) {
   D(D_REQ,printf("ERROR can't set ALARM \n");)
   *outptr++ =SETALARM+CANTSETALARM;
   *outptr++ =errno;  
   return(plErr_Other);
}
var_ptr_mot[11]=alm_cycle_id;

/* --- Signal anfordern ------------------------------------------------ */
if ( (signal_timeout=install_signalhandler(sighand,daten))==(-1) ){
   D(D_REQ,printf("ERROR can't get Signal.\n");)
   *outptr++ =GETSIGNAL+CANTGETSIGNAL;
   *outptr++ =errno;      
   return(plErr_Other);
}
var_ptr_mot[14]=signal_timeout;

/* --- Alarm fuer Timeout setzen --------------------------------------- */
t_out=( (var_ptr_mot[3]<<8) | 0x80000000 );
D(D_REQ,printf("TIMEOUT in 0x%x Sekunden\n",var_ptr_mot[3]);)
if ( (alm_set_id =alm_set(signal_timeout,t_out))==(-1) ) {
   D(D_REQ,printf("ERROR can't set ALARM \n");)
   *outptr++ =SETALARM+CANTSETALARM;
   *outptr++ =errno;      
   return(plErr_Other);
}
var_ptr_mot[12]=alm_set_id;
/*************************************************************************/

/* --- die Anzahl der Pulse laden -------------------------------------- */
if ( var_ptr_mot[5]<=2 ) loadpulse(memberlist[p[1]],m_pulse);

/* --- Software faehrt -------------------------------------------- */
var_ptr_mot[16]= (var_ptr_mot[16] | 1);

/* --- Motor einmal anfahren fahren, Rest mit Interceptroutine ---- */
if ( var_ptr_mot[10]<=4 ) {
   if ( var_ptr_mot[0] ) {
      printf("CNTR_steppos\n");
      *A_Addr(memberlist[p[1]],0)=CNTR_steppos;
   }
   else {
      *A_Addr(memberlist[p[1]],0)=CNTR_stepneg;
      printf("CNTR_stepneg\n");
   }
   *A_Addr(memberlist[p[1]],0)=GO_slow;
}
else {
   if ( var_ptr_mot[0] ) {
      *A_Addr(memberlist[p[1]],0)=CNTR_stepposAv;
      printf("CNTR_stepposAv\n");
   }
   else {
      *A_Addr(memberlist[p[1]],0)=CNTR_stepnegAv;
      printf("CNTR_stepnegAv\n");
   }
   *A_Addr(memberlist[p[1]],0)=GO_quick;
}

/* --- fuer Testzwecke ------------------------------------ */
/*
jojo=0x1000;
sleep(5);printf("jojo 0x%x Enc 0x%x \n",jojo ,*low);
while ( (abs(jojo) > 0x2) )
{
   encoderwert_SSI(memberlist[p[2]],channel,high,&jojo,var_ptr_enc[8]);
}
*A_Addr(memberlist[p[1]],0)=CNTR_reset;
*A_Addr(memberlist[p[1]],0)=CNTR_stop;
*/
/* --- fuer Testzwecke ------------------------------------ */

return(plOK);
}

plerrcode test_proc_M_E362(p)

int *p;
{
   if ( p[0] !=2 )                     return(plErr_ArgNum);
   if ( !memberlist )                  return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != E362 ){
      *outptr++=memberlist[p[1]];
                                       return(plErr_BadModTyp);
   }

   if ( (get_mod_type(memberlist[p[1]])!= E362) ) 
   {  *outptr++=memberlist[p[1]];
                                       return(plErr_BadModTyp);
   }
   if ( p[1] > MAX_VAR )               return( plErr_IllVar);
   if ( !var_list[p[1]].len )          return(plErr_NoVar);
   if (  var_list[p[1]].len != 17 )    return(plErr_IllVarSize);

   if ( get_mod_type(memberlist[p[2]])==E233 ) 
   {  if ( p[2] > MAX_VAR )            return( plErr_IllVar);
      if ( !var_list[p[2]].len )       return(plErr_NoVar);
      if (  var_list[p[2]].len != 3 )  return(plErr_IllVarSize);
   }
   else {
      if ( get_mod_type(memberlist[p[2]])==ESSI )
      {  if ( p[1] > MAX_VAR )            return( plErr_IllVar);
         if ( !var_list[p[2]].len )       return(plErr_NoVar);
         if (  var_list[p[2]].len != 9 )  return(plErr_IllVarSize);
      }
      else {  *outptr++=memberlist[p[2]];
                                       return(plErr_BadModTyp);
           } 

   }    
   return(plOK);
}

char name_proc_M_E362[]="M_E362";
int ver_proc_M_E362=1;

