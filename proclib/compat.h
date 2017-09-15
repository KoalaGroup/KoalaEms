/******************************************************************************
*                                                                             *
* compat.h                                                                    *
*                                                                             *
* created 29.09.95                                                            *
* last changed 08.10.95                                                       *
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

#if defined(NEED_GETHOSTBYNAMEDEF)
#include <netdb.h>
struct hostent *gethostbyname(const char*);
#endif

#if defined(NEED_GETHOSTNAMEDEF)
int gethostname(char*, int);
#endif

#if defined(NEED_NTOHLDEF)
unsigned int ntohl(unsigned int);
unsigned short ntohs(unsigned short) ;
unsigned int htonl (unsigned int);
unsigned short htons(unsigned short);
#endif

#if defined(NEED_BZERODEF)
void bzero (char*, int);
#endif

#if defined(NEED_SELECTDEF)
#include<sys/types.h>
#include<sys/time.h>
int select(int, fd_set*, fd_set*, fd_set*, struct timeval*);
#endif

#if defined(NEED_SETSOCKOPTDEF)
int setsockopt (int, int, int, char*, int);
#endif

#if defined(NEED_GETSOCKOPTDEF)
int getsockopt (int, int, int, char*, int*);
#endif

#if defined(NEED_SOCKETPAIRDEF)
int socketpair(int, int, int, int[2]);
#endif

#if defined(NEED_INET_ADDRDEF)
unsigned int inet_addr(char*);
#endif

#if defined(NEED_LISTENDEF)
int listen(int, int);
#endif

#if defined(NEED_BINDDEF)
#include <sys/types.h>
#include <sys/socket.h>
int bind(int, struct sockaddr*, int);
#endif

#if defined(NEED_ACCEPTDEF)
#include <sys/types.h>
#include <sys/socket.h>
int accept(int, struct sockaddr*, int*);
#endif

#if defined(NEED_GETRUSAGEDEF)
#include <sys/time.h>
#include <sys/resource.h>
int getrusage(int, struct rusage*);
#endif

#if defined(NEED_GETDTABLESIZEDEF)
int getdtablesize (void);
#endif

#if defined(NEED_INET_NTOADEF)
__END_DECLS
#include <netinet/in.h>
__BEGIN_DECLS
char *inet_ntoa(struct in_addr);
#endif

#if defined(NEED_SYSLOGDEF)
#if defined(__osf__)
int openlog(const char*, int, int);
void closelog(void);
int syslog(int, const char*, ...);
#elif defined(__ultrix)
int openlog(char*, int);
void syslog(int, char*, ... );
void closelog();
#else
#error compat.h: does not know anything about openlog, syslog and closelog
#endif
#endif

#if defined(NEED_GETWDDEF)
char *getwd (char*);
#endif

// #if defined(NEED_BOOL)
// #ifdef false
// #undef false
// #undef true
// #endif
// typedef enum {false, true} bool;
// #endif

__END_DECLS

#endif

/*****************************************************************************/
/*****************************************************************************/
