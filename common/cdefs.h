/******************************************************************************
*                                                                             *
* cdefs.h                                                                     *
*                                                                             *
* OS9 / unix                                                                  *
*                                                                             *
* 22.07.94                                                                    *
*                                                                             *
******************************************************************************/
#ifndef _cdefs_h_
#define _cdefs_h_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(HAVE_SYS_CDEFS_H) && !defined(GO32)
#include <sys/cdefs.h>
#else

#if defined(__cplusplus) || defined(c_plusplus)
#define __BEGIN_DECLS extern "C" {
#define __END_DECLS }
#else
#define __BEGIN_DECLS
#define __END_DECLS
#endif

#if defined(__STDC__) || defined(__cplusplus)
#define __CONCAT(x,y)   x ## y
#define __STRING(x) #x
#else
#define __CONCAT(x,y)   x/**/y
#define __STRING(x) "x"
#endif

#if defined(__STDC__) || defined(__cplusplus) || defined(c_plusplus)
#define __P(protos) protos
#else
#define __P(protos) ()
#endif

#endif /* HAVE_SYS_CDEFS_H */

#endif
/*****************************************************************************/
/*****************************************************************************/
