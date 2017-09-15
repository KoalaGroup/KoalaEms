/*
 * procs/fastbus/fb_lecroy/fb_lc1881_read.c
 * created 27.12.94 MiZi/PeWue/MaWo
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fb_lc1881_read.c,v 1.21 2011/04/06 20:30:31 wuestner Exp $";

#include <config.h>
#include <errno.h>
#include <string.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../../lowlevel/fastbus/fastbus.h"
#if PERFSPECT
#include "../../../lowlevel/perfspect/perfspect.h"
#endif
#include "../../../objects/var/variables.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../objects/pi/readout.h"
#include "../../../trigger/trigger.h"
#include "../../procs.h"
#include "../../procprops.h"
#include "fb_lc_util.h"

#define MAX_WC  65                    /* max number of words for single event */
#define MAX_WC_PL  66          /* max number of words for single event plus 1 */

extern ems_u32* outptr;
extern int *memberlist;
extern int wirbrauchen;
#if PERFSPECT
extern int perfbedarf;
#endif

RCS_REGISTER(cvsid, "procs/fastbus/fb_lecroy")

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1881_check_buf_2[]="fb_lc1881_check_buf";
int ver_proc_fb_lc1881_check_buf_2=2;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1881_check_buf_prop_2= {0,0,0,0};

procprop* prop_proc_fb_lc1881_check_buf_2()
{
    return(&fb_lc1881_check_buf_prop_2);
}
#endif
/******************************************************************************/
/*
Buffer Check of LeCroy ADC 1881
===============================

parameters:
-----------
  p[0] : 3 (number of parameters)
  p[1] : module pattern
  p[2] : crate
  p[3] : loginterval/seconds
  p[4] : bufcheck
  [p[5]: check only if eventcnt&p[5]==0 ]
*/
/******************************************************************************/

plerrcode proc_fb_lc1881_check_buf_2(ems_u32* p)
  /* check correct read and write pointer positions in lecroy adc1881 buffer */
  /* memory */
{
    int pat, secs;
    struct fastbus_dev* dev;

    T(proc_fb_lc1881_check_buf_2)

    if (p[0]>4 && trigger.eventcnt&p[5])
        return plOK;

    pat= p[1];
    dev=get_fastbus_device(p[2]);
    secs=p[3];
    return lc1881_buf(dev, pat, 1, trigger.eventcnt, secs, p[4]);
}

/******************************************************************************/

plerrcode test_proc_fb_lc1881_check_buf_2(ems_u32* p)
  /* test subroutine for "fb_lc1881_check_buf_2" */
{
    int pat, hpat;                      /* IS module pattern                  */
    plerrcode pres;

    T(test_proc_fb_lc1881_check_buf_2)

    if (p[0]!=4 && p[0]!=5) {
        *outptr++=p[0];
        return(plErr_ArgNum);
    }

    if (memberlist==0) {                /* check memberlist                   */
        return(plErr_NoISModulList);
    }

    if ((pres=find_odevice(modul_fastbus, p[2], 0))!=plOK) {
        *outptr++=2;
        *outptr++=p[2];
        return pres;
    }

    pat=p[1];

    hpat=lc_pat(LC_ADC_1881, "test_lc1881_check_buf_2");
    hpat|=lc_pat(LC_ADC_1881M, "test_lc1881_check_buf_2");
    if ((pat&hpat)!=pat) {
        printf("test_lc1881_check_buf_2: pat=%08x hpat=%08x\n", pat, hpat);
        return plErr_ArgRange;
    }

    wirbrauchen=36;
    return plOK;
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1881_check_buf[]="fb_lc1881_check_buf";
int ver_proc_fb_lc1881_check_buf=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1881_check_buf_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1881_check_buf()
{
return(&fb_lc1881_check_buf_prop);
}
#endif
/******************************************************************************/
/*
Buffer Check of LeCroy ADC 1881
===============================

parameters:
-----------
  p[0] : [2|3] (number of parameters)
  p[1] : module pattern variable
  p[2] : crate
  [p[3] : loginterval/seconds
  [p[4] : check only if eventcnt&p[4]==0 ]]
*/
/******************************************************************************/

plerrcode proc_fb_lc1881_check_buf(ems_u32* p)
  /* check correct read and write pointer positions in lecroy adc1881 buffer */
  /* memory */
{
    int pat, secs;
    struct fastbus_dev* dev;

    T(proc_fb_lc1881_check_buf)

    if (p[0]>3 && trigger.eventcnt&p[4])
        return plOK;

    pat= var_list[p[1]].var.val;
    dev=get_fastbus_device(p[2]);
    secs=p[0]>2?p[3]:0;

    return lc1881_buf(dev, pat, 1, trigger.eventcnt, secs, 0);
}

/******************************************************************************/

plerrcode test_proc_fb_lc1881_check_buf(ems_u32* p)
  /* test subroutine for "fb_lc1881_check_buf" */
{
    ems_u32 pat, hpat;                  /* IS module pattern                  */
    plerrcode pres;

    T(test_proc_fb_lc1881_check_buf)

    if ((p[0]<2) || (p[0]>4)) {
        *outptr++=p[0];
        return(plErr_ArgNum);
    }

    if (memberlist==0) {                /* check memberlist                   */
        return(plErr_NoISModulList);
    }

    if ((pres=find_odevice(modul_fastbus, p[2], 0))!=plOK) {
        *outptr++=2;
        *outptr++=p[2];
        return pres;
    }

    if ((pres=var_read_int(p[1], &pat))!=plOK) return pres;

    hpat=lc_pat(LC_ADC_1881, "test_lc1881_check_buf");
    hpat|=lc_pat(LC_ADC_1881M, "test_lc1881_check_buf");
    if ((pat&hpat)!=pat) {
        printf("lc1881_check_buf: pat=%08x hpat=%08x\n", pat, hpat);
        return plErr_ArgRange;
    }

    wirbrauchen= 3;
    return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1881_read_ptr[]="fb_lc1881_read_ptr";
int ver_proc_fb_lc1881_read_ptr=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1881_read_ptr_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1881_read_ptr()
{
return(&fb_lc1881_read_ptr_prop);
}
#endif
/******************************************************************************/
/*
Readout of LeCroy ADC 1881 Buffer Memory Pointers
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

plerrcode proc_fb_lc1881_read_ptr(ems_u32* p)
  /* read read/write pointer positions in lecroy adc1881 buffer memory        */
{
int n_words;                     /* number of read out words from all modules */
int pat,pa;
ems_u32 val, ssr;
ems_u32 *ptr;
struct fastbus_dev* dev;

T(proc_fb_lc1881_read_ptr)

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
    *outptr++= val&0x3f; *outptr++= (val>>8)&0x3f;
    n_words+= 2;
    DV(D_DUMP,printf("ADC 1881(%2d): pointer positions read out\n",pa);)
   }
  pat>>= 1; pa++;
 }

*ptr= n_words;                          /* number of read out data words      */

return(plOK);
}

