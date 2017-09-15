/* $ZEL: smpreadout.c,v 1.6 2005/02/11 17:58:31 wuestner Exp $
                                                                     */
/* Auslesen eines SMP-Speichers                                      */
/* Parameter: 1) Adresse des Smpmoduls aus der geladenen Instrument- */
/*               ierungsliste. Bei der Adresse des zweiten Rahmens   */
/*               muss                                                */
/*            2) DUMMYparameter hier 0x2800 bezieht sich auf den     */
/*               SMPrahmen.                                          */
/*                                                                   */
/* 03.05.'93                                                         */
/*                                                                   */
/* Autor: Twardo                                                     */
/*                                                                   */
/*                                                                   */
/*********************************************************************/
#include <types.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <sconf.h>
#include <debug.h>
#include "../../LOWLEVEL/VME/vme.h"

extern ems_u32* outptr;
extern int* memberlist;

plerrcode proc_SMPreadout(p)
int *p;
{

    int welcher,addr,typ;
    u_char *adresse,*dummy;

    T(VMEreadout) 

    welcher= *++p;
    addr=memberlist[welcher];              /* Slotnummer des Speichers */
    typ=get_mod_type(memberlist[welcher]); /* Typ noch nicht verwendet */
    D(D_USER,printf("Mem %d in Adresse %d\n",*p,addr);)

    dummy=SMPaddr(addr);
    *outptr++=(int)*dummy;

    return(plOK);
}

plerrcode test_proc_SMPreadout(p)
int *p;
{
    if(*p++!=2)return(plErr_ArgNum);
    if(*p>p[0])return(plErr_ArgRange);
    if(!memberlist)return(plErr_NoISModulList);
    if(get_mod_type(memberlist[*p])!=LED_2800){
  *outptr++=memberlist[*p];
  return(plErr_BadModTyp);
    }

    return(plOK);
}

char name_proc_SMPreadout[]="SMPreadout";

int ver_proc_SMPreadout=1;

