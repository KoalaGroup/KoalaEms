/*
 * lowlevel/rawmem/rawmem.c
 * created 2004-Jun-04 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: rawmem.c,v 1.5 2011/04/06 20:30:27 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#ifndef HAVE_CGETCAP
#include <getcap.h>
#endif
#include <emsctypes.h>
#include <errorcodes.h>
#include <dev/rawmem.h>
#include <rcs_ids.h>
#include "rawmem.h"

#if 0
#ifndef MAP_FAILED
#define MAP_FAILED ((void*)-1)
#else
#error map_failed is defined
#error es ist MAP_FAILED
#endif
#endif

RCS_REGISTER(cvsid, "lowlevel/rawmem")

struct rawmem_part {
        struct rawmem_part* next;
        struct rawmem_part* prev;
        ems_u32 paddr;
        ems_u32 len;
        int used;
};

static struct rawmem_part partroot;
static struct rawmem_part* parts=&partroot;
static struct rawmem rawmem;

static struct fraction fraction_used;

static int
gcD_euclid(int a, int b)
{
    int r;

    while (b>0) {
        r=a%b;
        a=b;
        b=r;
    }
    return a;
}

#if 0
static int
gcD_steiner(int a, int b)
{
    int k=0, c;

    while (!(a&1) && !(b&1)) {
        a>>=1;
        b>>=1;
        k++;
    }
    if (!(b&1)) {
        c=a;
        a=b;
        b=c;
    }
    do {
        while (!(a&1))
            a>>=1;
        if (a<b) {
            c=a;
            a=b;
            b=c;
        }
        a-=b;
    } while (a!=0);
    return b*(1<<k);
}
#endif

static int
lcM(int a, int b)
{
    int gcd;

    gcd=gcD_euclid(a, b);
    if (gcd==0)
        return 0;
    else
        return (a/gcd)*b;
}

static void
fraction_reduce(struct fraction* fraction)
{
    int gcd;

    gcd=gcD_euclid(fraction->numerator, fraction->denominator);
    fraction->numerator/=gcd;
    fraction->denominator/=gcd;
}

static struct fraction
fraction_add(struct fraction a, struct fraction b)
{
    struct fraction res;
    int lcm=lcM(a.denominator, b.denominator);
    a.numerator*=lcm/a.denominator;
    b.numerator*=lcm/b.denominator;
    res.numerator=a.numerator+b.numerator;
    res.denominator=lcm;
    return res;
}

static void
fraction_equalize(struct fraction *a, struct fraction *b)
{
    int lcm=lcM(a->denominator, b->denominator);
    a->numerator*=lcm/a->denominator;
    b->numerator*=lcm/b->denominator;
    a->denominator=lcm;
    b->denominator=lcm;
}

static char*
rawmem_p2v(ems_u32 paddr)
{
        return rawmem.buf+(paddr-rawmem.paddr);
}

#if 0
static ems_u32
rawmem_v2p(char* buf)
{
        return rawmem.paddr+(buf-rawmem.buf);
}
#endif

int
rawmem_low_printuse(FILE* outfilepath)
{
        fprintf(outfilepath, "  [:rmemp=<rawmempath>]\n");
        return 1;
}

errcode
rawmem_low_init(char* arg)
{
        char *rawmempath;

        printf("rawmem_low_init(%s)\n", arg);
        partroot.next=0;
        partroot.prev=0;
        partroot.paddr=0;
        partroot.len=0;
        partroot.used=0;

        rawmem.p = -1;
        rawmem.len = 0;
        rawmem.paddr = 0;
        rawmem.buf = 0;

        fraction_used.numerator=0;
        fraction_used.denominator=1;

        if ((!arg) || (cgetstr(arg, "rmemp", &rawmempath)) < 0) {
	        printf("no rawmem device given\n");
	        return OK;
        }
        rawmem.p = open(rawmempath, O_RDWR, 0);
        if (rawmem.p < 0) {
	        printf("open %s: %s\n", rawmempath, strerror(errno));
	        return Err_System;
        }
        if (fcntl(rawmem.p, F_SETFD, FD_CLOEXEC) <0 ) {
	        printf("fcntl(rawmem, FD_CLOEXEC): %s\n", strerror(errno));
        }
        if (ioctl(rawmem.p, RAWMEM_GETPBASE, &rawmem.paddr)<0) {
	        printf("ioctl(rawmem, getbase): %s\n", strerror(errno));
                close(rawmem.p);
	        return Err_System;
        }
        if (ioctl(rawmem.p, RAWMEM_GETSIZE, &rawmem.len)<0) {
	        printf("ioctl(rawmem,  getsize): %s\n", strerror(errno));
                close(rawmem.p);
	        return Err_System;
        }

        rawmem.buf = (char*)mmap(0, rawmem.len, PROT_READ|PROT_WRITE,
			     MAP_FILE | MAP_SHARED, rawmem.p, 0);
        if (rawmem.buf == MAP_FAILED) {
	        printf("mmap rawmem: %s\n", strerror(errno));
                close(rawmem.p);
	        return Err_System;
        }

        partroot.paddr=rawmem.paddr;
        partroot.len=rawmem.len;

        printf("rawmem: %ld bytes available\n", rawmem.len);
        return OK;
}

errcode
rawmem_low_done(void)
{
        if (rawmem.p >= 0) {
                if (rawmem.buf) munmap(rawmem.buf, rawmem.len);
	        close(rawmem.p);
	        rawmem.p = -1;
        }

        while (partroot.next) {
                struct rawmem_part* help=partroot.next;
                partroot.next=help->next;
                free(help);
        }

        return OK;
}

static int
rawmem_check_splitter(struct fraction request, const char* caller)
{
    struct fraction sum;

    sum=fraction_add(fraction_used, request);
    if (sum.numerator>sum.denominator) {
        struct fraction used_=fraction_used, request_=request;

        fraction_equalize(&used_, &request_);
        printf("rawmem_check: %s requested %d/%d but only %d/%d are available\n",
            caller,
            request_.numerator, request_.denominator,
            used_.denominator-used_.numerator,
            used_.denominator);
        return -1;
        
    }

    fraction_used=sum;
    fraction_reduce(&fraction_used);
    return 0;
}

#if 0
size_t
rawmem_size(void)
{
        return rawmem.len&~3;
}
#endif

#if 0
size_t
rawmem_available(void)
{
    unsigned int len=0;
    struct rawmem_part* part=parts;
    do {
        if ((!part->used) && (part->len>len)) len=part->len;
        part=part->next;
    } while (part);
    return len&~3;
}
#endif

#if 0
static void
rawmem_dump(void)
{
    struct rawmem_part* part=parts;
    do {
        printf("  %p 0x%08x %sused %d\n", part, part->paddr, part->used?"  ":"un", part->len);
        part=part->next;
    } while (part);
}
#endif

static int
rawmem_split_part(struct rawmem_part* part, size_t len)
{
    struct rawmem_part* newpart;

    if (part->len<len) {
        printf("rawmem_split_part(len=%llu): part has only %d bytes\n",
                (unsigned long long)len, part->len);
        return -1;
    }
    if (part->len==len) {
        printf("rawmem_split_part(len=%llu): part has exactly requested size\n",
                (unsigned long long)len);
        return 0;
    }
    newpart=(struct rawmem_part*)malloc(sizeof(struct rawmem_part));
    if (!newpart) {
        printf("rawmem_split_part(): cannot allocate new part\n");
        return -1;
    }
    newpart->next=part->next;
    newpart->prev=part;
    newpart->paddr=part->paddr+len;
    newpart->len=part->len-len;
    newpart->used=0;
    if (part->next)
            part->next->prev=newpart;
    part->next=newpart;
    part->len=len;
    return 0;
}

static int
rawmem_merge_parts(struct rawmem_part* part)
{
    struct rawmem_part* next=part->next;
    part->next=next->next;
    if (next->next)
            next->next->prev=part;
    part->len+=next->len;
    free(next);
    return 0;
}

int
rawmem_alloc(struct fraction req, struct rawmem* mem, const char* caller)
{
    struct rawmem_part* part=0;
    struct rawmem_part* help;
    size_t len;

    mem->p=rawmem.p;
    mem->len=0;
    mem->paddr=0;
    mem->buf=0;

    if (rawmem_check_splitter(req, caller)<0)
        return -1;

    len=rawmem.len&~3;
    len*=req.numerator;
    len/=req.denominator;
    len&=~3;

    if (len==0) {
        printf("rawmem_alloc: requested size is 0!\n");
        return 0;
    }

    /* find a part not smaller than len */
    help=parts;
    while (help) {
        if (!help->used && (help->len>=len)) {
            if (!part || (help->len<part->len)) {
                part=help;
            }
        }
        help=help->next;
    }
    if (!part) {
        printf("rawmem_alloc: no space for %llu bytes (program error)\n",
                (unsigned long long)len);
        return -1;
    }
    if (rawmem_split_part(part, len)<0)
        return -1;
    part->used=1;

    mem->len=len;
    mem->paddr=part->paddr;
    mem->buf=rawmem_p2v(part->paddr);

    printf("rawmem: %d/%d remaining\n",
            fraction_used.denominator-fraction_used.numerator,
            fraction_used.denominator);
