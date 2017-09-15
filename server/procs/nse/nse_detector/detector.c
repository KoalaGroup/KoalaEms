
#include <stdio.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <sconf.h>
#include <debug.h>
#include <types.h>
#include "../../../OBJECTS/VAR/variables.h"
#include "../../../LOWLEVEL/VME/vme.h"

#include "detector.h"

extern int start_detector();
extern int stop_detector();
extern int write_parm_detector();
extern int erase_detector();
extern get_parameter();
extern u_char timebit[1];
extern u_char inhibit[3];
extern int hoch();
extern int find_high_enable_bit(); 

extern ems_u32* outptr;
extern int* memberlist;

int xdet,ydet;
int pagesize,pagenumber;
/*****************************************************************************/
/*                                                                           */
/* Detector_Channel_Init(adresse)                                  */
/*                                                                           */
/* Initialisiert den Detektor.                                               */
/* Die Karte wird angehalten und das Memory geloescht. Dann werden die Time- */
/* und Inhibitbit gesetzt. Diese werden aus der zugehoerigen Variable        */
/* Varlength 2) entnommen. Es wird die Pagegroesse und die Zahl der          */
/* benoetigten Pages berechnet. Danach wird die Karte gestartet.             */
/* Da die Pagegroesse und Zahl der Pages global verwaltet werden, muss vor   */
/* Auslesen der Karte eine Initialisierung erfolgen.                         */
/*                                                                           */
/* 31.01.'94                                                                 */
/*                                                                           */
/* Autor Twardowski                                                          */
/*                                                                           */
/*****************************************************************************/

plerrcode proc_Detector_Channel_Init(p)

int *p;
{
int    welcher,adresse;
int *varptr;
int r_code;

welcher=p[1];
varptr=var_list[p[1]].var.ptr;

/* --- return Modulnumber ------------- */
*outptr++= welcher;

D(D_REQ,printf("Detector_Init Modul %d Adresse %x\n",p[1],memberlist[welcher]);)
D(D_REQ,printf("Timebits %x Inhibitbits %x\n",varptr[0],varptr[1]);)

adresse=memberlist[welcher];

/* --- get Timebits and Inhibitbits ------------- */
get_parameter(varptr);

/* --- Stop Channelcard ------------------------- */
if ( r_code=stop_detector(adresse) )
{  *outptr++ = C_STOP+r_code;
   return(plErr_Other);
}

/* --- Erase Memory ----------------------------- */
if ( r_code=clear_detector(adresse,MEMORY_OFFSET,MEMORY_SIZE/4) )
{  *outptr++ = C_CLEAR+r_code;
   D(D_REQ,printf("Detector_W_Parm CLEAR r_code %d\n",r_code);)
   return(plErr_Other);
}

/* --- WriteParameter --------------------------- */
if ( r_code=write_parm_detector(adresse) )
{  *outptr++ = C_W_PARAM+r_code;
   return(plErr_Other);
}

/* --- find Pagesize and Number of Pages -------- */
/* --- first eigth LowerBit X-Infomation and ---- */
/* --- next eigth LowerBit Y-Information -------- */
xdet=find_high_enable_bit(varptr[1],XDETMIN,XDETMAX);
ydet=find_high_enable_bit(varptr[1],YDETMIN,YDETMAX)-YDETMIN+1;
printf("xdet %d ydet %d var1 %x\n",xdet,ydet,varptr[1]);
if ( (pagesize=hoch(2,xdet))==(-1) ) 
{  *outptr++ =PAGE; 
   return(plErr_Other);
}
if ( (pagenumber=hoch(2,ydet))==(-1) ) 
{  *outptr++ =PAGE; 
   return(plErr_Other);
}

D(D_REQ,printf("Pagesize 0x%x*4Byte Pagenumber 0x%x\n",pagesize,pagenumber);)

/* --- Start Channelcard ------------------------ */
if ( r_code=start_detector(adresse) )
{  *outptr++ = C_START+r_code;
   return(plErr_Other);
}
return(plOK);
}

plerrcode test_proc_Detector_Channel_Init(p)

