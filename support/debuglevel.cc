/*
 * debuglevel.cc
 * 
 * created ??.??.1998 PW
 * 05.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#include "debuglevel.hxx"
#include "versions.hxx"

VERSION("Jun 06 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: debuglevel.cc,v 1.4 2004/11/26 14:45:32 wuestner Exp $")
#define XVERSION

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
const int debuglevel::D_COMM  =(1<<0) ;
const int debuglevel::D_TR    =(1<<1) ;
const int debuglevel::D_DUMP  =(1<<2) ;
const int debuglevel::D_MEM   =(1<<3) ;
const int debuglevel::D_EV    =(1<<4) ;
const int debuglevel::D_PL    =(1<<5) ;
const int debuglevel::D_USER  =(1<<6) ;
const int debuglevel::D_REQ   =(1<<7) ;
const int debuglevel::D_TRIG  =(1<<8) ;
const int debuglevel::D_TIME  =(1<<9) ;
const int debuglevel::D_FORK  =(1<<10);
const int debuglevel::D_LOW   =(1<<11);
int debuglevel::verbose_level=0;
int debuglevel::debug_level=0;
int debuglevel::trace_level=0;
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
