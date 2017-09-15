#include <stdio.h>

main(argc,argv)
int argc;char **argv;
{
	int *ip,len,dummy,i,*ptr,*buffer;
	if(argc==2)sscanf(argv[1],"%x",&buffer);else{
			printf("usage: %s bufferaddr\n",argv[0]);
			exit(0);
		}
	ip=buffer;
	for(;;){
		if(!(*(ip))){
/*			printf("Warte an %x\n",ip);*/
			while(!(*(ip)))tsleep(1);
	}
		len=i= *(ip+1);ptr= ip+2;
/*		printf("Buffer an %x, len %d,event %d OK\n",ip,len,*(ip+2));*/
		if(i>0)while(i--)dummy= *ptr++;
		*(ip)=0;
		if(len>-1)ip+=len+2;
		else if(len==-2)exit(0); else ip=buffer;
	}
}

