/*******************************************************************************
*                                                                              *
* fb_lc1877_init.c                                                             *
*                                                                              *
* OS9                                                                          *
*                                                                              *
* MaWo                                                                         *
* PeWue                                                                        *
*                                                                              *
* 28.12.94                                                                     *
* 26. 5.98  MDT: Enable Priming on LNE added (CSD#0<8>)                        *
*                                                                              *
*                                                                              *
*******************************************************************************/
#include <config.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include "../../../lowlevel/chi_neu/fbacc.h"
#include "../../../objects/var/variables.h"
#include "../../procprops.h"

#define SETUP_PAR  11                   /* number of setup parameters         */

extern int *outptr;
extern int *memberlist;
extern int wirbrauchen;

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1877_setup[]="fb_lc1877_setup";
int ver_proc_fb_lc1877_setup=1;

/******************************************************************************/

static procprop fb_lc1877_setup_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1877_setup()
{
return(&fb_lc1877_setup_prop);
}

/******************************************************************************/
/*
Setup of LeCroy TDC 1877
========================

parameters:
-----------
  p[ 1] : number of tdc1877 modules

module specific parameters:
---------------------------
  p[ 2]: (CSR# 1<31>)    mode:                     0- common stop, 1- common start
  p[ 3]: (CSR# 1< 1>)    common source:            0- front panel, 1- backplane
  p[ 4]: (CSR# 1<29:30>) edge detection:           0- no edge detection
                                                   1- only trailing edge detection
                                                   2- only leading edge detection
                                                   3- leading & trailing edge detection
  p[ 5]: (CSR# 1< 2>)    FCW via backplane:        0- disable, 1- enable
  p[ 6]: (CSR# 1<24:27>) internal FCW:             0- 2^10 ns
                                                   1..13 (odd)-
                                                      2^((n+1)/2) * 2^10 ns
                                                   2..12 (even)-
                                                      (2^(n/2) + 2^((n/2) - 1)) * 2^10 ns
                                                  14,15-
                                                      2^(n-6) * 2^10 ns
  p[ 7]: (CSR# 1< 3>)    Fast Clear via backplane: 0- disable, 1- enable
  p[ 8]: (CSR# 7< 0: 7>) broadcast class           1..15- broadcast class 0..3
                                                      (class n <==> bit n= 1)
  p[ 9]: (CSR#18< 0: 3>) LIFO depth:               0-    16 hits registered
                                                   1..15- n hits registered
  p[10]: (CSR#18< 4:15>) full scale time:          0..4095- n*8 ns
         (= fff(h) if p[ 2]= 1)
  p[11]: (CSR# 1< 4: 7>) common start timeout:     0- front panel
                                                   1..10- internal timer: timeout after 2^(5+n) ns
                                                  10..15- internal timer: timeout after 32.768 ns
         (ignored if parameter p[ 2]= 0)
  p[12]: (CSR# 0<11:12>) multiblock configuration  0- bypass
                                                   1- primary link
                                                   2- end link
                                                   3- middle link
*/
/******************************************************************************/

plerrcode proc_fb_lc1877_setup(p)      
  /* setup of fastbus lecroy tdc1877 */