int *p;
{
   if ( p[0] !=1 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != ILL_CHANNEL ){
      *outptr++=memberlist[p[1]];
                                   return(plErr_BadModTyp);
   }
   if ( p[1]>MAX_VAR )             return(plErr_IllVar);
   if ( !var_list[p[1]].len )      return(plErr_NoVar);
   if ( var_list[p[1]].len !=2)    return(plErr_IllVarSize);
   return(plOK);
}
char name_proc_Detector_Channel_Init[]="Detector_Channel_Init";
int ver_proc_Detector_Channel_Init=1;

/*****************************************************************************/
/*                                                                           */
/* Detector_Channel_Stop(adresse)                                            */
/*                                                                           */
/* Stoppt den Detektor.                                                      */
/*                                                                           */
/* 31.01.'94                                                                 */
/*                                                                           */
/* Autor Twardowski                                                          */
/*                                                                           */
/*****************************************************************************/

plerrcode proc_Detector_Channel_Stop(p)

int *p;
{
int    welcher,adresse;
int r_code;

welcher=p[1];

/* --- return Modulnumber ------------- */
*outptr++= welcher;

D(D_REQ,printf("Detector_stop Modul %d Adresse %x\n",p[1],memberlist[welcher]);)

adresse=memberlist[welcher];

/* --- Stop Channelcard --------------- */
if ( r_code=stop_detector(adresse) )
{  *outptr++ = C_STOP+r_code;
   return(plErr_Other);
}
return(plOK);
}

plerrcode test_proc_Detector_Channel_Stop(p)

int *p;
{
   if ( p[0] !=1 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != ILL_CHANNEL ){
      *outptr++=memberlist[p[1]];
                                   return(plErr_BadModTyp);
   }
   return(plOK);
}
char name_proc_Detector_Channel_Stop[]="Detector_Channel_Stop";
int ver_proc_Detector_Channel_Stop=1;

/*****************************************************************************/
/*                                                                           */
/* Detector_Channel_Start(adresse)                                           */
/*                                                                           */
/* Startet den Detektor.                                                     */
/*                                                                           */
/* 31.01.'94                                                                 */
/*                                                                           */
/* Autor Twardowski                                                          */
/*                                                                           */
/*****************************************************************************/
plerrcode proc_Detector_Channel_Start(p)

int *p;
{
int    welcher,adresse;
int r_code;


welcher=p[1];

/* --- return Modulnumber ------------- */
*outptr++= welcher;

D(D_REQ,printf("Detector_Start Modul %d Adresse %x\n",p[1],memberlist[welcher]);)

adresse=memberlist[welcher];

/* --- Start Channelcard -------------- */
if ( r_code=start_detector(adresse) )
{  *outptr++ = C_START+r_code;
   return(plErr_Other);
}
return(plOK);
}

plerrcode test_proc_Detector_Channel_Start(p)

int *p;
{
   if ( p[0] !=1 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != ILL_CHANNEL ){
      *outptr++=memberlist[p[1]];
                                   return(plErr_BadModTyp);
   }
   return(plOK);
}
char name_proc_Detector_Channel_Start[]="Detector_Channel_Start";
int ver_proc_Detector_Channel_Start=1;


/*****************************************************************************/
/*                                                                           */
/* Detector_Channel_W_Param(adresse)                                         */
/*                                                                           */
/* Schreibt Time- und Inhibitbits                                            */
/*                                                                           */
/* 31.01.'94                                                                 */
/*                                                                           */
/* Autor Twardowski                                                          */
/*                                                                           */
/*****************************************************************************/
plerrcode proc_Detector_Channel_W_Parm(p)

