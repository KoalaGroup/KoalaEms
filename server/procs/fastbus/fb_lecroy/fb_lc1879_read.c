/*
 * procs/fastbus/fb_lecroy/fb_lc1879_read.c
 * created 28.12.94 MiZi/PeWue/MaWo
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fb_lc1879_read.c,v 1.20 2015/04/06 21:33:30 wuestner Exp $";

#include <config.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
#include <../../../main/server.h>
#include "../../../commu/commu.h"
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../../lowlevel/devices.h"
#include "../../../objects/var/variables.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../trigger/trigger.h"
#include "../../procs.h"
#include "../../procprops.h"

#define MAX_WC  768                   /* max number of words for single event */
#define MAX_WC_PL  769         /* max number of words for single event plus 1 */

extern ems_u32* outptr;
extern int *memberlist;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/fastbus/fb_lecroy")

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1879_readout_t2[]="fb_lc1879_readout_test";
int ver_proc_fb_lc1879_readout_t2=2;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1879_readout_t2_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1879_readout_t2()
{
    return(&fb_lc1879_readout_t2_prop);
}
#endif
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

plerrcode proc_fb_lc1879_readout_t2(ems_u32* p)
  /* test of conversion for fastbus lecroy tdc1879 using module pattern & list */
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int n_mod;                              /* number of modules in sds pattern   */
ems_u32 *list;                          /* IS module list                     */
int wt_con;
ems_u32 con_pat;                        /* "end of conversion"-module pattern */
ems_u32 *ptr;
ems_u32 ssr=0;                          /* slave status response              */
int i;

T(proc_fb_lc1879_readout_t2)

ptr= outptr++;

