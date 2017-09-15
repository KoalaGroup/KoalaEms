/*******************************************************************************
*                                                                              *
* fb_lc1875_init.c                                                             *
*                                                                              *
* OS9                                                                          *
*                                                                              *
* MaWo                                                                         *
* PeWue                                                                        *
*                                                                              *
* 10. 5.95                                                                     *
* 23. 8.95  CAT ARM and STOP Enable/Disable impemented                         *
* 13.06.2000 PW adapted for SFI/Nanobridge                                     *
* 27.05.2002 PW multi crate support                                            *
*                                                                              *
*******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: fb_lc1875_init.c,v 1.9 2011/04/06 20:30:31 wuestner Exp $";

#include <config.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../../lowlevel/oscompat/oscompat.h"
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../../objects/var/variables.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../procs.h"
#include "../../procprops.h"

#define set_ctrl_bit(param, bit) (1<<((param)?(bit):(bit+16)))

#define SETUP_PAR  7                    /* number of setup parameters         */

extern ems_u32* outptr;
extern int *memberlist;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/fastbus/fb_lecroy")

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1875_setup[]="fb_lc1875_setup";
int ver_proc_fb_lc1875_setup=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1875_setup_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1875_setup()
{
return(&fb_lc1875_setup_prop);
}
#endif
/******************************************************************************/
/*
Setup of LeCroy TDC 1875
========================

parameters:
-----------
  p[ 1] : number of tdc1875 modules

module specific parameters:
---------------------------
  p[ 2]:                 operation mode:           0- F mode, 1- N mode
  p[ 3]: (CSR# 0<21| 5>) ARM source:               0- front panel, 1- backplane
  p[ 4]: (CSR# 0<24| 8>) STOP source:              0- front panel, 1- backplane
  p[ 5]: (CSR# 0< 9|25>) FCW source:               0- internal, 1- backplane
  p[ 6]: (CSR# 1<24:30>) active channel depth:     1..64- range of active channels
                                                      0..(n-1)
  p[ 7]: (CSR# 0<28|12>) autorange:                0- disable, 1- enable
  p[ 8]: (CSR# 0<29|13>) range:                    0- low, 1- high
         (ignored if parameter p[ 7]=1)
*/
/******************************************************************************/

