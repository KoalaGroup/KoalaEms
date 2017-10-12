/*******************************************************************************
*                                                                              *
* fb_lc1885f_read.c                                                            *
*                                                                              *
* OS9                                                                          *
*                                                                              *
* MaWo                                                                         *
* PeWue                                                                        *
*                                                                              *
* 10. 5.95                                                                     *
* 11. 5.95  sparsification implemented                                         *
* 23. 5.95  sparsification updated                                             *
* 24. 2.97  "fb_lc1885f_readout_pl" added                                      *
*           new standard readout version: "fb_lc1885f_readout_pl"              *
*                                                                              *
*******************************************************************************/

#include <config.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <unsoltypes.h>
#include "../../../lowlevel/chi_neu/fbacc.h"
#include "../../../objects/var/variables.h"
#include "../../procprops.h"

#define MAX_WC  96                    /* max number of words for single event */
#define MAX_WC_PL  97          /* max number of words for single event plus 1 */

extern int *outptr;
extern int *memberlist;
extern int wirbrauchen;
extern int eventcnt;


/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1885f_readout_pl[]="fb_lc1885f_readout";
int ver_proc_fb_lc1885f_readout_pl=2;

/******************************************************************************/

static procprop fb_lc1885f_readout_pl_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1885f_readout_pl()
{
return(&fb_lc1885f_readout_pl_prop);
}

/******************************************************************************/
/*
Readout of Le Croy ADC 1885F
============================

parameters:
-----------
  p[0]: 4 (number of parameters)
  p[1]: module pattern- variable id
  p[2]: module list- variable id
  p[3]: sparsification   0- disable, 1- enable
  p[4]: pedestal values- variable id

outptr:
-------
  ptr[ 0]:               number of read out data words (n_words)

 data words for all modules in IS:
  ptr[1...nr_of_words]:  data words
*/
/******************************************************************************/

plerrcode proc_fb_lc1885f_readout_pl(p)
  /* readout of fastbus lecroy adc1885f using module pattern & list */
int *p;
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int pat;                                /* IS module pattern                  */
int sds_pat;                            /* sparse data scan module pattern    */
int n_mod;                              /* number of modules in sds pattern   */
int *list;                              /* IS module list                     */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int sparse_wc;                          /* word count after sparsification    */
int wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
int *ptr;
int *rptr,*rptr_st;                     /* module data words                  */
int ped_offset;                         /* offset for pedestal list           */
int *pedestals;                        /* pedestal list for all modules in IS */
int data;                               /* adc value from data word           */
int csr;
int ssr;                                /* slave status response              */
int i,j;

T(proc_fb_lc1885f_readout_pl)

ptr= outptr++;  n_words= 0;
pat= var_list[p[1]].var.val;

if ((pat) && (p[3])) {                  /* prepare sparsification             */
  rptr_st= (int*) FB_BUFBEG;
  pedestals= var_list[p[4]].var.ptr;
 }

wt_con= MAX_WT_CON;
list= var_list[p[2]].var.ptr;
n_mod= *list++;
for (i=1;i<=n_mod;i++) {                /* test conversion status             */
  csr= FRCa(*list,1);
  if (sscode()) {
    outptr--;*outptr++=*list;*outptr++=sscode();*outptr++=0;
    return(plErr_HW);
   }
  while ((wt_con--) && (ssr=(FCWWss(csr)&0x7))) {  /* wait for conversion     */
    if (ssr>1) {                        /* test ss code                       */
      FCDISC();
      outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=1;
      return(plErr_HW);
     }
   }
  FCDISC();                             /* disconnect                         */
  if (ssr>1) {                          /* test ss code                       */
    outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=2;
    return(plErr_HW);
   }
  DV(D_DUMP,if (!ssr)  printf("ADC 1885F(%2d): end of conversion after %d/%d\n",*list,(MAX_WT_CON-wt_con),MAX_WT_CON);)
  list++;
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  int unsol_mes[5],unsol_num;
  con_pat= 0;
  list= var_list[p[2]].var.ptr;
  list++;
  for (i=1;i<=n_mod;i++) {              /* test conversion status             */
    csr= FRCa(*list,1);
    if (sscode()) {
      outptr--;*outptr++=*list;*outptr++=sscode();*outptr++=3;
      return(plErr_HW);
     }
    ssr= FCWWss(csr) & 0x7;
    FCDISC();
    if (ssr>1) {                        /* test ss code                       */
      outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=4;
      return(plErr_HW);
     }
    if (!ssr)                           /* conversion completed               */
      con_pat+= 1<<(*list);
    list++;
   }
  unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
  unsol_mes[2]=eventcnt;unsol_mes[3]=LC_ADC_1885F;unsol_mes[4]=con_pat;
  send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
 }
