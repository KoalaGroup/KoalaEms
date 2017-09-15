/******************************************************************************
*                                                                             *
*  fb_chips_readout.c                                                         *
*                                                                             *
*  Version:  MultiBlock ReadOut   (Do Commands / Programminvocation)          *
*                                                                             *
*            variable Trigger     (CHI Pulse / extern / GSI Triggermodule)    *
*                                                                             *
*            with / without SuperHeader  ( --> Jumper)                        *
*                                                                             *
*  OS9                                                                        *
*                                                                             *
*  95/01/24         Leege  /  FZR                                             *
*                                                                             *
******************************************************************************/

#include <config.h>
#include <debug.h>
#include <math.h>
/*#include <ssm.h>*/
#include <errorcodes.h>
/*#include "../../../lowlevel/chi_neu/chi_map.h"*/
#include "../../../lowlevel/chi_neu/fbacc.h"
/* chi_r_neu existiert nicht {PW} */
/*#include "../../../lowlevel/chi_r_neu/fbacc_ext.h"*/
#include "../../../trigger/chi/gsi/gsitrigger.h"

#ifdef NOVOLATILE
#define VOLATILE
#else
#define VOLATILE volatile
#endif

#define noSUPERHEADER

extern ems_u32* outptr;
extern int *memberlist;
extern ems_u32 eventcnt;

/* debug definition */

#define noFBDEBUG

/* global definitions */

#define PA_MAX     31            /* max. primary address */
#define CSR_0      0             /* standard status-control register */
#define CSR_1      1             /* read - write User ID */
#define CSR_2      2
#define CSR_5      5
#define DSR_1      1             /* read LIFO Setup mode */
#define DSR_2      2             /* MultiBlock ReadOut */
#define SEACQ_M    0x00000044    /* Single Event Acquisition */
#define QDC_rdy    0x00000080    /* data available */
#define RES_LIFO   0x80000000    /* Reset LIFO */
#define RES_CSR_0  0xc0000000    /* Reset CSR#0 + LIFO */
#define PARAM_AREA 0xc0000000    /* Addr. Parameter Area for Pedestals ... */
#define MUCH       0x7c000000    /* mask upper channel */
#define MLCH       0x00007c00    /* mask lower channel */
#define MUVAL      0x03ff0000    /* mask upper value (count) */
#define MLVAL      0x000003ff    /* mask lower value (count) */
#define INVALIDCH  0x00008000    /* testbit invalid channel */
#define MLIFO_WC   0x0000ffff    /* mask LIFO wordcounter */
#define MLW        0x0000ffff    /* mask low word */
#define MHW        0xffff0000    /* mask high word */
#define MHB        0xff000000    /* mask high byte */

#define ERRC       1000          /* Error Repeat Counter */
#define POLLC      100000        /* Polling Counter */

unsigned   csr0      = CSR_0;
unsigned   storecsr0 = CSR_0;
unsigned   csr2      = CSR_2;
unsigned   csr5      = CSR_5;
unsigned   csr_param = PARAM_AREA;
unsigned   errcvar   = ERRC;
unsigned   pollvar   = POLLC;

static VOLATILE int *stat, *ctrl, *fca, *cvt;
static int trignumber;

/*****************************************************************************/


/* Local procedures
   ================
*/

int module_config_test ()
{
  int i, modnum, modtyp, modid;

  T(module_config_test)

  if (memberlist == 0) {*outptr++ = 0xf3000000;
                        printf("FBMB-Error: NoISModulList\n");
                        return(plErr_NoISModulList);}

  modnum = memberlist[0];
  if (!(0 < modnum < 27)) {*outptr++ = 0xf3010000;
                           printf("FBMB-Error: BadISModList_1    modnum:%d\n", modnum);
                           return(plErr_BadISModList);}

  FBCLEAR ();  /* lowlevel: clear chi master port  */

  for (i = 1; i <= modnum; i++)  {
    modtyp = get_mod_type(memberlist[i]);
    if ( ! ((modtyp == 0x415010c2) || (modtyp == 0x435010c6)))
      {
      *outptr++ = 0xf3020000 + i;
      printf("FBMB-Error: BadISModList_2  modnum =%2d  modtyp = 0x%x\n",
             i, modtyp);
      return(plErr_BadISModList);
      }

/*  modid = fb_modul_id(memberlist[i]); */

      csr0 = FRC (memberlist[i], CSR_0);

      errcvar = 0;
      while (sscode() != 0) {
          if (!((sscode() != 0) && (errcvar < ERRC))) {
                              /* ss-code = 1: module busy */
              printf("Error - FRC(modid) ss-code: %x   modnum =%2d\n",
                      sscode(), i);
              *outptr ++ = 0xf4010000 + sscode(); return (plErr_HW);
          }
          csr0 = FRC (memberlist[i], CSR_0);
          errcvar++;
      }

    modid = csr0 >> 16;
    if ( (modtyp & 0x0000ffff) != modid)
      {
      *outptr++ = 0xf3030000 + i;
      printf("\nFBMB-Error: BadISModList_3\n");
      printf("modnum =%2d  modtyp = 0x%8x  modid(csr#0) = 0x%8x\n",
             i, modtyp, csr0);
      return(plErr_BadISModList);
      }
  }

  return (plOK);
}

/******************************************************************************/
/* da chi_r_neu nicht existiert... */
#ifdef HAVE_CHI_R_NEU

/* Simple Test ReadOut
   ===================

 FPSTRO (pa, sa, cycles, polling, csr0, start_sel)

p[0] : No. of parameters (== 6)
p[1] : primary address
p[2] : secondary address  (DSR#1)
p[3] : No. of measuring cycles
p[4] : bitcombination  for polling:  (CSR0 && polling) != 0
p[5] : value of CSR#0 to start measuring cycles
p[6] : value of CSR#0 for cycling activate measure
       == 0: only one output of CSR#0
       != 0: output this CSR#0 for every measuring cycle

outptr[0] : ss-code
outptr[1] : total count of reading values (n * LIFO's)
outptr[2] : value
...

*/


