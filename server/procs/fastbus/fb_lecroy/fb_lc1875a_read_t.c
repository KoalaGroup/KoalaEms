/*
 * procs/fastbus/fb_lecroy/fb_lc1875a_read.c
 * created 28.12.94 MiZi/PeWue/MaWo
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fb_lc1875a_read_t.c,v 1.6 2011/04/06 20:30:31 wuestner Exp $";

#include <config.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../../objects/var/variables.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../trigger/trigger.h"
#include "../../procs.h"
#include "../../procprops.h"
#include "../../../lowlevel/oscompat/oscompat.h"

#define MAX_WC  64                    /* max number of words for single event */
#define MAX_WC_PL  65          /* max number of words for single event plus 1 */

extern ems_u32* outptr;
extern int *memberlist;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/fastbus/fb_lecroy")

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1875a_readout_t2[]="fb_lc1875a_readout_test";
int ver_proc_fb_lc1875a_readout_t2=2;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1875a_readout_t2_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1875a_readout_t2()
{
return(&fb_lc1875a_readout_t2_prop);
}
#endif
/******************************************************************************/
/* 
Readout of LeCroy TDC 1875A
==========================

parameters:
-----------
  p[0] : 2 (number of parameters)
  p[1] : module pattern- variable id
  p[2] : module list- variable id

outptr:
-------
  ptr[ 0]:               number of read out data words (n_words)

 data words for all modules in IS:
  ptr[1...nr_of_words]:  data words
*/
/******************************************************************************/

plerrcode proc_fb_lc1875a_readout_t2(ems_u32* p)
  /* readout of fastbus lecroy tdc1875a using module pattern & list */
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

/* int pat;   *//* unused */            /* IS module pattern                  */
/* int sds_pat;  *//* unused */         /* sparse data scan module pattern    */
int n_mod;                              /* number of modules in sds pattern   */
int *list;                              /* IS module list                     */
int n_words;                     /* number of read out words from all modules */
/* int wc; *//* unused */               /* word count from block readout      */
int wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
int *ptr;
int ssr=0;                              /* slave status response              */
int i;
struct fastbus_dev* dev;

T(proc_fb_lc1875a_readout_t2)

ptr= outptr++;  n_words= 0;

wt_con= MAX_WT_CON;
list= var_list[p[2]].var.ptr;
n_mod= *list++;

dev=(modullist->entry+*list)->address.fastbus.dev;

for (i=1;i<=n_mod;i++) {                /* test conversion status             */
  ml_entry* module=modullist->entry+list[i];
  int pa=module->address.fastbus.pa;
  int ssr;

  dev->FCPC(dev, pa, &ssr);             /* primary address cycle              */
  while ((wt_con--) && ((dev->FCWSA(dev, 1, &ssr), ssr))) { /* wait for conversion      */
    if (ssr>1) {                        /* test ss code                       */
      dev->FCDISC(dev);
      outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=0;
      return(plErr_HW);
     }
   }
  dev->FCDISC(dev);                     /* disconnect                         */
  if (ssr>1) {                          /* test ss code                       */
    outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=1;
    return(plErr_HW);
   }
  DV(D_DUMP,if (!ssr)  printf("TDC 1875A(%2d): end of conversion after %d/%d\n",*list,(MAX_WT_CON-wt_con),MAX_WT_CON);)
  list++;
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  int unsol_mes[5],unsol_num;
  con_pat= 0;
  list= var_list[p[2]].var.ptr;
  list++;
  for (i=1;i<=n_mod;i++) {              /* test conversion status             */
    ml_entry* module=modullist->entry+*list;
    int pa=module->address.fastbus.pa;
    
    dev->FCPC(dev, pa, &ssr);           /* primary address cycle              */
    dev->FCWSA(dev, 1, &ssr);
    dev->FCDISC(dev);                           /* disconnect                         */
    if (ssr>1) {                        /* test ss code                       */
      outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=2;
      return(plErr_HW);
     }
    if (!ssr)                           /* conversion completed               */
      con_pat+= 1<<(*list);
    list++;
   }
  unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
  unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1875A;unsol_mes[4]=con_pat;
  send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
 }

