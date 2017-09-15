/*
 * procs/fastbus/fb_lecroy/fb_lc1885f_read.c
 * created 10.5.95 MaWo/MaWo
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fb_lc1885f_read.c,v 1.16 2011/04/06 20:30:31 wuestner Exp $";

#include <config.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
#include "../../../main/server.h"
#include "../../../commu/commu.h"
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../../objects/var/variables.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../trigger/trigger.h"
#include "../../procs.h"
#include "../../procprops.h"

#define MAX_WC  96                    /* max number of words for single event */
#define MAX_WC_PL  97          /* max number of words for single event plus 1 */

extern ems_u32* outptr;
extern int *memberlist;
extern int wirbrauchen;

#define FB_BUFBEG 0
#define FB_BUFEND 0

RCS_REGISTER(cvsid, "procs/fastbus/fb_lecroy")

/******************************************************************************/
static int
do_sparse(ems_u32* ptr, int wc, ems_u32* pedestals)
{
    int i, n=0;
    ems_u32* optr=ptr;

    for (i=wc; i>0; i--) {
        ems_u32 v, data;
        v=*ptr;
        data=v&0xfff;
        if (v&0x800000)
                data*=8;
        if (data>pedestals[(v>>16)&0x7f]) {
            *optr++=v;
            n++;
        }
        ptr++;
    }
    return n;
}
/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1885f_readout_3[]="fb_lc1885f_readout";
int ver_proc_fb_lc1885f_readout_3=3;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1885f_readout_3_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1885f_readout_3()
{
    return(&fb_lc1885f_readout_3_prop);
}
#endif
/******************************************************************************/
/*
Readout of Le Croy ADC 1885F
============================

parameters:
-----------
  p[ 0]:                 3|4 (number of parameters)
  p[ 1]:                 crate
  p[ 2]:                 module pattern
  p[ 3]:                 sparsification            0- disable, 1- enable
 [p[ 4]:                 pedestal values           variable id]

outptr:
-------
  ptr[ 0]:               number of read out data words (n_words)

 data words for all modules in IS:
  ptr[1...nr_of_words]:  data words
*/
/******************************************************************************/
plerrcode proc_fb_lc1885f_readout_3(ems_u32* p)
{
#define MAX_WT_CON  100                /* wait for conversion: max. timeout  */

    ems_u32 pat;                       /* IS module pattern                  */
    ems_u32 sds_pat;                   /* sparse data scan module pattern    */
    int wc;                            /* word count from block readout      */
    ems_u32 pa;
    int wt_con;
    ems_u32 con_pat;                   /* "end of conversion"-module pattern */
    ems_u32* ptr;
    int ped_offset;                    /* offset for pedestal list           */
    ems_u32* pedestals=0;             /* pedestal list for all modules in IS */
    ems_u32 csr;
    ems_u32 ssr=0;                                  /* slave status response */
    struct fastbus_dev* dev;

    T(proc_fb_lc1885f_readout_3)

    dev=get_fastbus_device(p[1]);

    ptr=outptr++;                                     /* space for wordcount */
    if (p[3])
        pedestals=var_list[p[4]].var.ptr;

    pat=p[2];
    con_pat=0;
    pa=0;
    wt_con= MAX_WT_CON;
    while (pat) {                                  /* test conversion status */
        if (pat&0x1) {
            dev->FRCa(dev, pa, 1, &csr, &ssr);
            if (ssr) {
                outptr--;
                *outptr++=ssr; *outptr++=0;
                return plErr_HW;
            }
            /* wait for conversion */
            while ((wt_con--) && (dev->FCWW(dev, csr, &ssr), ssr)) {
                if (ssr>1) {                                 /* test ss code */
                    dev->FCDISC(dev);
                    outptr--;
                    *outptr++=pa; *outptr++=ssr; *outptr++=1;
                    return plErr_HW;
                }
            }
            dev->FCDISC(dev);                                 /* disconnect  */
            if (ssr>1) {                                     /* test ss code */
                outptr--;
                *outptr++=pa; *outptr++=ssr; *outptr++=2;
                return plErr_HW;
            }
            if (!ssr) {                              /* conversion completed */
                con_pat+=1<<pa;
                D(D_DUMP, printf("ADC 1885F(%2d): end of conversion after "
                        "%d/%d\n", pa, (MAX_WT_CON-wt_con), MAX_WT_CON);)
            }
        }
        pat>>=1;
        pa++;
    }

    if (wt_con==-1) {              /* time elapsed: conversion not completed */
        send_unsol_warning(1, 3, trigger.eventcnt, LC_ADC_1885F, con_pat);
        goto raus;
    }

    pat=p[1];
    dev->FRCM(dev, 0x9D, &sds_pat, &ssr);                /* Sparse Data Scan */
    if (ssr) {
        outptr--;
        *outptr++=ssr; *outptr++=3;
        return plErr_HW;
    }
    sds_pat&=pat;

    pa=0;
    ped_offset=0;
    while (sds_pat) {
        /* block readout for geographic address "pa"*/
        if (sds_pat&0x1) {
            /* last data word invalid */
            wc=dev->FRDB_S(dev, pa, MAX_WC_PL, outptr, &ssr, "proc_fb_lc1885f_readout_3")-1;
            if (ssr!=2) {
                if (ssr == 0) {
                    send_unsol_warning(8, 3, trigger.eventcnt, LC_ADC_1885F, wc);
                } else {
                    *outptr++=ssr; *outptr++=4;
                    return plErr_HW;
                }
            }
            if (p[3]) { /* sparsification enabled */
                wc=do_sparse(outptr, wc, pedestals+ped_offset);
            }
            outptr+=wc;

            DV(D_DUMP,printf("ADC 1885F(%2d): readout completed (%d words)\n",pa,sparse_wc);)
        }
        if (pat&0x1)
            ped_offset+=MAX_WC;
        pat>>=1;
        sds_pat>>=1;
        pa++;
    }

raus:
    *ptr=outptr-ptr; /* number of read out data words */
    return plOK;
#undef MAX_WT_CON
}
/******************************************************************************/
plerrcode test_proc_fb_lc1885f_readout_3(ems_u32* p)
{
    int pat, hpat;                     /* IS module pattern                  */
    int n_mod;                         /* number of modules                  */
    int i;
    plerrcode pres;

    T(test_proc_fb_lc1885f_readout_3)

    if ((p[0]!=3)&&(p[0]!=4)) {
        *outptr++=p[0];
        return plErr_ArgNum;
    }

    if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK) {
        *outptr++=1;
        *outptr++=p[1];
        return pres;
    }

    if (memberlist==0) {
        return plErr_NoISModulList;
    }

    pat=p[2];
    hpat=0;
    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=modullist->entry+memberlist[i];
        if (module->modultype==LC_ADC_1885F) {
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

    if (p[0]==4) {
        unsigned int varsize;
        plerrcode pres;

        pres=var_attrib(p[4], &varsize);
        if (pres!=plOK)
            return pres;

        if (varsize!=n_mod*MAX_WC) {
            *outptr++=4; *outptr++=p[4];
            return plErr_IllVarSize;
        }
    }

    wirbrauchen= (n_mod*MAX_WC) + 1;
    return plOK;
}
/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1885f_readout_pl[]="fb_lc1885f_readout";
int ver_proc_fb_lc1885f_readout_pl=2;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1885f_readout_pl_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1885f_readout_pl()
{
return(&fb_lc1885f_readout_pl_prop);
}
#endif
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

