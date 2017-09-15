#include <sconf.h>
#include <debug.h>
#include <swap.h>
#include "../camac.h"

int *CAMACblread(dest, addr, num)
register int *dest;
register camadr_t addr;
register int num;
{
    int dummy;

    T(CAMACblread)
/*    dummy=CAMACread((int)addr|=(BURST|NOWAIT));*/
    addr=(camadr_t)((int)addr|(BURST|NOWAIT));
    dummy=CAMACread(addr);
    while (num--) *dest++=CAMACread(addr);
    return(dest);
}

int *CAMACblreadQstop(dest, addr,space)
register int *dest;
register camadr_t addr;
register int space;
{
    while(CAMACgotQ(*dest++=CAMACread(addr))&&(--space));
    if(!space){
	space=1000; /* Modul leeren */
	while(CAMACgotQ(CAMACread(addr))&&(--space));
	*dest++=0;
    }
    return(dest);
}

int *CAMACblreadAddrScan(dest,addr,num)
register int *dest;
register camadr_t addr;
register int num;
{
    while(num--){
	*dest++=CAMACread(addr);
    	CAMACinc(addr);
    }
    return(dest);
}

int *CAMACblwrite(addr, src, num)
register camadr_t addr;
register int *src, num;
{
    T(CAMACblwrite)
    addr=(camadr_t)((int)addr|(BURST|NOWAIT));
    while (num--)CAMACwrite(addr,*src++);
    return(src);
}

int *CAMACblread16(dest, addr, num) /* Byteordering inkonsistent !!! */
register int *dest;
register camadr_t addr;
register int num;
{
    int *save;
    int dummy;
#if defined(ultrix) || defined(__DECC)
    short *help;
#endif

    T(CAMACblread16)
    save=dest;

    /*dummy=CAMACread((int)addr|=(BURST|NOWAIT));*/
    addr=(camadr_t)((int)addr|(BURST|NOWAIT));
    dummy=CAMACread(addr);
    dummy=num;
#if defined(ultrix) || defined(__DECC)
    help=(short*)dest;
    while (num--) *(help++)=(short)CAMACread(addr);
    if (dummy&1) *(help++)=0;
    dest=(int*)help;
#else
    while (num--) *((short*)dest)++=(short)CAMACread(addr);
    if (dummy&1) *((short*)dest)++=0;
#endif
    swap_words_falls_noetig(save, dest-save);
    return(dest);
}