plerrcode proc_FPSTRO(p)
int *p;
{
  int i, sa, cycle, ccount, maxccount, superheader;
  int *ccoutptr;

  T(proc_FPSTRO)

  DV(D_USER, for (i=0; i<4; i++)  printf("\np[%1d]: %d", i, p[i]);)
  DV(D_USER, for (i=4; i<7; i++)  printf("\np[%1d]: %x", i, p[i]);)
  DV(D_USER, printf("\n");)

  outptr[1] = 0;
  cycle     = 1;
  ccoutptr  = outptr + 2;

  /*  first output   CSR#0   */


  FWC (p[1], CSR_0, p[5]);

  errcvar = 0;
  while (sscode() != 0) {
     if (!((sscode() != 0) && (errcvar < ERRC))) {
                        /* ss-code = 1: module busy */
        printf("Error - FWC(init csr#0) ss-code: %x\n", sscode());
        *outptr ++ = 0xf4020100 + sscode(); return (plErr_HW);
     }
     FWC (p[1], CSR_0, p[5]);
     errcvar ++;
  }


  do {

      /*  test:  cycle output of  CSR#0   */

      if (p[6] != 0)  {

         FWC (p[1], CSR_0, p[6]);

         errcvar = 0;
         while (sscode() != 0) {
            if (!((sscode() != 0) && (errcvar < ERRC))) {
                              /* ss-code = 1: module busy */
               printf("Error - FWC(cycle csr#0) ss-code: %x\n", sscode());
               *outptr ++ = 0xf4020200 + sscode(); return (plErr_HW);
            }
         FWC (p[1], CSR_0, p[6]);
         errcvar ++;
         }

       }

      /*  output  pulse   */

      FBPULSE ();

      DV(D_USER, printf("PULSE\n");)

      /*  polling   */

      pollvar = 0;

      do {
           csr0 = FRC (p[1], CSR_0);

           if (sscode() != 0) {
              if (sscode() > 1) {

                              /* err-code = 1: module busy */
                 printf("Error - FRC(polling)  ss-code: %x\n", sscode());
                 *outptr ++ = 0xf4010100 + sscode(); return (plErr_HW);
              }
           }
           if (pollvar++ > POLLC) {
              printf("Polling Error !  return-code: %2d \n", plErr_HW);
              *outptr ++ = 0xf7000000; return (plErr_HW);
           }


       DV(D_USER, printf("polling CSR#0: %x\n", csr0);)

      } while ((csr0 & p[4]) == 0);

       DV(D_USER, printf("end polling CSR#0: %x\n", csr0);)

       /*  read data   */

#ifdef SUPERHEADER
       superheader = FRC (p[1], CSR_5);

       errcvar = 0;
       while (sscode() != 0) {
          if (!((sscode() != 0) && (errcvar < ERRC))) {
                              /* err-code = 1: module busy */
               printf("Error - FRC(superheader) ss-code: %x\n", sscode());
               *outptr ++ = 0xf4010200 + sscode(); return (plErr_HW);
            }
         superheader = FRC (p[1], CSR_5);
         errcvar ++;
       }

       DV(D_USER, printf("superheader: %x\n", superheader);)
#endif

       maxccount = 18;

       ccount = FRDB (p[1], p[2], ccoutptr, maxccount);

       if (sscode() != 0) {
          if (sscode() != 2)  {    /* ss-code = 2: End of Block Read */
             printf("FRDB(data) ss-code: %x\n", sscode());
             *outptr ++ = 0xf5010000 + sscode(); return (plErr_HW);
          }
       }

       DV(D_USER, for (i=0; i<ccount; i++)
          printf("\ncycle:%5d       ccoutptr[%2d]: %x", cycle, i, ccoutptr[i]);)
       DV(D_USER, printf("\n");)

       outptr [1] += ccount;
       ccoutptr   += ccount;

    } while (++ cycle <= p[3]);

    outptr[0] = 0;
    outptr += outptr[1] + 2;

    return (plOK);
  }


plerrcode test_proc_FPSTRO(p)
int *p ;
{

  T(test_proc_FPSTRO)

  if (p[0] != 6) {*outptr++ = 0xf1000000;
                   printf("FBMB-Error: ArgNum\n");
                   return(plErr_ArgNum); }
  if ((p[1] < 0) || (p[1] > PA_MAX)) {*outptr++ = 0xf2000000;
                                       printf("FBMB-Error: ArgRange\n");
                                       return(plErr_ArgRange);}

  return ( module_config_test() );
}

char name_proc_FPSTRO[] = "FPSTRO";
int ver_proc_FPSTRO = 4;


/*****************************************************************************/

/* MultiBlock Test Initialisation  ( --> act. PS-IS )
   ==================================================

 FPSTINI (pa_ml, gate, no_mod)

p[0] : No. of parameters (== 3)
p[1] : primary address MasterLink
p[2] : gate generator/trigger: LW = 0     - CHIpulse / external generator --> CHIext
                                  = 1,2,3 - GSI-Triggermodul (addr. base) --> software trigger
                               HW = 0     - no trigger numbers
                                  > 0     - trigger numbers (GSI trigger)
p[3] : No. of modules

outptr[0] : ss-code

*/


plerrcode proc_FPSTINI(p)
int *p;
{

  int i, initsel, subevwc, maxwc;
  int cvtime, fcatime;
  char* addrbase;
  int *subevoutptr, *locoutptr;

  T(proc_FPSTINI)

  D(D_USER, printf("FPSTINI:  p[0] = %d   p[1] = %d   p[2] = 0x%x   p[3] = %d\n",
                    p[0], p[1], p[2], p[3] );)

  eventcnt = 1;

  /* test: initialise kind of trigger */

  if (p[2] > 0)  {

     /* GSI trigger module */

     trignumber = 0xf;
     cvtime     = 1000;
     fcatime    = 150;
     addrbase   = GSI_BASE + 0x100*(p[2] & MLW);
     stat       = (int*) (addrbase + STATUS);
     ctrl       = (int*) (addrbase + CONTROL);
     cvt        = (int*) (addrbase + CTIME);
     fca        = (int*) (addrbase + FCATIME);

/*     permit (addrbase, 16, ssm_RW);*/

     DV(D_USER, printf("stat=%8x  ctrl=%8x  cvt=%x  fca=%8x\n", stat, ctrl, cvt, fca);)

     *ctrl = MASTER_TRIG;
     DV(D_USER, printf("*ctrl = MASTER_TRIG = 0x%x\n", MASTER_TRIG);)
     *ctrl = BUS_ENABLE;
     DV(D_USER, printf("*ctrl = BUS_ENABLE = 0x%x\n", BUS_ENABLE);)
     *cvt  = 0x10000 - cvtime;
     DV(D_USER, printf("*cvt  = 0x10000 - cvtime = 0x%x\n", 0x10000 - cvtime);)
     *fca  = 0x10000 - fcatime;
     DV(D_USER, printf("*fca  = 0x10000 - fcatime = 0x%x\n", 0x10000 - fcatime);)
     *ctrl = CLEAR;
     DV(D_USER, printf("*ctrl = CLEAR = 0x%x\n", CLEAR);)
     *ctrl = GO;
     DV(D_USER, printf("*ctrl = GO = 0x%x\n", GO);)
  }

  else  {

     /*  init external/internal trigger (CHI-latch-ext)  */

     set_chi_busy();   /* lowlevel: disable triggersignals */

     INIT_LATCH_EXT();   /* lowlevel */
     LATCH_TRIGGER();    /* lowlevel: clear 1. triggersignal */

     /*  test data available  <-- dummy data  */

     initsel = 0;
     for (i = 0; i < p[3]; i++)  {
        csr0 = FRC (p[1] - i, CSR_0);
        errcvar = 0;
        while (sscode() != 0) {
          if (!((sscode() != 0) && (errcvar < ERRC))) {
                                 /* sscode= 1: module busy */
              printf("Error - FRC(inicsr#0) ss-code: %x\n", sscode());
              *outptr ++ = 0xf4010000 + sscode(); return (plErr_HW);
          }
          csr0 = FRC (p[1] - i, CSR_0);
          errcvar ++;
       }
       initsel = initsel | (csr0 & QDC_rdy);
     }
     if (initsel != 0)  {

          /* init FBUS: dummy mb-readout + clear CHI master port */

          D(D_USER, printf("initialise FB: dummy mb-readout ! \n");)
          maxwc = p[3] * 18 + 1;
          locoutptr = subevoutptr;
          if ( p[3] == 1)  {
             subevwc = FRDB (p[1], DSR_1, locoutptr, maxwc); /* ignore all errors */
          }
          else  {
             subevwc = FRDB (p[1], DSR_2, locoutptr, maxwc); /* ignore all errors */
          }

          FBCLEAR ();  /* lowlevel */
     }

  } /* end else */

  *outptr ++ = 0;  /* no errors */
  return (plOK);
}

