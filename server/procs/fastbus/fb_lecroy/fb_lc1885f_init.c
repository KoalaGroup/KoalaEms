/*
 * procs/fastbus/fb_lecroy/fb_lc1885f_init.c
 *
 * MaWo
 * PeWue
 * 
 * created 11.05.95
 * 15.06.2000 PW adapted for SFI/Nanobridge
 * 16.Jan.2003 PW multi crate support
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fb_lc1885f_init.c,v 1.13 2011/04/06 20:30:31 wuestner Exp $";

#include <config.h>
#include <debug.h>
#include <stdio.h>
#include <math.h>
#include <utime.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/ioctl.h>
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
#include "fb_lc1810_init.h"

#define set_ctrl_bit(param, bit) (1<<((param)?(bit):(bit+16)))

#define N_CHAN  96                      /* number of channels in one module   */
#define N_SETUP_PAR  6                  /* number of setup parameters         */

extern ems_u32* outptr;
extern int *memberlist;
extern int wirbrauchen;

#define FB_BUFBEG 0
#define FB_BUFEND 0

RCS_REGISTER(cvsid, "procs/fastbus/fb_lecroy")

/******************************************************************************/

static int
err_data_85f(int word, int pa, int chan)
{

T(err_data_85f)

if (((word>>27)&0x1f) != pa)    {return(1);}  /* geographic address           */
if (((word>>16)&0x7f) != chan)  {return(2);}  /* channel number               */

return(0);
}

/******************************************************************************/

char name_proc_fb_lc1885f_setup[]="fb_lc1885f_setup";
int ver_proc_fb_lc1885f_setup=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1885f_setup_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1885f_setup()
{
return(&fb_lc1885f_setup_prop);
}
#endif
/******************************************************************************/
/*
Setup of LeCroy ADC 1885F
=========================

parameters:
-----------
  p[ 1] : number of adc1885f modules

module specific parameters:
---------------------------
  p[ 2]:                 operation mode:           0- F mode, 1- N mode
  p[ 3]: (CSR# 0< 8|24>) gate source:              0- front panel, 1- backplane
  p[ 4]: (CSR# 0< 9|25>) FCW source:               0- internal, 1- external
  p[ 5]: (CSR# 1<24:30>) active channel depth:     1..96- range of active channels
                                                      0..(n-1)
  p[ 6]: (CSR# 0<28|12>) autorange:                0- disable, 1- enable
  p[ 7]: (CSR# 0<29|13>) range:                    0- low, 1- high
         (ignored if parameter p[ 6]=1)
*/
/******************************************************************************/

