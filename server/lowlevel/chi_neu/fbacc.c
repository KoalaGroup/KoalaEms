/******************************************************************************
*                                                                             *
* file fbacc.c                                                                *
*                                                                             *
* FastBus Access                                                              *
*                                                                             *
* PeWue                                                                       *
*                                                                             *
* 05.05.93                                                                    *
*                                                                             *
******************************************************************************/

#include <ptypes.h>
#include <stdio.h>
#include "chi_map.h"
#include <ssm.h>

long
  last_trans,
  err_proc,
  err_seq;

byte
  err_code,
  debuglevel,
  protokollevel;

long
  err_cont,
  P_BASE_A,
  error_adr,
  *tabpntr;

word
  tabsize;

int
  path;

#asm

FB_read equ 0
FB_write equ 1
FB_cont equ 0
FB_data equ 1
FB_int  equ 0
FB_poll equ 1

errtab:

        dc.l    FRC01,b_FRC,1,1
        dc.l    FRC02,b_FRC,1,2
        dc.l    FRC03,b_FRC,1,3
        dc.l    FRC04,b_FRC,1,4
        dc.l    FRC05,b_FRC,1,5

        dc.l    FRD01,b_FRD,2,1
        dc.l    FRD02,b_FRD,2,2
        dc.l    FRD03,b_FRD,2,3
        dc.l    FRD04,b_FRD,2,4
        dc.l    FRD05,b_FRD,2,5

        dc.l    FWC01,b_FWC,3,1
        dc.l    FWC02,b_FWC,3,2
        dc.l    FWC03,b_FWC,3,3
        dc.l    FWC04,b_FWC,3,4
        dc.l    FWC05,b_FWC,3,5

        dc.l    FWD01,b_FWD,4,1
        dc.l    FWD02,b_FWD,4,2
        dc.l    FWD03,b_FWD,4,3
        dc.l    FWD04,b_FWD,4,4
        dc.l    FWD05,b_FWD,4,5

        dc.l    FRCB_I01,b_FRCB_I,5,1
        dc.l    FRCB_I02,b_FRCB_I,5,2
        dc.l    FRCB_I03,b_FRCB_I,5,3
        dc.l    FRCB_I04,b_FRCB_I,5,4
        dc.l    FRCB_I05,b_FRCB_I,5,5
        dc.l    FRCB_I06,b_FRCB_I,5,6

        dc.l    FRDB_I01,b_FRDB_I,6,1
        dc.l    FRDB_I02,b_FRDB_I,6,2
        dc.l    FRDB_I03,b_FRDB_I,6,3
        dc.l    FRDB_I04,b_FRDB_I,6,4
        dc.l    FRDB_I05,b_FRDB_I,6,5
        dc.l    FRDB_I06,b_FRDB_I,6,6

        dc.l    FWCB_I01,b_FWCB_I,7,1
        dc.l    FWCB_I02,b_FWCB_I,7,2
        dc.l    FWCB_I03,b_FWCB_I,7,3
        dc.l    FWCB_I04,b_FWCB_I,7,4
        dc.l    FWCB_I05,b_FWCB_I,7,5
        dc.l    FWCB_I06,b_FWCB_I,7,6

        dc.l    FWDB_I01,b_FWDB_I,8,1
        dc.l    FWDB_I02,b_FWDB_I,8,2
        dc.l    FWDB_I03,b_FWDB_I,8,3
        dc.l    FWDB_I04,b_FWDB_I,8,4
        dc.l    FWDB_I05,b_FWDB_I,8,5
        dc.l    FWDB_I06,b_FWDB_I,8,6

        dc.l    FRCB_P01,b_FRCB_P,9,1
        dc.l    FRCB_P02,b_FRCB_P,9,2
        dc.l    FRCB_P03,b_FRCB_P,9,3
        dc.l    FRCB_P04,b_FRCB_P,9,4
        dc.l    FRCB_P05,b_FRCB_P,9,5
        dc.l    FRCB_P06,b_FRCB_P,9,6

        dc.l    FRDB_P01,b_FRDB_P,10,1
        dc.l    FRDB_P02,b_FRDB_P,10,2
        dc.l    FRDB_P03,b_FRDB_P,10,3
        dc.l    FRDB_P04,b_FRDB_P,10,4
        dc.l    FRDB_P05,b_FRDB_P,10,5
        dc.l    FRDB_P06,b_FRDB_P,10,6

        dc.l    FWCB_P01,b_FWCB_P,11,1
        dc.l    FWCB_P02,b_FWCB_P,11,2
        dc.l    FWCB_P03,b_FWCB_P,11,3
        dc.l    FWCB_P04,b_FWCB_P,11,4
        dc.l    FWCB_P05,b_FWCB_P,11,5
        dc.l    FWCB_P06,b_FWCB_P,11,6

        dc.l    FWDB_P01,b_FWDB_P,12,1
        dc.l    FWDB_P02,b_FWDB_P,12,2
        dc.l    FWDB_P03,b_FWDB_P,12,3
        dc.l    FWDB_P04,b_FWDB_P,12,4
        dc.l    FWDB_P05,b_FWDB_P,12,5
        dc.l    FWDB_P06,b_FWDB_P,12,6

        dc.l    FRCM01,b_FRCM,13,1
        dc.l    FRCM02,b_FRCM,13,2
        dc.l    FRCM03,b_FRCM,13,3
        dc.l    FRCM04,b_FRCM,13,4

        dc.l    FRDM01,b_FRDM,14,1
        dc.l    FRDM02,b_FRDM,14,2
        dc.l    FRDM03,b_FRDM,14,3
        dc.l    FRDM04,b_FRDM,14,4

        dc.l    FWCM01,b_FWCM,15,1
        dc.l    FWCM02,b_FWCM,15,2
        dc.l    FWCM03,b_FWCM,15,3
        dc.l    FWCM04,b_FWCM,15,4

        dc.l    FWDM01,b_FWDM,16,1
        dc.l    FWDM02,b_FWDM,16,2
        dc.l    FWDM03,b_FWDM,16,3
        dc.l    FWDM04,b_FWDM,16,4


        dc.l    FRCB_P_01,b_FRCB_P_,17,1
        dc.l    FRCB_P_02,b_FRCB_P_,17,2
        dc.l    FRCB_P_03,b_FRCB_P_,17,3
        dc.l    FRCB_P_04,b_FRCB_P_,17,4
        dc.l    FRCB_P_05,b_FRCB_P_,17,5
        dc.l    FRCB_P_06,b_FRCB_P_,17,6

        dc.l    FRDB_P_01,b_FRDB_P_,18,1
        dc.l    FRDB_P_02,b_FRDB_P_,18,2
        dc.l    FRDB_P_03,b_FRDB_P_,18,3
        dc.l    FRDB_P_04,b_FRDB_P_,18,4
        dc.l    FRDB_P_05,b_FRDB_P_,18,5
        dc.l    FRDB_P_06,b_FRDB_P_,18,6

        dc.l    FWCB_P_01,b_FWCB_P_,19,1
        dc.l    FWCB_P_02,b_FWCB_P_,19,2
        dc.l    FWCB_P_03,b_FWCB_P_,19,3
        dc.l    FWCB_P_04,b_FWCB_P_,19,4
        dc.l    FWCB_P_05,b_FWCB_P_,19,5
        dc.l    FWCB_P_06,b_FWCB_P_,19,6

        dc.l    FWDB_P_01,b_FWDB_P_,20,1
        dc.l    FWDB_P_02,b_FWDB_P_,20,2
        dc.l    FWDB_P_03,b_FWDB_P_,20,3
        dc.l    FWDB_P_04,b_FWDB_P_,20,4
        dc.l    FWDB_P_05,b_FWDB_P_,20,5
        dc.l    FWDB_P_06,b_FWDB_P_,20,6

        dc.l    FRD_F01,b_FRD_F,21,1
        dc.l    FRD_F02,b_FRD_F,21,2
        dc.l    FRD_F03,b_FRD_F,21,3
        dc.l    FRD_F04,b_FRD_F,21,4
        dc.l    FRD_F05,b_FRD_F,21,5