wt_con= MAX_WT_CON;
list= var_list[p[2]].var.ptr;
n_mod= *list++;
for (i=1;i<=n_mod;i++) {                /* test conversion status             */
  ml_entry* module=modullist->entry+*list;
  struct fastbus_dev* dev=module->address.fastbus.dev;
  int pa=module->address.fastbus.pa;
  
  dev->FCPD(dev, pa, &ssr);             /* primary address cycle              */
  while ((wt_con--) && (dev->FCWSA(dev, 0, &ssr), ssr)){/* wait for conversion*/
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
  DV(D_DUMP,if (!ssr)  printf("TDC 1879(%2d): end of conversion after %d/%d\n",*list,(MAX_WT_CON-wt_con),MAX_WT_CON);)
  list++;
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  ems_u32 unsol_mes[5]; int unsol_num;
  con_pat= 0;
  list= var_list[p[2]].var.ptr;
  list++;
  for (i=1;i<=n_mod;i++) {              /* test conversion status             */
    ml_entry* module=modullist->entry+*list;
    struct fastbus_dev* dev=module->address.fastbus.dev;
    int pa=module->address.fastbus.pa;
  
    dev->FCPD(dev, pa, &ssr);           /* primary address cycle              */
    dev->FCWSA(dev, 0, &ssr);
    dev->FCDISC(dev);
    if (ssr>1) {                        /* test ss code                       */
      outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=2;
      return(plErr_HW);
     }
    if (!ssr)                           /* conversion completed               */
      con_pat+= 1<<(*list);
    list++;
   }
  unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
  unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1879;unsol_mes[4]=con_pat;
  send_unsolicited(Unsol_Data,unsol_mes,unsol_num);
 }

*ptr= 0;                                /* number of read out data words      */

return(plOK);

#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1879_readout_t2(ems_u32* p)
  /* test subroutine for "fb_lc1879_readout_t2" */
{
int pat, hpat;                          /* IS module pattern                  */
ems_u32 *list;                          /* IS module list                     */
int n_mod;                              /* number of modules                  */
int i;

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

list= var_list[p[2]].var.ptr;
n_mod= *list++;
pat= var_list[p[1]].var.val;
hpat=0;

/* check whether all modules in var_list[p[2]] are members of this IS
   and are of type LC_TDC_1879 */
for (i=1; i<=n_mod; i++) {
    ml_entry* module;
    int j, idx;

    idx=*list++;
    for (j=1; (j<=memberlist[0]) && (idx!=memberlist[j]); j++);
    if (j>memberlist[0]) {
        printf("fb_lc1879_readout_t2: %d is not member of IS\n", idx);
        *outptr++=0; *outptr++=idx;
        return plErr_ArgRange;
    }

    module=modullist->entry+idx;
    if (module->modulclass!=modul_fastbus) {
        *outptr++=1; *outptr++=idx;
        return plErr_BadModTyp;
    }
    if (module->modultype!=LC_TDC_1879) {
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
    if ((unsigned int)p[3]>2) 
        wirbrauchen+= n_mod*MAX_WC;


    return plOK;
}
/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1879_readout_t1[]="fb_lc1879_readout_test";
int ver_proc_fb_lc1879_readout_t1=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1879_readout_t1_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1879_readout_t1()
{
return(&fb_lc1879_readout_t1_prop);
}
#endif
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

plerrcode proc_fb_lc1879_readout_t1(ems_u32* p)
  /* readout of fastbus lecroy tdc1879 using module pattern & list */
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int pat;                                /* IS module pattern                  */
ems_u32 sds_pat;                        /* sparse data scan module pattern    */
int n_mod;                              /* number of modules in sds pattern   */
ems_u32 *list;                          /* IS module list                     */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
ems_u32 *ptr;
ems_u32 ssr=0;                          /* slave status response              */
int edge;                               /* leading/trailing edge data         */
int i;
struct fastbus_dev* dev;

T(proc_fb_lc1879_readout_t1)

ptr= outptr++;  n_words= 0;

wt_con= MAX_WT_CON;
list= var_list[p[2]].var.ptr;
n_mod= *list++;

dev=modullist->entry[*list].address.fastbus.dev;

for (i=1;i<=n_mod;i++) {                /* test conversion status             */
  ml_entry* module=modullist->entry+*list;
  int pa=module->address.fastbus.pa;

  dev->FCPD(dev, pa, &ssr);             /* primary address cycle              */
  while ((wt_con--) && (dev->FCWSA(dev, 0, &ssr), ssr)) {  /* wait for conversion      */
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
  DV(D_DUMP,if (!ssr)  printf("TDC 1879(%2d): end of conversion after %d/%d\n",*list,(MAX_WT_CON-wt_con),MAX_WT_CON);)
  list++;
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  ems_u32 unsol_mes[5]; int unsol_num;
  con_pat= 0;
  list= var_list[p[2]].var.ptr;
  list++;
  for (i=1;i<=n_mod;i++) {              /* test conversion status             */
    ml_entry* module=modullist->entry+*list;
    int pa=module->address.fastbus.pa;
  
    dev->FCPD(dev, pa, &ssr);           /* primary address cycle              */
    dev->FCWSA(dev, 0, &ssr);
    dev->FCDISC(dev);
    if (ssr>1) {                        /* test ss code                       */
      outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=2;
      return(plErr_HW);
     }
    if (!ssr)                           /* conversion completed               */
      con_pat+= 1<<(*list);
    list++;
   }
  unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
  unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1879;unsol_mes[4]=con_pat;
  send_unsolicited(Unsol_Data,unsol_mes,unsol_num);
 }
else {                                  /* conversion completed               */
  pat= var_list[p[1]].var.val;
  edge= p[3];
  dev->FRCM(dev, 0xAD, &sds_pat, &ssr); /* Sparse Data Scan                   */
  sds_pat&=pat;
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=3;
    return(plErr_HW);
   }
  list= var_list[p[2]].var.ptr;
  list++;
  for (i=1;i<=n_mod;i++) {
    if ((sds_pat>>(*list))&0x1) {       /* valid data --> block readout       */
      /* XXX irgendwie fehlt hier der code */
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

plerrcode test_proc_fb_lc1879_readout_t1(ems_u32* p)
  /* test subroutine for "fb_lc1879_readout_t1" */
{
int pat, hpat;                          /* IS module pattern                  */
ems_u32 *list;                          /* IS module list                     */
int n_mod;                              /* number of modules                  */
int i;

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

list= var_list[p[2]].var.ptr;
n_mod= *list++;
pat= var_list[p[1]].var.val;
hpat=0;

/* check whether all modules in var_list[p[2]] are members of this IS
   and are of type LC_TDC_1879 */
for (i=1; i<=n_mod; i++) {
    ml_entry* module;
    int j, idx;

    idx=*list++;
    for (j=1; (j<=memberlist[0]) && (idx!=memberlist[j]); j++);
    if (j>memberlist[0]) {
        printf("fb_lc1879_readout_t1: %d is not member of IS\n", idx);
        *outptr++=0; *outptr++=idx;
        return plErr_ArgRange;
    }

    module=modullist->entry+idx;
    if (module->modulclass!=modul_fastbus) {
        *outptr++=1; *outptr++=idx;
        return plErr_BadModTyp;
    }
    if (module->modultype!=LC_TDC_1879) {
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
    if ((unsigned int)p[3]>2) 
        wirbrauchen+= n_mod*MAX_WC;

    return plOK;
}
/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1879_readout_p3[]="fb_lc1879_readout";
int ver_proc_fb_lc1879_readout_p3=3;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1879_readout_p3_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1879_readout_p3()
{
return(&fb_lc1879_readout_p3_prop);
}
#endif
/******************************************************************************/
/* 
Readout of LeCroy TDC 1879
==========================

parameters:
-----------
  p[0] : 3 (number of parameters)
  p[1] : module pattern
  p[2] : crate_ID
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
plerrcode proc_fb_lc1879_readout_p3(ems_u32* p)
  /* readout of fastbus lecroy tdc1879 using module pattern & list */
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

    u_int32_t pat;                      /* IS module pattern                  */
    ems_u32 sds_pat;                    /* sparse data scan module pattern    */
    int n_words;                 /* number of read out words from all modules */
    int wc;                             /* word count from block readout      */
    int wc_max;
    int wt_con;
    u_int32_t con_pat;                  /* "end of conversion"-module pattern */
    u_int32_t *ptr;
    u_int32_t ssr=0;                    /* slave status response              */
    int edge;                           /* leading/trailing edge data         */
    int pa;
    struct fastbus_dev* dev;
    int res;

    T(proc_fb_lc1879_readout_p3)

    pat=p[1];

    ptr=outptr++;
    n_words= 0;

    wt_con= MAX_WT_CON;

    dev=get_fastbus_device(p[2]);

/* wait for end of conversion */
    con_pat=pat; pa=0;
    while (con_pat) {
        if (!(con_pat&1)) {con_pat>>=1; pa++; continue;}
        
        res=dev->FCPD(dev, pa, &ssr);            /* primary address cycle     */
        if (res<0) return plErr_HW;
        while (--wt_con) {                       /* wait for conversion       */
            res=dev->FCWSA(dev, 0, &ssr);
            if (res<0)
                return plErr_HW;
            if (!ssr) { /* conversion completed      */
                res=dev->FCDISC(dev);
                if (res<0)
                    return plErr_HW;
                break;
            }
            if (ssr>1) {                         /* fatal error               */
                res=dev->FCDISC(dev);
                if (res<0)
                    return plErr_HW;
                outptr--; *outptr++=pa; *outptr++=ssr; *outptr++=0;
                return plErr_HW;
            }
        }
        if (!wt_con) {
            res=dev->FCDISC(dev);
            if (res<0)
                return plErr_HW;
            printf("lc1879_readout_p3: pa %d: conversion timeout\n", pa);
            outptr--; *outptr++=pa; *outptr++=2;
            return plErr_HW;
        }
        DV(D_DUMP, printf("TDC 1879(%2d): end of conversion after %d/%d\n",
            pa,(MAX_WT_CON-wt_con), MAX_WT_CON);)
        con_pat>>=1; pa++;
    }

/* Sparse Data Scan */
    res=dev->FRCM(dev, 0xAD, &sds_pat, &ssr);
    if (res<0)
        return plErr_HW;
    if (ssr) {
        outptr--; *outptr++=ssr; *outptr++=3;
        return plErr_HW;
    }
    sds_pat&=pat;

/* read data */
    edge= p[3];
{
    die("procs/fastbus/fb_lecroy/fb_lc1879_read.c:629");
    /* XXX was ist da falsch??? */
}
    if ((edge&3)==3)
        wc_max=2*MAX_WC+1;
    else
        wc_max=MAX_WC_PL;

    pa=0;
    while (sds_pat) {
        if (!(sds_pat&1)) {sds_pat>>=1; pa++; continue;}

        if (edge&0x1) {
            wc=dev->FRDB(dev, pa, 0, wc_max, outptr, &ssr, "proc_fb_lc1879_readout_p3");
            if (wc<0) return plErr_HW;
            wc--; /* last data word invalid  */
            if (ssr != 2) {            /* test ss code                       */
                if (ssr == 0) {
                    send_unsol_warning(8, 3, trigger.eventcnt, LC_TDC_1879, wc);
                } else {
                    *outptr++=ssr; *outptr++=4;
                    return plErr_HW;
                }
            }
            outptr+=wc;
            n_words+=wc;
        }
        if (edge&0x2) {
            wc=dev->FRDB(dev, pa, 1, wc_max, outptr, &ssr, "proc_fb_lc1879_readout_p3");
            if (wc<0) return plErr_HW;
            wc--; /* last data word invalid  */
            if (ssr != 2) {            /* test ss code                       */
                if (ssr == 0) {
                    send_unsol_warning(8, 3, trigger.eventcnt, LC_TDC_1879, wc);
                } else {
                    *outptr++=ssr;*outptr++=5;
                    return plErr_HW;
                }
            }
            outptr+=wc;
            n_words+= wc;
        }
        DV(D_DUMP,printf("TDC 1879(%2d): readout completed (%d words)\n",*list,wc);)
        sds_pat>>=1; pa++;
    }

    *ptr=n_words;                       /* number of read out data words      */

    return plOK;

#undef MAX_WT_CON
}

/******************************************************************************/

plerrcode test_proc_fb_lc1879_readout_p3(ems_u32* p)
  /* test subroutine for "fb_lc1879_readout_p3" */
{
    int pat, hpat;                      /* IS module pattern                  */
    int i, n;
    int edge, crate;
    struct fastbus_dev* dev;
    plerrcode pres;

    T(test_proc_fb_lc1879_readout_pl)

    if (p[0]!=3) {                      /* check number of parameters         */
        *outptr++=p[0];
        return plErr_ArgNum;
    }
 
    if (!memberlist) {                  /* check memberlist                   */
        return plErr_NoISModulList;
    }

    pat=p[1];
    crate=p[2];
    edge=p[3];

    if ((edge & 0x3)!=edge) {
        *outptr++=3; *outptr++=p[3];
        return plErr_ArgRange;
    }

    if ((pres=find_odevice(modul_fastbus, crate, (struct generic_dev**)&dev))!=plOK) {
        *outptr++=2;
        *outptr++=p[2];
        return pres;
    }

    hpat=0;
    for (i=memberlist[0]; i>0; i--) {
        ml_entry* module;

        if (!valid_module(i, modul_fastbus)) continue;
        module=ModulEnt(i);

        if (module->modultype!=LC_TDC_1879) continue;
        if (module->address.fastbus.dev!=dev) continue;
        hpat|=1<<module->address.fastbus.pa;
    }
    if ((pat&hpat)!=pat) {
        printf("test_fb_lc1879_readout_p3: pat=0x%08x hpat=0x%08x\n",
            pat, hpat);
        return plErr_ArgRange;
    }
    n=0;
    while (pat) {
        if (pat&1) n++;
        pat>>=1;
    }

    wirbrauchen=edge==3?(2*n*MAX_WC)+1:(n*MAX_WC)+1;

    return plOK;
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1879_readout_pl[]="fb_lc1879_readout";
int ver_proc_fb_lc1879_readout_pl=2;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1879_readout_pl_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1879_readout_pl()
{
return(&fb_lc1879_readout_pl_prop);
}
#endif
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

plerrcode proc_fb_lc1879_readout_pl(ems_u32* p)
  /* readout of fastbus lecroy tdc1879 using module pattern & list */
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int pat;                                /* IS module pattern                  */
ems_u32 sds_pat;                        /* sparse data scan module pattern    */
int n_mod;                              /* number of modules in sds pattern   */
ems_u32 *list;                          /* IS module list                     */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int wc_max;
int wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
ems_u32 *ptr;
ems_u32 ssr=0;                          /* slave status response              */
int edge;                               /* leading/trailing edge data         */
int i;
struct fastbus_dev* dev;

T(proc_fb_lc1879_readout_pl)

ptr= outptr++;  n_words= 0;

wt_con= MAX_WT_CON;
list= var_list[p[2]].var.ptr;
n_mod= *list++;

dev=modullist->entry[*list].address.fastbus.dev;

for (i=1;i<=n_mod;i++) {                /* test conversion status             */
  ml_entry* module=modullist->entry+*list;
  int pa=module->address.fastbus.pa;

  dev->FCPD(dev, pa, &ssr);             /* primary address cycle              */
  while ((wt_con--) && (dev->FCWSA(dev, 0, &ssr), ssr)) {  /* wait for conversion      */
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
  DV(D_DUMP,if (!ssr)  printf("TDC 1879(%2d): end of conversion after %d/%d\n",*list,(MAX_WT_CON-wt_con),MAX_WT_CON);)
  list++;
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  ems_u32 unsol_mes[5]; int unsol_num;
  con_pat= 0;
  list= var_list[p[2]].var.ptr;
  list++;
  for (i=1;i<=n_mod;i++) {              /* test conversion status             */
    ml_entry* module=modullist->entry+*list;
    int pa=module->address.fastbus.pa;

    dev->FCPD(dev, pa, &ssr);           /* primary address cycle              */
    dev->FCWSA(dev, 0, &ssr);
    dev->FCDISC(dev);
    if (ssr>1) {                        /* test ss code                       */
      outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=2;
      return(plErr_HW);
     }
    if (!ssr)                           /* conversion completed               */
      con_pat+= 1<<(*list);
    list++;
   }
  unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
  unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1879;unsol_mes[4]=con_pat;
  send_unsolicited(Unsol_Data,unsol_mes,unsol_num);
 }
else {                                  /* conversion completed               */
  pat= var_list[p[1]].var.val;
  edge= p[3];
  if (edge&0x3)
    wc_max= MAX_WC_PL;
  else
    wc_max= 2*MAX_WC + 1;
  dev->FRCM(dev, 0xAD, &sds_pat, &ssr);       /* Sparse Data Scan                   */
  sds_pat&=pat;
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=3;
    return(plErr_HW);
   }
  list= var_list[p[2]].var.ptr;
  list++;
  for (i=1;i<=n_mod;i++) {
    if ((sds_pat>>(*list))&0x1) {       /* valid data --> block readout       */
      ml_entry* module=modullist->entry+*list;
      int pa=module->address.fastbus.pa;

      if (edge&0x1) {
        wc=dev->FRDB(dev, pa, 0, wc_max, outptr, &ssr, "proc_fb_lc1879_readout_pl")-1;/* last data word invalid  */
        if (ssr != 2) {            /* test ss code                       */
          if (ssr == 0)
           { ems_u32 unsol_mes[5]; int unsol_num;
             unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
             unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1879;unsol_mes[4]=wc;
             send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
            }
          else {
            *outptr++=ssr;*outptr++=4;
            return(plErr_HW);
           }
         }
        outptr+=wc;  n_words+=wc;
       }
      if (edge&0x2) {
        wc=dev->FRDB(dev, pa, 1, wc_max, outptr, &ssr, "proc_fb_lc1879_readout_pl")-1;  /* last data word invalid*/
        if (ssr != 2) {            /* test ss code                       */
          if (ssr == 0)
           { ems_u32 unsol_mes[5]; int unsol_num;
             unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
             unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1879;unsol_mes[4]=wc;
             send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
            }
          else {
            *outptr++=ssr;*outptr++=5;
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

plerrcode test_proc_fb_lc1879_readout_pl(ems_u32* p)
  /* test subroutine for "fb_lc1879_readout_pl" */
{
int pat, hpat;                          /* IS module pattern                  */
ems_u32 *list;                          /* IS module list                     */
int n_mod;                              /* number of modules                  */
int i;

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

list= var_list[p[2]].var.ptr;
n_mod= *list++;
pat= var_list[p[1]].var.val;
hpat=0;

/* check whether all modules in var_list[p[2]] are members of this IS
   and are of type LC_TDC_1879 */
for (i=1; i<=n_mod; i++) {
    ml_entry* module;
    int j, idx;

    idx=*list++;
    for (j=1; (j<=memberlist[0]) && (idx!=memberlist[j]); j++);
    if (j>memberlist[0]) {
        printf("fb_lc1879_readout_pl: %d is not member of IS\n", idx);
        *outptr++=0; *outptr++=idx;
        return plErr_ArgRange;
    }

    module=modullist->entry+idx;
    if (module->modulclass!=modul_fastbus) {
        *outptr++=1; *outptr++=idx;
        return plErr_BadModTyp;
    }
    if (module->modultype!=LC_TDC_1879) {
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
    if ((unsigned int)p[3]>2) 
        wirbrauchen+= n_mod*MAX_WC;

    return plOK;
}
/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1879_readout[]="fb_lc1879_readout";
int ver_proc_fb_lc1879_readout=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1879_readout_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1879_readout()
{
return(&fb_lc1879_readout_prop);
}
#endif
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

plerrcode proc_fb_lc1879_readout(ems_u32* p)
  /* readout of fastbus lecroy tdc1879 */
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int pat;                                /* IS module pattern                  */
ems_u32 sds_pat;                        /* sparse data scan module pattern    */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int wc_max;
int pa;
int wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
ems_u32 *ptr;
ems_u32 ssr=0;                          /* slave status response              */
int edge;
struct fastbus_dev* dev;

T(proc_fb_lc1879_readout)

ptr= outptr++;  n_words= 0;

dev=modullist->entry[memberlist[1]].address.fastbus.dev;

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
    dev->FCPD(dev, pa, &ssr);           /* primary address cycle              */
    while ((wt_con--) && (dev->FCWSA(dev, 0, &ssr), ssr)) {  /* wait for conversion    */
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
      DV(D_DUMP,printf("TDC 1879(%2d): end of conversion after %d/%d\n",pa,(MAX_WT_CON-wt_con),MAX_WT_CON);)
     }
   }
  pat>>= 1; pa++;
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  ems_u32 unsol_mes[5]; int unsol_num;
  unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
  unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1879;unsol_mes[4]=con_pat;
  send_unsolicited(Unsol_Data,unsol_mes,unsol_num);
 }
else {                                  /* conversion completed               */
  pat= var_list[p[1]].var.val;
  dev->FRCM(dev, 0xAD, &sds_pat, &ssr); /* Sparse Data Scan                   */
  sds_pat&=pat;
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=2;
    return(plErr_HW);
   }
  pa= 0;
  while (sds_pat) {                     /* valid data                         */
    if (sds_pat&0x1) {            /* block readout for geographic address "pa"*/
      if (edge&0x1) {
        wc=dev->FRDB(dev, pa, 0,wc_max, outptr, &ssr, "proc_fb_lc1879_readout")-1;/* last data word invalid          */
        if (ssr != 2) {            /* test ss code                       */
          if (ssr == 0)
           { ems_u32 unsol_mes[5]; int unsol_num;
             unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
             unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1879;unsol_mes[4]=wc;
             send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
            }
          else {
            *outptr++=ssr;*outptr++=3;
            return(plErr_HW);
           }
         }
        outptr+=wc;  n_words+=wc;
       }
      if (edge&0x2) {
        wc=dev->FRDB(dev, pa, 1, wc_max, outptr, &ssr, "proc_fb_lc1879_readout")-1;/* last data word invalid          */
        if (ssr != 2) {            /* test ss code                       */
          if (ssr == 0)
           { ems_u32 unsol_mes[5]; int unsol_num;
             unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
             unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_TDC_1879;unsol_mes[4]=wc;
             send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
            }
          else {
            *outptr++=ssr;*outptr++=4;
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

plerrcode test_proc_fb_lc1879_readout(ems_u32* p)
  /* test subroutine for "fb_lc1879_readout" */
{
int pat, hpat;                          /* IS module pattern                  */
int n_mod;                              /* number of modules                  */
int i;

T(test_proc_fb_lc1879_readout)

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

pat= var_list[p[1]].var.val;
hpat=0;
for (i=1; i<=memberlist[0]; i++) {
    ml_entry* module=modullist->entry+memberlist[i];
    if (module->modultype==LC_TDC_1879) {
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
    if ((unsigned int)p[2]>2) 
        wirbrauchen+= n_mod*MAX_WC;

    return plOK;
}
/******************************************************************************/
/******************************************************************************/