plerrcode proc_fb_lc1885f_setup(ems_u32* p)
{
ems_u32 sa,val,r_val,csr_val;
int i;
int csr_mask;

T(proc_fb_lc1885f_setup)

p++;
for (i=1; i<=memberlist[0]; i++) {
  ml_entry* module=modullist->entry+memberlist[i];
  if (module->modultype==LC_ADC_1885F) {
    struct fastbus_dev* dev=module->address.fastbus.dev;
    int pa=module->address.fastbus.pa;
    ems_u32 ssr;

    D(D_USER,printf("ADC 1885F: pa= %d\n",pa);)

    sa= 0;                              /******          csr#0           ******/
    dev->FWCa(dev, pa, sa, 1<<11, &ssr);/* internal gate                      */
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=0;
      return(plErr_HW);
     }
    dev->FCWW(dev, 1<<27, &ssr);      /* clear                              */
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=1;
      return(plErr_HW);
     }
    tsleep(2);
    dev->FCWW(dev, 1<<30, &ssr);        /* reset                              */
    if (ssr) {                          /* reset                              */
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=2;
      return(plErr_HW);
     }

                                        /* test of operation mode             */
    dev->FCWW(dev, 1<<11, &ssr);      /* internal gate                      */
    if (ssr) {                          /* reset                              */
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=3;
      return(plErr_HW);
     }
    tsleep(2);

    sa= 1;                              /******           csr#1          ******/
    dev->FCRWS(dev, sa, &val, &ssr);    /* check conversion event counter     */
    val&=0xfff;
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=4;
      return(plErr_HW);
     }
    switch (val) {
      case 0x000:
        D(D_USER, printf("ADC 1885F(%2d): N mode\n",pa);)
        if (!p[1]) {
          dev->FCDISC(dev);
          *outptr++=pa;*outptr++=sa;*outptr++=5;
          return(plErr_HWTestFailed);
         }
        break;
      case 0x100:
        D(D_USER, printf("ADC 1885F(%2d): F mode\n",pa);)
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

    sa= 0;                              /******          csr#0           ******/
    dev->FCWWS(dev, sa, 1<<11, &ssr);   /* internal gate                      */
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=8;
      return(plErr_HW);
     }
    dev->FCWW(dev, 1<<27, &ssr);        /* clear                              */
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=9;
      return(plErr_HW);
     }
    tsleep(2);
    dev->FCWW(dev, 1<<30, &ssr);        /* reset                              */
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=10;
      return(plErr_HW);
     }

    sa= 0;                              /******          csr#0           ******/
    val= set_ctrl_bit(1,6) |            /* disable wait                       */
         set_ctrl_bit(0,10) |           /* disable test pulser                */
         set_ctrl_bit(1-p[2],8) |       /* gate source                        */
         set_ctrl_bit(1-p[3],9) |       /* FCW source                         */
         set_ctrl_bit(p[5],12);         /* autorange                          */
    csr_mask= 0x1740;
    if (!p[5]) {                        /* autorange disabled                 */
      val= val | set_ctrl_bit(p[6],13);  /* range                             */
      csr_mask= csr_mask | 0x2000;
     }
    csr_val= (val^set_ctrl_bit(1,6))&csr_mask; /* wait status                 */
    dev->FCWW(dev, val, &ssr);
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=11;
      return(plErr_HW);
     }
    dev->FCRW(dev, &r_val, &ssr);
    r_val&=csr_mask;
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=12;
      return(plErr_HW);
     }
    if (r_val^csr_val) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=13;
      return(plErr_HW);
     }

    sa= 1;                              /******           csr#1          ******/
    dev->FCRWS(dev, sa, &val, &ssr);           /* active channel depth        */
    val&=0x80ffffff;
    val|=p[4]<<24;
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=14;
      return(plErr_HW);
     }
    if (val != (p[4]<<24)) {
      ems_u32 unsol_mes[5]; int unsol_num;      
      unsol_num=5;unsol_mes[0]=6;unsol_mes[1]=3;
      unsol_mes[2]=pa;unsol_mes[3]=sa;unsol_mes[4]=val;
      send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
     }
    dev->FCWW(dev, val, &ssr);
    if (ssr) {
      dev->FCDISC(dev);
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=15;
      return(plErr_HW);
     }
    dev->FCRW(dev, &r_val, &ssr);
    dev->FCDISC(dev);                           /* disconnect                         */
    if (ssr) {
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=16;
      return(plErr_HW);
     }
    if (r_val^val) {
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=17;
      return(plErr_HW);
     }

    D(D_USER, printf("ADC 1885F(%2d): setup done\n",pa);)
    p+= N_SETUP_PAR;
   }
 }

return(plOK);
}


/******************************************************************************/

