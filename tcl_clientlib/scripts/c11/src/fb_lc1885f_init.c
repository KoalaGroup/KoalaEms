/*******************************************************************************
*                                                                              *
* fb_lc1885f_init.c                                                             *
*                                                                              *
* OS9                                                                          *
*                                                                              *
* MaWo                                                                         *
* PeWue                                                                        *
*                                                                              *
* 11. 5.95                                                                     *
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

#define set_ctrl_bit(param, bit) (1<<((param)?(bit):(bit+16)))

#define N_CHAN  96                      /* number of channels in one module   */
#define N_SETUP_PAR  6                  /* number of setup parameters         */

extern int *outptr;
extern int *memberlist;
extern int wirbrauchen;

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_lc1885f_setup[]="fb_lc1885f_setup";
int ver_proc_fb_lc1885f_setup=1;

/******************************************************************************/

static procprop fb_lc1885f_setup_prop= {0,0,0,0};

procprop* prop_proc_fb_lc1885f_setup()
{
return(&fb_lc1885f_setup_prop);
}

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

plerrcode proc_fb_lc1885f_setup(p)      
  /* setup of fastbus lecroy adc1885f */
int *p;
{
int pa,sa,val,r_val,csr_val;
int i;
int csr_mask;
int ssr;                                /* slave status response              */

T(proc_fb_lc1885f_setup)

p++;
for (i=1; i<=memberlist[0]; i++) {
  if (get_mod_type(memberlist[i])==LC_ADC_1885F) {
    pa= memberlist[i];
    D(D_USER,printf("ADC 1885F: pa= %d\n",pa);)

    sa= 0;                              /******          csr#0           ******/
    FWCa(pa,sa,1<<11);                  /* internal gate                      */
    if (sscode()) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=0;
      return(plErr_HW);
     }
    if (ssr=(FCWWss(1<<27)&0x7)) {      /* clear                              */
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=1;
      return(plErr_HW);
     }
    tsleep(2);
    if (ssr=(FCWWss(1<<30)&0x7)) {      /* reset                              */
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=2;
      return(plErr_HW);
     }

                                        /* test of operation mode             */
    if (ssr=(FCWWss(1<<11)&0x7)) {      /* internal gate                      */
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=3;
      return(plErr_HW);
     }
    tsleep(2);

    sa= 1;                              /******           csr#1          ******/
    val= FCRWS(sa) & 0xfff;             /* check conversion event counter     */
    if (sscode()) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=4;
      return(plErr_HW);
     }
    switch (val) {
      case 0x000:
        D(D_USER, printf("ADC 1885F(%2d): N mode\n",pa);)
        if (!p[1]) {
          FCDISC();
          *outptr++=pa;*outptr++=sa;*outptr++=5;
          return(plErr_HWTestFailed);
         }
        break;
      case 0x100:
        D(D_USER, printf("ADC 1885F(%2d): F mode\n",pa);)
        if (p[1]) {
          FCDISC();
          *outptr++=pa;*outptr++=sa;*outptr++=6;
          return(plErr_HWTestFailed);
         }
        break;
      default:
        FCDISC();
        *outptr++=pa;*outptr++=sa;*outptr++=7;
        return(plErr_HW);
        break;
     }

    sa= 0;                              /******          csr#0           ******/
    if (ssr=(FCWWS(sa,1<<11)&0x7)) {    /* internal gate                      */
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=8;
      return(plErr_HW);
     }
    if (ssr=(FCWWss(1<<27)&0x7)) {      /* clear                              */
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=9;
      return(plErr_HW);
     }
    tsleep(2);
    if (ssr=(FCWWss(1<<30)&0x7)) {      /* reset                              */
      FCDISC();
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
    if (ssr=(FCWWss(val)&0x7)) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=11;
      return(plErr_HW);
     }
    r_val= FCRW() & csr_mask;
    if (sscode()) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=12;
      return(plErr_HW);
     }
    if (r_val^csr_val) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=r_val;*outptr++=13;
      return(plErr_HW);
     }

    sa= 1;                              /******           csr#1          ******/
    val=(p[4]<<24) | (FCRWS(sa) & 0x80ffffff); /* active channel depth        */
    if (sscode()) {
      FCDISC();
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=14;
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
      *outptr++=pa;*outptr++=sa;*outptr++=ssr;*outptr++=15;
      return(plErr_HW);
     }
    r_val= FCRW();
    FCDISC();                           /* disconnect                         */
    if (sscode()) {
      *outptr++=pa;*outptr++=sa;*outptr++=sscode();*outptr++=16;
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

plerrcode test_proc_fb_lc1885f_setup(p)       
  /* test subroutine for "proc_fb_lc1885f_setup" */
int *p;
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
  if (get_mod_type(memberlist[i])==LC_ADC_1885F) {
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

static procprop fb_lc1885f_measure_ped_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1885f_measure_ped()
{
return(&fb_lc1885f_measure_ped_prop);
}

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

plerrcode proc_fb_lc1885f_measure_ped(p)
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
int wt_tri,tri,wt_con,csr;
int con_pat;                            /* "end of conversion"-module pattern */
int *ptr;
int read_err;
int i,j;
int pa_cat;
plerrcode pl_tri,pl_cat;
struct timeval first_tp,last_tp;        /* time structures for "gettimeofday" */
struct timezone first_tzp,last_tzp;
int patience;                           /* time still needed for measurement  */
int evt_old;
int ssr;                                /* slave status response              */

T(proc_fb_lc1885f_measure_ped)

ptr= outptr;  

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
  max_mod= (int)((float)(FB_BUFEND-FB_BUFBEG)/(float)(sizeof(int)*(N_CHAN+1)));
  if (n_mod > max_mod) {
    free(sum_st);free(quad_st);
    *outptr++=n_mod;*outptr++=max_mod;*outptr++=2;
    return(plErr_NoMem);
   }
  ped_st= (int*) FB_BUFBEG;
  D(D_USER,printf("ADC 1885F: memory for pedestal measurement allocated\n");)

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
  D(D_USER,printf("ADC 1885F: trigger for pedestal measurement initialized\n");)

  pat= var_list[p[4]].var.val;
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
      DV(D_USER,printf("ADC 1885F: trigger detected after %d/%d\n",(MAX_WT_TRI-wt_tri),MAX_WT_TRI);)
      con_pat= 0;  pa= 0;
      wt_con= MAX_WT_CON;
      while (pat) {                     /* test conversion status             */
        pat>>= 1; pa++;
        if (pat&0x1) {
          csr= FRCa(pa,1);
          if (sscode()) {
            free(sum_st);free(quad_st);
            *outptr++=sscode();*outptr++=4;
            return(plErr_HW);
           }
          while ((wt_con--) && (ssr=(FCWWss(csr)&0x7))) {
            if (ssr>1) {               /* test ss code                       */
              FCDISC();
              free(sum_st);free(quad_st);
              *outptr++=pa;*outptr++=ssr;*outptr++=5;
              return(plErr_HW);
             }
           }
          FCDISC();                           /* disconnect                         */
          if (ssr>1) {                        /* test ss code                       */
            free(sum_st);free(quad_st);
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
        { int unsol_mes[5],unsol_num;
          unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
          unsol_mes[2]=(evt+1);unsol_mes[3]=LC_ADC_1885F;unsol_mes[4]=con_pat;
          send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
         }
        DV(D_USER,printf("Event %d: conversion not completed, hit-pattern= %x\n",(evt+1),con_pat);)
       }
      else {                            /* conversion completed               */
        DV(D_USER,printf("ADC 1885F: end of conversion after %d/%d\n",(MAX_WT_CON-wt_con),MAX_WT_CON);)
        pat= var_list[p[4]].var.val;
        sds_pat= FRCM(0x9D) & pat;      /* Sparse Data Scan                   */
        if (sscode()) {
          free(sum_st);free(quad_st);
          *outptr++=sscode();*outptr++=7;
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
            wc= FRDB_S(pa,ped,N_CHAN);  /* last data word invalid             */
            if (sscode()) {
              free(sum_st);free(quad_st);
              *outptr++=pa;*outptr++=wc;*outptr++=sscode();*outptr++=9;
              return(plErr_HW);
             }
            DV(D_DUMP,printf("ADC 1885F(%2d): readout completed (%d words)\n",pa,wc);)
            ped= mod_st;
            j=1;
            do {
              if (read_err= err_data_85f(*ped,pa,(j-1))) { /* error in data word  */
                evt_max-=1;
                { int unsol_mes[6],unsol_num;
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
        if ((pl_cat= lc1810_check_reset(pa_cat,(evt+1))) != plOK) {
          free(sum_st);free(quad_st);
          *outptr++=10;
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
        *outptr++=11;
        return(pl_tri);
       }
     }
   }

  D(D_USER,printf("ADC 1885F: %d trigger for pedestal measurement recorded\n",evt_max);)
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

plerrcode test_proc_fb_lc1885f_measure_ped(p)
  /* test subroutine for "proc_fb_lc1885f_measure_ped" */
int *p;
{
int pat;                                /* IS module pattern                  */
int n_mod;                              /* number of modules                  */
int pa, isml_def;
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

pat= var_list[p[4]].var.val; n_mod= 0; pa= 0;
while (pat) {                            /* check: module defined in IS       */
  if (pat&0x1) {
    i= 1; isml_def= 0;
    do {
      if (pa==memberlist[i]) {
        if (get_mod_type(memberlist[i]) != LC_ADC_1885F) {
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

char name_proc_fb_lc1885f_build_ped[]="fb_lc1885f_build_ped";
int ver_proc_fb_lc1885f_build_ped=1;

/******************************************************************************/

static procprop fb_lc1885f_build_ped_prop= {1,-1,0,0};

procprop* prop_proc_fb_lc1885f_build_ped()
{
return(&fb_lc1885f_build_ped_prop);
}

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

plerrcode proc_fb_lc1885f_build_ped(p)
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
int wt_tri,tri,wt_con,csr;
int con_pat;                            /* "end of conversion"-module pattern */
int *ptr;
int read_err;
int i,j;
int pa_cat;
plerrcode pl_tri,pl_cat;
struct timeval first_tp,last_tp;        /* time structures for "gettimeofday" */
struct timezone first_tzp,last_tzp;
int patience;                           /* time still needed for measurement  */
int evt_old;
int ssr;                                /* slave status response              */

T(proc_fb_lc1885f_build_ped)

ptr= outptr;

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
  ped_st= (int*) FB_BUFBEG;
  D(D_USER,printf("ADC 1885F: memory for pedestal measurement allocated\n");)

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
  D(D_USER,printf("ADC 1885F: trigger for pedestal measurement initialized\n");)

  pat= var_list[p[4]].var.val;
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
      DV(D_USER,printf("ADC 1885F: trigger detected after %d/%d\n",(MAX_WT_TRI-wt_tri),MAX_WT_TRI);)
      con_pat= 0;  pa= 0;
      wt_con= MAX_WT_CON;               /* wait for end of conversion         */
      while (pat) {                     /* test conversion status             */
        pat>>= 1; pa++;
        if (pat&0x1) {
          csr= FRCa(pa,1);
          if (sscode()) {
            free(sum_st);free(quad_st);
            *outptr++=sscode();*outptr++=4;
            return(plErr_HW);
           }
          while ((wt_con--) && (ssr=(FCWWss(csr)&0x7))) {
            if (ssr>1) {                /* test ss code                       */
              FCDISC();
              free(sum_st);free(quad_st);
              *outptr++=pa;*outptr++=ssr;*outptr++=5;
              return(plErr_HW);
             }
           }
          FCDISC();                           /* disconnect                         */
          if (ssr>1) {                        /* test ss code                       */
            free(sum_st);free(quad_st);
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
        { int unsol_mes[5],unsol_num;
          unsol_num=5;unsol_mes[0]=1;unsol_mes[1]=3;
          unsol_mes[2]=(evt+1);unsol_mes[3]=LC_ADC_1885F;unsol_mes[4]=con_pat;
          send_unsolicited(Unsol_Warning,unsol_mes,unsol_num);
         }
        DV(D_USER,printf("Event %d: conversion not completed, hit-pattern= %x\n",(evt+1),con_pat);)
       }
      else {                            /* conversion completed               */
        DV(D_USER,printf("ADC 1885F: end of conversion after %d/%d\n",(MAX_WT_CON-wt_con),MAX_WT_CON);)
        pat= var_list[p[4]].var.val;
        sds_pat= FRCM(0x9D) & pat;      /* Sparse Data Scan                   */
        if (sscode()) {
          free(sum_st);free(quad_st);
          *outptr++=sscode();*outptr++=7;
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
            wc= FRDB_S(pa,ped,N_CHAN);
            if (sscode()) {
              free(sum_st);free(quad_st);
              *outptr++=pa;*outptr++=wc;*outptr++=sscode();*outptr++=9;
              return(plErr_HW);
             }
            DV(D_DUMP,printf("ADC 1885F(%2d): readout completed (%d words)\n",pa,wc);)
            ped= mod_st;
            j=1;
            do {
              if (read_err= err_data_85f(*ped,pa,(j-1))) { /* error in data word  */
                evt_max-=1;
                { int unsol_mes[6],unsol_num;
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
        if ((pl_cat= lc1810_check_reset(pa_cat,(evt+1))) != plOK) {
          free(sum_st);free(quad_st);
          *outptr++=10;
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
        *outptr++=11;
        return(pl_tri);
       }
     }
   }

  D(D_USER,printf("ADC 1885F: %d trigger for pedestal measurement recorded\n",evt_max);)
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
      thresh= (int)(mean + ((double)p[5] * sigm) + 0.5);
      *thr_ptr++= thresh;  fl_ptr++;
      D(D_USER,printf("ADC 1885F(%d), chan %2d: mean=%8.2f, sigma=%8.3f, thresh=%5d\n",i,(j-1),mean,sigm,thresh);)
      sum++; quad++;
     }
   }
  outptr+= 3*n_mod*N_CHAN;
  D(D_USER,printf("ADC 1885F: pedestal analysis done\n");)

  free(sum_st);  free(quad_st);
 }

*ptr= n_mod;                            /* number of output words             */

return(plOK);

#undef MAX_WT_TRI
#undef MAX_WT_CON
}


/******************************************************************************/

plerrcode test_proc_fb_lc1885f_build_ped(p)
  /* test subroutine for "proc_fb_lc1885f_build_ped" */
int *p;
{
int pat;                                /* IS module pattern                  */
int n_mod;                              /* number of modules                  */
int pa, isml_def;
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

pat= var_list[p[4]].var.val; n_mod= 0; pa= 0;
while (pat) {                            /* check: module defined in IS       */
  if (pat&0x1) {
    i= 1; isml_def= 0;
    do {
      if (pa==memberlist[i]) {
        if (get_mod_type(memberlist[i]) != LC_ADC_1885F) {
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
/******************************************************************************/

int err_data_85f(word,pa,chan)
int word,pa,chan;
{

T(err_data_85f)

if (((word>>27)&0x1f) != pa)    {return(1);}  /* geographic address           */
if (((word>>16)&0x7f) != chan)  {return(2);}  /* channel number               */

return(0);
}

/******************************************************************************/
/******************************************************************************/
