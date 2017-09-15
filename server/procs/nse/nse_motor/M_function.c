
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
#include "../../../OBJECTS/VAR/variables.h"
#include "../../../LOWLEVEL/VME/vme.h"
#include "./M_function.h"
#include "./list.h"
#include "../../../MAIN/signals.h"
extern u_char CNTR_r[8];
extern R1_[],R2_[],R3_[],R4_[],R5_[],R6_[],R7_[];

/* die Variablen fuer die verkettete Liste */
extern listpointer *head,*ptr,*c_n;
extern stdelement element;

extern ems_u32* outptr;
extern int* memberlist;

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
return( (bh*256+bm)*256+bl );
}


/****************************************************************************/
/*                                                                          */
/* Procedure encoderwert_ROC(adr)                                           */
/*                                                                          */
/*           Adresse  = Basisadresse (input)                                */ 
/*                                                                          */
/****************************************************************************/

int encoderwert_ROC(adr,wert,s)

int adr,*wert,s;
{ 
int altwert,rep=0;
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

   *wert = bh*256*256+bm*256+bl;

   if (*wert==altwert) {
      rep=10;
   }
   else {
      rep=rep+1;
      altwert=*wert;
   }
} while(rep<10);

if (*wert==altwert)
{  if ( s==2 ) *wert=(*wert>>2);
   return(0);
}
else return(NOSEMA); 

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
/* Prozeduren fuer SSI_Encoder                                              */
/*                                                                          */
/****************************************************************************/

