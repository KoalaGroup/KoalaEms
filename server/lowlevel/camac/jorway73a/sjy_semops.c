/* sjy_semops.c				jms 6sep94 */
/* 21aug97 das Modified vsd_semops for Linux CAMAC */

/* operations on the semaphores - init, get and put. 
 * sjy_inisem()		called in inicam
 * sjy_getsem,putsem	called to reserve the semaphore
 *
 */

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <errno.h>

#ifdef VXWORKS
#define SJY_SEM_TYPE SEM_ID
#else
#define SJY_SEM_TYPE int
#endif

#define SJY_FILE "/tmp/sjykeyfile"
SJY_SEM_TYPE sjy_sem = (SJY_SEM_TYPE) -1;

/* Note for flags:
 *	IPC_WAIT is default (dont sepecify IPC_NOWAIT)
 *	SEM_UNDO aids clean up on exit(2), by changing semadj
 */

int sjy_inisem()
{
#ifndef VXWORKS
  key_t	sjy_key;
  union semun arg;
#endif
  int fd;

/* only initialize it once */
  if(sjy_sem > (SJY_SEM_TYPE) -1)
    return( (int) sjy_sem);

#ifdef VXWORKS
  sjy_sem = semBCreate( SEM_Q_PRIORITY, SEM_FULL);
  return( (int) sjy_sem);
#else

/* create the key file */
  if(fd = open(SJY_FILE, O_CREAT | O_EXCL, 664) < 0){
    if (errno != EEXIST){
      printf("sjy_semops: Severe Error, could not create key file %s, 
             error=%d\n", SJY_FILE,errno);
      return(-1);
    } 
  }

/* reserve the Semaphore */
  if((sjy_key = ftok(SJY_FILE,1))== -1){
	printf("sjy_semops: Severe Error, could not get key for %s error=%d\n",
		SJY_FILE,errno);
        return (-1);
	}

/* check to see if its already in system */
   sjy_sem = semget(sjy_key,1,IPC_EXCL|IPC_CREAT|0777);
   if(errno != EEXIST){
   /* it didnt already exist, open it back up as non EXCL and set it to 1 */
     if(semctl(sjy_sem,0,IPC_RMID,arg) != 0){
	printf("sjy_semops: Severe Error, couldnt close semaphore\n");
	return(-1);
	}

     if((sjy_sem = semget(sjy_key,1,IPC_CREAT|0777)) == -1){
	printf("sjy_semops: Severe Error, couldnt reopen semaphore\n");
	return(-1);
	}

   /* set the semval to 1 to let only 1 process use it at a time */
     arg.val=1;
     if(semctl(sjy_sem,0,SETVAL,arg)!=0){
	printf("sjy_semops: Severe Error, couldnt set semaphore\n");
        return(-1);
        }
     }

   else{
   /* it already existed, so just open it up */
     sjy_sem = semget(sjy_key,1,IPC_CREAT|0777);
     }

   return(sjy_sem);
#endif
}

int sjy_getsem()
{
/* get the sjy semaphore */

#ifdef VXWORKS
  return(semTake (sjy_sem, WAIT_FOREVER));
#else
struct sembuf	sops[1];

/* load the sops structure */
sops[0].sem_num	= 0;
sops[0].sem_op	= -1;
sops[0].sem_flg	= SEM_UNDO;		/* WAIT for it */
return(semop(sjy_sem, sops, 1));

#endif
}

int sjy_putsem()
{
/* release the sjy semaphore */

#ifdef VXWORKS
  return(semGive (sjy_sem));
#else
struct sembuf	sops[1];

/* load the sops structure */
sops[0].sem_num	= 0;
sops[0].sem_op	= 1;
sops[0].sem_flg	= SEM_UNDO;
return(semop(sjy_sem, sops, 1));

#endif
}