errsize:dc.w    errsize-errtab

#endasm

char
  *procnames[]={"unknown procedure",
        "FRC","FRD","FWC","FWD",
        "FRCB_I","FRDB_I","FWCB_I","FWDB_I",
        "FRCB_P","FRDB_P","FWCB_P","FWDB_P",
        "FRCM","FRDM","FWCM","FWDM",
        "FRCB_P_","FRDB_P_","FWCB_P_","FWDB_P_",
        "FRD_F"
        };

/*****************************************************************************/

void printerror(dest)
FILE *dest;
{
fprintf(dest,"ERROR: %s seq: %d",procnames[err_proc],err_seq);
if
  (err_code & 0x40) fprintf(dest," interrupt during BT");
else if
  (err_code & 0x08) fprintf(dest," primary address");
else
  fprintf(dest," data time")
;
fprintf(dest," SS: code: %d\n",err_code & 7);
}/* printerror */

/*****************************************************************************/

void report_1()
{
switch (debuglevel)
  {
  case  0: printf("Fehler in berr.\n");break;
  case  1: printerror(stdout); break;
  default: printf("unexpected debuglevel %d.\n",debuglevel);
  }
switch (protokollevel)
  {
  case  0: break;
  default: printf("protokoll not jet implemented.\n");
  }
}/* report_1 */

