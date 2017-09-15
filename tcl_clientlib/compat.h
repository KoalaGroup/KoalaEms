/******************************************************************************
*                                                                             *
* compat.h                                                                    *
*                                                                             *
* created 28.07.96                                                            *
* last changed 28.07.96                                                       *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#ifndef _compat_h_
#define _compat_h_

#include "config.h"
#include <cdefs.h>

__BEGIN_DECLS

#if defined(NEED_GETTIMEOFDAYDEF)
#include <sys/time.h>
int gettimeofday(struct timeval*, struct timezone*);
#endif

#if defined(NEED_RINTDEF)
#include <sys/time.h>
double rint(double);
#endif

#if defined(NEED_LOCKFDEF)
#ifdef __ultrix
int lockf(int, int, long);
#else
#ifndef __linux
int lockf(int, int, off_t);
#endif
#endif
#endif

#if defined(NEED_FTRUNCATEDEF)
#ifdef __ultrix
int ftruncate(int, int);
#else
#include <sys/types.h>
int ftruncate(int, off_t);
#endif
#endif

#if defined(NEED_BOOL)
#ifdef false
#undef false
#undef true
#endif
typedef enum {false, true} bool;
#endif

__END_DECLS

#endif

/*****************************************************************************/
/*****************************************************************************/
