/*
 * swap_int.h
 * 
 * created: ??.??.1998 PW
 * $ZEL: swap_int.h,v 1.4 2014/07/07 20:14:04 wuestner Exp $
 */

#ifndef swap_int

#define U32(a) (static_cast<u_int32_t>(a))
#define swap_int(a) ( (U32(a) << 24) | \
                      (U32((a) << 8) & 0x00ff0000) | \
                      (U32((a) >> 8) & 0x0000ff00) | \
                      (U32(a) >>24) )

#endif