/*****************************************************************************/

void report_2()
{
long
  i;
printf("\n\nBusError exception in unknown procedure.\n");
printf("Fehleradresse : 0x%x\n",error_adr);
/*printf("Tabellenlaenge: %d\n",tabsize);
for (i=0;i<tabsize;i++)
  {
  printf("0x%x, %s\n",*((long*)tabpntr+4*i),
      procnames[*((long*)tabpntr+4*i+2)]);
  }*/
exit(1);
}/* report_2 */

/*****************************************************************************/

/* BusError handler */
#asm
berr:   tst.l   (err_cont,a6)
        beq     notfound

        move.l  #F_BASE,a2
        move.b  SSREG(a2),d0
        tst.b   d0
        bne     notime
        moveq   #8,d0
notime: move.b  d0,(err_code,a6)
        tst.b   (debuglevel,a6)
        bne     search
        move.l  #$ffffffff,(err_proc,a6)
        movem.l (a5),d0-d7/a0-a7
        jmp     ([(err_cont).l,a6])

search: moveq.l #0,d0
        lea     (errtab,a6),a2
        move.w  (errsize,a6),d2
bloop:  cmpa.l  (a2,d0.w),a0
        beq     found
        add.w   #16,d0
        cmp.w   d0,d2
        bne     bloop

        bra     notfound

found:  move.l  8(a2,d0.w),(err_proc,a6)
        move.l  12(a2,d0.w),(err_seq,a6)
        movem.l d0/a0-a5,-(a7)
        bsr     report_1
        movem.l (a7)+,d0/a0-a5
cont:   subq.l  #4,a1
        move.l  4(a2,d0.w),(a1)
        move.l  a1,-(a7)
        movem.l (a5),d0-d7/a0-a6
        move.l  (a7)+,a7
        rts

notfound:
        move.l  a0,(error_adr,a6)
        move.l  a2,(tabpntr,a6)
        lsr.w   #4,d2
        move.w  d2,(tabsize,a6)
        movem.l d0/a0-a5,-(a7)
        bsr     report_2
        movem.l (a7)+,d0/a0-a5
        movem.l (a5),d0-d7/a0-a7
        rts
#endasm

/*****************************************************************************/

/*
void FBCLEAR()
*/

#asm
FBCLEAR:
        movem.l d2-d7/a0-a4,-(a7)
        clr.l   (err_proc,a6)
        move.l  #F_BASE,a0
        tst.b   FCLEAR(a0)
        movem.l (a7)+,d2-d7/a0-a4
        rts
#endasm /* FBCLEAR */

/*****************************************************************************/

/*
void FBPULSE()
*/

#asm
FBPULSE:
        movem.l d2-d7/a0-a4,-(a7)
        clr.l   (err_proc,a6)
        move.l  #F_BASE,a0
        tst.b   OUT(a0)
        movem.l (a7)+,d2-d7/a0-a4
        rts
#endasm /* FBPULSE */

/*****************************************************************************/

#asm

FB_S    macro
\1:     movem.l d2-d7/a0-a4,-(a7)
        clr.l   (err_proc,a6)
        lea     b_\1(pc),a0
        move.l  a0,(err_cont,a6)
\101:   move.l  (P_BASE_A,a6),a0
 ifeq \2
\102:   move.l  d0,CGEO(a0)              pa
 else
\102:   move.l  d0,DGEO(a0)              pa
 endc
\103:   move.l  d1,SECAD(a0)             sa
 ifeq \3
\104:   move.l  RNDM(a0),d0              value
 else
\104:   move.l  48(a7),RNDM(a0)           value
 endc
