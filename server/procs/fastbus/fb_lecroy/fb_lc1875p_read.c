/*
 * procs/fastbus/fb_lecroy/fb_lc1875p_read.c
 * created: 28.12.94 MiZi/PeWue/MaWo
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fb_lc1875p_read.c,v 1.7 2011/04/06 20:30:31 wuestner Exp $";

#include <config.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../../objects/var/variables.h"
#include "../../../objects/domain/get_mod_type.h"
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

char name_proc_fb_lc1875p_readout[]="fb_lc1875p_readout";
int ver_proc_fb_lc1875p_readout=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1875p_readout_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1875p_readout()
{
return(&fb_lc1875p_readout_prop);
}
#endif
/******************************************************************************/
/* 
Readout of LeCroy TDC 1875A
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

plerrcode proc_fb_lc1875p_readout(ems_u32* p)
  /* readout of fastbus lecroy tdc1875a */
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
int ssresp=0;

T(proc_fb_lc1875p_readout)
FBPULSE();
ptr= outptr++;  n_words= 0;

pat= var_list[p[1]].var.val;  con_pat= 0;  pa= 0;
wt_con= MAX_WT_CON;
while (pat) {                           /* test conversion status             */
  if (pat&0x1) {
    FCPC(pa);                           /* primary address cycle              */
    while ((wt_con--) && (ssresp=(FCWSAss(1)&0x7))) {  /* wait for conversion */
      if (ssresp > 1) {                 /* test ss code                       */
        FCDISC();
        outptr--;*outptr++=pa;*outptr++=ssresp;*outptr++=0;
        return(plErr_HW);
       }
     }
    FCDISC();                           /* disconnect                         */
    if (ssresp > 1) {                   /* test ss code                       */
      outptr--;*outptr++=pa;*outptr++=ssresp;*outptr++=1;
      return(plErr_HW);
     }
    if (!ssresp) {                      /* conversion completed               */
      con_pat+= 1 << pa;
      DV(D_DUMP,printf("TDC 1875A(%2d): end of conversion after %d/%d\n",pa,(MAX_WT_CON-wt_con),MAX_WT_CON);)
     }
   }
  pat>>= 1; pa++;
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  int unsol_mes[5],unsol_num;
  unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
  unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1875A;unsol_mes[4]=con_pat;
  send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
 }
else {                                  /* conversion completed               */
  pat= var_list[p[1]].var.val;
  sds_pat= FRCM(0xDD) & pat;            /* Sparse Data Scan                   */
  if (sscode()) {
    outptr--;*outptr++=sscode();*outptr++=2;
    return(plErr_HW);
   }
  pa= 0;
  while (sds_pat) {                     /* valid data                         */
    if (sds_pat&0x1) {            /* block readout for geographic address "pa"*/
      wc= FRDB_S(pa,outptr,MAX_WC_PL) - 1;   /* last data word invalid        */
      if (sscode() != 2) {              /* test ss code                       */
        if (sscode() == 0)
         { int unsol_mes[5],unsol_num;
           unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
           unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1875A;unsol_mes[4]=wc;
           send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
          }
        else {
          *outptr++=sscode();*outptr++=3;
          return(plErr_HW);
         }
       }
      outptr+=wc;  n_words+=wc;
      DV(D_DUMP,printf("TDC 1875A(%2d): readout completed (%d words)\n",pa,wc);)
     }
    sds_pat>>= 1; pa++;
   }
 }

*ptr= n_words;                          /* number of read out data words      */
FBPULSE();
return(plOK);

#undef MAX_WT_CON
}

/******************************************************************************/

plerrcode test_proc_fb_lc1875p_readout(ems_u32* p)
  /* test subroutine for "fb_lc1875p_readout" */
{
int pat;                                /* IS module pattern                  */
int n_mod;                              /* number of modules                  */
int pa, in_mem;
int i;

T(test_proc_fb_lc1875p_readout)

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
    i= 1; in_mem= 0;
    do {
      if (pa==memberlist[i]) {
        if (get_mod_type(memberlist[i]) != LC_TDC_1875A) {
          *outptr++=pa;
          return(plErr_BadModTyp);
         }
        in_mem++;
       }
      i++;
     } while ((i<=memberlist[0]) && (!in_mem));
    if (!in_mem) {                      /* module not defined in IS           */
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