*ptr= 0;                                /* number of read out data words      */

return(plOK);

#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1875a_readout_t2(ems_u32* p)
  /* test subroutine for "fb_lc1875a_readout_t2" */
{
int pat, hpat;                          /* IS module pattern                  */
int *list;                              /* IS module list                     */
int n_mod;                              /* number of modules                  */
int i;

T(test_proc_fb_lc1875a_readout_t2)

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

list= var_list[p[2]].var.ptr;
n_mod= *list++;
pat= var_list[p[1]].var.val;
hpat=0;

/* check whether all modules in var_list[p[2]] are members of this IS
   and are of type LC_TDC_1875A */
for (i=1; i<=n_mod; i++) {
    ml_entry* module;
    int j, idx;

    idx=*list++;
    for (j=1; (j<=memberlist[0]) && (idx!=memberlist[j]); j++);
    if (j>memberlist[0]) {
        printf("fb_lc1875a_readout_t2: %d is not member of IS\n", idx);
        *outptr++=0; *outptr++=idx;
        return plErr_ArgRange;
    }

    module=modullist->entry+idx;
    if (module->modulclass!=modul_fastbus) {
        *outptr++=1; *outptr++=idx;
        return plErr_BadModTyp;
    }
    if (module->modultype!=LC_TDC_1875A) {
        *outptr++=2; *outptr++=idx;
        return plErr_BadModTyp;
    }
    hpat|=1<<module->address.fastbus.pa;
    /* XXX crate has to be checked */
}

if (hpat!=pat) {
    *outptr++=3; *outptr++=pat;
    return plErr_ArgRange;
}

wirbrauchen= (n_mod*MAX_WC) + 1;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1875a_readout_t1[]="fb_lc1875a_readout_test";
int ver_proc_fb_lc1875a_readout_t1=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1875a_readout_t1_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1875a_readout_t1()
{
return(&fb_lc1875a_readout_t1_prop);
}
#endif
/******************************************************************************/
/* 
Readout of LeCroy TDC 1875A
==========================

parameters:
-----------
  p[0] : 2 (number of parameters)
  p[1] : module pattern- variable id
  p[2] : module list- variable id

outptr:
-------
  ptr[ 0]:               number of read out data words (n_words)

 data words for all modules in IS:
  ptr[1...nr_of_words]:  data words
*/
/******************************************************************************/

plerrcode proc_fb_lc1875a_readout_t1(ems_u32* p)
  /* readout of fastbus lecroy tdc1875a using module pattern & list */
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int pat;                                /* IS module pattern                  */
int sds_pat;                            /* sparse data scan module pattern    */
int n_mod;                              /* number of modules in sds pattern   */
int *list;                              /* IS module list                     */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
int *ptr;
int ssr=0;                              /* slave status response              */
int i;
struct fastbus_dev* dev=0;

T(proc_fb_lc1875a_readout_t1)

ptr= outptr++;  n_words= 0;

wt_con= MAX_WT_CON;
list= var_list[p[2]].var.ptr;
n_mod= *list++;
for (i=1;i<=n_mod;i++) {                /* test conversion status             */
  ml_entry* module=modullist->entry+list[i];
  int pa=module->address.fastbus.pa;
  int ssr;

  dev->FCPC(dev, pa, &ssr);             /* primary address cycle              */
  while ((wt_con--) && ((dev->FCWSA(dev, 1, &ssr), ssr))) {  /* wait for conversion      */
    if (ssr>1) {                        /* test ss code                       */
      dev->FCDISC(dev);
      outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=0;
      return(plErr_HW);
     }
   }
  dev->FCDISC(dev);                             /* disconnect                         */
  if (ssr>1) {                          /* test ss code                       */
    outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=1;
    return(plErr_HW);
   }
  DV(D_DUMP,if (!ssr)  printf("TDC 1875A(%2d): end of conversion after %d/%d\n",*list,(MAX_WT_CON-wt_con),MAX_WT_CON);)
  list++;
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  int unsol_mes[5],unsol_num;
  con_pat= 0;
  list= var_list[p[2]].var.ptr;
  list++;
  for (i=1;i<=n_mod;i++) {              /* test conversion status             */
    ml_entry* module=modullist->entry+list[i];
    int pa=module->address.fastbus.pa;
    int ssr;

    dev->FCPC(dev, pa, &ssr);                        /* primary address cycle              */
    dev->FCWSA(dev, 1, &ssr);
    dev->FCDISC(dev);                           /* disconnect                         */
    if (ssr>1) {                        /* test ss code                       */
      outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=2;
      return(plErr_HW);
     }
    if (!ssr)                           /* conversion completed               */
      con_pat+= 1<<(*list);
    list++;
   }
  unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
  unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1875A;unsol_mes[4]=con_pat;
  send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
 }
else {                                  /* conversion completed               */
  pat= var_list[p[1]].var.val;
  dev->FRCM(dev, 0xDD, &sds_pat, &ssr); /* Sparse Data Scan                   */
  sds_pat&=pat;
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=3;
    return(plErr_HW);
   }
  list= var_list[p[2]].var.ptr;
  list++;
  for (i=1;i<=n_mod;i++) {
    if ((sds_pat>>(*list))&0x1) {       /* valid data --> block readout       */
      wc= 0;
      outptr+=wc;  n_words+=wc;
      DV(D_DUMP,printf("TDC 1875A(%2d): readout completed (%d words)\n",*list,wc);)
     }
    list++;
   }
 }

