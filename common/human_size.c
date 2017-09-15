/*
 * common/human_size.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: human_size.c,v 2.1 2011/04/13 19:57:23 wuestner Exp $";

#include <debug.h>
#include <errno.h>
#include <stdio.h>
#include "human_size.h"
#include "rcs_ids.h"

RCS_REGISTER(cvsid, "common")

/*****************************************************************************/
static const char* prefices[]={
#if 0 /* not official */
    "l",  /* 10**-63 lunto  */
    "mi", /* 10**-60 mikto  */
    "nk", /* 10**-57 nekto  */
    "o",  /* 10**-54 otro   */
    "pk", /* 10**-51 pekro  */
    "q",  /* 10**-48 quekto */
    "r",  /* 10**-45 rimto  */
    "s",  /* 10**-42 sotro  */
    "td", /* 10**-39 trekto */
    "u",  /* 10**-36 unto   */
    "v",  /* 10**-33 vunkto */
    "w",  /* 10**-30 wekto  */
    "x",  /* 10**-27 xonto  */
#endif
#if 0 /* official */
    "y",  /* 10**-24 yocto  */
    "z",  /* 10**-21 zepto  */
    "a",  /* 10**-18 atto   */
    "f",  /* 10**-15 femto  */
    "p",  /* 10**-12 pico   */
    "n",  /* 10**-9  nano   */
    "u",  /* 10**-6  micro  */
    "m",  /* 10**-3  milli  */
    "c",  /* 10**-2  centi  */
    "d",  /* 10**-1  deci   */
#endif
    "",  /* 10**-0  2**0 (none) */
#if 0
    "D", /* 10**1   deka   */
    "h", /* 10**2   hecto  */
#endif
    "Ki", /* 10**3  2**10 Kilo   */
    "Mi", /* 10**6  2**20 Mega   */
    "Gi", /* 10**9  2**30 Giga   */
    "Ti", /* 10**12 2**40 Tera   */
    "Pi", /* 10**15 2**50 Peta   */
    "Ei", /* 10**18 2**60 Exa    */
    "Zi", /* 10**21 2**70 Zetta  */
    "Yi", /* 10**24 2**80 Yotta  */
#if 0 /* not official */
    "Xi",  /* 10**27 2**90 Xona */
    "Wi",  /* 10**30 2**100 Weka */
    "Vi",  /* 10**33 2**110 Vunda */
    "Ui",  /* 10**36 2**120 Uda */
    "TDi", /* 10**39 2**130 Treda */
    "S",   /* 10**42 2**140 Sorta */
    "R",   /* 10**45 2**150 Rinta */
    "Q",   /* 10**48 2**160 Quexa */
    "PP",  /* 10**51 2**170 Pepta */
    "O",   /* 10**54 2**180 Ocha */
    "N",   /* 10**57 2**190 Nena */
    "MI",  /* 10**60 2**200 Minga */
    "L",   /* 10**63 2**210 Luma */
#endif
    0
};
/*****************************************************************************/
const char*
human_size(unsigned long long size)
{
    static char s[1024];
    const char** prefix=prefices;
    double d=size;

    while ((d>1024.) && (*(prefix+1))) {
        d/=1024.;
        prefix++;
    }
    sprintf(s, "%.3f %s", d, *prefix);
    return s;
}
/*****************************************************************************/
/*****************************************************************************/