plerrcode test_proc_fb_lc1885f_setup(ems_u32* p)
{
int pa; 
int n_p,unused;
int i,i_p;
int msk_p[6]={0x1,0x1,0x1,0x7f,0x1,0x1};

T(test_proc_fb_lc1885f_setup)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

unused= p[0];
D(D_USER,printf("ADC 1885F: number of parameters for all modules= %d\n",unused);)

unused-= 1; n_p= 1;
if (unused%N_SETUP_PAR) {               /* check number of parameters         */
  *outptr++=0;
  return(plErr_ArgNum);
 }
if ((unused/N_SETUP_PAR)!=p[1]) {
  *outptr++=1; 
  return(plErr_ArgNum);
 }

for (i=1; i<=memberlist[0]; i++) {
  ml_entry* module=modullist->entry+memberlist[i];
  if (module->modultype==LC_ADC_1885F) {
    pa= memberlist[i];
    D(D_USER, printf("ADC 1885F(%2d): number of parameters not used= %d\n",pa,unused);)
    if (unused<N_SETUP_PAR) {/*not enough parameters for modules defined in IS*/
      *outptr++=pa;*outptr++=unused;
      return(plErr_ArgNum);
     }
    if (p[n_p+5])                       /* autorange enabled                  */
      msk_p[5]= 0xffffffff;
    for (i_p=1; i_p<=N_SETUP_PAR; i_p++) {  /* check parameter ranges     */
      if (((unsigned int)p[n_p+i_p]&msk_p[i_p-1])!=(unsigned int)p[n_p+i_p]) {
        *outptr++=pa;*outptr++=i_p;*outptr++=0;
        return(plErr_ArgRange);
       }
     }
    if (((unsigned int)p[n_p+4]<1) || ((unsigned int)p[n_p+4]>N_CHAN)) {
      *outptr++=pa;*outptr++=4;*outptr++=1;  /* add. check for active depth   */
      return(plErr_ArgRange);
     }
    n_p+= N_SETUP_PAR; unused-= N_SETUP_PAR;
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

char name_proc_fb_lc1885f_measure_ped[]="fb_lc1885f_measure_ped";
int ver_proc_fb_lc1885f_measure_ped=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1885f_measure_ped_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1885f_measure_ped()
{
return(&fb_lc1885f_measure_ped_prop);
}
#endif
/******************************************************************************/
/*
measurement of LeCroy ADC 1885F pedestals
=========================================

parameters:
-----------
  p[ 1]: number of triggers:                       1..50000
  p[ 2]: GATE generator:                           0- external trigger
                                                   1- CHI trigger
                                                   2- GSI soft trigger
  p[ 3]: Address Space (CHI)                       1,2,3- base
  p[ 4]: module pattern                            variable id

output data format:
------------------
  ptr[0]:                2 * number of read out channels (int)

  for each channel in each read out module (start: n=1, 192 words per module):
  ptr[n]:                mean value for pedestal (float)
  ptr[n+1]:              standard deviation (float)
*/
/******************************************************************************/

plerrcode proc_fb_lc1885f_measure_ped(ems_u32* p)
{
#define MAX_WT_TRI  1000                /* wait for trigger: maximum timeout  */
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

float *fl_ptr;                    /* floating point format of returned values */
double *sum,*quad,*sum_st,*quad_st;    /* simple & quadratic sum of pedestals */
double data;
double chan_sum, chan_quad;  /* simple & quadratic sum for individual channel */
ems_u32 *ped,*mod_st,*ped_st=0;         /* read out pedestal values           */
double mean,sigm,mean_fact,sigm_fact; /* mean & standard deviation of pedestal*/
int pat;                                /* IS module pattern                  */
ems_u32 sds_pat;                            /* sparse data scan module pattern    */
int n_mod;                              /* number of modules                  */
int evt_max,evt;            /* number of events to be measured/ event counter */
double d_evt_max;
int wc;                                 /* word count from block readout      */
int pa;
int wt_tri,tri,wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
ems_u32 *ptr;
int read_err;
int i,j;
int pa_cat;
plerrcode pl_tri,pl_cat;
struct timeval first_tp,last_tp;        /* time structures for "gettimeofday" */
struct timezone first_tzp,last_tzp;
int patience;                           /* time still needed for measurement  */
int evt_old=0;
ems_u32 ssr, csr;
struct fastbus_dev* dev;

T(proc_fb_lc1885f_measure_ped)

ptr= outptr;  

dev=modullist->entry[memberlist[1]].address.fastbus.dev;

pat= var_list[p[4]].var.val; n_mod= 0;
while (pat) {
  if (pat&0x1)  n_mod++;
  pat>>=1;
 }

if (n_mod) {                            /* allocate memory                    */
  if (!(sum_st= (double*) calloc((n_mod*N_CHAN),sizeof(double)))) {
    *outptr++=0;
    return(plErr_NoMem);
   }
  if (!(quad_st=(double*) calloc((n_mod*N_CHAN),sizeof(double)))) {
    free(sum_st);
    *outptr++=1;
    return(plErr_NoMem);
   }
#ifdef CHIFASTBUS
  max_mod= (int)((float)(FB_BUFEND-FB_BUFBEG)/(float)(sizeof(int)*(N_CHAN+1)));
  if (n_mod > max_mod) {
    free(sum_st);free(quad_st);
    *outptr++=n_mod;*outptr++=max_mod;*outptr++=2;
    return(plErr_NoMem);
   }
  ped_st= (int*) FB_BUFBEG;
#endif /* CHIFASTBUS */
#ifdef SFIFASTBUS /* XXX */
  ped_st=(int*)malloc(sizeof(int)*(N_CHAN+1)*n_mod);
  if (!ped_st)
    {
    free(sum_st);free(quad_st);
    *outptr++=n_mod;*outptr++=-1;*outptr++=2;
    return(plErr_NoMem);
    }
#endif /* SFIFASTBUS */
  D(D_USER,printf("ADC 1885F: memory for pedestal measurement allocated\n");)

  evt_max= p[1]; evt= 0;

  switch (p[2]) {                       /* initialize trigger                 */
#ifdef CHIFASTBUS
    case 0:
    case 1: pl_tri= trigmod_init(p[3],1,1000,150);
            break;
    case 2: pl_tri= trigmod_soft_init(p[3],1,1000,150);
            break;
#endif /* CHIFASTBUS */
#ifdef SFIFASTBUS /* XXX */
    case 3: pl_tri= plOK; /* Sync. Modul not used with SFI */
            {
            int val=(p[3]<<16)&0xffff0000;
            ioctl(sfi.path, SFI_NIMOUT, &val);
            }
            break;
#endif /* SFIFASTBUS */
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
    *outptr++=3;
    return(pl_tri);
   }
  D(D_USER,printf("ADC 1885F: trigger for pedestal measurement initialized\n");)

  pat= var_list[p[4]].var.val;
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
      { ems_u32 unsol_mes[3]; int unsol_num;
        unsol_num=4;unsol_mes[0]=0;unsol_mes[1]=2;
        unsol_mes[2]=evt;unsol_mes[3]=patience;
        send_unsolicited(Unsol_Patience,unsol_mes,unsol_num);
      }
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
#ifdef SFIFASTBUS /* XXX */
        case 3: FBPULSE_(&sfi, p[3]);
                tsleep(2);
                tri= 1;
                break;
#endif /* SFIFASTBUS */
        default:break;
       }
      wt_tri--;
     } while ((wt_tri) && !(tri));
    if (wt_tri==0) {                    /* time elapsed: no trigger detected  */
      evt_max-=1;
      DV(D_USER,printf("Event %d: no trigger detected\n",(evt+1));)
      { ems_u32 unsol_mes[3]; int unsol_num;
        unsol_num=3;unsol_mes[0]=0;unsol_mes[1]=1;
        unsol_mes[2]=(evt+1);
        send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
      }
     }
    else {                              /* trigger detected                   */
      DV(D_USER,printf("ADC 1885F: trigger detected after %d/%d\n",(MAX_WT_TRI-wt_tri),MAX_WT_TRI);)
      con_pat= 0;  pa= 0;
      wt_con= MAX_WT_CON;
      while (pat) {                     /* test conversion status             */
        pat>>= 1; pa++;
        if (pat&0x1) {
          dev->FRCa(dev, pa, 1, &csr, &ssr);
          if (ssr) {
            free(sum_st);free(quad_st);
#ifdef SFIFASTBUS /* XXX */
          free(ped_st);
#endif /* SFIFASTBUS */
            *outptr++=ssr;*outptr++=4;
            return(plErr_HW);
           }
          while ((wt_con--) && (dev->FCWW(dev, csr, &ssr), ssr)) {
            if (ssr>1) {               /* test ss code                       */
              dev->FCDISC(dev);
              free(sum_st);free(quad_st);
#ifdef SFIFASTBUS /* XXX */
              free(ped_st);
#endif /* SFIFASTBUS */
              *outptr++=pa;*outptr++=ssr;*outptr++=5;
              return(plErr_HW);
             }
           }
          dev->FCDISC(dev);                           /* disconnect                         */
          if (ssr>1) {                        /* test ss code                       */
            free(sum_st);free(quad_st);
#ifdef SFIFASTBUS /* XXX */
            free(ped_st);
#endif /* SFIFASTBUS */
            *outptr++=pa;*outptr++=ssr;*outptr++=6;
            return(plErr_HW);
           }
          if (!ssr) {                         /* conversion completed               */
            con_pat+= 1 << pa;
            D(D_DUMP,printf("ADC 1885F(%2d): end of conversion after %d/%d\n",pa,(MAX_WT_CON-wt_con),MAX_WT_CON);)
           }
         }
       }
      if (wt_con==-1) {              /* time elapsed: conversion not completed*/
        evt_max-=1;
        { ems_u32 unsol_mes[5]; int unsol_num;
          unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
          unsol_mes[2]=(evt+1);unsol_mes[3]=LC_ADC_1885F;unsol_mes[4]=con_pat;
          send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
         }
        DV(D_USER,printf("Event %d: conversion not completed, hit-pattern= %x\n",(evt+1),con_pat);)
       }
      else {                            /* conversion completed               */
        DV(D_USER,printf("ADC 1885F: end of conversion after %d/%d\n",(MAX_WT_CON-wt_con),MAX_WT_CON);)
        pat= var_list[p[4]].var.val;
        dev->FRCM(dev, 0x9D, &sds_pat, &ssr); /* Sparse Data Scan             */
        sds_pat&=pat;
        if (ssr) {
          free(sum_st);free(quad_st);
#ifdef SFIFASTBUS /* XXX */
          free(ped_st);
#endif /* SFIFASTBUS */
          *outptr++=ssr;*outptr++=7;
          return(plErr_HW);
         }
        if (sds_pat^pat) {              /* modules with no valid data         */
          free(sum_st);free(quad_st);
#ifdef SFIFASTBUS /* XXX */
          free(ped_st);
#endif /* SFIFASTBUS */
          *outptr++=sds_pat;*outptr++=8;
          return(plErr_HW);
         }
        pa= 0; read_err=0;
        sum= sum_st; quad= quad_st; mod_st= ped_st; ped= ped_st;
        do {                            /* valid data                         */
          if (sds_pat&0x1) {      /* block readout for geographic address "pa"*/
            wc=dev->FRDB_S(dev, pa, N_CHAN, ped, &ssr, "proc_fb_lc1885f_measure_ped");/*last data word invalid*/
            if (ssr) {
              free(sum_st);free(quad_st);
#ifdef SFIFASTBUS /* XXX */
              free(ped_st);
#endif /* SFIFASTBUS */
              *outptr++=pa;*outptr++=wc;*outptr++=ssr;*outptr++=9;
              return(plErr_HW);
             }
            DV(D_DUMP,printf("ADC 1885F(%2d): readout completed (%d words)\n",pa,wc);)
            ped= mod_st;
            j=1;
            do {
              if ((read_err=err_data_85f(*ped,pa,(j-1)))) {/* error in data word*/
                evt_max-=1;
                { ems_u32 unsol_mes[6];
                  int unsol_num;
                  unsol_num=6;unsol_mes[0]=2;unsol_mes[1]=4;
                  unsol_mes[2]=(evt+1);unsol_mes[3]=pa;unsol_mes[4]=*ped;unsol_mes[5]=read_err;
                  send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
                 }
               }
              else {                    /* data word o.k.                     */
                DV(D_DUMP,printf("ADC 1885F(%2d), chan %2d: pedestal=%4d\n",pa,(j-1),((*ped)&0xfff));)
                ped++; j++;
               }
             } while ((j<=N_CHAN) && (!read_err));
            if (!read_err)  mod_st+=wc;
           }
          sds_pat>>= 1; pa++;
         } while ((sds_pat) && (!read_err));
        if (!read_err) {                /* data from event o.k.               */
          ped= ped_st;
          for (i=1; i<=n_mod; i++) {    /* increase simple & quadratic sum    */
            for (j=1; j<=N_CHAN; j++) {
              if ((*ped)&0x800000) {data= (double)(((*ped)&0xfff)*8);}
              else                 {data= (double)((*ped)&0xfff);}
              (*sum)+= data;
              (*quad)+= data * data;
              ped++; sum++; quad++;
             }
           }
          evt++;
         }
        DV(D_USER,printf("ADC 1885F, evt %d: %d modules read out\n",(read_err?(evt+1):evt),n_mod);)
       }
      if (pa_cat)  {      /* reset cat and check for multiple trigger signals */
        if ((pl_cat= lc1810_check_reset(dev, pa_cat,(evt+1))) != plOK) {
          free(sum_st);free(quad_st);
#ifdef SFIFASTBUS /* XXX */
          free(ped_st);
#endif /* SFIFASTBUS */
          *outptr++=10;
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
#ifdef SFIFASTBUS /* XXX */
        free(ped_st);
#endif /* SFIFASTBUS */
        *outptr++=11;
        return(pl_tri);
       }
     }
   }

  D(D_USER,printf("ADC 1885F: %d trigger for pedestal measurement recorded\n",evt_max);)
  if (evt_max < p[1]) {
    ems_u32 unsol_mes[3]; int unsol_num;
    unsol_num=3;unsol_mes[0]=3;unsol_mes[1]=1;
    unsol_mes[2]=evt_max;
    send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
   }

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
      D(D_USER,printf("ADC 1885F(%d), chan %2d: mean=%8.2f, sigma=%8.3f\n",i,(j-1),mean,sigm);)
      sum++; quad++;
     }
   }
  outptr+= 2*n_mod*N_CHAN;
  D(D_USER,printf("ADC 1885F: pedestal analysis done\n");)

  free(sum_st);  free(quad_st);
 }

