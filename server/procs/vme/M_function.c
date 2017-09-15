
/****************************************************************************/
/*                                                                          */
/* Prozeduren fuer die Motorsteuerung                                       */
/*                                                                          */
/* Erstellungsdatum : 24.05. '93                                            */
/*                                                                          */
/* letzte Aenderung : 18.08. '93                                            */
/*                                                                          */
/* Autor : Twardowski                                                       */
/*                                                                          */
/****************************************************************************/


#include <errorcodes.h>
#include <modultypes.h>
#include <sconf.h>
#include <debug.h>
#include <types.h>
#include "../../OBJECTS/VAR/variables.h"
#include "../../LOWLEVEL/VME/vme.h"
#include "./M_function.h"
#include "./list.h"
#include "../../MAIN/signals.h"
extern u_char CNTR_r[8];
extern R1_[],R2_[],R3_[],R4_[],R5_[],R6_[],R7_[];

/* die Variablen fuer die verkettete Liste */
extern listpointer *head,*ptr,*c_n;
extern stdelement element;

/* ein Pointer der sich die Position des CurrentPointers nach */
/* dem locate(sigcode) merkt */
listpointer *point;

/* Pointer, der auf die zu dem Motor gehoerige Variable gesetzt wird. */
int *var_ptr;

/* globale Variablen fuer die Prozedure encoderwert_E362 */
#define MODUL_high 1
#define MODUL_low  0
#define ADC_convert_busy 128

int ADC_control,scale=3;
int range_mask[]={0,0,16,32,48};
int range_scale[]={0,1,2,5,10};

/****************************************************************************/
/*                                                                          */
/* Procedure createliste                                                    */
/*                                                                          */
/****************************************************************************/
void createliste()
{
head=0;
c_n=0;
}

/****************************************************************************/
/*                                                                          */
/* Procedure insertafter(a)                                                 */
/*                                                                          */
/****************************************************************************/
void insertafter(a)
stdelement a;
{

ptr=(listpointer *)malloc(sizeof(listpointer)); 
ptr->element=a;

if ( head!=0 ) {
   ptr->next=c_n->next;
   c_n->next=ptr;
   c_n=ptr;
}
else {
   ptr->next=0;
   head=ptr;
}
c_n=ptr;
}
/****************************************************************************/
/*                                                                          */
/* retrieve                                                                 */
/*                                                                          */
/****************************************************************************/

void retrieve(elm)
stdelement elm;
{
elm=c_n->element;
}

/****************************************************************************/
/*                                                                          */
/* find_next                                                                */
/*                                                                          */
/****************************************************************************/

void find_next( fail )

int fail;
{
fail=0;
if ( c_n->next != 0 ) c_n=c_n->next;
else fail=1;
}

/****************************************************************************/
/*                                                                          */
/* locate                                                                   */
/*                                                                          */
/****************************************************************************/

void locate( sig,fail )

int sig;
int fail;
{
fail=0;
c_n=head;
retrieve(element);
while (!fail&&(element.signal_timeout!=sig)&&(element.signal_look!=sig)) {
   find_next(fail);
   retrieve(element);
}
}

/****************************************************************************/
/*                                                                          */
/* delete                                                                   */
/*                                                                          */
/****************************************************************************/

void delete()
{
if ( c_n!=head ) {
   ptr=head;
   while ( ptr->next != c_n ) {
      ptr=ptr->next;
   }
   ptr->next=c_n->next;
}
else head=head->next;

free(c_n);
c_n=head;

}

/****************************************************************************/
/*                                                                          */
/* Procedure loadr(adr,r,value)                                             */
/*                                                                          */
/*           Adresse  = Basisadresse (input)                                */ 
/*           r        = Numer des Bytes der Felder                          */
/*           value    = Wert, der geladen werden soll                       */
/*                                                                          */
/****************************************************************************/

void loadr(adr,r,value)

int adr;
u_char r;
int value;
{ 
u_char bl=0,bm=0,bh=0;
u_char *byte3;

byte3=(u_char *)malloc(4*sizeof(byte3));

byte3=MAKBYT(3,value);
bl=byte3[1];
bm=byte3[2];
*A_Addr(adr,0)=CNTR_r[r];
*A_Addr(adr,1)=bl;
*A_Addr(adr,2)=bm;

free(byte3);
}