plerrcode test_proc_FPSTINI(p)
int *p ;
{

  T(test_proc_FPSTINI)

  D(D_USER, printf("test_FPSTINI:  p[0] = %d   p[1] = %d   p[2] = 0x%x   p[3] = %d\n",
                    p[0], p[1], p[2], p[3] );)


  if (p[0] != 3) { *outptr++ = 0xf1000000;
                   printf("FBMB-Error: ArgNum\n");
                   return(plErr_ArgNum);}
  if ((p[1] < 0) || (p[1] > PA_MAX)) {*outptr++ = 0xf2000001;
                                      printf("FBMB-Error: ArgRange p[1]\n");
                                      return(plErr_ArgRange);}
  if ((p[2] < 0) || ((p[2] & MLW) > 3)) {*outptr++ = 0xf2000002;
                                        printf("FBMB-Error: ArgRange p[2]\n");
                                        return(plErr_ArgRange);}
  if ((p[3] < 0) || (p[3] > 25)) {*outptr++ = 0xf2000003;
                                   printf("FBMB-Error: ArgRange p[3]\n");
                                   return(plErr_ArgRange);}

  return( module_config_test() );
}

char name_proc_FPSTINI[] = "FPSTINI";
int ver_proc_FPSTINI = 4;

/**********************************************************************************************/


/* MultiBlock Test Readout (trigger: extern / intern / GSI)  --> Data with / without SuperHeader
   =============================================================================================

 FPSMBRO (pa_ml, gate ,no_mod, no_subev)

p[0] : No. of parameters (== 4)
p[1] : primary address MasterLink
p[2] : gate generator/trigger: LW = 0     - CHIpulse / external generator --> CHIext
                                  = 1,2,3 - GSI-Triggermodul (addr. base) --> software trigger
                               HW = 0     - no trigger number
                                  > 0     - trigger numbers (GSI trigger)
p[3] : No. of modules
p[4] : No. of subevents

outptr[0] : ss-code
outptr[1] : total count of reading values
outptr[2] : begin subeventblock 1
...
outptr[n] : end of subeventblocks

*/


plerrcode proc_FPSMBRO(p)
int *p;
{
  int wc, subevwc, maxwc, i , notrigger, trigbits, trignum, modnum, initsel, status;
  int *subevoutptr, *locoutptr;

  T(proc_FPSMBRO)

  D(D_USER, printf("FPSMBRO: p[0]=%d  p[1]=%d  p[2]=0x%x  p[3]=%d  p[4]=%d\n",
                  p[0], p[1], p[2], p[3], p[4]);)

  wc = 0;
  subevoutptr = outptr + 2;


  /* FrontEnd Datacycles  =  p[4] : No. of subevents */

  do {

       /* test: gate / trigger */

       if (p[2] == 0)  {

          /*  Reset CHI-Busy  <--  Testmodus: Erfassen der gesamten Totzeit
                                            im Messzyklus                  */

          reset_chi_busy();   /* lowlevel  */

          /*  test extern trigger signal  */

          notrigger = 0;
          while ( LATCH_TRIGGER() == 0 ) {
              notrigger++;
              if ( notrigger % 10000 == 0 )
                   printf("No external trigger signal: %d \n", notrigger);
              if ( notrigger > 1000000) {
                  printf("\nError:  No external trigger signal !\n");
                  *outptr ++ =0xf6000000 ; return (plErr_HW);
              }
          }

          /*  Set CHI-Busy   <--   Testmodus   */

          set_chi_busy();   /* lowlevel */

       } /* endif */


       else  {

          /* GSI Trigger module: test EON + trigger numbers (MT4...MT1) */

          notrigger = 0;
               do  { status = *stat;
                     notrigger++;
                     if ( notrigger % 10000 == 0 )
                          printf("No gsi trigger signal: %d \n", notrigger);
                     if ( notrigger > 1000000) {
                         printf("\nError:  No gsi trigger signal !\n");
                         *outptr ++ =0xf6000000;
                         return (plErr_HW);
                     }
               } while ((status & EON) == 0);
               trigbits = p[2] >> 16;
               if (trigbits != 0) {
                  for (trignum = 1; trignum < 16; trignum++)  {
                     if ((trigbits & 0x00000001) != 0)
                        if (trignum == (status & 0x0000000f))  goto dataread;
                     trigbits = trigbits >> 1;
                  }
                  printf("\nError:  Bad gsi trigger signal !   trigger number: %d\n",
                          status & 0x0000000f );
                  *outptr ++ =0xf6000001 ;
                  return (plErr_HW);
               }
       }

       /* output data block: MultiBlock ReadOut */

       dataread:
       maxwc = p[3] * 18 + 1;
       locoutptr = subevoutptr + 1;

         /* test: special version - 1 module ? */

         if ( p[3] == 1)  {
           subevwc = FRDB (p[1], DSR_1, locoutptr, maxwc);
           if (sscode() != 0 ) {
              if (sscode()!= 2)
                {printf("Error - FRDB1(data)  ss-code: %x\n", sscode());
                 printf("rest cyc: %d   subevwc: %d   wc: %d\n", p[4] , subevwc, wc);
                 for (i=0; i<=(wc+subevwc+3); i++)
                    printf("outptr [%d]: %8x\n", i, outptr [i] );
                 printf("end of buffer (+3)\n");
                 *outptr ++ = 0xf5010000 + sscode(); return (plErr_HW); }
           }
         D(D_EV, printf("fbmb_readout1: event=%d  datawords=%d  dat[0]=0x%8x  dat[1]=0x%8x\n",
                         eventcnt, subevwc, locoutptr[0], locoutptr[1]);)

           /* test: data structure ok ? */
           if (!(((locoutptr[0] & MHB) == 0xc2000000) || ((locoutptr[0] & MHB) == 0xc6000000)) )
                   printf(" - Error SH/MH  mod:%d -  \n", p[1]);
           D(D_EV, if ((locoutptr[0] & 0x0000fffe) == 0) printf(" - Null Event  mod:%d -  \n", p[1]);)

         }

         else  {
            subevwc = FRDB (p[1], DSR_2, locoutptr, maxwc);
            if (sscode() != 0 ) {
              if (sscode()!= 2)
                {printf("Error - FRDB2(data) ss-code: %x\n", sscode());
                 printf("rest cyc: %d   subevwc: %d   wc: %d\n", p[4] , subevwc, wc);
                 for (i=0; i<=(wc+subevwc+3); i++)
                    printf("outptr [%d]: %8x\n", i, outptr [i] );
                 printf("end of buffer (+3)\n");
                 *outptr ++ = 0xf5020000 | sscode(); return (plErr_HW); }
            }
         D(D_EV, printf("fbmb_readout2: event=%d  datawords=%d  dat[0]=0x%8x  dat[1]=0x%8x\n",
                         eventcnt, subevwc, locoutptr[0], locoutptr[1]);)

           /* test: data structure ok ? */
           for (i = 0; i < subevwc-2; i++)  {
              if ((locoutptr[i] & MHW) == (locoutptr[i+1] & MHW))  {
                 if (!(((locoutptr[i] & MHB) == 0xc2000000) || ((locoutptr[i] & MHB) == 0xc6000000)) )
                     printf(" - Error SuperHeader  slot:%d -  \n", (locoutptr[i] >> 16) & 0x1f);
                 i++;
              }
              if (!(((locoutptr[i] & MHB) == 0xc2000000) || ((locoutptr[i] & MHB) == 0xc6000000)) )
                  printf(" - Error ModuleHeader  slot:%d -  \n", (locoutptr[i] >> 16) & 0x1f);

              D(D_EV, if ((locoutptr[i] & 0x0000fffe) == 0)
                         printf(" - Null Event  slot:%d - \n", (locoutptr[i] >> 16) & 0x1f);)
              i += (((locoutptr[i] & MLW) >> 10) +1) >> 1;
           }

         }

       DV(D_EV, for (i=2; i<subevwc; i++) printf("subevoutptr [%2d]=0x%8x\n", i, locoutptr[i]);)

       if (subevoutptr [subevwc-2] == 0x1fffff) subevwc -= 1;
       subevoutptr [0] = subevwc - 1;
       subevoutptr += subevwc ;

       wc += subevwc ;

       /*  reset trigger: test kind of trigger  */


       if (p[2] == 0)  {

          /* external / internal trigger --> chi ext */
             clear_latch();
       }

       else  {

          /* gsi trigger */
             *stat = EV_IRQ_CLEAR;
             *stat = DT_CLEAR;

       }

     eventcnt++;

     D(D_TRIG, if(eventcnt % 1000 == 0) printf("event:%d\n",eventcnt);)

     } while (-- p[4] > 0);

   outptr [0] = 0;             /* no errors */
   outptr [1] = wc;

   DV(D_USER, for (i = 0; i <= outptr [1]+1; i ++)
       printf("outptr [%d]: 0x%8x\n", i, outptr [i] );)


   outptr += outptr [1] + 2;
   return (plOK);

}

