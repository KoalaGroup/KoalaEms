/*******************************************************************************
*                                                                              *
* fb_lc1810_init.c                                                             *
*                                                                              *
* OS9                                                                          *
*                                                                              *
* MaWo                                                                         *
* PeWue                                                                        *
* MiZi                                                                         *
*                                                                              *
* 29.12.94                                                                     *
*  1. 2.95  Unsol_Data replaced by Unsol_Warning                               *
* 25. 2.97  procedure name reassignment:                                       *
*             fb_lc1810_reset       <--> fb_lc1810_reset;1                     *
*             fb_lc1810_check_reset <--> fb_lc1810_reset;2                     *
*           "fb_lc1810_check" and "fb_lc1810_check_start" added                *
*             fb_lc1810_check       <--> fb_lc1810_check;1                     *
*             fb_lc1810_check_start <--> fb_lc1810_check;2                     *
* 09.10.97  compatible with ADC 1881M                                          *
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

#define SETUP_PAR  3                    /* number of setup parameters         */

extern int *outptr;
extern int *memberlist;
extern int wirbrauchen;
extern int eventcnt;

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1810_setup[]="fb_lc1810_setup";
int ver_proc_fb_lc1810_setup=1;

/******************************************************************************/

static procprop fb_lc1810_setup_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1810_setup()
{
return(&fb_lc1810_setup_prop);
}

/******************************************************************************/
/*
Setup of LeCroy CAT 1810
========================

parameters:
-----------
  p[ 1] : number of cat1810 modules

module specific parameters:
---------------------------
  p[ 2]: (DSR# 4< 0: 5>) FCW                       0..63- (n * 1024) + 100 ns
  p[ 3]: (DSR# 2< 0: 7>) TDC1879 frequency ref.    6..131- 1 / (1.6e-11*(256-n)*"tdc")
                                                      "tdc"= 2^(csr#0<1>)
  p[ 4]: (CSR# 0< 1>)    test selection:           0- no test
*/
/******************************************************************************/

plerrcode proc_fb_lc1810_setup(p)      
int *p;
{
int pa,sa,val,r_val;
int i;
int ssr;                                 /* slave status response             */

T(proc_fb_lc1810_setup)

p++;
i= 0; pa= 0;
do {
  i++;
  if (get_mod_type(memberlist[i])==LC_CAT_1810) {
    pa= memberlist[i];
    D(D_USER,printf("CAT 1810: pa= %d\n",pa);)

    sa= 4;                              /******           dsr#4          ******/
    val= p[1];                          /* fast clear window                  */
    FWDa(pa,sa,val);                    /* primary address cycle              */
    if (sscode()) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=0;
      return(plErr_HW);
     }
    r_val= FCRW() & 0x3f;
    if (sscode()) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=1;
      return(plErr_HW);
     }
    if (r_val^val) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=2;
      return(plErr_HW);
     }

    sa= 2;                              /******           dsr#2          ******/
    val= p[2];                          /* tdc1879 frequency reference        */
    if (ssr=(FCWWS(sa,val)&0x7)) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=3;
      return(plErr_HW);
     }
    r_val= FCRW() & 0xff;
    FCDISC();                           /* disconnect                         */
    if (sscode()) {
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=4;
      return(plErr_HW);
     }
    if (r_val^val) {
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=5;
      return(plErr_HW);
     }

    sa= 1;                              /******           csr#1          ******/
    val= p[3];                          /* test selection                     */
    FWCa(pa,sa,val);                    /* primary address cycle              */
    if (sscode()) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=6;
      return(plErr_HW);
     }
    r_val= FCRW() & 0x7;
    FCDISC();                           /* disconnect                         */
    if (sscode()) {
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=7;
      return(plErr_HW);
     }
    if (r_val) {
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=8;
      return(plErr_HW);
     }

    FWCM(0x8d,0);                       /* clear module                       */
    if (sscode()) {
      *outptr++=pa;*outptr++=sscode();*outptr++=9;
      return(plErr_HW);
     }
    tsleep(2);

    sa= 1;                              /******           csr#1          ******/
    r_val= FRC(pa,sa) & 0x38;
    if (sscode()) {
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=10;
      return(plErr_HW);
     }
    if (r_val) {                        /* error bits not cleared             */
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=11;
      return(plErr_HW);
     }

    D(D_USER, printf("CAT 1810(%2d): setup done\n",pa);)
    p+= SETUP_PAR;
   }
 } while ((i<memberlist[0]) && (!pa));

return(plOK);
}

/******************************************************************************/

plerrcode test_proc_fb_lc1810_setup(p)       
  /* test subroutine for "proc_fb_lc1810_setup" */
