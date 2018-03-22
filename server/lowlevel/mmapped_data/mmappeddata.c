/*
 * lowlevel/mmapped_data/mmappeddata.c
 * 
 * created: 2011-08-04 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: mmappeddata.c,v 1.5 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/sem.h>

#include <errorcodes.h>
#include <rcs_ids.h>
#include "mmapped_data.h"
#include "mmappeddata.h"

RCS_REGISTER(cvsid, "lowlevel/mmapped_data")

struct mmaphandle {
    char *filename;
    void *addr;
    size_t length;
    ems_u32 last_sequence;
    int prot;
    int flags;
    int fd;
    key_t sem_key;
    int sem_id;
};

/*
 * struct mem has to match the struct used in wago2shm.c and others.
 * We should use a common header file!
 */
struct mem {
    u_int32_t sequence;
    u_int32_t valid;
    u_int32_t size;
    u_int32_t data[1];
};

/*
 * proj_id has to match the id used in wago2shm.c and others.
 * We should use a common header file!
 */
static const int proj_id=31; /* arbitrary constant for ftok */

static struct mmaphandle *maps=0;
static unsigned int nummaps=0;

/*****************************************************************************/
plerrcode
mmap_data(char *name, ems_u32 *handle)
{
    struct mmaphandle *map=0;
    unsigned int idx;

    /* find an unused map entry or create a new one */
    for (idx=0; idx<nummaps; idx++) {
        if (!maps[idx].addr) {
            map=maps+idx;
            break;
        }
    }
    if (!map) { /* create a new entry */
        map=realloc(maps, sizeof(struct mmaphandle)*(nummaps+1));
        if (!map) {
            printf("mmap_data: realloc(num=%d): %s\n",
                    nummaps+1, strerror(errno));
            return plErr_NoMem;
        }
        maps=map;
        idx=nummaps;
        nummaps++;
        map=maps+idx;
    }
    memset(map, 0, sizeof(struct mmaphandle));
    *handle=idx;

    /* fill the map structure with default values */
    map->filename=strdup(name);
    if (!map->filename) {
        printf("mmap_data: strdup: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    map->length=getpagesize();
    map->prot=PROT_READ;
    map->flags=MAP_SHARED;

    /* open the file */
    map->fd=open(map->filename, O_RDONLY);
    if (map->fd<0) {
        printf("mmap_data: open \"%s\": %s\n", map->filename, strerror(errno));
        return plErr_NoFile;
    }

    /* map the file */
    map->addr=mmap(0, map->length, map->prot, map->flags, map->fd, 0);
    if (map->addr==MAP_FAILED) {
        printf("mmap_data: mmap \"%s\": %s\n",
                map->filename, strerror(errno));
        return plErr_System;
    }

    /* create a key from the file */
    map->sem_key=ftok(map->filename, proj_id);
    if (map->sem_key==-1) {
        printf("mmap_data: ftok: %s\n", strerror(errno));
        return plErr_System;
    }

    /* attach the semaphore */
    map->sem_id=semget(map->sem_key, 1/*nsems*/, 0 /*semflg*/);
    if (map->sem_id<0) {
        printf("mmap_data: semget: %s\n", strerror(errno));
        return plErr_System;
    }

    return plOK;
}
/*****************************************************************************/
static int
do_remap(struct mmaphandle *map, size_t size)
{
    int pagesize=getpagesize();
    void *new_map;
    /* because pagesize is a power of 2 the next line rounds size up to
       the next page boundery */
    size=(size+pagesize-1)&~(pagesize-1);

    if (map->length>=size)
        return 0;

    new_map=mremap(map->addr, map->length, size, MREMAP_MAYMOVE);
    if (new_map==MAP_FAILED) {
        printf("mmap_data: mremap: %s\n", strerror(errno));
        return -1;
    }
    map->addr=new_map;
    map->length=size;
    return 0;
}
/*****************************************************************************/
static void
do_unmap(struct mmaphandle *map)
{
/*
 *     do we have to close/detach the semaphore?
 */

    if (!map->addr)
        return;
    munmap(map->addr, map->length);
    map->addr=0;
    close(map->fd);
    free(map->filename);
}
/******************************************************************************/
plerrcode
munmap_data(ems_u32 handle)
{
    struct mmaphandle *map=maps+handle;

    if (handle>=nummaps || !map->addr) {
        printf("munmap_data: invalid handle %d\n", handle);
        return plErr_ArgRange;
    }
    do_unmap(map);
    return plOK;
}
/******************************************************************************/
static int
get_lock(int sem_id, int wait)
{
    struct sembuf sops;

    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=wait?SEM_UNDO:IPC_NOWAIT|SEM_UNDO;
    if (semop(sem_id, &sops, 1)<0) {
        if (errno!=EAGAIN) {
            printf("mmappeddata: semop(-1): %s\n", strerror(errno));
        }
        return -1;
    }
    return 0;
}
/******************************************************************************/
static void release_lock(int sem_id)
{
    struct sembuf sops;

    sops.sem_num=0;
    sops.sem_op=1;
    sops.sem_flg=SEM_UNDO;
    if (semop(sem_id, &sops, 1)<0)
        printf("mmappeddata: semop(+1): %s\n", strerror(errno));
}
/*****************************************************************************/
plerrcode read_mmapped_data(ems_u32** outptr, ems_u32 handle,
        int dont_suppress, int wait)
{
    struct mmaphandle *map=maps+handle;
    struct mem *mem;
    ems_u32 *ptr=*outptr;
    int size, i;
    plerrcode pres=plOK;

    if (handle>=nummaps || !map->addr) {
        printf("read_mmapped_data: invalid handle %d\n", handle);
        return plErr_ArgRange;
    }
    mem=(struct mem*)map->addr;

    /*
     * we read the sequence number without the lock because we just
     * want to know whether it has changed or not
     */
    if (!dont_suppress) {
        if (mem->sequence==map->last_sequence)
            return plOK;
    }

    if (!mem->valid) {
        /* ... but nobody sets valid to 'false' */
        printf("read_mmapped_data: mem content not valid\n");
        return plErr_Other;
    }

    if (get_lock(map->sem_id, wait)<0)
        return plOK;

    map->last_sequence=mem->sequence;
    size=mem->size;
    if (size+sizeof(struct mem)>map->length) {
        if (do_remap(map, size+sizeof(struct mem))<0) {
            pres=plErr_NoMem;
            goto error;
        }
        mem=(struct mem*)map->addr;
    }

    *ptr++=size;
    for (i=0; i<size; i++) {
        *ptr++=mem->data[i];
    }
    *outptr=ptr;

error:
    release_lock(map->sem_id);
    return pres;
}
/*****************************************************************************/

/*****************************************************************************/
errcode
mmapped_data_low_init(__attribute__((unused)) char* arg)
{
    maps=0;
    nummaps=0;
    return OK;
}
/*****************************************************************************/
errcode
mmapped_data_low_done(void)
{
    unsigned int i;

    for (i=0; i<nummaps; i++)
        do_unmap(maps+i);
    return OK;
}
/*****************************************************************************/
int
mmapped_data_low_printuse(__attribute__((unused)) FILE* outpath)
{
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