plerrcode test_proc_FPSMBRO(p)
int *p ;
{

  T(test_proc_FPSMBRO)

  D(D_USER, printf("test_FPSMBRO: p[0]=%d  p[1]=%d  p[2]=0x%x  p[3]=%d  p[4]=%d\n",
                  p[0], p[1], p[2], p[3], p[4]);)

  if (p[0] != 4) { *outptr++ = 0xf1000000;
                   printf("FBMB-Error: ArgNum\n");
                   return(plErr_ArgNum);}
  if ((p[1] < 0) || (p[1] > PA_MAX)) {*outptr++ = 0xf2000001;
                                      printf("FBMB-Error: ArgRange p[1]\n");
                                      return(plErr_ArgRange);}
  if ((p[2] < 0) || ((p[2] & MLW) > 3)) {*outptr++ = 0xf2000002;
                                        printf("FBMB-Error: ArgRange p[2]\n");
                                        return(plErr_ArgRange);}
  if ((p[3] < 0) || (p[3] > 25)) {*outptr++ = 0xf2000003;
                                   printf("FBMB-Error: ArgRange p[3]\n");
                                   return(plErr_ArgRange);}

  return(plOK);
}

char name_proc_FPSMBRO[] = "FPSMBRO";
int ver_proc_FPSMBRO = 17;

#endif /* HAVE_CHI_R_NEU */

/************************************************************************************************/


/* MultiBlock Readout (Programminvocation - without polling)  -->  Data with / without SuperHeader
   ===============================================================================================

 FBMB_Readout (pa_ml,no_mod)

p[0] : No. of parameters (== 2)
p[1] : primary address MasterLink
p[2] : No. of modules

outptr[0] : begin subeventblock
...
outptr[n] : end of subeventblock

*/


plerrcode proc_FBMB_Readout(p)
int *p;
{
  int subevwc, maxwc, i ;
  int *subevoutptr;

  T(proc_FBMB_Readout)

  D(D_USER, printf("FBMB_Readout activ: p[0]=%d   p[1]=%d   p[2]=%d \n", p[0], p[1], p[2]); )

    subevoutptr = outptr;

  /* output data block: MultiBlock ReadOut */

  maxwc = p[2] * 18 + 1;
  subevwc = 0;
  /* test: special version - 1 module ? */

  if ( p[2] == 1)  {
      subevwc = FRDB (p[1], DSR_1, subevoutptr, maxwc);
      if (sscode() != 0 ) {
         if (sscode()!= 2)  /* SS = 2: End of Block ReadOut */
            {printf("Error - FRDB1(data) ss-code: %x    subevwc:%d\n", sscode(), subevwc);
             *outptr ++ = 0xf5010000 + sscode(); return (plErr_HW); }
         }
       DV(D_USER, printf("fbmb_readout1: %d datawords\n", subevwc-1);)
      }

   else  {
       subevwc = FRDB (p[1], DSR_2, subevoutptr, maxwc);
       if (sscode() != 0 ) {
          if (sscode()!= 2)  /* SS = 2: End of MultiBlock ReadOut */
             {printf("Error - FRDB2(data) ss-code: %x    subevwc:%d\n", sscode(), subevwc);
              *outptr ++ = 0xf5020000 + sscode(); return (plErr_HW); }
          }
       DV(D_USER, printf("fbmb_readout2: %d datawords\n", subevwc-1);)
       }


   if (outptr[subevwc-1] == 0x001fffff) subevwc -= 1;

   DV(D_USER, for (i = 0; i < subevwc; i ++)
              printf("outptr [%d]: 0x%8x\n", i, outptr [i] );)

   outptr += subevwc;
   return (plOK);

}

