/*
 * common/getcap.h
 * $ZEL: getcap.h,v 2.4 2004/02/24 12:47:19 wuestner Exp $
 */

#ifndef _getcap_h_
#define _getcap_h_

#include <cdefs.h>

__BEGIN_DECLS
char* cgetcap(char*, const char*, int);
int cgetstr(char*, const char*, char**);
int cgetustr(char*, const char*, char**);
int cgetnum(char*, const char*, long*);
__END_DECLS

#endif