else {                                  /* conversion completed               */
  sds_pat= FRCM(0x9D) & pat;            /* Sparse Data Scan                   */
  DV(D_DUMP,printf("ADC 1885F: mod pattern= %x, sds pattern= %x\n",pat,sds_pat);)
  if (sscode()) {
    outptr--;*outptr++=sscode();*outptr++=5;
    return(plErr_HW);
   }
  list= var_list[p[2]].var.ptr;
  list++;
  if (p[3]) {
    ped_offset= 0;
    for (i=1;i<=n_mod;i++) {
      if ((sds_pat>>(*list))&0x1) {     /* valid data --> block readout       */
        rptr= rptr_st;
        sparse_wc= 0;
        wc= FRDB_S(*list,rptr,MAX_WC_PL) - 1;  /* last data word invalid      */
        if (sscode()!=2) {              /* test ss code                       */
          if (sscode()==0)
           { int unsol_mes[5],unsol_num;
             unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
             unsol_mes[2]=eventcnt;unsol_mes[3]=LC_ADC_1885F;unsol_mes[4]=wc;
             send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
            }
          else {
            *outptr++=sscode();*outptr++=6;
            return(plErr_HW);
           }
         }
        for (j=1;j<=wc;j++) {
          data= *rptr & 0xfff;
          if (*rptr&0x800000)  data= data * 8;
          if (data>pedestals[ped_offset+((*rptr & 0x7f0000)>>16)]) {
            *outptr++= *rptr;
            n_words++;
            sparse_wc++;
           }
          rptr++;
         }        
        DV(D_DUMP,printf("ADC 1885F(%2d): readout completed (%d words)\n",*list,sparse_wc);)
       }
      else {
        ped_offset+=MAX_WC;
       }
      list++;
     }
   }
  else {
    for (i=1;i<=n_mod;i++) {
      if ((sds_pat>>(*list))&0x1) {     /* valid data --> block readout       */
        wc= FRDB_S(*list,outptr,MAX_WC_PL) - 1;  /* last data word invalid    */
        if (sscode()!=2) {              /* test ss code                       */
          if (sscode()==0)
           { int unsol_mes[5],unsol_num;
             unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
             unsol_mes[2]=eventcnt;unsol_mes[3]=LC_ADC_1885F;unsol_mes[4]=wc;
             send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
            }
          else {
            *outptr++=sscode();*outptr++=7;
            return(plErr_HW);
           }
         }
        outptr+=wc;  n_words+= wc;
        DV(D_DUMP,printf("ADC 1885F(%2d): readout completed (%d words)\n",*list,wc);)
       }
      list++;
     }
   }
 }

*ptr= n_words;                          /* number of read out data words      */

return(plOK);

#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1885f_readout_pl(p)
  /* test subroutine for "fb_lc1885f_readout_pl" */
int *p;
{
int pat;                                /* IS module pattern                  */
int *list;                              /* IS module list                     */
int n_mod;                              /* number of modules                  */
int pa, isml_def;
int i,j;

T(test_proc_fb_lc1885f_readout_pl)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

if (p[0]!=4) {                          /* check number of parameters         */
  *outptr++=p[0];
  return(plErr_ArgNum);
 }
                                        /* check parameter values             */
if ((unsigned int)p[1]>MAX_VAR) {
  *outptr++=1;*outptr++=p[1];
  return(plErr_IllVar);
 }
if (!var_list[p[1]].len) {
  *outptr++=1;*outptr++=p[1];
  return(plErr_NoVar);
 }
if (var_list[p[1]].len!=1) {
  *outptr++=1;*outptr++=p[1];
  return(plErr_IllVarSize);
 }

if ((unsigned int)p[2]>MAX_VAR) {
  *outptr++=2;*outptr++=p[2];
  return(plErr_IllVar);
 }
if (!var_list[p[2]].len) {
  *outptr++=2;*outptr++=p[2];
  return(plErr_NoVar);
 }
if (var_list[p[2]].len!=26) {
  *outptr++=2;*outptr++=p[2];
  return(plErr_IllVarSize);
 }

if (((unsigned int)p[3]&0x1)!=(unsigned int)p[3]) {
  *outptr++=3;*outptr++=p[3];
  return(plErr_ArgRange);
 }

if (p[3]) {
  if ((unsigned int)p[4]>MAX_VAR) {
    *outptr++=4;*outptr++=p[4];
    return(plErr_IllVar);
   }
  if (!var_list[p[4]].len) {
    *outptr++=4;*outptr++=p[4];
    return(plErr_NoVar);
   }
  if (var_list[p[4]].len!=(n_mod*MAX_WC)) {
    *outptr++=4;*outptr++=p[4];
    return(plErr_IllVarSize);
   }
 }

pat= var_list[p[1]].var.val; pa= 0;
while (pat) {                            /* check: module defined in IS       */
  if (pat&0x1) {
    j= 1; isml_def= 0;
    do {
      if (pa==memberlist[j]) {
        if (get_mod_type(memberlist[j])!=LC_ADC_1885F) {
          *outptr++=0;*outptr++=pa;
          return(plErr_BadModTyp);
         }
        isml_def++;
       }
      j++;
     } while ((j<=memberlist[0]) && (!isml_def));
    if (!isml_def) {                    /* module not defined in IS           */
      *outptr++=0;*outptr++=pa;
      return(plErr_BadISModList);
     }
   }
  pat>>= 1; pa++;
 }

list= var_list[p[2]].var.ptr;
n_mod= *list++;
for (i=1;i<=n_mod;i++) {                /* check list: module defined in IS   */
  pa= *list++;
  j= 1; isml_def= 0;
  do {
    if (pa==memberlist[j]) {
      if (get_mod_type(memberlist[j])!=LC_ADC_1885F) {
        *outptr++=1;*outptr++=pa;
        return(plErr_BadModTyp);
       }
      isml_def++;
     }
    j++;
   } while ((j<=memberlist[0]) && (!isml_def));
  if (!isml_def) {                      /* module not defined in IS           */
    *outptr++=1;*outptr++=pa;
    return(plErr_BadISModList);
   }
 }

wirbrauchen= (n_mod*MAX_WC) + 1;
return(plOK);
}


