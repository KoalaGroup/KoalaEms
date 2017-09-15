/* conf/config.h.  Generated automatically by configure.  */
/* conf/config.h.in.  Generated automatically from configure.in by autoheader.  */

#ifndef _config_h_
#define _config_h_

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
/* #undef WORDS_BIGENDIAN */

/* Gibt es <sys/cdefs.h>? */
#define HAVE_SYS_CDEFS_H 1

/* Define if you have sys/statfs.h */
#define HAVE_SYS_STATFS_H 1

/* Ist "char *strdup()" nicht deklariert? */
#define NEED_STRDUP 1

/* Ist "char *index()" nicht deklariert? */
#define NEED_INDEX 1

/* Versteht der Compiler "volatile" nicht? */
/* #undef volatile */

/* Sind Konstruktionen wie "(char)a++" nicht zulaessig? */
#define NO_LHS_CASTS 1

/* Define if you have the gettimeofday function.  */
#define HAVE_GETTIMEOFDAY 1

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <sys/ioctl.h> header file.  */
#define HAVE_SYS_IOCTL_H 1

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the netinet library (-lnetinet).  */
/* #undef HAVE_LIBNETINET */

/* Define if you have the cgetcap function.  */
/* #undef HAVE_CGETCAP */

/* Define if you have the MD4Init function.  */
/* #undef HAVE_MD4INIT */

/* Define if you have the MD5Init function.  */
/* #undef HAVE_MD5INIT */

#ifndef __unix__
#if defined(__NetBSD__)
#define __unix__
#endif
#endif

#endif
