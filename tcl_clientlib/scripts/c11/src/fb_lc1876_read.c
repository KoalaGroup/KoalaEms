/*******************************************************************************
*                                                                              *
* fb_lc1876_read.c                                                             *
*                                                                              *
* OS9                                                                          *
*                                                                              *
* MaWo                                                                         *
* PeWue                                                                        *
*                                                                              *
* 27.12.94                                                                     *
* 30.12.94  #undef for local constants                                         *
*  6. 1.95  procprop for fb_lc1876_check_buf corrected                         *
* 18. 1.95  "end of conversion"-test modified                                  *
*  1. 2.95  Unsol_Data replaced by Unsol_Warning                               *
* 21.03.95  new standard readout version: "fb_lc1876_readout_check"            *
* 24. 2.97  "fb_lc1876_readout_pl" added                                       *
*           new standard readout version: "fb_lc1876_readout_pl"               *
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

#define MAX_WC  1537                  /* max number of words for single event */
#define MAX_WC_PL  1538        /* max number of words for single event plus 1 */

extern int *outptr;
extern int *memberlist;
extern int wirbrauchen;
extern int eventcnt;


/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1876_check_buf[]="fb_lc1876_check_buf";
int ver_proc_fb_lc1876_check_buf=1;

/******************************************************************************/

static procprop fb_lc1876_check_buf_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1876_check_buf()
{
return(&fb_lc1876_check_buf_prop);
}

/******************************************************************************/
/*
Buffer Check of LeCroy TDC 1876
===============================

parameters:
-----------
  p[ 0] : 1 (number of parameters)
  p[ 1] : module pattern- variable id
*/
/******************************************************************************/

plerrcode proc_fb_lc1876_check_buf(p)
  /* check correct read and write pointer positions in lecroy tdc1876 buffer */
  /* memory */
int *p;
{
int pat;

T(proc_fb_lc1876_check_buf)

pat= var_list[p[1]].var.val;

return(lc1876_buf(pat,1,eventcnt));
}

/******************************************************************************/

plerrcode test_proc_fb_lc1876_check_buf(p)
  /* test subroutine for "fb_lc1876_check_buf" */
