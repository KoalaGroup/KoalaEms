#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>

static int fertig,*buffer,*ip;

void gib_weiter(buf,size)
int *buf,size;
{
    int evno;
    evno=buf[1];
    if(!(evno%100000))printf("Event %d\n",evno);
}

cleanup(sig)
int sig;
{
    shmdt(buffer);
    exit(0);
}

void liesringbuffer(){
    register int len;
    if(!(*ip)){
	/*sleep(0);*/
	return;
    }
    len= *(ip+1);
    if(len>1){
	gib_weiter(ip+1,len+1);
	*ip=0;
	ip+=len+2;
    }else{
	*ip=0;
	if(len==-2)fertig=2;
	else ip=buffer;
    }
}

main(argc,argv)
int argc;
char **argv;
{
    key_t key;
    int j,shmid;

    if(argc<2){
	printf("%s modulname\n",argv[0]);
	exit(1);
    }

    key=0;
    for(j=0;j<strlen(argv[1]);j++){
	key<<=7;
	key^=argv[1][j];
    }
    shmid=shmget(key,0,0600);
    if(shmid==-1){
	perror("shmget");
	exit(1);
    }
    buffer=(int*)shmat(shmid,0,0);
    if(buffer==(int*)-1){
	perror("shmat");
	exit(1);
    }

    signal(SIGINT,cleanup);

    ip=buffer;
    fertig=0;

    while(!fertig)liesringbuffer();

    cleanup(0);
}
