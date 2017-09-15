#include <errorcodes.h>
#include <types.h>
#include <stdio.h>
#include <modultypes.h>
#include <sconf.h>
#include <debug.h>
#include "../../OBJECTS/VAR/variables.h"
#include "../../LOWLEVEL/VME/vme.h"

extern ems_u32* outptr;
extern int* memberlist;

/******************************************************************************
*                                                                             *
* DIO_E217_in(Adresse,Value)                                                  *
*                                                                             *
*        Adresse = Basisadresse                                               *
*        Value   = Wert (input)                                               *
*                                                                             *
* Confirmation: Nummer des Moduls                                             *
*                                                                             *
******************************************************************************/
plerrcode proc_DIO_E217_in(p)
int *p;

{
int welcher,adresse;
int value;
u_char *b;

welcher=p[1];

*outptr++=welcher;

D(D_REQ,printf("DIO_E217_in Modul %d\n",p[1]);)
adresse=memberlist[welcher];
value  =p[2];

b=(u_char *)malloc(5*sizeof(b));

b=MAKBYT(4,value);

*A_Addr(adresse,0) = b[1];
*A_Addr(adresse,1) = b[2];
*A_Addr(adresse,2) = b[3];
*A_Addr(adresse,3) = b[4];

return(plOK);
}
/***************************************************************************/
plerrcode test_proc_DIO_E217_in(p)
int *p;
{
if ( p[0]!=2 )                   return(plErr_ArgNum);
if ( !memberlist )               return(plErr_NoISModulList);
if ( get_mod_type(memberlist[p[1]]) != E217 ) {
   *outptr++ = memberlist[p[1]];
                                 return(plErr_BadModTyp);
}

return(plOK);

}

char name_proc_DIO_E217_in[]="DIO_E217_in";
int ver_proc_DIO_E217_in=1;
/*****************************************************************************

/******************************************************************************
*                                                                             *
* DIO_E217_out(Adresse,Value)                                                 *
*                                                                             *
*        Adresse    = Basisadresse                                            *
*        Variable   = Wert (input)                                            *
*                                                                             *
* Confirmation: Modulnummer Wert des Moduls                                   *
*                                                                             *
******************************************************************************/


plerrcode proc_DIO_E217_out(p)
int *p;

{
int welcher,adresse;
u_char b[5];

welcher=p[1];
adresse=memberlist[welcher];
*outptr++=welcher;

D(D_REQ,printf("DIO_E217_in Modul %d\n",p[1]);)

b[1]=*A_Addr(adresse,0);
b[2]=*A_Addr(adresse,1);
b[3]=*A_Addr(adresse,2);
b[4]=*A_Addr(adresse,3);

var_list[p[2]].var.val=MAKDOU(4,b);

*outptr++ = var_list[p[2]].var.val;
return(plOK);
}
/***************************************************************************/
plerrcode test_proc_DIO_E217_out(p)
int *p;

{
if ( p[0]!=2 )                   return(plErr_ArgNum);
if ( !memberlist )               return(plErr_NoISModulList);
if ( get_mod_type(memberlist[p[1]]) != E217 ) {
   *outptr++ = memberlist[p[1]];
                                 return(plErr_BadModTyp);
}
if ( p[2]>MAX_VAR )              return(plErr_IllVar);
if ( !var_list[p[2]].len )       return(plErr_NoVar);
if ( var_list[p[2]].len !=1 )    return(plErr_IllVarSize);

return(plOK);

}

char name_proc_DIO_E217_out[]="DIO_E217_out";
int ver_proc_DIO_E217_out=1;

