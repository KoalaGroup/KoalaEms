/*
 * common/xdrfloat.h
 * $ZEL: xdrfloat.h,v 2.5 2004/11/26 14:37:52 wuestner Exp $
 */

#ifndef _xdrfloat_h_
#define _xdrfloat_h_

#include <cdefs.h>

#error this code is junk!
#error use nom/denom instead

#define outfloat(ptr,fl) ((*(float*)ptr=fl,ptr+1))
#define extractfloat(fl,ptr) ((*fl= *(float*)ptr,ptr+1))

__BEGIN_DECLS
ems_u32* outdouble(ems_u32*, double);
const ems_u32* extractdouble(double*, const ems_u32*);
__END_DECLS

#endif