int *p;
{
int    welcher,adresse;
int *varptr;
int r_code;


welcher=p[1];

/* --- return Modulnumber ------------- */
*outptr++= welcher;
varptr=var_list[p[1]].var.ptr;

D(D_REQ,printf("Detector_W_Parm Modul %d Adresse %x\n",p[1],memberlist[welcher]);)
D(D_REQ,printf("Timebits %x Inhibitbits %x\n",varptr[0],varptr[1]);)

adresse=memberlist[welcher];

/* --- get Timebits and Inhibitbits --- */
get_parameter(varptr);

/* --- Stop Channelcard --------------- */
if ( r_code=stop_detector(adresse) )
{  *outptr++ = C_STOP+r_code;
   D(D_REQ,printf("Detector_W_Parm STOP r_code %d\n",r_code);)
   return(plErr_Other);
}

/* --- Write Parameter ---------------- */
if ( r_code=write_parm_detector(adresse) )
{  *outptr++ = C_W_PARAM+r_code;
   D(D_REQ,printf("Detector_W_Parm W_PARAM r_code %d\n",r_code);)
   return(plErr_Other);
}
return(plOK);
}

plerrcode test_proc_Detector_Channel_W_Parm(p)
int *p;
{
   if ( p[0] !=1 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != ILL_CHANNEL ){
      *outptr++=memberlist[p[1]];
                                   return(plErr_BadModTyp);
   }
   if ( p[1]>MAX_VAR )             return(plErr_IllVar);
   if ( !var_list[p[1]].len )      return(plErr_NoVar);
   if ( var_list[p[1]].len !=2)    return(plErr_IllVarSize);
   return(plOK);
}

char name_proc_Detector_Channel_W_Parm[]="Detector_Channel_W_Parm";
int ver_proc_Detector_Channel_W_Parm=1;

/*****************************************************************************/
/*                                                                           */
/* Detector_Channel_ClearMem(adresse,a_addr,count)                           */
/*                                                                           */
/* loescht Memorybereiche                                                    */
/* Parameter: adresse: Basisadresse der Detectorkarte                        */
/*            a_addr : Startadresse (Offset auf die Basisadresse)            */
/*            count  : Zahl der zu loeschenden Zaehlkanaele (je 4 Byte)      */
/*                                                                           */
/* 31.01.'94                                                                 */
/*                                                                           */
/* Autor Twardowski                                                          */
/*                                                                           */
/*****************************************************************************/
plerrcode proc_Detector_Channel_ClearMem(p)

int *p;
{
int    welcher,adresse,a_addr,count;
int r_code;

welcher=p[1];
adresse=memberlist[welcher];
a_addr =p[2];
count  =p[3];

/* --- return Modulnumber ------------- */
*outptr++= welcher;

D(D_REQ,printf("Detector_ClearMem Modul %d Adresse %x\n",p[1],memberlist[welcher]);)
D(D_REQ,printf("ERASE: %x ... %x\n",a_addr,a_addr+count*4);)

/* --- Stop Channelcard --------------- */
if ( r_code=stop_detector(adresse) )
{  *outptr++ = C_STOP+r_code;
   D(D_REQ,printf("Detector_ClearMem STOP r_code %d\n",r_code);)
   return(plErr_Other);
}

/* --- Clear Memory ------------------- */
if ( r_code=clear_detector(adresse,a_addr,count) )
{  *outptr++ = C_CLEAR+r_code;
   D(D_REQ,printf("Detector_ClearMem Clear r_code %d\n",r_code);)
   return(plErr_Other);
}
return(plOK);
}

plerrcode test_proc_Detector_Channel_ClearMem(p)
int *p;
{
   if ( p[0] !=3 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != ILL_CHANNEL ){
      *outptr++=memberlist[p[1]];
                                   return(plErr_BadModTyp);
   }
   return(plOK);
}

char name_proc_Detector_Channel_ClearMem[]="Detector_Channel_ClearMem";
int ver_proc_Detector_Channel_ClearMem=1;

/*****************************************************************************/
/*                                                                           */
/* Detector_Channel_ReadMem(adresse,page,sum)                                */
/*                                                                           */
/* liesst Memorybereiche                                                     */
/* Parameter: adresse: Basisadresse der Detectorkarte                        */
/*            page   : 0 alles, sonst Pagenummer                             */
/*            sum    : 0 alle einzeln, sonst Summe                           */
/*                                                                           */
/* 09.02.'94                                                                 */
/*                                                                           */
/* Autor Twardowski                                                          */
/*                                                                           */
/*****************************************************************************/
plerrcode proc_Detector_Channel_ReadMem(p)