*ptr= n_mod;                            /* number of output words             */

return(plOK);

#undef MAX_WT_TRI
#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1885f_measure_ped(ems_u32* p)
{
int pat, hpat;                          /* IS module pattern                  */
int n_mod;                              /* number of modules                  */
int i;

T(test_proc_fb_lc1885f_measure_ped)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

if (p[0]!=4) {                          /* check number of parameters         */
  *outptr++=p[0];
  return(plErr_ArgNum);
 }
                                        /* check parameter values             */
if (((unsigned int)p[1]<1)||((unsigned int)p[1]>50000)) {
  *outptr++=1;
  return(plErr_ArgRange);
 }
if ((unsigned int)p[2]>2) {
  *outptr++=2;
  return(plErr_ArgRange);
 }
if (((unsigned int)p[3]<1)||((unsigned int)p[3]>3)) {
  *outptr++=3;
  return(plErr_ArgRange);
 }
if ((unsigned int)p[4]>MAX_VAR) {
  *outptr++=4;
  return(plErr_IllVar);
 }
if (!var_list[p[4]].len) {
  *outptr++=4;
  return(plErr_NoVar);
 }
if (var_list[p[4]].len!=1) {
  *outptr++=4;
  return(plErr_IllVarSize);
 }

pat= var_list[p[5]].var.val;
hpat=0;
for (i=1; i<=memberlist[0]; i++) {
    ml_entry* module=modullist->entry+memberlist[i];
    if (module->modultype==LC_ADC_1885F) {
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

wirbrauchen= (2*n_mod*N_CHAN) + 1;
return(plOK);
}


/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1885f_build_ped[]="fb_lc1885f_build_ped";
int ver_proc_fb_lc1885f_build_ped=1;

/******************************************************************************/
#ifdef PROCPROPS
static procprop fb_lc1885f_build_ped_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1885f_build_ped()
{
return(&fb_lc1885f_build_ped_prop);
}
#endif
/******************************************************************************/
/*
measurement and building of LeCroy ADC 1885F pedestals
======================================================

parameters:
-----------
  p[ 1]: number of triggers:                       1..50000
  p[ 2]: GATE generator:                           0- external trigger
                                                   1- CHI trigger
                                                   2- GSI soft trigger
  p[ 3]: Address Space (CHI)                       1,2,3- base
  p[ 4]: module pattern                            id of variable with module pattern
  p[ 5]: threshold setting                         0..15- pedestal= mean + n*sigma

output data format:
------------------
  ptr[0]:                3 * number of read out channels (int)

  for each channel in each read out module (start: n=1, 288 words per module):
  ptr[n]:                mean value for pedestal (float)
  ptr[n+1]:              standard deviation (float)
  ptr[n+2]:              threshold (int)
*/
/******************************************************************************/

plerrcode proc_fb_lc1885f_build_ped(ems_u32* p)
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
int max_mod;                     /* maximum number of modules (limited memory)*/
int evt_max,evt;            /* number of events to be measured/ event counter */
double d_evt_max;
int wc;                                 /* word count from block readout      */
int pa;
int wt_tri,tri,wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
ems_u32 *ptr;
int read_err;
int i,j;
int pa_cat;
plerrcode pl_tri,pl_cat;
struct timeval first_tp,last_tp;        /* time structures for "gettimeofday" */
struct timezone first_tzp,last_tzp;
int patience;                           /* time still needed for measurement  */
int evt_old;
ems_u32 ssr, csr;                       /* slave status response              */
struct fastbus_dev* dev;

T(proc_fb_lc1885f_build_ped)

ptr= outptr;

dev=(modullist->entry+(var_list[p[2]].var.ptr[0]))->address.fastbus.dev;

pat= var_list[p[4]].var.val; n_mod= 0;
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
  max_mod= (int)((float)(FB_BUFEND-FB_BUFBEG)/(float)(sizeof(int)*(N_CHAN+1)));
  if (n_mod > max_mod) {
    free(sum_st);free(quad_st);
    *outptr++=n_mod;*outptr++=max_mod;*outptr++=2;
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
#ifdef SFIFASTBUS
  ped_st=(int*)malloc(sizeof(int)*(N_CHAN+1)*n_mod);
  if (!ped_st)
    {
    free(sum_st);free(quad_st);
    *outptr++=n_mod;*outptr++=-1;*outptr++=2;
    return(plErr_NoMem);
    }
#endif /* SFIFASTBUS */
  D(D_USER,printf("ADC 1885F: memory for pedestal measurement allocated\n");)

  evt_max= p[1]; evt= 0;

  switch (p[2]) {                       /* initialize trigger                 */
#ifdef CHIFASTBUS
    case 0:
    case 1: pl_tri= trigmod_init(p[3],1,1000,150);
            break;
    case 2: pl_tri= trigmod_soft_init(p[3],1,1000,150);
            break;
#endif /* CHIFASTBUS */
#ifdef SFIFASTBUS
    case 3: pl_tri= plOK; /* Sync. Modul not used with SFI */
            {
            int val=(p[3]<<16)&0xffff0000;
            ioctl(sfi.path, SFI_NIMOUT, &val);
            }
            break;
#endif /* SFIFASTBUS */
    default:pl_tri= plErr_Program;
            break;
   }
  if (pl_tri!=plOK) {
    free(sum_st);free(quad_st);
#ifdef SFIFASTBUS
    free(ped_st);
#endif /* SFIFASTBUS */
    *outptr++=3;
    return(pl_tri);
   }
  D(D_USER,printf("ADC 1885F: trigger for pedestal measurement initialized\n");)

  pat= var_list[p[4]].var.val;
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
      { ems_u32 unsol_mes[3]; int unsol_num;
        unsol_num=4;unsol_mes[0]=0;unsol_mes[1]=2;
        unsol_mes[2]=evt;unsol_mes[3]=patience;
        send_unsolicited(Unsol_Patience,unsol_mes,unsol_num);
      }
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
#ifdef SFIFASTBUS
        case 3: FBPULSE_(&sfi, p[3]);
                tsleep(2);
                tri= 1;
                break;
#endif /* SFIFASTBUS */
        default:break;
       }
      wt_tri--;
     } while ((wt_tri) && !(tri));
    if (wt_tri==0) {                    /* time elapsed: no trigger detected  */
      evt_max-=1;
      DV(D_USER,printf("Event %d: no trigger detected\n",(evt+1));)
      { ems_u32 unsol_mes[3]; int unsol_num;
        unsol_num=3;unsol_mes[0]=0;unsol_mes[1]=1;
        unsol_mes[2]=(evt+1);
        send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
      }
     }
    else {                              /* trigger detected                   */
      DV(D_USER,printf("ADC 1885F: trigger detected after %d/%d\n",(MAX_WT_TRI-wt_tri),MAX_WT_TRI);)
      con_pat= 0;  pa= 0;
      wt_con= MAX_WT_CON;               /* wait for end of conversion         */
      while (pat) {                     /* test conversion status             */
        pat>>= 1; pa++;
        if (pat&0x1) {
          dev->FRCa(dev, pa, 1, &csr, &ssr);
          if (ssr) {
            free(sum_st);free(quad_st);
#ifdef SFIFASTBUS
            free(ped_st);
#endif /* SFIFASTBUS */
            *outptr++=ssr;*outptr++=4;
            return(plErr_HW);
           }
          while ((wt_con--) && (dev->FCWW(dev, csr, &ssr), ssr)) {
            if (ssr>1) {                /* test ss code                       */
              dev->FCDISC(dev);
              free(sum_st);free(quad_st);
#ifdef SFIFASTBUS
              free(ped_st);
#endif /* SFIFASTBUS */
              *outptr++=pa;*outptr++=ssr;*outptr++=5;
              return(plErr_HW);
             }
           }
          dev->FCDISC(dev);                           /* disconnect                         */
          if (ssr>1) {                        /* test ss code                       */
            free(sum_st);free(quad_st);
#ifdef SFIFASTBUS
            free(ped_st);
#endif /* SFIFASTBUS */
            *outptr++=pa;*outptr++=ssr;*outptr++=6;
            return(plErr_HW);
           }
          if (!ssr) {                         /* conversion completed               */
            con_pat+= 1 << pa;
            D(D_DUMP,printf("ADC 1885F(%2d): end of conversion after %d/%d\n",pa,(MAX_WT_CON-wt_con),MAX_WT_CON);)
           }
         }
       }
      if (wt_con==-1) {              /* time elapsed: conversion not completed*/
        evt_max-=1;
        { ems_u32 unsol_mes[5]; int unsol_num;
          unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
          unsol_mes[2]=(evt+1);unsol_mes[3]=LC_ADC_1885F;unsol_mes[4]=con_pat;
          send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
         }
        DV(D_USER,printf("Event %d: conversion not completed, hit-pattern= %x\n",(evt+1),con_pat);)
       }
      else {                            /* conversion completed               */
        DV(D_USER,printf("ADC 1885F: end of conversion after %d/%d\n",(MAX_WT_CON-wt_con),MAX_WT_CON);)
        pat= var_list[p[4]].var.val;
        dev->FRCM(dev, 0x9D, &sds_pat, &ssr); /* Sparse Data Scan             */
        sds_pat&=pat;
        if (ssr) {
          free(sum_st);free(quad_st);
#ifdef SFIFASTBUS
          free(ped_st);
#endif /* SFIFASTBUS */
          *outptr++=ssr;*outptr++=7;
          return(plErr_HW);
         }
        if (sds_pat^pat) {              /* modules with no valid data         */
          free(sum_st);free(quad_st);
          *outptr++=sds_pat;*outptr++=8;
          return(plErr_HW);
         }
        pa= 0; read_err=0;
        sum= sum_st; quad= quad_st; mod_st= ped_st; ped= ped_st;
        do {                            /* valid data                         */
          if (sds_pat&0x1) {      /* block readout for geographic address "pa"*/
            wc=dev->FRDB_S(dev, pa, N_CHAN, ped, &ssr, "proc_fb_lc1885f_build_ped");
            if (ssr) {
              free(sum_st);free(quad_st);
#ifdef SFIFASTBUS
              free(ped_st);
#endif /* SFIFASTBUS */
              *outptr++=pa;*outptr++=wc;*outptr++=ssr;*outptr++=9;
              return(plErr_HW);
             }
            DV(D_DUMP,printf("ADC 1885F(%2d): readout completed (%d words)\n",pa,wc);)
            ped= mod_st;
            j=1;
            do {
              if ((read_err=err_data_85f(*ped,pa,(j-1)))) { /* error in data word  */
                evt_max-=1;
                { ems_u32 unsol_mes[6];
                  int unsol_num;
                  unsol_num=6;unsol_mes[0]=2;unsol_mes[1]=4;
                  unsol_mes[2]=(evt+1);unsol_mes[3]=pa;unsol_mes[4]=*ped;unsol_mes[5]=read_err;
                  send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
                 }
               }
              else {                    /* data word o.k.                     */
                DV(D_DUMP,printf("ADC 1885F(%2d), chan %2d: pedestal=%4d\n",pa,(j-1),((*ped)&0xfff));)
                ped++; j++;
               }
             } while ((j<=N_CHAN) && (!read_err));
            if (!read_err)  mod_st+=wc;
           }
          sds_pat>>= 1; pa++;
         } while ((sds_pat) && (!read_err));
        if (!read_err) {                /* data from event o.k.               */
          ped= ped_st;
          for (i=1; i<=n_mod; i++) {    /* increase simple & quadratic sum    */
            for (j=1; j<=N_CHAN; j++) {
              if ((*ped)&0x800000) {data= (double)(((*ped)&0xfff)*8);}
              else                 {data= (double)((*ped)&0xfff);}
              (*sum)+= data;
              (*quad)+= data * data;
              ped++; sum++; quad++;
             }
           }
          evt++;
         }
        DV(D_USER,printf("ADC 1885F, evt %d: %d modules read out\n",(read_err?(evt+1):evt),n_mod);)
       }
      if (pa_cat)  {      /* reset cat and check for multiple trigger signals */
        if ((pl_cat= lc1810_check_reset(dev, pa_cat,(evt+1))) != plOK) {
          free(sum_st);free(quad_st);
#ifdef SFIFASTBUS
          free(ped_st);
#endif /* SFIFASTBUS */
          *outptr++=10;
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
#ifdef SFIFASTBUS
        free(ped_st);
#endif /* SFIFASTBUS */
        *outptr++=11;
        return(pl_tri);
       }
     }
   }

  D(D_USER,printf("ADC 1885F: %d trigger for pedestal measurement recorded\n",evt_max);)
  if (evt_max < p[1]) {
    ems_u32 unsol_mes[3]; int unsol_num;
    unsol_num=3;unsol_mes[0]=3;unsol_mes[1]=1;
    unsol_mes[2]=evt_max;
    send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
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
      thresh= (int)(mean + ((double)p[5] * sigm) + 0.5);
      *thr_ptr++= thresh;  fl_ptr++;
      D(D_USER,printf("ADC 1885F(%d), chan %2d: mean=%8.2f, sigma=%8.3f, thresh=%5d\n",i,(j-1),mean,sigm,thresh);)
      sum++; quad++;
     }
   }
  outptr+= 3*n_mod*N_CHAN;
  D(D_USER,printf("ADC 1885F: pedestal analysis done\n");)

  free(sum_st);  free(quad_st);
