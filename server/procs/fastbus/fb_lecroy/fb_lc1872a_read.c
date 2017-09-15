/*
 * procs/fastbus/fb_lecroy/fb_lc1872a_read.c
 * created 22.7.96 MaWo/PeWue
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fb_lc1872a_read.c,v 1.12 2011/04/06 20:30:31 wuestner Exp $";

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

#define MAX_WC  64                    /* max number of words for single event */
#define MAX_WC_PL  65          /* max number of words for single event plus 1 */

extern ems_u32* outptr;
extern int *memberlist;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/fastbus/fb_lecroy")

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1872a_readout_pl[]="fb_lc1872a_readout";
int ver_proc_fb_lc1872a_readout_pl=2;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1872a_readout_pl_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1872a_readout_pl()
{
return(&fb_lc1872a_readout_pl_prop);
}
#endif
/******************************************************************************/
/* 
Readout of LeCroy TDC 1872A
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

plerrcode proc_fb_lc1872a_readout_pl(ems_u32* p)
  /* readout of fastbus lecroy tdc1872a using module pattern & list */
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

ems_u32 pat;                            /* IS module pattern                  */
ems_u32 sds_pat;                        /* sparse data scan module pattern    */
int n_mod;                              /* number of modules in sds pattern   */
ems_u32 *list;                          /* IS module list                     */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
/* int pa; *//* unused */
int wt_con;
ems_u32 con_pat;                        /* "end of conversion"-module pattern */
ems_u32 *ptr;
ems_u32 ssr=0;
int i;
struct fastbus_dev* dev;

T(proc_fb_lc1872a_readout_pl)

ptr= outptr++;  n_words= 0;

wt_con= MAX_WT_CON;
list= var_list[p[2]].var.ptr;
n_mod= *list++;

dev=modullist->entry[*list].address.fastbus.dev;

for (i=1;i<=n_mod;i++) {                /* test conversion status             */
  ml_entry* module=modullist->entry+*list;
  int pa=module->address.fastbus.pa;

  dev->FCPC(dev, pa, &ssr);             /* primary address cycle              */
/* wait for conversion */
  while (wt_con-- && (dev->FCWSA(dev, 1, &ssr), ssr)) {   
    if (ssr>1) {                        /* test ss code                       */
      dev->FCDISC(dev);
      outptr--; *outptr++=*list; *outptr++=ssr; *outptr++=0;
      return(plErr_HW);
     }
   }
  dev->FCDISC(dev);                     /* disconnect                         */
  if (ssr>1) {                          /* test ss code                       */
    outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=1;
    return(plErr_HW);
   }
  DV(D_DUMP,if (!ssr)  printf("TDC 1872A(%2d): end of conversion after %d/%d\n",
        *list,(MAX_WT_CON-wt_con),MAX_WT_CON);)
  list++;
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  ems_u32 unsol_mes[5];
  int unsol_num;
  con_pat= 0;
  list= var_list[p[2]].var.ptr;
  list++;
  for (i=1;i<=n_mod;i++) {              /* test conversion status             */
    ml_entry* module=modullist->entry+*list;
    int pa=module->address.fastbus.pa;

    dev->FCPC(dev, pa, &ssr);           /* primary address cycle              */
    dev->FCWSA(dev, 1, &ssr);
    dev->FCDISC(dev);                           /* disconnect                 */
    if (ssr>1) {                        /* test ss code                       */
      outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=2;
      return(plErr_HW);
     }
    if (!ssr)                           /* conversion completed               */
      con_pat+= 1<<(*list);
    list++;
   }
  unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
  unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1872A;unsol_mes[4]=con_pat;
  send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
 }
else {                                  /* conversion completed               */
  pat= var_list[p[1]].var.val;
  dev->FRCM(dev, 0xDD, &sds_pat, &ssr); /* Sparse Data Scan                   */
  sds_pat&=pat;
  if (ssr) {
    outptr--; *outptr++=ssr; *outptr++=3;
    return(plErr_HW);
  }
  list= var_list[p[2]].var.ptr;
  list++;
  for (i=1;i<=n_mod;i++) {
    if ((sds_pat>>(*list))&0x1) {       /* valid data --> block readout       */
      ml_entry* module=modullist->entry+*list;
      int pa=module->address.fastbus.pa;

      wc=dev->FRDB_S(dev, pa, MAX_WC_PL, outptr, &ssr, "proc_fb_lc1872a_readout_pl")-1;
      if (ssr!=2) {                     /* test ss code                       */
        if (ssr==0)
         { ems_u32 unsol_mes[5];
           int unsol_num;
           unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
           unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1872A;unsol_mes[4]=wc;
           send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
          }
        else {
          *outptr++=ssr;*outptr++=4;
          return(plErr_HW);
         }
       }
      outptr+=wc;  n_words+=wc;
      DV(D_DUMP,printf("TDC 1872A(%2d): readout completed (%d words)\n",*list,wc);)
     }
    list++;
   }
 }

*ptr= n_words;                          /* number of read out data words      */

return(plOK);

