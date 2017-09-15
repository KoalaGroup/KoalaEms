/******************************************************************************
*                                                                             *
* swap.c                                                                      *
*                                                                             *
* ULTRIX/OS9/OSF1                                                             *
*                                                                             *
* 15.09.93                                                                    *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: swap.c,v 2.7 2011/04/07 14:07:08 wuestner Exp $";

#ifdef OSK
#include <types.h>
#include <in.h>
#else
#ifdef GO32
#include <djgppstd.h>
#else
#include <sys/types.h>
#include <netinet/in.h>
#endif
#endif
#include "swap.h"
#include "rcs_ids.h"

RCS_REGISTER(cvsid, "common")

/*****************************************************************************/
/*
swap konvertiert ein Integerarray der Laenge len mit Adresse adr von Network
in Host-Byteorder (oder andersrum)
*/
void swap(ems_u32* adr, unsigned int len)
{
    while (len--) {
        *adr=ntohl(*adr);
        adr++;
    }
}

/*****************************************************************************/
/*
swap_words vertauscht 16-bit-Worte in einem Integerarray der Laenge len mit
Adresse adr
*/
void swap_words(ems_u32* adr, unsigned int len)
{
    while (len--) {
        *adr=((*adr>>16)&0x0000ffff) | ((*adr<<16)&0xffff0000U);
        adr++;
    }
}

/*****************************************************************************/
/*****************************************************************************/

