/*******************************************************************************
*                                                                              *
* fb_lc1881_init.c                                                             *
*                                                                              *
* OS9                                                                          *
*                                                                              *
* MaWo                                                                         *
* PeWue                                                                        *
* MiZi                                                                         *
*                                                                              *
* 29.12.94                                                                     *
* 30.12.94  #undef for local constants                                         *
* 11. 1.95  CHI trigger for pedestal measurement implemented                   *
*  1. 2.95  Unsol_Data replaced by Unsol_Warning                               *
*  2. 2.95  Unsol_Patience added for pedestal measurement                      *
*           compiler error at pedestal analysis "corrected"                    *
* 09.10.97  compatible with ADC 1881M                                          *
*                                                                              *
*******************************************************************************/
#include <math.h>
#include <utime.h>
#include <config.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <unsoltypes.h>
#include "../../../lowlevel/chi_neu/fbacc.h"
#include "../../../objects/var/variables.h"
#include "../../procprops.h"

#define N_CHAN  64                      /* number of channels in one module   */
#define N_SETUP_PAR  7                  /* number of setup parameters         */

extern int *outptr;
extern int *memberlist;
extern int wirbrauchen;

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1881_setup[]="fb_lc1881_setup";
int ver_proc_fb_lc1881_setup=1;

/******************************************************************************/