#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1872a_readout_pl(ems_u32* p)
  /* test subroutine for "fb_lc1872a_readout_pl" */
{
ems_u32 pat, hpat;                      /* IS module pattern                  */
ems_u32 *list;                          /* IS module list                     */
int n_mod;                              /* number of modules                  */
int i;

T(test_proc_fb_lc1872a_readout_pl)

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
   and are of type LC_TDC_1872A */
for (i=1; i<=n_mod; i++) {
    ml_entry* module;
    int j, idx;

    idx=*list++;
    for (j=1; (j<=memberlist[0]) && (idx!=memberlist[j]); j++);
    if (j>memberlist[0]) {
        printf("fb_lc1872a_readout_pl: %d is not member of IS\n", idx);
        *outptr++=0; *outptr++=idx;
        return plErr_ArgRange;
    }

    module=modullist->entry+idx;
    if (module->modulclass!=modul_fastbus) {
        *outptr++=1; *outptr++=idx;
        return plErr_BadModTyp;
    }
    if (module->modultype!=LC_TDC_1872A) {
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

char name_proc_fb_lc1872a_readout[]="fb_lc1872a_readout";
int ver_proc_fb_lc1872a_readout=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1872a_readout_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1872a_readout()
{
return(&fb_lc1872a_readout_prop);
}
#endif
/******************************************************************************/
/* 
Readout of LeCroy TDC 1872A
==========================

parameters:
-----------
  p[0] : 1 (number of parameters)
  p[1] : module pattern- variable id

outptr:
-------
  ptr[ 0]:               number of read out data words (n_words)

 data words for all modules in IS:
  ptr[1...nr_of_words]:  data words
*/
/******************************************************************************/

plerrcode proc_fb_lc1872a_readout(ems_u32* p)
  /* readout of fastbus lecroy tdc1872a */
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

ems_u32 pat;                            /* IS module pattern                  */
int pa;
ems_u32 sds_pat;                        /* sparse data scan module pattern    */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int wt_con;
ems_u32 con_pat;                        /* "end of conversion"-module pattern */
ems_u32 *ptr;
ems_u32 ssr;                            /* slave status response              */
struct fastbus_dev* dev;

T(proc_fb_lc1872a_readout)

ptr= outptr++;  n_words= 0;

dev=modullist->entry[memberlist[1]].address.fastbus.dev;

pat= var_list[p[1]].var.val;  con_pat= 0;  pa=0;
wt_con= MAX_WT_CON;
while (pat) {                           /* test conversion status             */
  if (pat&0x1) {

    dev->FCPC(dev, pa, &ssr);           /* primary address cycle              */
/* wait for conversion */
    while ((wt_con--) && (dev->FCWSA(dev, 1, &ssr), ssr)) {
      if (ssr>1) {                      /* test ss code                       */
        dev->FCDISC(dev);
        outptr--;*outptr++=pa;*outptr++=ssr;*outptr++=0;
        return(plErr_HW);
      }
    }
    dev->FCDISC(dev);                           /* disconnect                         */
    if (ssr>1) {                        /* test ss code                       */
      outptr--;*outptr++=pa;*outptr++=ssr;*outptr++=1;
      return(plErr_HW);
    }
    if (!ssr) {                         /* conversion completed               */
      con_pat+= 1 << pa;
      DV(D_DUMP,printf("TDC 1872A(%2d): end of conversion after %d/%d\n",pa,(MAX_WT_CON-wt_con),MAX_WT_CON);)
    }
  }
  pat>>= 1; pa++;
}

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  ems_u32 unsol_mes[5];
  int unsol_num;
  unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
  unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1872A;unsol_mes[4]=con_pat;
  send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
 }
else {                                  /* conversion completed               */
  pat= var_list[p[1]].var.val;
  dev->FRCM(dev, 0xDD, &sds_pat, &ssr); /* Sparse Data Scan                   */
  sds_pat&=pat;
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=2;
    return(plErr_HW);
   }
  pa=0;
  while (sds_pat) {                     /* valid data                         */
    if (sds_pat&0x1) {            /* block readout for geographic address "pa"*/

      wc=dev->FRDB_S(dev, pa, MAX_WC_PL, outptr, &ssr, "proc_fb_lc1872a_readout") - 1;/* last data word invalid        */
      if (ssr!= 2) {              /* test ss code                       */
        if (ssr== 0)
         { ems_u32 unsol_mes[5];
           int unsol_num;
           unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
           unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1872A;unsol_mes[4]=wc;
           send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
          }
        else {
          *outptr++=ssr;*outptr++=3;
          return(plErr_HW);
         }
       }
      outptr+=wc;  n_words+=wc;
      DV(D_DUMP,printf("TDC 1872A(%2d): readout completed (%d words)\n",pa,wc);)
     }
    sds_pat>>= 1; pa++;
   }
 }

*ptr= n_words;                          /* number of read out data words      */

return(plOK);

#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1872a_readout(ems_u32* p)
  /* test subroutine for "fb_lc1872a_readout" */
{
ems_u32 pat, hpat;                      /* IS module pattern                  */
int i, n_mod;

T(test_proc_fb_lc1872a_readout)

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

pat= var_list[p[1]].var.val;
hpat=0;
for (i=1; i<=memberlist[0]; i++) {
    ml_entry* module=modullist->entry+memberlist[i];
    if (module->modultype==LC_TDC_1872A) {
        hpat|=1<<module->address.fastbus.pa;
    }
}
if ((pat&hpat)!=pat) {
    return plErr_ArgRange;
}

n_mod=0;
for (i=1; i<=memberlist[0]; i++) {
    if (pat&(1<<i)) n_mod++;
}

wirbrauchen= (n_mod*MAX_WC) + 1;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/