plerrcode proc_fb_lc1885f_readout_pl(ems_u32* p)
  /* readout of fastbus lecroy adc1885f using module pattern & list */
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int pat;                                /* IS module pattern                  */
ems_u32 sds_pat;                        /* sparse data scan module pattern    */
int n_mod;                              /* number of modules in sds pattern   */
ems_u32 *list;                          /* IS module list                     */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int sparse_wc;                          /* word count after sparsification    */
int wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
ems_u32 *ptr;
ems_u32 *rptr,*rptr_st=0;               /* module data words                  */
int ped_offset;                         /* offset for pedestal list           */
ems_u32 *pedestals=0;                  /* pedestal list for all modules in IS */
int data;                               /* adc value from data word           */
ems_u32 csr;
ems_u32 ssr=0;                          /* slave status response              */
int i,j;
struct fastbus_dev* dev;

T(proc_fb_lc1885f_readout_pl)

ptr= outptr++;  n_words= 0;
pat= var_list[p[1]].var.val;

if ((pat) && (p[3])) {                  /* prepare sparsification             */
  rptr_st= FB_BUFBEG;
  pedestals= var_list[p[4]].var.ptr;
 }

wt_con= MAX_WT_CON;
list= var_list[p[2]].var.ptr;
dev=(modullist->entry+list[0])->address.fastbus.dev;

