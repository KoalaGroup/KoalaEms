/*
 * lowlevel/rawmem/rawmem.h
 * $ZEL: rawmem.h,v 1.3 2008/06/26 14:23:54 wuestner Exp $
 * 
 * created 2004-Jun-04 PW
 */

#ifndef _rawmem_h_
#define _rawmem_h_

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <emsctypes.h>
#include <errorcodes.h>

struct fraction {
    int numerator;
    int denominator;
};

struct rawmem {
    int p;               /* path to rawmem devize */
    unsigned long len;   /* size of rawmem */
    unsigned long paddr; /* physical address of rawmem */
    char *buf;           /* rawmem mapping in userspace */
};

int rawmem_low_printuse(FILE*);
errcode rawmem_low_init(char*);
errcode rawmem_low_done(void);

/*size_t rawmem_size(void);*/
/*size_t rawmem_available(void);*/
size_t rawmem_available_split(int numerator, int denominator);
/*int rawmem_alloc(size_t len, struct rawmem* mem);*/
int rawmem_alloc(struct fraction, struct rawmem* mem,
    const char* caller);
int rawmem_alloc_part(const char* part, struct rawmem* mem,
    const char* caller);
int rawmem_free(struct rawmem* mem);

#endif
