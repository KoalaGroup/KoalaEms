/*
 * common/xdrstring.c
 * created (before?) 21.02.95
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: xdrstring.c,v 2.12 2011/04/07 14:07:08 wuestner Exp $";

#ifndef OSK
#include <stdlib.h>
#endif
#include "swap.h"
#include "xdrstring.h"
#include <cdefs.h>
#include <arpa/inet.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(HAVE_STRINGS_H) || defined(OSK)
#include <strings.h>
#else
#include <string.h>
#endif

#include "rcs_ids.h"

RCS_REGISTER(cvsid, "common")

/*****************************************************************************/
/*
outstring konvertiert einen String vom c-Format in XDR-Format.
s ist Pointer auf den Quellstring, dest ist die Zieladresse in der XDR-Daten-
struktur, der Funktionswert zeigt auf die erste freie Adresse nach dem
String in der XDR-Datenstruktur
*/
ems_u32* outstring(ems_u32* dest, const char *s)
{
    int len;
    ems_u32 *save;

    *dest++=(len=(ems_u32)strlen(s));
    save=dest;
    memcpy(dest, s, len);
    dest += len>>2;
    switch (len & 3) {
        case 2: *dest++ &= MASK0_15; break;
        case 1: *dest++ &= MASK0_23; break;
        case 3: *dest++ &= MASK0_7; break;
    }
    swap_falls_noetig(save, dest-save);
    return(dest);
}

/* Wie oben, nur werden maximal n Characters kopiert */
ems_u32* outnstring(ems_u32* dest, const char *s, int n)
{
    int len;
    ems_u32 *save;

    len=strlen(s);
    if (len>n)
        len=n;
    *dest++=len;
    save=dest;
    memcpy(dest,s,len);
    dest+=(len>>2);
    switch(len&3) {
        case 2: *dest++&=MASK0_15;break;
        case 1: *dest++&=MASK0_23;break;
        case 3: *dest++&=MASK0_7;break;
    }
    swap_falls_noetig(save,dest-save);
    return(dest);
}
/*****************************************************************************/
ems_u32* append_xdrstring(ems_u32* dest, const char *s, int n)
{
    ems_u32 *wp, *new;
    char *cp;
    int clen, len;

    len=strlen(s);
    if (n>0 && len>n)
        len=n;
    clen=xdrstrclen(dest);

    /* address of first new character */
    cp=(char*)(dest+1)+clen;
    /* address of first word to change */
    wp=dest+clen/4+1;
    if (clen&3)
        *wp=ntohl(*wp);
    /* append new string */
    memcpy(cp, s, len);
    (*dest)+=len;

    new=dest+xdrstrlen(dest);
    switch(xdrstrclen(dest)&3) {
        case 1: *(new-1)&=MASK0_23;break;
        case 2: *(new-1)&=MASK0_15;break;
        case 3: *(new-1)&=MASK0_7;break;
    }
    swap_falls_noetig(wp, new-wp);
    return new;
}
/*****************************************************************************/
/*
extractstring konvertiert einen String vom XDR-Format in c-Format.
p ist Pointer auf den Quellstring in der XDR-Datenstruktur, s ist die
Zieladresse , der Funktionswert zeigt auf die naechste Adresse nach dem
String in der XDR-Datenstruktur
*/

ems_u32* extractstring(char *s, const ems_u32 *p)
{
    register int size, len;

    size= *p++;                 /* Stringlaenge im Normalformat = xdrstrclen */
    len=(size+3)>>2;            /* Anzahl der Worte = xdrstrlen-1 */
    swap_falls_noetig((ems_u32*)p, len);  /* Host-byte-ordering herstellen*/
    memcpy(s, p, size);
    s[size]=0;                            /* String abschliessen */
    swap_falls_noetig((ems_u32*)p, len);  /* zurueckdrehen, Original ist wiederhergestellt */
    return((ems_u32*)p+len);                  /* Pointer auf naechstes XDR-Element */
}

/*****************************************************************************/

ems_u32* xdrstrcdup(char** s, const ems_u32* ptr)
{
    ems_u32 *help;
    *s=(char*)malloc(xdrstrclen(ptr)+1);
    if (*s)
        help=extractstring(*s, ptr);
    else
        help=0;
    return(help);
}

/*****************************************************************************/
/*****************************************************************************/