n_mod= *list++;
for (i=1;i<=n_mod;i++) {                /* test conversion status             */
  int pa=modullist->entry[*list].address.fastbus.pa;

  dev->FRCa(dev, pa, 1, &csr, &ssr);
  if (ssr) {
    outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=0;
    return(plErr_HW);
   }
  while ((wt_con--) && (dev->FCWW(dev, csr, &ssr), ssr)) {  /* wait for conversion     */
    if (ssr>1) {                        /* test ss code                       */
      dev->FCDISC(dev);
      outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=1;
      return(plErr_HW);
     }
   }
  dev->FCDISC(dev);                             /* disconnect                         */
  if (ssr>1) {                          /* test ss code                       */
    outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=2;
    return(plErr_HW);
   }
  DV(D_DUMP,if (!ssr)  printf("ADC 1885F(%2d): end of conversion after %d/%d\n",*list,(MAX_WT_CON-wt_con),MAX_WT_CON);)
  list++;
 }

if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  ems_u32 unsol_mes[5]; int unsol_num;
  con_pat= 0;
  list= var_list[p[2]].var.ptr;
  list++;
  for (i=1;i<=n_mod;i++) {              /* test conversion status             */
    int pa=modullist->entry[*list].address.fastbus.pa;

    dev->FRCa(dev, pa, 1, &csr, &ssr);
    if (ssr) {
      outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=3;
      return(plErr_HW);
     }
    dev->FCWW(dev, csr, &ssr);
    dev->FCDISC(dev);
    if (ssr>1) {                        /* test ss code                       */
      outptr--;*outptr++=*list;*outptr++=ssr;*outptr++=4;
      return(plErr_HW);
     }
{
    die("procs/fastbus/fb_lecroy/fb_lc1885f_read.c:389");
    /*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
    /* was ist hier falsch???*/
}
    if (!ssr)                           /* conversion completed               */
      con_pat+= 1<<(*list);
    list++;
   }
  unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
  unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_ADC_1885F;unsol_mes[4]=con_pat;
  send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
 }