/****************************************************************************/
/*                                                                          */
/* Procedure loadpulse(adr,value)                                           */
/*                                                                          */
/*           Adresse  = Basisadresse (input)                                */ 
/*           value    = Wert, der geladen werden soll                       */
/*                                                                          */
/****************************************************************************/

void loadpulse(adr,value)

int adr;
int value;
{ 
u_char bl=0,bm=0,bh=0;
u_char *byte3;

byte3=(u_char *)malloc(4*sizeof(byte3));

byte3=MAKBYT(3,value);
bl=byte3[1];
bm=byte3[2];
bh=byte3[3];

*A_Addr(adr,0)=CNTR_pulse;
*A_Addr(adr,1)=bl;
*A_Addr(adr,2)=bm;
*A_Addr(adr,3)=bh;

free(byte3);
}

/****************************************************************************/
/*                                                                          */
/* Procedure getpulse(adr)                                                  */
/*                                                                          */
/*           Adresse  = Basisadresse (input)                                */ 
/*                                                                          */
/****************************************************************************/

int getpulse(adr)

int adr;
{ 
u_char bl=0,bm=0,bh=0;

*A_Addr(adr,0)=CNTR_pulse;
bl=*A_Addr(adr,1);
bm=*A_Addr(adr,2);
bh=*A_Addr(adr,3);

return( (bh*265+bm)*265+bl );
}


/****************************************************************************/
/*                                                                          */
/* Procedure encoderwert_ROC(adr)                                           */
/*                                                                          */
/*           Adresse  = Basisadresse (input)                                */ 
/*                                                                          */
/****************************************************************************/

int encoderwert_ROC(adr)

int adr;
{ 
int wert,altwert,rep=0;
u_char bl,bm,bh;

do { } while( *A_Addr(adr,3) != 255 );

bl = *A_Addr(adr,0);
bm = *A_Addr(adr,1);
bh = *A_Addr(adr,2);
   
bh = bh & 1;

altwert = bh*256*256+bm*256+bl;

do {
   do { } while( *A_Addr(adr,3) != 255 );

   bl = *A_Addr(adr,0);
   bm = *A_Addr(adr,1);
   bh = *A_Addr(adr,2);
   
   bh = bh & 1;

   wert = bh*256*256+bm*256+bl;

   if (wert==altwert) {
      rep=10;
   }
   else {
      rep=rep+1;
      altwert=wert;
   }
} while(rep<10);

if (wert==altwert) return(wert);
else return(-1); 

}
/****************************************************************************/
/**** Proceduren fuer encoderwert_E362                                      */
/****************************************************************************/

/****************************************************************************/
/* ADC_get                                                                  */
/****************************************************************************/
float ADC_get(adresse)
int adresse;
{
int ADC_high,ADC_low;
float ADC_value,ADC_raw;

do {
   ADC_high=*A_Addr(adresse,MODUL_high);
}
while ( (ADC_high & ADC_convert_busy) );

ADC_high=ADC_high & 0x1F;
ADC_low=*A_Addr(adresse,MODUL_low);
ADC_raw=ADC_high*256+ADC_low;
ADC_value=(2*range_scale[scale]*(ADC_raw/8191))-range_scale[scale];

return(ADC_value);
}
/****************************************************************************/
/* ADC_start                                                                */
/****************************************************************************/
void ADC_start(adresse,channel)
int adresse;
int channel;
{
int ADC_control;

/* --- make control byte -------------------------------------------------- */
ADC_control=channel+range_mask[scale];  

/* --- start measurement by writeing control word ------------------------- */
*A_Addr(adresse,1)=(u_char)ADC_control;
}
/****************************************************************************/
/*                                                                          */
/* Procedure encoderwert_E233(addr,chan)                                    */
/*                                                                          */
/*           Adresse  = Basisadresse (input)                                */ 
/*                                                                          */
/****************************************************************************/
float encoderwert_E233(addr,chan)
int addr,chan;
{
float value=0;
int i;

for (i=1;i<=10;i++) {
   ADC_start(addr,chan);
   value=value+ADC_get(addr);
}
value=value/10.0;
return( value );
}
/****************************************************************************/
/*                                                                          */
/* Prozeduren fuer die Interceptroutine                                     */
/*                                                                          */
/****************************************************************************/


