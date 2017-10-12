/*******************************************************************************
*                                                                              *
* fb_lc1879_read.c                                                             *
*                                                                              *
* OS9                                                                          *
*                                                                              *
* MaWo                                                                         *
* PeWue                                                                        *
* MiZi                                                                         *
*                                                                              *
* 28.12.94                                                                     *
* 30.12.94  #undef for local constants                                         *
*  3. 1.95  max. number of words reduced (1536 --> 768) to 8 hits/channel      *
*  6. 1.95  "wirbrauchen" for fb_lc1879_readout changed                        *
* 29. 9.95  max. number of words increased to 16 hits/channel                  *
* 24. 2.97  "fb_lc1879_readout_pl" added                                       *
*           new standard readout version: "fb_lc1879_readout_pl"               *
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

#define MAX_WC  768                   /* max number of words for single event */
#define MAX_WC_PL  769         /* max number of words for single event plus 1 */

extern int *outptr;
extern int *memberlist;
extern int wirbrauchen;
extern int eventcnt;


/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1879_readout_t2[]="fb_lc1879_readout_test";
int ver_proc_fb_lc1879_readout_t2=2;

/******************************************************************************/

static procprop fb_lc1879_readout_t2_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1879_readout_t2()
{
return(&fb_lc1879_readout_t2_prop);
}

/******************************************************************************/
/* 
Readout of LeCroy TDC 1879
==========================

parameters:
-----------
  p[0] : 3 (number of parameters)
  p[1] : module pattern- variable id
  p[2] : module list- variable id
  p[3] : edge detection- 1 --> only leading edge detection
                         2 --> only trailing edge detection
                         3 --> leading & trailing edge detection

outptr:
-------
  ptr[ 0]:               number of read out data words (n_words)

 data words for all modules in IS:
  ptr[1...n_words]:      leading/trailing edge data words
*/
/******************************************************************************/

plerrcode proc_fb_lc1879_readout_t2(p)
  /* test of conversion for fastbus lecroy tdc1879 using module pattern & list */
int *p;
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int pat;                                /* IS module pattern                  */
int sds_pat;                            /* sparse data scan module pattern    */
int n_mod;                              /* number of modules in sds pattern   */
int *list;                              /* IS module list                     */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int wc_max;
int wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
int *ptr;
int ssr;                                /* slave status response              */
int edge;                               /* leading/trailing edge data         */
int i;

T(proc_fb_lc1879_readout_t2)

ptr= outptr++;  n_words= 0;

wt_con= MAX_WT_CON;
list= var_list[p[2]].var.ptr;
n_mod= *list++;
for (i=1;i<=n_mod;i++) {                /* test conversion status             */
  FCPD(*list);                          /* primary address cycle              */
  while ((wt_con--) && (ssr=(FCWSAss(0)&0x7))) {  /* wait for conversion      */
    if (ssr>1) {                        /* test ss code                       */
      FCDISC();
      outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=0;
      return(plErr_HW);
     }
   }
  FCDISC();                             /* disconnect                         */
  if (ssr>1) {                          /* test ss code                       */
    outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=1;
    return(plErr_HW);
   }
  DV(D_DUMP,if (!ssr)  printf("TDC 1879(%2d): end of conversion after %d/%d\n",*list,(MAX_WT_CON-wt_con),MAX_WT_CON);)
  list++;
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  int unsol_mes[5],unsol_num;
  con_pat= 0;
  list= var_list[p[2]].var.ptr;
  list++;
  for (i=1;i<=n_mod;i++) {              /* test conversion status             */
    FCPD(*list);                        /* primary address cycle              */
    ssr= FCWSAss(0) & 0x7;
    FCDISC();
    if (ssr>1) {                        /* test ss code                       */
      outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=2;
      return(plErr_HW);
     }
    if (!ssr)                           /* conversion completed               */
      con_pat+= 1<<(*list);
    list++;
   }
  unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
  unsol_mes[2]=eventcnt;unsol_mes[3]=LC_TDC_1879;unsol_mes[4]=con_pat;
  send_unsolicited(Unsol_Data,unsol_mes,unsol_num);
 }

*ptr= 0;                                /* number of read out data words      */

return(plOK);

#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1879_readout_t2(p)
  /* test subroutine for "fb_lc1879_readout_t2" */
