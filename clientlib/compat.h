/*
 * clientlib/compat.h
 * $ZEL: compat.h,v 2.8 2004/02/11 12:24:15 wuestner Exp $
 * 
 * created 29.09.95 PW
 * last changed 02.10.95 PW
 */

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
#define gethostbyname _disabled_gethostbyname
#include <netdb.h>
#undef gethostbyname
struct hostent *gethostbyname(const char*);
#endif

#if defined(NEED_GETHOSTNAMEDEF)
int gethostname(char*, int);
#endif

#if defined(__cplusplus) || defined(c_plusplus)
#if defined(NEED_NTOHLDEF)
#define ntohl _disabled_ntohl
#define htons _disabled_htons
#include <netinet/in.h>
#undef ntohl
#undef htons
unsigned int ntohl(unsigned int);
unsigned short htons(unsigned short);
#endif
#endif
#ifdef HAVE_BZERO
#ifdef NEED_BZERODEF
void bzero (char*, int);
#endif /* NEED_BZERODEF */
#else  /* HAVE_BZERO */
#define bzero(x,y) memset(x, 0, y)
#ifdef NEED_MEMSETDEF
void *memset(void *s, int c, size_t n);
#endif
#endif /* HAVE_BZERO */

#if defined(NEED_SELECTDEF)
#include<sys/types.h>
#include<sys/time.h>
int select(int, fd_set*, fd_set*, fd_set*, struct timeval*);
#endif

#if defined(NEED_SETSOCKOPTDEF)
int socket(int, int, int);
int connect(int, struct sockaddr *, int);
int setsockopt (int, int, int, char*, int);
#endif

#if defined(NEED_IN_ADDR_TDEF)
typedef unsigned int in_addr_t;
#endif

/*
#if defined(NEED_INET_ADDRDEF)
#include <netinet/in.h>
#include <arpa/inet.h>
unsigned int inet_addr(char*);
#endif
*/

__END_DECLS

#endif

/*****************************************************************************/
/*****************************************************************************/