/****************************************************************************/
/*                                                                          */
/* faehrt den Motor langsam in positiver Richtung                           */
/*                                                                          */
/****************************************************************************/
drive_slow_pos(motoradresse,dif,direction)
int motoradresse,dif,direction;
{

/* --- zuletzt positiv gefahren -------------------------------------- */
direction=1;

*A_Addr(motoradresse,0)=CNTR_reset;
*A_Addr(motoradresse,0)=CNTR_master;
loadr(motoradresse,1,R1_[1]);
loadr(motoradresse,7,R7_[1]);
loadpulse(motoradresse,dif);
*A_Addr(motoradresse,0)=CNTR_steppos; 
*A_Addr(motoradresse,0)=GO_slow;

/*
point->element.rep=point->element.try;
*/
}
/****************************************************************************/
/*                                                                          */
/* faehrt den Motor in positiver Richtung                                   */
/*                                                                          */
/****************************************************************************/
drive_pos(motoradresse,dif,direction)
int motoradresse,dif,direction;
{
direction=1;
*A_Addr(motoradresse,0)=CNTR_reset;
*A_Addr(motoradresse,0)=CNTR_master;
loadr(motoradresse,1,R1_[3]);
loadr(motoradresse,7,R7_[3]);
loadpulse(motoradresse,dif);
*A_Addr(motoradresse,0)=CNTR_steppos; 
*A_Addr(motoradresse,0)=GO_slow;
}
/****************************************************************************/
/*                                                                          */
/* faehrt den Motor langsam in negativer Richtung                           */
/*                                                                          */
/****************************************************************************/
drive_slow_neg(motoradresse,dif,direction)
int motoradresse,dif,direction;
{

/* --- zuletzt negativ gefahren -------------------------------------- */
direction=0;

*A_Addr(motoradresse,0)=CNTR_reset;
*A_Addr(motoradresse,0)=CNTR_master;
loadr(motoradresse,1,R1_[1]);
loadr(motoradresse,7,R7_[1]);
loadpulse(motoradresse,-(dif));
*A_Addr(motoradresse,0)=CNTR_stepneg; 
*A_Addr(motoradresse,0)=GO_slow;

/*
point->element.rep=point->element.try;
*/
}
/****************************************************************************/
/*                                                                          */
/* faehrt den Motor in negativer Richtung                                   */
/*                                                                          */
/****************************************************************************/
drive_neg(motoradresse,dif,direction)
int motoradresse,dif,direction;
{

direction=0;
*A_Addr(motoradresse,0)=CNTR_reset;
*A_Addr(motoradresse,0)=CNTR_master;
loadr(motoradresse,1,R1_[3]);
loadr(motoradresse,7,R7_[3]);
loadpulse(motoradresse,-(dif));
*A_Addr(motoradresse,0)=CNTR_stepneg; 
*A_Addr(motoradresse,0)=GO_slow;
}
/****************************************************************************/
/*                                                                          */
/* wird vom Signalhandler angesprungen und                                  */
/* uebernimmt das Nachjustieren des Motors.                                 */
/*                                                                          */
/****************************************************************************/

int sighand(sigcode,arg)

