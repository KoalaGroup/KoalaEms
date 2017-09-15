/*
 * common/xdrstring.h
 * $ZEL: xdrstring.h,v 2.8 2009/08/26 13:31:13 wuestner Exp $
 *
 * created 21.02.95 MD
 */

#ifndef _xdrstring_h_
#define _xdrstring_h_

#include <cdefs.h>
#include "emsctypes.h"

/*****************************************************************************/
/*
xdrstrlen ermittelt die Laenge (in Integers) eines Strings im XDR-Format
xdrstrclen ermittelt die Laenge (in Characters) eines Strings im XDR-Format
strxdrlen ermittelt die XDR-Laenge (in Integers) eines Strings im c-Format
*/
#define xdrstrlen(p) ((*(p) +3)/4+1)
#define xdrstrclen(p) (*(p))
#define strxdrlen(s) ((strlen(s) +3)/4+1)

__BEGIN_DECLS
ems_u32* outstring (ems_u32*, const char *);
ems_u32* outnstring (ems_u32*, const char *, int);
ems_u32* append_xdrstring(ems_u32* dest, const char *s, int n);
ems_u32* extractstring (char *, const ems_u32*);
ems_u32* xdrstrcdup (char**, const ems_u32*);
__END_DECLS

#endif

/*****************************************************************************/
/*****************************************************************************/
