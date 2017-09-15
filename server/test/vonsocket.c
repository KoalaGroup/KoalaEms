#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <eventformat.h>

#define BUFSIZE 11000
int buf[BUFSIZE];

xrecv(sock,buf,len)
int sock,*buf,len;
{
    int restlen,da,i;
    char *bufptr;
    restlen=len*sizeof(int);
    bufptr=(char*)buf;
    while(restlen){
	da=recv(sock,bufptr,restlen,0);
	if(da>0){
	    bufptr+=da;
	    restlen-=da;
	}else{
	    if(da==-1)perror("recv");
	    else printf("recv: %d\n",da);
	    exit(0);
	}
    }
    for(i=0;i<len;i++)buf[i]=ntohl(buf[i]);
}

main(argc,argv)
int argc;
char **argv;
{
    u_short optval=1;
    struct sockaddr_in sa;
    int sock,s;
    int port,size;

    if(argc!=2){
	printf("%s port\n", argv[0]);
	exit(1);
    }
    sscanf(argv[1],"%d", &port);
    printf("*:%d\n", port);

    if((sock=socket(AF_INET,SOCK_STREAM,0))==-1){
	perror("socket");
	exit(1);
    }
    if(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(char*)&optval,
		  sizeof(optval))==-1){
	perror("setsockopt");
	exit(1);
    }
    bzero(&sa, sizeof(struct sockaddr_in));
    sa.sin_family=AF_INET;
    sa.sin_port=htons(port);
    sa.sin_addr.s_addr=INADDR_ANY;
    if (bind(sock,&sa,sizeof(struct sockaddr_in))==-1){
	perror("bind");
	exit(1);
    }
    if(listen(sock, 8)==-1){
	perror("listen");
	exit(1);
    }
    size=sizeof(struct sockaddr);
    if((s=accept(sock,&sa, &size))==-1){
	perror("accept");
	exit(1);
    }

    printf("accepted\n");
    for(;;){
      struct eventheader ev;
      int i,anz,testlen;

/*recv(s,buf,BUFSIZE*4,0);
continue;*/

xrecv(s,&ev,sizeof(struct eventheader)/sizeof(int));
      testlen=ev.len-sizeof(struct eventinfo)/sizeof(int);
      anz=ev.info.numofsubevs;
      printf("Blocklen %d, Event %d, Trigger %d, %d Instrumentierungssysteme\n",
	     ev.len,ev.info.evno,ev.info.trigno,anz);
      for(i=0;i<anz;i++){
	union{
	  int raw[2];
	  struct subeventheader sev;
	}se;
	int sublen;
	do{
	  xrecv(s,&se,1);
	}while(se.raw[0]==PADDING_VAL);
	xrecv(s,&se.raw[1],sizeof(struct subeventheader)/sizeof(int)-1);
	sublen=se.sev.subevlen;
	if(sublen>BUFSIZE){
	  printf("sublen=%d\n",sublen);
	  exit(1);
	}
	printf("    IS %d: %d Datenworte\n",se.sev.isid,sublen);
	testlen-=(sublen+2);
	xrecv(s,buf,sublen); /* ignoriere Daten */
      }
      if(testlen)printf("Fehler in Laenge\n");
    }
    close(s);
    close(sock);
}