b_\1:
\105:   move.l  #F_BASE,a1
        move.l  FSR(a1),d1
        move.l  d1,(last_trans,a6)
        tst.b   DISCON(a0)
        clr.l   (err_cont,a6)
        movem.l (a7)+,d2-d7/a0-a4
        rts
        endm


*long FRC(pa,sa)
*long
*  pa,sa;

        FB_S    FRC,FB_cont,FB_read


*long FRD(pa,sa)
*long
*  pa,sa;

        FB_S    FRD,FB_data,FB_read


*long FWC(pa,sa,val)
*long
*  pa,sa,val;

        FB_S    FWC,FB_cont,FB_write


*long FWD(pa,sa,val)
*long
*  pa,sa,val;

        FB_S    FWD,FB_data,FB_write

*long FRD_F(long pa, long sa)
FRD_F:     movem.l d2-d7/a0-a4,-(a7)
           move.l  #F_BASE,a1
           move.w  #$ff00,SSENABLE(a1)
           clr.l   d3
           move.b  d3,SSREG(a1)
           clr.l   (err_proc,a6)
           lea     b_FRD_F(pc),a0
           move.l  a0,(err_cont,a6)
FRD_F01:   move.l  (P_BASE_A,a6),a0
FRD_F02:   move.l  d0,DGEO(a0)              pa
FRD_F03:   move.l  d1,SECAD(a0)             sa
FRD_F04:   move.l  RNDM(a0),d0              value
b_FRD_F:
FRD_F05:   move.l  #F_BASE,a1
           move.l  FSR(a1),d1
           move.l  d1,(last_trans,a6)
           tst.b   DISCON(a0)
           clr.l   (err_cont,a6)
           clr.l   d1
           move.b  SSREG(a1),d1
           move.b  d1,(err_code,a6)
           move.w  #$ffff,SSENABLE(a1)
           movem.l (a7)+,d2-d7/a0-a4
           rts

#endasm

/*****************************************************************************/

#asm

DMA_MODE equ $01010101

FB_B    macro

\1:     movem.l d2-d7/a0-a4,-(a7)
        clr.l   (err_proc,a6)
        lea     b_\1(pc),a0
        move.l  a0,(err_cont,a6)
        move.l  (P_BASE_A,a6),a0
        move.l  #F_BASE,a1
        move.l  #REG_BASE,a2

*        move.w  #$ffff,SSENABLE(a1)
*        move.l  #DMA_MODE,WRCR(a1)
        move.l  52(a7),d2                count
 ifne \3                                FB_write
        addq.l  #1,d2
 endc
        move.l  d2,LDWC(a1)
        move.l  48(a7),d2                buffer
        sub.l   #M_BASE,d2
        lsr.l   #2,d2
*
        move.l  d2,LDAC(a1)
*        tst.b   OUT(a1)                 trigger for scope
 ifeq \2                                control- or data-space
\101:   move.l  d0,CGEO(a0)             pa
 else
\101:   move.l  d0,DGEO(a0)             pa
 endc
\102:   move.l  d1,SECAD(a0)            sa
 ifeq \4                                interrupt or polling
        clr.l   d0
        moveq.l #1,d1
        os9     F$SigMask
        tst.w   CLRIN(a1)
        tst.w   ENIN(a1)
 endc
 ifeq \3                                read or write
\103:   tst.b   BHR(a0)
 else
\103:   tst.b   BHW(a0)
 endc
 ifeq \4                                interrupt or polling
\104:   clr.l   d0
        os9     F$Sleep
        tst.w   DISIN(a1)
 else
p_\1:
\104:   btst.b  #3,POLLREG0(a2)
        beq     ok1_\1
*        tst.b   OUT(a1)                 trigger for scope
        bra     p_\1
ok1_\1:
 endc
*        tst.b   OUT(a1)                 trigger for scope
        move.b  SSREG(a1),d2
        andi.b  #7,d2
        beq     ok2_\1
        move.b  d2,(err_code,a6)
        move.l  #\5,(err_proc,a6)
        clr.l   (err_seq,a6)
ok2_\1:
b_\1:
\105:   tst.b   DISCON(a0)
\106:   tst.b   d2
        beq     r_\1
        tst.l   (debuglevel,a6)
        beq     r_\1
        bsr     report_1
r_\1:   move.l  RDWC(a1),d0
        andi.l  #$ffffff,d0
 ifne \3                                FB_write
        subi.l  #1,d0
 endc
        clr.l   (err_cont,a6)
        movem.l (a7)+,d2-d7/a0-a4
        rts
        endm