static procprop fb_lc1881_setup_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1881_setup()
{
return(&fb_lc1881_setup_prop);
}

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
  p[ 8]:                 number of pedestal values 0,64
  p[ 9]: (CSR#C0000000)  pedestal value chan#0:    0..4095
  ...
  p[72]: (CSR#C000003f)  pedestal value chan#63:   0..4095
         (p[ 9]..p[72] ignored if parameter p[ 8]=0)
*/
/******************************************************************************/

plerrcode proc_fb_lc1881_setup(p)      
  /* setup of fastbus lecroy adc1881 */
int *p;
{
int pa,sa,val,r_val;
int i,chan;
int n_mod_par;                          /* number of parameters per module    */
int ssr;                                /* slave status response              */

T(proc_fb_lc1881_setup)

n_mod_par= N_SETUP_PAR + N_CHAN;
p++;
for (i=1; i<=memberlist[0]; i++) {
  if ((get_mod_type(memberlist[i])==LC_ADC_1881) || (get_mod_type(memberlist[i])==LC_ADC_1881M)) {
    pa= memberlist[i];
    D(D_USER,printf("ADC 1881: pa= %d\n",pa);)

    sa= 0;                              /******          csr#0           ******/
    FWCa(pa,sa,1<<30);                  /* master reset & primary address     */
    if (sscode()) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=0;
      return(plErr_HW);
     }

    sa= 1;                              /******          csr#1           ******/
    val= p[1]*(1<< 1)  |                /* gate via front panel/backplane     */
         p[2]*(1<< 3)  |                /* enable fast clear via backplane    */
         p[3]*(1<< 2)  |                /* fcw internal/via backplane         */
         p[4]*(1<<30);                  /* enable sparsification              */
    if (!p[3])
      val= val | p[5]*(1<<24);          /* internal FCW                       */
    if (get_mod_type(memberlist[i])==LC_ADC_1881M)
      val= val | 0x40;
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
    val= p[6];                          /* broadcast classes                  */
    if (ssr=(FCWWS(sa,val)&0x7)) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=4;
      return(plErr_HW);
     }
    r_val= FCRW() & 0xf;
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

    D(D_USER, printf("ADC 1881(%2d): parameter setup done\n",pa);)
    p+= N_SETUP_PAR;

    if (p[0]) {                         /* download threshold values          */
      for (chan=0; chan<=(N_CHAN-1); chan++) {
        sa= 0xc0000000 + chan;
        val= p[chan+1];
        DV(D_USER,printf("ADC 1881(%2d, chan %2d): ped= %4d\n",pa,chan,val);)
        if (ssr=(FCWWS(sa,val)&0x7)) {
          FCDISC();
          *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=7;
          return(plErr_HW);
         }
        r_val= FCRW() & 0xffff;
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
       }
      D(D_USER, printf("ADC 1881(%2d): pedestals downloaded\n",pa);)
      p+= N_CHAN;
     }

    sa= 0;                              /******           csr#0          ******/
    if (ssr=(FCWWS(sa,1<<2)&0x7)) {     /* enable gate                       */
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
    if (!(r_val&0x4)) {
      *outptr++=pa;*outptr++=sa;*outptr++=12;
      return(plErr_HW);
     }
   }
 }

return(plOK);
}


/******************************************************************************/

plerrcode test_proc_fb_lc1881_setup(p)       
  /* test subroutine for "proc_fb_lc1881_setup" */
int *p;
{
int pa; 
int n_p,unused;
int i,i_p;
int msk_p[7]={0x1,0x1,0x1,0x1,0xf,0xf,0xfff};
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
  if ((get_mod_type(memberlist[i])==LC_ADC_1881) || (get_mod_type(memberlist[i])==LC_ADC_1881M)) {
    pa= memberlist[i];
    D(D_USER, printf("ADC 1881(%2d): number of parameters not used= %d\n",pa,unused);)
    if (unused<N_SETUP_PAR) {/*not enough parameters for modules defined in IS*/
      *outptr++=pa;*outptr++=unused;*outptr++=0;
      return(plErr_ArgNum);
     }
    if (p[n_p+3])                       /* internal fcw ignored               */
      msk_p[4]= 0xffffffff;
    for (i_p=1; i_p<=(N_SETUP_PAR-1); i_p++) {  /* check parameter ranges     */
      if (((unsigned int)p[n_p+i_p]&msk_p[i_p-1])!=(unsigned int)p[n_p+i_p]) {
        *outptr++=pa;*outptr++=i_p;*outptr++=0;
        return(plErr_ArgRange);
       }
     }
    if (!p[n_p+3]) {                    /* additional check for internal fcw  */
      if ((unsigned int)p[n_p+5]>9) {
        *outptr++=pa;*outptr++=5;
        return(plErr_ArgRange);
       }
     }
    if ((unsigned int)p[n_p+6]<1) {    /* additional check for broadcast class*/
      *outptr++=pa;*outptr++=6;
      return(plErr_ArgRange);
     }
    n_p+= N_SETUP_PAR; unused-= N_SETUP_PAR;

    if ((p[n_p]!=0) && (p[n_p]!=N_CHAN)) {  /* number of pedestal values      */
      *outptr++=pa;*outptr++=7;
      return(plErr_ArgRange);
     }

    if (p[n_p]==N_CHAN) {               /* check pedestal values              */
      if (unused<N_CHAN) {
        *outptr++=pa;*outptr++=unused;*outptr++=1;
        return(plErr_ArgNum);
       }
      for (i_p=1; i_p<=N_CHAN; i_p++) {
        if (((unsigned int)p[n_p+i_p]&msk_p[6])!=(unsigned int)p[n_p+i_p]) {
          *outptr++=pa;*outptr++=i_p;*outptr++=1;
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
/******************************************************************************/

char name_proc_fb_lc1881_measure_ped[]="fb_lc1881_measure_ped";
int ver_proc_fb_lc1881_measure_ped=1;

/******************************************************************************/

static procprop fb_lc1881_measure_ped_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1881_measure_ped()
{
return(&fb_lc1881_measure_ped_prop);
}

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
  p[ 3]: Address Space (CHI)                       1,2,3- base
  p[ 4]: broadcast class for readout               0..3- broadcast class 0..3
  p[ 5]: module pattern                            id of variable with module pattern

output data format:
------------------
  ptr[0]:                2 * number of read out channels (int)

  for each channel in each read out module (start: n=1, 128 words per module):
  ptr[n]:                mean value for pedestal (float)
  ptr[n+1]:              standard deviation (float)
*/
/******************************************************************************/

plerrcode proc_fb_lc1881_measure_ped(p)
int *p;
{
#define MAX_WT_TRI  1000                /* wait for trigger: maximum timeout  */
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

float *fl_ptr;                    /* floating point format of returned values */
double *sum,*quad,*sum_st,*quad_st;    /* simple & quadratic sum of pedestals */
double data;
double chan_sum, chan_quad;  /* simple & quadratic sum for individual channel */
int *ped,*mod_st,*ped_st;               /* read out pedestal values           */
double mean,sigm,mean_fact,sigm_fact; /* mean & standard deviation of pedestal*/
int pat;                                /* IS module pattern                  */
int sds_pat;                            /* sparse data scan module pattern    */
int n_mod;                              /* number of modules                  */
int max_mod;                     /* maximum number of modules (limited memory)*/
int evt_max,evt;            /* number of events to be measured/ event counter */
double d_evt_max;
int wc;                                 /* word count from block readout      */
int pa;
int wt_tri,tri,wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
int *ptr;
int read_err;
int i,j;
int pa_cat;
plerrcode pl_tri,pl_cat,buf_check;
struct timeval first_tp,last_tp;        /* time structures for "gettimeofday" */
struct timezone first_tzp,last_tzp;
int patience;                           /* time still needed for measurement  */
int evt_old;

T(proc_fb_lc1881_measure_ped)

ptr= outptr;  

pat= var_list[p[5]].var.val; n_mod= 0;
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
  max_mod= (int)((float)(FB_BUFEND-FB_BUFBEG)/(float)(sizeof(int)*(N_CHAN+1)));
  if (n_mod > max_mod) {
    free(sum_st);free(quad_st);
    *outptr++=n_mod;*outptr++=max_mod;*outptr++=2;
    return(plErr_NoMem);
   }
  ped_st= (int*) FB_BUFBEG;
  D(D_USER,printf("ADC 1881: memory for pedestal measurement allocated\n");)

  evt_max= p[1]; evt= 0;

  switch (p[2]) {                       /* initialize trigger                 */
    case 0:
    case 1: pl_tri= trigmod_init(p[3],1,1000,150);
            break;
    case 2: pl_tri= trigmod_soft_init(p[3],1,1000,150);
            break;
    default:pl_tri= plErr_Program;
            break;
   }
  if (pl_tri!=plOK) {
    free(sum_st);free(quad_st);
    *outptr++=3;
    return(pl_tri);
   }
  D(D_USER,printf("ADC 1881: trigger for pedestal measurement initialized\n");)

  pat= var_list[p[5]].var.val;
  i=1; pa_cat= 0;                       /* cat 1810 defined in IS?            */
  while ((!pa_cat) && (i<=memberlist[0])) {
    if (get_mod_type(memberlist[i])==LC_CAT_1810)
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
      { int unsol_mes[3],unsol_num;
        unsol_num=4;unsol_mes[0]=0;unsol_mes[1]=2;
        unsol_mes[2]=evt;unsol_mes[3]=patience;
        send_unsolicited(Unsol_Patience,unsol_mes,unsol_num);
      }
     }
    DV(D_USER,printf("Event= %d von %d\n",(evt+1),evt_max);)
    wt_tri= MAX_WT_TRI; tri= 0;
    do {                                /* wait for trigger                   */
      switch (p[2]) {
        case 0: tsleep(2);
                tri= trigmod_get();
                break;
        case 1: FBPULSE();
                tsleep(2);
                tri= trigmod_get();
                break;
        case 2: tri= trigmod_soft_get();
                break;
        default:break;
       }
      wt_tri--;
     } while ((wt_tri) && !(tri));
    if (wt_tri==0) {                    /* time elapsed: no trigger detected  */
      evt_max-=1;
      DV(D_USER,printf("Event %d: no trigger detected\n",(evt+1));)
      { int unsol_mes[3],unsol_num;
        unsol_num=3;unsol_mes[0]=0;unsol_mes[1]=1;
        unsol_mes[2]=(evt+1);
        send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
      }
     }
    else {                              /* trigger detected                   */
      DV(D_USER,printf("ADC 1881: trigger detected after %d/%d\n",(MAX_WT_TRI-wt_tri),MAX_WT_TRI);)
      wt_con= MAX_WT_CON;               /* wait for end of conversion         */
      while ((wt_con--) && ((FRCM(0xBD)&pat)^pat)) {
        if (sscode()) {
          free(sum_st);free(quad_st);
          *outptr++=sscode();*outptr++=4;
          return(plErr_HW);
         }
       }
      if (sscode()) {
        free(sum_st);free(quad_st);
        *outptr++=sscode();*outptr++=5;
        return(plErr_HW);
       }
      if (wt_con==-1) {              /* time elapsed: conversion not completed*/
        evt_max-=1;
        con_pat= FRCM(0xBD) & pat;
        if (sscode()) {
          free(sum_st);free(quad_st);
          *outptr++=sscode();*outptr++=6;
          return(plErr_HW);
         }
        { int unsol_mes[5],unsol_num;
          unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
          unsol_mes[2]=(evt+1);unsol_mes[3]=LC_ADC_1881;unsol_mes[4]=con_pat;
          send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
         }
        DV(D_USER,printf("Event %d: conversion not completed, hit-pattern= %x\n",(evt+1),con_pat);)
       }
      else {                            /* conversion completed               */
        DV(D_USER,printf("ADC 1881: end of conversion after %d/%d\n",(MAX_WT_CON-wt_con),MAX_WT_CON);)
        if ((buf_check=lc1881_buf(pat,2,(evt+1)))!=plOK) {
          free(sum_st);free(quad_st);
          *outptr++=7;
          return(buf_check);
         }
        FWCM((0x5 | (p[4]<<4)),0x400);  /* LNE for broadcast class p[4]       */
        if (sscode()) {
          free(sum_st);free(quad_st);
          *outptr++=sscode();*outptr++=8;
          return(plErr_HW);
         }
        sds_pat= FRCM(0xCD) & pat;      /* Sparse Data Scan                   */
        if (sscode()) {
          free(sum_st);free(quad_st);
          *outptr++=sscode();*outptr++=9;
          return(plErr_HW);
         }
        if (sds_pat^pat) {              /* modules with no valid data         */
          free(sum_st);free(quad_st);
          *outptr++=sds_pat;*outptr++=10;
          return(plErr_HW);
         }
        pa= 0; read_err=0;
        sum= sum_st; quad= quad_st; mod_st= ped_st; ped= ped_st;
        do {
          if (sds_pat&0x1) {      /* block readout for geographic address "pa"*/
            wc= FRDB_S(pa,ped,(N_CHAN+1));
            if (sscode()) {
              free(sum_st);free(quad_st);
              *outptr++=pa;*outptr++=wc;*outptr++=sscode();*outptr++=11;
              return(plErr_HW);
             }
            DV(D_USER,printf("ADC 1881(%2d): readout completed (%d words)\n",pa,wc);)
            ped= mod_st;
            if (read_err= err_header(*ped,pa)) { /* error in header word      */
              evt_max-=1;
              { int unsol_mes[6],unsol_num;
                unsol_num=6;unsol_mes[0]=2;unsol_mes[1]=4;
                unsol_mes[2]=(evt+1);unsol_mes[3]=pa;unsol_mes[4]=*ped;unsol_mes[5]=read_err;
                send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
               }
             }
            else {                      /* header word o.k.                   */
              ped++; j=1;
              do {
                if (read_err= err_data(*ped,pa,(j-1))) {  /* err in data word */
                  evt_max-=1;
                  { int unsol_mes[6],unsol_num;
                    unsol_num=6;unsol_mes[0]=2;unsol_mes[1]=4;
                    unsol_mes[2]=(evt+1);unsol_mes[3]=pa;unsol_mes[4]=*ped;unsol_mes[5]=read_err;
                    send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
                   }
                 }
                else {                  /* data word o.k.                     */
                  if (get_mod_type(pa)==LC_ADC_1881)
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
        if ((pl_cat= lc1810_check_reset(pa_cat,(evt+1))) != plOK) {
          free(sum_st);free(quad_st);
          *outptr++=12;
          return(pl_cat);
         }
        DV(D_USER,printf("CAT 1810(%2d), evt %d: reset carried out\n",pa_cat,(evt+1));)
       }
     }
    if (evt<evt_max) {                  /* reset trigger                      */
      switch (p[2]) {
        case 0:
        case 1: trigmod_reset();
                break;
        case 2: trigmod_soft_reset();
                break;
        default:break;
       }
     }
    else {       /* call "trigmod..done" after last trigger has been detected */
      switch (p[2]) {
        case 0:
        case 1: pl_tri= trigmod_done();
                break;
        case 2: pl_tri= trigmod_soft_done();
                break;
        default:pl_tri= plErr_Program;
                break;
       }
      if (pl_tri!=plOK) {
        free(sum_st);free(quad_st);
        *outptr++=13;
        return(pl_tri);
       }
     }
   }

  D(D_USER,printf("ADC 1881: %d trigger for pedestal measurement recorded\n",evt_max);)
  if (evt_max < p[1]) {
    int unsol_mes[3],unsol_num;
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

plerrcode test_proc_fb_lc1881_measure_ped(p)
  /* test subroutine for "proc_fb_lc1881_measure_ped" */
int *p;
{
int pat;                                /* IS module pattern                  */
int n_mod;                              /* number of modules                  */
int pa, isml_def;
int i;

T(test_proc_fb_lc1881_measure_ped)

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

pat= var_list[p[5]].var.val; n_mod= 0; pa= 0;
while (pat) {                            /* check: module defined in IS       */
  if (pat&0x1) {
    i= 1; isml_def= 0;
    do {
      if (pa==memberlist[i]) {
        if ((get_mod_type(memberlist[i])!=LC_ADC_1881) && (get_mod_type(memberlist[i])!=LC_ADC_1881M)) {
          *outptr++=pa;
          return(plErr_BadModTyp);
         }
        isml_def++;
       }
      i++;
     } while ((i<=memberlist[0]) && (!isml_def));
    if (!isml_def) {                    /* module not defined in IS           */
      *outptr++=pa;
      return(plErr_BadISModList);
     }
    n_mod++;
   }
  pat>>= 1; pa++;
 }

wirbrauchen= (2*n_mod*N_CHAN) + 1;
return(plOK);
}


/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1881_build_ped[]="fb_lc1881_build_ped";
int ver_proc_fb_lc1881_build_ped=1;

/******************************************************************************/

static procprop fb_lc1881_build_ped_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1881_build_ped()
{
return(&fb_lc1881_build_ped_prop);
}

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

plerrcode proc_fb_lc1881_build_ped(p)
int *p;
{
#define MAX_WT_TRI  1000                /* wait for trigger: maximum timeout  */
#define MAX_WT_CON  100                 /* wait for conversion: max. timeout  */

float *fl_ptr;                    /* floating point format of returned values */
int thresh,*thr_ptr;       /* calculated threshold and returned thresh. values*/
double *sum,*quad,*sum_st,*quad_st;    /* simple & quadratic sum of pedestals */
double data;
double chan_sum, chan_quad;  /* simple & quadratic sum for individual channel */
int *ped,*mod_st,*ped_st;               /* read out pedestal values           */
double mean,sigm,mean_fact,sigm_fact; /* mean & standard deviation of pedestal*/
int pat;                                /* IS module pattern                  */
int sds_pat;                            /* sparse data scan module pattern    */
int n_mod;                              /* number of modules                  */
int max_mod;                     /* maximum number of modules (limited memory)*/
int evt_max,evt;            /* number of events to be measured/ event counter */
double d_evt_max;
int wc;                                 /* word count from block readout      */
int pa;
int wt_tri,tri,wt_con;
int con_pat;                            /* "end of conversion"-module pattern */
int *ptr;
int read_err;
int i,j;
int pa_cat;
plerrcode pl_tri,pl_cat,buf_check;
struct timeval first_tp,last_tp;        /* time structures for "gettimeofday" */
struct timezone first_tzp,last_tzp;
int patience;                           /* time still needed for measurement  */
int evt_old;

T(proc_fb_lc1881_build_ped)

ptr= outptr;

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
  max_mod= (int)((float)(FB_BUFEND-FB_BUFBEG)/(float)(sizeof(int)*(N_CHAN+1)));
  if (n_mod > max_mod) {
    free(sum_st);free(quad_st);
    *outptr++=n_mod;*outptr++=max_mod;*outptr++=2;
    return(plErr_NoMem);
   }
  ped_st= (int*) FB_BUFBEG;
  D(D_USER,printf("ADC 1881: memory for pedestal measurement allocated\n");)

  evt_max= p[1]; evt= 0;

  switch (p[2]) {                       /* initialize trigger                 */
    case 0:
    case 1: pl_tri= trigmod_init(p[3],1,1000,150);
            break;
    case 2: pl_tri= trigmod_soft_init(p[3],1,1000,150);
            break;
    default:pl_tri= plErr_Program;
            break;
   }
  if (pl_tri!=plOK) {
    free(sum_st);free(quad_st);
    *outptr++=3;
    return(pl_tri);
   }
  D(D_USER,printf("ADC 1881: trigger for pedestal measurement initialized\n");)

  pat= var_list[p[5]].var.val;
  i=1; pa_cat= 0;                       /* cat 1810 defined in IS?            */
  while ((!pa_cat) && (i<=memberlist[0])) {
    if (get_mod_type(memberlist[i])==LC_CAT_1810)
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
      { int unsol_mes[3],unsol_num;
        unsol_num=4;unsol_mes[0]=0;unsol_mes[1]=2;
        unsol_mes[2]=evt;unsol_mes[3]=patience;
        send_unsolicited(Unsol_Patience,unsol_mes,unsol_num);
      }
     }
    DV(D_USER,printf("Event= %d von %d\n",(evt+1),evt_max);)
    wt_tri=MAX_WT_TRI; tri=0;
    do {                                /* wait for trigger                   */
      switch (p[2]) {
        case 0: tsleep(2);
                tri= trigmod_get();
                break;
        case 1: FBPULSE();
                tsleep(2);
                tri= trigmod_get();
                break;
        case 2: tri= trigmod_soft_get();
                break;
        default:break;
       }
      wt_tri--;
     } while ((wt_tri) && !(tri));
    if (wt_tri==0) {                    /* time elapsed: no trigger detected  */
      evt_max-=1;
      DV(D_USER,printf("Event %d: no trigger detected\n",(evt+1));)
      { int unsol_mes[3],unsol_num;
        unsol_num=3;unsol_mes[0]=0;unsol_mes[1]=1;
        unsol_mes[2]=(evt+1);
        send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
      }
     }
    else {                              /* trigger detected                   */
      DV(D_USER,printf("ADC 1881: trigger detected after %d/%d\n",(MAX_WT_TRI-wt_tri),MAX_WT_TRI);)
      wt_con= MAX_WT_CON;               /* wait for end of conversion         */
      con_pat= FRCM(0xBD);
      DV(D_DUMP,printf("ADC 1881: conversion pattern %8x\n",con_pat);)
      while ((wt_con--) && ((con_pat&pat)^pat)) {
        if (sscode()) {
          free(sum_st);free(quad_st);
          *outptr++=sscode();*outptr++=4;
          return(plErr_HW);
         }
        con_pat= FRCM(0xBD);
        DV(D_DUMP,printf("ADC 1881: conversion pattern %8x\n",con_pat);)
       }
      if (sscode()) {
        free(sum_st);free(quad_st);
        *outptr++=sscode();*outptr++=5;
        return(plErr_HW);
       }
      if (wt_con==-1) {              /* time elapsed: conversion not completed*/
        evt_max-=1;
        con_pat= FRCM(0xBD) & pat;
        if (sscode()) {
          free(sum_st);free(quad_st);
          *outptr++=sscode();*outptr++=6;
          return(plErr_HW);
         }
        { int unsol_mes[5],unsol_num;
          unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
          unsol_mes[2]=(evt+1);unsol_mes[3]=LC_ADC_1881;unsol_mes[4]=con_pat;
          send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
         }
        DV(D_USER,printf("Event %d: conversion not completed, hit pattern= %x\n",(evt+1),con_pat);)
       }
      else {                            /* conversion completed               */
        DV(D_USER,printf("ADC 1881: end of conversion after %d/%d\n",(MAX_WT_CON-wt_con),MAX_WT_CON);)
        if ((buf_check=lc1881_buf(pat,2,(evt+1)))!=plOK) {
          free(sum_st);free(quad_st);
          *outptr++=7;
          return(buf_check);
         }
        FWCM((0x5 | (p[4]<<4)),0x400);  /* LNE for broadcast class p[4]       */
        if (sscode()) {
          free(sum_st);free(quad_st);
          *outptr++=sscode();*outptr++=8;
          return(plErr_HW);
         }
        sds_pat= FRCM(0xCD) & pat;      /* Sparse Data Scan                   */
        if (sscode()) {
          free(sum_st);free(quad_st);
          *outptr++=sscode();*outptr++=9;
          return(plErr_HW);
         }
        if (sds_pat^pat) {              /* modules with no valid data         */
          free(sum_st);free(quad_st);
          *outptr++=sds_pat;*outptr++=10;
          return(plErr_HW);
         }
        pa= 0; read_err=0;
        sum= sum_st; quad= quad_st; mod_st= ped_st; ped= ped_st;
        do {
          if (sds_pat&0x1) {      /* block readout for geographic address "pa"*/
            wc= FRDB_S(pa,ped,(N_CHAN+1));
            if (sscode()) {
              free(sum_st);free(quad_st);
              *outptr++=pa;*outptr++=wc;*outptr++=sscode();*outptr++=11;
              return(plErr_HW);
             }
            DV(D_USER,printf("ADC 1881(%2d): readout completed (%d words)\n",pa,wc);)
            ped= mod_st;
            if (read_err= err_header(*ped,pa)) { /* error in header word      */
              evt_max-=1;
              { int unsol_mes[6],unsol_num;
                unsol_num=6;unsol_mes[0]=2;unsol_mes[1]=4;
                unsol_mes[2]=(evt+1);unsol_mes[3]=pa;unsol_mes[4]=*ped;unsol_mes[5]=read_err;
                send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
               }
             }
            else {                      /* header word o.k.                   */
              ped++; j=1;
              do {
                if (read_err= err_data(*ped,pa,(j-1))) {  /* err in data word */
                  evt_max-=1;
                  { int unsol_mes[6],unsol_num;
                    unsol_num=6;unsol_mes[0]=2;unsol_mes[1]=4;
                    unsol_mes[2]=(evt+1);unsol_mes[3]=pa;unsol_mes[4]=*ped;unsol_mes[5]=read_err;
                    send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
                   }
                 }
                else {                  /* data word o.k.                     */
                  if (get_mod_type(pa)==LC_ADC_1881)
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
        if ((pl_cat= lc1810_check_reset(pa_cat,(evt+1))) != plOK) {
          free(sum_st);free(quad_st);
          *outptr++=12;
          return(pl_cat);
         }
        DV(D_USER,printf("CAT 1810(%2d), evt %d: reset carried out\n",pa_cat,(evt+1));)
       }
     }
    if (evt<evt_max) {                  /* reset trigger                      */
      switch (p[2]) {
        case 0:
        case 1: trigmod_reset();
                break;
        case 2: trigmod_soft_reset();
                break;
        default:break;
       }
     }
    else {       /* call "trigmod..done" after last trigger has been detected */
      switch (p[2]) {
        case 0:
        case 1: pl_tri= trigmod_done();
                break;
        case 2: pl_tri= trigmod_soft_done();
                break;
        default:pl_tri= plErr_Program;
                break;
       }
      if (pl_tri!=plOK) {
        free(sum_st);free(quad_st);
        *outptr++=13;
        return(pl_tri);
       }
     }
   }

  D(D_USER,printf("ADC 1881: %d trigger for pedestal measurement recorded\n",evt_max);)
  if (evt_max < p[1]) {
    int unsol_mes[3],unsol_num;
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
      chan_quad= (*quad);  /* to get rid of a compiler error in incrementing  */
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
 }

*ptr= n_mod;                            /* number of output words             */

return(plOK);

#undef MAX_WT_TRI
#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1881_build_ped(p)
  /* test subroutine for "proc_fb_lc1881_build_ped" */
int *p;
{
int pat;                                /* IS module pattern                  */
int n_mod;                              /* number of modules                  */
int pa, isml_def;
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
if ((unsigned int)p[2]>2) {
  *outptr++=2;
  return(plErr_ArgRange);
 }
if (((unsigned int)p[3]<1)||((unsigned int)p[3]>3)) {
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

pat= var_list[p[5]].var.val; n_mod= 0; pa= 0;
while (pat) {                            /* check: module defined in IS       */
  if (pat&0x1) {
    i= 1; isml_def= 0;
    do {
      if (pa==memberlist[i]) {
        if ((get_mod_type(memberlist[i])!=LC_ADC_1881) && (get_mod_type(memberlist[i])!=LC_ADC_1881M)) {
          *outptr++=pa;
          return(plErr_BadModTyp);
         }
        isml_def++;
       }
      i++;
     } while ((i<=memberlist[0]) && (!isml_def));
    if (!isml_def) {                    /* module not defined in IS           */
      *outptr++=pa;
      return(plErr_BadISModList);
     }
    n_mod++;
   }
  pat>>= 1; pa++;
 }

wirbrauchen= (3*n_mod*N_CHAN) + 1;
return(plOK);
}


/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1881_read_ped[]="fb_lc1881_read_ped";
int ver_proc_fb_lc1881_read_ped=1;

/******************************************************************************/

static procprop fb_lc1881_read_ped_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1881_read_ped()
{
return(&fb_lc1881_read_ped_prop);
}

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

plerrcode proc_fb_lc1881_read_ped()
  /* readout of pedestal values for fastbus lecroy adc1881 */
{
int *ptr;
int n_mod;                              /* number of modules                  */
int pa,sa,chan;
int val,r_val;
int i;
int ssr;                                /* slave status response              */

T(proc_fb_lc1881_read_ped)

ptr= outptr++; n_mod= 0;

for (i=1; i<=memberlist[0]; i++) {
  if ((get_mod_type(memberlist[i])==LC_ADC_1881) || (get_mod_type(memberlist[i])==LC_ADC_1881M)) {
    pa= memberlist[i]; n_mod++;
    D(D_USER, printf("ADC 1881(%2d): reading pedestals\n",pa);)

    FWCa(pa,0,1<<18);                   /* disable gate                       */
    if (sscode()) {
      FCDISC();
      outptr--;*outptr++=pa;*outptr++=sscode();*outptr++=0;
      return(plErr_HW);
     }
    r_val= FCRW();
    if (sscode()) {
      FCDISC();
      outptr--;*outptr++=pa;*outptr++=sscode();*outptr++=1;
      return(plErr_HW);
     }
    if (r_val&0x4) {
      FCDISC();
      outptr--;*outptr++=pa;*outptr++=0;*outptr++=2;
      return(plErr_HW);
     }

    for (chan=0; chan<=(N_CHAN-1); chan++) {  /* readout of pedestal memory   */
      sa= 0xc0000000 + chan;
      val= FCRWS(sa) & 0xffff;
      if (sscode()) {
        FCDISC();
        *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=3;
        return(plErr_HW);
       }
      *outptr++= val;
      DV(D_USER,printf("ADC 1881(%2d, chan %2d): ped= %5d\n",pa,chan,val);)
     }

    if (ssr=(FCWWS(0,1<<2)&0x7)) {      /* enable gate                        */
      FCDISC();
      *outptr++=pa;*outptr++=ssr;*outptr++=7;
      return(plErr_HW);
     }
    r_val= FCRW();
    FCDISC();                           /* disconnect                         */
    if (sscode()) {
      *outptr++=pa;*outptr++=sscode();*outptr++=8;
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

plerrcode test_proc_fb_lc1881_read_ped()
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
  if ((get_mod_type(memberlist[i])==LC_ADC_1881) || (get_mod_type(memberlist[i])==LC_ADC_1881M))
    n_mod++;
 }

wirbrauchen= (n_mod*N_CHAN) + 1;
return(plOK);
}


/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

int err_header(word,pa)
int word,pa;
{
int parity;

T(err_header)

if ((word>>27) != pa)           {return(1);}  /* geographic address           */
if (get_mod_type(pa)==LC_ADC_1881) {
  if (((word>>17)&0x7f) != 127) {return(2);}  /* channel= 127 if header       */
 }
else if (get_mod_type(pa)==LC_ADC_1881M) {
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

int err_data(word,pa,chan)
int word,pa,chan;
{
int parity;

T(err_data)

if (((word>>27)&0x1f) != pa)    {return(1);}  /* geographic address           */
if (((word>>17)&0x3f) != chan)  {return(2);}  /* channel number               */
if (get_mod_type(pa)==LC_ADC_1881) {
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
/******************************************************************************/
