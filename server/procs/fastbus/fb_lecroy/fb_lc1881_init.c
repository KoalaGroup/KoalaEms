/*
 * procs/fastbus/fb_lecroy/fb_lc1881_init.c
 * 
 * MaWo
 * PeWue
 * MiZi
 * 
 * 29.12.94
 * 30.12.94  #undef for local constants
 * 11. 1.95  CHI trigger for pedestal measurement implemented
 *  1. 2.95  Unsol_Data replaced by Unsol_Warning
 *  2. 2.95  Unsol_Patience added for pedestal measurement
 *           compiler error at pedestal analysis "corrected"
 * 09.10.97  compatible with ADC 1881M
 * 14.06.2000 PW adapted for SFI/Nanobridge
 * 02.Feb.2001 PW multiblock setup for 1881M
 * 16.May.2001 PW max. thresholds for 1881M: 0x3fff
 * 03.06.2002 PW multi crate support
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fb_lc1881_init.c,v 1.16 2011/04/06 20:30:31 wuestner Exp $";

#include <math.h>
#include <utime.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
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
#include "../fastbus_verify.h"
#include "fb_lc_util.h"
#include "fb_lc1810_init.h"

#define N_CHAN  64                      /* number of channels in one module   */
#define N_SETUP_PAR  8                  /* number of setup parameters         */

extern ems_u32* outptr;
extern int *memberlist;
extern int wirbrauchen;

#define FB_BUFBEG 0
#define FB_BUFEND 0

RCS_REGISTER(cvsid, "procs/fastbus/fb_lecroy")

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1881_setup[]="fb_lc1881_setup";
int ver_proc_fb_lc1881_setup=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1881_setup_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1881_setup()
{
return(&fb_lc1881_setup_prop);
}
#endif
/******************************************************************************/
/*
Setup of LeCroy ADC 1881
========================

parameters:
-----------
  p[ 1] : number of adc1881 modules

module specific parameters:
---------------------------
  p[ 2]: (CSR# 1< 1>)    gate source:              0- front panel, 1- backplane
  p[ 3]: (CSR# 1< 3>)    Fast Clear via backplane: 0- disable, 1- enable
  p[ 4]: (CSR# 1< 2>)    FCW source:               0- internal, 1- backplane
  p[ 5]: (CSR# 1<30>)    sparsification:           0- disable, 1- enable
  p[ 6]: (CSR# 1<24:27>) internal FCW              0..9- 2*n + 14 us
         (ignored if parameter p[ 4]=1)
  p[ 7]: (CSR# 7< 0: 3>) broadcast class           1..15- broadcast class 0..3
                                                      (class n <==> bit n= 1)
  p[ 8]: (CSR# 0<11:12>) multiblock configuration
                                                   0  bypass
                                                   1  primary link
                                                   2  end link
                                                   3  middle link
  p[ 9]:                 number of pedestal values 0,64
  p[10]: (CSR#C0000000)  pedestal value chan#0:    0..0x3fff
  ...
  p[73]: (CSR#C000003f)  pedestal value chan#63:   0..0x3fff
         (p[10]..p[73] ignored if parameter p[ 9]=0)
*/
/******************************************************************************/

plerrcode proc_fb_lc1881_setup(ems_u32* p)      
  /* setup of fastbus lecroy adc1881 */
{
ems_u32 sa, val, r_val;
int i,chan;
ems_u32 ssr;                            /* slave status response              */

T(proc_fb_lc1881_setup)

p++;
for (i=1; i<=memberlist[0]; i++) {
  ml_entry* module=modullist->entry+memberlist[i];
  if ((module->modultype==LC_ADC_1881) ||
        (module->modultype==LC_ADC_1881M)) {
    struct fastbus_dev* dev=module->address.fastbus.dev;
    int pa=module->address.fastbus.pa;

    D(D_USER,printf("ADC 1881: pa= %d\n",pa);)
printf("lc1881_setup; pa=%d\n", pa);

    sa= 0;                              /******          csr#0           ******/
    dev->FWCa(dev, pa, sa, 1<<30, &ssr);/* master reset & primary address     */
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=0;
      return(plErr_HW);
     }
    if (p[7]) {/* multiblock configuration */
      dev->FCWW(dev, (p[7]*(1<<11))|(1<<8), &ssr);
      if (ssr) {
        dev->FCDISC(dev);
        *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=13;
        return(plErr_HW);
      }
    }

    sa= 1;                              /******          csr#1           ******/
    val= p[1]*(1<< 1)  |                /* gate via front panel/backplane     */
         p[2]*(1<< 3)  |                /* enable fast clear via backplane    */
         p[3]*(1<< 2)  |                /* fcw internal/via backplane         */
         p[4]*(1<<30);                  /* enable sparsification              */
    if (!p[3])
      val= val | p[5]*(1<<24);          /* internal FCW                       */
    if (module->modultype==LC_ADC_1881M)
      val= val | 0x40;
    dev->FCWWS(dev, sa, val, &ssr);
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=1;
      return(plErr_HW);
     }
    dev->FCRW(dev, &r_val, &ssr);
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=2;
      return(plErr_HW);
     }
    if (r_val^val) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=3;
      return(plErr_HW);
     }

    sa= 7;                              /******          csr#7           ******/
    val= p[6];                          /* broadcast classes                  */
    dev->FCWWS(dev, sa, val, &ssr);
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=4;
      return(plErr_HW);
     }
    dev->FCRW(dev, &r_val, &ssr);
    r_val&=0xf;
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

    D(D_USER, printf("ADC 1881(%2d): parameter setup done\n",pa);)
    p+= N_SETUP_PAR;

    if (p[0]) {                         /* download threshold values          */
      for (chan=0; chan<=(N_CHAN-1); chan++) {
        sa= 0xc0000000 + chan;
        val= p[chan+1];
        DV(D_USER,printf("ADC 1881(%2d, chan %2d): ped= %4d\n",pa,chan,val);)
        dev->FCWWS(dev, sa, val, &ssr);
        if (ssr) {
          dev->FCDISC(dev);
          *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=7;
          return(plErr_HW);
         }
        dev->FCRW(dev, &r_val, &ssr);
        r_val&=0xffff;
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
       }
      D(D_USER, printf("ADC 1881(%2d): pedestals downloaded\n",pa);)
      p+= N_CHAN;
     }

    sa= 0;                              /******           csr#0          ******/
    dev->FCWWS(dev, sa, 1<<2, &ssr);     /* enable gate                       */
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=10;
      return(plErr_HW);
     }
    dev->FCRW(dev, &r_val, &ssr);
    dev->FCDISC(dev);                           /* disconnect                         */
    if (ssr) {
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=11;
      return(plErr_HW);
     }
    if (!(r_val&0x4)) {
      *outptr++=pa;*outptr++=sa;*outptr++=12;
      return(plErr_HW);
     }
   }
 }