int *p;
{
int pa,sa,val,r_val;
int i;
int ssr;                                 /* slave status response             */

T(proc_fb_lc1877_setup)

p++;
for (i=1; i<=memberlist[0]; i++) {
  if (get_mod_type(memberlist[i])==LC_TDC_1877) {
    pa= memberlist[i];
    D(D_USER,printf("TDC 1877: pa= %d\n",pa);)

    sa= 0;                              /******           csr#0          ******/
    FWCa(pa,sa,1<<30);                  /* master reset & primary address     */
    if (sscode()) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=0;
      return(plErr_HW);
     }
    val= p[11]*(1<<11);                 /* mult mod data transfer             */
    if (p[11])
      val= val | (1<<8);                /* priming on lne enable (mdt only)   */
    FCWW(val);
    if (sscode()) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=1;
      return(plErr_HW);
     }
    r_val= FCRW() & 0x1900;
    if (sscode()) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=2;
      return(plErr_HW);
     }
    if (r_val^val) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=3;
      return(plErr_HW);
     }

    sa= 1;                              /******          csr#1           ******/
    val= p[1]*(1<<31) |                 /* mode: common start/stop            */
         p[2]*(1<< 1) |                 /* common start/stop source           */
         p[3]*(1<<29) |                 /* edge detection                     */
         p[4]*(1<< 2) |                 /* enable FCW via backplane           */
         p[5]*(1<<24) |                 /* internal FCW                       */
         p[6]*(1<< 3);                  /* enable fast clear via backplane    */
    if (p[1]) {                         /* common start:                      */
      val= val | (p[10]*(1<< 4));       /*   timeout                          */
     }
    if (ssr=(FCWWS(sa,val)&0x7)) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=4;
      return(plErr_HW);
     }
    r_val= FCRW();
    if (sscode()) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=5;
      return(plErr_HW);
     }
    if (r_val^val) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=6;
      return(plErr_HW);
     }

    sa= 7;                              /******          csr#7           ******/
    val= p[7];                          /* broadcast classes                  */
    if (ssr=(FCWWS(sa,val)&0x7)) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=7;
      return(plErr_HW);
     }
    r_val= FCRW() & 0xf;
    if (sscode()) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=8;
      return(plErr_HW);
     }
    if (r_val^val) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=9;
      return(plErr_HW);
     }

    sa= 18;                             /******          csr#18          ******/
    if (p[1])
      val= 0xfff * (1<<4);              /* common start: maximum full scale   */
    else
      val= p[9] * (1<<4);               /* full scale                         */
    val= val | p[8];                    /* LIFO depth                         */
    if (ssr=(FCWWS(sa,val)&0x7)) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=10;
      return(plErr_HW);
     }
    r_val= FCRW();
    FCDISC();                           /* disconnect                         */
    if (sscode()) {
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=11;
      return(plErr_HW);
     }
    if (r_val^val) {
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=12;
      return(plErr_HW);
     }

    D(D_USER, printf("TDC 1877(%2d): setup done\n",pa);)
    p+= SETUP_PAR;
   }
 }

return(plOK);
}


/******************************************************************************/

plerrcode test_proc_fb_lc1877_setup(p)       
  /* test subroutine for "proc_fb_lc1877_setup" */
int *p;
{
int pa; 
int n_p,unused;
int i,i_p;
int msk_p[11]={0x1,0x1,0x3,0x1,0xf,0x1,0xf,0xf,0xfff,0xf,0x3};

T(test_proc_fb_lc1877_setup)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

unused= p[0];
D(D_USER,printf("TDC 1877: number of parameters for all modules= %d\n",unused);)

unused-= 1; n_p= 1;
if (unused%SETUP_PAR) {                 /* check number of parameters         */
  *outptr++=0;
  return(plErr_ArgNum);
 }
if ((unused/SETUP_PAR)!=p[1]) {
  *outptr++=1;
  return(plErr_ArgNum);
 }

for (i=1; i<=memberlist[0]; i++) {
  if (get_mod_type(memberlist[i])==LC_TDC_1877) {
    pa= memberlist[i];
    D(D_USER, printf("TDC 1877(%2d): number of parameters not used= %d\n",pa,unused);)
    if (unused<SETUP_PAR) { /* not enough parameters for modules defined in IS*/
      *outptr++=pa;*outptr++=unused;
      return(plErr_ArgNum);
     }
    if (p[n_p+1])                       /* full scale time ignored            */
      msk_p[7]= 0xffffffff;
    else                                /* common start timeout ignored       */
      msk_p[9]= 0xffffffff;
    for (i_p=1; i_p<=SETUP_PAR; i_p++) {  /* check parameter ranges           */
      if (((unsigned int)p[n_p+i_p]&msk_p[i_p-1])!=(unsigned int)p[n_p+i_p]) {
        *outptr++=pa;*outptr++=i_p;*outptr++=0;
        return(plErr_ArgRange);
       }
     }
    if ((unsigned int)p[n_p+7]<1) {    /* additional check for broadcast class*/
      *outptr++=pa;*outptr++=7;*outptr++=1;
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
 
/*****************************************************************************/
/*****************************************************************************/
