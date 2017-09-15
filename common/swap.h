/*
 * common/swap.h
 * $ZEL: swap.h,v 2.6 2004/11/26 14:37:51 wuestner Exp $
 * 
 * created before 15.09.93
 */

#ifndef _swap_h_
#define _swap_h_
#include <cdefs.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "emsctypes.h"

/*
swap_falls_noetig() ruft swap() auf, wenn das Hostbyteordering nicht
  Motorola-like ist

swap_words_falls_noetig() ruft swap_words() auf, wenn das Hostbyteordering
  nicht Motorola-like ist

swap u. swap_words sind in swap.c definiert
*/

#ifdef WORDS_BIGENDIAN

#define swap_falls_noetig(x,y)
#define swap_words_falls_noetig(x,y)
#define MASK0_23 0xff000000
#define MASK0_15 0xffff0000
#define MASK0_7 0xffffff00

#else

#define swap_falls_noetig(x,y) swap(x,y)
#define swap_words_falls_noetig(x,y) swap_words(x,y)
__BEGIN_DECLS
void swap __P((ems_u32*, unsigned int));
void swap_words __P((ems_u32*, unsigned int));
__END_DECLS
#define MASK0_7 0x00ffffff
#define MASK0_15 0x0000ffff
#define MASK0_23 0x000000ff

#endif /* WORDS_BIGENDIAN */

#endif
/*****************************************************************************/
/*****************************************************************************/
