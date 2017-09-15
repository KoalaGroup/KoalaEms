#include <errno.h>
#include <strings.h>
#if defined(unix) || defined(__unix__)
#include <stdlib.h>
#endif

#include <errorcodes.h>

static int vics;
static int *vichandles;

#ifdef OSK
#include "vic.h"
#define SEEK_SET 0
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "vicvsbreg.h"
static int *vicspaces;
#endif

errcode vicvsb_low_init(arg)
char *arg;
{
  char *vicpath,*help;
  int i;
  if((!arg)||(cgetstr(arg,"vicp",&vicpath)<0)){
    printf("kein VIC-Device gegeben\n");
    return(Err_ArgNum);
  }
  vics=1;
  help=vicpath;
  while(help=index(help,',')){
    vics++;
    help++;
  }
  vichandles=(int*)calloc(vics,sizeof(int));
  if(!vichandles)return(Err_NoMem);
#if defined(unix) || defined(__unix__)
  vicspaces=(int*)calloc(vics,sizeof(int));
  if(!vicspaces)return(Err_NoMem);
#endif
  help=vicpath;
  for(i=0;i<vics;i++){
    char *end;
    end=index(help,',');
    if(end)*end='\0';
    vichandles[i]=
#ifdef OSK
      open(help,3);
#else
      open(help,O_RDWR,0);
#endif
    if(vichandles[i]==-1){
      printf("kann VICbus (%s) nicht erreichen, Error %d\n",help,errno);
      if(end)*end=',';
      free(vichandles);
#if defined(unix) || defined(__unix__)
      free(vicspaces);
#endif
      return(Err_System);
    }
    if(end)*end=',';
#if defined(unix) || defined(__unix__)
    if(ioctl(vichandles[i],GETVICSPACE,&vicspaces[i])<0){
      int j;
      printf("kann VIC-Space nicht ermitteln, Error %d\n",errno);
      for(j=0;j<=i;j++)close(vichandles[j]);
      free(vichandles);
      free(vicspaces);
      return(Err_System);
    }
#endif
    help=end+1;
  }
  return(OK);
}

errcode vicvsb_low_done()
{
  int i;
  if(vichandles){
    for(i=0;i<vics;i++)
      close(vichandles[i]);
    free(vichandles);
    vichandles=(int*)0;
  }
#if defined(unix) || defined(__unix__)
  if(vicspaces)free(vicspaces);
#endif
  return(OK);
}

int vicread(crate,space,adr,val)
int crate,space,adr;
int *val;
{
  if(crate>=vics)return(-1);
#ifdef OSK
  *val=_getstat(vichandles[crate],ss_singleTransfer,space,adr);
#else
  if(space!=vicspaces[crate])return(-3);
  lseek(vichandles[crate],adr,SEEK_SET);
  read(vichandles[crate],val,sizeof(int));
#endif
  return(0);
}

int vicwrite(crate,space,adr,val)
int crate,space,adr,val;
{
  if(crate>=vics)return(-1);
#ifdef OSK
  _setstat(vichandles[crate],ss_singleTransfer,space,adr,val);
#else
  if(space!=vicspaces[crate])return(-3);
  lseek(vichandles[crate],adr,SEEK_SET);
  write(vichandles[crate],&val,sizeof(int));
#endif
  return(0);
}

int vicblread(crate,space,adr,buf,len)
int crate,space,adr;
int *buf,*len;
{
  int res;
  if(crate>=vics)return(-1);
#ifdef OSK
  _setstat(vichandles[crate],ss_setSpace,space);
#else
  if(space!=vicspaces[crate])return(-3);
#endif
  lseek(vichandles[crate],adr,SEEK_SET);
  res=read(vichandles[crate],buf,(*len)*sizeof(int));
  if(res<0)return(-1);
  *len=res/sizeof(int);
  return(0);
}

int vicblwrite(crate,space,adr,buf,len)
int crate,space,adr;
int *buf;
int len;
{
  if(crate>=vics)return(-1);
#ifdef OSK
  _setstat(vichandles[crate],ss_setSpace,space);
#else
  if(space!=vicspaces[crate])return(-3);
#endif
  lseek(vichandles[crate],adr,SEEK_SET);
  if(write(vichandles[crate],buf,len*sizeof(int))<0)return(-1);
  return(0);
}

int csrread(crate,num,val)
int crate,num;
int *val;
{
  if(crate>=vics)return(-1);
#ifdef OSK
  *val=_getstat(vichandles[crate],ss_CSR,num);
  if(*val==-1)return(-1);
#else
  {
  struct viccsr r;
  r.reg=num;
  if(ioctl(vichandles[crate],READCSR,&r)<0)return(-1);
  *val=r.val;
  }
#endif
  return(0);
}

int csrwrite(crate,num,val)
int crate,num,val;
{
  if(crate>=vics)return(-1);
#ifdef OSK
  if(_setstat(vichandles[crate],ss_CSR,num,val)==-1)return(-1);
#else
  {
  struct viccsr r;
  r.reg=num;
  r.val=val;
  if(ioctl(vichandles[crate],WRITECSR,&r)<0)return(-1);
  }
#endif
  return(0);
}