plerrcode test_proc_FBMB_Readout(p)
int *p ;
{

  T(test_proc_FBMB_Readout)

  D(D_USER, printf("test_FBMB_Readout: p[0]=%d   p[1]=%d   p[2]=%d \n",
                    p[0], p[1], p[2]); )

  if (p[0] != 2) { *outptr ++ = 0xf1000000;
                   printf("FBMB-Error: ArgNum \n");
                   return(plErr_ArgNum);}
  if ((p[1] < 0) || (p[1] > PA_MAX)) {*outptr ++ = 0xf2000000;
                                      printf("FBMB-Error: ArgRange\n");
                                      return(plErr_ArgRange);}

  return( module_config_test() );
}

char name_proc_FBMB_Readout[] = "FBMB_Readout";
int ver_proc_FBMB_Readout = 9;


/*****************************************************************************/


/*  Build Pedestals (Data with / without SuperHeader)
    =================================================

 FBMB_BuildPed (pa, base,evt_max, fact)

p[0] : No. of parameters (== 4)
p[1] : primary address
p[2] : gate generator/trigger: 0     - CHIpulse / external generator --> CHIext
                               1,2,3 - GSI-Triggermodul (addr. base) --> software trigger
p[3] : number of events for evaluation pedestals
p[4] : multiplicity factor (integer * 1000):    ped = - (mean + p[4] * sigma)

outptr[0] :  ss-code   (integer)
outptr[1] :  Mean value channel 1 (integer * 1000)
outptr[2] :  Variance channel 1   (integer * 1000)
outptr[3] :  Pedestal channel 1   (integer)
  ...
outptr[94] : Mean value channel 32 (integer * 1000)
outptr[95] : Variance channel 32   (integer * 1000)
outptr[96] : Pedestal channel 32   (integer)

*/