int sigcode;
ClientData arg;
{
stdelement element;

element.motoradresse=arg.ptr[1];
element.motortyp=arg.ptr[2];
element.encoderadresse=arg.ptr[3];
element.encodertyp=arg.ptr[4];
element.encoder=arg.ptr[5];
element.encoderalt=arg.ptr[6];
element.justieren=arg.ptr[7];
element.pulse=arg.ptr[8];
element.direction=arg.ptr[9];
element.ziel=arg.ptr[10];
element.dif=arg.ptr[11];
element.again=arg.ptr[12];
element.signal_timeout=arg.ptr[13];
element.signal_look=arg.ptr[14];
element.alm_set_id=arg.ptr[15];
element.alm_cycle_id=arg.ptr[16];
element.var=arg.ptr[17];
element.try=arg.ptr[18];
element.t_out=arg.ptr[19];
element.rep=arg.ptr[20];


printf("****** Signal %d erhalten *******\n",sigcode);

/* --- setzte Pointer auf die zu dem Motor gehoerige Variable ------------- */
var_ptr=(int *)var_list[element.var].var.ptr;

/* --- Signal: TimeoutSignal der Motor wird gestoppt ---------------------- */
if ( sigcode==element.signal_timeout ) {

/* --- der Motor wird gestoppt vorher Pulsregister auslesen --------------- */
   var_ptr[1]=getpulse(element.motoradresse);
   *A_Addr(element.motoradresse,0)=CNTR_reset;

/* --- die Alarme fuer diesen Motor werden geloescht ---------------------- */
   alm_delete(element.alm_cycle_id);
   alm_delete(element.alm_set_id);

/* --- Das Timeout- und LookSignal wird wieder freigegeben. --------------- */
   remove_signalhandler(element.signal_timeout);
   remove_signalhandler(element.signal_look);

}

/* --- Signal: siehe nach, ob der Motor noch laeuft ----------------------- */
if ( sigcode==element.signal_look ) {

/* --- Pulsregister wird ausgelesen und ggf. nachjustiert ----------------- */
   if ( !(var_ptr[1]=getpulse(element.motoradresse)) ) {

/* --- wenn ohne nachjustieren, dann hier abbrechen ----------------------- */
      if ( !element.justieren || (element.encodertyp==E233_0) ||
           (element.encodertyp==E233_1) ) {

/* --- die Alarme fuer diesen Motor werden geloescht ---------------------- */
         alm_delete(element.alm_cycle_id);
         alm_delete(element.alm_set_id);

/* --- Das Timeout- und LookSignal wird wieder freigegeben. --------------- */
         remove_signalhandler(element.signal_timeout);
         remove_signalhandler(element.signal_look);

     }

/* --- ansonsten wird nachjustiert ---------------------------------------- */
      else {

/* --- Encoder auslesen und Richtung und Geschwindigkeit waehlen ---------- */
         if ( element.encodertyp == ROC717 ) 
            element.encoder = encoderwert_ROC(element.encoderadresse);

/* --- Ist der Motor im EPS-Bereich oder die Anzahl der Justierversuche --- */
/* --- erreicht, wird ggf. nochmals von der richtigen Seite angefahren ---- */
         if ( (element.rep>=element.try)
            || (abs(element.dif)<=EPS) ) {

/* --- Ist der Motor schon einmal im EPS-Bereich gewesen und somit schon -- */
/* --- von der positiven Seite positioniert, kann hier abgebrochen werden - */
            switch( arg.ptr[12] ) {
               case 2 : {
               
/* --- die Alarme fuer diesen Motor werden geloescht ---------------------- */
                  alm_delete(element.alm_cycle_id);
                  alm_delete(element.alm_set_id);

/* --- Das Timeout- und LookSignal wird wieder freigegeben. --------------- */
                  remove_signalhandler(element.signal_timeout);
                  remove_signalhandler(element.signal_look);
                  break;
               }
/* --- sonst wird der Motor von der positiven Seite angefahren ----------- */
               case 0 : {
                  if ( !element.direction ) {

/* --- fahre 10 Schritte zurueck ----------------------------------------- */
                     element.again=1;
                     drive_slow_neg(element.motoradresse,-10,element.direction);
                     element.rep=element.try;
                  }
                  else element.again=2;
               break;
               }
               case 1 : {

/* --- fahre 10 Schritte vorwaerts ---------------------------------------- */
                  element.again=2;
                  drive_slow_pos(element.motoradresse,10,element.direction);
                  break;
               }
            }
         }
         else {
            if ( element.dif>EPS ) {
               if ( element.dif<=10 ) {
                  drive_slow_pos(element.motoradresse,element.dif,
                                 element.direction);
               }
               else {
                  drive_pos(element.motoradresse,element.dif,
                            element.direction);
               }

            }
            else {

               if ( element.dif < (-EPS) ) {
                  if ( element.dif > (-10) ) {
                     drive_slow_neg(element.motoradresse,element.dif,
                                    element.direction);
                  }
                  else {
                     drive_neg(element.motoradresse,element.dif,
                               element.direction);
                  }

               }
               else element.rep=element.try;
            }/* else */
         }/* else */
         element.rep=element.rep +1;
      } 
   } /*if */
}/* if look */

arg.ptr[5]=element.encoder;
arg.ptr[6]=element.encoderalt;
arg.ptr[8]=element.pulse;   
arg.ptr[9]=element.direction;
arg.ptr[10]=element.ziel;
arg.ptr[11]=element.dif;
arg.ptr[12]=element.again;
arg.ptr[18]=element.try;
arg.ptr[20]=element.rep;

} /*sighand*/
