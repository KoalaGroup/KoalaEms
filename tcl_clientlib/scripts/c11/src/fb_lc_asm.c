/*******************************************************************************
*                                                                              *
* fb_lc_asm.c                                                                  *
*                                                                              *
* OS9                                                                          *
*                                                                              *
* MaWo                                                                         *
*                                                                              *
* 30.12.94                                                                     *
*                                                                              *
*******************************************************************************/

#include <config.h>
#include <debug.h>
#include <errorcodes.h>
#include "../../procprops.h"
#include "../../../lowlevel/chi_neu/fbacc.h"
#include "fb_lc.h"

extern int *outptr;
extern int wirbrauchen;

/******************************************************************************/
/******************************************************************************/

char name_proc_PATBIT[]="PATBIT";
int ver_proc_PATBIT=1;

/******************************************************************************/

static procprop PATBIT_prop= {0,3,0,0};

procprop* prop_proc_PATBIT()
{
return(&PATBIT_prop);
}

/******************************************************************************/
/*
Extract & Clear lowest set Bit from Word
========================================

parameters:
-----------
  p[0] : 1 (number of parameters)
  p[1] : word

outptr:
-------
  ptr[ 0]:               bit position
*/
/******************************************************************************/

plerrcode proc_PATBIT(p)
int *p;
{
int word;

T(proc_PATBIT)

word= p[1];
*outptr++= p[1];
*outptr++= PATBIT(&word);
*outptr++= word;

return(plOK);
}

/******************************************************************************/

plerrcode test_proc_PATBIT(p)
  /* test subroutine for "PATBIT" */
int *p;
{
T(test_proc_PATBIT)

if (p[0]!=1) {                          /* check number of parameters         */
  *outptr++=p[0];
  return(plErr_ArgNum);
 }

wirbrauchen= 3;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_PATtest[]="PATtest";
int ver_proc_PATtest=1;

/******************************************************************************/

static procprop PATtest_prop= {0,3,0,0};

procprop* prop_proc_PATtest()
{
return(&PATtest_prop);
}

/******************************************************************************/
/*
Extract & Clear lowest set Bit from Word
========================================

parameters:
-----------
  p[0] : 1 (number of parameters)
  p[1] : word

outptr:
-------
  ptr[ 0]:               bit position
*/
/******************************************************************************/

plerrcode proc_PATtest(p)
int *p;
{
int word;

T(proc_PATtest)

word= p[1];
*outptr++= p[1];
*outptr++= PATtest(&word);
*outptr++= word;

return(plOK);
}

/******************************************************************************/

plerrcode test_proc_PATtest(p)
  /* test subroutine for "PATtest" */
int *p;
{
T(test_proc_PATtest)

if (p[0]!=1) {                          /* check number of parameters         */
  *outptr++=p[0];
  return(plErr_ArgNum);
 }

wirbrauchen= 3;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_MODMEM[]="MODMEM";
int ver_proc_MODMEM=1;

/******************************************************************************/

static procprop MODMEM_prop= {0,3,0,0};

procprop* prop_proc_MODMEM()
{
return(&MODMEM_prop);
}

/******************************************************************************/
/*
Set up list of bits set in word at memory address
=================================================

parameters:
-----------
  p[0] : 1 (number of parameters)
  p[1] : word

outptr:
-------
  ptr[ 0]:               bit position
*/
/******************************************************************************/

plerrcode proc_MODMEM(p)
int *p;
{
int word;
int *lptr, *lptr_st;
int n_mod;
int i;

T(proc_MODMEM)

if (!(lptr_st= (int*) calloc(32,sizeof(int)))) {
  *outptr++=0;
  return(plErr_NoMem);
 }
lptr= lptr_st;

word= p[1];
n_mod= MODMEM(word,lptr);
lptr= lptr_st;

*outptr++= word;
for (i=1; i<=n_mod; i++) {
  *outptr++= *lptr++;
 }

free(lptr_st);

return(plOK);
}

/******************************************************************************/

plerrcode test_proc_MODMEM(p)
  /* test subroutine for "MODMEM" */
int *p;
{
T(test_proc_PATtest)

if (p[0]!=1) {                          /* check number of parameters         */
  *outptr++=p[0];
  return(plErr_ArgNum);
 }

wirbrauchen= 33;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/

char name_proc_fb_chipuls[]="fb_chipuls";
int ver_proc_fb_chipuls=1;

/******************************************************************************/

static procprop fb_chipuls_prop= {0,0,0,0};

procprop* prop_proc_fb_chipuls()
{
return(&fb_chipuls_prop);
}

/******************************************************************************/
/*
Generate output pulses from CHI
===============================

parameters:
-----------
  p[0] : 1 (number of parameters)
  p[1] : number of pulses

*/
/******************************************************************************/

plerrcode proc_fb_chipuls(p)
int *p;
{
int i;

T(proc_fb_chipuls)

*outptr++= p[1];
for (i=1; i<=p[1]; i++) {
  FBPULSE();
  tsleep(2);
 }
return(plOK);
}

/******************************************************************************/

plerrcode test_proc_fb_chipuls(p)
  /* test subroutine for "fb_chipuls" */
int *p;
{
T(test_proc_fb_chipuls)

if (p[0]!=1) {                          /* check number of parameters         */
  *outptr++=p[0];
  return(plErr_ArgNum);
 }

wirbrauchen= 1;
return(plOK);
}

/******************************************************************************/
/******************************************************************************/
