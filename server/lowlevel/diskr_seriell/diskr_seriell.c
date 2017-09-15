#define DISKR_COM "/t1"

#include <modes.h>

#include <errorcodes.h>

static int fd;

enum{
	com_GetStatus,
	com_WriteUth,
	com_ReadUth,
	com_WritePW,
	com_ReadPW,
	com_WriteIout,
	com_ReadIout,
	com_WriteMask,
	com_ReadMask,
	com_WriteAll,
	com_ReadAll
};

static int sendser(crate,buf,len)
int crate;
char *buf;
int len;
{
	char c[2];
	int i;
	c[0]=0xff;
	c[1]=crate&0xff;
	write(fd,c,2);
	read(fd,&c[1],1);
	if(c[1]!=crate&0xff)return(-1);
	for(i=0;i<len;i++){
		register int flag=0;
		if(buf[i]==0xff){
			flag=1;
			write(fd,c,1);
		}
		write(fd,&buf[i],1);
		if(flag){
			read(fd,c,1);
			if(c[0]!=0xff)return(-1);
		}
		read(fd,&c[1],1);
		if(fd[1]!=buf[i])return(-1);
	}
	return(0);
}

static int recvser(buf,len)
char *buf;
int len;
{
int i;
if(read(fd,buf,len)!=len)return(-1);
for(i=0;i<len;i++){
	if(buf[i]==0xff){
		register int j;
		if(buf[i+1]!=0xff)return(-1);
		for(j=i+2;j<len;j++)buf[j-1]=buf[j];
		if(read(fd,&buf[len-1],1)!=1)return(-1);
	}
return(0);
}

int diskr_GetStatus(crate)
int crate;
{
char buf[1];
buf[0]=com_GetStatus;
sendser(crate,buf,1);
recvser(buf,1);
return(buf[0]);
}

diskr_WriteUth(crate,modul,channel,Uth);
int crate,modul,channel,Uth;
{
char buf[4];
buf[0]=com_WriteUth;
buf[1]=modul;
buf[2]=channel;
buf[3]=Uth;
sendser(crate,buf,4);
}

int diskr_ReadUth(crate,modul,channel)
int crate,modul,channel;
{
char buf[3];
buf[0]=com_ReadUth;
buf[1]=modul;
buf[2]=channel;
sendser(crate,buf,3);
recvser(buf,1);
return(buf[0]);
}

diskr_WritePW(crate,modul,PW)
int crate,modul,pw;
{
char buf[3];
buf[0]=com_WritePW;
buf[1]=modul;
buf[2]=PW;
sendser(crate,buf,3);
}

int diskr_ReadPW(crate,modul)
int crate,modul;
{
char buf[2];
buf[0]=com_ReadPW;
buf[1]=modul;
sendser(crate,buf,2);
recvser(buf,1);
return(buf[0]);
}

diskr_WriteIout(crate,modul,Iout)
int crate,modul,Iout;
{
char buf[3];
buf[0]=com_WriteIout;
buf[1]=modul;
buf[2]=Iout;
sendser(crate,buf,3);
}

int diskr_ReadIout(crate,modul)
int crate,modul;
{
char buf[2];
buf[0]=com_ReadIout;
buf[1]=modul;
sendser(crate,buf,2);
recvser(buf,1);
return(buf[0]);
}

diskr_WriteMask(crate,modul,mask)
int crate,modul,mask;
{
char buf[4];
buf[0]=com_WriteMask;
buf[1]=modul;
buf[2]=mask&0xff;
buf[3]=(mask&0xff00)>>8;
sendser(crate,buf,4);
}

int diskr_ReadMask(crate,modul)
int crate,modul;
{
char buf[2];
buf[0]=com_ReadMask;
buf[1]=modul;
sendser(crate,buf,2);
recvser(buf,2);
return(buf[0]|(buf[1]<<8));
}

diskr_WriteAll(crate,modul,data)
int crate,modul,*data;
{
char buf[22];
int i;
buf[0]=com_WriteAll;
buf[1]=modul;
for(i=0;i<20;i++)buf[i+2]=data[i];
sendser(crate,buf,22);
}

diskr_ReadAll(crate,modul,buffer,len)
int crate,modul,*buffer,len;
{
char buf[20];
buf[0]=com_ReadAll;
buf[1]=modul;
sendser(crate,buf,2);
recvser(buf,20);
for(i=0;(i<20)&&(i<len);i++)buffer[i]=buf[i];
}

errcode diskr_seriell_low_init()
{
fd=open(DISKR_COM,3);
if(fd==-1)return(Err_System);
return(OK);
}

errcode diskr_seriell_low_done()
{
if(fd!=-1)close(fd);
return(OK);
}