/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1885f_readout[]="fb_lc1885f_readout";
int ver_proc_fb_lc1885f_readout=1;

/******************************************************************************/

static procprop fb_lc1885f_readout_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1885f_readout()
{
return(&fb_lc1885f_readout_prop);
}

/******************************************************************************/
/*
Readout of Le Croy ADC 1885F
============================

parameters:
-----------
  p[ 0]:                 4 (number of parameters)
  p[ 1]:                 module pattern            variable id
  p[ 2]:                 sparsification            0- disable, 1- enable
  p[ 3]:                 pedestal values           variable id

outptr:
-------
  ptr[ 0]:               number of read out data words (n_words)

 data words for all modules in IS:
  ptr[1...nr_of_words]:  data words
*/
/******************************************************************************/

plerrcode proc_fb_lc1885f_readout(p)
  /* readout of fastbus lecroy adc1885f */
int *p;
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int pat;                                /* IS module pattern                  */
int sds_pat;                            /* sparse data scan module pattern    */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int sparse_wc;                          /* word count after sparsification    */
int pa;
int wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
int *ptr;
int *rptr,*rptr_st;                     /* module data words                  */
int ped_offset;                         /* offset for pedestal list           */
int *pedestals;                        /* pedestal list for all modules in IS */
int data;                               /* adc value from data word           */
int i;
int csr;
int ssr;                                /* slave status response              */

T(proc_fb_lc1885f_readout)

ptr= outptr++;  n_words= 0;
pat= var_list[p[1]].var.val;

if ((pat) && (p[2])) {                  /* prepare sparsification             */
  rptr_st= (int*) FB_BUFBEG;
  pedestals= var_list[p[3]].var.ptr;
 }

con_pat= 0;  pa= 0;
wt_con= MAX_WT_CON;
while (pat) {                           /* test conversion status             */
  if (pat&0x1) {
    csr= FRCa(pa,1);
    if (sscode()) {
      outptr--;*outptr++=sscode();*outptr++=0;
      return(plErr_HW);
     }
    while ((wt_con--) && (ssr=(FCWWss(csr)&0x7))) {  /* wait for conversion   */
      if (ssr>1) {                      /* test ss code                       */
        FCDISC();
        outptr--;*outptr++=pa;*outptr++=ssr;*outptr++=1;
        return(plErr_HW);
       }
     }
    FCDISC();                           /* disconnect                         */
    if (ssr>1) {                        /* test ss code                       */
      outptr--;*outptr++=pa;*outptr++=ssr;*outptr++=2;
      return(plErr_HW);
     }
    if (!ssr) {                         /* conversion completed               */
      con_pat+= 1 << pa;
      D(D_DUMP,printf("ADC 1885F(%2d): end of conversion after %d/%d\n",pa,(MAX_WT_CON-wt_con),MAX_WT_CON);)
     }
   }
  pat>>= 1; pa++;
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  int unsol_mes[5],unsol_num;
  unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
  unsol_mes[2]=eventcnt;unsol_mes[3]=LC_ADC_1885F;unsol_mes[4]=con_pat;
  send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
 }