*ptr= n_words;                          /* number of read out data words      */

return(plOK);

#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1875a_readout_t1(ems_u32* p)
  /* test subroutine for "fb_lc1875a_readout_t1" */
{
int pat, hpat;                          /* IS module pattern                  */
int *list;                              /* IS module list                     */
int n_mod;                              /* number of modules                  */
int i;

T(test_proc_fb_lc1875a_readout_t1)

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

list= var_list[p[2]].var.ptr;
n_mod= *list++;
pat= var_list[p[1]].var.val;
hpat=0;

/* check whether all modules in var_list[p[2]] are members of this IS
   and are of type LC_TDC_1875A */
for (i=1; i<=n_mod; i++) {
    ml_entry* module;
    int j, idx;

    idx=*list++;
    for (j=1; (j<=memberlist[0]) && (idx!=memberlist[j]); j++);
    if (j>memberlist[0]) {
        printf("fb_lc1875a_readout_t1: %d is not member of IS\n", idx);
        *outptr++=0; *outptr++=idx;
        return plErr_ArgRange;
    }

    module=modullist->entry+idx;
    if (module->modulclass!=modul_fastbus) {
        *outptr++=1; *outptr++=idx;
        return plErr_BadModTyp;
    }
    if (module->modultype!=LC_TDC_1875A) {
        *outptr++=2; *outptr++=idx;
        return plErr_BadModTyp;
    }
    hpat|=1<<module->address.fastbus.pa;
    /* XXX crate has to be checked */
}

if (hpat!=pat) {
    *outptr++=3; *outptr++=pat;
    return plErr_ArgRange;
}

wirbrauchen= (n_mod*MAX_WC) + 1;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1875a_readout_pl[]="fb_lc1875a_readout";
int ver_proc_fb_lc1875a_readout_pl=2;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1875a_readout_pl_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1875a_readout_pl()
{
return(&fb_lc1875a_readout_pl_prop);
}
#endif
/******************************************************************************/
/* 
Readout of LeCroy TDC 1875A
==========================

parameters:
-----------
  p[0] : 2 (number of parameters)
  p[1] : module pattern- variable id
  p[2] : module list- variable id

outptr:
-------
  ptr[ 0]:               number of read out data words (n_words)

 data words for all modules in IS:
  ptr[1...nr_of_words]:  data words
*/
/******************************************************************************/

plerrcode proc_fb_lc1875a_readout_pl(ems_u32* p)
  /* readout of fastbus lecroy tdc1875a using module pattern & list */
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int pat;                                /* IS module pattern                  */
int sds_pat;                            /* sparse data scan module pattern    */
int n_mod;                              /* number of modules in sds pattern   */
int *list;                              /* IS module list                     */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
int *ptr;
int ssr=0;                              /* slave status response              */
int i;
struct fastbus_dev* dev=0;

T(proc_fb_lc1875a_readout_pl)

ptr= outptr++;  n_words= 0;

wt_con= MAX_WT_CON;
list= var_list[p[2]].var.ptr;
n_mod= *list++;
for (i=1;i<=n_mod;i++) {                /* test conversion status             */
  ml_entry* module=modullist->entry+list[i];
  int pa=module->address.fastbus.pa;
  int ssr;

  if (!dev) dev=module->address.fastbus.dev;
  dev->FCPC(dev, pa, &ssr);             /* primary address cycle              */
  while ((wt_con--) && ((dev->FCWSA(dev, 1, &ssr), ssr))) {  /* wait for conversion      */
    if (ssr>1) {                        /* test ss code                       */
      dev->FCDISC(dev);
      outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=0;
      return(plErr_HW);
     }
   }
  dev->FCDISC(dev);                             /* disconnect                         */
  if (ssr>1) {                          /* test ss code                       */
    outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=1;
    return(plErr_HW);
   }
  DV(D_DUMP,if (!ssr)  printf("TDC 1875A(%2d): end of conversion after %d/%d\n",*list,(MAX_WT_CON-wt_con),MAX_WT_CON);)
  list++;
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  int unsol_mes[5],unsol_num;
  con_pat= 0;
  list= var_list[p[2]].var.ptr;
  list++;
  for (i=1;i<=n_mod;i++) {              /* test conversion status             */
    ml_entry* module=modullist->entry+list[i];
    int pa=module->address.fastbus.pa;
    int ssr;

    dev->FCPC(dev, pa, &ssr);           /* primary address cycle              */
    dev->FCWSA(dev, 1, &ssr);
    dev->FCDISC(dev);                           /* disconnect                         */
    if (ssr>1) {                        /* test ss code                       */
      outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=2;
      return(plErr_HW);
     }
    if (!ssr)                           /* conversion completed               */
      con_pat+= 1<<(*list);
    list++;
   }
  unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
  unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1875A;unsol_mes[4]=con_pat;
  send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
 }
else {                                  /* conversion completed               */
  pat= var_list[p[1]].var.val;
  dev->FRCM(dev, 0xDD, &sds_pat, &ssr); /* Sparse Data Scan                   */
  sds_pat&=pat;
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=3;
    return(plErr_HW);
   }
  list= var_list[p[2]].var.ptr;
  list++;
  for (i=1;i<=n_mod;i++) {
    if ((sds_pat>>(*list))&0x1) {       /* valid data --> block readout       */
      ml_entry* module=modullist->entry+list[i];
      int pa=module->address.fastbus.pa;
      int ssr;

      wc=dev->FRDB_S(dev, pa, MAX_WC_PL, outptr, &ssr)-1; /* last data word invalid     */
      if (ssr!=2) {                /* test ss code                       */
        if (ssr==0)
         { int unsol_mes[5],unsol_num;
           unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
           unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1875A;unsol_mes[4]=wc;
           send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
          }
        else {
          *outptr++=ssr;*outptr++=4;
          return(plErr_HW);
         }
       }
      outptr+=wc;  n_words+=wc;
      DV(D_DUMP,printf("TDC 1875A(%2d): readout completed (%d words)\n",*list,wc);)
     }
    list++;
   }
 }