#ifdef SFIFASTBUS
  free(ped_st);
#endif /* SFIFASTBUS */
 }

*ptr= n_mod;                            /* number of output words             */

return(plOK);

#undef MAX_WT_TRI
#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1885f_build_ped(ems_u32* p)
{
int pat, hpat;                                /* IS module pattern                  */
int n_mod;                              /* number of modules                  */
int i;

T(test_proc_fb_lc1885f_build_ped)

if (memberlist==0) {                    /* check memberlist                   */
  return(plErr_NoISModulList);
 }
if (memberlist[0]==0) {
  return(plErr_BadISModList);
 }

if (p[0]!=5) {                          /* check number of parameters         */
  *outptr++=p[0];
  return(plErr_ArgNum);
 }
                                        /* check parameter values             */
if (((unsigned int)p[1]<1)||((unsigned int)p[1]>50000)) {
  *outptr++=1;
  return(plErr_ArgRange);
 }
if ((unsigned int)p[2]>2) {
  *outptr++=2;
  return(plErr_ArgRange);
 }
if (((unsigned int)p[3]<1)||((unsigned int)p[3]>3)) {
  *outptr++=3;
  return(plErr_ArgRange);
 }
if ((unsigned int)p[4]>MAX_VAR) {
  *outptr++=4;
  return(plErr_IllVar);
 }
if (!var_list[p[4]].len) {
  *outptr++=4;
  return(plErr_NoVar);
 }
if (var_list[p[4]].len!=1) {
  *outptr++=4;
  return(plErr_IllVarSize);
 }
if (((unsigned int)p[5]&0xf)!=(unsigned int)p[5]) {
  *outptr++=5;
  return(plErr_ArgRange);
 }

pat= var_list[p[4]].var.val;
hpat=0;
for (i=1; i<=memberlist[0]; i++) {
    ml_entry* module=modullist->entry+memberlist[i];
    if (module->modultype==LC_ADC_1885F) {
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