plerrcode proc_FBMB_BuildPed(p)
int *p;
{
  int wc, wc_smh, maxwc, ch_no, evt_val, evt_max, i, j;
  int cvtime, fcatime, status;
  char* addrbase;
  int *outptr_evt;
  int evt_sum [32], evt_sqr_sum [32], ped [32];
  float mean_values [32];
  float variance [32];
  double sigma, help;

  T(proc_FBMB_BuildPed)

  D(D_USER, printf("FBMB Build Pedestals: p[0]=%d  p[1]=%d  p[2]=%d  p[3]=%d  p[4]=%d\n",
                  p[0], p[1], p[2], p[3], p[4]);)

  /* store CSR#0 */

  storecsr0 = FRC (p[1], CSR_0);

  errcvar = 0;
  while (sscode() != 0) {
     if (!((sscode() != 0) && (errcvar < ERRC))) {
                          /* sscode= 1: module busy */
          printf("Error - FWC(store csr#0) ss-code: %x\n", sscode());
          *outptr ++ = 0xf4020100 + sscode(); return (plErr_HW);
     }
       storecsr0 = FRC (p[1], CSR_0);
       errcvar ++;
   }

 /* clear + set CSR#0 ( data acquisitions mode) */

  FWC (p[1], CSR_0, RES_CSR_0);

  errcvar = 0;
  while (sscode() != 0) {
     if (!((sscode() != 0) && (errcvar < ERRC))) {
                          /* sscode= 1: module busy */
          printf("Error - FWC(res csr#0) ss-code: %x\n", sscode());
          *outptr ++ = 0xf4020200 + sscode(); return (plErr_HW);
     }
     FWC (p[1], CSR_0, RES_CSR_0);
     errcvar ++;
  }

  FWC (p[1], CSR_0, SEACQ_M);

  errcvar = 0;
  while (sscode() != 0) {
     if (!((sscode() != 0) && (errcvar < ERRC))) {
                          /* err-code = 1: module busy */
          printf("Error - FWC(acqm csr#0) ss-code: %x\n", sscode());
          *outptr ++ = 0xf4020300 + sscode(); return (plErr_HW);
     }
     FWC (p[1], CSR_0, SEACQ_M);
     errcvar ++;
  }


  /* measurement cycle of offset events for channels */

    /* initialise arrays and global values */

       for (i = 0; i < 32; i ++)  {
          evt_sum [i] = 0;
          evt_sqr_sum [i] = 0;
       }

       maxwc = 19;
       evt_max = p[3];

       /*
       if (outptr >= (int*)OUT_BUFBEG && outptr <= (int*)OUT_BUFEND)
         {outptr_ped = outptr;}
       if (outptr >= (int*)EVENT_BUFBEG && outptr <= (int*)EVENT_BUFEND)
         {outptr_ped = (int*)OUT_BUFBEG;}
       */

     /* test: initialise trigger */


     if (p[2] > 0)  {
        plerrcode res;
        if ((res=trigmod_soft_init(p[2], 15, 1000, 150))!=plOK)
          {
          *outptr++=0xf8000000;
          return(res);
          }
/*
        trignumber = 0xf;
        cvtime     = 1000;
        fcatime    = 150;
        addrbase   = GSI_BASE + 0x100*(p[2] & MLW);
        stat       = (int*) (addrbase + STATUS);
        ctrl       = (int*) (addrbase + CONTROL);
        cvt        = (int*) (addrbase + CTIME);
        fca        = (int*) (addrbase + FCATIME);

        permit (addrbase, 16, ssm_RW);

        D(D_USER, printf("stat=%8x  ctrl=%8x  cvt=%x  fca=%8x\n", stat, ctrl, cvt, fca);)

        *ctrl = MASTER_TRIG;
        D(D_USER, printf("*ctrl = MASTER_TRIG = 0x%x\n", MASTER_TRIG);)
        *ctrl = BUS_ENABLE;
        D(D_USER, printf("*ctrl = BUS_ENABLE = 0x%x\n", BUS_ENABLE);)
        *cvt  = 0x10000 - cvtime;
        D(D_USER, printf("*cvt  = 0x10000 - cvtime = 0x%x\n", 0x10000 - cvtime);)
        *fca  = 0x10000 - fcatime;
        D(D_USER, printf("*fca  = 0x10000 - fcatime = 0x%x\n", 0x10000 - fcatime);)
        *ctrl = CLEAR;
        D(D_USER, printf("*ctrl = CLEAR = 0x%x\n", CLEAR);)
        *ctrl = GO;
        D(D_USER, printf("*ctrl = GO = 0x%x\n", GO);)
*/
      }

  for (i = 0; i < evt_max; i ++)  {

     /* test: gate / trigger */

     if (p[2] == 0)  {
#ifdef HAVE_CHI_R_NEU

        /* reset busy -> enable chi_ext_trigger  /  output  pulse   */

        reset_chi_busy();  /* lowlevel */
        FBPULSE ();

        D(D_USER, if (i%100 == 0)  printf("ext/internal trigger: %4d\n", i);)

        /*  polling data available  (ext/internal trigger) */

        pollvar = 0;
        do {
            csr0 = FRC (p[1], CSR_0);
            if (sscode() != 0 ) {
               if (sscode() == 0)
                   {printf("Error - FRC(polling) ss-code: %x\n", sscode());
                    *outptr ++ = 0xf5010100 + sscode();
                    return (plErr_HW);
               }
            }
            if (pollvar++ > POLLC)  {
               printf("Polling Error: ext/internal trigger\n");
               *outptr ++ = 0xf7000000;
                return (plErr_HW);
            }
         } while ((csr0 & QDC_rdy) == 0);
#endif /* HAVE_CHI_R_NEU */
     }

     else  {

        /* GSI software trigger --> set trigger number in stat_reg */
        int res;
        res=trigmod_soft_get();
        D(D_USER, printf("trigmod_soft_get()=%d; i=%d\n", res, i);)
        D(D_USER, if (i%100 == 0)  printf("software trigger: %4d\n", i);)

/*        *stat = trignumber;
        tsleep (2);          --> 2*0.1 s = 20 ms ?


        pollvar = 0;
        do {

             status = *stat;
             if (pollvar++ > POLLC)  {
                printf("Software Trigger Error !\n");
                *outptr ++ = 0xf7000000;
                return (plErr_HW);
             }
        } while ((status & EON) == 0);
*/
     }

     outptr_evt = outptr;

     /* test: reset trigger */

      if (p[2] == 0)  {
#ifdef HAVE_CHI_R_NEU
          set_chi_busy();  /* lowlevel */
#endif /* HAVE_CHI_R_NEU */
         }

      else  {
            trigmod_soft_reset();
/*
              *stat = EV_IRQ_CLEAR;
              *stat = DT_CLEAR;
*/
            }


      /* Block ReadOut channels   */

      wc = FRDB (p[1], DSR_1, outptr_evt, maxwc);

      if (sscode() != 0 ) {
          if (sscode()!= 2)  /* SS = 2: End of Block ReadOut */
             {printf("Error - FRDB1(data) ss-code: %x\n", sscode());
              *outptr ++ = 0xf5010000 + sscode();
               return (plErr_HW); }
      }

      /* test: with/without SuperHeader ? */
      if ((outptr_evt[0] & MHW) == (outptr_evt[1] & MHW))  outptr_evt++;
      /* test: ModuleHeader ok ? */
      if ( ! (((outptr_evt[0] & MHB) == 0xc2000000) || ((outptr_evt[0] & MHB) == 0xc6000000)))
         { printf("Error - FRDB1(ModuleHeader): %8x\n", outptr_evt[0]);
          *outptr ++ = 0xf5020000;
          return (plErr_HW);
       }

      wc_smh = (((outptr_evt[0] & MLW) >> 10) + 1) >> 1;

      DV(D_USER, printf("i:%4d  /  wc_fkt:%4d  /  wc_smh:%4d  /  ss-code:%2d\n",
                         i, wc, wc_smh, sscode());)

      for (j = 1; j <= wc_smh; j ++)  {
         ch_no = (outptr_evt[j] & MUCH) >> 26;
         evt_val = ((outptr_evt[j] & MUVAL) >> 16);
         evt_sum [ch_no] += evt_val ;
         evt_sqr_sum [ch_no] += evt_val * evt_val;
         DV(D_USER, printf("j:%2d  ch_no:%2d  count:%4d  evt_sum:%10d  evt_sqr_sum:%10d\n",
                            j, ch_no, evt_val, evt_sum [ch_no], evt_sqr_sum [ch_no]);)
         if ( (outptr_evt[j] & INVALIDCH) == 0)  {
             ch_no = (outptr_evt[j] & MLCH) >> 10;
             evt_val = (outptr_evt[j] & MLVAL);
             evt_sum [ch_no] += evt_val;
             evt_sqr_sum [ch_no] += evt_val * evt_val;
             DV(D_USER, printf("j:%2d  ch_no:%2d  count:%4d  evt_sum:%10d  evt_sqr_sum:%10d\n",
                                j, ch_no, evt_val, evt_sum [ch_no], evt_sqr_sum [ch_no]);)
         }
      }
  } /* end for i < event_max */
  if (p[2]>0) trigmod_soft_done();

  /* build mean values, variances, standard deviations (sigma) and pedestals of channels */

  for (i = 0; i < 32; i ++)  {

     mean_values [i] = (float)(evt_sum [i] / evt_max);

     variance [i] = (double)(evt_sqr_sum[i] / evt_max)
                     - (double)mean_values[i] * (double)mean_values[i];

     if (variance [i] < 0)  variance [i] = 0;
     sigma = sqrt (variance [i]);

     help = mean_values [i] + p[4] * sigma / 1000.0;
     ped [i] = 0 - (int)(help + 0.5);

     D(D_USER, printf("chan:%2d   mean:%10.3f   variance:%10.3f   sigma:%8.3f   ped: %4d\n",
                       i, mean_values [i], variance [i], sigma, ped [i] );)
  }

  /* clear + restore CSR#0 */

  FWC (p[1], CSR_0, RES_CSR_0);

  errcvar = 0;
  while (sscode() != 0) {
     if (!((sscode() != 0) && (errcvar < ERRC))) {
                          /* sscode= 1: module busy */
          printf("Error - FWC(res csr#0) ss-code: %x\n", sscode());
          *outptr ++ = 0xf4020100 + sscode();
          return (plErr_HW);
     }
     FWC (p[1], CSR_0, RES_CSR_0);
     errcvar ++;
  }

  FWC (p[1], CSR_0, storecsr0 & MLW);

  errcvar = 0;
  while (sscode() != 0) {
     if (!((sscode() != 0) && (errcvar < ERRC))) {
                          /* sscode= 1: module busy */
          printf("Error - FWC(restore csr#0) ss-code: %x\n", sscode());
          *outptr ++ = 0xf4020200 + sscode(); return (plErr_HW);
     }
     FWC (p[1], CSR_0, storecsr0 & MLW);
     errcvar ++;
  }


  /* output mean values and variance: --> integer format (value * 1000 --> higher precision)
                                                 ( float or double format not allowed ) */
  outptr[0] = 0;  /* no errors */

  for (i=1; i<=32; i++)  {
      outptr[3*i-2] = (int)((mean_values [i-1] + 0.0005) * 1000);
      outptr[3*i-1] = (int)((variance [i-1] + 0.0005) * 1000);
      outptr[3*i]   =  ped [i-1];
  }
  D(D_USER, for (i=0; i<=96; i++) printf("outptr[%2d] = %d\n", i, outptr[i]);)

  outptr += 97;
  return (plOK);

}

