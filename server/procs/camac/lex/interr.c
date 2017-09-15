/*****************************************************************************
FIRMA:          KFA-Juelich, Institute ZEL, IKP
PROJEKT:        LEX (Low Energy X-ray spectrometer)

MODULNAME:      interr.c
SCHLAGWORTE:    Sammlung von Prozeduren fuer den Betrieb des Schlumberger
                Interrupt-Moduls JIR 10.  (Hier abgekuerzt mit IM).

AUTOR:          THOMAS SIEMS
DATUM:          18.10.1993
VERSION:        1.0
MODIFIKATIONEN: 18.11.1994, "Ubergebe Position in ML, erspart modulorder.h
                und erm"oglicht mehrere IR-Module anzusprechen.
STATUS:         freigegeben

BESCHREIBUNG:   JIR-Behandlung im Server; read/write/set/reset
   Um den CAMAC anzusprechen, muss auf die Adresse CAMACaddr etwas geschrieben
   werden (Hardware-Anbindung). 'IMReadIR' z.B. erscheint im client als
   Kommando mit beliebig vielen Argumenten. Dort wird dann der Pointer p
   creiert: p[0]-->#Anzahl Argumente n, p[1-n]: Argumente. 'test_proc_IMReadIR
   ueberprueft dann, ob fuer dieses Kommando die richtige Anzahl Argumente
   gegeben wurde (hier: 0), deren Bereich und das Vorhandensein einer
   memberlist (welche Module zu diesem Instrumentierungs-System (IS) gehoeren).

   Es gibt folgende Moeglichkeiten der Kommunikation mit dem CAMAC:
        write:          *CAMACaddr(p[1],p[2],p[3])=p[4]  (schreibe p[4])
        read :          *outptr++=*CAMACaddr(,,)
        control:        register int dummy=*CAMACaddr(,,)
        block-read:     outptr=CAMACblread(outptr,CAMACaddr(,,),p[4])
        block-read:     register int *addr=CAMACaddr(,,),space=p[4]

 *****************************************************************************/

#include <errorcodes.h>
#include <modultypes.h>
#include "../../../lowlevel/camac/camac.h"
#include <sconf.h>
#include <debug.h>
#include "interr.h"

extern ems_u32* outptr;
extern int* memberlist;

/*---------------------------------------------------------------------------*/
plerrcode proc_IM_read_ir(p)    /* READ input register */
int* p;
{
        *outptr++=*CAMACaddr(memberlist[p[1]],IM_SUB_ADR,IM_READ_IR);
        return(plOK);
}

plerrcode test_proc_IM_read_ir(p)
int *p;
{
    if(p[0]!=1)return(plErr_ArgNum);
    if(!memberlist)return(plErr_NoISModulList);
    if(*memberlist<3)return(plErr_BadISModList);
    if(get_mod_type(memberlist[p[1]])!=IM_JIR10)
        {
        *outptr++=memberlist[p[1]];
        return(plErr_BadModTyp);
        }
    return(plOK);
}

char name_proc_IM_read_ir[]="IMReadIR";
int ver_proc_IM_read_ir=1;
/*---------------------------------------------------------------------------*/
plerrcode proc_IM_read_mr(p)    /* READ mask register */
int* p;
{
        *outptr++=*CAMACaddr(memberlist[p[1]],IM_SUB_ADR,IM_READ_MR);
        return(plOK);
}

plerrcode test_proc_IM_read_mr(p)
int *p;
{
    if(p[0]!=1)return(plErr_ArgNum);
    if(!memberlist)return(plErr_NoISModulList);
    if(*memberlist<3)return(plErr_BadISModList);
    if(get_mod_type(memberlist[p[1]])!=IM_JIR10)
        {
        *outptr++=memberlist[p[1]];
        return(plErr_BadModTyp);
        }
    return(plOK);
}

char name_proc_IM_read_mr[]="IMReadMR";
int ver_proc_IM_read_mr=1;
/*---------------------------------------------------------------------------*/
plerrcode proc_IM_read_ir_cm(p) /* READ input register & clear mask register */
int* p;
{
*outptr++=*CAMACaddr(memberlist[p[1]],IM_SUB_ADR,IM_READ_IR_CM);
return(plOK);
}

plerrcode test_proc_IM_read_ir_cm(p)
int *p;
{
    if(p[0]!=1)return(plErr_ArgNum);
    if(!memberlist)return(plErr_NoISModulList);
    if(*memberlist<3)return(plErr_BadISModList);
    if(get_mod_type(memberlist[p[1]])!=IM_JIR10)
        {
        *outptr++=memberlist[p[1]];
        return(plErr_BadModTyp);
        }
    return(plOK);
}

char name_proc_IM_read_ir_cm[]="IMReadIRCM";
int ver_proc_IM_read_ir_cm=1;
/*---------------------------------------------------------------------------*/
plerrcode proc_IM_test_lam(p)   /* CONTROL-Command: test LAM */
int* p;
{
        register int dummy=*CAMACaddr(memberlist[p[1]],IM_SUB_ADR,
                                      IM_TEST_LAM);
        return(plOK);
}

plerrcode test_proc_IM_test_lam(p)
int *p;
{
    if(p[0]!=1)return(plErr_ArgNum);
    if(!memberlist)return(plErr_NoISModulList);
    if(*memberlist<3)return(plErr_BadISModList);
    if(get_mod_type(memberlist[p[1]])!=IM_JIR10)
        {
        *outptr++=memberlist[p[1]];
        return(plErr_BadModTyp);
        }
    return(plOK);
}

