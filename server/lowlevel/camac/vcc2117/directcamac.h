/******************************************************************************
*                                                                             *
* directcamac.h                                                               *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* creatred before 08.03.94                                                    *
* last changed 01.02.95                                                       *
*                                                                             *
******************************************************************************/
#ifndef _directcamac_h_
#define _directcamac_h_

#include <sconf.h>
#include "vcc2117.h"

#define CAMACread(addr) (*(volatile int*)(addr))
#define CAMACwrite(addr,val) *(volatile int*)(addr)=(val)
#define CAMACcntl(addr) *(volatile int*)(addr)=0

typedef volatile int *camadr_t;
#define CAMACinc(a) a++

#ifdef OSK
#define CAMACbase CAMAC_START
#else
extern char *camacbase;
#define CAMACbase camacbase
#endif

#endif
/*****************************************************************************/
/*****************************************************************************/