/****************************************************************************/
/*                                                                          */
/* new_parm stoesst die Parameteruebergabe an.                              */
/*                                                                          */
/****************************************************************************/
void new_parm(addr)
int addr;
{
*A_Addr(addr,PARM_NEW_)=1;
}
/****************************************************************************/
/*                                                                          */
/* Procedure write_SSI_parameter(adresse,var)                               */
/*                                                                          */
/*           adresse = Basisadresse des Encoders                            */
/*           var = Pointer auf Variable mit den erforderlichen Parametern   */ 
/*                                                                          */
/****************************************************************************/
int write_SSI_parameter(addr,var)
int addr,*var;
{
u_char t_fkt[2],p_fkt[2],m_time,b_count,code,subaddr;
int i,k,erg,dummy;
int b_hlp=19; /* Offset fuer erste Bitzahladresse */
int c_hlp=20; /* Offset fuer erste Codierungsadresse */

/* --- subaddr ---------------- */
subaddr= ( (addr >> 28) & 0x255);

/* --- get Bitcount ----------- */
b_count= ( var[1] & 255 );
/* !!!! hier noch falsche Bitzahlen abfangen (8-64) */

/* --- get Code, Gray-Code 0, else Binaer -------- */
code=var[2];

/* --- get Teilfaktor --------- */ 
t_fkt[0]= ( var[3] & 255 );
t_fkt[1]= ( (var[3] >> 8)&255 );

/* --- get Pausenzeitfaktor --- */
p_fkt[0]= ( var[4] & 255 );
p_fkt[1]= ( (var[4]>>8) & 255 );

/* --- get Messzeit ----------- */
m_time= ( var[5] & 255 );

/* --- write Parameter Teilfaktor --- */
k=0;
while ( !erg && ((k+=1)<MAX_GET_SEMA) )
{
erg= ( *A_Addr(addr,S_GET_)&1 );
if (erg) break;
else dummy= *A_Addr(addr,1);
} 
if (k>=MAX_GET_SEMA) return(NOSEMA);
*A_Addr(addr,T_LOW_)=t_fkt[0];
*A_Addr(addr,T_HIGH_)=t_fkt[1];
new_parm(addr);
dummy= *A_Addr(addr,1);

D(D_REQ,printf("WRITEPARM: Adresse %x Subadr. %x Bitcount %x Code %x 
           Teilfaktor   %x %x
           Pausenfaktor %x %x 
           Messzeit     %x \n",
        addr,subaddr,b_count,code,t_fkt[0],t_fkt[1],p_fkt[0],p_fkt[1],m_time);)

/* --- write Parameter Pausenzeitfaktor --- */
k=0;
while ( !erg && ((k+=1)<MAX_GET_SEMA) )
{
erg= ( *A_Addr(addr,S_GET_)&1 );
if (erg) break;
else dummy= *A_Addr(addr,1);
} 
if (k>=MAX_GET_SEMA) return(NOSEMA); 
*A_Addr(addr,P_LOW_)=p_fkt[0];
*A_Addr(addr,P_HIGH_)=p_fkt[1];
new_parm(addr);
dummy= *A_Addr(addr,1);

/* --- write Parameter Anzahl Messungen --- */
k=0;
while ( !erg && ((k+=1)<MAX_GET_SEMA) )
{
erg= ( *A_Addr(addr,S_GET_)&1 );
if (erg) break;
else dummy= *A_Addr(addr,1);
} 
if (k>=MAX_GET_SEMA) return(NOSEMA);
*A_Addr(addr,M_NB_)=m_time;
new_parm(addr);
dummy= *A_Addr(addr,1);

/* --- write Parameter Bitzahl und Codierung (binaer) --- */
k=0;
while ( !erg && ((k+=1)<MAX_GET_SEMA) )
{
erg= ( *A_Addr(addr,S_GET_)&1 );
if (erg) break;
else dummy= *A_Addr(addr,1);
} 
if (k>=MAX_GET_SEMA) return(NOSEMA);
b_hlp=b_hlp+10*(subaddr);
*A_Addr(addr,b_hlp)=b_count;
new_parm(addr);
dummy= *A_Addr(addr,1);

k=0;
while ( !erg && ((k+=1)<MAX_GET_SEMA) )
{
erg= ( *A_Addr(addr,S_GET_)&1 );
if (erg) break;
else dummy= *A_Addr(addr,1);
} 
if (k>=MAX_GET_SEMA) return(NOSEMA);
*A_Addr(addr,c_hlp)=code; /* binaere Codierung */
c_hlp=c_hlp+10*(subaddr);
new_parm(addr);
dummy= *A_Addr(addr,1);

return(plOK);
}
/****************************************************************************/
/*                                                                          */
/* Procedure encoderwert_SSI(adresse,channel,high,low,s)                    */
/*                                                                          */
/*           adresse = Basisadresse des Encoders                            */
/*           channel = Channelnummer im Encoder                             */ 
/*           high    = High Byte                                            */
/*           low     = Low Byte                                             */
/*                                                                          */
/****************************************************************************/
int encoderwert_SSI(addr,channel,high,low,s)
int addr,channel,*high,*low,s;
{
u_char b[7];
int c_addr,i,k,hlp;
int erg,dummy;

c_addr= 11+channel*10;
k=0;

while ( !erg && ((k+=1)<MAX_GET_SEMA) )
{
erg= ( *A_Addr(addr,S_GET_)&1 );
if (erg) break;
else dummy=*A_Addr(addr,1);
} 
if (!erg) return(NOSEMA);   

for (i=0;i<8;i++)
{  b[i]=*A_Addr(addr,c_addr);
   c_addr=c_addr+1;
}
dummy= *A_Addr(addr,1);

D(D_REQ,printf("vor Shift Byte 7..0: 
                %x %x %x %x %x %x %x %x\n",
                b[7],b[6],b[5],b[4],b[3],b[2],b[1],b[0]);)

*high=b[7];*low=b[3];
for(i=1;i<4;i++)
{  *high=(*high<<8);*high+=b[7-i];
   *low=(*low<<8);*low+=b[3-i];
}
if ( s==2 )
{  *low=(*low>>2);
   hlp=(*high & 3);
   *low=*low+(hlp<<30);
   *high=(*high>>2);
}
return(0);                  
}
/****************************************************************************/
/*                                                                          */
/* Procedure read_SSI_parameter(adresse)                                    */
/*                                                                          */
/*           adresse = Basisadresse des Encoders                            */
/*                                                                          */
/****************************************************************************/
int read_SSI_parameter(addr)
int addr;
{
u_char t_fkt,p_fkt,m_time,b_count,code,subaddr;
int i,k,dummy,erg;
int b_hlp=19; /* Offset fuer erste Bitzahladresse */
int c_hlp=20; /* Offset fuer erste Codierungsadresse */

/* --- subaddr ---------------- */
subaddr= ( (addr >> 28) & 0x255);

/* --- read Parameter Bitzahl und Codierung  --- */
k=0;
while ( !erg && ((k+=1)<MAX_GET_SEMA) )
{
erg= ( *A_Addr(addr,S_GET_)&1 );
if (erg) break;
else dummy= *A_Addr(addr,1);
} 
if (!erg) return(NOSEMA);   
b_hlp=b_hlp+10*(subaddr);
b_count= *A_Addr(addr,b_hlp);
dummy= *A_Addr(addr,1);
*outptr++ =b_count;

k=0;
while ( !erg && ((k+=1)<MAX_GET_SEMA) )
{
erg= ( *A_Addr(addr,S_GET_)&1 );
if (erg) break;
else dummy= *A_Addr(addr,1);
} 
if (!erg) return(NOSEMA);   
code= *A_Addr(addr,c_hlp); 
c_hlp=c_hlp+10*(subaddr);
dummy= *A_Addr(addr,1);
*outptr++ =code;

/* --- read Parameter Teilfaktor --- */
k=0;
while ( !erg && ((k+=1)<MAX_GET_SEMA) )
{
erg= ( *A_Addr(addr,S_GET_)&1 );
if (erg) break;
else dummy= *A_Addr(addr,1);
} 
if (!erg) return(NOSEMA);   
t_fkt= *A_Addr(addr,T_HIGH_);
t_fkt= (t_fkt<<8)+ *A_Addr(addr,T_LOW_);
dummy= *A_Addr(addr,1);
*outptr++ =t_fkt;

/* --- read Parameter Pausenzeitfaktor --- */

k=0;
while ( !erg && ((k+=1)<MAX_GET_SEMA) )
{
erg= ( *A_Addr(addr,S_GET_)&1 );
if (erg) break;
else dummy= *A_Addr(addr,1);
} 
if (!erg) return(NOSEMA);   
p_fkt= *A_Addr(addr,P_HIGH_);
p_fkt= (p_fkt<<8)+*A_Addr(addr,P_LOW_);
dummy= *A_Addr(addr,1);
*outptr++ =p_fkt;

/* --- read Parameter Anzahl Messungen --- */

k=0;
while ( !erg && ((k+=1)<MAX_GET_SEMA) )
{
erg= ( *A_Addr(addr,S_GET_)&1 );
if (erg) break;
else dummy= *A_Addr(addr,1);
} 
if (!erg) return(NOSEMA);   
m_time= *A_Addr(addr,M_NB_);
new_parm(addr);
dummy= *A_Addr(addr,1);
*outptr++ =m_time;

D(D_REQ,printf("READPARM:  Adresse %x Subadr. %x Bitcount %x Code %x 
           Teilfaktor   %x 
           Pausenfaktor %x 
           Messzeit     %x \n",
        addr,subaddr,b_count,code,t_fkt,p_fkt,m_time);)


return(0);
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
D(D_REQ,printf("FAHRE LANGSAM POSITIV\n");)
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
D(D_REQ,printf("FAHRE NORMAL POSITIV\n");)
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
D(D_REQ,printf("FAHRE LANGSAM NEGATIV\n");)

*A_Addr(motoradresse,0)=CNTR_reset;
*A_Addr(motoradresse,0)=CNTR_master;
loadr(motoradresse,1,R1_[1]);
loadr(motoradresse,7,R7_[1]);
loadpulse(motoradresse,dif);
*A_Addr(motoradresse,0)=CNTR_stepneg; 
*A_Addr(motoradresse,0)=GO_slow;

}
/****************************************************************************/
/*                                                                          */
/* faehrt den Motor in negativer Richtung                                   */
/*                                                                          */
/****************************************************************************/
drive_neg(motoradresse,dif,direction)
int motoradresse,dif,direction;
{
D(D_REQ,printf("FAHRE NORMAL NEGATIV\n");)
direction=0;
*A_Addr(motoradresse,0)=CNTR_reset;
*A_Addr(motoradresse,0)=CNTR_master;
loadr(motoradresse,1,R1_[3]);
loadr(motoradresse,7,R7_[3]);
loadpulse(motoradresse,dif);
*A_Addr(motoradresse,0)=CNTR_stepneg; 
*A_Addr(motoradresse,0)=GO_slow;
}
/****************************************************************************/
/*                                                                          */
/* Stoppt den Motor, lloescht die Alarme und gibt die Signal wieder frei.   */
/*                                                                          */
/****************************************************************************/
stop_motor( addr,var )
int addr,*var;
{
*A_Addr(addr,0)=CNTR_reset;
alm_delete(var[11]);
alm_delete(var[12]);
remove_signalhandler(var[13]);
remove_signalhandler(var[14]);
var[15]=0;
var[16]= (var[16] & 0xFFFFFFFE);
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
static int dif,direction,vorz;
int *ssi_high,*ssi_low,r_code;
int *var_ptr_enc,*var_ptr_mot;
int channel,status,ref,lim_l,lim_r;

var_ptr_mot=(int *)var_list[arg.ptr[1]].var.ptr;
var_ptr_enc=(int *)var_list[arg.ptr[2]].var.ptr;

ssi_high=(int *)malloc(sizeof(int));
ssi_low =(int *)malloc(sizeof(int));
channel = ( (memberlist[arg.ptr[2]] >> 28)&0xFF );

D(D_REQ,printf("****** Signal %d erhalten *******\n",sigcode);)

/* --- Signal: TimeoutSignal der Motor wird gestoppt ---------------------- */
if ( sigcode==var_ptr_mot[14] ) {

D(D_REQ,printf("****** TIMEOUT *******\n");)
/* --- der Motor wird gestoppt -------------------------------------------- */
   stop_motor(memberlist[arg.ptr[1]],var_ptr_mot);
   var_ptr_mot[16]= (var_ptr_mot[16] | 32);
}
/****************************************************************************/

/* --- Signal: siehe nach, ob der Motor noch laeuft ----------------------- */
if ( sigcode==var_ptr_mot[13] ) {

/* --- Pulsregister wird ausgelesen und ggf. nachjustiert ----------------- */
   if ( !(getpulse(memberlist[arg.ptr[1]])) ) {

/****************************************************************************/
/* --- Referenzpunkt oder Limits ------------------------------------------ */
   if ( var_ptr_mot[5]>2 ) 
   {  status= *A_Addr(memberlist[arg.ptr[2]],0);
      ref  = (status>>2)&1;
      lim_l= (status)&1;
      lim_r= (status>>1)&1;
      switch ( var_ptr_mot[5] ) {
         case 3 : if ( ref ) { 
                     stop_motor(memberlist[arg.ptr[1]],var_ptr_mot);
                     var_ptr_mot[16]= (var_ptr_mot[16] | 16);
                  }
                  else {
                    if ( var_ptr_mot[15] ) stop_motor(memberlist[arg.ptr[1]],var_ptr_mot);
                    else {
                     if ( var_ptr_mot[0] ) drive_neg(memberlist[arg.ptr[1]],dif,direction);
                     else                  drive_pos(memberlist[arg.ptr[1]],dif,direction);
                     var_ptr_mot[15]=1; 
                    } /*else*/
                  } /*else*/ 
         case 4 : if ( ref ) {
                     if ( var_ptr_mot[0] ) drive_pos(memberlist[arg.ptr[1]],dif,direction);
                     else                  drive_neg(memberlist[arg.ptr[1]],dif,direction);
                  }
                  else { 
                     stop_motor(memberlist[arg.ptr[1]],var_ptr_mot);
                     if ( lim_l ) var_ptr_mot[16]= (var_ptr_mot[16] | 4);
                     else { if ( lim_r) var_ptr_mot[16]= (var_ptr_mot[16] | 8);}
                  }
         } /*switch*/
   } /*if Referenz oder Limits*/
/****************************************************************************/

   else {
/* --- Encoder auslesen und Richtung und Geschwindigkeit waehlen ---------- */
        switch ( get_mod_type(memberlist[arg.ptr[2]]) ) {
           case ESSI   : 

/* --- warum auch immer?! So lassen !!! Mit Prozeduraufruf geht es nicht. - */ 
{
u_char b[7],s;
int c_addr,i,k,hlp;
int erg,dummy;

channel= ( (memberlist[arg.ptr[2]] >> 28) & 255 ); 
c_addr= 11+channel*10;
s=var_ptr_enc[8];
k=0;

while ( !erg && ((k+=1)<MAX_GET_SEMA) )
{
erg= ( *A_Addr(memberlist[arg.ptr[2]],S_GET_)&1 );
if (erg) break;
else dummy=*A_Addr(memberlist[arg.ptr[2]],1);
} 
if (!erg) 
{  stop_motor(memberlist[arg.ptr[1]],var_ptr_mot);
   var_ptr_mot[16]= (var_ptr_mot[16] | 256);
}
for (i=0;i<8;i++)
{  b[i]=*A_Addr(memberlist[arg.ptr[2]],c_addr);
   c_addr=c_addr+1;
}
dummy= *A_Addr(memberlist[arg.ptr[2]],1);

*ssi_high=b[7];*ssi_low=b[3];
for(i=1;i<4;i++)
{  *ssi_high=(*ssi_high<<8);*ssi_high+=b[7-i];
   *ssi_low=(*ssi_low<<8);*ssi_low+=b[3-i];
}
if (s==2)
{  *ssi_low=(*ssi_low>>2);
   hlp=(*ssi_high & 3);
   *ssi_low=*ssi_low+(hlp<<30);
   *ssi_high=(*ssi_high>>2);
}                  
break;
}
           case E233   : {
                         break;
                         }
        } /*switch*/ 

/* --- Differenz zwischen Ziel- und IstPosition --------------------------- */
D(D_REQ,printf("ENCODER IST 0x%x SOLL 0x%x \n",*ssi_low,arg.ptr[6]);)
       dif=arg.ptr[6]-*ssi_low;
       if (dif>=0) vorz=1;
       else
       {  vorz=0; dif= (-dif);}
/* !!! Vorsicht! wenn Ziel = 0 und Ist = FFFA dann wuerde Motor fasch rum fahren !!! */

/* --- wenn ohne nachjustieren, dann hier abbrechen ----------------------- */
      if ( !var_ptr_mot[1] || (get_mod_type(memberlist[arg.ptr[2]])==E233)  ) 
      {

/* --- der Motor wird gestoppt -------------------------------------------- */
        stop_motor(memberlist[arg.ptr[1]],var_ptr_mot);
        var_ptr_mot[16]= (var_ptr_mot[16] | 128);
        if ( !dif ) var_ptr_mot[16]= (var_ptr_mot[16] | 2);
     }
/* --- ansonsten wird nachjustiert ---------------------------------------- */
     else {

/* --- Ist der Motor im EPS-Bereich oder die Anzahl der Justierversuche --- */
/* --- erreicht, wird ggf. nochmals von der richtigen Seite angefahren ---- */
         if ( !var_ptr_mot[2] ) {
      
/* --- der Motor wird gestoppt -------------------------------------------- */
           stop_motor(memberlist[arg.ptr[1]],var_ptr_mot);
           var_ptr_mot[16]= (var_ptr_mot[16] | 64);
           if ( !dif ) var_ptr_mot[16]= (var_ptr_mot[16] | 2);
         }
         if ( (abs(dif)<=EPS)| var_ptr_mot[15] ) {

/* --- Ist der Motor im EPS-Bereich gewesen, wird ggf. nochmal negativ ---- */
/* --- gefahren, um dann abschliessend positiv zu positionieren. ---------- */
            switch ( var_ptr_mot[15] ) {  
               case 2 : {
/* --- Fahre das Ziel schrittweise an ------------------------------------- */
                  if ( abs(*ssi_low-arg.ptr[6]) > var_ptr_enc[0] )
                  {  if ( *ssi_low>arg.ptr[6] )
                        drive_slow_pos(memberlist[arg.ptr[1]],1,direction);
                     else
                        drive_slow_neg(memberlist[arg.ptr[1]],10,direction);
                  }
                  else {
/* --- der Motor wird gestoppt -------------------------------------------- */
                     stop_motor(memberlist[arg.ptr[1]],var_ptr_mot);
                     var_ptr_mot[16]= (var_ptr_mot[16] | 2);
                  }
                  break;
               }
/* --- sonst wird der Motor von der positiven Seite angefahren ----------- */
               case 0 : {
                  if ( *ssi_low<=arg.ptr[6] ) {

/* --- fahre 10 Schritte zurueck ----------------------------------------- */
                     drive_slow_neg(memberlist[arg.ptr[1]],30,direction);
                  }
                  var_ptr_mot[15]=2;
                  break;
               }
            } /* switch */
         } /* if ( abs(dif)<=EPS) ) */

/****************************************************************************/
         else {
/* --- Umrechnungsfaktor: Encoderdifferenz => Pulse => Motorschritte ------ */
            dif= (int)( ((float)dif*(float)var_ptr_mot[9])/(float)var_ptr_enc[7] );

            if ( vorz ) {
               if ( dif < (10*EPS) ) drive_slow_neg(memberlist[arg.ptr[1]],dif,direction);
               else                  drive_neg(memberlist[arg.ptr[1]],dif,direction);
            }
            else {
               if ( dif < (10*EPS) ) drive_slow_pos(memberlist[arg.ptr[1]],dif,direction);
               else                  drive_pos(memberlist[arg.ptr[1]],dif,direction);
            }/* else */
         }/* else nicht im EPS-Bereich*/
         var_ptr_mot[2]=var_ptr_mot[2]-1;
      } /*else mit justage*/
    } /* kein Refpkt oder Limit*/
   } /*if Motor steht */
}/* if signal look */

} /*sighand*/