int *p;
{
int pa; 
int n_p,unused;
int i,i_p; 
int msk_p[3]={0x3f,0xff,0x0};

T(test_proc_fb_lc1810_setup)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

unused= p[0];
D(D_USER,printf("CAT 1810: number of parameters for all modules= %d\n",unused);)

unused-= 1; n_p= 1;
if (unused!=SETUP_PAR) {                /* check number of parameters         */
  *outptr++=0;
  return(plErr_ArgNum);
 }
if (p[1]!=1) {
  *outptr++=1;
  return(plErr_ArgNum);
 }

for (i=1; i<=memberlist[0]; i++) {
  if (get_mod_type(memberlist[i])==LC_CAT_1810) {
    pa= memberlist[i];
    D(D_USER, printf("CAT 1810(%2d): number of parameters not used= %d\n",pa,unused);)
    if (unused<SETUP_PAR) { /* not enough parameters for modules defined in IS*/
      *outptr++=pa;*outptr++=unused;
      return(plErr_ArgNum);
     }
    for (i_p=1; i_p<=SETUP_PAR; i_p++) {  /* check parameter ranges           */
      if (((unsigned int)p[n_p+i_p]&msk_p[i_p-1])!=(unsigned int)p[n_p+i_p]) {
        *outptr++=pa;*outptr++=i_p;*outptr++=0;
        return(plErr_ArgRange);
       }
     }
    if (((unsigned int)p[n_p+2]<6) || ((unsigned int)p[n_p+2]>131)) {
      *outptr++=pa;*outptr++=2;*outptr++=1;
      return(plErr_ArgRange);
     }
    n_p+= SETUP_PAR; unused-= SETUP_PAR;
   }
 }

if (unused) {                           /* unused parameters left             */
  *outptr++=2;
  return(plErr_ArgNum);
 }

wirbrauchen= 4;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/

plerrcode lc1810_check_reset(pa,evt)
 /* check for multiple occurence of gate signals and reset lecroy cat1810 */
int pa,evt;
{
int err_flag;                           /* error flag bit pattern             */

T(lc1810_check_reset)

err_flag= FRC(pa,1) & 0x38;             /* check multiple gate occurence      */
if (sscode()) {
  *outptr++=pa;*outptr++=sscode();*outptr++=0;
  return(plErr_HW);
 }
if (err_flag) {
  { int unsol_mes[6],unsol_num;
    unsol_num=5;unsol_mes[0]=5;unsol_mes[1]=3;
    unsol_mes[2]=evt;unsol_mes[3]=pa;unsol_mes[4]=err_flag;
    send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
   }
 }

FWCM(0x8d,0);                           /* clear module                       */
if (sscode()) {
  *outptr++=sscode();*outptr++=1;
  return(plErr_HW);
 }

return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1810_check_start[]="fb_lc1810_check";
int ver_proc_fb_lc1810_check_start=2;

/******************************************************************************/

static procprop fb_lc1810_check_start_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1810_check_start()
{
return(&fb_lc1810_check_start_prop);
}

/******************************************************************************/
/*
Multiple Trigger Test of LeCroy CAT 1810
========================================

parameters:
-----------
  p[ 0] : 1 (number of parameters)
  p[ 1] : pa - geographic address
*/
/******************************************************************************/

plerrcode proc_fb_lc1810_check_start(p)
  /* check on multiple gate occurence for fastbus lecroy cat1810              */
  /* r/w pointer positions in multi--event modules are checked and corrected  */
  /*   at start of readout */
int *p;
{
int err_flag;                           /* error flag bit pattern             */
int pat;                                /* IS module pattern                  */
plerrcode buf_check;

T(proc_fb_lc1810_check_start)

err_flag= FRC(p[1],1) & 0x38;           /* check multiple gate occurence      */
if (sscode()) {
  *outptr++=p[1];*outptr++=sscode();*outptr++=0;
  return(plErr_HW);
 }
if (err_flag) {                         /* multiple gate occured              */
  if (eventcnt==1) {             /* check and correct buffer ptrs at start ro */
    { int unsol_mes[6],unsol_num;
      unsol_num=5;unsol_mes[0]=5;unsol_mes[1]=3;
      unsol_mes[2]=eventcnt;unsol_mes[3]=p[1];unsol_mes[4]=err_flag;
      send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
     }
    if (pat=lc_pat(LC_TDC_1876)) {      /* check r/w ptr for tdc1876          */
      if ((buf_check=lc1876_buf_converted(pat,2,eventcnt))!=plOK) {
        *outptr++=LC_TDC_1876;*outptr++=1;
        return(buf_check);
       }
     }
    if (pat=lc_pat(LC_TDC_1877)) {      /* check r/w ptr for tdc1877          */
      if ((buf_check=lc1877_buf_converted(pat,2,eventcnt))!=plOK) {
        *outptr++=LC_TDC_1877;*outptr++=2;
        return(buf_check);
       }
     }
    if (pat=lc_pat(LC_ADC_1881)) {      /* check r/w ptr for adc1881          */
      if ((buf_check=lc1881_buf_converted(pat,2,eventcnt))!=plOK) {
        *outptr++=LC_ADC_1881;*outptr++=3;
        return(buf_check);
       }
     }
    if (pat=lc_pat(LC_ADC_1881M)) {     /* check r/w ptr for adc1881m         */
      if ((buf_check=lc1881_buf_converted(pat,2,eventcnt))!=plOK) {
        *outptr++=LC_ADC_1881M;*outptr++=4;
        return(buf_check);
       }
     }
   }
  else {                                /* multiple gate error                */
    *outptr++=p[1];*outptr++=err_flag;*outptr++=5;
    return(plErr_HW);
   }
 }

return(plOK);
}

/******************************************************************************/

plerrcode test_proc_fb_lc1810_check_start(p)
  /* test subroutine for "proc_fb_lc1810_check_start" */
int *p;
{
int isml_def;
int i;

T(test_proc_fb_lc1810_check_start)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

if (p[0]!=1) {                          /* check number of parameters         */
  return(plErr_ArgNum);
 }
                                        /* check parameter values             */
if ((unsigned int)p[1]>25) {
  *outptr++=p[1];
  return(plErr_ArgRange);
 }

isml_def= 0;
i= 1;
do {
  if (p[1]==memberlist[i]) {
    if (get_mod_type(memberlist[i])!=LC_CAT_1810) {
      *outptr++=p[1];
      return(plErr_BadModTyp);
     }
    isml_def++;
   }
  i++;
 } while ((i<=memberlist[0]) && (!isml_def));
if (!isml_def) {                        /* module not defined in IS           */
  *outptr++=p[1];
  return(plErr_BadISModList);
 }

wirbrauchen= 5;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1810_check[]="fb_lc1810_check";
int ver_proc_fb_lc1810_check=1;

/******************************************************************************/

static procprop fb_lc1810_check_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1810_check()
{
return(&fb_lc1810_check_prop);
}

/******************************************************************************/
/*
Multiple Trigger Test of LeCroy CAT 1810
========================================

parameters:
-----------
  p[ 0] : 1 (number of parameters)
  p[ 1] : pa - geographic address
*/
/*****************************************************************************/

plerrcode proc_fb_lc1810_check(p)      
  /* check on multiple gate occurence for fastbus lecroy cat1810              */
  /* r/w pointer positions in multi--event modules are checked and corrected  */
int *p;
{
int err_flag;                           /* error flag bit pattern             */
int pat;                                /* IS module pattern                  */
plerrcode buf_check;

T(proc_fb_lc1810_check)

err_flag= FRC(p[1],1) & 0x38;           /* check multiple gate occurence      */
if (sscode()) {
  *outptr++=p[1];*outptr++=sscode();*outptr++=0;
  return(plErr_HW);
 }
if (err_flag) {                         /* multiple gate occured              */
  { int unsol_mes[6],unsol_num;
    unsol_num=5;unsol_mes[0]=5;unsol_mes[1]=3;
    unsol_mes[2]=eventcnt;unsol_mes[3]=p[1];unsol_mes[4]=err_flag;
    send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
   }
  if (pat=lc_pat(LC_TDC_1876)) {        /* check r/w ptr for tdc1876          */
    if ((buf_check=lc1876_buf_converted(pat,2,eventcnt))!=plOK) {
      *outptr++=LC_TDC_1876;*outptr++=1;
      return(buf_check);
     }
   }
  if (pat=lc_pat(LC_TDC_1877)) {        /* check r/w ptr for tdc1877          */
    if ((buf_check=lc1877_buf_converted(pat,2,eventcnt))!=plOK) {
      *outptr++=LC_TDC_1877;*outptr++=2;
      return(buf_check);
     }
   }
  if (pat=lc_pat(LC_ADC_1881)) {        /* check r/w ptr for adc1881          */
    if ((buf_check=lc1881_buf_converted(pat,2,eventcnt))!=plOK) {
      *outptr++=LC_ADC_1881;*outptr++=3;
      return(buf_check);
     }
   }
  if (pat=lc_pat(LC_ADC_1881M)) {       /* check r/w ptr for adc1881          */
    if ((buf_check=lc1881_buf_converted(pat,2,eventcnt))!=plOK) {
      *outptr++=LC_ADC_1881;*outptr++=4;
      return(buf_check);
     }
   }
 }

return(plOK);
}

/******************************************************************************/

plerrcode test_proc_fb_lc1810_check(p)
  /* test subroutine for "proc_fb_lc1810_check" */
int *p;
{
int isml_def;
int i;

T(test_proc_fb_lc1810_check)

if (memberlist==0)                      /* check memberlist                   */
  return(plErr_NoISModulList);
  
if (memberlist[0]==0)
  return(plErr_BadISModList);

if (p[0]!=1) {                          /* check number of parameters         */
  return(plErr_ArgNum);
 }
                                        /* check parameter values             */
if ((unsigned int)p[1]>25) {
  *outptr++=p[1];
  return(plErr_ArgRange);
 }

isml_def= 0;
i= 1;
do {
  if (p[1]==memberlist[i]) {
    if (get_mod_type(memberlist[i])!=LC_CAT_1810) {
      *outptr++=p[1];
      return(plErr_BadModTyp);
     }
    isml_def++;
   }
  i++;
 } while ((i<=memberlist[0]) && (!isml_def));
 
if (!isml_def) {                        /* module not defined in IS           */
  *outptr++=p[1];
  return(plErr_BadISModList);
 }

wirbrauchen= 5;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1810_check_reset[]="fb_lc1810_reset";
int ver_proc_fb_lc1810_check_reset=2;

/******************************************************************************/

static procprop fb_lc1810_check_reset_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1810_check_reset()
{
return(&fb_lc1810_check_reset_prop);
}

/******************************************************************************/
/*
Reset of LeCroy CAT 1810
========================

parameters:
-----------
  p[ 0] : 1 (number of parameters)
  p[ 1] : pa - geographic address
*/
/******************************************************************************/

plerrcode proc_fb_lc1810_check_reset(p)
  /* reset of fastbus lecroy cat1810 and check on multiple gate occurence     */
int *p;
{
int err_flag;                           /* error flag bit pattern             */

T(proc_fb_lc1810_check_reset)

err_flag= FRC(p[1],1) & 0x38;           /* check multiple gate occurence      */
if (sscode()) {
  *outptr++=p[1];*outptr++=sscode();*outptr++=0;
  return(plErr_HW);
 }
if (err_flag) {                         /* multiple gate occured              */
  *outptr++=p[1];*outptr++=err_flag;*outptr++=1;
  return(plErr_HW);
 }

FWCM(0x8d,0);                           /* clear module                       */
if (sscode()) {
  *outptr++=sscode();*outptr++=2;
  return(plErr_HW);
 }

return(plOK);
}

/******************************************************************************/

plerrcode test_proc_fb_lc1810_check_reset(p)
  /* test subroutine for "proc_fb_lc1810_check_reset" */
int *p;
{
int isml_def;
int i;

T(test_proc_fb_lc1810_check_reset)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

if (p[0]!=1) {                          /* check number of parameters         */
  return(plErr_ArgNum);
 }
                                        /* check parameter values             */
if ((unsigned int)p[1]>25) {
  *outptr++=p[1];
  return(plErr_ArgRange);
 }

isml_def= 0;
i= 1;
do {
  if (p[1]==memberlist[i]) {
    if (get_mod_type(memberlist[i])!=LC_CAT_1810) {
      *outptr++=p[1];
      return(plErr_BadModTyp);
     }
    isml_def++;
   }
  i++;
 } while ((i<=memberlist[0]) && (!isml_def));
if (!isml_def) {                        /* module not defined in IS           */
  *outptr++=p[1];
  return(plErr_BadISModList);
 }

wirbrauchen= 3;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1810_reset[]="fb_lc1810_reset";
int ver_proc_fb_lc1810_reset=1;

/******************************************************************************/

static procprop fb_lc1810_reset_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1810_reset()
{
return(&fb_lc1810_reset_prop);
}

/******************************************************************************/
/*
Reset of LeCroy CAT 1810
========================
*/
/******************************************************************************/

plerrcode proc_fb_lc1810_reset()      
{

T(proc_fb_lc1810_reset)

FWCM(0x8d,0);                           /* clear module                       */
if (sscode()) {
  *outptr++=sscode();*outptr++=0;
  return(plErr_HW);
 }

return(plOK);
}

/******************************************************************************/

plerrcode test_proc_fb_lc1810_reset()       
  /* test subroutine for "proc_fb_lc1810_reset" */
{
int pa,i;

T(test_proc_fb_lc1810_reset)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

i= 0; pa= 0;                            /* check definition of cat1810 in IS  */
do {
  i++;
  if (get_mod_type(memberlist[i])==LC_CAT_1810) {
    pa= memberlist[i];
   }
 } while ((i<memberlist[0]) && (!pa));

if (!pa) {                              /* cat 1810 not defined in IS         */
  return(plErr_NotInModList);
 }

wirbrauchen= 2;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/