int *p;
{
int pat;                                /* IS module pattern                  */
int *list;                              /* IS module list                     */
int n_mod;                              /* number of modules                  */
int pa, isml_def;
int i,j;

T(test_proc_fb_lc1879_readout_t2)

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

if (((unsigned int)p[3]&0x3)!=(unsigned int)p[3]) {
  *outptr++=p[3];
  return(plErr_ArgRange);
 }

pat= var_list[p[1]].var.val; pa= 0;
while (pat) {                           /* check patt: IS module definition   */
  if (pat&0x1) {
    j= 1; isml_def= 0;
    do {
      if (pa==memberlist[j]) {
        if (get_mod_type(memberlist[j])!=LC_TDC_1879) {
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
      if (get_mod_type(memberlist[j])!=LC_TDC_1879) {
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
if ((unsigned int)p[3]>2) 
  wirbrauchen+= n_mod*MAX_WC;

return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1879_readout_t1[]="fb_lc1879_readout_test";
int ver_proc_fb_lc1879_readout_t1=1;

/******************************************************************************/

static procprop fb_lc1879_readout_t1_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1879_readout_t1()
{
return(&fb_lc1879_readout_t1_prop);
}

/******************************************************************************/
/* 
Readout of LeCroy TDC 1879
==========================

parameters:
-----------
  p[0] : 3 (number of parameters)
  p[1] : module pattern- variable id
  p[2] : module list- variable id
  p[3] : edge detection- 1 --> only leading edge detection
                         2 --> only trailing edge detection
                         3 --> leading & trailing edge detection

outptr:
-------
  ptr[ 0]:               number of read out data words (n_words)

 data words for all modules in IS:
  ptr[1...n_words]:      leading/trailing edge data words
*/
/******************************************************************************/

plerrcode proc_fb_lc1879_readout_t1(p)
  /* readout of fastbus lecroy tdc1879 using module pattern & list */
int *p;
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int pat;                                /* IS module pattern                  */
int sds_pat;                            /* sparse data scan module pattern    */
int n_mod;                              /* number of modules in sds pattern   */
int *list;                              /* IS module list                     */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int wc_max;
int wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
int *ptr;
int ssr;                                /* slave status response              */
int edge;                               /* leading/trailing edge data         */
int i;

T(proc_fb_lc1879_readout_t1)

ptr= outptr++;  n_words= 0;

wt_con= MAX_WT_CON;
list= var_list[p[2]].var.ptr;
n_mod= *list++;
for (i=1;i<=n_mod;i++) {                /* test conversion status             */
  FCPD(*list);                          /* primary address cycle              */
  while ((wt_con--) && (ssr=(FCWSAss(0)&0x7))) {  /* wait for conversion      */
    if (ssr>1) {                        /* test ss code                       */
      FCDISC();
      outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=0;
      return(plErr_HW);
     }
   }
  FCDISC();                             /* disconnect                         */
  if (ssr>1) {                          /* test ss code                       */
    outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=1;
    return(plErr_HW);
   }
  DV(D_DUMP,if (!ssr)  printf("TDC 1879(%2d): end of conversion after %d/%d\n",*list,(MAX_WT_CON-wt_con),MAX_WT_CON);)
  list++;
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  int unsol_mes[5],unsol_num;
  con_pat= 0;
  list= var_list[p[2]].var.ptr;
  list++;
  for (i=1;i<=n_mod;i++) {              /* test conversion status             */
    FCPD(*list);                        /* primary address cycle              */
    ssr= FCWSAss(0) & 0x7;
    FCDISC();
    if (ssr>1) {                        /* test ss code                       */
      outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=2;
      return(plErr_HW);
     }
    if (!ssr)                           /* conversion completed               */
      con_pat+= 1<<(*list);
    list++;
   }
  unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
  unsol_mes[2]=eventcnt;unsol_mes[3]=LC_TDC_1879;unsol_mes[4]=con_pat;
  send_unsolicited(Unsol_Data,unsol_mes,unsol_num);
 }
else {                                  /* conversion completed               */
  pat= var_list[p[1]].var.val;
  edge= p[3];
  if (edge&0x3)
    wc_max= MAX_WC_PL;
  else
    wc_max= 2*MAX_WC + 1;
  sds_pat= FRCM(0xAD) & pat;            /* Sparse Data Scan                   */
  if (sscode()) {
    outptr--;*outptr++=sscode();*outptr++=3;
    return(plErr_HW);
   }
  list= var_list[p[2]].var.ptr;
  list++;
  for (i=1;i<=n_mod;i++) {
    if ((sds_pat>>(*list))&0x1) {       /* valid data --> block readout       */
      if (edge&0x1) {
        wc= 0;
        outptr+=wc;  n_words+=wc;
       }
      if (edge&0x2) {
        wc= 0;
        outptr+=wc;  n_words+= wc;
       }
      DV(D_DUMP,printf("TDC 1879(%2d): readout completed (%d words)\n",*list,wc);)
     }
    list++;
   }
 }

*ptr= n_words;                          /* number of read out data words      */

return(plOK);

#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1879_readout_t1(p)
  /* test subroutine for "fb_lc1879_readout_t1" */
int *p;
{
int pat;                                /* IS module pattern                  */
int *list;                              /* IS module list                     */
int n_mod;                              /* number of modules                  */
int pa, isml_def;
int i,j;

T(test_proc_fb_lc1879_readout_t1)

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

if (((unsigned int)p[3]&0x3)!=(unsigned int)p[3]) {
  *outptr++=p[3];
  return(plErr_ArgRange);
 }

pat= var_list[p[1]].var.val; pa= 0;
while (pat) {                           /* check patt: IS module definition   */
  if (pat&0x1) {
    j= 1; isml_def= 0;
    do {
      if (pa==memberlist[j]) {
        if (get_mod_type(memberlist[j])!=LC_TDC_1879) {
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
      if (get_mod_type(memberlist[j])!=LC_TDC_1879) {
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
if ((unsigned int)p[3]>2) 
  wirbrauchen+= n_mod*MAX_WC;

return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1879_readout_pl[]="fb_lc1879_readout";
int ver_proc_fb_lc1879_readout_pl=2;

/******************************************************************************/

static procprop fb_lc1879_readout_pl_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1879_readout_pl()
{
return(&fb_lc1879_readout_pl_prop);
}

/******************************************************************************/
/* 
Readout of LeCroy TDC 1879
==========================

parameters:
-----------
  p[0] : 3 (number of parameters)
  p[1] : module pattern- variable id
  p[2] : module list- variable id
  p[3] : edge detection- 1 --> only leading edge detection
                         2 --> only trailing edge detection
                         3 --> leading & trailing edge detection

outptr:
-------
  ptr[ 0]:               number of read out data words (n_words)

 data words for all modules in IS:
  ptr[1...n_words]:      leading/trailing edge data words
*/
/******************************************************************************/

plerrcode proc_fb_lc1879_readout_pl(p)
  /* readout of fastbus lecroy tdc1879 using module pattern & list */
int *p;
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int pat;                                /* IS module pattern                  */
int sds_pat;                            /* sparse data scan module pattern    */
int n_mod;                              /* number of modules in sds pattern   */
int *list;                              /* IS module list                     */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int wc_max;
int wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
int *ptr;
int ssr;                                /* slave status response              */
int edge;                               /* leading/trailing edge data         */
int i;

T(proc_fb_lc1879_readout_pl)

ptr= outptr++;  n_words= 0;

wt_con= MAX_WT_CON;
list= var_list[p[2]].var.ptr;
n_mod= *list++;
for (i=1;i<=n_mod;i++) {                /* test conversion status             */
  FCPD(*list);                          /* primary address cycle              */
  while ((wt_con--) && (ssr=(FCWSAss(0)&0x7))) {  /* wait for conversion      */
    if (ssr>1) {                        /* test ss code                       */
      FCDISC();
      outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=0;
      return(plErr_HW);
     }
   }
  FCDISC();                             /* disconnect                         */
  if (ssr>1) {                          /* test ss code                       */
    outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=1;
    return(plErr_HW);
   }
  DV(D_DUMP,if (!ssr)  printf("TDC 1879(%2d): end of conversion after %d/%d\n",*list,(MAX_WT_CON-wt_con),MAX_WT_CON);)
  list++;
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  int unsol_mes[5],unsol_num;
  con_pat= 0;
  list= var_list[p[2]].var.ptr;
  list++;
  for (i=1;i<=n_mod;i++) {              /* test conversion status             */
    FCPD(*list);                        /* primary address cycle              */
    ssr= FCWSAss(0) & 0x7;
    FCDISC();
    if (ssr>1) {                        /* test ss code                       */
      outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=2;
      return(plErr_HW);
     }
    if (!ssr)                           /* conversion completed               */
      con_pat+= 1<<(*list);
    list++;
   }
  unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
  unsol_mes[2]=eventcnt;unsol_mes[3]=LC_TDC_1879;unsol_mes[4]=con_pat;
  send_unsolicited(Unsol_Data,unsol_mes,unsol_num);
 }
else {                                  /* conversion completed               */
  pat= var_list[p[1]].var.val;
  edge= p[3];
  if (edge&0x3)
    wc_max= MAX_WC_PL;
  else
    wc_max= 2*MAX_WC + 1;
  sds_pat= FRCM(0xAD) & pat;            /* Sparse Data Scan                   */
  if (sscode()) {
    outptr--;*outptr++=sscode();*outptr++=3;
    return(plErr_HW);
   }
  list= var_list[p[2]].var.ptr;
  list++;
  for (i=1;i<=n_mod;i++) {
    if ((sds_pat>>(*list))&0x1) {       /* valid data --> block readout       */
      if (edge&0x1) {
        wc= FRDB(*list,0,outptr,wc_max) - 1;  /* last data word invalid       */
        if (sscode() != 2) {            /* test ss code                       */
          if (sscode() == 0)
           { int unsol_mes[5],unsol_num;
             unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
             unsol_mes[2]=eventcnt;unsol_mes[3]=LC_TDC_1879;unsol_mes[4]=wc;
             send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
            }
          else {
            *outptr++=sscode();*outptr++=4;
            return(plErr_HW);
           }
         }
        outptr+=wc;  n_words+=wc;
       }
      if (edge&0x2) {
        wc= FRDB(*list,1,outptr,wc_max) - 1;  /* last data word invalid       */
        if (sscode() != 2) {            /* test ss code                       */
          if (sscode() == 0)
           { int unsol_mes[5],unsol_num;
             unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
             unsol_mes[2]=eventcnt;unsol_mes[3]=LC_TDC_1879;unsol_mes[4]=wc;
             send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
            }
          else {
            *outptr++=sscode();*outptr++=5;
            return(plErr_HW);
           }
         }
        outptr+=wc;  n_words+= wc;
       }
      DV(D_DUMP,printf("TDC 1879(%2d): readout completed (%d words)\n",*list,wc);)
     }
    list++;
   }
 }

*ptr= n_words;                          /* number of read out data words      */

return(plOK);

#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1879_readout_pl(p)
  /* test subroutine for "fb_lc1879_readout_pl" */
int *p;
{
int pat;                                /* IS module pattern                  */
int *list;                              /* IS module list                     */
int n_mod;                              /* number of modules                  */
int pa, isml_def;
int i,j;

T(test_proc_fb_lc1879_readout_pl)

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

if (((unsigned int)p[3]&0x3)!=(unsigned int)p[3]) {
  *outptr++=p[3];
  return(plErr_ArgRange);
 }

pat= var_list[p[1]].var.val; pa= 0;
while (pat) {                           /* check patt: IS module definition   */
  if (pat&0x1) {
    j= 1; isml_def= 0;
    do {
      if (pa==memberlist[j]) {
        if (get_mod_type(memberlist[j])!=LC_TDC_1879) {
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
      if (get_mod_type(memberlist[j])!=LC_TDC_1879) {
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
if ((unsigned int)p[3]>2) 
  wirbrauchen+= n_mod*MAX_WC;

return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1879_readout[]="fb_lc1879_readout";
int ver_proc_fb_lc1879_readout=1;

/******************************************************************************/

static procprop fb_lc1879_readout_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1879_readout()
{
return(&fb_lc1879_readout_prop);
}

/******************************************************************************/
/* 
Readout of LeCroy TDC 1879
==========================

parameters:
-----------
  p[0] : 2 (number of parameters)
  p[1] : module pattern- variable id
  p[2] : edge detection- 1 --> only leading edge detection
                         2 --> only trailing edge detection
                         3 --> leading & trailing edge detection

outptr:
-------
  ptr[ 0]:               number of read out data words (n_words)

 data words for all modules in IS:
  ptr[1...n_words]:      leading/trailing edge data words
*/
/******************************************************************************/

plerrcode proc_fb_lc1879_readout(p)
  /* readout of fastbus lecroy tdc1879 */
int *p;
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int pat;                                /* IS module pattern                  */
int sds_pat;                            /* sparse data scan module pattern    */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int wc_max;
int pa;
int wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
int *ptr;
int ssr;                                /* slave status response              */
int edge;

T(proc_fb_lc1879_readout)

ptr= outptr++;  n_words= 0;

pat= var_list[p[1]].var.val;  con_pat= 0;  pa= 0;
edge= p[2];
if (edge&0x3) {
  wc_max= MAX_WC_PL;
 }
else {
  wc_max= 2*MAX_WC + 1;
 }
wt_con= MAX_WT_CON;
while (pat) {                           /* test conversion status             */
  if (pat&0x1) {
    FCPD(pa);                           /* primary address cycle              */
    while ((wt_con--) && (ssr=(FCWSAss(0)&0x7))) {  /* wait for conversion    */
      if (ssr>1) {                      /* test ss code                       */
        FCDISC();
        outptr--;*outptr++=pa;*outptr++=ssr;*outptr++=0;
        return(plErr_HW);
       }
     }
    FCDISC();                           /* disconnect                         */
    if (ssr>1) {                        /* test ss code                       */
      outptr--;*outptr++=pa;*outptr++=ssr;*outptr++=1;
      return(plErr_HW);
     }
    if (!ssr) {                         /* conversion completed               */
      con_pat+= 1 << pa;
      DV(D_DUMP,printf("TDC 1879(%2d): end of conversion after %d/%d\n",pa,(MAX_WT_CON-wt_con),MAX_WT_CON);)
     }
   }
  pat>>= 1; pa++;
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  int unsol_mes[5],unsol_num;
  unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
  unsol_mes[2]=eventcnt;unsol_mes[3]=LC_TDC_1879;unsol_mes[4]=con_pat;
  send_unsolicited(Unsol_Data,unsol_mes,unsol_num);
 }
else {                                  /* conversion completed               */
  pat= var_list[p[1]].var.val;
  sds_pat= FRCM(0xAD) & pat;            /* Sparse Data Scan                   */
  if (sscode()) {
    outptr--;*outptr++=sscode();*outptr++=2;
    return(plErr_HW);
   }
  pa= 0;
  while (sds_pat) {                     /* valid data                         */
    if (sds_pat&0x1) {            /* block readout for geographic address "pa"*/
      if (edge&0x1) {
        wc= FRDB(pa,0,outptr,wc_max) - 1;  /* last data word invalid          */
        if (sscode() != 2) {            /* test ss code                       */
          if (sscode() == 0)
           { int unsol_mes[5],unsol_num;
             unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
             unsol_mes[2]=eventcnt;unsol_mes[3]=LC_TDC_1879;unsol_mes[4]=wc;
             send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
            }
          else {
            *outptr++=sscode();*outptr++=3;
            return(plErr_HW);
           }
         }
        outptr+=wc;  n_words+=wc;
       }
      if (edge&0x2) {
        wc= FRDB(pa,1,outptr,wc_max) - 1;  /* last data word invalid          */
        if (sscode() != 2) {            /* test ss code                       */
          if (sscode() == 0)
           { int unsol_mes[5],unsol_num;
             unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
             unsol_mes[2]=eventcnt;unsol_mes[3]=LC_TDC_1879;unsol_mes[4]=wc;
             send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
            }
          else {
            *outptr++=sscode();*outptr++=4;
            return(plErr_HW);
           }
         }
        outptr+=wc;  n_words+= wc;
       }
      DV(D_DUMP,printf("TDC 1879(%2d): readout completed (%d words)\n",pa,wc);)
     }
    sds_pat>>= 1; pa++;
   }
 }

*ptr= n_words;                          /* number of read out data words      */

return(plOK);

#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1879_readout(p)
  /* test subroutine for "fb_lc1879_readout" */
int *p;
{
int pat;                                /* IS module pattern                  */
int n_mod;                              /* number of modules                  */
int pa, isml_def;
int i;

T(test_proc_fb_lc1879_readout)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

if (p[0]!=2) {                          /* check number of parameters         */
  *outptr++=p[0];
  return(plErr_ArgNum);
 }
                                        /* check parameter values             */
if ((unsigned int)p[1]>MAX_VAR) {
  *outptr++=p[1];
  return(plErr_IllVar);
 }
if (!var_list[p[1]].len) {
  *outptr++=p[1];
  return(plErr_NoVar);
 }
if (var_list[p[1]].len!=1) {
  *outptr++=p[1];
  return(plErr_IllVarSize);
 }

if (((unsigned int)p[2]&0x3)!=(unsigned int)p[2]) {
  *outptr++=p[2];
  return(plErr_ArgRange);
 }

pat= var_list[p[1]].var.val; n_mod= 0; pa= 0;
while (pat) {                            /* check: module defined in IS       */
  if (pat&0x1) {
    i= 1; isml_def= 0;
    do {
      if (pa==memberlist[i]) {
        if (get_mod_type(memberlist[i]) != LC_TDC_1879) {
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

wirbrauchen= (n_mod*MAX_WC) + 1;
if ((unsigned int)p[2]>2) 
  wirbrauchen+= n_mod*MAX_WC;

return(plOK);
}

/******************************************************************************/
/******************************************************************************/