char name_proc_IM_test_lam[]="IMTestLam";
int ver_proc_IM_test_lam=1;
/*---------------------------------------------------------------------------*/
plerrcode proc_IM_clear_ir(p)   /* CONTROL-Command, loesche input register */
int* p;
{
        register int dummy=*CAMACaddr(memberlist[p[1]],IM_SUB_ADR,
                                      IM_CLEAR_IR);
        return(plOK);
}

plerrcode test_proc_IM_clear_ir(p)
int *p;
{
    if(p[0]!=1)return(plErr_ArgNum);
    if(!memberlist)return(plErr_NoISModulList);
    if(*memberlist<3)return(plErr_BadISModList);
    if(get_mod_type(memberlist[p[1]])!=IM_JIR10)
        {
        *outptr++=memberlist[p[1]];
        return(plErr_BadModTyp);
        }
    return(plOK);
}

char name_proc_IM_clear_ir[]="IMClearIR";
int ver_proc_IM_clear_ir=1;
/*---------------------------------------------------------------------------*/
plerrcode proc_IM_owrite_mr(p)    /* WRITE-Command, overwrite mask register */
int *p;
{
        *CAMACaddr(memberlist[p[1]],IM_SUB_ADR,IM_OWRITE_MR)=p[2];
        return(plOK);
}

plerrcode test_proc_IM_owrite_mr(p)
int *p;
{
        if(p[0]!=2)return(plErr_ArgNum);
        if((p[2]<0)&&(p[2]>0xFFFF))return(plErr_ArgRange);
        if(!memberlist)return(plErr_NoISModulList);
        if(*memberlist<3)return(plErr_BadISModList);
        if(get_mod_type(memberlist[p[1]])!=IM_JIR10)
        {
                *outptr++=memberlist[p[1]];
                return(plErr_BadModTyp);
        }
        return(plOK);
}

char name_proc_IM_owrite_mr[]="IMOWriteMR";
int ver_proc_IM_owrite_mr=1;
/*---------------------------------------------------------------------------*/
/* WRITE: selective clear input register */
/*        selective clear mask register  */
plerrcode proc_IM_SelClIR_SelSetMR(p)
int *p;
{
        *CAMACaddr(memberlist[p[1]],IM_SUB_ADR,IM_SCIR_SSMR)=p[2];
        return(plOK);
}

plerrcode test_proc_IM_SelClIR_SelSetMR(p)
int *p;
{
        if(p[0]!=2)return(plErr_ArgNum);
        if((p[2]<0)&&(p[2]>0xFFFF))return(plErr_ArgRange);
        if(!memberlist)return(plErr_NoISModulList);
        if(*memberlist<3)return(plErr_BadISModList);
        if(get_mod_type(memberlist[p[1]])!=IM_JIR10)
        {
                *outptr++=memberlist[p[1]];
                return(plErr_BadModTyp);
        }
        return(plOK);
}

char name_proc_IM_SelClIR_SelSetMR[]="IM_SelClIR_SelSetMR";
int ver_proc_IM_SelClIR_SelSetMR=1;
/*---------------------------------------------------------------------------*/
plerrcode proc_IM_dis_lam(p)   /* CONTROL-Command, disable lam */
int* p;
{
        register int dummy=*CAMACaddr(memberlist[p[1]],IM_SUB_ADR,
                                      IM_DIS_LAM);
        return(plOK);
}

plerrcode test_proc_IM_dis_lam(p)
int *p;
{
    if(p[1]!=0)return(plErr_ArgNum);
    if(!memberlist)return(plErr_NoISModulList);
    if(*memberlist<3)return(plErr_BadISModList);
    if(get_mod_type(memberlist[p[1]])!=IM_JIR10)
        {
        *outptr++=memberlist[p[1]];
        return(plErr_BadModTyp);
        }
    return(plOK);
}

char name_proc_IM_dis_lam[]="IMDisLam";
int ver_proc_IM_dis_lam=1;
/*---------------------------------------------------------------------------*/
plerrcode proc_IM_ena_lam(p)   /* CONTROL-Command, enable lam */
int* p;
{
        register int dummy=*CAMACaddr(memberlist[p[1]],IM_SUB_ADR,
                                      IM_ENA_LAM);
        return(plOK);
}

plerrcode test_proc_IM_ena_lam(p)
int *p;
{
    if(p[0]!=1)return(plErr_ArgNum);
    if(!memberlist)return(plErr_NoISModulList);
    if(*memberlist<3)return(plErr_BadISModList);
    if(get_mod_type(memberlist[p[1]])!=IM_JIR10)
        {
        *outptr++=memberlist[p[1]];
        return(plErr_BadModTyp);
        }
    return(plOK);
}

char name_proc_IM_ena_lam[]="IMEnaLam";
int ver_proc_IM_ena_lam=1;
/*---------------------------------------------------------------------------*/
/* CONTROL-Command, init: Clear IR + MR, disable Lam */
plerrcode proc_IM_init(p)
int* p;
{
register int dummy=*CAMACaddr(memberlist[p[1]],IM_SUB_ADR, IM_INIT);
return(plOK);
}

plerrcode test_proc_IM_init(p)
int *p;
{
    if(p[0]!=1)return(plErr_ArgNum);
    if(!memberlist)return(plErr_NoISModulList);
    if(*memberlist<3)return(plErr_BadISModList);
    if(get_mod_type(memberlist[p[1]])!=IM_JIR10)
        {
        *outptr++=memberlist[p[1]];
        return(plErr_BadModTyp);
        }
    return(plOK);
}

char name_proc_IM_init[]="IMInit";
int ver_proc_IM_init=1;
/*---------------------------------------------------------------------------*/
