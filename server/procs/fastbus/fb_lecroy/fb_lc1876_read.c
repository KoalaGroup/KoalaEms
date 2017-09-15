/*
 * procs/fastbus/fb_lecroy/fb_lc1876_read.c
 * created 27.12.94 MaWo/MaWo
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fb_lc1876_read.c,v 1.13 2011/04/06 20:30:31 wuestner Exp $";

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
#include "fb_lc_util.h"

#define MAX_WC  1537                  /* max number of words for single event */
#define MAX_WC_PL  1538        /* max number of words for single event plus 1 */

extern ems_u32* outptr;
extern int *memberlist;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/fastbus/fb_lecroy")

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1876_check_buf_2[]="fb_lc1876_check_buf";
int ver_proc_fb_lc1876_check_buf_2=2;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1876_check_buf_prop_2= {0,0,0,0};

procprop* prop_proc_fb_lc1876_check_buf_2()
{
return(&fb_lc1876_check_buf_prop_2);
}
#endif
/******************************************************************************/
/*
Buffer Check of LeCroy TDC 1876
===============================

parameters:
-----------
  p[0] : 3 (number of parameters)
  p[1] : module pattern
  p[2] : crate
  p[3] : loginterval/seconds
  p[4] : bufcheck
*/
/******************************************************************************/

plerrcode proc_fb_lc1876_check_buf_2(ems_u32* p)
  /* check correct read and write pointer positions in lecroy tdc1876 buffer */
  /* memory */
{
    int pat, secs;
    struct fastbus_dev* dev;

    T(proc_fb_lc1876_check_buf_2)

    pat= p[1];
    dev=get_fastbus_device(p[2]);
    secs=p[3];

    return lc1876_buf(dev, pat, 1, trigger.eventcnt, secs, p[4]);
}

/******************************************************************************/