int *p;
{
int pat;                                /* IS module pattern                  */
int pa, isml_def;
int i;

T(test_proc_fb_lc1876_check_buf)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

if (p[0]!=1) {                          /* check number of parameters         */
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

pat= var_list[p[1]].var.val; pa= 0;
while (pat) {                            /* check: module defined in IS       */
  if (pat&0x1) {
    i= 1; isml_def= 0;
    do {
      if (pa==memberlist[i]) {
        if (get_mod_type(memberlist[i]) != LC_TDC_1876) {
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
   }
  pat>>= 1; pa++;
 }

wirbrauchen= 3;
return(plOK);
}


/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1876_read_ptr[]="fb_lc1876_read_ptr";
int ver_proc_fb_lc1876_read_ptr=1;

/******************************************************************************/

static procprop fb_lc1876_read_ptr_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1876_read_ptr()
{
return(&fb_lc1876_read_ptr_prop);
}

/******************************************************************************/
/*
Readout of LeCroy TDC 1876 Buffer Memory Pointers
=================================================

parameters:
-----------
  p[0] : 1 (number of parameters)
  p[1] : module pattern- variable id

outptr:
-------
  ptr[0]:                number of read out data words (n_words)

 structure for each read out module (start: n=1):
  ptr[n]:                write pointer position
  ptr[n+1]:              read pointer position
*/
/******************************************************************************/

plerrcode proc_fb_lc1876_read_ptr(p)
  /* read read/write pointer positions in lecroy tdc1876 buffer memory        */
int *p;
{
int n_words;                     /* number of read out words from all modules */
int pat,pa;
int val;
int *ptr;

T(proc_fb_lc1876_read_ptr)

ptr= outptr++;  n_words= 0;

pat= var_list[p[1]].var.val;

pa= 0;
while (pat) {
  if (pat&0x1) {
    val= FRC(pa,16);
    if (sscode()) {
      *outptr++=pa;*outptr++=sscode();*outptr++=0;
      return(plErr_HW);
     }
    *outptr++= val&0x7; *outptr++= (val>>8)&0x7;
    n_words+= 2;
    DV(D_DUMP,printf("TDC 1876(%2d): pointer positions read out\n",pa);)
   }
  pat>>= 1; pa++;
 }

*ptr= n_words;                          /* number of read out data words      */

return(plOK);
}

/******************************************************************************/

plerrcode test_proc_fb_lc1876_read_ptr(p)
  /* test subroutine for "fb_lc1876_read_ptr" */
int *p;
{
int pat;                                /* IS module pattern                  */
int n_mod;                              /* number of modules                  */
int pa, isml_def;
int i;

T(test_proc_fb_lc1876_read_ptr)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

if (p[0]!=1) {                          /* check number of parameters         */
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

pat= var_list[p[1]].var.val; n_mod= 0; pa= 0;
while (pat) {                            /* check: module defined in IS       */
  if (pat&0x1) {
    i= 1; isml_def= 0;
    do {
      if (pa==memberlist[i]) {
        if (get_mod_type(memberlist[i]) != LC_TDC_1876) {
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

wirbrauchen= (n_mod*2) + 1;
return(plOK);
}


/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1876_readout_pl[]="fb_lc1876_readout";
int ver_proc_fb_lc1876_readout_pl=3;

/******************************************************************************/

static procprop fb_lc1876_readout_pl_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1876_readout_pl()
{
return(&fb_lc1876_readout_pl_prop);
}

/******************************************************************************/
/* 
Readout of Le Croy TDC 1876
===========================

parameters:
-----------
  p[ 0]:                 2 (number of parameters)
  p[ 1]:                 module pattern- variable id
  p[ 2]:                 module list- variable id
  p[ 3]:                 broadcast class

outptr:
-------
  ptr[ 0]:               number of read out data words (n_words)

 data words for all modules in IS:
  ptr[1...n_words]:      header & data words

 structure for each read out module (start: n=1):
  ptr[ n]:               header word (containing word count wc)
  ptr[(n+1)...(n+wc)]:   data words
*/
/******************************************************************************/

plerrcode proc_fb_lc1876_readout_pl(p)
  /* readout of fastbus lecroy tdc1876 using module pattern & list */
int *p;
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int pat;                                /* IS module pattern                  */
int sds_pat;                            /* sparse data scan module pattern    */
int n_mod;                              /* number of modules in module list   */
int *list;                              /* IS module list                     */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
int *ptr;
int i;

T(proc_fb_lc1876_readout_pl)

ptr= outptr++;  n_words= 0;

pat= var_list[p[1]].var.val;
wt_con= MAX_WT_CON;
if ((FRCM(0x9)&pat)^pat) {             /* wait for end of conversion         */
  while ((wt_con--) && ((FRCM(0x9)&pat)^pat)) {
    if (sscode()) {
      outptr--;*outptr++=sscode();*outptr++=0;
      return(plErr_HW);
     }
   }
 }
if (sscode()) {
  outptr--;*outptr++=sscode();*outptr++=1;
  return(plErr_HW);
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  con_pat= FRCM(0x9) & pat;
  if (sscode()) {
    outptr--;*outptr++=sscode();*outptr++=2;
    return(plErr_HW);
   }
  { int unsol_mes[5],unsol_num;
    unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
    unsol_mes[2]=eventcnt;unsol_mes[3]=LC_TDC_1876;unsol_mes[4]=con_pat;
    send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
   }
 }
else {                                 /* conversion completed               */
  DV(D_DUMP,printf("TDC 1876: end of conversion after %d/%d\n",(MAX_WT_CON-wt_con),MAX_WT_CON);)
  FWCM((0x5 | (p[3]<<4)),0x400);        /* LNE for broadcast class p[3]       */
  if (sscode()) {
    outptr--;*outptr++=sscode();*outptr++=3;
    return(plErr_HW);
   }
  sds_pat= FRCM(0xCD) & pat;            /* Sparse Data Scan                   */
  if (sscode()) {
    outptr--;*outptr++=sscode();*outptr++=4;
    return(plErr_HW);
   }
  list= var_list[p[2]].var.ptr;
  n_mod= *list++;
  for (i=1;i<=n_mod;i++) {
    if ((sds_pat>>(*list))&0x1) {       /* valid data --> block readout       */
      wc= FRDB_S(*list,outptr,MAX_WC_PL) - 1;  /* last data word invalid      */
      if (sscode() != 2) {              /* test ss code                       */
        if (sscode() == 0)
         { int unsol_mes[5],unsol_num;
           unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
           unsol_mes[2]=eventcnt;unsol_mes[3]=LC_TDC_1876;unsol_mes[4]=wc;
           send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
          }
        else {
          *outptr++=sscode();*outptr++=5;
          return(plErr_HW);
         }
       }
      outptr+=wc;  n_words+= wc;
      DV(D_DUMP,printf("TDC 1876(%2d): readout completed (%d words)\n",*list,wc);)
     }
    list++;
   }
 }

*ptr= n_words;                          /* number of read out data words      */

return(plOK);

#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1876_readout_pl(p)
  /* test subroutine for "fb_lc1876_readout_pl" */
int *p;
{
int pat;                                /* IS module pattern                  */
int *list;                              /* IS module list                     */
int n_mod;                              /* number of modules                  */
int pa, isml_def;
int i,j;

T(test_proc_fb_lc1876_readout_pl)

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
 *outptr++=1; *outptr++=p[1];
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

if ((unsigned int)p[3]>7) {
  *outptr++=3;*outptr++=p[3];
  return(plErr_ArgRange);
 }

pat= var_list[p[1]].var.val; pa= 0;
while (pat) {                           /* check patt: IS module definition   */
  if (pat&0x1) {
    j= 1; isml_def= 0;
    do {
      if (pa==memberlist[j]) {
        if (get_mod_type(memberlist[j])!=LC_TDC_1876) {
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
      if (get_mod_type(memberlist[j])!=LC_TDC_1876) {
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

char name_proc_fb_lc1876_readout_check[]="fb_lc1876_readout";
int ver_proc_fb_lc1876_readout_check=2;

/******************************************************************************/

static procprop fb_lc1876_readout_check_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1876_readout_check()
{
return(&fb_lc1876_readout_check_prop);
}

/******************************************************************************/
/* 
Readout of Le Croy TDC 1876
===========================

parameters:
-----------
  p[ 0]:                 2 (number of parameters)
  p[ 1]:                 module pattern- variable id
  p[ 2]:                 broadcast class

outptr:
-------
  ptr[ 0]:               number of read out data words (n_words)

 data words for all modules in IS:
  ptr[1...n_words]:      header & data words

 structure for each read out module (start: n=1):
  ptr[ n]:               header word (containing word count wc)
  ptr[(n+1)...(n+wc)]:   data words
*/
/******************************************************************************/

plerrcode proc_fb_lc1876_readout_check(p)
  /* readout of fastbus lecroy tdc1876 */
int *p;
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int pat;                                /* IS module pattern                  */
int sds_pat;                            /* sparse data scan module pattern    */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int pa;
int wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
int *ptr;
plerrcode buf_check;

T(proc_fb_lc1876_readout_check)

ptr= outptr++;  n_words= 0;

pat= var_list[p[1]].var.val;
wt_con= MAX_WT_CON;
if ((FRCM(0x9)&pat)^pat) {             /* wait for end of conversion         */
  while ((wt_con--) && ((FRCM(0x9)&pat)^pat)) {
    if (sscode()) {
      outptr--;*outptr++=sscode();*outptr++=0;
      return(plErr_HW);
     }
   }
 }
if (sscode()) {
  outptr--;*outptr++=sscode();*outptr++=1;
  return(plErr_HW);
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  con_pat= FRCM(0x9) & pat;
  if (sscode()) {
    outptr--;*outptr++=sscode();*outptr++=2;
    return(plErr_HW);
   }
  { int unsol_mes[5],unsol_num;
    unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
    unsol_mes[2]=eventcnt;unsol_mes[3]=LC_TDC_1876;unsol_mes[4]=con_pat;
    send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
   }
 }
else {                                 /* conversion completed               */
  DV(D_DUMP,printf("TDC 1876: end of conversion after %d/%d\n",(MAX_WT_CON-wt_con),MAX_WT_CON);)
  if ((buf_check=lc1876_buf(pat,2,eventcnt))!=plOK) {
    outptr--;*outptr++=3;
    return(buf_check);
   }
  FWCM((0x5 | (p[2]<<4)),0x400);        /* LNE for broadcast class p[2]       */
  if (sscode()) {
    outptr--;*outptr++=sscode();*outptr++=4;
    return(plErr_HW);
   }
  sds_pat= FRCM(0xCD) & pat;            /* Sparse Data Scan                   */
  if (sscode()) {
    outptr--;*outptr++=sscode();*outptr++=5;
    return(plErr_HW);
   }
  pa= 0;
  while (sds_pat) {                     /* valid data                         */
    if (sds_pat&0x1) {            /* block readout for geographic address "pa"*/
      wc= FRDB_S(pa,outptr,MAX_WC_PL) - 1;  /* last data word invalid         */
      if (sscode() != 2) {              /* test ss code                       */
        if (sscode() == 0)
         { int unsol_mes[5],unsol_num;
           unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
           unsol_mes[2]=eventcnt;unsol_mes[3]=LC_TDC_1876;unsol_mes[4]=wc;
           send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
          }
        else {
          *outptr++=sscode();*outptr++=6;
          return(plErr_HW);
         }
       }
      outptr+=wc;  n_words+= wc;
      DV(D_DUMP,printf("TDC 1876(%2d): readout completed (%d words)\n",pa,wc);)
     }
    sds_pat>>= 1; pa++;
   }
 }

*ptr= n_words;                          /* number of read out data words      */

return(plOK);

#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1876_readout_check(p)
  /* test subroutine for "fb_lc1876_readout_check" */
int *p;
{
int pat;                                /* IS module pattern                  */
int n_mod;                              /* number of modules                  */
int pa, isml_def;
int i;

T(test_proc_fb_lc1876_readout_check)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

if (p[0]!=2) {
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

if ((unsigned int)p[2]>7) {
  *outptr++=p[2];
  return(plErr_ArgRange);
 }

pat= var_list[p[1]].var.val; n_mod= 0; pa= 0;
while (pat) {                            /* check: module defined in IS       */
  if (pat&0x1) {
    i= 1; isml_def= 0;
    do {
      if (pa==memberlist[i]) {
        if (get_mod_type(memberlist[i]) != LC_TDC_1876) {
          *outptr++=pa;
          return(plErr_BadModTyp);
         }
        isml_def++;
       }
      i++;
     } while ((i<=memberlist[0]) && (!isml_def));
    if (!isml_def) {
      *outptr++=pa;
      return(plErr_BadISModList);
     }
    n_mod++;
   }
  pat>>= 1; pa++;
 }

wirbrauchen= (n_mod*MAX_WC) + 1;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1876_readout[]="fb_lc1876_readout";
int ver_proc_fb_lc1876_readout=1;

/******************************************************************************/

static procprop fb_lc1876_readout_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1876_readout()
{
return(&fb_lc1876_readout_prop);
}

/******************************************************************************/
/* 
Readout of Le Croy TDC 1876
===========================

parameters:
-----------
  p[ 0]:                 2 (number of parameters)
  p[ 1]:                 module pattern- variable id
  p[ 2]:                 broadcast class

outptr:
-------
  ptr[ 0]:               number of read out data words (n_words)

 data words for all modules in IS:
  ptr[1...n_words]:      header & data words

 structure for each read out module (start: n=1):
  ptr[ n]:               header word (containing word count wc)
  ptr[(n+1)...(n+wc)]:   data words
*/
/******************************************************************************/

plerrcode proc_fb_lc1876_readout(p)
  /* readout of fastbus lecroy tdc1876 */
int *p;
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int pat;                                /* IS module pattern                  */
int sds_pat;                            /* sparse data scan module pattern    */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int pa;
int wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
int *ptr;

T(proc_fb_lc1876_readout)

ptr= outptr++;  n_words= 0;

pat= var_list[p[1]].var.val;
wt_con= MAX_WT_CON;
if ((FRCM(0x9)&pat)^pat) {             /* wait for end of conversion         */
  while ((wt_con--) && ((FRCM(0x9)&pat)^pat)) {
    if (sscode()) {
      outptr--;*outptr++=sscode();*outptr++=0;
      return(plErr_HW);
     }
   }
 }
if (sscode()) {
  outptr--;*outptr++=sscode();*outptr++=1;
  return(plErr_HW);
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  con_pat= FRCM(0x9) & pat;
  if (sscode()) {
    outptr--;*outptr++=sscode();*outptr++=2;
    return(plErr_HW);
   }
  { int unsol_mes[5],unsol_num;
    unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
    unsol_mes[2]=eventcnt;unsol_mes[3]=LC_TDC_1876;unsol_mes[4]=con_pat;
    send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
   }
 }
else {                                 /* conversion completed               */
  DV(D_DUMP,printf("TDC 1876: end of conversion after %d/%d\n",(MAX_WT_CON-wt_con),MAX_WT_CON);)
  FWCM((0x5 | (p[2]<<4)),0x400);        /* LNE for broadcast class p[2]       */
  if (sscode()) {
    outptr--;*outptr++=sscode();*outptr++=3;
    return(plErr_HW);
   }
  sds_pat= FRCM(0xCD) & pat;            /* Sparse Data Scan                   */
  if (sscode()) {
    outptr--;*outptr++=sscode();*outptr++=4;
    return(plErr_HW);
   }
  pa= 0;
  while (sds_pat) {                     /* valid data                         */
    if (sds_pat&0x1) {            /* block readout for geographic address "pa"*/
      wc= FRDB_S(pa,outptr,MAX_WC_PL) - 1;  /* last data word invalid         */
      if (sscode() != 2) {              /* test ss code                       */
        if (sscode() == 0)
         { int unsol_mes[5],unsol_num;
           unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
           unsol_mes[2]=eventcnt;unsol_mes[3]=LC_TDC_1876;unsol_mes[4]=wc;
           send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
          }
        else {
          *outptr++=sscode();*outptr++=5;
          return(plErr_HW);
         }
       }
      outptr+=wc;  n_words+= wc;
      DV(D_DUMP,printf("TDC 1876(%2d): readout completed (%d words)\n",pa,wc);)
     }
    sds_pat>>= 1; pa++;
   }
 }

*ptr= n_words;                          /* number of read out data words      */

return(plOK);

#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1876_readout(p)
  /* test subroutine for "fb_lc1876_readout" */
int *p;
{
int pat;                                /* IS module pattern                  */
int n_mod;                              /* number of modules                  */
int pa, isml_def;
int i;

T(test_proc_fb_lc1876_readout)

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

if ((unsigned int)p[2]>7) {
  *outptr++=p[2];
  return(plErr_ArgRange);
 }

pat= var_list[p[1]].var.val; n_mod= 0; pa= 0;
while (pat) {                            /* check: module defined in IS       */
  if (pat&0x1) {
    i= 1; isml_def= 0;
    do {
      if (pa==memberlist[i]) {
        if (get_mod_type(memberlist[i]) != LC_TDC_1876) {
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
return(plOK);
}


/******************************************************************************/
/******************************************************************************/
