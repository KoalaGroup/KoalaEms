/*
 * lowlevel/oscompat/datamodule.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: datamodule.c,v 1.11 2011/04/06 20:30:27 wuestner Exp $";

#include <sconf.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>

#include "oscompat.h"

#if /*defined(unix) ||*/ defined(__unix__) || defined(Lynx)

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#ifdef Lynx
#include <stdlib.h>
#include <smem.h>
#include <mem.h>
#endif /* Lynx */

#ifdef DMALLOC
#include <dmalloc.h>
#endif
#include <rcs_ids.h>

RCS_REGISTER(cvsid, "lowlevel/oscompat")

int *init_datamod(char* name, int size, modresc* ref)
{
    key_t key;
    int i;
    key=0;
    for (i=0; i<strlen(name); i++) {
        key<<=7;
        key^=name[i];
    }
    D(D_MEM, printf("init_datamod(%s): try shmget(key=%d, size=%d, perm=0%o)\n",
        name, (int)key, size, 0777|IPC_CREAT);)
    ref->shmid=shmget(key,size,0777|IPC_CREAT);
    if (ref->shmid==-1)
        return 0;
    D(D_MEM, printf("init_datamod(%s): try shmat(shmid=%d, 0, 0)\n",
        name, ref->shmid);)
    ref->shmaddr=(int*)shmat(ref->shmid,0,0);
    if (ref->shmaddr==(int*)-1)
        return 0;
    return ref->shmaddr;
}

void done_datamod(modresc* ref)
{
    shmdt((char*)(ref->shmaddr));
    shmctl(ref->shmid,IPC_RMID,0);
}

int *link_datamod(char* name, modresc* ref)
{
    key_t key;
    int j;
    struct shmid_ds ds;

    key=0;
    for (j=0; j<strlen(name); j++) {
        key<<=7;
        key^=name[j];
    }
    D(D_MEM, printf("init_datamod(%s): try shmget(key=%d, size=0, perm=0%o)\n",
        name, (int)key, 0600);)
    printf("init_datamod(%s): try shmget(key=%d, size=0, perm=0%o)\n",
        name, (int)key, 0600);
    ref->shmid=shmget(key,0,0600);
    if (ref->shmid==-1)
        return 0;
    D(D_MEM, printf("init_datamod(%s): try shmat(shmid=%d, 0, 0)\n",
        name, ref->shmid);)
    printf("init_datamod(%s): try shmat(shmid=%d, 0, 0)\n", name, ref->shmid);
    ref->shmaddr=(int*)shmat(ref->shmid,0,0);
    if (ref->shmaddr==(int*)-1)
        return 0;
    if (shmctl(ref->shmid, IPC_STAT, &ds)<0) {
        printf("shmctl(%s): %s\n", name, strerror(errno));
        ref->len=0;
    } else {
        ref->len=ds.shm_segsz;
    }
    printf("len of datamod=%d\n", ref->len);
    return ref->shmaddr;
}

void unlink_datamod(modresc* ref)
{
    shmdt((char*)ref->shmaddr);
}

int *map_memory(int* start, int len, mmapresc* ref)
{
#ifdef Lynx
    static int cnt=0;
    long int relstart;
    int *res;
    ref->name=(char*)malloc(20);
    if (!ref->name)
        return 0;
    snprintf(ref->name, 20,"mapping%d",cnt++);
    relstart=(long int)start-DRAMBASE;
    ref->start=(int*)(relstart&-0x1000L);
    ref->len=(len+(relstart-(long int)ref->start+0x1000)/4-1)&(-0x1000/4);
    res=smem_create(ref->name,(char*)ref->start,ref->len*4,SM_WRITE|SM_READ);
    if (!res) {
        printf("map_memory (%x,%x) failed\n",start,len);
        return(0);
    }
    return res+(relstart-(long int)ref->start)/4;
#else /* Lynx */
    return 0;
#endif /* Lynx */
}

void unmap_memory(mmapresc* ref)
{
#ifdef Lynx
    if(ref->name) {
        smem_create(ref->name,(char*)ref->start,ref->len*4,SM_DETACH);
        smem_remove(ref->name);
        free(ref->name);
    }
#else /* Lynx */
#endif /* Lynx */
}

#else /* unix || Lynx */

#include <errno.h>

#include <sconf.h>
#ifdef USE_SSM
#include <ssm.h>
#endif /* USE_SSM */

int *init_datamod(char* name, int size, modresc* ref)
{
    mod_exec *head;
    int *ptr;

    if ((head= *ref=(mod_exec*)modlink(name,mktypelang(MT_DATA, ML_ANY)))==
            (mod_exec*)-1) {
        short attr,perm;
        attr=mkattrevs(MA_REENT,0);
        perm=MP_OWNER_MASK|MP_GROUP_MASK|MP_WORLD_MASK;
        head= *ref=(mod_exec*)_mkdata_module(name,size,attr,perm);
    }
    if (head!=(mod_exec*)-1) {
        ptr=(int*)((char*)head+head->_mexec);
    } else {
        int help;
        help=errno;
        printf("kann Datenmodul %s nicht anlegen\n",name);
        errno=help;
        return 0;
    }
    return ptr;
}

void done_datamod(modresc* ref)
{
    if (*ref!=(mod_exec*)-1) munlink(*ref);
}

int *link_datamod(char* name, modresc* ref)
{
    mod_exec* head;
    head= *ref=(mod_exec*)modlink(name,mktypelang(MT_DATA, ML_ANY));
    if (head==(mod_exec*)-1)
        return 0;
    return (int*)((char*)head+head->_mexec);
}

void unlink_datamod(modresc* ref)
{
   if (*ref!=(mod_exec*)-1) munlink(*ref);
}

int *map_memory(int *start, int len, mmapresc* ref)
{
#ifdef USE_SSM
    int res;
    res=permit(start,4*len,ssm_RW);
    if (res==-1) {
        printf("map_memory (%x,%x) failed\n",start,len);
        return(0);
    }
    ref->start=start;
    ref->len=len;
#endif /* USE_SSM */
    return start;
}

void unmap_memory(mmapresc* ref)
{
#ifdef USE_SSM
    protect(ref->start,ref->len*4);
#endif /* USE_SSM */
}

#endif /* unix || Lynx */

int oscompat_low_printuse(FILE* outfilepath)
{
/* nothing to configure */
    return 0;
}

errcode oscompat_low_init(char* arg)
{
    return OK;
}

errcode oscompat_low_done()
{
    return OK;
}
