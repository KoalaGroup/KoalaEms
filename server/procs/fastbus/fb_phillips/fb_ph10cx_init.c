/*
 * procs/fastbus/fb_phillips/fb_ph10Cx_init.c
 * created 04.Feb.2001 PW
 * 04.Jun.2002 PW multi crate support                                            *
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fb_ph10cx_init.c,v 1.11 2017/10/09 21:25:37 wuestner Exp $";

#include <math.h>
#include <utime.h>
#include <stdlib.h>
#ifdef __unix__
#include <sys/time.h>
#endif
#include <sconf.h>
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

#define N_CHAN  32                      /* number of channels in one module   */
#define N_SETUP_PAR  6                  /* number of basic setup parameters   */

extern ems_u32* outptr;
extern int *memberlist;

RCS_REGISTER(cvsid, "procs/fastbus/fb_phillips")

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_ph10cx_setup[]="fb_ph10Cx_setup";
int ver_proc_fb_ph10cx_setup=1;

/******************************************************************************/
/*
Setup of Phillips TDC 10C2 and ADC 10C6
=======================================

parameters:
-----------
  p[ 1] : number of TDC 10C2 and ADC 10C6 modules

module specific parameters:
---------------------------
  pp[ 0]: (CSR# 1<7:0>)   User Identification       0..255
  pp[ 1]: (CSR# 0< 6>)    data protect              0|1
  pp[ 2]: (CSR# 0< 8>)    enable lower threshold    0- disable, 1- enable
  pp[ 3]: (CSR# 0< 9>)    enable upper threshold    0- disable, 1- enable
  pp[ 4]: (CSR# 0<13>)    enable pedestal           0- disable, 1- enable
  pp[ 5]: (CSR# 0<11:12>) multiblock configuration
                                                   0  disabled
                                                   1  primary link
                                                   2  end link
                                                   3  middle link
  pp[ 6]:                 number of pedestal values 0|32|64|96
  pp[7...]
    for each channel up to three words must be provided:
      lower threshold, if enabled
      upper threshold, if enabled
      pedestal, if enabled

the sequence above is repeated for every TDC 10C2 and ADC 10C6 modul
  in the memberlist
*/
/******************************************************************************/

plerrcode proc_fb_ph10cx_setup(ems_u32* p)      
  /* setup of fastbus lecroy TDC 10C2 and ADC 10C6 */
{
ems_u32 *pp, sa, val;
int i, chan, help;
ems_u32 ssr;                            /* slave status response              */

T(proc_fb_ph10Cx_setup)

p+=2;
for (i=1; i<=memberlist[0]; i++) {
  ml_entry* module=modullist->entry+memberlist[i];
  if ((module->modultype==PH_ADC_10C2) ||
      (module->modultype==PH_TDC_10C6))
   {
    struct fastbus_dev* dev=module->address.fastbus.dev;
    int pa=module->address.fastbus.pa;

    D(D_USER,printf("TDC 10C2 or ADC 10C6: pa=%d\n", pa);)

    sa= 0;                              /******          csr#0           ******/
    dev->FWCa(dev, pa, sa, 1<<30, &ssr);/* master reset & primary address     */
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=0;
      return(plErr_HW);
    }

    val=
      ((p[1]&1)<<6)|  /* data protect */
      ((p[2]&1)<<8)|  /* enable lower threshold */
      ((p[3]&1)<<9)|  /* enable upper threshold */
      ((p[4]&1)<<13)| /* enable pedestal */
      ((p[5])&3)<<11; /* megablock */
    dev->FCWW(dev, val, &ssr);
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=1;
      return(plErr_HW);
    }
    
    sa= 1;                              /******          csr#1           ******/
    dev->FCWWS(dev, sa, p[0]&0xff, &ssr);
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=1;
      return(plErr_HW);
     }

    D(D_USER, printf("10Cx (%2d): parameter setup done\n",pa);)
    pp=p+N_SETUP_PAR;
    printf("Number of thresholds for Module %d: %d\n", pa, *pp);
    help=*pp++; /* skip counter */
    if (help) {                         /* download threshold values          */
      for (chan=0; chan<=(N_CHAN-1); chan++) {
        sa= 0xc0000000+3*chan;
/* testing p[4] moved downwards  {ts} 22-May-2001 */
        if (p[2]) { /* lower threshold */
          val=*pp++;
          dev->FCWWS(dev, sa+1, val, &ssr);
          if (ssr) {
            dev->FCDISC(dev);
            *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=3;
            return(plErr_HW);
          }
        }
        if (p[3]) { /* upper threshold */
          val=(*pp++)>>2;
          dev->FCWWS(dev, sa+2, val, &ssr);
          if (ssr) {
            dev->FCDISC(dev);
            *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=4;
            return(plErr_HW);
          }
        }
/* testing p[4] moved here  {ts} 22-May-2001 */
        if (p[4]) { /* pedestal */
          val=*pp++;
          dev->FCWWS(dev, sa, val, &ssr);
          if (ssr) {
            dev->FCDISC(dev);
            *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=2;
            return(plErr_HW);
          }
        }
       }
    }
    p=pp;

    sa= 0;                              /******          csr#0           ******/
    dev->FCWWS(dev, sa, 1<<2, &ssr);    /* acquisition mode                   */
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=5;
      return(plErr_HW);
    }

    dev->FCDISC(dev);
    if (ssr) {
      *outptr++=pa; *outptr++=sa; *outptr++=ssr; *outptr++=6;
      return(plErr_HW);
     }
   }
 }

return(plOK);
}
/******************************************************************************/

plerrcode test_proc_fb_ph10cx_setup(ems_u32* p)       
{
int unused, i, j, n, nm, idx;
int msk[6]={0xff, 0x1, 0x1, 0x1, 0x1, 0x3};

T(test_proc_fb_ph10Cx_setup)

if (!memberlist) return plErr_NoISModulList;
if (!memberlist[0]) return plErr_BadISModList;

unused=p[0];      /* number of arguments */
n=p[1]; unused--; /* number of phillips modules */
/* check modullist */
nm=0;
for (i=1; i<=memberlist[0]; i++) {
  ml_entry* entry=modullist->entry+memberlist[i];
  if ((entry->modultype==PH_ADC_10C2) ||
      (entry->modultype==PH_TDC_10C6))
    nm++;
}
if (nm!=n) {
  *outptr++=0; *outptr++=1;
  return plErr_ArgNum;
}
idx=2;
for (i=0; i<n; i++) {
  if (unused<N_SETUP_PAR+1) {
    *outptr++=1; *outptr++=i; *outptr++=idx;
    return plErr_ArgNum;
  }
  for (j=0; j<N_SETUP_PAR; j++) {  /* check parameter ranges     */
    if (((unsigned int)p[idx+j]&msk[j])!=(unsigned int)p[idx+j]) {
      *outptr++=2; *outptr++=i; *outptr++=idx+j;
      return plErr_ArgRange;
    }
  }
  nm=(p[idx+2]+p[idx+3]+p[idx+4])*N_CHAN;
  idx+=N_SETUP_PAR;
  if (p[idx]!=nm) {
    *outptr++=3; *outptr++=i; *outptr++=idx;
    return plErr_ArgRange;
  }
  idx+=nm+1;
}
wirbrauchen=4;
return plOK;
}
/******************************************************************************/
/******************************************************************************/