else {                                  /* conversion completed               */
  dev->FRCM(dev, 0x9D, &sds_pat, &ssr);            /* Sparse Data Scan                   */
  sds_pat&=pat;
  DV(D_DUMP,printf("ADC 1885F: mod pattern= %x, sds pattern= %x\n",pat,sds_pat);)
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=5;
    return(plErr_HW);
   }
  list= var_list[p[2]].var.ptr;
  list++;
  if (p[3]) {
    ped_offset= 0;
    for (i=1;i<=n_mod;i++) {
      int pa=modullist->entry[*list].address.fastbus.pa;
      if ((sds_pat>>pa)&0x1) {     /* valid data --> block readout       */
        rptr= rptr_st;
        sparse_wc= 0;
        wc=dev->FRDB_S(dev, pa, MAX_WC_PL, rptr, &ssr, "proc_fb_lc1885f_readout_pl")-1;  /* last data word invalid      */
        if (ssr!=2) {              /* test ss code                       */
          if (ssr==0)
           { ems_u32 unsol_mes[5]; int unsol_num;
             unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
             unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_ADC_1885F;unsol_mes[4]=wc;
             send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
            }
          else {
            *outptr++=ssr;*outptr++=6;
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
      int pa=modullist->entry[*list].address.fastbus.pa;
      if ((sds_pat>>pa)&0x1) {     /* valid data --> block readout       */
        wc=dev->FRDB_S(dev, pa, MAX_WC_PL, outptr, &ssr, "proc_fb_lc1885f_readout_pl")-1;/* last data word invalid    */
        if (ssr!=2) {              /* test ss code                       */
          if (ssr==0)
           { ems_u32 unsol_mes[5]; int unsol_num;
             unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
             unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_ADC_1885F;unsol_mes[4]=wc;
             send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
            }
          else {
            *outptr++=ssr;*outptr++=7;
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

plerrcode test_proc_fb_lc1885f_readout_pl(ems_u32* p)
  /* test subroutine for "fb_lc1885f_readout_pl" */
{
int pat, hpat;                          /* IS module pattern                  */
ems_u32 *list;                          /* IS module list                     */
int n_mod=0;                            /* number of modules                  */
int i;

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

list= var_list[p[2]].var.ptr;
n_mod= *list++;
pat= var_list[p[1]].var.val;
hpat=0;

/* check whether all modules in var_list[p[2]] are members of this IS
   and are of type LC_ADC_1885F */
for (i=1; i<=n_mod; i++) {
    ml_entry* module;
    int j, idx;

    idx=*list++;
    for (j=1; (j<=memberlist[0]) && (idx!=memberlist[j]); j++);
    if (j>memberlist[0]) {
        printf("fb_lc1885f_readout_pl: %d is not member of IS\n", idx);
        *outptr++=0; *outptr++=idx;
        return plErr_ArgRange;
    }

    module=modullist->entry+idx;
    if (module->modulclass!=modul_fastbus) {
        *outptr++=1; *outptr++=idx;
        return plErr_BadModTyp;
    }
    if (module->modultype!=LC_ADC_1885F) {
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

char name_proc_fb_lc1885f_readout[]="fb_lc1885f_readout";
int ver_proc_fb_lc1885f_readout=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1885f_readout_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1885f_readout()
{
return(&fb_lc1885f_readout_prop);
}
#endif
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

plerrcode proc_fb_lc1885f_readout(ems_u32* p)
  /* readout of fastbus lecroy adc1885f */
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int pat;                                /* IS module pattern                  */
ems_u32 sds_pat;                        /* sparse data scan module pattern    */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int sparse_wc;                          /* word count after sparsification    */
int pa;
int wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
ems_u32 *ptr;
ems_u32 *rptr,*rptr_st=0;               /* module data words                  */
int ped_offset;                         /* offset for pedestal list           */
ems_u32 *pedestals=0;                  /* pedestal list for all modules in IS */
int data;                               /* adc value from data word           */
int i;
ems_u32 csr;
ems_u32 ssr=0;                          /* slave status response              */
struct fastbus_dev* dev;

T(proc_fb_lc1885f_readout)

ptr= outptr++;  n_words= 0;
pat= var_list[p[1]].var.val;

if ((pat) && (p[2])) {                  /* prepare sparsification             */
  rptr_st= FB_BUFBEG;
  pedestals= var_list[p[3]].var.ptr;
 }

dev=(modullist->entry+memberlist[1])->address.fastbus.dev;

con_pat= 0;  pa= 0;
wt_con= MAX_WT_CON;
while (pat) {                           /* test conversion status             */
  if (pat&0x1) {
    dev->FRCa(dev, pa, 1, &csr, &ssr);
    if (ssr) {
      outptr--;*outptr++=ssr;*outptr++=0;
      return(plErr_HW);
     }
    while ((wt_con--) && (dev->FCWW(dev, csr, &ssr), ssr)) {  /* wait for conversion   */
      if (ssr>1) {                      /* test ss code                       */
        dev->FCDISC(dev);
        outptr--;*outptr++=pa;*outptr++=ssr;*outptr++=1;
        return(plErr_HW);
       }
     }
    dev->FCDISC(dev);                           /* disconnect                         */
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
  ems_u32 unsol_mes[5]; int unsol_num;
  unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
  unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_ADC_1885F;unsol_mes[4]=con_pat;
  send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
 }
else {                                  /* conversion completed               */
  pat= var_list[p[1]].var.val;
  dev->FRCM(dev, 0x9D, &sds_pat, &ssr);            /* Sparse Data Scan                   */
  sds_pat&=pat;
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=3;
    return(plErr_HW);
   }
  pa= 0;
  if (p[2]) {
    ped_offset= 0;
    while (sds_pat) {                   /* valid data                         */
      if (sds_pat&0x1) {          /* block readout for geographic address "pa"*/
        rptr= rptr_st;
        sparse_wc= 0;
        wc=dev->FRDB_S(dev, pa, MAX_WC_PL, rptr, &ssr, "proc_fb_lc1885f_readout") - 1;  /* last data word invalid         */
        if (ssr != 2) {            /* test ss code                       */
          if (ssr == 0)
           { ems_u32 unsol_mes[5]; int unsol_num;
             unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
             unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_ADC_1885F;unsol_mes[4]=wc;
             send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
            }
          else {
            *outptr++=ssr;*outptr++=4;
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
        wc=dev->FRDB_S(dev, pa, MAX_WC_PL, outptr, &ssr, "proc_fb_lc1885f_readout") - 1;  /* last data word invalid       */
        if (ssr != 2) {            /* test ss code                       */
          if (ssr == 0)
           { ems_u32 unsol_mes[5]; int unsol_num;
             unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
             unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_ADC_1885F;unsol_mes[4]=wc;
             send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
            }
          else {
            *outptr++=ssr;*outptr++=5;
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

plerrcode test_proc_fb_lc1885f_readout(ems_u32* p)
  /* test subroutine for "fb_lc1885f_readout" */
{
int pat, hpat;                          /* IS module pattern                  */
int n_mod;                              /* number of modules                  */
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

pat= var_list[p[1]].var.val;
hpat=0;
for (i=1; i<=memberlist[0]; i++) {
    ml_entry* module=modullist->entry+memberlist[i];
    if (module->modultype==LC_ADC_1885F) {
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

if (((unsigned int)p[2]&0x1)!=(unsigned int)p[2]) {
  *outptr++=p[2];
  return(plErr_ArgRange);
 }

if (p[2]) {
  if ((unsigned int)p[4]>MAX_VAR) {
    *outptr++=3;*outptr++=p[4];
    return(plErr_IllVar);
   }
  if (!var_list[p[3]].len) {
    *outptr++=3;*outptr++=p[4];
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
