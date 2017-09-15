/*****************************************************************************
FIRMA:          KFA-Juelich, Institute ZEL, IKP
PROJEKT:        LEX (Low Energy X-ray spectrometer)

MODULNAME:      scaler.c
SCHLAGWORTE:    Sammlung von Prozeduren fuer den Scaler-Betrieb

AUTOR:          THOMAS SIEMS
DATUM:          26.8.1993
VERSION:        1.0
MODIFIKATIONEN:
    18.11.1994: Position in Modulliste wird beim Aufruf vom klient aus als
                Argument (p[1]) "ubergeben (macht modulorder.h unn"otig und
                erm"oglicht mehrere Scaler in einen IS anzusprechen.
    23.12.1993: CONV2BCD fuer SCAwrite implementiert
    14.12.1993: int in VOLATILE int bei dummy umgeaendert

STATUS:         freigegeben

BESCHREIBUNG:   Scaler-Behandlung im Server; read/write/set/reset
   Um den CAMAC anzusprechen, muss auf die Adresse CAMACaddr etwas geschrieben
   werden (Hardware-Anbindung). 'Set_scaler' z.B. erscheint im client als
   Kommando mit beliebig vielen Argumenten. Dort wird dann der Pointer p
   creiert: p[0]-->#Anzahl Argumente n, p[1-n]: Argumente. 'test_proc_Set_scaler
   ueberprueft dann, ob fuer dieses Kommando die richtige Anzahl Argumente
   gegeben wurde (Set_scaler-->1), deren Bereich und das Vorhandensein einer
   memberlist (welche Module zu diesem Instrumentierungs-System (IS) gehoeren).

   Es gibt folgende Moeglichkeiten der Kommunikation mit dem CAMAC:
        write:          *CAMACaddr(p[1],p[2],p[3])=p[4]  (schreibe p[4])
        read :          *outptr++=*CAMACaddr(,,)
        control:        register VOLATILE int dummy=*CAMACaddr(,,)
        block-read:     outptr=CAMACblread(outptr,CAMACaddr(,,),p[4])
        block-read:     register int *addr=CAMACaddr(,,),space=p[4]

 *****************************************************************************/

#include <errorcodes.h>
#include <modultypes.h>
#include "../../../lowlevel/camac/camac.h"
#include <sconf.h>
#include <debug.h>
#include "scaler.h"

extern ems_u32* outptr;
extern int *memberlist;

/*---------------------------------------------------------------------------*/
plerrcode proc_Enable_scaler(p)   /* CONTROL-Command */
int *p;
{
        register VOLATILE int dummy=*CAMACaddr(memberlist[p[1]],
                                               SCA_SUB_ADR,SCA_ENABLE);
        return(plOK);
}

plerrcode test_proc_Enable_scaler(p)
int *p;
{
    if(p[0]!=1)return(plErr_ArgNum);
    if(!memberlist)return(plErr_NoISModulList);
    if(*memberlist<3)return(plErr_BadISModList);
    if(get_mod_type(memberlist[p[1]])!=SCALER_JEA20)
        {
        *outptr++=memberlist[p[1]];
        return(plErr_BadModTyp);
        }
    return(plOK);
}

char name_proc_Enable_scaler[]="ScaEnable";
int ver_proc_Enable_scaler=1;
/*---------------------------------------------------------------------------*/
plerrcode proc_Disable_scaler(p)   /* CONTROL-Command */
int* p;
{
        register int VOLATILE dummy=*CAMACaddr(memberlist[p[1]],
                                     SCA_SUB_ADR, SCA_DISABLE);
        return(plOK);
}

plerrcode test_proc_Disable_scaler(p)
int *p;
{
    if(p[0]!=1) return(plErr_ArgNum);
    if(!memberlist) return(plErr_NoISModulList);
    if(*memberlist<3) return(plErr_BadISModList);
    if(get_mod_type(memberlist[p[1]])!=SCALER_JEA20)
        {
        *outptr++=memberlist[p[1]];
        return(plErr_BadModTyp);
        }
    return(plOK);
}

char name_proc_Disable_scaler[]="ScaDisable";
int ver_proc_Disable_scaler=1;
/*---------------------------------------------------------------------------*/
plerrcode proc_Write_scaler(p)    /* WRITE-Command */
int *p;
{
        int dummy;
        dummy = Conv2BCD(p[2]);
        *CAMACaddr(memberlist[p[1]],SCA_SUB_ADR,SCA_WRITE)=dummy;
        return(plOK);
}

plerrcode test_proc_Write_scaler(p)
int *p;
{
        if(p[0]!=2) return(plErr_ArgNum);
        if((p[2]<0) || (p[2]>999999)) return(plErr_ArgRange);
        if(!memberlist) return(plErr_NoISModulList);
        if(*memberlist<3) return(plErr_BadISModList);
        if( get_mod_type(memberlist[p[1]])!=SCALER_JEA20 )
            {
            *outptr++=memberlist[p[1]];
            return(plErr_BadModTyp);
            }
        return(plOK);
}