else {                                  /* conversion completed               */
  pat= var_list[p[1]].var.val;
  sds_pat= FRCM(0x9D) & pat;            /* Sparse Data Scan                   */
  if (sscode()) {
    outptr--;*outptr++=sscode();*outptr++=3;
    return(plErr_HW);
   }
  pa= 0;
  if (p[2]) {
    ped_offset= 0;
    while (sds_pat) {                   /* valid data                         */
      if (sds_pat&0x1) {          /* block readout for geographic address "pa"*/
        rptr= rptr_st;
        sparse_wc= 0;
        wc= FRDB_S(pa,rptr,MAX_WC_PL) - 1;  /* last data word invalid         */
        if (sscode() != 2) {            /* test ss code                       */
          if (sscode() == 0)
           { int unsol_mes[5],unsol_num;
             unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
             unsol_mes[2]=eventcnt;unsol_mes[3]=LC_ADC_1885F;unsol_mes[4]=wc;
             send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
            }
          else {
            *outptr++=sscode();*outptr++=4;
            return(plErr_HW);
           }
         }
        for (i=1; i<=wc; i++) {
          data= *rptr & 0xfff;
          if (*rptr&0x800000)  data= data * 8;
          if (data>pedestals[ped_offset+((*rptr & 0x7f0000)>>16)]) {
            *outptr++= *rptr;
            n_words++;
            sparse_wc++;
           }
          rptr++;
         }        
        DV(D_DUMP,printf("ADC 1885F(%2d): readout completed (%d words)\n",pa,sparse_wc);)
       }
      else {
        if (pat&0x1)  ped_offset+=MAX_WC;
       }
      pat>>= 1; sds_pat>>= 1; pa++;
     }
   }
  else {
    while (sds_pat) {                   /* valid data                         */
      if (sds_pat&0x1) {          /* block readout for geographic address "pa"*/
        wc= FRDB_S(pa,outptr,MAX_WC_PL) - 1;  /* last data word invalid       */
        if (sscode() != 2) {            /* test ss code                       */
          if (sscode() == 0)
           { int unsol_mes[5],unsol_num;
             unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
             unsol_mes[2]=eventcnt;unsol_mes[3]=LC_ADC_1885F;unsol_mes[4]=wc;
             send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
            }
          else {
            *outptr++=sscode();*outptr++=5;
            return(plErr_HW);
           }
         }
        outptr+=wc;  n_words+= wc;
        DV(D_DUMP,printf("ADC 1885F(%2d): readout completed (%d words)\n",pa,wc);)
       }
      sds_pat>>= 1; pa++;
     }
   }
 }

*ptr= n_words;                          /* number of read out data words      */

return(plOK);

#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1885f_readout(p)
  /* test subroutine for "fb_lc1885f_readout" */
int *p;
{
int pat;                                /* IS module pattern                  */
int n_mod;                              /* number of modules                  */
int pa, isml_def;
int i;

T(test_proc_fb_lc1885f_readout)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

if (p[0]!=3) {                          /* check number of parameters         */
  *outptr++=p[0];
  return(plErr_ArgNum);
 }
                                        /* check parameter values             */
if ((unsigned int)p[1]>MAX_VAR) {
  *outptr++=1;*outptr++=p[1];
  return(plErr_IllVar);
 }
if (!var_list[p[1]].len) {
  *outptr++=1;*outptr++=p[1];
  return(plErr_NoVar);
 }
if (var_list[p[1]].len!=1) {
  *outptr++=1;*outptr++=p[1];
  return(plErr_IllVarSize);
 }

pat= var_list[p[1]].var.val; n_mod= 0; pa= 0;
while (pat) {                            /* check: module defined in IS       */
  if (pat&0x1) {
    i= 1; isml_def= 0;
    do {
      if (pa==memberlist[i]) {
        if (get_mod_type(memberlist[i]) != LC_ADC_1885F) {
          *outptr++=pa;
          return(plErr_BadModTyp);
         }
        isml_def++;
       }
      i++;
     } while ((i<=memberlist[0]) && (!isml_def));
    if (!isml_def) {                    /* module not defined in IS           */
      *outptr++=pa;
      return(plErr_BadISModList);
     }
    n_mod++;
   }
  pat>>= 1; pa++;
 }

if (((unsigned int)p[2]&0x1)!=(unsigned int)p[2]) {
  *outptr++=p[2];
  return(plErr_ArgRange);
 }

if (p[2]) {
  if ((unsigned int)p[3]>MAX_VAR) {
    *outptr++=3;*outptr++=p[3];
    return(plErr_IllVar);
   }
  if (!var_list[p[3]].len) {
    *outptr++=3;*outptr++=p[3];
    return(plErr_NoVar);
   }
  if (var_list[p[3]].len!=(n_mod*MAX_WC)) {
    *outptr++=3;*outptr++=p[3];
    return(plErr_IllVarSize);
   }
 }

wirbrauchen= (n_mod*MAX_WC) + 1;
return(plOK);
}


/******************************************************************************/
/******************************************************************************/
