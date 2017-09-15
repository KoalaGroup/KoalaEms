/*
 * dataout/cluster/clusterpool.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: clusterpool.c,v 1.7 2011/04/13 20:04:13 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <emsctypes.h>
#include <human_size.h>
#include <rcs_ids.h>
#include "clusterpool.h"
#include "../../main/server.h"
#include "../../lowlevel/oscompat/oscompat.h"
#include "../../lowlevel/lowbuf/lowbuf.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#undef POOLDEBUG
#ifdef POOLDEBUG
#   undef POOLDEBUGV

#   define PD(x) {x}
#   ifdef POOLDEBUGV
#       define PDV(x) {x}
#   else
#       define PDV(x) {}
#   endif
    struct entry {
        struct entry* next;
        struct entry* prev;
        void* addr;
        size_t size;
        char* text;
    };
    struct entry* eroot=0;
#else
#   define PD(x) {}
#   define PDV(x) {}
#endif

#define POOL_MAGIC 0xa55aa55a
enum poolstat {pool_free, pool_used, pool_end};
struct ctrl {
    size_t size;
    ems_u32 magic;
    enum poolstat stat;
};

enum clpooltype clpooltype=clpool_none;
static void* pool;
static size_t poolsize;
static char poolname[1024];
static struct ctrl* next_start;
static modresc modref;
static size_t ctrlsize;

RCS_REGISTER(cvsid, "dataout/cluster")

/*****************************************************************************/
#ifdef POOLDEBUG
static void
debug_dump_entries(void)
{
    printf("+++ EDUMP +++\n");
    struct entry* e=eroot;
    while (e) {
        printf("%p %lld %s\n", e->addr, (unsigned long long)e->size,
            e->text?e->text:"events");
        e=e->next;
    }
}
/*****************************************************************************/
static void
dump_pool(void)
{
    static void* poolend;
    struct ctrl *c, *nc;
    enum poolstat laststat;

    debug_dump_entries();
    printf("+++ POOLDUMP +++\n");
    poolend=(char*)pool+poolsize-ctrlsize;
    printf("pool=%p poolend=%p\n", pool, poolend);

    nc=pool;

    do {
        c=nc;
        if (((void*)c<pool) || ((void*)c>poolend)) {
            printf("pool=%p c=%p end=%p\n", pool, c, poolend);
            exit(1);
        }
        printf("c=%p size=0x%08llx magic=%08x stat=%d\n", c,
                (unsigned long long)c->size, c->magic, c->stat);
        laststat=c->stat;
        nc=(struct ctrl*)((char*)c+ctrlsize+c->size);
    } while ((c->magic==POOL_MAGIC) && (laststat!=pool_end));
    printf("--- pooldump ---\n");
}
/*****************************************************************************/
static void
debug_add_entry(void* addr, size_t size, const char* text)
{
    struct entry* e;
    if (text||1)
        printf("pooldebug:add     %p 0x%08llx: %s\n",
                addr, (unsigned long long)size, text?text:"events");
    e=malloc(sizeof(struct entry));
    e->next=eroot;
    e->prev=0;
    e->addr=addr;
    e->size=size;
    e->text=text?strdup(text):0;
    if (eroot)
        eroot->prev=e;
    eroot=e;
}
/*****************************************************************************/
static void
debug_remove_entry(void* addr)
{
    struct entry* e=eroot;
    PDV(printf("pooldebug:remove %p\n", addr);)
    while (e && (e->addr!=addr))
        e=e->next;
    if (e) {
        if (e->text||1)
            printf("pooldebug:remove %p 0x%08llx: %s\n",
                e->addr, (unsigned long long)e->size, e->text?e->text:"events");
        if (e->prev)
            e->prev->next=e->next;
        else
            eroot=e->next;
        if (e->next)
            e->next->prev=e->prev;
        if (e->text)
            free(e->text);
        free(e);
    } else {
        printf("pooldebug:remove %p not allocated\n", addr);
    }
}
/*****************************************************************************/
static void
debug_free_entries(void)
{
    while (eroot) {
        struct entry* e=eroot;
        eroot=e->next;
        free(e);
    }
}
#endif
/*****************************************************************************/
void
clusterpool_dump(void)
{
#ifdef POOLDEBUG
    dump_pool();
#endif
}
/*****************************************************************************/
enum clpooltype
clusterpool_type(void)
{
    return clpooltype;
}
/*****************************************************************************/
const char*
clusterpool_name(void)
{
    return poolname;
}
/*****************************************************************************/
off_t
clusterpool_offset(void* cluster)
{
    return (char*)cluster-(char*)pool;
}
/*****************************************************************************/
static errcode
alloc_clusterpool_malloc(size_t size)
{
    pool=calloc(1, size);
    if (pool==0) {
        printf("alloc_clusterpool_malloc(%lld): %s\n", (long long int)size,
                strerror(errno));
        return Err_NoMem;
    }
    if (mlock(pool, size)<0) {
        /* not fatal but alarming */
        printf("mlock(clusterpool): %s\n", strerror(errno));
    }
    return OK;
}
/*****************************************************************************/
static errcode
alloc_clusterpool_lowbuf(size_t size)
{
#ifdef LOWLEVELBUFFER
    if (size>lowbuf_dataoutbuffersize()*sizeof(int)) {
        printf("alloc_clusterpool_lowbuf: "
            "requested buffer(%d)>available buffer(%d)\n",
            size, lowbuf_dataoutbuffersize()*sizeof(int));
        return Err_ArgRange;
    }
    pool=lowbuf_dataoutbuffer();
    return OK;
#else
    return Err_NotImpl;
#endif
}
/*****************************************************************************/
static errcode
alloc_clusterpool_shm(size_t size)
{
    snprintf(poolname, 1024, "%s_%05d", "ems", getpid());
    pool=init_datamod(poolname, size, &modref);
    if (pool==0) {
        printf("alloc_clusterpool_shm: init_datamoPD(%s, %lld Byte, ...): %s\n",
                poolname, (unsigned long long)size, strerror(errno));
        return Err_System;
    }
    return OK;
}
/*****************************************************************************/
void
clusterpool_clear(void)
{
    struct ctrl *c;

printf("clusterpool_clear\n");
    c=(struct ctrl*)pool;
    c->size=poolsize-2*ctrlsize;
    c->magic=POOL_MAGIC;
    c->stat=pool_free;
    next_start=(struct ctrl*)pool;
    PD(printf("alloc_clusterpool: first ctrl=%p size=0x%llx stat=%d\n",
        c, (unsigned long long)c->size, c->stat);)

    c=(struct ctrl*)((char*)c+ctrlsize+c->size);
    c->size=0;
    c->magic=POOL_MAGIC;
    c->stat=pool_end;
    PD(printf("alloc_clusterpool: last ctrl=%p size=0x%llx stat=%d\n",
        c, (unsigned long long)c->size, c->stat);)
    PD(eroot=0;)
}
/*****************************************************************************/
/*
 * clusterpool_alloc allocates a pool large enough to hold clnum clusters
 * of size clsize (in terms of sizeof(ems_u32))
 * the clusters allocated from the pool may have different size (smaller or
 * larger)
 */
