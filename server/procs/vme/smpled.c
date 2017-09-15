/******************************************************************************
* $ZEL: smpled.c,v 1.4 2002/10/25 12:05:46 wuestner Exp $
*                                                                             *
* SmpLed.c                                                                    *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 27.04.93                                                                    *
*                                                                             *
* SmpLed(Loop,CFG,BASISADDR)                                                  *
*       int Loop Anzahl der Durchlaeufe                                       *
*       int CFG  Nummer des SMP-Rahmens                                       *
*       int BASISADDR Basisadresse der Moduls                                 *
*                                                                             *
* Autor: Twardo,Pohl(Programm)                                                *
*                                                                             *
******************************************************************************/
#include <errorcodes.h>
#include <types.h>
#include <stdio.h>
#include "../../OBJECTS/VAR/variables.h"
#include "../../LOWLEVEL/VME/vme.h"

/*****************************************************************************/

plerrcode proc_SmpLed(p)
int *p;
{
u_char  *smp_io;
int CFG;
int loop;
int BASISADDR;

int les0,les1,les2,les3;
int wert0,wert1,wert2,wert3;
double errcount;
double sequenz;

   loop=p[1];      /* Anzahl der Durchlaeufe */
   CFG=p[2];       /* Nummer des SMP-Rahmens */
   BASISADDR=p[3]; /* Basisadresse des Moduls */

   printf("          VSMP10 Test\n");
   printf("          Loop=%d\n",loop);
   printf("          VME=%x\n",CFG);

if(CFG==1) {
   smp_io = (u_char *)(A24_BASE+A24_IO_1); /* Pointer auf die Basisadresse */
                                           /* des ersten Smp-Rahmens */ 
} 
else {
   if(CFG==2) {
      smp_io = (u_char *)(A24_BASE+A24_IO_2); /* Pointer auf die Basisadresse */
                                              /* des zweiten Smp-Rahmens */ 
   }
   else {
      printf("Rahmen %d existiert nicht\n",CFG);
      exit(2);
   }
}
   
   les0=les1=les2=les3=0;
   wert0=wert1=wert2=wert3=0;
   sequenz=0;
   errcount=0;

   while ((loop--)>=0) {
   smp_io[BASISADDR]=wert0;
   les0=smp_io[BASISADDR];
   if (wert0!=les0) {
       printf("ERR AT BASISADDR OUT %x IN %x\n",wert0,les0);
       errcount=errcount+1;
       printf("Errcount_%10.0f\n",errcount);
   }
   smp_io[BASISADDR+1]=wert1;
   les1=smp_io[BASISADDR+1];
   if (wert1!=les1) {
      printf("ERR AT BASISADDR+1 OUT %x IN %x\n",wert1,les1);
      errcount=errcount+1;
      printf("Errcount_%10.0f\n",errcount);
   }
   smp_io[BASISADDR+2]=wert2;
   les2=smp_io[BASISADDR+2];
   if (wert2!=les2) {
      printf(" ERR AT BASISADDR+2 OUT %x IN %x\n",wert2,les2);
      errcount=errcount+1;
      printf("Errcount_%10.0f\n",errcount);
   }
   smp_io[BASISADDR+3]=wert3;
   les3=smp_io[BASISADDR+3];
   if (wert3!=les3) {
      printf("ERR AT BASISADDR+3 OUT %x IN %x\n",wert3,les3);
      errcount=errcount+1;
      printf("Errcount_%10.0f\n",errcount);
   }
   wert0=wert0+1;
   if (wert0==256) {wert1=wert1+1;wert0=0;}
   if (wert1==256) {wert2=wert2+1;wert1=0;}
   if (wert2==256) {wert3=wert3+1;wert2=0;}
   if (wert3==256) {wert0=wert1=wert2=wert3=0;sequenz=sequenz+1;
      printf("Sequenz_%10.0f Errcount_%10.0f\n",sequenz,errcount);
   }
}
printf("          SMPled fertig neue Version\n");
return(plOK);
}
/***************************************************************************/
plerrcode test_proc_SmpLed(p)
int *p;
{
if (*p!=3) return(plErr_ArgNum); /* Uberpruefung der Anzahl der */
return(plOK);                    /* uebergebenen Argumente */ 
}

char name_proc_SmpLed[]="SmpLed";
int ver_proc_SmpLed=1;
/****************************************************************************/