FB_B_S  macro

\1:     movem.l d2-d7/a0-a4,-(a7)
        clr.l   (err_proc,a6)
        lea     b_\1(pc),a0
        move.l  a0,(err_cont,a6)
        move.l  (P_BASE_A,a6),a0
        move.l  #F_BASE,a1
        move.l  #REG_BASE,a2

*        move.w  #$ffff,SSENABLE(a1)
*        move.l  #DMA_MODE,WRCR(a1)
        move.l  48(a7),d2                count
 ifne \3                                FB_write
        addq.l  #1,d2
 endc
        move.l  d2,LDWC(a1)
        move.l  d1,d2                buffer
        sub.l   #M_BASE,d2
        lsr.l   #2,d2
*
        move.l  d2,LDAC(a1)
*        tst.b   OUT(a1)                 trigger for scope
 ifeq \2                                control- or data-space
\101:   move.l  d0,CGEO(a0)             pa
 else
\101:   move.l  d0,DGEO(a0)             pa
 endc
\102:
*move.l  d1,SECAD(a0)            sa
 ifeq \4                                interrupt or polling
        clr.l   d0
        moveq.l #1,d1
        os9     F$SigMask
        tst.w   CLRIN(a1)
        tst.w   ENIN(a1)
 endc
 ifeq \3                                read or write
\103:   tst.b   BHR(a0)
 else
\103:   tst.b   BHW(a0)
 endc
 ifeq \4                                interrupt or polling
\104:   clr.l   d0
        os9     F$Sleep
        tst.w   DISIN(a1)
 else
p_\1:
\104:   btst.b  #3,POLLREG0(a2)
        beq     ok1_\1
*        tst.b   OUT(a1)                 trigger for scope
        bra     p_\1
ok1_\1:
 endc
*        tst.b   OUT(a1)                 trigger for scope
        move.b  SSREG(a1),d2
        andi.b  #7,d2
        beq     ok2_\1
        move.b  d2,(err_code,a6)
        move.l  #\5,(err_proc,a6)
        clr.l   (err_seq,a6)
ok2_\1:
b_\1:
\105:   tst.b   DISCON(a0)
\106:   tst.b   d2
        beq     r_\1
        tst.l   (debuglevel,a6)
        beq     r_\1
        bsr     report_1
r_\1:   move.l  RDWC(a1),d0
        andi.l  #$ffffff,d0
 ifne \3                                FB_write
        subi.l  #1,d0
 endc
        clr.l   (err_cont,a6)
        movem.l (a7)+,d2-d7/a0-a4
        rts
        endm


*long FRCB_I(pa,sa,dest,count)
*long
*  pa,sa,*dest,count;

        FB_B    FRCB_I,FB_cont,FB_read,FB_int,5


*long FRDB_I(pa,sa,dest,count)
*long
*  pa,sa,*dest,count;

        FB_B    FRDB_I,FB_data,FB_read,FB_int,6

*long FWCB_I(pa,sa,source,count)
*long
*  pa,sa,*source,count;

        FB_B    FWCB_I,FB_cont,FB_write,FB_int,7


*long FWDB_I(pa,sa,source,count)
*long
*  pa,sa,*source,count;

        FB_B    FWDB_I,FB_data,FB_write,FB_int,8


*long FRCB_P(pa,sa,dest,count)
*long
*  pa,sa,*dest,count;

        FB_B    FRCB_P,FB_cont,FB_read,FB_poll,9


*long FRDB_P(pa,sa,dest,count)
*long
*  pa,sa,*dest,count;

        FB_B    FRDB_P,FB_data,FB_read,FB_poll,10

*long FWCB_P(pa,sa,source,count)
*long
*  pa,sa,*source,count;

        FB_B    FWCB_P,FB_cont,FB_write,FB_poll,11


*long FWDB_P(pa,sa,source,count)
*long
*  pa,sa,*source,count;

        FB_B    FWDB_P,FB_data,FB_write,FB_poll,12

        FB_B_S    FRCB_P_,FB_cont,FB_read,FB_poll,17
        FB_B_S    FRDB_P_,FB_data,FB_read,FB_poll,18
        FB_B_S    FWCB_P_,FB_cont,FB_write,FB_poll,19
        FB_B_S    FWDB_P_,FB_data,FB_write,FB_poll,20

#endasm