plerrcode proc_fb_lc1875_setup(ems_u32* p)      
  /* setup of fastbus lecroy tdc1875 */
{
ems_u32 sa, val, r_val, csr_val;
int i;
int csr_mask;

T(proc_fb_lc1875_setup)

p++;
for (i=1; i<=memberlist[0]; i++) {
  ml_entry* module=modullist->entry+memberlist[i];
  if (module->modultype==LC_TDC_1875) {
    struct fastbus_dev* dev=module->address.fastbus.dev;
    int pa=module->address.fastbus.pa;
    ems_u32 ssr;

    D(D_USER,printf("TDC 1875: pa= %d\n",pa);)

    sa= 0;                              /******           csr#0          ******/
    dev->FWCa(dev, pa, sa, 1<<27, &ssr);/* clear                              */
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=0;
      return(plErr_HW);
    }
    tsleep(2);
    dev->FCWW(dev, 1<<30, &ssr);
    if (ssr) {                          /* reset                              */
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=1;
      return(plErr_HW);
    }

                                        /* test of operation mode             */
    dev->FCWW(dev, 1<<5, &ssr);
    if (ssr) {                          /* ARM                                */
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=0;*outptr++=ssr;*outptr++=2;
      return(plErr_HW);
    }
    dev->FCWW(dev, 1<<11, &ssr);
    if (ssr) {                          /* STOP                               */
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=0;*outptr++=ssr;*outptr++=3;
      return(plErr_HW);
    }
    tsleep(2);

    sa= 1;                              /******           csr#1          ******/
    dev->FCRWS(dev, sa, &val, &ssr);    /* check conversion event counter     */
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=4;
      return(plErr_HW);
    }
    switch (val) {
      case 0x000:
        D(D_USER, printf("TDC 1875(%2d): N mode\n",pa);)
        if (!p[1]) {
          dev->FCDISC(dev);
          *outptr++=pa;*outptr++=sa;*outptr++=5;
          return(plErr_HWTestFailed);
        }
        break;
      case 0x100:
        D(D_USER, printf("TDC 1875(%2d): F mode\n",pa);)
        if (p[1]) {
          dev->FCDISC(dev);
          *outptr++=pa;*outptr++=sa;*outptr++=6;
          return(plErr_HWTestFailed);
        }
        break;
      default:
        dev->FCDISC(dev);
        *outptr++=pa;*outptr++=sa;*outptr++=7;
        return(plErr_HW);
        break;
    }

    sa= 0;                              /******           csr#0          ******/
    dev->FCWWS(dev, sa, 1<<27, &ssr);   /* clear                              */
    if (ssr) {    
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=8;
      return(plErr_HW);
    }
    tsleep(2);
    dev->FCWW(dev, 1<<30, &ssr);
    if (ssr) {                          /* reset                              */
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=9;
      return(plErr_HW);
    }

    val= set_ctrl_bit(p[2],5) |         /* ARM source                         */
         set_ctrl_bit(1,6) |            /* disable wait                       */
         set_ctrl_bit(p[3],8) |         /* FCW source                         */
         set_ctrl_bit(1-p[4],9) |       /* FCW source                         */
         set_ctrl_bit(0,10) |           /* disable test pulser                */
         set_ctrl_bit(p[6],12);         /* autorange                          */
    csr_mask= 0x1760;
    if (!p[6]) {                        /* autorange disabled                 */
      val= val | set_ctrl_bit(p[7],13);  /* range                             */
      csr_mask= csr_mask | 0x2000;
    }
    csr_val= (val^set_ctrl_bit(1,6))&csr_mask; /* wait status                 */
    dev->FCWW(dev, val, &ssr);
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=10;
      return(plErr_HW);
    }
    dev->FCRW(dev, &r_val, &ssr);
    r_val&=csr_mask;
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=11;
      return(plErr_HW);
    }
    if (r_val^csr_val) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=12;
      return(plErr_HW);
    }

    sa= 1;                              /******           csr#1          ******/
    dev->FCRWS(dev, sa, &val, &ssr);
    val=(val&0x80ffffff)|(p[5]<<24);
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=13;
      return(plErr_HW);
    }
    if (val != (p[5]<<24)) {
      ems_u32 unsol_mes[5];
      int unsol_num;      
      unsol_num=5;unsol_mes[0]=6;unsol_mes[1]=3;
      unsol_mes[2]=pa;unsol_mes[3]=sa;unsol_mes[4]=val;
      send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
    }
    dev->FCWW(dev, val, &ssr);
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=14;
      return(plErr_HW);
    }
    dev->FCRW(dev, &r_val, &ssr);
    dev->FCDISC(dev);                           /* disconnect                         */
    if (ssr) {
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=15;
      return(plErr_HW);
    }
    if (r_val^val) {
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=16;
      return(plErr_HW);
    }

    D(D_USER, printf("TDC 1875(%2d): setup done\n",pa);)
    p+= SETUP_PAR;
  }
}

return(plOK);
}


/******************************************************************************/

plerrcode test_proc_fb_lc1875_setup(ems_u32* p)       
  /* test subroutine for "proc_fb_lc1875_setup" */
{
int n_p,unused;
int i,i_p;
int msk_p[7]={0x1,0x1,0x1,0x1,0x7f,0x1,0x1};

T(test_proc_fb_lc1875_setup)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

unused= p[0];
D(D_USER,printf("TDC 1875: number of parameters for all modules= %d\n",unused);)

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
  if (module->modultype==LC_TDC_1875) {
    D(D_USER, printf("TDC 1875(%2d): number of parameters not used= %d\n",i,unused);)
    if (unused<SETUP_PAR) { /* not enough parameters for modules defined in IS*/
      *outptr++=i;*outptr++=unused;
      return(plErr_ArgNum);
     }
    if (p[n_p+6])                       /* autorange enabled                  */
      msk_p[6]= 0xffffffff;
    for (i_p=1; i_p<=SETUP_PAR; i_p++) {  /* check parameter ranges           */
      if (((unsigned int)p[n_p+i_p]&msk_p[i_p-1])!=(unsigned int)p[n_p+i_p]) {
        *outptr++=i;*outptr++=i_p;*outptr++=0;
        return(plErr_ArgRange);
       }
     }
    if (((unsigned int)p[n_p+5]<1) || ((unsigned int)p[n_p+5]>64)) {
      *outptr++=i;*outptr++=5;*outptr++=1;  /* add. check for active depth   */
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
