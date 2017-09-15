#define ALIGN8 1

#include <eventformat.h>
#include "../../../lowlevel/profile/profile.h"
#include "../../../dataout/dataout.h"
#include "rtinfo.h"
#include "gibweiter.h"

#ifdef OSK
#define SEEK_SET 0
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#ifndef NOVOLATILE
#define VOLATILE volatile
#else
#define VOLATILE
#endif

extern int bufferanz;
static int neues_event,nochraus;
static int *doutptr,*iscnt;

void initgibweiter()
{
  nochraus=bufferanz;
  neues_event=1;
}

void gib_weiter(buf,evh)
volatile int *buf;
struct eventheader evh;
{
  int *dest,i;

  if(evh.info.evno==neues_event){
    int *src;
    src=(int*)&(evh.info);
    dest=next_databuf;
    iscnt=&(((struct eventinfo*)dest)->numofsubevs);
    i=sizeof(struct eventinfo)/sizeof(int);
    while(i--)*dest++= *src++;
    neues_event++;
  }else{
    *iscnt+=evh.info.numofsubevs;
    dest=doutptr;
  }
  i=evh.len-sizeof(struct eventinfo)/sizeof(int);
  while(i--)*dest++= *buf++;
  doutptr=dest;
  if(!(--nochraus)){
    PROFILE_END(PROF_RO);
    flush_databuf(doutptr);
    nochraus=bufferanz;
  }
}

void gib_weiter_d(info,buf,evh)
struct inpinfo *info;
volatile int *buf;
struct eventheader evh;
{
  int *dest,i,datalen;
#ifdef ALIGN8
  int inpos;
#endif

  if(evh.info.evno==neues_event){
    int *src;
    src=(int*)&(evh.info);
    dest=next_databuf;
    iscnt=&(((struct eventinfo*)dest)->numofsubevs);
    i=sizeof(struct eventinfo)/sizeof(int);
    while(i--)*dest++= *src++;
    neues_event++;
  }else{
    *iscnt+=evh.info.numofsubevs;
    dest=doutptr;
  }

#ifdef ALIGN8
  inpos=(int)buf-info->base;
  if((inpos^(int)dest)&4){
    *dest++=PADDING_VAL;
  }
  lseek(info->path,inpos,SEEK_SET);
#else
  lseek(info->path,(int)buf-info->base,SEEK_SET);
#endif

  datalen=evh.len*sizeof(int)-sizeof(struct eventinfo);
  read(info->path,dest,datalen);
  dest+=datalen/sizeof(int);
  doutptr=dest;
  if(!(--nochraus)){
    PROFILE_END(PROF_RO);
    flush_databuf(doutptr);
    nochraus=bufferanz;
  }
}
