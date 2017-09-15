/*
 * lowlevel/cosybeam/cosybeam.c
 * 
 * created: 06.Aug.2001 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: cosybeam.c,v 1.9 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include <errorcodes.h>
#ifndef HAVE_CGETCAP
#include <getcap.h>
#endif
#include <rcs_ids.h>
#include "cosybeam.h"

key_t shm_key;
key_t sem_key;
int shm_id, sem_id;
volatile struct shmformat* shm;
int sequence;

RCS_REGISTER(cvsid, "lowlevel/cosybeam")

/*****************************************************************************/
static int init_shm(void)
{
    shm_id=shmget(shm_key, sizeof(struct shmformat), IPC_CREAT|0666);
    if (shm_id<0) {
        printf("cosybeam.c:shmget: strerror(errno)\n");
        return -1;
    }

    shm=(struct shmformat*)shmat(shm_id, 0, 0);
    if (shm==(struct shmformat*)-1) {
        printf("cosybeam.c:shmat: strerror(errno)\n");
        return -1;
    }
    return 0;
}
/*****************************************************************************/
static void done_shm(void)
{
    if (shm)
        shmdt((void*)shm);
}
/*****************************************************************************/
static int init_sem(void)
{
    sem_id=semget(sem_key, 1/*nsems*/, IPC_CREAT|0666 /*semflg*/);
    if (sem_id<0) {
        printf("cosybeam.c:semget: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}
/*****************************************************************************/
static void done_sem(void)
{
}
/******************************************************************************/
static int get_lock(int wait)
{
    struct sembuf sops;

    sops.sem_num=0;
    sops.sem_op=-1;
    sops.sem_flg=wait?SEM_UNDO:IPC_NOWAIT|SEM_UNDO;
    if (semop(sem_id, &sops, 1)<0) {
        if (errno!=EAGAIN) {
            printf("cosybeam.c: semop(-1): %s\n", strerror(errno));
        }
        return -1;
    }
    return 0;
}
/******************************************************************************/
static void release_lock(void)
{
struct sembuf sops;

    sops.sem_num=0;
    sops.sem_op=1;
    sops.sem_flg=SEM_UNDO;
    if (semop(sem_id, &sops, 1)<0)
        printf("cosybeam.c: semop(+1): %s\n", strerror(errno));
}
/*****************************************************************************/
errcode cosybeam_low_init(char* arg)
{
    char *beamkey;

    shm_key=-1;
    sem_key=-1;
    shm_id=-1;
    sem_id=-1;
    shm=0;

    if((!arg) || (cgetstr(arg, "beamkey", &beamkey) < 0)){
        printf("no keyfile for cosybeam given\n");
        /*return(Err_ArgNum);*/
        return(OK);
    }
    if (strcmp(beamkey, "none")==0) {
        printf("beamkey not used\n");
        return OK;
    }

    shm_key=ftok(beamkey, 19);
    free(beamkey);
    if (shm_key==-1) {
        printf("cosybeam_low_init: ftok: %s\n", strerror(errno));
        return Err_System;
    }
    sem_key=shm_key;

    if (init_sem()<0) return Err_System;
    if (init_shm()<0) return Err_System;

    sequence=shm->sequence;

    return OK;
}
/*****************************************************************************/
errcode cosybeam_low_done(void)
{
    done_sem();
    done_shm();
    return OK;
}
/*****************************************************************************/
int cosybeam_low_printuse(FILE* outpath)
{
    fprintf(outpath, "  [:beamkey=<path for beamkey>]\n");
    return 1;
}
/*****************************************************************************/
plerrcode cosybeam_get_data(ems_u32** outptr, int num, int dont_suppress,
    int wait)
{
    ems_u32* ptr=*outptr;
    int i;

    if (!shm) return plErr_Other;
    if (!dont_suppress) {
        if (shm->sequence==sequence) return plOK;
    }
    if (get_lock(wait)<0) return plOK;

    sequence=shm->sequence;
    if (shm->data.num<num) num=shm->data.num;
    if (shm->valid) {
        *ptr++=4+num;
        *ptr++=shm->data.version;
        *ptr++=shm->data.tv.tv_sec;
        *ptr++=shm->data.tv.tv_usec;
        *ptr++=num;
        for (i=0; i<num; i++) {
            *ptr++=shm->data.vals[i];
        }
        *outptr=ptr;
    }
    release_lock();
    return plOK;
}
/*****************************************************************************/
/*****************************************************************************/
