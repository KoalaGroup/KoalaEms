/*******************************************************************************
*                                                                              *
* fb_lc1876_init.c                                                             *
*                                                                              *
* OS9                                                                          *
*                                                                              *
* MaWo                                                                         *
* PeWue                                                                        *
*                                                                              *
* 27.12.94                                                                     *
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

#define SETUP_PAR  10                   /* number of setup parameters         */

extern int *outptr;
extern int *memberlist;
extern int wirbrauchen;

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1876_setup[]="fb_lc1876_setup";
int ver_proc_fb_lc1876_setup=1;

/******************************************************************************/

static procprop fb_lc1876_setup_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1876_setup()
{
return(&fb_lc1876_setup_prop);
}

/******************************************************************************/
/*
Setup of LeCroy TDC 1876
========================

parameters:
-----------
  p[ 1] : number of tdc1876 modules

module specific parameters:
---------------------------
  p[ 2]: (CSR# 1<31>)    mode:                     0- common stop, 1- common start
  p[ 3]: (CSR# 1< 1>)    common source:            0- front panel, 1- backplane
  p[ 4]: (CSR# 1<29:30>) edge detection:           0- leading & trailing edge detection
                                                   1- only leading edge detection
                                                   2- only trailing edge detection
  p[ 5]: (CSR# 1< 2>)    FCW via backplane:        0- disable, 1- enable
  p[ 6]: (CSR# 1<24:27>) internal FCW:             0- 2^10 ns
                                                   1..13 (odd)-
                                                      2^((n+1)/2) * 2^10 ns
                                                   2..12 (even)-
                                                      (2^(n/2) + 2^((n/2) - 1)) * 2^10 ns
                                                  14,15-
                                                      2^(n-6) * 2^10 ns
  p[ 7]: (CSR# 1< 3>)    Fast Clear via backplane: 0- disable, 1- enable
  p[ 8]: (CSR# 7< 0: 7>) broadcast class           1..255- broadcast class 0..7
                                                      (class n <==> bit n= 1)
  p[ 9]: (CSR#18< 4:15>) lower time limit:         0..4095- n*16 ns
  p[10]: (CSR#18<20:31>) upper time limit:         0..4095- (n+1)*16 ns
  p[11]: (CSR# 1< 4: 7>) common start timeout:     0- front panel
                                                   1..11- internal timer: timeout after 2^(5+n) ns
                                                  11..15- internal timer: timeout after 65.536 ns
         (ignored if parameter p[ 2]=0)
*/
/******************************************************************************/

plerrcode proc_fb_lc1876_setup(p)      
  /* setup of fastbus lecroy tdc1876 */
int *p;
{
int pa,sa,val,r_val;
int i;
int ssr;                                 /* slave status response             */

T(proc_fb_lc1876_setup)

p++;
for (i=1; i<=memberlist[0]; i++) {
  if (get_mod_type(memberlist[i])==LC_TDC_1876) {
    pa= memberlist[i];
    D(D_USER,printf("TDC 1876: pa= %d\n",pa);)

    sa= 0;                              /******           csr#0          ******/
    FWCa(pa,sa,1<<30);                  /* master reset & primary address     */
    if (sscode()) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=0;
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
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=1;
      return(plErr_HW);
     }
    r_val= FCRW();
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

    sa= 7;                              /******          csr#7           ******/
    val= p[7];                         /* broadcast classes                  */
    if (ssr=(FCWWS(sa,val)&0x7)) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=4;
      return(plErr_HW);
     }
    r_val= FCRW() & 0xff;
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

    sa= 18;                             /******          csr#18          ******/
    val= p[ 8]*(1<< 4) | p[ 9]*(1<<20); /* lower and upper time limit         */
    if (ssr=(FCWWS(sa,val)&0x7)) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=7;
      return(plErr_HW);
     }
    r_val= FCRW();
    FCDISC();                           /* disconnect                         */
    if (sscode()) {
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=8;
      return(plErr_HW);
     }
    if (r_val^val) {
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=9;
      return(plErr_HW);
     }

    D(D_USER, printf("TDC 1876(%2d): setup done\n",pa);)
    p+= SETUP_PAR;
   }
 }

return(plOK);
}


/******************************************************************************/

plerrcode test_proc_fb_lc1876_setup(p)       
  /* test subroutine for "proc_fb_lc1876_setup" */
int *p;
{
int pa; 
int n_p,unused;
int i,i_p;
int msk_p[10]={0x1,0x1,0x3,0x1,0xf,0x1,0xff,0xfff,0xfff,0xf};

T(test_proc_fb_lc1876_setup)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

unused= p[0];
D(D_USER,printf("TDC 1876: number of parameters for all modules= %d\n",unused);)

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
  if (get_mod_type(memberlist[i])==LC_TDC_1876) {
    pa= memberlist[i];
    D(D_USER, printf("TDC 1876(%2d): number of parameters not used= %d\n",pa,unused);)
    if (unused<SETUP_PAR) { /* not enough parameters for modules defined in IS*/
      *outptr++=pa;*outptr++=unused;
      return(plErr_ArgNum);
     }
    if (!p[n_p+1])                      /* common start timeout ignored       */
      msk_p[9]= 0xffffffff;
    for (i_p=1; i_p<=SETUP_PAR; i_p++) {  /* check parameter ranges           */
      if (((unsigned int)p[n_p+i_p]&msk_p[i_p-1])!=(unsigned int)p[n_p+i_p]) {
        *outptr++=pa;*outptr++=i_p;*outptr++=0;
        return(plErr_ArgRange);
       }
     }
    if ((unsigned int)p[n_p+3]>2) {   /* additional check for edge detection*/
      *outptr++=pa;*outptr++=3;*outptr++=1;
      return(plErr_ArgRange);
     }
    if ((unsigned int)p[n_p+7]<1) {  /* additional check for broadcast class*/
      *outptr++=pa;*outptr++=7;*outptr++=1;
      return(plErr_ArgRange);
     }
    n_p+= SETUP_PAR; unused-= SETUP_PAR;
   }
 }

if (unused) {                         /* unused parameters left             */
  *outptr++=2;
  return(plErr_ArgNum);
 }

wirbrauchen= 4;
return(plOK);
}
 
/*****************************************************************************/
/*****************************************************************************/
