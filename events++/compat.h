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

/*
#if defined(NEED_INET_ADDRDEF)
#include <netinet/in.h>
#include <arpa/inet.h>
unsigned int inet_addr(char*);
#endif
*/

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
