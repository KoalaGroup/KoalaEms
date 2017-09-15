/*
 * swap_int.h
 * 
 * created: ??.??.1998 PW
 */

#ifndef swap_int

#define swap_int(a) ( ((a) << 24) | \
		      (((a) << 8) & 0x00ff0000) | \
		      (((a) >> 8) & 0x0000ff00) | \
	((unsigned int)(a) >>24) )

#endif