int *p;
{
int    welcher,adresse;
int r_code;

welcher=p[1];
adresse=memberlist[welcher];

/* --- return Modulnumber ------------- */
*outptr++= welcher;

D(D_REQ,printf("Detector_ReadMem Modul %d Adresse %x\n",p[1],memberlist[welcher]);)
D(D_REQ,printf("READ: Page %d Sum %d\n",p[2],p[3]);)
fflush(stdout);

/* --- read Memory -------------------- */
if ( r_code=readmem_detector(adresse,p[2],p[3]) )
{  *outptr++ = C_READ+r_code;
   D(D_REQ,printf("Detector_ReadMem READ r_code %d\n",r_code);)
   return(plErr_Other);
}

return(plOK);
}

plerrcode test_proc_Detector_Channel_ReadMem(p)
int *p;
{
   if ( p[0] !=3 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != ILL_CHANNEL ){
      *outptr++=memberlist[p[1]];
                                   return(plErr_BadModTyp);
   }
   if ( p[2]<0 ) return(plErr_BadHWAddr);
   return(plOK);
}

char name_proc_Detector_Channel_ReadMem[]="Detector_Channel_ReadMem";
int ver_proc_Detector_Channel_ReadMem=1;

/*****************************************************************************/
/*                                                                           */
/* Detector_Channel_Status(adresse)                                          */
/*                                                                           */
/* liesst:                                                                   */
/* Commandregister (offset 0x4000 0..3)                                      */
/* Writeparameter (offset 0x4000 6..9, A..D, E..11)                          */
/* TimeBits (offset 0x4000 F8..FB)                                           */
/* InhibitBits (offset 0x4000 FC..FF)                                        */
/* Pagenumber, Pagesize                                                      */
/*                                                                           */
/* Parameter: adresse: Basisadresse der Detectorkarte                        */
/*                                                                           */
/* 09.02.'94                                                                 */
/*                                                                           */
/* Autor Twardowski                                                          */
/*                                                                           */
/*****************************************************************************/
plerrcode proc_Detector_Channel_Status(p)

int *p;
{
int    welcher,adresse;
int *hlp;

welcher=p[1];
adresse=memberlist[welcher];

/* --- return Modulnumber ------------- */
*outptr++= welcher;

D(D_REQ,printf("Detector_Status Modul %d Adresse %x\n",p[1],memberlist[welcher]);)

/* --- Commandregister ---------------- */
hlp = (int *)(adresse+CARD_OFFSET);
*outptr++ = *hlp;

/* --- WriteParameter ----------------- */
hlp = (int *)(adresse+CARD_OFFSET+INHIBIT_OFFSET);
*outptr++ = *hlp;
hlp = (int *)(adresse+CARD_OFFSET+0xA);
*outptr++ = *hlp;
hlp = (int *)(adresse+CARD_OFFSET+0xE);
*outptr++ = *hlp;

/* --- TimeBits set -------------------- */
hlp = (int *)(adresse+CARD_OFFSET+0xF8);
*outptr++ = *hlp;

/* --- InhibitBits set ----------------- */
hlp = (int *)(adresse+CARD_OFFSET+0xFC);
*outptr++ = *hlp;

/* --- Pagenumber, Pagesize ------------ */
*outptr++ = pagenumber;
*outptr++ = pagesize;

return(plOK);
}

plerrcode test_proc_Detector_Channel_Status(p)
int *p;
{
   if ( p[0] !=1 )                 return(plErr_ArgNum);
   if ( !memberlist )              return(plErr_NoISModulList);
   if ( get_mod_type(memberlist[p[1]]) != ILL_CHANNEL ){
      *outptr++=memberlist[p[1]];
                                   return(plErr_BadModTyp);
   }
   return(plOK);
}

char name_proc_Detector_Channel_Status[]="Detector_Channel_Status";
int ver_proc_Detector_Channel_Status=1;