return(plOK);
}


/******************************************************************************/

plerrcode test_proc_fb_lc1881_setup(ems_u32* p)       
  /* test subroutine for "proc_fb_lc1881_setup" */
{
int idx; 
int n_p,unused;
int i,i_p;
int msk_p[8]={0x1,0x1,0x1,0x1,0xf,0xf,0x3,0x3fff};
int n_mod_par;                          /* number of parameters per module    */

T(test_proc_fb_lc1881_setup)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

n_mod_par= N_SETUP_PAR + N_CHAN;
unused= p[0];
D(D_USER,printf("ADC 1881: number of parameters for all modules= %d\n",unused);)

unused-= 1; n_p= 1;
if ((unused%N_SETUP_PAR) && (unused%n_mod_par)) { /*check number of parameters*/
  *outptr++=0;
  return(plErr_ArgNum);
 }
if (((unused/N_SETUP_PAR)!=p[1]) && ((unused/n_mod_par)!=p[1])) {
  *outptr++=1;
  return(plErr_ArgNum);
 }

for (i=1; i<=memberlist[0]; i++) {
  ml_entry* module=modullist->entry+memberlist[i];
  if ((module->modultype==LC_ADC_1881) || (module->modultype==LC_ADC_1881M)) {
    plerrcode pres;
    D(D_USER, printf("1881: memberlist[%d]: %d\n", i, memberlist[i]);)
    if ((pres=verify_fastbus_module(module))!=plOK)
        return pres;
    idx= memberlist[i];
    D(D_USER, printf("ADC 1881(idx=%2d): number of parameters not yet used= %d\n",
            idx, unused);)
    if (unused<N_SETUP_PAR) {/*not enough parameters for modules defined in IS*/
      *outptr++=idx;*outptr++=unused;*outptr++=0;
      return(plErr_ArgNum);
     }
    if (p[n_p+3])                       /* internal fcw ignored               */
      msk_p[4]= 0xffffffff;
    for (i_p=1; i_p<=(N_SETUP_PAR-1); i_p++) {  /* check parameter ranges     */
      if (((unsigned int)p[n_p+i_p]&msk_p[i_p-1])!=(unsigned int)p[n_p+i_p]) {
        *outptr++=idx;*outptr++=i_p;*outptr++=0;
        return(plErr_ArgRange);
       }
     }
    if (!p[n_p+3]) {                    /* additional check for internal fcw  */
      if ((unsigned int)p[n_p+5]>9) {
        *outptr++=idx;*outptr++=5;
        return(plErr_ArgRange);
       }
     }
    if ((unsigned int)p[n_p+6]<1) {    /* additional check for broadcast class*/
      *outptr++=idx;*outptr++=6;
      return(plErr_ArgRange);
     }
    n_p+= N_SETUP_PAR; unused-= N_SETUP_PAR;

    if ((p[n_p]!=0) && (p[n_p]!=N_CHAN)) {  /* number of pedestal values      */
      *outptr++=idx;*outptr++=7;
      return(plErr_ArgRange);
     }

    if (p[n_p]==N_CHAN) {               /* check pedestal values              */
      if (unused<N_CHAN) {
        *outptr++=idx;*outptr++=unused;*outptr++=1;
        return(plErr_ArgNum);
       }
      for (i_p=1; i_p<=N_CHAN; i_p++) {
        if (((unsigned int)p[n_p+i_p]&msk_p[7])!=(unsigned int)p[n_p+i_p]) {
          *outptr++=idx;*outptr++=i_p;*outptr++=1;
          return(plErr_ArgRange);
         }
       }
      n_p+= N_CHAN; unused-= N_CHAN;
     }
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

static int err_header(int word, int pa, int modultype)
{
int parity;

T(err_header)

if ((word>>27) != pa)           {return(1);}  /* geographic address           */
if (modultype==LC_ADC_1881) {
  if (((word>>17)&0x7f) != 127) {return(2);}  /* channel= 127 if header       */
 }
else if (modultype==LC_ADC_1881M) {
  if (((word>>17)&0x7f) != 0)   {return(2);}  /* channel= 0 if header         */
 }
if ((word&0x7f) != (N_CHAN+1))  {return(3);}  /* word count                   */

parity=0;
while (word) {
  if (word&0x1)  parity++;
  word>>=1;
 }
if (parity%2)                    {return(4);} /* parity error                 */

return(0);
}

/******************************************************************************/

static int err_data(int word, int pa, int chan, int modultype)
{
int parity;

T(err_data)

if (((word>>27)&0x1f) != pa)    {return(1);}  /* geographic address           */
if (((word>>17)&0x3f) != chan)  {return(2);}  /* channel number               */
if (modultype==LC_ADC_1881) {
  if ((word>>13)&0x7)           {return(3);}  /* over range indicator         */
 }

parity=0;
while (word) {
  if (word&0x1)  parity++;
  word>>=1;
 }
if (parity%2)                   {return(4);}  /* parity error                 */

return(0);
}

/******************************************************************************/

char name_proc_fb_lc1881_measure_ped[]="fb_lc1881_measure_ped";
int ver_proc_fb_lc1881_measure_ped=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1881_measure_ped_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1881_measure_ped()
{
return(&fb_lc1881_measure_ped_prop);
}
#endif
/******************************************************************************/
/*
measurement of LeCroy ADC 1881 pedestals
========================================

parameters:
-----------
  p[ 1]: number of triggers:                       1..50000
  p[ 2]: GATE generator:                           0- external trigger
                                                   1- CHI trigger
                                                   2- GSI soft trigger
                                                   3- SFI output
  p[ 3]: Address Space (CHI)                       1,2,3- base
         or bitmask (SFI)
  p[ 4]: broadcast class for readout               0..3- broadcast class 0..3
  p[ 5]: pa of CAT_1810
output data format:
------------------
  ptr[0]:                2 * number of read out channels (int)

  for each channel in each read out module (start: n=1, 128 words per module):
  ptr[n]:                mean value for pedestal (float)
  ptr[n+1]:              standard deviation (float)
*/
/******************************************************************************/

plerrcode proc_fb_lc1881_measure_ped(ems_u32* p)
{
#define MAX_WT_TRI  1000                /* wait for trigger: maximum timeout  */
#define MAX_WT_CON  10                 /* wait for conversion: max. timeout  */

float *fl_ptr;                    /* floating point format of returned values */
double *sum,*quad,*sum_st,*quad_st;    /* simple & quadratic sum of pedestals */
double data;
double chan_sum, chan_quad;  /* simple & quadratic sum for individual channel */
ems_u32 *ped,*mod_st,*ped_st;           /* read out pedestal values           */
double mean,sigm,mean_fact,sigm_fact; /* mean & standard deviation of pedestal*/
int pat;                                /* IS module pattern                  */
ems_u32 sds_pat;                        /* sparse data scan module pattern    */
int n_mod;                              /* number of modules                  */
#ifdef CHIFASTBUS
int max_mod;                     /* maximum number of modules (limited memory)*/
#endif /* CHIFASTBUS */
int evt_max,evt;            /* number of events to be measured/ event counter */
double d_evt_max;
int wc;                                 /* word count from block readout      */
int pa;
int wt_tri,tri,wt_con;
ems_u32 con_pat, val, ssr;              /* "end of conversion"-module pattern */
ems_u32 *ptr;
int read_err;
int i,j;
int pa_cat;
plerrcode pl_tri,pl_cat,buf_check;
struct timeval first_tp,last_tp;        /* time structures for "gettimeofday" */
struct timezone first_tzp,last_tzp;
int patience;                           /* time still needed for measurement  */
int evt_old=0;
struct fastbus_dev* dev;
int modtype[32];
T(proc_fb_lc1881_measure_ped)

ptr= outptr;  

dev=modullist->entry[memberlist[1]].address.fastbus.dev;
pat= 0;
n_mod= 0;
for (i=1; i<=memberlist[0]; i++) {
  ml_entry* entry=modullist->entry+memberlist[i];
  int pa=entry->address.fastbus.pa;
  modtype[pa]=entry->modultype;
  if ((entry->modultype==LC_ADC_1881) || (entry->modultype==LC_ADC_1881M)) {
    pat|=1<<entry->address.fastbus.pa;
    n_mod++;
   }
 }
printf("lc1881_ped: pat=0x%x\n", pat);
if (!pat) {
  send_unsol_warning(0, 2, LC_ADC_1881, pat);
 }

if (n_mod) {                            /* allocate memory                    */
  if (!(sum_st= (double*) calloc((n_mod*N_CHAN),sizeof(double)))) {
    printf("fb_lc1881_measure_ped: allocate %lld bytes for sum_st: %s\n",
           (unsigned long long)((n_mod*N_CHAN)*sizeof(double)), strerror(errno));
    *outptr++=0;
    return(plErr_NoMem);
   }
  if (!(quad_st=(double*) calloc((n_mod*N_CHAN),sizeof(double)))) {
    free(sum_st);
    *outptr++=1;
    printf("fb_lc1881_measure_ped: allocate %lld bytes for quad_st: %s\n",
           (unsigned long long)((n_mod*N_CHAN)*sizeof(double)),strerror(errno));
    return(plErr_NoMem);
   }
  ped_st= NULL;
#ifdef CHIFASTBUS
  max_mod= (int)((float)(FB_BUFEND-FB_BUFBEG)/(float)(sizeof(int)*(N_CHAN+1)));
  if (n_mod > max_mod) {
    free(sum_st);free(quad_st);
    *outptr++=n_mod;*outptr++=max_mod;*outptr++=2;
    return(plErr_NoMem);
   }
  ped_st= (int*) FB_BUFBEG;
#endif /* CHIFASTBUS */
#ifdef FASTBUS_SIS3100_SFI /* XXX */
  ped_st=malloc(sizeof(ems_u32)*(N_CHAN+1)*n_mod);
  if (!ped_st)
    {
    printf("fb_lc1881_measure_ped: allocate %lld bytes for ped_st: %s\n",
           (unsigned long long)((n_mod*(N_CHAN+1))*sizeof(int)),strerror(errno));
    free(sum_st);free(quad_st);
    *outptr++=n_mod;*outptr++=-1;*outptr++=2;
    return(plErr_NoMem);
    }
#endif /* FASTBUS_SIS3100_SFI */
  D(D_USER,printf("ADC 1881: memory for pedestal measurement allocated\n");)

  evt_max= p[1]; evt= 0;

  switch (p[2]) {                       /* initialize trigger                 */
#ifdef CHIFASTBUS
    case 0:
    case 1: pl_tri= trigmod_init(p[3],1,1000,150);
            break;
    case 2: pl_tri= trigmod_soft_init(p[3],1,1000,150);
            break;
#endif /* CHIFASTBUS */
/*
XXX FASTBUS_SFI und FASTBUS_SIS3100_SFI darf keinen Unterschied bewirken!!!
FASTBUS_SIS3100_SFI ist nur interessant, wenn SIS3100 selber benutzt wird.
*/
#ifdef FASTBUS_SFI /* XXX */
    case 3: pl_tri= plOK; /* Sync. Modul not used with SFI */
            {
            int val=(p[3]<<16)&0xffff0000;
            ioctl(sfi.path, SFI_NIMOUT, &val);
            }
            break;
#endif /* FASTBUS_SFI */
#ifdef FASTBUS_SIS3100_SFI
    case 3: pl_tri= plOK; /* Sync. Modul not used with SFI */
            dev->LEVELOUT(dev, 0, 0, p[3]);
            break;
#endif /* FASTBUS_SIS3100_SFI */
    default:pl_tri= plErr_Program;
            break;
   }
  if (pl_tri!=plOK) {
    free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI
    free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
    printf("fb_lc1881_measure_ped: trigmod init failed\n");
    *outptr++=3;
    return(pl_tri);
   }
  D(D_USER,printf("ADC 1881: trigger for pedestal measurement initialized\n");)

    pa_cat=-1;
    if (p[0]>4) {
        pa_cat=p[5];
    } else {                     /* cat 1810 defined in IS?            */
        i=1;
        while ((!pa_cat) && (i<=memberlist[0])) {
            if (modullist->entry[memberlist[i]].modultype==LC_CAT_1810)
            pa_cat= memberlist[i];
            i++;
        }
    }
    if (pa_cat>=0) {
        D(D_USER,printf("CAT 1810(%2d) defined for pedestal measurement\n",pa_cat);)
    }

  evt_old= 0;
  gettimeofday(&first_tp,&first_tzp);
  while (evt<evt_max) {
    if ((!(evt%100)) && (evt!=evt_old)) {  /* send unsolicited message with   */
      evt_old= evt;                     /* estimation of time still needed    */
      gettimeofday(&last_tp,&last_tzp);
      patience= ((last_tp.tv_sec-first_tp.tv_sec)  * (evt_max-evt)) / evt;
      send_unsol_patience(0, 2, evt, patience);
     }
    DV(D_USER,printf("Event= %d von %d\n",(evt+1),evt_max);)
    wt_tri= MAX_WT_TRI; tri= 0;
    do {                                /* wait for trigger                   */
      switch (p[2]) {
#ifdef CHIFASTBUS
        case 0: tsleep(2);
                tri= trigmod_get();
                break;
        case 1: FBPULSE();
                tsleep(2);
                tri= trigmod_get();
                break;
        case 2: tri= trigmod_soft_get();
                break;
#endif /* CHIFASTBUS */
#ifdef FASTBUS_SFI /* XXX */
        case 3: FBPULSE_(&sfi, p[3]);
                tsleep(2);
                tri= 1;
                break;
#endif /* FASTBUS_SFI */
#ifdef FASTBUS_SIS3100_SFI
        case 3: dev->PULSEOUT(dev, 0, p[3]);
                tsleep(2);
                tri= 1;
                break;
#endif /* FASTBUS_SIS3100_SFI */
        default:break;
       }
      wt_tri--;
     } while ((wt_tri) && !(tri));
    if (wt_tri==0) {                    /* time elapsed: no trigger detected  */
      evt_max-=1;
      DV(D_USER,printf("Event %d: no trigger detected\n",(evt+1));)
      send_unsol_warning(1, 1, evt+1);
     }
    else {                              /* trigger detected                   */
      DV(D_USER,printf("ADC 1881: trigger detected after %d/%d\n",(MAX_WT_TRI-wt_tri),MAX_WT_TRI);)
      wt_con= MAX_WT_CON;               /* wait for end of conversion         */
      while ((wt_con--) && (dev->FRCM(dev, 0xBD, &val, &ssr), (val&pat)^pat)) {
       if (ssr) {
          free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI /* XXX */
          free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
          printf("fb_lc1881_measure_ped: ssr (test of conversion)= %d\n",ssr);
          *outptr++=ssr;*outptr++=4;
          return(plErr_HW);
         }
       }
      if (ssr) {
        free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI /* XXX */
        free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
        printf("fb_lc1881_measure_ped: ssr (end of conversion)= %d\n",ssr);
        *outptr++=ssr;*outptr++=5;
        return(plErr_HW);
       }
      if (wt_con==-1) {              /* time elapsed: conversion not completed*/
        evt_max-=1;
        if (dev->FRCM(dev, 0xBD, &con_pat, &ssr)<0) {
            printf("lc1881_ped: IO error in FRCM\n");
            return plErr_HW;
        }
        con_pat&=pat;
        if (ssr) {
          free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI /* XXX */
          free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
          printf("fb_lc1881_measure_ped: ssr (no conversion)= %d\n",ssr);
          *outptr++=ssr;*outptr++=6;
          return(plErr_HW);
         }
        send_unsol_warning(2, 3, evt+1, LC_ADC_1881, con_pat);
        DV(D_USER,printf("Event %d: conversion not completed, hit-pattern= %x\n",(evt+1),con_pat);)
       }
      else {                            /* conversion completed               */
        DV(D_USER,printf("ADC 1881: end of conversion after %d/%d\n",(MAX_WT_CON-wt_con),MAX_WT_CON);)
        if ((buf_check=lc1881_buf(dev, pat, 2, (evt+1), 0, 0))!=plOK) {
          free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI /* XXX */
          free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
          printf("fb_lc1881_measure_ped: buf_check= %d\n",buf_check);
          *outptr++=7;
          return(buf_check);
         }
        dev->FWCM(dev, (0x5 | (p[4]<<4)),0x400, &ssr);  /* LNE for broadcast class p[4]       */
        if (ssr) {
          free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI /* XXX */
          free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
          printf("fb_lc1881_measure_ped: ssr (load next event)= %d\n",ssr);
          *outptr++=ssr;*outptr++=8;
          return(plErr_HW);
         }
        dev->FRCM(dev, 0xCD, &sds_pat, &ssr);      /* Sparse Data Scan                   */
        sds_pat&=pat;
        if (ssr) {
          free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI /* XXX */
          free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
          printf("fb_lc1881_measure_ped: ssr (Sparse Data Scan)= %d\n",ssr);
          *outptr++=ssr;*outptr++=9;
          return(plErr_HW);
         }
        if (sds_pat^pat) {              /* modules with no valid data         */
          free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI /* XXX */
          free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
          printf("fb_lc1881_measure_ped: ssr (Sparse Data Scan)= %d\n",ssr);
          *outptr++=sds_pat;*outptr++=10;
          return(plErr_HW);
         }
        pa= 0; read_err=0;
        sum= sum_st; quad= quad_st; mod_st= ped_st; ped= ped_st;
        do {
          if (sds_pat&0x1) {      /* block readout for geographic address "pa"*/
            wc=dev->FRDB_S(dev, pa, (N_CHAN+1), ped, &ssr, "proc_fb_lc1881_measure_ped");
            if (ssr) {
              free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI /* XXX */
              free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
              printf("fb_lc1881_measure_ped: ssr (block readout)= %d (pa= %d)\n",ssr,pa);
              *outptr++=pa;*outptr++=wc;*outptr++=ssr;*outptr++=11;
              return(plErr_HW);
             }
            DV(D_USER,printf("ADC 1881(%2d): readout completed (%d words)\n",pa,wc);)
            ped= mod_st;
            if ((read_err=err_header(*ped,pa, modtype[pa]))) {/* error in header word      */
              evt_max-=1;
              send_unsol_warning(3, 4, evt+1, pa, *ped, read_err);
            } else {                      /* header word o.k.                   */
              ped++; j=1;
              do {
                if ((read_err= err_data(*ped,pa,(j-1), modtype[pa]))) {  /* err in data word */
                  evt_max-=1;
                  send_unsol_warning(3, 4, evt+1, pa, *ped, read_err);
                 }
                else {                  /* data word o.k.                     */
                  if (modtype[pa]==LC_ADC_1881)
                    *ped= *ped & 0xffffdfff;
                  DV(D_DUMP,printf("ADC 1881(%2d), chan %2d: pedestal=%4d\n",pa,(j-1),((*ped)&0x3fff));)
                  ped++; j++;
                 }
               } while ((j<=N_CHAN) && (!read_err));
              if (!read_err)  mod_st+=wc;
             }
           }
          sds_pat>>= 1; pa++;
         } while ((sds_pat) && (!read_err));
        if (!read_err) {                /* data from event o.k.               */
          ped= ped_st;
          for (i=1; i<=n_mod; i++) {    /* increase simple & quadratic sum    */
            ped++;
            for (j=1; j<=N_CHAN; j++) {
              data= (double)((*ped)&0x3fff);
              (*sum)+= data;
              (*quad)+= data * data;
              ped++; sum++; quad++;
             }
           }
          evt++;
         }
        DV(D_USER,printf("ADC 1881, evt %d: %d modules read out\n",(read_err?(evt+1):evt),n_mod);)
       }
      if (pa_cat>=0)  {/* reset cat and check for multiple trigger signals */
        if ((pl_cat= lc1810_check_reset(dev, pa_cat,(evt+1))) != plOK) {
          free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI /* XXX */
          free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
          printf("fb_lc1881_measure_ped: reset cat: %d\n",pl_cat);
          *outptr++=12;
          return(pl_cat);
         }
        DV(D_USER,printf("CAT 1810(%2d), evt %d: reset carried out\n",pa_cat,(evt+1));)
       }
     }
    if (evt<evt_max) {                  /* reset trigger                      */
      switch (p[2]) {
#ifdef CHIFASTBUS
    case 0:
        case 1: trigmod_reset();
                break;
        case 2: trigmod_soft_reset();
                break;
#endif /* CHIFASTBUS */
        default:break;
       }
     }
    else {       /* call "trigmod..done" after last trigger has been detected */
      switch (p[2]) {
#ifdef CHIFASTBUS
        case 0:
        case 1: pl_tri= trigmod_done();
                break;
        case 2: pl_tri= trigmod_soft_done();
                break;
#endif /* CHIFASTBUS */
        case 3: break;
        default:pl_tri= plErr_Program;
                break;
       }
      if (pl_tri!=plOK) {
        free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI /* XXX */
        free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
        printf("fb_lc1881_measure_ped: reset trigger: %d\n",pl_tri);
        *outptr++=13;
        return(pl_tri);
       }
     }
   }

  D(D_USER,printf("ADC 1881: %d trigger for pedestal measurement recorded\n",evt_max);)
  if (evt_max < p[1]) 
        send_unsol_warning(4, 1, evt_max);

  d_evt_max= (double)evt_max;
  mean_fact= 0.; sigm_fact= 0.;
  sum= sum_st; quad= quad_st;
  outptr++;
  fl_ptr= (float*)outptr;
  if (evt_max)
    mean_fact= 1. / d_evt_max;
  if ((evt_max-1)>0)
    sigm_fact= sqrt(1./(d_evt_max-1.));
  for (i=1; i<=n_mod; i++) {          /* calculate mean and standard deviation*/
    for (j=1; j<=N_CHAN; j++) {
      chan_sum= (*sum);  /* attention: the unnecessary variables are needed   */
      chan_quad= (*quad);  /* to get rid of a compiler error in incrementing  */
      mean= mean_fact * chan_sum;  /* the pointers "sum" and "quad"           */
      sigm= sigm_fact * sqrt((chan_quad - d_evt_max * mean*mean));
      *fl_ptr++= (float)mean;
      *fl_ptr++= (float)sigm;
      D(D_USER,printf("ADC 1881(%d), chan %2d: mean=%8.2f, sigma=%8.3f\n",i,(j-1),mean,sigm);)
      sum++; quad++;
     }
   }
  outptr+= 2*n_mod*N_CHAN;
  D(D_USER,printf("ADC 1881: pedestal analysis done\n");)

  free(sum_st);  free(quad_st);
 }

*ptr= n_mod;                            /* number of output words             */

return(plOK);

#undef MAX_WT_TRI
#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1881_measure_ped(ems_u32* p)
  /* test subroutine for "proc_fb_lc1881_measure_ped" */
{
int n_mod;                              /* number of modules                  */
int i;

T(test_proc_fb_lc1881_measure_ped)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

if ((p[0]!=4) && (p[0]!=5)) {
  *outptr++=0;
  printf("test_proc_fb_lc1881_measure_ped: wrong nb parameters %d\n",p[0]);
  return(plErr_ArgNum);
 }
                                        /* check parameter values             */
if (((unsigned int)p[1]<1)||((unsigned int)p[1]>50000)) {
  *outptr++=1;
  printf("test_proc_fb_lc1881_measure_ped: wrong nb triggers %d\n",p[1]);
  return(plErr_ArgRange);
 }
if ((unsigned int)p[2]>3) {
  *outptr++=2;
  printf("test_proc_fb_lc1881_measure_ped: wrong bitmask %d\n",p[2]);
  return(plErr_ArgRange);
 }
if ((((unsigned int)p[3]<1)||((unsigned int)p[3]>3)) && (p[2]<3)) {
  *outptr++=3;
  printf("test_proc_fb_lc1881_measure_ped: wrong bitmask %d\n",p[2]);
  return(plErr_ArgRange);
 }
if ((unsigned int)p[4]>3) {
  *outptr++=4;
  printf("test_proc_fb_lc1881_measure_ped: wrong bc class %d\n",p[3]);
  return(plErr_ArgRange);
 }

n_mod=0;
for (i=1; i<=memberlist[0]; i++) {
    ml_entry* module=modullist->entry+memberlist[i];
    if ((module->modultype==LC_ADC_1881) || (module->modultype==LC_ADC_1881M)) {
        n_mod++;
    }
}

wirbrauchen= (2*n_mod*N_CHAN) + 1;
return(plOK);
}


/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1881_build_ped[]="fb_lc1881_build_ped";
int ver_proc_fb_lc1881_build_ped=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1881_build_ped_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1881_build_ped()
{
return(&fb_lc1881_build_ped_prop);
}
#endif
/******************************************************************************/
/*
measurement and building of LeCroy ADC 1881 pedestals
=====================================================

parameters:
-----------
  p[ 1]: number of triggers:                       1..50000
  p[ 2]: GATE generator:                           0- external trigger
                                                   1- CHI trigger
                                                   2- GSI soft trigger
  p[ 3]: Address Space (CHI)                       1,2,3- base
  p[ 4]: broadcast class for readout               0..3- broadcast class 0..3
  p[ 5]: module pattern                            id of variable with module pattern
  p[ 6]: threshold setting                         0..15- pedestal= mean + n*sigma

output data format:
------------------
  ptr[0]:                3 * number of read out channels (int)

  for each channel in each read out module (start: n=1, 192 words per module):
  ptr[n]:                mean value for pedestal (float)
  ptr[n+1]:              standard deviation (float)
  ptr[n+2]:              threshold (int)
*/
/******************************************************************************/

plerrcode proc_fb_lc1881_build_ped(ems_u32* p)
{
#define MAX_WT_TRI  1000                /* wait for trigger: maximum timeout  */
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

float *fl_ptr;                    /* floating point format of returned values */
ems_u32 thresh, *thr_ptr;  /* calculated threshold and returned thresh. values*/
double *sum,*quad,*sum_st,*quad_st;    /* simple & quadratic sum of pedestals */
double data;
double chan_sum;                         /* simple sum for individual channel */
ems_u32 *ped,*mod_st,*ped_st;           /* read out pedestal values           */
double mean,sigm,mean_fact,sigm_fact; /* mean & standard deviation of pedestal*/
int pat;                                /* IS module pattern                  */
ems_u32 sds_pat;                        /* sparse data scan module pattern    */
int n_mod;                              /* number of modules                  */
#ifdef CHIFASTBUS
int max_mod;                     /* maximum number of modules (limited memory)*/
#endif /* CHIFASTBUS */
int evt_max,evt;            /* number of events to be measured/ event counter */
double d_evt_max;
int wc;                                 /* word count from block readout      */
int pa;
int wt_tri,tri,wt_con;
ems_u32 con_pat, ssr;                   /* "end of conversion"-module pattern */
ems_u32 *ptr;
int read_err;
int i,j;
int pa_cat;
plerrcode pl_tri,pl_cat,buf_check;
struct timeval first_tp,last_tp;        /* time structures for "gettimeofday" */
struct timezone first_tzp,last_tzp;
int patience;                           /* time still needed for measurement  */
int evt_old=0;
struct fastbus_dev* dev;
int modtype[32];

T(proc_fb_lc1881_build_ped)

ptr= outptr;

dev=(modullist->entry+(var_list[p[2]].var.ptr[0]))->address.fastbus.dev;
for (i=1; i<=memberlist[0]; i++) {
    ml_entry* entry=modullist->entry+memberlist[i];
    int pa=entry->address.fastbus.pa;
    modtype[pa]=entry->modultype;
}

pat= var_list[p[5]].var.val; n_mod= 0;
while (pat) {
  if (pat&0x1)  n_mod++;
  pat>>=1;
 }

if (n_mod) {                        /* allocate memory                    */
  if (!(sum_st= (double*) calloc((n_mod*N_CHAN),sizeof(double)))) {
    *outptr++=0;
    return(plErr_NoMem);
   }
  if (!(quad_st=(double*) calloc((n_mod*N_CHAN),sizeof(double)))) {
    free(sum_st);
    *outptr++=1;
    return(plErr_NoMem);
   }
  ped_st= NULL;
#ifdef CHIFASTBUS
  max_mod= (int)((float)(FB_BUFEND-FB_BUFBEG)/(float)(sizeof(int)*(N_CHAN+1)));
  if (n_mod > max_mod) {
    free(sum_st);free(quad_st);
    *outptr++=n_mod;*outptr++=max_mod;*outptr++=2;
    return(plErr_NoMem);
   }
  ped_st= (int*) FB_BUFBEG;
#endif /* CHIFASTBUS */
#ifdef FASTBUS_SIS3100_SFI
  ped_st=malloc(sizeof(ems_u32)*(N_CHAN+1)*n_mod);
  if (!ped_st)
    {
    free(sum_st);free(quad_st);
    *outptr++=n_mod;*outptr++=-1;*outptr++=2;
    return(plErr_NoMem);
    }
#endif /* FASTBUS_SIS3100_SFI */
  D(D_USER,printf("ADC 1881: memory for pedestal measurement allocated\n");)

  evt_max= p[1]; evt= 0;

  switch (p[2]) {                       /* initialize trigger                 */
#ifdef CHIFASTBUS
    case 0:
    case 1: pl_tri= trigmod_init(p[3],1,1000,150);
            break;
    case 2: pl_tri= trigmod_soft_init(p[3],1,1000,150);
            break;
#endif /* CHIFASTBUS */
#ifdef FASTBUS_SFI
    case 3: pl_tri= plOK; /* Sync. Modul not used with SFI */
            {
            int val=(p[3]<<16)&0xffff0000;
            ioctl(sfi.path, SFI_NIMOUT, &val);
            }
            break;
#endif /* FASTBUS_SFI */
#ifdef FASTBUS_SIS3100_SFI
    case 3: pl_tri= plOK; /* Sync. Modul not used with SFI */
            dev->LEVELOUT(dev, 0, 0, p[3]);
            break;
#endif /* FASTBUS_SIS3100_SFI */
    default:pl_tri= plErr_Program;
            break;
   }
  if (pl_tri!=plOK) {
    free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI
    free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
    *outptr++=3;
    return(pl_tri);
   }
  D(D_USER,printf("ADC 1881: trigger for pedestal measurement initialized\n");)

  pat= var_list[p[5]].var.val;
  i=1; pa_cat= 0;                       /* cat 1810 defined in IS?            */
  while ((!pa_cat) && (i<=memberlist[0])) {
    if (modullist->entry[memberlist[i]].modultype==LC_CAT_1810)
      pa_cat= memberlist[i];
    i++;
   }
  if (pa_cat)
    D(D_USER,printf("CAT 1810(%2d) defined for pedestal measurement\n",pa_cat);)

  evt_old= 0;
  gettimeofday(&first_tp,&first_tzp);
  while (evt<evt_max) {
    if ((!(evt%100)) && (evt!=evt_old)) {  /* send unsolicited message with   */
      evt_old= evt;                     /* estimation of time still needed    */
      gettimeofday(&last_tp,&last_tzp);
      patience= ((last_tp.tv_sec-first_tp.tv_sec)  * (evt_max-evt)) / evt;
      send_unsol_patience(0, 2, evt, patience);
     }
    DV(D_USER,printf("Event= %d von %d\n",(evt+1),evt_max);)
    wt_tri=MAX_WT_TRI; tri=0;
    do {                                /* wait for trigger                   */
      switch (p[2]) {
#ifdef CHIFASTBUS
        case 0: tsleep(2);
                tri= trigmod_get();
                break;
        case 1: FBPULSE();
                tsleep(2);
                tri= trigmod_get();
                break;
        case 2: tri= trigmod_soft_get();
                break;
#endif /* CHIFASTBUS */
#ifdef FASTBUS_SFI
        case 3: FBPULSE_(&sfi, p[3]);
                tsleep(2);
                tri= 1;
                break;
#endif /* FASTBUS_SFI */
#ifdef FASTBUS_SIS3100_SFI
        case 3: dev->PULSEOUT(dev, 0, p[3]);
                tsleep(2);
                tri= 1;
                break;
#endif /* FASTBUS_SIS3100_SFI */
        default:break;
       }
      wt_tri--;
     } while ((wt_tri) && !(tri));
    if (wt_tri==0) {                    /* time elapsed: no trigger detected  */
      evt_max-=1;
      DV(D_USER,printf("Event %d: no trigger detected\n",(evt+1));)
      send_unsol_warning(0, 1, evt+1);
     }
    else {                              /* trigger detected                   */
      DV(D_USER,printf("ADC 1881: trigger detected after %d/%d\n",(MAX_WT_TRI-wt_tri),MAX_WT_TRI);)
      wt_con= MAX_WT_CON;               /* wait for end of conversion         */
      dev->FRCM(dev, 0xBD, &con_pat, &ssr);
      DV(D_DUMP,printf("ADC 1881: conversion pattern %8x\n",con_pat);)
      while ((wt_con--) && ((con_pat&pat)^pat)) {
        if (ssr) {
          free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI
          free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
          *outptr++=ssr;*outptr++=4;
          return(plErr_HW);
         }
        dev->FRCM(dev, 0xBD, &con_pat, &ssr);
        DV(D_DUMP,printf("ADC 1881: conversion pattern %8x\n",con_pat);)
       }
      if (ssr) {
        free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI
        free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
        *outptr++=ssr;*outptr++=5;
        return(plErr_HW);
       }
      if (wt_con==-1) {              /* time elapsed: conversion not completed*/
        evt_max-=1;
        dev->FRCM(dev, 0xBD, &con_pat, &ssr);
        con_pat&=pat;
        if (ssr) {
          free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI
          free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
          *outptr++=ssr;*outptr++=6;
          return(plErr_HW);
         }
        send_unsol_warning(1, 3, evt+1, LC_ADC_1881, con_pat);
        DV(D_USER,printf("Event %d: conversion not completed, hit pattern= %x\n",(evt+1),con_pat);)
       }
      else {                            /* conversion completed               */
        DV(D_USER,printf("ADC 1881: end of conversion after %d/%d\n",(MAX_WT_CON-wt_con),MAX_WT_CON);)
        if ((buf_check=lc1881_buf(dev, pat, 2, (evt+1), 0, 0))!=plOK) {
          free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI
          free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
          *outptr++=7;
          return(buf_check);
         }
        dev->FWCM(dev, (0x5 | (p[4]<<4)),0x400, &ssr);  /* LNE for broadcast class p[4]       */
        if (ssr) {
          free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI
          free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
          *outptr++=ssr;*outptr++=8;
          return(plErr_HW);
         }
        dev->FRCM(dev, 0xCD, &sds_pat, &ssr);      /* Sparse Data Scan                   */
        sds_pat&=pat;
        if (ssr) {
          free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI
          free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
          *outptr++=ssr;*outptr++=9;
          return(plErr_HW);
         }
        if (sds_pat^pat) {              /* modules with no valid data         */
          free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI
          free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
          *outptr++=sds_pat;*outptr++=10;
          return(plErr_HW);
         }
        pa= 0; read_err=0;
        sum= sum_st; quad= quad_st; mod_st= ped_st; ped= ped_st;
        do {
          if (sds_pat&0x1) {      /* block readout for geographic address "pa"*/
            wc=dev->FRDB_S(dev, pa, (N_CHAN+1), ped, &ssr, "proc_fb_lc1881_build_ped");
            if (ssr) {
              free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI
              free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
              *outptr++=pa;*outptr++=wc;*outptr++=ssr;*outptr++=11;
              return(plErr_HW);
             }
            DV(D_USER,printf("ADC 1881(%2d): readout completed (%d words)\n",pa,wc);)
            ped= mod_st;
            if ((read_err=err_header(*ped,pa, modtype[pa]))) { /* error in header word      */
              evt_max-=1;
              send_unsol_warning(2, 4, evt+1, pa, *ped, read_err);
             }
            else {                      /* header word o.k.                   */
              ped++; j=1;
              do {
                if ((read_err= err_data(*ped,pa,(j-1), modtype[pa]))) {  /* err in data word */
                  evt_max-=1;
                  send_unsol_warning(2, 4, evt+1, pa, *ped, read_err);
                 }
                else {                  /* data word o.k.                     */
                  if (modtype[pa]==LC_ADC_1881)
                    *ped= *ped & 0xffffdfff;
                  DV(D_DUMP,printf("ADC 1881(%2d), chan %2d: pedestal=%4d\n",pa,(j-1),((*ped)&0x3fff));)
                  ped++; j++;
                 }
               } while ((j<=N_CHAN) && (!read_err));
              if (!read_err)  mod_st+=wc;
             }
           }
          sds_pat>>= 1; pa++;
         } while ((sds_pat) && (!read_err));
        if (!read_err) {                /* data from event o.k.               */
          ped= ped_st;
          for (i=1; i<=n_mod; i++) {    /* increase simple & quadratic sum    */
            ped++;
            for (j=1; j<=N_CHAN; j++) {
              data= (double)((*ped)&0x3fff);
              (*sum)+= data;
              (*quad)+= data * data;
              ped++; sum++; quad++;
             }
           }
          evt++;
         }
        DV(D_USER,printf("ADC 1881, evt %d: %d modules read out\n",(read_err?(evt+1):evt),n_mod);)
       }
      if (pa_cat)  {      /* reset cat and check for multiple trigger signals */
        if ((pl_cat= lc1810_check_reset(dev, pa_cat,(evt+1))) != plOK) {
          free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI
          free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
          *outptr++=12;
          return(pl_cat);
         }
        DV(D_USER,printf("CAT 1810(%2d), evt %d: reset carried out\n",pa_cat,(evt+1));)
       }
     }
    if (evt<evt_max) {                  /* reset trigger                      */
      switch (p[2]) {
#ifdef CHIFASTBUS
        case 0:
        case 1: trigmod_reset();
                break;
        case 2: trigmod_soft_reset();
                break;
#endif /* CHIFASTBUS */
        default:break;
       }
     }
    else {       /* call "trigmod..done" after last trigger has been detected */
      switch (p[2]) {
#ifdef CHIFASTBUS
        case 0:
        case 1: pl_tri= trigmod_done();
                break;
        case 2: pl_tri= trigmod_soft_done();
                break;
#endif /* CHIFASTBUS */
        case 3: break;
        default:pl_tri= plErr_Program;
                break;
       }
      if (pl_tri!=plOK) {
        free(sum_st);free(quad_st);
#ifdef FASTBUS_SIS3100_SFI
        free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
        *outptr++=13;
        return(pl_tri);
       }
     }
   }

  D(D_USER,printf("ADC 1881: %d trigger for pedestal measurement recorded\n",evt_max);)
  if (evt_max < p[1]) {
    send_unsol_warning(3, 1, evt_max);
   }

  d_evt_max= (double)evt_max;
  mean_fact= 0.; sigm_fact= 0.;
  sum= sum_st; quad= quad_st;
  outptr++;
  fl_ptr= (float*)outptr;  thr_ptr= outptr;
  if (evt_max)
    mean_fact= 1. / d_evt_max;
  if ((evt_max-1)>0)
    sigm_fact= sqrt(1./(d_evt_max-1.));
  for (i=1; i<=n_mod; i++) {            /* calculate mean, sigma and threshold*/
    for (j=1; j<=N_CHAN; j++) {
      chan_sum= (*sum);  /* attention: the unnecessary variables are needed   */
      mean= mean_fact * chan_sum;  /* the pointers "sum" and "quad"           */
      sigm= sigm_fact * sqrt(((*quad) - d_evt_max * mean*mean));
      *fl_ptr++= (float)mean;  thr_ptr++;
      *fl_ptr++= (float)sigm;  thr_ptr++;
      thresh= (int)(mean + ((double)p[6] * sigm) + 0.5);
      *thr_ptr++= thresh;  fl_ptr++;
      D(D_USER,printf("ADC 1881(%d), chan %2d: mean=%8.2f, sigma=%8.3f, thresh=%5d\n",i,(j-1),mean,sigm,thresh);)
      sum++; quad++;
     }
   }
  outptr+= 3*n_mod*N_CHAN;
  D(D_USER,printf("ADC 1881: pedestal analysis done\n");)

  free(sum_st);  free(quad_st);
#ifdef FASTBUS_SIS3100_SFI
  free(ped_st);
#endif /* FASTBUS_SIS3100_SFI */
 }

*ptr= n_mod;                            /* number of output words             */

return(plOK);

#undef MAX_WT_TRI
#undef MAX_WT_CON
}

/******************************************************************************/

plerrcode test_proc_fb_lc1881_build_ped(ems_u32* p)
  /* test subroutine for "proc_fb_lc1881_build_ped" */
{
int pat, hpat;                          /* IS module pattern                  */
int n_mod;                              /* number of modules                  */
int i;

T(test_proc_fb_lc1881_build_ped)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

if (p[0]!=6) {                          /* check number of parameters         */
  *outptr++=p[0];
  return(plErr_ArgNum);
 }
                                        /* check parameter values             */
if (((unsigned int)p[1]<1)||((unsigned int)p[1]>50000)) {
  *outptr++=1;
  return(plErr_ArgRange);
 }
if ((unsigned int)p[2]>3) {
  *outptr++=2;
  return(plErr_ArgRange);
 }
if ((((unsigned int)p[3]<1)||((unsigned int)p[3]>3)) && (p[2]<3)) {
  *outptr++=3;
  return(plErr_ArgRange);
 }
if ((unsigned int)p[4]>3) {
  *outptr++=4;
  return(plErr_ArgRange);
 }
if ((unsigned int)p[5]>MAX_VAR) {
  *outptr++=5;
  return(plErr_IllVar);
 }
if (!var_list[p[5]].len) {
  *outptr++=5;
  return(plErr_NoVar);
 }
if (var_list[p[5]].len!=1) {
  *outptr++=5;
  return(plErr_IllVarSize);
 }
if (((unsigned int)p[6]&0xf)!=(unsigned int)p[6]) {
  *outptr++=6;
  return(plErr_ArgRange);
 }

pat= var_list[p[5]].var.val;
hpat=0;
for (i=1; i<=memberlist[0]; i++) {
    ml_entry* module=modullist->entry+memberlist[i];
    if ((module->modultype==LC_ADC_1881) || (module->modultype==LC_ADC_1881M)) {
        hpat|=1<<module->address.fastbus.pa;
    }
}
if ((pat&hpat)!=pat) {
    return plErr_ArgRange;
}

n_mod=0;
for (i=1; i<=memberlist[0]; i++) {
    if (pat&(1<<i)) n_mod++;
}

wirbrauchen= (3*n_mod*N_CHAN) + 1;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1881_read_ped[]="fb_lc1881_read_ped";
int ver_proc_fb_lc1881_read_ped=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1881_read_ped_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1881_read_ped()
{
return(&fb_lc1881_read_ped_prop);
}
#endif
/******************************************************************************/
/*
readout of LeCroy ADC 1881 pedestals
====================================

output data format:
------------------
  ptr[0]:                number of read out channels

  for each channel in each read out module (start: n=1, 64 words per module):
  ptr[n]:                individual threshold
*/
/******************************************************************************/

plerrcode proc_fb_lc1881_read_ped(ems_u32* p)
  /* readout of pedestal values for fastbus lecroy adc1881 */
{
ems_u32 *ptr;
int n_mod;                              /* number of modules                  */
int sa,chan;
ems_u32 val,r_val;
int i;
ems_u32 ssr;                            /* slave status response              */

T(proc_fb_lc1881_read_ped)

ptr= outptr++; n_mod= 0;

for (i=1; i<=memberlist[0]; i++) {
  ml_entry* module=modullist->entry+memberlist[i];
  if ((module->modultype==LC_ADC_1881) || (module->modultype==LC_ADC_1881M)) {
    struct fastbus_dev* dev=module->address.fastbus.dev;
    int pa=module->address.fastbus.pa;

    D(D_USER, printf("ADC 1881(%2d): reading pedestals\n",pa);)

    dev->FWCa(dev, pa,0,1<<18, &ssr);   /* disable gate                       */
    if (ssr) {
      dev->FCDISC(dev);
      outptr--;*outptr++=pa;*outptr++=ssr;*outptr++=0;
      return(plErr_HW);
     }
    dev->FCRW(dev, &r_val, &ssr);
    if (ssr) {
      dev->FCDISC(dev);
      outptr--;*outptr++=pa;*outptr++=ssr;*outptr++=1;
      return(plErr_HW);
     }
    if (r_val&0x4) {
      dev->FCDISC(dev);
      outptr--;*outptr++=pa;*outptr++=0;*outptr++=2;
      return(plErr_HW);
     }

    for (chan=0; chan<=(N_CHAN-1); chan++) {  /* readout of pedestal memory   */
      sa= 0xc0000000 + chan;
      dev->FCRWS(dev, sa, &val, &ssr);
      val&=0xffff;
      if (ssr) {
        dev->FCDISC(dev);
        *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=3;
        return(plErr_HW);
       }
      *outptr++= val;
      DV(D_USER,printf("ADC 1881(%2d, chan %2d): ped= %5d\n",pa,chan,val);)
     }
    dev->FCWWS(dev, 0,1<<2, &ssr);
    if (ssr) {      /* enable gate                        */
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=ssr;*outptr++=7;
      return(plErr_HW);
     }
    dev->FCRW(dev, &r_val, &ssr);
    dev->FCDISC(dev);                           /* disconnect                         */
    if (ssr) {
      *outptr++=pa;*outptr++=ssr;*outptr++=8;
      return(plErr_HW);
     }
    if (!(r_val&0x4)) {
      *outptr++=pa;*outptr++=0;*outptr++=9;
      return(plErr_HW);
     }

    D(D_USER, printf("ADC 1881(%2d): pedestals read\n",pa);)
   }
 }

*ptr= n_mod*N_CHAN;                     /* number of read out modules         */

return(plOK);
}

/******************************************************************************/

plerrcode test_proc_fb_lc1881_read_ped(ems_u32* p)
  /* test subroutine for "proc_fb_lc1881_read_ped" */
{
int n_mod;                              /* number of modules                  */
int i;

T(test_proc_fb_lc1881_read_ped)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

n_mod= 0;

for (i=1; i<=memberlist[0]; i++) {
    ml_entry* entry=modullist->entry+memberlist[i];
    if ((entry->modultype==LC_ADC_1881) || (entry->modultype==LC_ADC_1881M))
            n_mod++;
 }

wirbrauchen= (n_mod*N_CHAN) + 1;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/
