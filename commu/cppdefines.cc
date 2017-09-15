/*
 * cppdefines.cc
 * 
 * created before 02.02.96 PW
 */

#include <debug.hxx>
#include <commu_def.hxx>
#include <config.h>
#include <cppdefines.hxx>
#include "versions.hxx"

VERSION("Feb 02 1996", __FILE__, __DATE__, __TIME__,
"$ZEL: cppdefines.cc,v 2.7 2004/11/26 20:39:44 wuestner Exp $")
#define XVERSION

#define __xSTRING(x) #x
#define __sSTRING(x) __xSTRING(x)

const char* cppdefs[]={
#ifdef __DECCXX
  "__DECCXX",
#endif
#ifdef __GNUC__
  "__GNUC__",
#endif
#ifdef __bsd4_2
  "__bsd4_2",
#endif
#ifdef __POSIX
  "__POSIX",
#endif
#ifdef __SYSTEM_FIVE
  "__SYSTEM_FIVE",
#endif
#ifdef COSYLOG
  "COSYLOG",
#endif
#ifdef COSYDB
  "COSYDB",
#endif
#ifdef ODB
  "ODB",
#endif
#ifdef NSDB
  "NSDB",
#endif
#ifdef DEBUG
  "DEBUG",
#endif
#ifdef TRACE
  "TRACE",
#endif
#ifdef DECNET
  "DECNET",
#endif
  "DEFUSOCKET=" __sSTRING(DEFUSOCKET),
  "DEFISOCKET=" __sSTRING(DEFISOCKET),
  "DEFDSOCKET=" __sSTRING(DEFDSOCKET),
  "DEFCOMMBEZ=" __sSTRING(DEFCOMMBEZ),
//  "DEFLOGFILE=" __sSTRING(DEFLOGFILE),
  "MSGLOWWATER=" __sSTRING(MSGLOWWATER),
  "MSGHIGHWATER=" __sSTRING(MSGHIGHWATER),
  0};

/*****************************************************************************/
/*****************************************************************************/