/******************************************************************************/

plerrcode test_proc_fb_lc1881_read_ptr(ems_u32* p)
  /* test subroutine for "fb_lc1881_read_ptr" */
{
plerrcode res;
ems_u32 pat, hpat;                      /* IS module pattern                  */
int n_mod;                              /* number of modules                  */

T(test_proc_fb_lc1881_read_ptr)

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

if ((res=var_read_int(p[1], &pat))!=plOK) return res;

hpat=lc_pat(LC_ADC_1881, "test_lc1881_read_ptr");
hpat|=lc_pat(LC_ADC_1881M, "test_lc1881_read_ptr");
if ((pat&hpat)!=pat) return plErr_ArgRange;

n_mod=0;
while (pat) {
    if (pat&1) n_mod++;
    pat>>=1;
}

wirbrauchen= (n_mod*2) + 1;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1881_readout_t2[]="fb_lc1881_readout_test";
int ver_proc_fb_lc1881_readout_t2=2;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1881_readout_t2_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1881_readout_t2()
{
return(&fb_lc1881_readout_t2_prop);
}
#endif
/******************************************************************************/
/*
Readout of Le Croy ADC 1881
===========================

parameters:
-----------
  p[ 0]:                 3 (number of parameters)
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
plerrcode proc_fb_lc1881_readout_t2(ems_u32* p)
/* test end of conversion for lecroy adc1881 using module pattern & list */
{
#define MAX_WT_CON  100               /* wait for conversion: max. timeout  */

    int pat;                          /* IS module pattern                  */
    int wt_con;
    ems_u32 con_pat, val, ssr;            /* "end of conversion"-module pattern */
    ems_u32 *ptr;
    struct fastbus_dev* dev;

    T(proc_fb_lc1881_readout_t2)

    ptr= outptr++;

    dev=(modullist->entry+(var_list[p[2]].var.ptr[0]))->address.fastbus.dev;

    pat= var_list[p[1]].var.val;
    wt_con= MAX_WT_CON;
    dev->FRCM(dev, 0xBD, &val, &ssr); /* wait for end of conversion         */
    if ((val&pat)^pat) {
        while ((wt_con--) && (dev->FRCM(dev, 0xBD, &val, &ssr), (val&pat)^pat)) {
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

    if (wt_con==-1) {             /* time elapsed: conversion not completed */
        dev->FRCM(dev, 0xBD, &con_pat, &ssr);
        con_pat&=pat;
        if (ssr) {
            outptr--;*outptr++=ssr;*outptr++=2;
            return(plErr_HW);
        }
        {
            ems_u32 unsol_mes[5]; int unsol_num;
            unsol_num=5;
            unsol_mes[0]=1;
            unsol_mes[1]=3;
            unsol_mes[2]=trigger.eventcnt;
            unsol_mes[3]=LC_ADC_1881;
            unsol_mes[4]=con_pat;
            send_unsolicited(Unsol_Warning, unsol_mes, unsol_num);
        }
    }

    *ptr= 0;                          /* number of read out data words      */

    return(plOK);

    #undef MAX_WT_CON
}
/******************************************************************************/
plerrcode test_proc_fb_lc1881_readout_t2(ems_u32* p)
  /* test subroutine for "fb_lc1881_readout_t2" */
{
plerrcode res;
ems_u32 pat, hpat;                      /* IS module pattern                  */
ems_u32 *list;                          /* IS module list                     */
int n_mod;                              /* number of modules                  */
int i;

T(test_proc_fb_lc1881_readout_t1)

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
if ((res=var_read_int(p[1], &pat))!=plOK) return res;
if ((res=var_get_ptr(p[2], &list))!=plOK) return res;

if ((unsigned int)p[3]>3) {
  *outptr++=p[3];
  return(plErr_ArgRange);
 }

n_mod= *list++;
hpat=0;

/* check whether all modules in var_list[p[2]] are members of this IS
   and are of type LC_ADC_1881 or LC_ADC_1881M */
for (i=1; i<=n_mod; i++) {
    ml_entry* module;
    int j, idx;

    idx=*list++;
    for (j=1; (j<=memberlist[0]) && (idx!=memberlist[j]); j++);
    if (j>memberlist[0]) {
        printf("fb_lc1881_readout_t2: %d is not member of IS\n", idx);
        *outptr++=0; *outptr++=idx;
        return plErr_ArgRange;
    }

    module=modullist->entry+idx;
    if (module->modulclass!=modul_fastbus) {
        *outptr++=1; *outptr++=idx;
        return plErr_BadModTyp;
    }
    if ((module->modultype!=LC_ADC_1881) && (module->modultype!=LC_ADC_1881M)) {
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

char name_proc_fb_lc1881_readout_t1[]="fb_lc1881_readout_test";
int ver_proc_fb_lc1881_readout_t1=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1881_readout_t1_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1881_readout_t1()
{
return(&fb_lc1881_readout_t1_prop);
}
#endif
/******************************************************************************/
/*
Readout of Le Croy ADC 1881
===========================

parameters:
-----------
  p[ 0]:                 3 (number of parameters)
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

plerrcode proc_fb_lc1881_readout_t1(ems_u32* p)
  /* readout of fastbus lecroy adc1881 using module pattern & list */
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

int pat;                                /* IS module pattern                  */
ems_u32 sds_pat;                        /* sparse data scan module pattern    */
int n_mod;                              /* number of modules in module list   */
ems_u32 *list;                          /* IS module list                     */
int n_words;                     /* number of read out words from all modules */
int wc;                                 /* word count from block readout      */
int wt_con;
ems_u32 con_pat, val, ssr;              /* "end of conversion"-module pattern */
ems_u32 *ptr;
int i;
struct fastbus_dev* dev;

T(proc_fb_lc1881_readout_t1)

ptr= outptr++;  n_words= 0;

dev=(modullist->entry+(var_list[p[2]].var.ptr[0]))->address.fastbus.dev;

pat= var_list[p[1]].var.val;
wt_con= MAX_WT_CON;
dev->FRCM(dev, 0xBD, &val, &ssr);
if ((val&pat)^pat) {             /* wait for end of conversion         */
  while ((wt_con--) && (dev->FRCM(dev, 0xBD, &val, &ssr), (val&pat)^pat)) {
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
  dev->FRCM(dev, 0xBD, &con_pat, &ssr);
  con_pat&=pat;
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=2;
    return(plErr_HW);
   }
  { ems_u32 unsol_mes[5]; int unsol_num;
    unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
    unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_ADC_1881;unsol_mes[4]=con_pat;
    send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
   }
 }
else {                                  /* conversion completed               */
  DV(D_DUMP,printf("ADC 1881: end of conversion after %d/%d\n",(MAX_WT_CON-wt_con),MAX_WT_CON);)
  dev->FWCM(dev, (0x5 | (p[3]<<4)),0x400, &ssr);/* LNE for broadcast class p[2]       */
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
  for (i=1; i<= n_mod; i++) {
    if ((sds_pat>>(*list))&0x1) {       /* valid data --> block readout       */
      wc= 0;
      outptr+=wc;  n_words+= wc;
      DV(D_DUMP,printf("ADC 1881 (%2d): readout completed (%d words)\n",*list,wc);)
     }
    list++;
   }
 }

*ptr= n_words;                          /* number of read out data words      */

return(plOK);

#undef MAX_WT_CON
}

/******************************************************************************/

plerrcode test_proc_fb_lc1881_readout_t1(ems_u32* p)
  /* test subroutine for "fb_lc1881_readout_t1" */
{
plerrcode res;
ems_u32 pat, hpat;                      /* IS module pattern                  */
ems_u32 *list;                          /* IS module list                     */
int n_mod;                              /* number of modules                  */
int i;

T(test_proc_fb_lc1881_readout_t1)

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

if ((res=var_read_int(p[1], &pat))!=plOK) return res;
if ((res=var_get_ptr(p[2], &list))!=plOK) return res;

if ((unsigned int)p[3]>3) {
  *outptr++=p[3];
  return(plErr_ArgRange);
 }

n_mod= *list++;
hpat=0;

/* check whether all modules in var_list[p[2]] are members of this IS
   and are of type LC_ADC_1881 */
for (i=1; i<=n_mod; i++) {
    ml_entry* module;
    int j, idx;

    idx=*list++;
    for (j=1; (j<=memberlist[0]) && (idx!=memberlist[j]); j++);
    if (j>memberlist[0]) {
        printf("fb_lc1881_readout_t1: %d is not member of IS\n", idx);
        *outptr++=0; *outptr++=idx;
        return plErr_ArgRange;
    }

    module=modullist->entry+idx;
    if (module->modulclass!=modul_fastbus) {
        *outptr++=1; *outptr++=idx;
        return plErr_BadModTyp;
    }
    if ((module->modultype!=LC_ADC_1881) && (module->modultype!=LC_ADC_1881M)) {
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

char name_proc_fb_lc1881_readout_p5[]="fb_lc1881_readout";
int ver_proc_fb_lc1881_readout_p5=5;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1881_readout_p5_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1881_readout_p5()
{
return(&fb_lc1881_readout_p5_prop);
}
#endif
/******************************************************************************/
/*
Readout of Le Croy ADC 1881
===========================

parameters:
-----------
  p[ 0]:                 3 (number of parameters)
  p[ 1]:                 module pattern
  p[ 2]:                 crate_ID
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
plerrcode proc_fb_lc1881_readout_p5(ems_u32* p)
/* readout of fastbus lecroy adc1881 using module pattern */
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

    int pat;                            /* IS module pattern                  */
    ems_u32 sds_pat;                        /* sparse data scan module pattern    */
    int n_words;                 /* number of read out words from all modules */
    int wc;                             /* word count from block readout      */
    int wt_con;
    ems_u32 val, ssr;                   /* "end of conversion"-module pattern */
    ems_u32 *ptr;
    int pa;
    struct fastbus_dev* dev;

    T(proc_fb_lc1881_readout_pl)

    pat=p[1];

    ptr= outptr++;
    n_words= 0;

    wt_con= MAX_WT_CON;

    dev=get_fastbus_device(p[2]);

/* wait for end of conversion */
    dev->FRCM(dev, 0xBD, &val, &ssr);
    if (ssr) {
        outptr--; *outptr++=ssr; *outptr++=0;
        return plErr_HW;
    }
    while (--wt_con && ((val&pat)!=pat)) {
        dev->FRCM(dev, 0xBD, &val, &ssr);
        if (ssr) {
            outptr--; *outptr++=ssr; *outptr++=0;
            return plErr_HW;
        }
    }

    if (!wt_con) {               /* time elapsed: conversion not completed */
        send_unsol_warning(0, 3, trigger.eventcnt, LC_ADC_1881, val);
        *ptr=0;
        return 0;
    }

    DV(D_DUMP, printf("ADC 1881: end of conversion after %d/%d\n",
            (MAX_WT_CON-wt_con),MAX_WT_CON);)

/* 'load next event' for broadcast class p[3] */
    dev->FWCM(dev, (0x5 | (p[3]<<4)),0x400, &ssr);
    if (ssr) {
        outptr--;*outptr++=ssr;*outptr++=3;
        return(plErr_HW);
    }

/* Sparse Data Scan */
    dev->FRCM(dev, 0xCD, &sds_pat, &ssr);
    sds_pat&=pat;
    if (ssr) {
        outptr--; *outptr++=ssr; *outptr++=4;
        return plErr_HW;
    }

/* read data */

    pa=0;
    while (sds_pat) {
        if (!(sds_pat&1)) {sds_pat>>=1; pa++; continue;}

        /* last data word invalid */
        wc=dev->FRDB_S(dev, pa, MAX_WC_PL, outptr, &ssr,
                "proc_fb_lc1881_readout_p5");
        wc--;
        if (ssr != 2) {               /* test ss code                       */
            if (ssr == 0) {
                if (wc != MAX_WC) {
                    send_unsol_warning(8, 3, trigger.eventcnt, LC_ADC_1881, wc);
                }
            } else {
                *outptr++=ssr;*outptr++=5;
                return plErr_HW;
            }
        }
#if 0
{
        int i, bnr, ga;
        ems_u32 head;
        static int count=0;
        if (count<3) {
            printf("read1881.5 test\n");
            count++;
        }
        for (i=0; i<wc; i++) {
            if ((outptr[i]==0) || (outptr[i]==0x5a5a5a5a)) {
                printf("read1881: evt=%d pa=%d idx=%d: 0x%08x\n", trigger.eventcnt, pa, i, outptr[i]);
            }
        }
        head=outptr[0];
        wc=head&0x7f;
        bnr=(head>>7)&0x3f;
        ga=(head>>27)&0x1f;
        if (ga!=pa) {
            printf("read1881: evt=%d pa=%d idx=%d: ga=%d\n", trigger.eventcnt, pa, i, ga);
        }

}
#endif
        outptr+=wc;  n_words+= wc;
        DV(D_DUMP,printf("ADC 1881 (%2d): readout completed (%d words)\n",*list,wc);)
        sds_pat>>=1; pa++;
    }

    *ptr= n_words;                          /* number of read out data words      */

    return(plOK);

#undef MAX_WT_CON
}

/******************************************************************************/

plerrcode test_proc_fb_lc1881_readout_p5(ems_u32* p)
  /* test subroutine for "fb_lc1881_readout_p5" */
{
    int pat, hpat;                          /* IS module pattern                  */
    int i, n, crate;
    struct fastbus_dev* dev;
    plerrcode pres;

    T(test_proc_fb_lc1881_readout_p5)

    if (p[0]!=3) {                      /* check number of parameters         */
        *outptr++=p[0];
        return plErr_ArgNum;
    }

    if (!memberlist) {                  /* check memberlist                   */
        return plErr_NoISModulList;
    }

    pat=p[1];
    crate=p[2];

    if ((pres=find_odevice(modul_fastbus, crate, (struct generic_dev**)&dev))!=plOK) {
        *outptr++=2;
        *outptr++=p[2];
        return pres;
    }

    hpat=0;
    for (i=memberlist[0]; i>0; i--) {
        ml_entry* module;

        if (!valid_module(i, modul_fastbus, 0)) continue;
        module=ModulEnt(i);

        if ((module->modultype!=LC_ADC_1881)&&
            (module->modultype!=LC_ADC_1881M)) continue;
        if (module->address.fastbus.dev!=dev) continue;
        hpat|=1<<module->address.fastbus.pa;
    }
    if ((pat&hpat)!=pat) {
        printf("test_fb_lc1881_readout_p5: pat=0x%08x hpat=0x%08x\n",
            pat, hpat);
        return plErr_ArgRange;
    }
    n=0;
    while (pat) {
        if (pat&1) n++;
        pat>>=1;
    }

    wirbrauchen=n*MAX_WC+1;
    return plOK;
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1881_readout_pl[]="fb_lc1881_readout";
int ver_proc_fb_lc1881_readout_pl=3;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1881_readout_pl_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1881_readout_pl()
{
return(&fb_lc1881_readout_pl_prop);
}
#endif
/******************************************************************************/
/*
Readout of Le Croy ADC 1881
===========================

parameters:
-----------
  p[ 0]:                 3 (number of parameters)
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
plerrcode proc_fb_lc1881_readout_pl(ems_u32* p)
/* readout of fastbus lecroy adc1881 using module pattern & list */
{
#define MAX_WT_CON  100                /* wait for conversion: max. timeout  */

    int pat;                           /* IS module pattern                  */
    ems_u32 sds_pat;                   /* sparse data scan module pattern    */
    int n_mod;                         /* number of modules in module list   */
    ems_u32 *list;                     /* IS module list                     */
    int n_words;                /* number of read out words from all modules */
    int wc;                            /* word count from block readout      */
    int wt_con;
    ems_u32 con_pat, val, ssr;         /* "end of conversion"-module pattern */
    ems_u32 *ptr;
    int i;
    struct fastbus_dev* dev;

    T(proc_fb_lc1881_readout_pl)

    ptr= outptr++;
    n_words= 0;

    dev=(modullist->entry+(var_list[p[2]].var.ptr[0]))->address.fastbus.dev;

    pat= var_list[p[1]].var.val;
    wt_con= MAX_WT_CON;
    dev->FRCM(dev, 0xBD, &val, &ssr);
    if ((val&pat)^pat) {
        /* wait for end of conversion */
        while ((wt_con--) && (dev->FRCM(dev, 0xBD, &val, &ssr), (val&pat)^pat)) {
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

    if (wt_con==-1) { /* time elapsed: conversion not completed */
        dev->FRCM(dev, 0xBD, &con_pat, &ssr);
        con_pat&=pat;
        if (ssr) {
            outptr--;*outptr++=ssr;*outptr++=2;
            return(plErr_HW);
        }
        {
            ems_u32 unsol_mes[5]; int unsol_num;
            unsol_num=5;
            unsol_mes[0]=1;
            unsol_mes[1]=3;
            unsol_mes[2]=trigger.eventcnt;
            unsol_mes[3]=LC_ADC_1881;
            unsol_mes[4]=con_pat;
            send_unsolicited(Unsol_Warning, unsol_mes, unsol_num);
        }
    } else { /* conversion completed               */
        DV(D_DUMP, printf("ADC 1881: end of conversion after %d/%d\n",
                (MAX_WT_CON-wt_con),MAX_WT_CON);)
        /* LNE for broadcast class p[2]       */
        dev->FWCM(dev, (0x5 | (p[3]<<4)),0x400, &ssr);
        if (ssr) {
            outptr--;*outptr++=ssr;*outptr++=3;
            return(plErr_HW);
        }
        dev->FRCM(dev, 0xCD, &sds_pat, &ssr); /* Sparse Data Scan            */
        sds_pat&=pat;
        if (ssr) {
            outptr--;*outptr++=ssr;*outptr++=4;
            return(plErr_HW);
        }
        list= var_list[p[2]].var.ptr;
        n_mod= *list++;
        for (i=1; i<= n_mod; i++) {
            if ((sds_pat>>(*list))&0x1) { /* valid data --> block readout  */
                /* last data word invalid */
                wc=dev->FRDB_S(dev, *list, MAX_WC_PL, outptr, &ssr, "proc_fb_lc1881_readout_pl")-1;
                if (ssr != 2) {      /* test ss code                       */
                    if (ssr == 0) {
                        if (wc != MAX_WC) {
                            ems_u32 unsol_mes[5];
                            int unsol_num;
                            unsol_num=5;
                            unsol_mes[0]=8;
                            unsol_mes[1]=3;
                            unsol_mes[2]=trigger.eventcnt;
                            unsol_mes[3]=LC_ADC_1881;
                            unsol_mes[4]=wc;
                            send_unsolicited(Unsol_Warning, unsol_mes, unsol_num);
                        }
                    } else {
                        *outptr++=ssr;*outptr++=5;
                        return(plErr_HW);
                    }
                }
                outptr+=wc;
                n_words+= wc;
                DV(D_DUMP, printf("ADC 1881 (%2d): readout completed "
                        "(%d words)\n", *list,wc);)
            }
            list++;
        }
    }

    *ptr= n_words;                          /* number of read out data words      */

    return(plOK);

#undef MAX_WT_CON
}
/******************************************************************************/

plerrcode test_proc_fb_lc1881_readout_pl(ems_u32* p)
  /* test subroutine for "fb_lc1881_readout_pl" */
{
int pat, hpat;                          /* IS module pattern                  */
ems_u32 *list;                          /* IS module list                     */
int n_mod;                              /* number of modules                  */
int i;

T(test_proc_fb_lc1881_readout_pl)

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

if ((unsigned int)p[3]>3) {
  *outptr++=p[3];
  return(plErr_ArgRange);
 }

list= var_list[p[2]].var.ptr;
n_mod= *list++;
pat= var_list[p[1]].var.val;
hpat=0;

/* check whether all modules in var_list[p[2]] are members of this IS
   and are of type LC_ADC_1881 */
for (i=1; i<=n_mod; i++) {
    ml_entry* module;
    int j, idx;

    idx=*list++;
    for (j=1; (j<=memberlist[0]) && (idx!=memberlist[j]); j++);
    if (j>memberlist[0]) {
        printf("fb_lc1881_readout_pl: %d is not member of IS\n", idx);
        *outptr++=0; *outptr++=idx;
        return plErr_ArgRange;
    }

    module=modullist->entry+idx;
    if (module->modulclass!=modul_fastbus) {
        *outptr++=1; *outptr++=idx;
        return plErr_BadModTyp;
    }
    if ((module->modultype!=LC_ADC_1881) && (module->modultype!=LC_ADC_1881M)) {
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

char name_proc_fb_lc1881_readout_multi[]="fb_lc1881_readout";
int ver_proc_fb_lc1881_readout_multi=4;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1881_readout_multi_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1881_readout_multi()
{
return(&fb_lc1881_readout_multi_prop);
}
#endif
/******************************************************************************/
/*
Readout of Le Croy ADC 1881
===========================

parameters:
-----------
  p[ 0]:                 4 (number of parameters)
  p[ 1]:                 module pattern- variable id
  p[ 2]:                 broadcast class
  p[ 3]:                 primary link; index in modullist
  p[ 4]:                 number of modules
 [p[ 5]:                 type of check:
                           0: none
                           1: all
                           |0x10: don't abort
 ]

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
static int
parity(u_int32_t d)
{
    d ^= d>>16;
    d ^= d>>8;
    d ^= d>>4;
    d ^= d>>2;
    d ^= d>>1;
    return d & 1;
}
/******************************************************************************/
static int
fb_lc1881_datacheck(ems_u32 *d, int flags/*ignored*/, int dump)
{
    int wc=*d++;
    int error=0;
    int last_geo=-1;
    int last_buffer=-1;
    int chans=0;
    int i;

    if ((trigger.eventcnt%1000)==0) {
        dump=1;
    }

    if (dump) {
        printf("fb_lc1881 event %d, %d words\n", trigger.eventcnt, wc);
        printf("               geo buf chan  adc\n");
    }

    for (i=0; i<wc; i++) {
        int geo;

        if (dump)
            printf("  %4d %08x", i, *d);
        if (parity(*d)) {
            if (dump)
                printf("\n    ERROR: wrong parity\n");
            error=1;
            continue;
        }
        geo=(*d>>27)&0x1f;
        if (!chans) { /* header */
            last_buffer=(*d>>7)&0x3f;
            chans=*d&0x7f;
            if (dump)
                printf(" header: geo=%2d buffer=%2d words=%3d\n",
                    geo, last_buffer, chans);
            if (last_geo>=0 && geo!=last_geo-1) {
                if (dump)
                    printf("    ERROR: illegal geo-sequence\n");
                error=1;
            }
            if (!chans) {
                if (dump)
                    printf("    ERROR: illegal word count\n");
                error=1;
            }
            last_geo=geo;
            chans--; /* header was counted too */
        } else { /* data word */
            int date, buffer, channel;
            buffer=(*d>>24)&0x3;
            channel=(*d>>17)&0x3f;
            date=*d&0x3fff;
            if (dump)
                printf(" %2d  %d   %2d %4d\n", geo, buffer, channel, date);
            if (buffer!=(last_buffer&0x3)) {
                if (dump)
                    printf("    ERROR: illegal buffer\n");
                error=1;
            }
            if (geo!=last_geo) {
                if (dump)
                    printf("    ERROR: illegal geo in data\n");
                error=1;
            }
            chans--;
        }
        d++;
    }
    return error;
}
/******************************************************************************/

plerrcode proc_fb_lc1881_readout_multi(ems_u32* p)
  /* multiblock readout of fastbus lecroy adc1881 using module pattern */
{
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

    int pat;                            /* IS module pattern                  */
    int wc;                             /* word count from block readout      */
    int wt_con;
    ems_u32 con_pat, val, ssr;          /* "end of conversion"-module pattern */
    ems_u32 *ptr;
    int pa;
    ml_entry* module;
    struct fastbus_dev* dev;

    T(proc_fb_lc1881_readout_multi)

    ptr= outptr++;

    module=modullist->entry+p[3];
    dev=module->address.fastbus.dev;
    pa=module->address.fastbus.pa;

    pat= var_list[p[1]].var.val;
    wt_con= MAX_WT_CON;
    dev->FRCM(dev, 0xBD, &val, &ssr);
    if ((val&pat)!=pat) {              /* wait for end of conversion         */
        while((wt_con--)&&(dev->FRCM(dev, 0xBD, &val, &ssr),(val&pat)!=pat)) {
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

    if (wt_con==-1) {              /* time elapsed: conversion not completed */
        dev->FRCM(dev, 0xBD, &con_pat, &ssr);
        con_pat&=pat;
        if (ssr) {
            outptr--;*outptr++=ssr;*outptr++=2;
            return(plErr_HW);
        }
#if 0
        outptr--;*outptr++=con_pat;*outptr++=3;
        return(plErr_HW);
#else
        send_unsol_warning(1, 3, trigger.eventcnt, LC_ADC_1881, con_pat);
        invalidate_event();
        return plOK;
#endif
    }

/* conversion completed */
#if PERFSPECT
    perfspect_record_time("proc_fb_lc1881_readout_multi");
#endif
                                       /* LNE for broadcast class p[2]       */
    dev->FWCM(dev, (0x5 | (p[2]<<4)), 0x400, &ssr);
    if (ssr) {
        outptr--;*outptr++=ssr;*outptr++=4;
        return(plErr_HW);
    }
#if PERFSPECT
    perfspect_record_time("proc_fb_lc1881_readout_multi");
#endif
    wc=dev->FRDB_S(dev, pa, (p[4]*MAX_WC_PL)+101, outptr, &ssr, "proc_fb_lc1881_readout_multi");
#if PERFSPECT
    perfspect_record_time("proc_fb_lc1881_readout_multi");
#endif
    if (wc<0) {
        printf("fb_lc1881_readout_multi: FRDB_S: %s\n", strerror(errno));
        outptr--;*outptr++=errno;
        return plErr_System;
    }
if (ssr==0) ssr=2;
    if (ssr != 2) {
        if (ssr == 0) {
            printf("fb_lc1881_readout_multi: ss=0 wc=%d\n", wc);
            send_unsol_warning(8, 3, trigger.eventcnt, LC_ADC_1881, wc);
            invalidate_event();
            wc++;
        } else {
            outptr--;*outptr++=ssr;*outptr++=5;
            return(plErr_HW);
        }
    }
/* XXX change of wc after delay_read is ILLEGAL */
    wc--;                              /* last word is dummy                 */
    outptr+=wc;
    *ptr= wc;                          /* number of read out data words      */


    if ((p[0]>4) && p[5]) {
        if (fb_lc1881_datacheck(ptr, p[5], 0)) {
            fb_lc1881_datacheck(ptr, p[5], 1);
            //if (!(p[5]&0x10))
                return plErr_HW;
        }
    }

    return(plOK);

#undef MAX_WT_CON
}

/******************************************************************************/

plerrcode test_proc_fb_lc1881_readout_multi(ems_u32* p)
  /* test subroutine for "fb_lc1881_readout_multi" */
{
    plerrcode res;
    ems_u32 pat, hpat;                      /* IS module pattern              */
    int n_mod;                              /* number of modules              */
    ml_entry* module;

    T(test_proc_fb_lc1881_readout_multi)

    if (memberlist==0) {
        return(plErr_NoISModulList);
    }
    if (memberlist[0]==0) {
        return(plErr_BadISModList);
    }

    if (p[0]<4 && p[0]>5)
        return plErr_ArgNum;

    printf("fb_lc1881_readout: p[0]=%d\n", p[0]);
    if (p[0]>4)
        printf("fb_lc1881_readout: p[5]=%d\n", p[5]);

    if ((res=var_read_int(p[1], &pat))!=plOK) return res;

    if ((unsigned int)p[2]>3) {
        printf("lc1881_readout_multi: bc=%d\n", p[2]);
        *outptr++=p[2]; *outptr++=2;
        return(plErr_ArgRange);
    }

    if ((unsigned int)p[3]>modullist->modnum) {
        *outptr++=p[3]; *outptr++=3;
        return plErr_ArgRange;
    }

    module=modullist->entry+p[3];
    if (module->modultype!=LC_ADC_1881M) {
        *outptr++=module->modultype; *outptr++=3;
        return plErr_BadModTyp;
    }

    hpat=lc_pat(LC_ADC_1881M, "test_lc1881_readout_multi");
    if ((pat&hpat)!=pat) {
        printf("lc1881_readout_multi: pat=%08x hpat=%08x\n", pat, hpat);
        return plErr_ArgRange;
    }

    n_mod=0;
    while (pat) {
        if (pat&1) n_mod++;
        pat>>=1;
    }

    wirbrauchen= (n_mod*MAX_WC) + 1;
#if PERFSPECT
    perfnames[0]="conv-complete";
    perfnames[1]="before-FRDB_S";
    dev->FRDB_S_perfspect_needs(dev, &perfbedarf, perfnames+2);
    perfnames[2+perfbedarf]="after-FRDB_S";
    perfbedarf+=3;

    perfnames[0]="conv-complete";
    perfnames[1]="before-FRDB_S";
    perfnames[2]="FRdcB-DMAstart";
    perfnames[3]="FRdcB-DMAend";
    perfnames[4]="FRdcB-FIFOa";
    perfnames[5]="FRdcB-FIFOb";
    perfnames[6]="after-FRDB_S";
#endif

    return plOK;
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1881_readout_check[]="fb_lc1881_readout";
int ver_proc_fb_lc1881_readout_check=2;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1881_readout_check_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1881_readout_check()
{
return(&fb_lc1881_readout_check_prop);
}
#endif
/******************************************************************************/
/*
Readout of Le Croy ADC 1881
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

plerrcode proc_fb_lc1881_readout_check(ems_u32* p)
  /* readout of fastbus lecroy adc1881 */
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

T(proc_fb_lc1881_readout_check)

ptr= outptr++;  n_words= 0;

dev=modullist->entry[memberlist[1]].address.fastbus.dev;

pat= var_list[p[1]].var.val;
wt_con= MAX_WT_CON;
dev->FRCM(dev, 0xBD, &val, &ssr);
if ((val&pat)^pat) {             /* wait for end of conversion         */
  while ((wt_con--) && (dev->FRCM(dev, 0xBD, &val, &ssr), (val&pat)^pat)) {
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
  dev->FRCM(dev, 0xBD, &con_pat, &ssr);
  con_pat&=pat;
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=2;
    return(plErr_HW);
   }
  { ems_u32 unsol_mes[5]; int unsol_num;
    unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
    unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_ADC_1881;unsol_mes[4]=con_pat;
    send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
   }
 }
else {                                  /* conversion completed               */
  DV(D_DUMP,printf("ADC 1881: end of conversion after %d/%d\n",(MAX_WT_CON-wt_con),MAX_WT_CON);)
  if ((buf_check=lc1881_buf(dev, pat, 2, trigger.eventcnt, 0, 0))!=plOK) {
    outptr--;*outptr++=3;
    return(buf_check);
   }
  dev->FWCM(dev, (0x5 | (p[2]<<4)),0x400, &ssr);        /* LNE for broadcast class p[2]       */
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=4;
    return(plErr_HW);
   }
  dev->FRCM(dev, 0xCD, &sds_pat, &ssr);            /* Sparse Data Scan                   */
  sds_pat&=pat;
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=5;
    return(plErr_HW);
   }
  pa= 0;
  while (sds_pat) {                     /* valid data                         */
    if (sds_pat&0x1) {            /* block readout for geographic address "pa"*/
      wc=dev->FRDB_S(dev, pa, MAX_WC_PL, outptr, &ssr, "proc_fb_lc1881_readout_check")-1;/* last data word invalid         */
      if (ssr != 2) {              /* test ss code                       */
        if (ssr == 0) {
          { ems_u32 unsol_mes[5]; int unsol_num;
           unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
           unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_ADC_1881;unsol_mes[4]=wc;
           send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
          }
         }
        else {
          *outptr++=ssr;*outptr++=6;
          return(plErr_HW);
         }
       }
      outptr+=wc;  n_words+= wc;
      DV(D_DUMP,printf("ADC 1881(%2d): readout completed (%d words)\n",pa,wc);)
     }
    sds_pat>>= 1; pa++;
   }
 }

*ptr= n_words;                          /* number of read out data words      */

return(plOK);

#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1881_readout_check(ems_u32* p)
  /* test subroutine for "fb_lc1881_readout_check" */
{
plerrcode res;
ems_u32 pat, hpat;                      /* IS module pattern                  */
int n_mod;                              /* number of modules                  */

T(test_proc_fb_lc1881_readout_check)

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
if ((res=var_read_int(p[1], &pat))!=plOK) return res;

if ((unsigned int)p[2]>3) {
  *outptr++=p[2];
  return(plErr_ArgRange);
 }

hpat=lc_pat(LC_ADC_1881, "test_lc1881_readout_check");
hpat|=lc_pat(LC_ADC_1881M, "test_lc1881_readout_check");
if ((pat&hpat)!=pat) return plErr_ArgRange;

n_mod=0;
while (pat) {
    if (pat&1) n_mod++;
    pat>>=1;
}

wirbrauchen= (n_mod*MAX_WC) + 1;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1881_readout[]="fb_lc1881_readout";
int ver_proc_fb_lc1881_readout=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1881_readout_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1881_readout()
{
return(&fb_lc1881_readout_prop);
}
#endif
/******************************************************************************/
/*
Readout of Le Croy ADC 1881
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

plerrcode proc_fb_lc1881_readout(ems_u32* p)
  /* readout of fastbus lecroy adc1881 */
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

T(proc_fb_lc1881_readout)

ptr= outptr++;  n_words= 0;

dev=modullist->entry[memberlist[1]].address.fastbus.dev;

pat= var_list[p[1]].var.val;
wt_con= MAX_WT_CON;
dev->FRCM(dev, 0xBD, &val, &ssr);
if ((val&pat)^pat) {             /* wait for end of conversion         */
  while ((wt_con--) && (dev->FRCM(dev, 0xBD, &val, &ssr), (val&pat)^pat)) {
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
  dev->FRCM(dev, 0xBD, &con_pat, &ssr);
  con_pat&=pat;
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=2;
    return(plErr_HW);
   }
  { ems_u32 unsol_mes[5]; int unsol_num;
    unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
    unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_ADC_1881;unsol_mes[4]=con_pat;
    send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
   }
 }
else {                                  /* conversion completed               */
  DV(D_DUMP,printf("ADC 1881: end of conversion after %d/%d\n",(MAX_WT_CON-wt_con),MAX_WT_CON);)
  dev->FWCM(dev, (0x5 | (p[2]<<4)),0x400, &ssr);        /* LNE for broadcast class p[2]       */
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=3;
    return(plErr_HW);
   }
  dev->FRCM(dev, 0xCD, &sds_pat, &ssr);            /* Sparse Data Scan                   */
  sds_pat&=pat;
  if (ssr) {
    outptr--;*outptr++=ssr;*outptr++=4;
    return(plErr_HW);
   }
  pa= 0;
  while (sds_pat) {                     /* valid data                         */
    if (sds_pat&0x1) {            /* block readout for geographic address "pa"*/
      wc=dev->FRDB_S(dev, pa, MAX_WC_PL, outptr, &ssr, "proc_fb_lc1881_readout")-1;  /* last data word invalid         */
      if (ssr != 2) {              /* test ss code                       */
        if (ssr == 0)
         { ems_u32 unsol_mes[5]; int unsol_num;
           unsol_num=5;unsol_mes[0]=8;unsol_mes[1]=3;
           unsol_mes[2]=trigger.eventcnt;unsol_mes[3]=LC_ADC_1881;unsol_mes[4]=wc;
           send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
          }
        else {
          *outptr++=ssr;*outptr++=5;
          return(plErr_HW);
         }
       }
      outptr+=wc;  n_words+= wc;
      DV(D_DUMP,printf("ADC 1881(%2d): readout completed (%d words)\n",pa,wc);)
     }
    sds_pat>>= 1; pa++;
   }
 }

*ptr= n_words;                          /* number of read out data words      */

return(plOK);

#undef MAX_WT_CON
}

/******************************************************************************/

plerrcode test_proc_fb_lc1881_readout(ems_u32* p)
  /* test subroutine for "fb_lc1881_readout" */
{
plerrcode res;
ems_u32 pat, hpat;                      /* IS module pattern                  */
int n_mod;                              /* number of modules                  */

T(test_proc_fb_lc1881_readout)

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
if ((res=var_read_int(p[1], &pat))!=plOK) return res;

if ((unsigned int)p[2]>3) {
  *outptr++=p[2];
  return(plErr_ArgRange);
 }

hpat=lc_pat(LC_ADC_1881, "test_lc1881_readout");
hpat|=lc_pat(LC_ADC_1881M, "test_lc1881_readout");
if ((pat&hpat)!=pat) return plErr_ArgRange;

n_mod=0;
while (pat) {
    if (pat&1) n_mod++;
    pat>>=1;
}

wirbrauchen= (n_mod*MAX_WC) + 1;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/