*ptr= n_words;                          /* number of read out data words      */

return(plOK);

#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1875a_readout_pl(ems_u32* p)
  /* test subroutine for "fb_lc1875a_readout_pl" */
{
int pat, hpat;                          /* IS module pattern                  */
int *list;                              /* IS module list                     */
int n_mod;                              /* number of modules                  */
int i;

T(test_proc_fb_lc1875a_readout_pl)

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

list= var_list[p[2]].var.ptr;
n_mod= *list++;
pat= var_list[p[1]].var.val;
hpat=0;

/* check whether all modules in var_list[p[2]] are members of this IS
   and are of type LC_TDC_1875A */
for (i=1; i<=n_mod; i++) {
    ml_entry* module;
    int j, idx;

    idx=*list++;
    for (j=1; (j<=memberlist[0]) && (idx!=memberlist[j]); j++);
    if (j>memberlist[0]) {
        printf("fb_lc1875a_readout_pl: %d is not member of IS\n", idx);
        *outptr++=0; *outptr++=idx;
        return plErr_ArgRange;
    }

    module=modullist->entry+idx;
    if (module->modulclass!=modul_fastbus) {
        *outptr++=1; *outptr++=idx;
        return plErr_BadModTyp;
    }
    if (module->modultype!=LC_TDC_1875A) {
        *outptr++=2; *outptr++=idx;
        return plErr_BadModTyp;
    }
    hpat|=1<<module->address.fastbus.pa;
    /* XXX crate has to be checked */
}

if (hpat!=pat) {
    *outptr++=3; *outptr++=pat;
    return plErr_ArgRange;
}

wirbrauchen= (n_mod*MAX_WC) + 1;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1875a_readout[]="fb_lc1875a_readout";
int ver_proc_fb_lc1875a_readout=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1875a_readout_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1875a_readout()
{
return(&fb_lc1875a_readout_prop);
}
#endif
/******************************************************************************/
/* 
Readout of LeCroy TDC 1875A
==========================

parameters:
-----------
  p[0] : 1 (number of parameters)
  p[1] : module

outptr:
-------
  ptr[ 0]:               number of read out data words (n_words)

 data words for all modules in IS:
  ptr[1...nr_of_words]:  data words
*/
/******************************************************************************/

plerrcode proc_fb_lc1875a_readout(ems_u32* p)
  /* readout of fastbus lecroy tdc1875a */
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int pat;                                /* IS module pattern                  */
int sds_pat;                            /* sparse data scan module pattern    */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int pa;
int wt_con, weiter;
int *ptr;
int ssr=0;                              /* slave status response              */
struct fastbus_dev* dev;
struct timeval tv0, tv1;
ml_entry* module;

T(proc_fb_lc1875a_readout)

ptr= outptr++;  n_words= 0;

module=ModulEnt(p[1]);
dev=module->address.fastbus.dev;
pa= module->address.fastbus.pa;

dev->FCPC(dev, pa, &ssr);           /* primary address cycle              */
gettimeofday(&tv0, 0);

dev->LEVELOUT(dev, 0, 0xffff, 0x0000);
dev->LEVELOUT(dev, 0, 0x0000, 0xffff);

weiter=1; wt_con=0;
do {
    dev->FCWSA(dev, 1, &ssr);
    if (ssr>1) {
        dev->FCDISC(dev);
        printf("FCWSA failed\n");
        return plErr_HW;
    }
    if (ssr==1) weiter=0;
    gettimeofday(&tv1, 0);
    if (((tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec))>1000)
            weiter=0;
    wt_con++;
} while (weiter);
*outptr++=wt_con;
*outptr++=ssr;

gettimeofday(&tv0, 0);
weiter=1; wt_con=0;
do {
    dev->FCWSA(dev, 1, &ssr);
    if (ssr>1) {
        dev->FCDISC(dev);
        printf("FCWSA failed\n");
        return plErr_HW;
    }
    if (ssr==0) weiter=0;
    gettimeofday(&tv1, 0);
    if (((tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec))>1000)
            weiter=0;
    wt_con++;
} while (weiter);
*outptr++=wt_con;
*outptr++=ssr;

dev->FCDISC(dev);

pat=1<<pa;

dev->FRCM(dev, 0xDD, &sds_pat, &ssr);   /* Sparse Data Scan                   */
if (ssr) {
    printf("FRCM failed\n");
    return plErr_HW;
}
*outptr++=sds_pat;

wc=dev->FRDB_S(dev, pa,MAX_WC_PL, outptr, &ssr)-1;/* last data word invalid        */
if (ssr != 2) {                   /* test ss code                       */
    printf("FRDB_S: ss=%d\n", ssr);
}
outptr+=wc;  n_words+=wc;

*ptr= n_words;                          /* number of read out data words      */

return(plOK);

#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1875a_readout(ems_u32* p)
  /* test subroutine for "fb_lc1875a_readout" */
{
T(test_proc_fb_lc1875a_readout)

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
wirbrauchen= (MAX_WC) + 1;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/
