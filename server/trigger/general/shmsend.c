static const char* cvsid __attribute__((unused))=
    "$ZEL: shmsend.c,v 1.3 2011/04/06 20:30:36 wuestner Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <rcs_ids.h>

static int id;
static int* addr;
static int eventcnt;
static int speed;

RCS_REGISTER(cvsid, "trigger/general")

int main(int argc, char* argv[])
{
if (argc!=2)
  {
  printf("usage: %s speed\n", argv[0]);
  exit(1);
  }
speed=atoi(argv[1]);

id=shmget(815, sizeof(int), IPC_CREAT|0666);
if (id==-1)
  {
  printf("shmsend: shmget: %s\n", strerror(errno));
  exit(1);
  }
addr=(int*)shmat(id, (void*)0, 0666);
if (addr==(int*)-1)
  {
  printf("shmsend: shmat: %s\n", strerror(errno));
  exit(1);
  }
/* *addr=0; */
eventcnt=0;
while(1)
  {
  sleep(1);
  *addr+=speed;
  }
}
