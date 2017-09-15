/*
 * common/xdrfloat.c
 * 
 * 25.02.1999 PW
 */
#include "xdrfloat.h"

#error this code is junk!
#error use nom/denom instead

/*****************************************************************************/
ems_u32* outdouble(ems_u32* ptr, double d)
{
    union {
        double d;
        struct {
            ems_u32 h, l;
        } i;
    } x;
    x.d=d;
    *ptr++=x.i.l;
    *ptr++=x.i.h;
    return ptr;
}
/*****************************************************************************/
const ems_u32* extractdouble(double* d, const ems_u32* ptr)
{
    union {
        double d;
        struct {
            ems_u32 h, l;
        } i;
    } x;
    x.i.l=*ptr++;
    x.i.h=*ptr++;
    *d=x.d;
    return ptr;
}
/*****************************************************************************/
/*****************************************************************************/
