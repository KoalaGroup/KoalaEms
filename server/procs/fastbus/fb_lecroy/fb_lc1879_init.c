/*******************************************************************************
*                                                                              *
* fb_lc1879_init.c                                                             *
*                                                                              *
* MaWo                                                                         *
* PeWue                                                                        *
* MiZi                                                                         *
*                                                                              *
* 28.12.94                                                                     *
* 14.06.2000 PW adapted for SFI/Nanobridge                                     *
* 03.06.2002 PW multi crate support                                            *
* 03.06.2002 PW multi crate support                                            *
*                                                                              *
*******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: fb_lc1879_init.c,v 1.11 2017/10/09 21:25:37 wuestner Exp $";

#include <config.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../../objects/var/variables.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../procs.h"
#include "../../procprops.h"

#define set_ctrl_bit(param, bit) (1<<((param)?(bit):(bit)+16))

#define SETUP_PAR  5                    /* number of setup parameters         */

extern ems_u32* outptr;
extern int *memberlist;

RCS_REGISTER(cvsid, "procs/fastbus/fb_lecroy")

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1879_setup[]="fb_lc1879_setup";
int ver_proc_fb_lc1879_setup=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1879_setup_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1879_setup()
{
return(&fb_lc1879_setup_prop);
}
#endif
/******************************************************************************/
/*
Setup of LeCroy TDC 1879
========================

parameters:
-----------
  p[ 1] : number of tdc1879 modules

module specific parameters:
---------------------------
  p[ 2]: (CSR# 0< 0>)    common stop source:       0- front panel, 1- backplane
  p[ 3]: (CSR# 0< 9>)    frequency reference       0- internal, 1- external
  p[ 4]: (CSR# 1< 1: 2>) resolution:               0..3- time bin size= 2^(n+1) ns
  p[ 5]: (CSR#c0< 0: 3>) double pulse resolution:  0..15- time bins (leading zeros before hit)
  p[ 6]: (CSR#c1< 0: 4>) readout depth (range):    0..31- 16*(n+1) time bins
*/
/******************************************************************************/

plerrcode proc_fb_lc1879_setup(ems_u32* p)      
  /* setup of fastbus lecroy tdc1879 */
{
ems_u32 sa,val,r_val,csr_val;
int i;
ems_u32 ssr;                             /* slave status response             */

T(proc_fb_lc1879_setup)

p++;
for (i=1; i<=memberlist[0]; i++) {
  ml_entry* module=modullist->entry+memberlist[i];
  if (module->modultype==LC_TDC_1879) {
    struct fastbus_dev* dev=module->address.fastbus.dev;
    int pa=module->address.fastbus.pa;

    D(D_USER,printf("TDC 1879: pa= %d\n",pa);)
printf("lc1879_setup; pa=%d\n", pa);
    sa= 0;                              /******           csr#0          ******/
    dev->FWCa(dev, pa, sa, set_ctrl_bit(0,12), &ssr);/* disable stop          */
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=0;
      return(plErr_HW);
     }

    val= set_ctrl_bit(1-p[1], 8) |      /* common stop source                 */
         set_ctrl_bit(0, 6) |           /* disable bin 1                      */
         set_ctrl_bit(1-p[2], 9) |      /* frequency reference                */
         set_ctrl_bit(0,10) |           /* disable test even                  */
         set_ctrl_bit(0,11) |           /* disable test odd                   */
         set_ctrl_bit(1,13);            /* enable encoding                    */
    csr_val= val;
    dev->FCWW(dev, val, &ssr);
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=1;
      return(plErr_HW);
     }
    dev->FCRW(dev, &r_val, &ssr);
    r_val&=0x3f40;
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=2;
      return(plErr_HW);
     }
    if (r_val^(val&0xffff)) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=3;
      return(plErr_HW);
     }

    sa= 1;                              /******           csr#1          ******/
    val= p[3];                          /* resolution                         */
    dev->FCWWS(dev, sa, val, &ssr);
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=4;
      return(plErr_HW);
     }
    dev->FCRW(dev, &r_val, &ssr);
    r_val&=0x3;
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=5;
      return(plErr_HW);
     }
    if (r_val^val) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=6;
      return(plErr_HW);
     }

    sa= 0xC0000000;                     /******        csr#c0000000      ******/
    val= p[4];                          /* double pulse resolution            */
    if (dev->FCWWS(dev, sa, val, &ssr), ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=7;
      return(plErr_HW);
     }
    dev->FCRW(dev, &r_val, &ssr);
    r_val&=0xf;
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=8;
      return(plErr_HW);
     }
    if (r_val^val) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=9;
      return(plErr_HW);
     }

    sa= 0xC0000001;                     /******        csr#c0000001      ******/
    val= p[5];                          /* active readout depth               */
    if (dev->FCWWS(dev, sa, val, &ssr), ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=10;
      return(plErr_HW);
     }
    dev->FCRW(dev, &r_val, &ssr);
    r_val&=0x1f;
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=11;
      return(plErr_HW);
     }
    if (r_val^val) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=12;
      return(plErr_HW);
     }

    sa= 0;                              /******           csr#0          ******/
    val= csr_val | set_ctrl_bit(1,12);
    if (dev->FCWWS(dev, sa, val, &ssr), ssr) {      /* enable stop                        */
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=13;
      return(plErr_HW);
     }
    dev->FCRW(dev, &r_val, &ssr);
    r_val&=0x3f40;
    dev->FCDISC(dev);                           /* disconnect                         */
    if (ssr) {
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=14;
      return(plErr_HW);
     }
    if (r_val^(val&0xffff)) {
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=15;
      return(plErr_HW);
     }

    D(D_USER, printf("TDC 1879(%2d): setup done\n",pa);)
    p+= SETUP_PAR;
   }
 }

return(plOK);
}

/******************************************************************************/

plerrcode test_proc_fb_lc1879_setup(ems_u32* p)       
  /* test subroutine for "proc_fb_lc1879_setup" */
{
int n_p,unused;
int i,i_p; 
int msk_p[5]={0x1,0x1,0x3,0xf,0x1f};

T(test_proc_fb_lc1879_setup)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

unused= p[0];
D(D_USER,printf("TDC 1879: number of parameters for all modules= %d\n",unused);)

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
  ml_entry* module=modullist->entry+memberlist[i];
  if (module->modultype==LC_TDC_1879) {
    D(D_USER, printf("TDC 1879(%2d): number of parameters not used= %d\n",i,unused);)
    if (unused<SETUP_PAR) { /* not enough parameters for modules defined in IS*/
      *outptr++=i;*outptr++=unused;
      return(plErr_ArgNum);
     }
    for (i_p=1; i_p<=SETUP_PAR; i_p++) {  /* check parameter ranges           */
      if (((unsigned int)p[n_p+i_p]&msk_p[i_p-1])!=(unsigned int)p[n_p+i_p]) {
        *outptr++=i;*outptr++=i_p;
        return(plErr_ArgRange);
       }
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