errcode
clusterpool_alloc(enum clpooltype pooltype, size_t clsize, int clnum)
{
    errcode res;

/* find the smallest power of 2 which is not smaller then sizeof(struct ctrl)*/
    ctrlsize=1;
    while (ctrlsize<sizeof(struct ctrl))
        ctrlsize<<=1;
    PD(printf("sizeof(ctrl)=%lld ctrlsize=%lld\n",
        (unsigned long long)sizeof(struct ctrl), (unsigned long long)ctrlsize);)

/* round clsize to multiple of struct ctrl */
    clsize*=sizeof(ems_u32);
    clsize=(clsize+ctrlsize-1) & ~(ctrlsize-1);
    poolsize=(clsize+ctrlsize)*clnum;

    printf("alloc_clusterpool: type=");
    switch (pooltype) {
        case clpool_none:   printf("none");   break;
        case clpool_malloc: printf("malloc"); break;
        case clpool_lowbuf: printf("lowbuf"); break;
        case clpool_shm:    printf("shm");    break;
        default: printf("unknown (%d)", pooltype);
    }
    printf(" clsize=0x%llx clnum=%d\n", (unsigned long long)clsize, clnum);
    printf("poolsize=0x%llx (%sByte)\n", (unsigned long long)poolsize,
            human_size(poolsize));

    if (clpooltype!=clpool_none) {
        printf("alloc_clusterpool: pool already allocated\n");
        return Err_Program;
    }

    poolname[0]='\0';

    switch (pooltype) {
    case clpool_malloc:
        res=alloc_clusterpool_malloc(poolsize);
        break;
    case clpool_lowbuf:
        res=alloc_clusterpool_lowbuf(poolsize);
        break;
    case clpool_shm:
        res=alloc_clusterpool_shm(poolsize);
        break;
    default:
        printf("alloc_clusterpool: unknown pooltype %d\n", pooltype);
        return Err_Program;
    }
    if (res!=OK)
        return res;

    PD(printf("alloc_clusterpool: pool=%p\n", pool);)
    clpooltype=pooltype;

    clusterpool_clear();

    PD(dump_pool();)
    return OK;
}
/*****************************************************************************/
errcode
clusterpool_release(void)
{
    switch (clpooltype) {
    case clpool_none:
        printf("free_clusterpool: no pool allocated\n");
        return Err_Program;
    case clpool_malloc:
        if (munlock(pool, poolsize)<0) {
            /* not fatal but alarming */
            printf("mlock(clusterpool): %s\n", strerror(errno));
        }
        free(pool);
        break;
    case clpool_lowbuf:
        /* nothing to do */
    case clpool_shm:
        done_datamod(&modref);
        break;
    }
    clpooltype=clpool_none;
    PD(debug_free_entries();)

    return OK;
}
/*****************************************************************************/
static void
allocate_free_block(struct ctrl *c, size_t size)
{
    PDV(printf("allocate_free_block: ctrl=%p stat=%d size=0x%llx\n",
            c, c->stat, (unsigned long long)size);)
    if (c->size-size<=ctrlsize) {
        /* not possible to split this block */
        PD(printf("allocate_free_block: using whole block size=0x%llx\n",
            (unsigned long long)c->size);)
        c->stat=pool_used;
    } else {
        struct ctrl* nc;

        nc=(struct ctrl*)((char*)c+ctrlsize+size);
        nc->size=c->size-size-ctrlsize;
        nc->magic=POOL_MAGIC;
        nc->stat=pool_free;
        PDV(printf("allocate_free_block: nc=%p stat=%d size=0x%llx\n",
            nc, nc->stat, (unsigned long long)nc->size);)

        c->size=size;
        c->stat=pool_used;
    }
}
/*****************************************************************************/
static void
merge_blocks(struct ctrl* c)
{
    struct ctrl* nc;
    enum poolstat stat;

    PDV(printf("merge_blocks: ctrl=%p stat=%d\n", c, c->stat);)
    do {
        nc=(struct ctrl*)((char*)c+ctrlsize+c->size);
        PDV(printf("merge_blocks: nc=%p stat=%d\n", nc, nc->stat);)
        if (nc->magic!=POOL_MAGIC) {
            printf("merge_blocks: clusterpool corrupted\n");
            /*panic();*/exit(1);
        }
        stat=nc->stat;
        switch (stat) {
        case pool_free:
            c->size+=nc->size+ctrlsize;
            PDV(printf("merge_blocks: new size=0x%llx\n",
                (unsigned long long)c->size);)
            break;
        case pool_used:
            break;
        case pool_end:
            break;
        }
        
    } while (stat==pool_free);
}
/*****************************************************************************/
static struct ctrl*
find_free_block(size_t size)
{
    struct ctrl *c;

    /* begin the search at the last allocated block */
    c=next_start;
    PDV(printf("find_free_block: ctrl=%p stat=%d (next_start) size=0x%llx\n",
            c, c->stat, (unsigned long long)size);)
    do {
        if (c->magic!=POOL_MAGIC) {
            printf("find_free_block: clusterpool corrupted\n");
            /*panic();*/exit(1);
        }
        switch (c->stat) {
        case pool_free:
            merge_blocks(c);
            if (c->size>=size) {
                allocate_free_block(c, size);
                next_start=(struct ctrl*)((char*)c+ctrlsize+c->size);
                PDV(printf("find_free_block: next_start=%p\n", next_start);)
                return c;
            } else {
                c=(struct ctrl*)((char*)c+ctrlsize+c->size);
                PDV(printf("find_free_block: next ctrl=%p stat=%d\n", c, c->stat);)
            }
            break;
        case pool_used:
            c=(struct ctrl*)((char*)c+ctrlsize+c->size);
            PDV(printf("find_free_block: next ctrl=%p stat=%d\n", c, c->stat);)
            break;
        case pool_end:
            break;
        }
    } while (c->stat!=pool_end);

    /* try the same from begin of pool */
    c=pool;
    PDV(printf("find_free_block: ctrl=%p stat=%d (pool)\n", c, c->stat);)
    do {
        if (c->magic!=POOL_MAGIC) {
            printf("find_free_block: clusterpool corrupted\n");
            /*panic();*/exit(1);
        }
        switch (c->stat) {
        case pool_free:
            merge_blocks(c);
            if (c->size>=size) {
                allocate_free_block(c, size);
                next_start=(struct ctrl*)((char*)c+ctrlsize+c->size);
                PDV(printf("find_free_block: next_start=%p\n", next_start);)
                return c;
            } else {
                c=(struct ctrl*)((char*)c+ctrlsize+c->size);
                PDV(printf("find_free_block: next ctrl=%p stat=%d\n", c, c->stat);)
            }
            break;
        case pool_used:
            c=(struct ctrl*)((char*)c+ctrlsize+c->size);
            PDV(printf("find_free_block: next ctrl=%p stat=%d\n", c, c->stat);)
            break;
        case pool_end:
            break;
        }
    } while (c<next_start);

    /* no free space found */
    return 0;
}
/*****************************************************************************/
void*
alloc_cluster(size_t size, const char* text)
{
    struct ctrl *c;
    void* cluster;

    PDV(dump_pool();)
    PD(if (text||1) printf("alloc_cluster               size=0x%llx: %s\n",
            (unsigned long long)size, text?text:"events");)
    if (clpooltype==clpool_none) {
        printf("alloc_cluster(%llu): pool not initialised!\n",
            (unsigned long long)size);
        return 0;
    }

/* round size to multiple of ctrlsize (for alignment) */
    size=(size+ctrlsize-1) & ~(ctrlsize-1);
    PDV(printf("alloc_cluster: size=0x%llx (rounded)\n", (unsigned long long)size);)

    c=find_free_block(size);
    if (c==0) {
        /*printf("alloc_cluster(%llu): no free buffer found!\n",
            (unsigned long long)size);*/
        PD(dump_pool();)
        return 0;
    }

    cluster=((char*)c)+ctrlsize;
    PD(debug_add_entry(cluster, size, text);)
    PDV(printf("alloc_cluster: control=%p cluster=%p\n", c, cluster);)
    PDV(dump_pool();)

#ifdef USE_CLUSTER_BEEF
    memset(cluster, 0xa5, size);
#endif

    return cluster;
}
/*****************************************************************************/
void
free_cluster(void* cluster)
{
    struct ctrl *c;

    PDV(printf("free_cluster:    %p\n", cluster);)
    c=(struct ctrl*)((char*)cluster-ctrlsize);
    PDV(printf("free_cluster: control=%p\n", c);)
    PD(debug_remove_entry(cluster);)

    if (clpooltype==clpool_none) {
        printf("free_cluster(%p): pool not initialised!\n", cluster);
        return;
    }

    if (c->magic!=POOL_MAGIC) {
        printf("free_cluster(%p): no magic!\n", cluster);
        return;
    }

    if (c->stat!=pool_used) {
        printf("free_cluster(%p): block not allocated!\n", cluster);
        return;
    }

    c->stat=pool_free;
}
/*****************************************************************************/
int
clusterpool_valid(void* cluster)
{
    struct ctrl *c=pool;
    while ((((char*)c)+ctrlsize!=cluster) && (c->stat!=pool_end)) {
        c=(struct ctrl*)((char*)c+ctrlsize+c->size);
    }
    if (c->stat==pool_used)
        return 1;
    else
        return 0;
}
/*****************************************************************************/
/*****************************************************************************/