plerrcode test_proc_fb_lc1876_check_buf_2(ems_u32* p)
  /* test subroutine for "fb_lc1876_check_buf" */
{
    int pat, hpat;                      /* IS module pattern                  */

    T(test_proc_fb_lc1876_check_buf_2)

    if (p[0]!=4) {
        *outptr++=p[0];
        return(plErr_ArgNum);
    }

    if (memberlist==0) {                /* check memberlist                   */
        return(plErr_NoISModulList);
    }

    pat=p[1];

    hpat=lc_pat(LC_TDC_1876, "test_lc1876_check_buf_2");
    if ((pat&hpat)!=pat) {
        printf("test_lc1876_check_buf_2: pat=%08x hpat=%08x\n", pat, hpat);
        return plErr_ArgRange;
    }

    wirbrauchen=36;
    return plOK;
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1876_check_buf[]="fb_lc1876_check_buf";
int ver_proc_fb_lc1876_check_buf=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1876_check_buf_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1876_check_buf()
{
return(&fb_lc1876_check_buf_prop);
}
#endif
/******************************************************************************/
/*
Buffer Check of LeCroy TDC 1876
===============================

parameters:
-----------
  p[0] : [1|2] (number of parameters)
  p[ 1] : module pattern- variable id
  [p[2] : loginterval/seconds]
*/
/******************************************************************************/

plerrcode proc_fb_lc1876_check_buf(ems_u32* p)
  /* check correct read and write pointer positions in lecroy tdc1876 buffer */
  /* memory */
{
int pat, secs;
struct fastbus_dev* dev;

T(proc_fb_lc1876_check_buf)

dev=modullist->entry[memberlist[1]].address.fastbus.dev;

pat= var_list[p[1]].var.val;
secs=p[0]>1?p[2]:0;

return lc1876_buf(dev, pat, 1, trigger.eventcnt, secs, 0);
}

/******************************************************************************/

plerrcode test_proc_fb_lc1876_check_buf(ems_u32* p)
  /* test subroutine for "fb_lc1876_check_buf" */
{
int pat, hpat;                          /* IS module pattern                  */
int i;

T(test_proc_fb_lc1876_check_buf)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

if ((p[0]!=1) && (p[0]!=2)) {
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
    if (module->modultype==LC_TDC_1876) {
        hpat|=1<<module->address.fastbus.pa;
    }
}
if ((pat&hpat)!=pat) {
    return plErr_ArgRange;
}

wirbrauchen= 3;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1876_read_ptr[]="fb_lc1876_read_ptr";
int ver_proc_fb_lc1876_read_ptr=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1876_read_ptr_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1876_read_ptr()
{
return(&fb_lc1876_read_ptr_prop);
}
#endif
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

plerrcode proc_fb_lc1876_read_ptr(ems_u32* p)
  /* read read/write pointer positions in lecroy tdc1876 buffer memory        */
{
int n_words;                     /* number of read out words from all modules */
int pat,pa;
ems_u32 val, ssr;
ems_u32 *ptr;
struct fastbus_dev* dev;

T(proc_fb_lc1876_read_ptr)

ptr= outptr++;  n_words= 0;

pat= var_list[p[1]].var.val;

dev=modullist->entry[memberlist[1]].address.fastbus.dev;

pa= 0;
while (pat) {
  if (pat&0x1) {
    dev->FRC(dev, pa, 16, &val, &ssr);
    if (ssr) {
      *outptr++=pa;*outptr++=ssr;*outptr++=0;
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

plerrcode test_proc_fb_lc1876_read_ptr(ems_u32* p)
  /* test subroutine for "fb_lc1876_read_ptr" */
{
int pat, hpat;                          /* IS module pattern                  */
int n_mod;                              /* number of modules                  */
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

pat= var_list[p[1]].var.val;
hpat=0;
for (i=1; i<=memberlist[0]; i++) {
    ml_entry* module=modullist->entry+memberlist[i];
    if (module->modultype==LC_TDC_1876) {
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

wirbrauchen= (n_mod*2) + 1;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1876_readout_pl[]="fb_lc1876_readout";
int ver_proc_fb_lc1876_readout_pl=3;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1876_readout_pl_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1876_readout_pl()
{
return(&fb_lc1876_readout_pl_prop);
}
#endif
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

plerrcode proc_fb_lc1876_readout_pl(ems_u32* p)
  /* readout of fastbus lecroy tdc1876 using module pattern & list */
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int pat;                                /* IS module pattern                  */
ems_u32 sds_pat;                        /* sparse data scan module pattern    */
int n_mod;                              /* number of modules in module list   */
ems_u32 *list;                          /* IS module list                     */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int wt_con;
ems_u32 con_pat;                        /* "end of conversion"-module pattern */
ems_u32 *ptr;
ems_u32 i, val, ssr;
struct fastbus_dev* dev;

T(proc_fb_lc1876_readout_pl)

ptr= outptr++;  n_words= 0;

dev=(modullist->entry+(var_list[p[2]].var.ptr[0]))->address.fastbus.dev;
pat= var_list[p[1]].var.val;
wt_con= MAX_WT_CON;
dev->FRCM(dev, 0x9, &val, &ssr);
if ((val&pat)^pat) {             /* wait for end of conversion         */
  while ((wt_con--) && (dev->FRCM(dev, 0x9, &val, &ssr), (val&pat)^pat)) {
    if (ssr) {
      outptr--;*outptr++=ssr;*outptr++=0;
      return(plErr_HW);
     }
   }
 }
if (ssr) {
  outptr--;*outptr++=ssr;*outptr++=1;
  return(plErr_HW);
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  dev->FRCM(dev, 0x9, &con_pat, &ssr);
  con_pat&=pat;
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=2;
    return(plErr_HW);
   }
  { ems_u32 unsol_mes[5];
    int unsol_num;
    unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
    unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1876;unsol_mes[4]=con_pat;
    send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
   }
 }
else {                                 /* conversion completed               */
  DV(D_DUMP,printf("TDC 1876: end of conversion after %d/%d\n",(MAX_WT_CON-wt_con),MAX_WT_CON);)
  dev->FWCM(dev, (0x5 | (p[3]<<4)), 0x400, &ssr);/* LNE for broadcast class p[3]       */
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=3;
    return(plErr_HW);
   }
  dev->FRCM(dev, 0xCD, &sds_pat, &ssr); /* Sparse Data Scan                   */
  sds_pat&=pat;
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=4;
    return(plErr_HW);
   }
  list= var_list[p[2]].var.ptr;
  n_mod= *list++;
  for (i=1;i<=n_mod;i++) {
    if ((sds_pat>>(*list))&0x1) {       /* valid data --> block readout       */
      ml_entry* module=modullist->entry+*list;
      int pa=module->address.fastbus.pa;

      wc=dev->FRDB_S(dev, pa, MAX_WC_PL, outptr, &ssr, "proc_fb_lc1876_readout_pl")-1;  /* last data word invalid      */
      if (ssr!= 2) {              /* test ss code                       */
        if (ssr== 0)
         { ems_u32 unsol_mes[5];
           int unsol_num;
           unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
           unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1876;unsol_mes[4]=wc;
           send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
          }
        else {
          *outptr++=ssr;*outptr++=5;
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

plerrcode test_proc_fb_lc1876_readout_pl(ems_u32* p)
  /* test subroutine for "fb_lc1876_readout_pl" */
{
int pat, hpat;                          /* IS module pattern                  */
ems_u32 *list;                          /* IS module list                     */
int n_mod;                              /* number of modules                  */
int i;

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

list= var_list[p[2]].var.ptr;
n_mod= *list++;
pat= var_list[p[1]].var.val;
hpat=0;

/* check whether all modules in var_list[p[2]] are members of this IS
   and are of type LC_TDC_1876 */
for (i=1; i<=n_mod; i++) {
    ml_entry* module;
    int j, idx;

    idx=*list++;
    for (j=1; (j<=memberlist[0]) && (idx!=memberlist[j]); j++);
    if (j>memberlist[0]) {
        printf("fb_lc1876_readout_pl: %d is not member of IS\n", idx);
        *outptr++=0; *outptr++=idx;
        return plErr_ArgRange;
    }

    module=modullist->entry+idx;
    if (module->modulclass!=modul_fastbus) {
        *outptr++=1; *outptr++=idx;
        return plErr_BadModTyp;
    }
    if (module->modultype!=LC_TDC_1876) {
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

char name_proc_fb_lc1876_readout_check[]="fb_lc1876_readout";
int ver_proc_fb_lc1876_readout_check=2;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1876_readout_check_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1876_readout_check()
{
return(&fb_lc1876_readout_check_prop);
}
#endif
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

plerrcode proc_fb_lc1876_readout_check(ems_u32* p)
  /* readout of fastbus lecroy tdc1876 */
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int pat;                                /* IS module pattern                  */
ems_u32 sds_pat;                        /* sparse data scan module pattern    */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int pa;
int wt_con;
ems_u32 con_pat, val, ssr;              /* "end of conversion"-module pattern */
ems_u32 *ptr;
plerrcode buf_check;
struct fastbus_dev* dev;

T(proc_fb_lc1876_readout_check)

ptr= outptr++;  n_words= 0;

pat= var_list[p[1]].var.val;
dev=modullist->entry[memberlist[1]].address.fastbus.dev;

wt_con= MAX_WT_CON;
dev->FRCM(dev, 0x9, &val, &ssr);
if ((val&pat)^pat) {             /* wait for end of conversion         */
  while ((wt_con--) && (dev->FRCM(dev, 0x9, &val, &ssr), (val&pat)^pat)) {
    if (ssr) {
      outptr--;*outptr++=ssr;*outptr++=0;
      return(plErr_HW);
     }
   }
 }
if (ssr) {
  outptr--;*outptr++=ssr;*outptr++=1;
  return(plErr_HW);
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  dev->FRCM(dev, 0x9, &con_pat, &ssr);
  con_pat&=pat;
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=2;
    return(plErr_HW);
   }
  { ems_u32 unsol_mes[5];
    int unsol_num;
    unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
    unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1876;unsol_mes[4]=con_pat;
    send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
   }
 }
else {                                 /* conversion completed               */
  DV(D_DUMP,printf("TDC 1876: end of conversion after %d/%d\n",(MAX_WT_CON-wt_con),MAX_WT_CON);)
  if ((buf_check=lc1876_buf(dev, pat, 2, trigger.eventcnt, 0, 0))!=plOK) {
    outptr--;*outptr++=3;
    return(buf_check);
   }
  dev->FWCM(dev, (0x5|(p[2]<<4)), 0x400, &ssr);/* LNE for broadcast class p[2]*/
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=4;
    return(plErr_HW);
   }
  dev->FRCM(dev, 0xCD, &sds_pat, &ssr); /* Sparse Data Scan                   */
  sds_pat&=pat;
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=5;
    return(plErr_HW);
   }
  pa= 0;
  while (sds_pat) {                     /* valid data                         */
    if (sds_pat&0x1) {            /* block readout for geographic address "pa"*/
      wc=dev->FRDB_S(dev, pa, MAX_WC_PL, outptr, &ssr, "proc_fb_lc1876_readout_check")-1;/* last data word invalid         */
      if (ssr!=2) {                     /* test ss code                       */
        if (ssr==0)
         { ems_u32 unsol_mes[5];
           int unsol_num;
           unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
           unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1876;unsol_mes[4]=wc;
           send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
          }
        else {
          *outptr++=ssr;*outptr++=6;
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

plerrcode test_proc_fb_lc1876_readout_check(ems_u32* p)
  /* test subroutine for "fb_lc1876_readout_check" */
{
int pat, hpat;                          /* IS module pattern                  */
int n_mod;                              /* number of modules                  */
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

pat= var_list[p[1]].var.val;
hpat=0;
for (i=1; i<=memberlist[0]; i++) {
    ml_entry* module=modullist->entry+memberlist[i];
    if (module->modultype==LC_TDC_1876) {
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

char name_proc_fb_lc1876_readout[]="fb_lc1876_readout";
int ver_proc_fb_lc1876_readout=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1876_readout_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1876_readout()
{
return(&fb_lc1876_readout_prop);
}
#endif
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

plerrcode proc_fb_lc1876_readout(ems_u32* p)
  /* readout of fastbus lecroy tdc1876 */
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int pat;                                /* IS module pattern                  */
ems_u32 sds_pat;                        /* sparse data scan module pattern    */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int pa;
int wt_con;
ems_u32 con_pat, val, ssr;              /* "end of conversion"-module pattern */
ems_u32 *ptr;
struct fastbus_dev* dev;

T(proc_fb_lc1876_readout)

ptr= outptr++;  n_words= 0;

pat= var_list[p[1]].var.val;
dev=modullist->entry[memberlist[1]].address.fastbus.dev;

wt_con= MAX_WT_CON;
/* wait for end of conversion         */
dev->FRCM(dev, 0x9, &val, &ssr);
if ((val&pat)^pat) {
  while ((wt_con--) && (dev->FRCM(dev, 0x9, &val, &ssr), (val&pat)^pat)) {
    if (ssr) {
      outptr--;*outptr++=ssr;*outptr++=0;
      return(plErr_HW);
     }
   }
 }
if (ssr) {
  outptr--;*outptr++=ssr;*outptr++=1;
  return(plErr_HW);
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  dev->FRCM(dev, 0x9, &con_pat, &ssr);
  con_pat&=pat;
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=2;
    return(plErr_HW);
   }
  { ems_u32 unsol_mes[5];
    int unsol_num;
    unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
    unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1876;unsol_mes[4]=con_pat;
    send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
   }
 }
else {                                 /* conversion completed               */
  DV(D_DUMP,printf("TDC 1876: end of conversion after %d/%d\n",(MAX_WT_CON-wt_con),MAX_WT_CON);)
  dev->FWCM(dev, (0x5|(p[2]<<4)), 0x400, &ssr);/* LNE for broadcast class p[2] */
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=3;
    return(plErr_HW);
   }
  dev->FRCM(dev, 0xCD, &sds_pat, &ssr); /* Sparse Data Scan                   */
  sds_pat&=pat;
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=4;
    return(plErr_HW);
   }
  pa= 0;
  while (sds_pat) {                     /* valid data                         */
    if (sds_pat&0x1) {            /* block readout for geographic address "pa"*/
      wc=dev->FRDB_S(dev, pa, MAX_WC_PL, outptr, &ssr, "proc_fb_lc1876_readout")-1;/* last data word invalid         */
      if (ssr != 2) {              /* test ss code                       */
        if (ssr == 0)
         { ems_u32 unsol_mes[5];
           int unsol_num;
           unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
           unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1876;unsol_mes[4]=wc;
           send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
          }
        else {
          *outptr++=ssr;*outptr++=5;
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

plerrcode test_proc_fb_lc1876_readout(ems_u32* p)
  /* test subroutine for "fb_lc1876_readout" */
{
int pat, hpat;                          /* IS module pattern                  */
int n_mod;                              /* number of modules                  */
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

pat= var_list[p[1]].var.val;
hpat=0;
for (i=1; i<=memberlist[0]; i++) {
    ml_entry* module=modullist->entry+memberlist[i];
    if (module->modultype==LC_TDC_1876) {
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
