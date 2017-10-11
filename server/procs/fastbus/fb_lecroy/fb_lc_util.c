/*
 * procs/fastbus/fb_lecroy/fb_lc_util.c
 * 27.12.94
 * 30.12.94  #undef for local constants
 *  1. 2.95  Unsol_Data replaced by Unsol_Warning
 * 25. 2.97  "lc1876_buf_converted", "lc1877_buf_converted" and
 *             "lc_1881_buf_converted" added for use in fb_lc1810_check
 * 15.06.2000 PW adapted for SFI/Nanobridge
 * 03.06.2002 PW multi crate support
 * 24.Jun.2002 PW secs added in lc1881_buf and lc1877_buf              
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fb_lc_util.c,v 1.16 2015/04/06 21:33:31 wuestner Exp $";

#include <config.h>
#include <errno.h>
#include <string.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <unsoltypes.h>
#include <emsctypes.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../../objects/var/variables.h"
#include "../../procprops.h"
#include "../../procs.h"
#include "../../procprops.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../objects/pi/readout.h"
#include "fb_lc_util.h"

extern ems_u32* outptr;
extern int *memberlist;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/fastbus/fb_lecroy")

/******************************************************************************/

int lc_pat(int type, char* caller)
  /* create pattern of single module type in instrumentation system memberlist*/
  /* parameter: module type (from modultypes.h) */
  /* return value: bit pattern of geographic addresses */
{
int pat;
int i;

T(lc_pat)

pat= 0;
for (i=1; i<=memberlist[0]; i++) {
    ml_entry* entry=modullist->entry+memberlist[i];
    if ((entry->modulclass==modul_fastbus) &&
        (entry->modultype == type)) {
            pat|=1<<entry->address.fastbus.pa;
   }
 }
#if 0
if (!pat) {
    printf("lc_pat(type=0x%x caller=%s): empty pattern\n", type, caller);
    printf("members: (%d)\n", memberlist[0]);
    for (i=1; i<=memberlist[0]; i++) {
        printf("[%2d] --> %2d --> 0x%x\n",
            i,
            memberlist[i],
            modullist->entry[memberlist[i]].modultype);
    }
    send_unsol_warning(7, 1, type);
 }
#endif
return(pat);
}

/******************************************************************************/
/******************************************************************************/