#if 0
    printf("rawmem dump:\n");
    rawmem_dump();
#endif
    return 0;
}

int
rawmem_alloc_part(const char* part, struct rawmem* mem, const char* caller)
{
    char *part_, *slash, *end;
    int numerator, denominator;
    struct fraction req;

    part_=strdup(part);
    slash=strchr(part_, '/');
    if (slash) {
        *slash++='\0';
        denominator=strtol(slash, &end, 0);
        if (*end) {
            printf("%s: cannot parse denominator %s of rawmempart %s\n",
                    caller, slash, part);
            free(part_);
            return -1;
        }
    } else {
        denominator=1;
    }
    if (denominator==0) {
        printf("%s: invalid denominator %d in rawmempart\n",
            caller, denominator);
            free(part_);
            return -1;
    }

    numerator=strtol(part_, &end, 0);
    if (*end) {
        printf("%s: cannot parse numerator %s of rawmempart %s\n",
                caller, part_, part);
        free(part_);
        return -1;
    }

    if (numerator*denominator<0) {
        printf("%s: illegal sign of rawmempart\n", caller);
        free(part_);
        return -1;
    }
    free(part_);

    req.numerator=numerator;
    req.denominator=denominator;
    fraction_reduce(&req);
    return rawmem_alloc(req, mem, caller);
}

int
rawmem_free(struct rawmem* mem)
{
    /* find part */
    struct rawmem_part* part=parts;

    if (!mem->paddr)
        return 0;

    while (part) {
        if (part->paddr==mem->paddr)
            break;
        part=part->next;
    }
    if (!part) {
        printf("rawmem_free: part (0x%08lx) not found.\n", mem->paddr);
        return -1;
    }
    part->used=0;
    if ((part->next) && (!part->next->used)) {
        rawmem_merge_parts(part);
    }
    if ((part->prev) && (!part->prev->used)) {
        rawmem_merge_parts(part->prev);
    }
    return 0;
}
