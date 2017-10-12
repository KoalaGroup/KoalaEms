/*******************************************************************************
*                                                                              *
* fb_lc_util.c                                                                 *
*                                                                              *
* OS9                                                                          *
*                                                                              *
* MaWo                                                                         *
* PeWue                                                                        *
*                                                                              *
* 27.12.94                                                                     *
* 30.12.94  #undef for local constants                                         *
*  1. 2.95  Unsol_Data replaced by Unsol_Warning                               *
* 25. 2.97  "lc1876_buf_converted", "lc1877_buf_converted" and                 *
*             "lc_1881_buf_converted" added for use in fb_lc1810_check         *
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

extern int *outptr;
extern int *memberlist;
extern int wirbrauchen;

/******************************************************************************/
/******************************************************************************/

int lc_pat(type)
  /* create pattern of single module type in instrumentation system memberlist*/
  /* parameter: module type (from modultypes.h) */
  /* return value: bit pattern of geographic addresses */
int type;
{
int pat;
int i;

T(lc_pat)

pat= 0;
for (i=1; i<=memberlist[0]; i++) {
  if (get_mod_type(memberlist[i]) == type) {
    pat+= (1<<memberlist[i]);
   }
 }

if (!pat) {
  { int unsol_mes[3],unsol_num;
    unsol_num=3;unsol_mes[0]=7;unsol_mes[1]=1;
    unsol_mes[2]=type;
    send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
   }
 }

return(pat);
}

/******************************************************************************/
/******************************************************************************/

int lc_list(type,ptr)
  /* create list of single module type in instrumentation system memberlist*/
  /* parameter: module type (from modultypes.h) */
  /*            pointer to module specific list */
  /* return value: number of modules in list */