static int lc_list(ems_u32 type, ems_u32* ptr)
  /* create list of single module type in instrumentation system memberlist*/
  /* parameter: module type (from modultypes.h) */
  /*            pointer to module specific list */
  /* return value: number of modules in list */
{
int n_mod;
int i;

T(lc_list)

n_mod= 0;
D(D_USER,printf("mod_type %x: list of defined modules= ",(type & 0xffff));)
for (i=1; i<=memberlist[0]; i++) {
    ml_entry* entry=modullist->entry+memberlist[i];
    if (entry->modultype == type) {
        *ptr++= memberlist[i];
        D(D_USER,printf("  pa= %3x",memberlist[i]);)
        n_mod++;
    }
}

return(n_mod);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc_create_pat[]="fb_lc_create_pat";
int ver_proc_fb_lc_create_pat=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc_create_pat_prop= {0,0,0,0};

procprop* prop_proc_fb_lc_create_pat()
{
return(&fb_lc_create_pat_prop);
}
#endif
/******************************************************************************/
/* get pattern of module positions in instrumentation system from memberlist  */
/*
input parameters:
-----------------
  p[ 0]: number of parameters                      2*n- n= number of patterns
----------
  p[ 1]: module type
  p[ 2]: variable id
  ...
----------

output:
-------
  var_list[id] containing module patterns
*/
/******************************************************************************/

plerrcode proc_fb_lc_create_pat(ems_u32* p)
{
int pat,n_pat;
ems_u32 *l;
int i;

T(proc_fb_lc_create_pat)

n_pat= (*p++)/2;
l= p;

for (i=1;i<=n_pat;i++) {
  var_list[*(++l)].var.val= 0;
  l++;
 }

for (i=1;i<=n_pat;i++) {
  pat= lc_pat(*p++, "proc_fb_lc_create_pat");
  D(D_USER,printf("mod_type %x: pattern= %x\n",(*p & 0xffff),pat);)
  var_list[*p].var.val= var_list[*p].var.val | pat;
  p++;
 }

return(plOK);
}

/******************************************************************************/

plerrcode test_proc_fb_lc_create_pat(ems_u32* p)
  /* test subroutine for "proc_fb_lc_create_pat" */
{
int n_pat;
int i;

T(test_proc_fb_lc_create_pat)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

if (p[0]%2) {                           /* check number of parameters         */
  return(plErr_ArgNum);
 }

n_pat= p[0]/2;
for (i=1; i<=n_pat; i++) {              /* check variable assignments         */
  if ((unsigned int)p[2*i]>MAX_VAR) {
    *outptr++=p[2*i];
    return(plErr_IllVar);
   }
 }

wirbrauchen= 0;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc_mod_pat[]="fb_lc_mod_pat";
int ver_proc_fb_lc_mod_pat=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc_mod_pat_prop= {0,0,0,0};

procprop* prop_proc_fb_lc_mod_pat()
{
return(&fb_lc_mod_pat_prop);
}
#endif
/******************************************************************************/
/* get pattern of module positions in instrumentation system from memberlist  */
/* different module types can be combined to one bit pattern                  */
/*
input parameters:
-----------------
  p[0]:     number of parameters
----------
  p[1]:     number of module patterns for instrumentation system
----------
  p[2]:     number of module types m for pattern
  p[3]:     module type
  ...
  p[3+m-1]: module type
  p[3+m]:   variable id
----------
  ...
----------

output:
-------
  var_list[id] containing module patterns
*/
/******************************************************************************/

plerrcode proc_fb_lc_mod_pat(ems_u32* p)
{
int n_pat;                              /* number of module patterns          */
int pat,typ_pat;                    /* module pattern and module type pattern */
int n_typ;                              /* number of module types for pattern */
int i,j;

T(proc_fb_lc_mod_pat)

p++;
n_pat= *p++;

for (i=1; i<=n_pat; i++) {
  pat=0; n_typ= *p++;
  for (j=1; j<=n_typ; j++) {
    typ_pat= lc_pat(*p, "proc_fb_lc_mod_pat");
    D(D_USER,printf("mod_type %x: pattern= %x\n",((*p) & 0xffff),typ_pat);)
    pat+= typ_pat; p++;
   }
  var_list[*p].var.val= pat;
  D(D_USER,printf("%d. pattern(var %d)= %x\n",i,*p,pat);)
  p++;
 }

return(plOK);
}

/******************************************************************************/

plerrcode test_proc_fb_lc_mod_pat(ems_u32* p)
  /* test subroutine for "proc_fb_lc_mod_pat" */
{
int n_pat;                              /* number of module patterns          */
int n_typ;                              /* number of module types for pattern */
int n_p,unused;
int i;

T(test_proc_fb_lc_mod_pat)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

unused= p[0];                           /* check number of parameters         */
if (!(unused)) {
  *outptr++=0;
  return(plErr_ArgNum);
 }

n_pat= p[1];
if (!(n_pat)) {                         /* check number of patterns           */
  *outptr++=1;
  return(plErr_ArgNum);
 }

unused-= 1; n_p= 1;
for (i=1; i<=n_pat; i++) {
  if (!(unused)) {                      /* check number of parameters         */
    *outptr++=i;*outptr++=2;
    return(plErr_ArgNum);
   }
  n_typ= p[n_p+1];                /* check number of module types for pattern */
  if (!(n_typ)) {
    *outptr++=3;
    return(plErr_ArgNum);
   }
  unused-= 1; n_p+= 1;
  if (unused<(n_typ+1)) {               /* check number of parameters         */
    *outptr++=i;*outptr++=unused;*outptr++=4;
    return(plErr_ArgNum);
   }
  if ((unsigned int)p[n_p+n_typ+1]>MAX_VAR){  /* check variable assignment    */
    *outptr++=p[n_p+n_typ+1];
    return(plErr_IllVar);
   }
  if (!var_list[p[n_p+n_typ+1]].len) {
    *outptr++=p[n_p+n_typ+1];
    return(plErr_NoVar);
   }
  if (var_list[p[n_p+n_typ+1]].len!=1) {
    *outptr++=p[n_p+n_typ+1];
    return(plErr_IllVarSize);
   }
  unused-= n_typ+1; n_p+= n_typ+1;
 }

if (unused) {                           /* unused parameters left             */
  *outptr++=5;
  return(plErr_ArgNum);
 }

wirbrauchen= 0;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc_create_list[]="fb_lc_create_list";
int ver_proc_fb_lc_create_list=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc_create_list_prop= {0,0,0,0};

procprop* prop_proc_fb_lc_create_list()
{
return(&fb_lc_create_list_prop);
}
#endif
/******************************************************************************/
/* get list of module positions in instrumentation system from memberlist     */
/*
input parameters:
-----------------
  p[ 0]: number of parameters                      2*n- n= number of lists
----------
  p[ 1]: module type
  p[ 2]: variable id
  ...
----------

output:
-------
  var_list[id] containing module list
*/
/******************************************************************************/

plerrcode proc_fb_lc_create_list(ems_u32* p)
{
int n_list;
ems_u32 type;
int i;
ems_u32 *var_ptr, *l;

T(proc_fb_lc_create_list)

n_list= (*p++)/2;
l= p;
for (i=1;i<=n_list;i++) {
  *var_list[*(++l)].var.ptr= 0;
  l++;
 }

for (i=1; i<=n_list; i++) {
  type= *p++;
  var_ptr= var_list[*p].var.ptr;
  var_ptr+= *var_list[*p].var.ptr + 1;
  *var_list[*p].var.ptr+= lc_list(type,var_ptr);
  p++;
 }

return(plOK);
}

/******************************************************************************/

plerrcode test_proc_fb_lc_create_list(ems_u32* p)
  /* test subroutine for "proc_fb_lc_create_list" */
{
int n_list;
int i;

T(test_proc_fb_lc_create_list)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

if (p[0]%2) {                           /* check number of parameters         */
  return(plErr_ArgNum);
 }

n_list= p[0]/2;
for (i=1; i<=n_list; i++) {              /* check variable assignments         */
  if ((unsigned int)p[2*i]>MAX_VAR) {
    *outptr++=2*i;*outptr++=p[2*i];
    return(plErr_IllVar);
   }
 }

wirbrauchen= 0;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc_create_patlist[]="fb_lc_create_patlist";
int ver_proc_fb_lc_create_patlist=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc_create_patlist_prop= {0,0,0,0};

procprop* prop_proc_fb_lc_create_patlist()
{
return(&fb_lc_create_patlist_prop);
}
#endif
/******************************************************************************/
/* get pattern and list of module positions in is from memberlist             */
/*
input parameters:
-----------------
  p[ 0]: number of parameters                      3*n- n= number of module types
----------
  p[ 1]: module type
  p[ 2]: variable id1 (pattern)
  p[ 3]: variable id2 (list)
  ...
----------

output:
-------
  var_list[id1] containing module pattern
  var_list[id2] containing module list
*/
/******************************************************************************/

plerrcode proc_fb_lc_create_patlist(ems_u32* p)
{
ems_u32 type;
ems_u32 pat,n_pat;
int i;
ems_u32 *var_ptr;

T(proc_fb_lc_create_patlist)

n_pat= (*p++)/3;

for (i=1; i<=n_pat; i++) {
  type= *p++;
  pat= lc_pat(type, "proc_fb_lc_create_patlist");
  var_list[*p++].var.val= pat;
  var_ptr= var_list[*p].var.ptr;
  var_ptr++;
  *var_list[*p].var.ptr= lc_list(type,var_ptr);
  p++;
 }

return(plOK);
}

/******************************************************************************/

plerrcode test_proc_fb_lc_create_patlist(ems_u32* p)
  /* test subroutine for "proc_fb_lc_create_patlist" */
{
int n_pat;
int i;

T(test_proc_fb_lc_create_pat)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

if (p[0]%3) {                           /* check number of parameters         */
  return(plErr_ArgNum);
 }

n_pat= p[0]/3;
for (i=1; i<=n_pat; i++) {              /* check variable assignments         */
  if ((unsigned int)p[2*i]>MAX_VAR) {
    *outptr++=2*i;*outptr++=p[2*i];
    return(plErr_IllVar);
   }
  if (!var_list[p[2*i]].len) {
    *outptr++=2*i;*outptr++=p[2*i];
    return(plErr_NoVar);
   }
  if (var_list[p[2*i]].len!=1) {
    *outptr++=2*i;*outptr++=p[2*i];
    return(plErr_IllVarSize);
   }
  if ((unsigned int)p[3*i]>MAX_VAR) {
    *outptr++=3*i;*outptr++=p[3*i];
    return(plErr_IllVar);
   }
  if (!var_list[p[3*i]].len) {
    *outptr++=3*i;*outptr++=p[3*i];
    return(plErr_NoVar);
   }
  if (var_list[p[3*i]].len!=26) {
    *outptr++=3*i;*outptr++=p[3*i];
    return(plErr_IllVarSize);
   }
 }

wirbrauchen= 0;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/
plerrcode lc18XX_buf_A(struct fastbus_dev* dev, int idx, ems_u32 pat, int p_diff,
    int n_buf, ems_u32 modultype, ems_u32 evt, int secs, int blockcheck)
  /* check correct read and write pointer positions in ADC 1881 buffer memory
     parameter:
        pat = module pattern
        p_diff = correct difference of read and write pointer positions
        evt = id of event
  */
{
    static ems_u32 lastevt[BUFCHECK_MODULES]={0, 0, 0};
    static int nextsec[BUFCHECK_MODULES]={0, 0, 0};
    static int count[BUFCHECK_MODULES]={0, 0, 0};
    static ems_u32 last_good_event[BUFCHECK_MODULES]={0, 0, 0};

    ems_u32* blockhelp=0;
    int diff;                          /* write/read pointer position diff   */
    int err=0;
    ems_u32 pa, val;
    ems_u32 ssr;                       /* slave status response              */
    int bufmask=n_buf-1;

    T(lc18XX_buf_A)

    if (blockcheck) {
        if (evt<=last_good_event[idx]) last_good_event[idx]=0;
        *outptr++=modultype;
        *outptr++=last_good_event[idx]+1;
        *outptr++=evt;
        blockhelp=outptr++;
        last_good_event[idx]=evt;
    }
    pa= 0;
    while (pat) {
        if (pat&0x1) {
            int errdiff=0;
            int rbuf;

            dev->FRCa(dev, pa, 16, &val, &ssr); /* primary address cycle     */
            if (ssr) {
                dev->FCDISC(dev);
                *outptr++=pa;*outptr++=ssr;*outptr++=0;
                return(plErr_HW);
            }
            rbuf=(val>>8)&bufmask;
            diff= (val&bufmask) - rbuf;
            while ((diff!=p_diff) && (diff!=(p_diff-n_buf))) {
                errdiff++;
                dev->FCWWS(dev, 0, 0x400, &ssr);
                if (ssr) {                      /* load next event           */
                    dev->FCDISC(dev);
                    *outptr++=pa;*outptr++=ssr;*outptr++=1;
                    return(plErr_HW);
                }
                dev->FCRWS(dev, 16, &val, &ssr);
                if (ssr) {
                    dev->FCDISC(dev);
                    *outptr++=pa;*outptr++=ssr;*outptr++=2;
                    return(plErr_HW);
                }
                diff= (val&bufmask) - ((val>>8)&bufmask);
            }
            dev->FCDISC(dev);                   /* disconnect                */
            if (errdiff) {
                err++;
                if (blockcheck) {
                    *outptr++=pa;
                    *outptr++=errdiff;
                } else {
                    invalidate_event();
                    if (secs)
                        count[idx]++;
                    else
                        send_unsol_warning(4, 4, evt, pa, (val&bufmask), rbuf);
                }
            }
        }
        pat>>= 1; pa++;
    }
    if (blockcheck) {
        *blockhelp=err;
    }

    if (secs && count[idx]) {
        struct timeval tv;
        gettimeofday(&tv, 0);
        if (tv.tv_sec>=nextsec[idx]) {
            int tdiff=tv.tv_sec-nextsec[idx]+secs;
            int ediff=evt-lastevt[idx];
            if (ediff<0) ediff=evt;
            send_unsol_warning(9, 5, evt, modultype, ediff, tdiff, count[idx]);
            nextsec[idx]=tv.tv_sec+secs;
            lastevt[idx]=evt;
            count[idx]=0;
        }
    }

    return plOK;
}
/******************************************************************************/
static plerrcode
correct_ptr(struct fastbus_dev* dev, int pa, int p_diff, int n_buf,
    int* errdiff)
{
    ems_u32 val, ssr;
    int diff;
    int bufmask=n_buf-1;
    int count=0;

    *errdiff=0;
    dev->FRC(dev, pa, 16, &val, &ssr);
    if (ssr) {
        *outptr++=pa; *outptr++=ssr; *outptr++=0;
        return plErr_HW;
    }
    diff= (val&bufmask) - ((val>>8)&bufmask);
    while ((diff!=p_diff) && (diff!=(p_diff-n_buf))) {
        (*errdiff)++;
        dev->FWC(dev, pa, 0, 0x400, &ssr); /* load next event */
        if (ssr) {
            *outptr++=pa; *outptr++=ssr; *outptr++=1;
            return plErr_HW;
        }
        dev->FRC(dev, pa, 16, &val, &ssr);
        if (ssr) {
            *outptr++=pa; *outptr++=ssr; *outptr++=2;
            return plErr_HW;
        }
        diff=(val&bufmask) - ((val>>8)&bufmask);
        if ((diff<63) || (diff>63)) {
            printf("correct_ptr: pa=%d p_diff=%d n_buf=%d bufmask=0x%x\n",
                pa, p_diff, n_buf, bufmask);
            printf("             val=0x%08x diff=%d count=%d\n", val, diff, count);
        }
        count++;
        if (count>128) {
            printf("correct_ptr: terminated: count=%d\n", count);
            return plErr_Program;
        }
    }
    return plOK;
}
/******************************************************************************/
plerrcode lc18XX_buf_B(struct fastbus_dev* dev, int idx, ems_u32 pat, int p_diff,
    int n_buf, ems_u32 modultype, ems_u32 evt, int secs, int blockcheck)
  /* check correct read and write pointer positions in ADC 1881 buffer memory
     parameter:  pat = module pattern
                 p_diff = correct difference of read and write pointer positions
                 evt = id of event
  */
{
    static ems_u32 lastevt[BUFCHECK_MODULES]={0, 0, 0};
    static int nextsec[BUFCHECK_MODULES]={0, 0, 0};
    static int count[BUFCHECK_MODULES]={0, 0, 0};
    static ems_u32 last_good_event[BUFCHECK_MODULES]={0, 0, 0};

    ems_u32* blockhelp=0;
    int err;
    plerrcode pres;
    int i, res;
    int num_pa=0;
    ems_u32 pa[32];
    ems_u32 val[32];
    ems_u32 ssr;
    int bufmask=n_buf-1;

    T(lc18XX_buf_B)

    if (blockcheck) {
        if (evt<=last_good_event[idx]) last_good_event[idx]=0;
        *outptr++=modultype;
        *outptr++=last_good_event[idx]+1;
        *outptr++=evt;
        blockhelp=outptr++;
        last_good_event[idx]=evt;
    }

    /* generate list of pa */
    i=0; num_pa=0;
    while (pat) {
        if (pat&1) {
            pa[num_pa++]=i;
        }
        pat>>=1;
        i++;
    }

    res=dev->multiFRC(dev, num_pa, pa, 16, val, &ssr);
    if (res) {
        printf("lc18XX_buf: multi_FRC_B: %s\n", strerror(errno));
        *outptr++=errno;
        return plErr_System;
    }
    if (ssr) {
        *outptr++=pa[0]; *outptr++=ssr; *outptr++=0;
        return plErr_HW;
    }

    err=0;
    for (i=0; i<num_pa; i++) {
        int errdiff=0;
        int v=val[i];
        int diff=(v&bufmask)-((v>>8)&bufmask);
        if ((diff!=p_diff)&&(diff!=(p_diff-n_buf))) {
            pres=correct_ptr(dev, pa[i], p_diff, n_buf, &errdiff);
            if (pres) return pres;
            err++;
            if (blockcheck) {
                *outptr++=pa[i];
                *outptr++=errdiff;
            } else {
                invalidate_event();
                if (secs)
                    count[idx]++;
                else
                    send_unsol_warning(4, 4, evt, pa[i], v&bufmask, (v>>8)&bufmask);
            }
        }
    }

    if (blockcheck) {
        *blockhelp=err;
    }

    if (secs && count[idx]) {
        struct timeval tv;
        gettimeofday(&tv, 0);
        if (tv.tv_sec>=nextsec[idx]) {
            int tdiff=tv.tv_sec-nextsec[idx]+secs;
            int ediff=evt-lastevt[idx];
            if (ediff<0) ediff=evt;
            send_unsol_warning(9, 5, evt, modultype, ediff, tdiff, count[idx]);
            nextsec[idx]=tv.tv_sec+secs;
            lastevt[idx]=evt;
            count[idx]=0;
        }
    }

    return plOK;
}
/******************************************************************************/
/******************************************************************************/
plerrcode lc1876_buf_converted(struct fastbus_dev* dev, ems_u32 pat,
        int p_diff, int evt)

  /* check end of conversion and correct r/w pointer positions in TDC 1876
       buffer memory
     parameter:  pat = module pattern
                 p_diff = correct difference of read and write pointer positions
                 evt = id of event
  */

{
int diff;                               /* write/read pointer position diff   */
ems_u32 pa,val;
int wt_con;
ems_u32 con_pat;                        /* "end of conversion"-module pattern */
ems_u32 ssr;                            /* slave status response              */

#define N_BUF  8                        /* number of event buffers            */
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

T(lc1876_buf_converted)

wt_con= MAX_WT_CON;
dev->FRCM(dev, 0x9, &val, &ssr);
if ((val&pat)^pat) {             /* wait for end of conversion         */
  while ((wt_con--) && (dev->FRCM(dev, 0x9, &val, &ssr), (val&pat)^pat)) {
    if (ssr) {
      *outptr++=ssr;*outptr++=0;
      return(plErr_HW);
     }
   }
 }
if (ssr) {
  *outptr++=ssr;*outptr++=1;
  return(plErr_HW);
 }
if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  dev->FRCM(dev, 0x9, &con_pat, &ssr);
  con_pat&=pat;
  if (ssr) {
    *outptr++=ssr;*outptr++=2;
    return(plErr_HW);
   }
  *outptr++=con_pat;*outptr++=3;
  return(plErr_HWTestFailed);
 }
else {                                  /* conversion completed               */
  pa= 0;
  while (pat) {
    if (pat&0x1) {
      dev->FRCa(dev, pa, 16, &val, &ssr);       /* primary address cycle     */
      if (ssr) {
        dev->FCDISC(dev);
        *outptr++=pa;*outptr++=ssr;*outptr++=4;
        return(plErr_HW);
       }
      diff= (val&0x7) - ((val>>8)&0x7);
      while ((diff!=p_diff) && (diff!=(p_diff-N_BUF))) {
        {
          send_unsol_warning(4, 4, evt, pa, (val&0x7), ((val>>8)&0x7));
         }
        dev->FCWWS(dev, 0, 0x400, &ssr);        /* load next event           */
        if (ssr) {
          dev->FCDISC(dev);
          *outptr++=pa;*outptr++=ssr;*outptr++=5;
          return(plErr_HW);
         }
        dev->FCRWS(dev, 16, &val, &ssr);
        if (ssr) {
          dev->FCDISC(dev);
          *outptr++=pa;*outptr++=ssr;*outptr++=6;
          return(plErr_HW);
         }
        diff= (val&0x7) - ((val>>8)&0x7);
       }
      dev->FCDISC(dev);                         /* disconnect                */
     }
    pat>>= 1; pa++;
   }
 }

return(plOK);

#undef MAX_WT_CON
#undef N_BUF
}

/******************************************************************************/
/******************************************************************************/

plerrcode lc1877_buf_converted(struct fastbus_dev* dev, ems_u32 pat,
        int p_diff, int evt)
  /* check end of conversion and correct r/w pointer positions in TDC 1877
       buffer memory
     parameter:  pat = module pattern
                 p_diff = correct difference of read and write pointer positions
                 evt = id of event
  */
{
int diff;                               /* write/read pointer position diff   */
ems_u32 pa,val;
int wt_con;
ems_u32 con_pat;                        /* "end of conversion"-module pattern */
ems_u32 ssr;                            /* slave status response              */

#define N_BUF  8                        /* number of event buffers            */
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

T(lc1877_buf_converted)

wt_con= MAX_WT_CON;
dev->FRCM(dev, 0x9, &val, &ssr);
if ((val&pat)^pat) {             /* wait for end of conversion         */
  while ((wt_con--) && (dev->FRCM(dev, 0x9, &val, &ssr), (val&pat)^pat)) {
    if (ssr) {
      *outptr++=ssr;*outptr++=0;
      return(plErr_HW);
     }
   }
 }
if (ssr) {
  *outptr++=ssr;*outptr++=1;
  return(plErr_HW);
 }
if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  dev->FRCM(dev, 0x9, &con_pat, &ssr);
  con_pat&=pat;
  if (ssr) {
    *outptr++=ssr;*outptr++=2;
    return(plErr_HW);
   }
  *outptr++=con_pat;*outptr++=3;
  return(plErr_HWTestFailed);
 }
else {                                  /* conversion completed               */
  pa= 0;
  while (pat) {
    if (pat&0x1) {
      dev->FRCa(dev, pa, 16, &val, &ssr);                 /* primary address cycle              */
      if (ssr) {
        dev->FCDISC(dev);
        *outptr++=pa;*outptr++=ssr;*outptr++=4;
        return(plErr_HW);
       }
      diff= (val&0x7) - ((val>>8)&0x7);
      while ((diff!=p_diff) && (diff!=(p_diff-N_BUF))) {
        {
          send_unsol_warning(4, 4, evt, pa, (val&0x7), ((val>>8)&0x7));
         }
        dev->FCWWS(dev, 0, 0x400, &ssr);
        if (ssr) {  /* load next event                   */
          dev->FCDISC(dev);
          *outptr++=pa;*outptr++=ssr;*outptr++=5;
          return(plErr_HW);
         }
        dev->FCRWS(dev, 16, &val, &ssr);
        if (ssr) {
          dev->FCDISC(dev);
          *outptr++=pa;*outptr++=ssr;*outptr++=6;
          return(plErr_HW);
         }
        diff= (val&0x7) - ((val>>8)&0x7);
       }
      dev->FCDISC(dev);                         /* disconnect                         */
     }
    pat>>= 1; pa++;
   }
 }

return(plOK);

#undef MAX_WT_CON
#undef N_BUF
}

/******************************************************************************/
/******************************************************************************/

plerrcode lc1881_buf_converted(struct fastbus_dev* dev, ems_u32 pat,
        int p_diff, int evt)
  /* check end of conversion and correct r/w pointer positions in ADC 1881
       buffer memory
     parameter:  pat = module pattern
                 p_diff = correct difference of read and write pointer positions
                 evt = id of event
  */
{
int diff;                               /* write/read pointer position diff   */
ems_u32 pa, val;
int wt_con;
ems_u32 con_pat;                        /* "end of conversion"-module pattern */
ems_u32 ssr;                            /* slave status response              */

#define N_BUF  64                       /* number of event buffers            */
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

T(lc1881_buf_converted)

wt_con= MAX_WT_CON;
dev->FRCM(dev, 0xBD, &val, &ssr);
if ((val&pat)^pat) {             /* wait for end of conversion         */
  while ((wt_con--) && (dev->FRCM(dev, 0xBD, &val, &ssr), (val&pat)^pat)) {
    if (ssr) {
      *outptr++=ssr;*outptr++=0;
      return(plErr_HW);
     }
   }
 }
if (ssr) {
  *outptr++=ssr;*outptr++=1;
  return(plErr_HW);
 }
if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  dev->FRCM(dev, 0xBD, &con_pat, &ssr);
  con_pat&=pat;
  if (ssr) {
    *outptr++=ssr;*outptr++=2;
    return(plErr_HW);
   }
  *outptr++=con_pat;*outptr++=3;
  return(plErr_HWTestFailed);
 }
else {                                  /* conversion completed               */
  pa= 0;
  while (pat) {
    if (pat&0x1) {
      dev->FRCa(dev, pa, 16, &val, &ssr);                 /* primary address cycle              */
      if (ssr) {
        dev->FCDISC(dev);
        *outptr++=pa;*outptr++=ssr;*outptr++=4;
        return(plErr_HW);
       }
      diff= (val&0x3f) - ((val>>8)&0x3f);
      while ((diff!=p_diff) && (diff!=(p_diff-N_BUF))) {
        {
          send_unsol_warning(4, 4, evt, pa, (val&0x7), ((val>>8)&0x7));
         }
        dev->FCWWS(dev, 0, 0x400, &ssr);
        if (ssr) {  /* load next event                   */
          dev->FCDISC(dev);
          *outptr++=pa;*outptr++=ssr;*outptr++=5;
          return(plErr_HW);
         }
        dev->FCRWS(dev, 16, &val, &ssr);
        if (ssr) {
          dev->FCDISC(dev);
          *outptr++=pa;*outptr++=ssr;*outptr++=6;
          return(plErr_HW);
         }
        diff= (val&0x3f) - ((val>>8)&0x3f);
       }
      dev->FCDISC(dev);                         /* disconnect                         */
     }
    pat>>= 1; pa++;
   }
 }

return(plOK);

#undef MAX_WT_CON
#undef N_BUF
}

/******************************************************************************/
/******************************************************************************/