char name_proc_Write_scaler[]="ScaWrite";
int ver_proc_Write_scaler=1;
/*---------------------------------------------------------------------------*/
plerrcode proc_Reset_scaler(p)   /* CONTROL-Command */
int* p;
{
register int VOLATILE dummy;

dummy==*CAMACaddr(memberlist[p[1]], SCA_SUB_ADR, SCA_RESET);
return(plOK);
}

plerrcode test_proc_Reset_scaler(p)
int *p;
{
if (p[0]!=1) return(plErr_ArgNum);
if (!memberlist) return (plErr_NoISModulList);
if (*memberlist<3) return (plErr_BadISModList);
if (get_mod_type(memberlist[p[1]])!=SCALER_JEA20)
  {
  *outptr++=memberlist[p[1]];
  return(plErr_BadModTyp);
  }
return(plOK);
}

char name_proc_Reset_scaler[]="ScaReset";
int ver_proc_Reset_scaler=1;
/*---------------------------------------------------------------------------*/
plerrcode proc_Read_scaler(p)    /* READ-Command */
int* p;
{
        *outptr++ = *CAMACaddr(memberlist[p[1]],SCA_SUB_ADR,SCA_READ);
        return(plOK);
}

plerrcode test_proc_Read_scaler(p)
int *p;
{
    if(p[0]!=1) return(plErr_ArgNum);
    if(!memberlist) return(plErr_NoISModulList);
    if(*memberlist<3) return(plErr_BadISModList);
    if( get_mod_type(memberlist[p[1]])!=SCALER_JEA20 )
        {
        *outptr++ = memberlist[p[1]];
        return(plErr_BadModTyp);
        }
    return(plOK);
}

char name_proc_Read_scaler[]="ScaRead";
int ver_proc_Read_scaler=1;
/*---------------------------------------------------------------------------*/
plerrcode proc_Test_lam_scaler(p)   /* CONTROL-Command */
int* p;
{
        register VOLATILE int dummy=*CAMACaddr(memberlist[p[1]],
                                     SCA_SUB_ADR, SCA_TEST_LAM);
        return(plOK);
}

plerrcode test_proc_Test_lam_scaler(p)
int *p;
{
    if(p[0]!=1) return(plErr_ArgNum);
    if(!memberlist) return(plErr_NoISModulList);
    if(*memberlist<3) return(plErr_BadISModList);
    if( get_mod_type(memberlist[p[1]])!=SCALER_JEA20 )
        {
        *outptr++ = memberlist[p[1]];
        return(plErr_BadModTyp);
        }
    return(plOK);
}

char name_proc_Test_lam_scaler[]="ScaTestLam";
int ver_proc_Test_lam_scaler=1;
/*---------------------------------------------------------------------------*/
plerrcode proc_Clear_lam_scaler(p)   /* CONTROL-Command */
int* p;
{
        register int VOLATILE dummy=*CAMACaddr(memberlist[p[1]],
                                     SCA_SUB_ADR, SCA_CLEAR_LAM);
        return(plOK);
}

plerrcode test_proc_Clear_lam_scaler(p)
int *p;
{
    if(p[0]!=1) return(plErr_ArgNum);
    if(!memberlist) return(plErr_NoISModulList);
    if(*memberlist<3) return(plErr_BadISModList);
    if( get_mod_type(memberlist[p[1]])!=SCALER_JEA20 )
        {
        *outptr++ = memberlist[p[1]];
        return(plErr_BadModTyp);
        }
    return(plOK);
}

char name_proc_Clear_lam_scaler[]="ScaClearLam";
int ver_proc_Clear_lam_scaler=1;
/*---------------------------------------------------------------------------*/
plerrcode proc_Inc_scaler(p)   /* CONTROL-Command, incrementiere um 1 */
int* p;
{
        register VOLATILE int dummy=*CAMACaddr(memberlist[p[1]],
                                     SCA_SUB_ADR, SCA_INCREMENT);
        return(plOK);
}

plerrcode test_proc_Inc_scaler(p)
int *p;
{
    if(p[0]!=1) return(plErr_ArgNum);
    if(!memberlist) return(plErr_NoISModulList);
    if(*memberlist<3) return(plErr_BadISModList);
    if( get_mod_type(memberlist[p[1]])!=SCALER_JEA20 )
        {
        *outptr++ = memberlist[p[1]];
        return(plErr_BadModTyp);
        }
    return(plOK);
}

char name_proc_Inc_scaler[]="ScaIncrement";
int ver_proc_Inc_scaler=1;
/*---------------------------------------------------------------------------*/
