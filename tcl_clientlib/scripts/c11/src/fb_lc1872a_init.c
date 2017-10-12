/*******************************************************************************
*                                                                              *
* fb_lc1872a_init.c                                                            *
*                                                                              *
* OS9                                                                          *
*                                                                              *
* MaWo                                                                         *
* PeWue                                                                        *
*                                                                              *
* 22. 7.96                                                                     *
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

#define set_ctrl_bit(param, bit) (1<<((param)?(bit):(bit+16)))

#define SETUP_PAR  4                    /* number of setup parameters         */

extern int *outptr;
extern int *memberlist;
extern int wirbrauchen;

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1872a_setup[]="fb_lc1872a_setup";
int ver_proc_fb_lc1872a_setup=1;

/******************************************************************************/

static procprop fb_lc1872a_setup_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1872a_setup()
{
return(&fb_lc1872a_setup_prop);
}

/******************************************************************************/
/*
Setup of LeCroy TDC 1872A
========================

parameters:
-----------
  p[ 1] : number of tdc1872a modules

module specific parameters:
---------------------------
  p[ 2]:                 operation mode:           0- F mode, 1- N mode
  p[ 3]: (CSR# 0<24| 8>) common start source:      0- front panel, 1- backplane
  p[ 4]: (CSR# 0< 9|25>) FCW source:               0- internal, 1- backplane
  p[ 5]: (CSR# 1<24:30>) active channel depth:     1..64- range of active channels
                                                      0..(n-1)
*/
/******************************************************************************/

plerrcode proc_fb_lc1872a_setup(p)      
  /* setup of fastbus lecroy tdc1872a */
int *p;
{
int pa,sa,val,r_val,csr_val;
int i;
int csr_mask;
int ssr;                                 /* slave status response             */

T(proc_fb_lc1872a_setup)

p++;
for (i=1; i<=memberlist[0]; i++) {
  if (get_mod_type(memberlist[i])==LC_TDC_1872A) {
    pa= memberlist[i];
    D(D_USER,printf("TDC 1872A: pa= %d\n",pa);)

    sa= 0;                              /******           csr#0          ******/
    FWCa(pa,sa,1<<27);                  /* clear                              */
    if (sscode()) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=0;
      return(plErr_HW);
     }
    tsleep(2);
    if (ssr=(FCWWss(1<<30)&0x7)) {      /* reset                              */
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=1;
      return(plErr_HW);
     }

                                        /* test of operation mode             */
    if (ssr=(FCWWss(1<<11)&0x7)) {      /* COM                                */
      FCDISC();
      *outptr++=pa;*outptr++=0;*outptr++=ssr;*outptr++=2;
      return(plErr_HW);
     }
    tsleep(2);

    sa= 1;                              /******           csr#1          ******/
    val= FCRWS(sa) & 0xfff;             /* check conversion event counter     */
    if (sscode()) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=3;
      return(plErr_HW);
     }
    switch (val) {
      case 0x000:
        D(D_USER, printf("TDC 1872A(%2d): N mode\n",pa);)
        if (!p[1]) {
          FCDISC();
          *outptr++=pa;*outptr++=sa;*outptr++=4;
          return(plErr_HWTestFailed);
         }
        break;
      case 0x100:
        D(D_USER, printf("TDC 1872A(%2d): F mode\n",pa);)
        if (p[1]) {
          FCDISC();
          *outptr++=pa;*outptr++=sa;*outptr++=5;
          return(plErr_HWTestFailed);
         }
        break;
      default:
        FCDISC();
        *outptr++=pa;*outptr++=sa;*outptr++=6;
        return(plErr_HW);
        break;
     }

    sa= 0;                              /******           csr#0          ******/
    if (ssr=(FCWWS(sa,1<<27)&0x7)) {    /* clear                              */
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=7;
      return(plErr_HW);
     }
    tsleep(2);
    if (ssr=(FCWWss(1<<30)&0x7)) {      /* reset                              */
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=8;
      return(plErr_HW);
     }

    val= set_ctrl_bit(1,6) |            /* disable wait                       */
         set_ctrl_bit(0,10) |           /* disable test pulser                */
         set_ctrl_bit(p[2],8) |         /* common start source                */
         set_ctrl_bit(1-p[3],9);        /* FCW source                         */
    csr_mask= 0x740;
    csr_val= (val^set_ctrl_bit(1,6))&csr_mask; /* wait status                 */
    if (ssr=(FCWWss(val)&0x7)) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=9;
      return(plErr_HW);
     }
    r_val= FCRW() & csr_mask;
    if (sscode()) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=10;
      return(plErr_HW);
     }
    if (r_val^csr_val) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=11;
      return(plErr_HW);
     }

    sa= 1;                              /******           csr#1          ******/
    val=(p[4]<<24) | (FCRWS(sa) & 0x80ffffff);
    if (sscode()) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=12;
      return(plErr_HW);
     }
    if (val != (p[4]<<24)) {
      int unsol_mes[5],unsol_num;      
      unsol_num=5;unsol_mes[0]=6;unsol_mes[1]=3;
      unsol_mes[2]=pa;unsol_mes[3]=sa;unsol_mes[4]=val;
      send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
     }
    if (ssr=(FCWWss(val)&0x7)) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=13;
      return(plErr_HW);
     }
    r_val= FCRW();
    FCDISC();                           /* disconnect                         */
    if (sscode()) {
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=14;
      return(plErr_HW);
     }
    if (r_val^val) {
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=15;
      return(plErr_HW);
     }

    D(D_USER, printf("TDC 1872A(%2d): setup done\n",pa);)
    p+= SETUP_PAR;
   }
 }

return(plOK);
}


/******************************************************************************/

plerrcode test_proc_fb_lc1872a_setup(p)       
  /* test subroutine for "proc_fb_lc1872a_setup" */
int *p;
{
int pa;
int n_p,unused;
int i,i_p;
int msk_p[4]={0x1,0x1,0x1,0x7f};

T(test_proc_fb_lc1872a_setup)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

unused= p[0];
D(D_USER,printf("TDC 1872A: number of parameters for all modules= %d\n",unused);)

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
  if (get_mod_type(memberlist[i])==LC_TDC_1872A) {
    pa= memberlist[i];
    D(D_USER, printf("TDC 1872A(%2d): number of parameters not used= %d\n",pa,unused);)
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
    if (((unsigned int)p[n_p+4]<1) || ((unsigned int)p[n_p+4]>64)) {
      *outptr++=pa;*outptr++=4;*outptr++=1;  /* add. check for edge detection */
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