int type;
int *ptr;
{
int n_mod;
int i;

T(lc_list)

n_mod= 0;
D(D_USER,printf("mod_type %x: list of defined modules= ",(type & 0xffff));)
for (i=1; i<=memberlist[0]; i++) {
  if (get_mod_type(memberlist[i]) == type) {
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

static procprop fb_lc_create_pat_prop= {0,0,0,0};

procprop* prop_proc_fb_lc_create_pat()
{
return(&fb_lc_create_pat_prop);
}

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

plerrcode proc_fb_lc_create_pat(p)
int *p;
{
int pat,n_pat;
int i, *l;

T(proc_fb_lc_create_pat)

n_pat= (*p++)/2;
l= p;

for (i=1;i<=n_pat;i++) {
  var_list[*(++l)].var.val= 0;
  l++;
 }

for (i=1;i<=n_pat;i++) {
  pat= lc_pat(*p++);
  D(D_USER,printf("mod_type %x: pattern= %x\n",(*p & 0xffff),pat);)
  var_list[*p].var.val= var_list[*p].var.val | pat;
  p++;
 }

return(plOK);
}


/******************************************************************************/

plerrcode test_proc_fb_lc_create_pat(p)
  /* test subroutine for "proc_fb_lc_create_pat" */
int *p;
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

static procprop fb_lc_mod_pat_prop= {0,0,0,0};

procprop* prop_proc_fb_lc_mod_pat()
{
return(&fb_lc_mod_pat_prop);
}

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

plerrcode proc_fb_lc_mod_pat(p)
int *p;
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
    typ_pat= lc_pat(*p);
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

plerrcode test_proc_fb_lc_mod_pat(p)
  /* test subroutine for "proc_fb_lc_mod_pat" */
int *p;
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

static procprop fb_lc_create_list_prop= {0,0,0,0};

procprop* prop_proc_fb_lc_create_list()
{
return(&fb_lc_create_list_prop);
}

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

plerrcode proc_fb_lc_create_list(p)
int *p;
{
int n_list;
int type;
int i, *l;
int *var_ptr;

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

plerrcode test_proc_fb_lc_create_list(p)
  /* test subroutine for "proc_fb_lc_create_list" */
int *p;
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

static procprop fb_lc_create_patlist_prop= {0,0,0,0};

procprop* prop_proc_fb_lc_create_patlist()
{
return(&fb_lc_create_patlist_prop);
}

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

plerrcode proc_fb_lc_create_patlist(p)
int *p;
{
int type;
int pat,n_pat;
int i;
int *var_ptr;

T(proc_fb_lc_create_patlist)

n_pat= (*p++)/3;

for (i=1; i<=n_pat; i++) {
  type= *p++;
  pat= lc_pat(type);
  var_list[*p++].var.val= pat;
  var_ptr= var_list[*p].var.ptr;
  var_ptr++;
  *var_list[*p].var.ptr= lc_list(type,var_ptr);
  p++;
 }

return(plOK);
}


/******************************************************************************/

plerrcode test_proc_fb_lc_create_patlist(p)
  /* test subroutine for "proc_fb_lc_create_patlist" */
int *p;
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

plerrcode lc1876_buf(pat,p_diff,evt)
  /* check correct read and write pointer positions in TDC 1876 buffer memory
     parameter:  pat = module pattern
                 p_diff = correct difference of read and write pointer positions
                 evt = id of event
  */
int pat,p_diff,evt;
{
int diff;                               /* write/read pointer position diff   */
int pa,val;
int ssr;                                /* slave status response              */

#define N_BUF  8                        /* number of event buffers            */

T(lc1876_buf)

pa= 0;
while (pat) {
  if (pat&0x1) {
    val= FRCa(pa,16);                   /* primary address cycle              */
    if (sscode()) {
      FCDISC();
      *outptr++=pa;*outptr++=sscode();*outptr++=0;
      return(plErr_HW);
     }
    diff= (val&0x7) - ((val>>8)&0x7);
    while ((diff!=p_diff) && (diff!=(p_diff-N_BUF))) {
      { int unsol_mes[6],unsol_num;
        unsol_num=6;unsol_mes[0]=4;unsol_mes[1]=4;
        unsol_mes[2]=evt;unsol_mes[3]=pa;unsol_mes[4]=(val&0x7);unsol_mes[5]=((val>>8)&0x7);
        send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
       }
      if (ssr=(FCWWS(0,0x400)&0x7)) {   /* load next event                    */
        FCDISC();
        *outptr++=pa;*outptr++=ssr;*outptr++=1;
        return(plErr_HW);
       }
      val= FCRWS(16);
      if (sscode()) {
        FCDISC();
        *outptr++=pa;*outptr++=sscode();*outptr++=2;
        return(plErr_HW);
       }
      diff= (val&0x7) - ((val>>8)&0x7);
     }
    FCDISC();                           /* disconnect                         */
   }
  pat>>= 1; pa++;
 }

return(plOK);

#undef N_BUF
}


/******************************************************************************/
/******************************************************************************/

plerrcode lc1876_buf_converted(pat,p_diff,evt)
  /* check end of conversion and correct r/w pointer positions in TDC 1876
       buffer memory
     parameter:  pat = module pattern
                 p_diff = correct difference of read and write pointer positions
                 evt = id of event
  */
int pat,p_diff,evt;
{
int diff;                               /* write/read pointer position diff   */
int pa,val;
int wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
int ssr;                                /* slave status response              */

#define N_BUF  8                        /* number of event buffers            */
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

T(lc1876_buf_converted)

wt_con= MAX_WT_CON;
if ((FRCM(0x9)&pat)^pat) {             /* wait for end of conversion         */
  while ((wt_con--) && ((FRCM(0x9)&pat)^pat)) {
    if (sscode()) {
      *outptr++=sscode();*outptr++=0;
      return(plErr_HW);
     }
   }
 }
if (sscode()) {
  *outptr++=sscode();*outptr++=1;
  return(plErr_HW);
 }
if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  con_pat= FRCM(0x9) & pat;
  if (sscode()) {
    *outptr++=sscode();*outptr++=2;
    return(plErr_HW);
   }
  *outptr++=con_pat;*outptr++=3;
  return(plErr_HWTestFailed);
 }
else {                                  /* conversion completed               */
  pa= 0;
  while (pat) {
    if (pat&0x1) {
      val= FRCa(pa,16);                 /* primary address cycle              */
      if (sscode()) {
        FCDISC();
        *outptr++=pa;*outptr++=sscode();*outptr++=4;
        return(plErr_HW);
       }
      diff= (val&0x7) - ((val>>8)&0x7);
      while ((diff!=p_diff) && (diff!=(p_diff-N_BUF))) {
        { int unsol_mes[6],unsol_num;
          unsol_num=6;unsol_mes[0]=4;unsol_mes[1]=4;
          unsol_mes[2]=evt;unsol_mes[3]=pa;unsol_mes[4]=(val&0x7);unsol_mes[5]=((val>>8)&0x7);
          send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
         }
        if (ssr=(FCWWS(0,0x400)&0x7)) {  /* load next event                   */
          FCDISC();
          *outptr++=pa;*outptr++=ssr;*outptr++=5;
          return(plErr_HW);
         }
        val= FCRWS(16);
        if (sscode()) {
          FCDISC();
          *outptr++=pa;*outptr++=sscode();*outptr++=6;
          return(plErr_HW);
         }
        diff= (val&0x7) - ((val>>8)&0x7);
       }
      FCDISC();                         /* disconnect                         */
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

plerrcode lc1877_buf(pat,p_diff,evt)
  /* check correct read and write pointer positions in TDC 1877 buffer memory
     parameter:  pat = module pattern
                 p_diff = correct difference of read and write pointer positions
                 evt = id of event
  */
int pat,p_diff,evt;
{
int diff;                               /* write/read pointer position diff   */
int pa,val;
int ssr;                                /* slave status response              */

#define N_BUF  8                        /* number of event buffers            */

T(lc1877_buf)

pa= 0;
while (pat) {
  if (pat&0x1) {
    val= FRCa(pa,16);                   /* primary address cycle              */
    if (sscode()) {
      FCDISC();
      *outptr++=pa;*outptr++=sscode();*outptr++=0;
      return(plErr_HW);
     }
    diff= (val&0x7) - ((val>>8)&0x7);
    while ((diff!=p_diff) && (diff!=(p_diff-N_BUF))) {
      { int unsol_mes[6],unsol_num;
        unsol_num=6;unsol_mes[0]=4;unsol_mes[1]=4;
        unsol_mes[2]=evt;unsol_mes[3]=pa;unsol_mes[4]=(val&0x7);unsol_mes[5]=((val>>8)&0x7);
        send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
       }
      if (ssr=(FCWWS(0,0x400)&0x7)) {   /* load next event                    */
        FCDISC();
        *outptr++=pa;*outptr++=ssr;*outptr++=1;
        return(plErr_HW);
       }
      val= FCRWS(16);
      if (sscode()) {
        FCDISC();
        *outptr++=pa;*outptr++=sscode();*outptr++=2;
        return(plErr_HW);
       }
      diff= (val&0x7) - ((val>>8)&0x7);
     }
    FCDISC();                           /* disconnect                         */
   }
  pat>>= 1; pa++;
 }

return(plOK);

#undef N_BUF
}


/******************************************************************************/
/******************************************************************************/

plerrcode lc1877_buf_converted(pat,p_diff,evt)
  /* check end of conversion and correct r/w pointer positions in TDC 1877
       buffer memory
     parameter:  pat = module pattern
                 p_diff = correct difference of read and write pointer positions
                 evt = id of event
  */
int pat,p_diff,evt;
{
int diff;                               /* write/read pointer position diff   */
int pa,val;
int wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
int ssr;                                /* slave status response              */

#define N_BUF  8                        /* number of event buffers            */
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

T(lc1877_buf_converted)

wt_con= MAX_WT_CON;
if ((FRCM(0x9)&pat)^pat) {             /* wait for end of conversion         */
  while ((wt_con--) && ((FRCM(0x9)&pat)^pat)) {
    if (sscode()) {
      *outptr++=sscode();*outptr++=0;
      return(plErr_HW);
     }
   }
 }
if (sscode()) {
  *outptr++=sscode();*outptr++=1;
  return(plErr_HW);
 }
if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  con_pat= FRCM(0x9) & pat;
  if (sscode()) {
    *outptr++=sscode();*outptr++=2;
    return(plErr_HW);
   }
  *outptr++=con_pat;*outptr++=3;
  return(plErr_HWTestFailed);
 }
else {                                  /* conversion completed               */
  pa= 0;
  while (pat) {
    if (pat&0x1) {
      val= FRCa(pa,16);                 /* primary address cycle              */
      if (sscode()) {
        FCDISC();
        *outptr++=pa;*outptr++=sscode();*outptr++=4;
        return(plErr_HW);
       }
      diff= (val&0x7) - ((val>>8)&0x7);
      while ((diff!=p_diff) && (diff!=(p_diff-N_BUF))) {
        { int unsol_mes[6],unsol_num;
          unsol_num=6;unsol_mes[0]=4;unsol_mes[1]=4;
          unsol_mes[2]=evt;unsol_mes[3]=pa;unsol_mes[4]=(val&0x7);unsol_mes[5]=((val>>8)&0x7);
          send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
         }
        if (ssr=(FCWWS(0,0x400)&0x7)) {  /* load next event                   */
          FCDISC();
          *outptr++=pa;*outptr++=ssr;*outptr++=5;
          return(plErr_HW);
         }
        val= FCRWS(16);
        if (sscode()) {
          FCDISC();
          *outptr++=pa;*outptr++=sscode();*outptr++=6;
          return(plErr_HW);
         }
        diff= (val&0x7) - ((val>>8)&0x7);
       }
      FCDISC();                         /* disconnect                         */
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

plerrcode lc1881_buf(pat,p_diff,evt)
  /* check correct read and write pointer positions in ADC 1881 buffer memory
     parameter:  pat = module pattern
                 p_diff = correct difference of read and write pointer positions
                 evt = id of event
  */
int pat,p_diff,evt;
{
int diff;                               /* write/read pointer position diff   */
int pa,val;
int ssr;                                /* slave status response              */

#define N_BUF  64                       /* number of event buffers            */

T(lc1881_buf)

pa= 0;
while (pat) {
  if (pat&0x1) {
    val= FRCa(pa,16);                   /* primary address cycle              */
    if (sscode()) {
      FCDISC();
      *outptr++=pa;*outptr++=sscode();*outptr++=0;
      return(plErr_HW);
     }
    diff= (val&0x3f) - ((val>>8)&0x3f);
    while ((diff!=p_diff) && (diff!=(p_diff-N_BUF))) {
      { int unsol_mes[6],unsol_num;
        unsol_num=6;unsol_mes[0]=4;unsol_mes[1]=4;
        unsol_mes[2]=evt;unsol_mes[3]=pa;unsol_mes[4]=(val&0x3f);unsol_mes[5]=((val>>8)&0x3f);
        send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
       }
      if (ssr=(FCWWS(0,0x400)&0x7)) {   /* load next event                    */
        FCDISC();
        *outptr++=pa;*outptr++=ssr;*outptr++=1;
        return(plErr_HW);
       }
      val= FCRWS(16);
      if (sscode()) {
        FCDISC();
        *outptr++=pa;*outptr++=sscode();*outptr++=2;
        return(plErr_HW);
       }
      diff= (val&0x3f) - ((val>>8)&0x3f);
     }
    FCDISC();                           /* disconnect                         */
   }
  pat>>= 1; pa++;
 }

return(plOK);

#undef N_BUF
}


/******************************************************************************/
/******************************************************************************/

plerrcode lc1881_buf_converted(pat,p_diff,evt)
  /* check end of conversion and correct r/w pointer positions in ADC 1881
       buffer memory
     parameter:  pat = module pattern
                 p_diff = correct difference of read and write pointer positions
                 evt = id of event
  */
int pat,p_diff,evt;
{
int diff;                               /* write/read pointer position diff   */
int pa,val;
int wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
int ssr;                                /* slave status response              */

#define N_BUF  64                       /* number of event buffers            */
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

T(lc1881_buf_converted)

wt_con= MAX_WT_CON;
if ((FRCM(0xBD)&pat)^pat) {             /* wait for end of conversion         */
  while ((wt_con--) && ((FRCM(0xBD)&pat)^pat)) {
    if (sscode()) {
      *outptr++=sscode();*outptr++=0;
      return(plErr_HW);
     }
   }
 }
if (sscode()) {
  *outptr++=sscode();*outptr++=1;
  return(plErr_HW);
 }
if (wt_con==-1) {                   /* time elapsed: conversion not completed */
  con_pat= FRCM(0xBD) & pat;
  if (sscode()) {
    *outptr++=sscode();*outptr++=2;
    return(plErr_HW);
   }
  *outptr++=con_pat;*outptr++=3;
  return(plErr_HWTestFailed);
 }
else {                                  /* conversion completed               */
  pa= 0;
  while (pat) {
    if (pat&0x1) {
      val= FRCa(pa,16);                 /* primary address cycle              */
      if (sscode()) {
        FCDISC();
        *outptr++=pa;*outptr++=sscode();*outptr++=4;
        return(plErr_HW);
       }
      diff= (val&0x3f) - ((val>>8)&0x3f);
      while ((diff!=p_diff) && (diff!=(p_diff-N_BUF))) {
        { int unsol_mes[6],unsol_num;
          unsol_num=6;unsol_mes[0]=4;unsol_mes[1]=4;
          unsol_mes[2]=evt;unsol_mes[3]=pa;unsol_mes[4]=(val&0x7);unsol_mes[5]=((val>>8)&0x7);
          send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
         }
        if (ssr=(FCWWS(0,0x400)&0x7)) {  /* load next event                   */
          FCDISC();
          *outptr++=pa;*outptr++=ssr;*outptr++=5;
          return(plErr_HW);
         }
        val= FCRWS(16);
        if (sscode()) {
          FCDISC();
          *outptr++=pa;*outptr++=sscode();*outptr++=6;
          return(plErr_HW);
         }
        diff= (val&0x3f) - ((val>>8)&0x3f);
       }
      FCDISC();                         /* disconnect                         */
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