plerrcode test_proc_FBMB_BuildPed(p)
int *p;
{

  T(test_proc_FBMB_BuildPed)

  D(D_USER, printf("test_FBMB Build Pedestals: p[0]=%d  p[1]=%d  p[2]=%d  p[3]=%d  p[4]=%d\n",
                  p[0], p[1], p[2], p[3], p[4]);)

  if (p[0] != 4)  {*outptr ++ = 0xf1000000;
                   printf("FBMB-Error: ArgNum \n");
                   return(plErr_ArgNum); }
  if ((p[1]<0) || (p[1]>PA_MAX))  {*outptr ++ = 0xf2000001;
                                   printf("FBMB-Error: ArgRange p[1] \n");
                                   return(plErr_ArgRange); }
#ifdef HAVE_CHI_R_NEU
  if ((p[2]<0) || (p[2]>3))       {*outptr ++ = 0xf2000002;
                                   printf("FBMB-Error: ArgRange p[2] \n");
                                   return(plErr_ArgRange); }
#else /* HAVE_CHI_R_NEU */
  /* CHIpulse nicht verfuegbar; deswegen p[2]==0 nicht erlaubt */
  if ((p[2]<1) || (p[2]>3))       {*outptr ++ = 0xf2000002;
                                   printf("FBMB-Error: ArgRange p[2] \n");
                                   return(plErr_ArgRange); }
#endif /* HAVE_CHI_R_NEU */
  if ((p[3]<1) || (p[3]>1000000)) {*outptr ++ = 0xf2000003;
                                   printf("FBMB-Error: ArgRange p[3] \n");
                                   return(plErr_ArgRange); }
  if ((p[4]<0) || (p[4]>10000))      {*outptr ++ = 0xf2000004;
                                   printf("FBMB-Error: ArgRange p[4] \n");
                                   return(plErr_ArgRange); }

  return( module_config_test() );
}

char name_proc_FBMB_BuildPed[] = "FBMB_BuildPed";
int ver_proc_FBMB_BuildPed = 16;


/*****************************************************************************/

/* Read Setup
   ==========

 FBMB_ReadSetup (pa)

p[0] :  No. of parameters (== 1)
p[1] :  primary address

outptr[0] :  ss-code
outptr[1] :  ID (Module/User Identifier)
outptr[2] :  value of CSR#0
outptr[3] :  Pedestal channel 1
outptr[4] :  Lower Threshold channel 1
outptr[5] :  Upper Threshold channel 1
  ...
outptr[96] : Pedestal channel 32 ...
outptr[97] : Lower Threshold channel 32
outptr[98] : Upper Threshold channel 32

*/


plerrcode proc_FBMB_ReadSetup(p)
int *p;
{
  int i;

  T(proc_FBMB_ReadSetup)

  D(D_USER, printf("FBMB Read Setup: p[0]=%d   p[1]=%d\n", p[0], p[1]);)

  /* read ID - Register  */

  outptr[1] = FRC (p[1], CSR_1);

  errcvar = 0;
  while (sscode() != 0) {
     /*printf("sscode():%x \n", sscode() );*/
     if (!((sscode() != 0) && (errcvar < ERRC))) {
                          /* sscode= 1: module busy */
          printf("Error - FRC(id csr#1) ss-code: %x\n", sscode());
          *outptr ++ = 0xf4020100 + sscode();
          return (plErr_HW);
     }
     /*printf("ss-code:%x   errcvar:%d\n", sscode(), errcvar);*/
     outptr[1] = FRC (p[1], CSR_1);
     errcvar ++;
  }

  /* read + store CSR#0 */

  outptr[2] = FRC (p[1], CSR_0);

  errcvar = 0;
  while (sscode() != 0) {
  /*printf("sscode():%x \n", sscode() );*/

     if (!((sscode() != 0) && (errcvar < ERRC))) {
                          /* sscode= 1: module busy */
          printf("Error - FRC(store csr#0) ss-code: %x\n", sscode());
          *outptr ++ = 0xf4020200 + sscode(); return (plErr_HW);
     }
     /*printf("ss-code:%x   errcvar:%d\n", sscode(), errcvar);*/
     outptr[2] = FRC (p[1], CSR_0);
     errcvar ++;
  }

  /* clear CSR#0 - set Setup Mode */

  FWC (p[1], CSR_0, RES_CSR_0);

  errcvar = 0;
  while (sscode() != 0) {
  /*printf("sscode():%x \n", sscode() );*/
     if (!((sscode() != 0) && (errcvar < ERRC))) {
                          /* sscode= 1: module busy */
          printf("Error - FWC(res csr#0) ss-code: %x\n", sscode());
          *outptr ++ = 0xf4020300 + sscode();
          return (plErr_HW);
     }
     /*printf("ss-code:%x   errcvar:%d\n", sscode(), errcvar);*/
     FWC (p[1], CSR_0, RES_CSR_0);
     errcvar ++;
  }

  /* read pedestals - lower threshold - upper threshold */

  csr_param = PARAM_AREA;
  for (i=0; i<32; i++)  {

     outptr[3 + 3*i] = FRC (p[1], csr_param);         /* Pedestal */
     errcvar = 0;
     while (sscode() != 0) {
        if (!((sscode() != 0) && (errcvar < ERRC))) {
                          /* sscode= 1: module busy */
           printf("Error - FRC(pedestal) ss-code: %x\n", sscode());
           *outptr ++ = 0xf4010100 + sscode();
           return (plErr_HW);
        }
        outptr[3 + 3*i] = FRC (p[1], csr_param);
        errcvar ++;
     }
     csr_param++;

     outptr[4 + 3*i] = FRC (p[1], csr_param);         /* Lower Threshold.*/
     errcvar = 0;
     while (sscode() != 0) {
        if (!((sscode() != 0) && (errcvar < ERRC))) {
                          /* sscode= 1: module busy */
           printf("Error - FRC(l.thresh) ss-code: %x\n", sscode());
           *outptr ++ = 0xf4010200 + sscode();
           return (plErr_HW);
        }
        outptr[3 + 3*i] = FRC (p[1], csr_param);
        errcvar ++;
     }
     csr_param++;

     outptr[5 + 3*i] = FRC (p[1], csr_param);         /* Upper Threshold */
     errcvar = 0;
     while (sscode() != 0) {
        if (!((sscode() != 0) && (errcvar < ERRC))) {
                          /* sscode= 1: module busy */
           printf("Error - FRC(u.thresh) ss-code: %x\n", sscode());
           *outptr ++ = 0xf4010300 + sscode();
           return (plErr_HW);
        }
        outptr[3 + 3*i] = FRC (p[1], csr_param);
        errcvar ++;
     }
     csr_param++;

   }

  /* reset CSR#0 */

      FWC (p[1], CSR_0, outptr[2] & MLW);

      errcvar = 0;
      while (sscode() != 0) {
         if (!((sscode() != 0) && (errcvar < ERRC))) {
                          /* sscode= 1: module busy */
            printf("Error - FWC(set csr#0) ss-code: %x\n", sscode());
            *outptr ++ = 0xf4020400 + sscode(); return (plErr_HW);
         }
         FWC (p[1], CSR_0, outptr[2] & MLW);
         errcvar ++;
      }
  outptr[0] = 0;  /* no errors */

  DV(D_USER, for (i=0; i<99; i++) printf("outptr[%2d] = %d\n", i, outptr[i]);)

  outptr += 99;

  return (plOK);

}

