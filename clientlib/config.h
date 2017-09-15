/* config.h.  Generated automatically by configure.  */
/* config.h.in.  Generated automatically from configure.in by autoheader.  */

#ifndef _config_h_
#define _config_h_

#define HAVE_CONFIG_H 1

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Gibt es <sys/cdefs.h>? */
#define HAVE_SYS_CDEFS_H 1

/* Gibt es <sys/select.h>? */
#define HAVE_SYS_SELECT_H 1

/* Define if you have the gettimeofday function.  */
#define HAVE_GETTIMEOFDAY 1

/* Define if you have the select function.  */
#define HAVE_SELECT 1

/* Define if you have the socket function.  */
#define HAVE_SOCKET 1

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the bzero function. */
#define HAVE_BZERO 1

/* Define if you have the memset function. */
#define HAVE_MEMSET 1

#if !defined(__cplusplus) && !defined(c_plusplus)
/* Define to empty if the keyword does not work.  */
/* #undef const */
#endif /* cplusplus */

/* Define if you dont have a valid declaration for gettimeofday() */
/* #undef NEED_GETTIMEOFDAYDEF */

/* Define if you dont have a valid declaration for gethostbyname() */
/* #undef NEED_GETHOSTBYNAMEDEF */

/* Define if you dont have a valid declaration for gethostname() */
/* #undef NEED_GETHOSTNAMEDEF */

/* Define if you dont have a valid declaration for ntohl() and htons() */
/* #undef NEED_NTOHLDEF */

/* Define if you dont have a valid declaration for bzero() */
/* #undef NEED_BZERODEF */

/* Define if you dont have a valid declaration for memset() */
/* #undef NEED_MEMSETDEF */

/* Define if you dont have a valid declaration for select() */
/* #undef NEED_SELECTDEF */

/* Define if you dont have a valid declaration for setsockopt() */
/* #undef NEED_SETSOCKOPTDEF */

/* Define if you dont have a valid declaration for in_addr_t */
/* #undef NEED_IN_ADDR_TDEF */

/* Define if you dont have a valid declaration for inet_addr() */
/* #undef NEED_INET_ADDRDEF */

/* sys_errlist not defined in stdio.h? */
/* #undef NEED_SYS_ERRLIST */

#endif /* _config_h_ */