/*****************************************************************************/
#asm

FB_BC   macro
\1:     movem.l d2-d7/a0-a4,-(a7)
        clr.l   (err_proc,a6)
        lea     b_\1(pc),a0
        move.l  a0,(err_cont,a6)
\101:   move.l  (P_BASE_A,a6),a0
 ifeq \2
\102:   move.l  d0,CBREG(a0)             broadcast-address, control space
 else
\102:   move.l  d0,DBREG(a0)             broadcast-address, data space
 endc
 ifeq \3
\103:   move.l  RNDM(a0),d0              read value
 else
\103:   move.l  d1,RNDM(a0)              write value
 endc
b_\1:
\104:   tst.b   DISCON(a0)
        clr.l   (err_cont,a6)
        movem.l (a7)+,d2-d7/a0-a4
        rts
        endm

*long FRCM(long ba)

        FB_BC FRCM,FB_cont,FB_read

*long FRDM(long ba)

        FB_BC FRDM,FB_data,FB_read

*long FWCM(long ba, long val)

        FB_BC FWCM,FB_cont,FB_write

*long FWDM(long ba, long val)

        FB_BC FWDM,FB_data,FB_write

#endasm

/*****************************************************************************/

long get_ga_chi()
{
unsigned char
  garegval;

garegval=*(char*)(F_BASE+GAREG);
garegval&=0x1f;
return(garegval);
}/* get_ga_chi */

/*****************************************************************************/

/*boolean berr_init()
{*/
#asm
berr_init:
        movem.l d2-d7/a0-a4,-(a7)
        suba.l  a0,a0
        sub.l   d0,d0
        lea     traptab(pc),a1
        os9     F$STrap
        bcs     notrap
        moveq.l #1,d0
        movem.l (a7)+,d2-d7/a0-a4
        rts
notrap: move.l  d1,errno(a6)
        clr.l   d0
        movem.l (a7)+,d2-d7/a0-a4
        rts

traptab:dc.w    T_BusErr,berr-*-4
        dc.w    -1

#endasm
/* berr_init */

/*****************************************************************************/

boolean berr_done()
{
return(TRUE);
}

/*****************************************************************************/

void fb_masks_init()
{
/*byte
  imask,
  *imaskreg;*/
*(byte*)(IMASKREG+REG_BASE)|=IM6;
/*imask=*imaskreg;*/
/*imask=imask | IM6;*/
/**imaskreg=imask;*/

*(word*)(SSENABLE+F_BASE)=0xffff;
*(long*)(WRCR+F_BASE)=0x01010101;
}

/*****************************************************************************/

void fb_masks_done()
{
}

/*****************************************************************************/

void setdebuglevel(level)
byte
  level;
{
debuglevel=level;
}/* setdebuglevel */

/*****************************************************************************/

byte getdebuglevel()
{
return(debuglevel);
}/* getdebuglevel */

/*****************************************************************************/

void setprotokollevel(level)
byte
  level;
{
protokollevel=level;
}/* setprotokollevel */

/*****************************************************************************/

byte getprotokollevel()
{
return(protokollevel);
}/* getprotokollevel */

/*****************************************************************************/

boolean fbacc_init(arbitrationlevel)
long
  arbitrationlevel;
{
if ((arbitrationlevel<0) || (arbitrationlevel>31))
  {
  printf("arbitrationlevel %d not allowed.\n",arbitrationlevel);
  return(FALSE);
  }
P_BASE_A=P_BASE+0x40*arbitrationlevel;
if (!berr_init())
  {
  printf("Cannot install BusError exception handler.\n");
  return(FALSE);
  }
if (!permit_FB())
  {
  printf("Cannot get permission to access FastBus key addresses.\n");
  return(FALSE);
  }
fb_masks_init();
/*if ((path=open("/fbIdsc",1))==-1)
  {
  printf("Cannot open \"/fbIdsc\"; error: %d.\n",errno);
  return(TRUE);
  }
if (_ss_ssig(path,0x77)==-1)
  {
  printf("Cannot set up signal to be sent on interrupt; error: %d.\n",errno);
  return(FALSE);
  }*/
setdebuglevel(0);
setprotokollevel(0);
return(TRUE);
}

/*****************************************************************************/

void fbacc_done()
{
berr_done();
/*close("/fbIdsc");*/
fb_masks_done();
protect_FB();
}

/*****************************************************************************/
/*****************************************************************************/