plerrcode test_proc_FBMB_ReadSetup(p)
int *p;
{

  T(test_proc_FBMB_ReadSetup)

  D(D_USER, printf("test_FBMB Read Setup: p[0]=%d   p[1]=%d\n", p[0], p[1]);)

  if (p[0] != 1)  {*outptr ++ = 0xf1000000;
                   printf("FBMB-Error: ArgNum \n");
                   return(plErr_ArgNum); }
  if ((p[1]<0) || (p[1]>PA_MAX))  {*outptr ++ = 0xf2000000;
                                   printf("FBMB-Error: ArgRange \n");
                                   return(plErr_ArgRange); }


  return( module_config_test() );
}

char name_proc_FBMB_ReadSetup[] = "FBMB_ReadSetup";
int ver_proc_FBMB_ReadSetup = 6;


/*****************************************************************************/

/* Load Setup
   ==========

 FBMB_LoadSetup (pa, id, csr0, pd0 .. pd31, lthr0 .. lthr31, uthr0 .. uthr31)

p[0] :  No. of parameters (== 99)
p[1] :  primary address
p[2] :  ID (Module/User Identifier)
p[3] :  value of CSR#0
p[4] :  Pedestal channel 1
p[5] :  Lower Threshold channel 1
p[6] :  Upper Threshold channel 1
 ...
p[97] : Pedestal channel 32
p[98] : Lower Threshold channel 32
p[99] : Upper Threshold channel 32

outptr[0] : ss-code

*/

plerrcode proc_FBMB_LoadSetup(p)
int *p;
{
  int i;

  T(proc_FBMB_LoadSetup)

  D(D_USER, printf("FBMB Load Setup: p[0]=%d   p[1]=%d   p[2]=0x%x   p[3]=0x%x\n",
                   p[0], p[1], p[2], p[3]);)
  DV(D_USER, for (i=4; i<=99; i++)  {
                if (i % 3 == 0)   printf("   p[%2d] =%4d\n", i, p[i]);
                else  printf("   p[%2d] =%4d", i, p[i]);
             } )

  /* set Setup Mode */

      FWC (p[1], CSR_0, RES_CSR_0);

      errcvar = 0;
      while (sscode() != 0) {
         if (!((sscode() != 0) && (errcvar < ERRC))) {
                          /* ss-code= 1: module busy */
            printf("Error - FWC(res csr#0) ss-code: %x\n", sscode());
            *outptr ++ = 0xf4020100 + sscode();
            return (plErr_HW);
         }
         FWC (p[1], CSR_0, RES_CSR_0);
         errcvar ++;
      }

  /* write Pedestals, Thresholds */

  csr_param = PARAM_AREA;
  for (i=0; i<32; i++)  {

         FWC (p[1], csr_param, p[4 + 3*i]);      /* Pedestals */
         errcvar = 0;
         while (sscode() != 0) {
            if (!((sscode() != 0) && (errcvar < ERRC))) {
                          /* ss-code= 1: module busy */
               printf("Error - FWC(pedestal) ss-code: %x\n", sscode());
               *outptr ++ = 0xf4020200 + sscode();
               return (plErr_HW);
            }
            FWC (p[1], csr_param, p[4 + 3*i]);
            errcvar ++;
         }
         csr_param++;

         FWC (p[1], csr_param, p[5 + 3*i]);      /* Lower Thresholds */
         errcvar = 0;
         while (sscode() != 0) {
            if (!((sscode() != 0) && (errcvar < ERRC))) {
                          /* ss-code= 1: module busy */
               printf("Error - FWC(l.thresh) ss-code: %x\n", sscode());
               *outptr ++ = 0xf4020300 + sscode();
               return (plErr_HW);
            }
            FWC (p[1], csr_param, p[5 + 3*i]);
            errcvar ++;
         }
         csr_param++;

         FWC (p[1], csr_param, p[6 + 3*i]);      /* Upper Thresholds */
         errcvar = 0;
         while (sscode() != 0) {
            if (!((sscode() != 0) && (errcvar < ERRC))) {
                          /* ss-code= 1: module busy */
               printf("Error - FWC(u.thresh) ss-code: %x\n", sscode());
               *outptr ++ = 0xf4020400 + sscode();
               return (plErr_HW);
            }
            FWC (p[1], csr_param, p[6 + 3*i]);
            errcvar ++;
         }
         csr_param++;

    }

     /* load User-ID */

          FWC (p[1], CSR_1, p[2]);
          errcvar = 0;
          while (sscode() != 0) {
             if (!((sscode() != 0) && (errcvar < ERRC))) {
                          /* ss-code = 1: module busy */
                 printf("Error - FWC(user-id) ss-code: %x\n", sscode());
                 *outptr ++ = 0xf4020500 + sscode();
                 return (plErr_HW);
             }
             FWC (p[1], CSR_1, p[2]);
             errcvar ++;
          }


     /* load CSR#0 */

          FWC (p[1], CSR_0, p[3]);
          errcvar = 0;
          while (sscode() != 0) {
             if (!((sscode() != 0) && (errcvar < ERRC))) {
                          /* ss-code= 1: module busy */
                 printf("Error - FWC(load csr#0) ss-code: %x\n", sscode());
                 *outptr ++ = 0xf4020600 +sscode();
                 return (plErr_HW);
             }
             FWC (p[1], CSR_1, p[3]);
             errcvar ++;
          }


     *outptr ++ = 0;  /* no errors */

     return (plOK);

}

plerrcode test_proc_FBMB_LoadSetup(p)
int *p;
{ int i;

  T(test_proc_FBMB_LoadSetup)

  D(D_USER, printf("test_FBMB Load Setup: p[0]=%d   p[1]=%d\n", p[0], p[1]);)

  if (p[0] != 99)  {*outptr ++ = 0xf1000000;
                    printf("FBMB-Error: ArgNum \n");
                    return(plErr_ArgNum); }
  if ((p[1]<0) || (p[1]>PA_MAX))  {*outptr ++ = 0xf2000000;
                                   printf("FBMB-Error: ArgRange \n");
                                   return(plErr_ArgRange); }

  return( module_config_test() );
}

char name_proc_FBMB_LoadSetup[] = "FBMB_LoadSetup";
int ver_proc_FBMB_LoadSetup = 6;


/*****************************************************************************/
/*****************************************************************************/
