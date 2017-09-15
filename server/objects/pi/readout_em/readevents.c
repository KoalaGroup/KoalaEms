/******************************************************************************
*                                                                             *
* readevents.c                                                                *
*                                                                             *
* 08.07.94                                                                    *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: readevents.c,v 1.11 2011/04/06 20:30:29 wuestner Exp $";

#if defined(unix) || defined(__unix__)
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#endif
#include <sconf.h>
#include <errorcodes.h>
#include <unsoltypes.h>
#include <eventformat.h>
#include <rcs_ids.h>
#include "../../../lowlevel/profile/profile.h"
#include "../../../commu/commu.h"
#include "../../../dataout/dataout.h"
#include "../readout.h"

#include "rtinfo.h"
#include "readevents.h"
#include "gibweiter.h"

RCS_REGISTER(cvsid, "objects/pi/readout_em")

extern int bufferanz;
extern struct inpinfo roinfo[];

ems_u32 eventcnt;
static int es_wartet_noch_jemand;

void initevreader()
{
es_wartet_noch_jemand=1;
eventcnt=0;
initgibweiter();
}

#ifdef OSK
#include "../../../lowlevel/vicvsb/vic.h"
#define read1(info,adr) _getstat(info->path,ss_singleTransfer,info->space,adr)
#define write1(info,adr,val) _setstat(info->path,ss_singleTransfer,info->space,adr,val)

void read4(info,adr,buf)
struct inpinfo *info;
int *adr,*buf;
{
  int i;
  i=4;
  while(i--){
    *buf++=_getstat(info->path,ss_singleTransfer,info->space,adr++);
  }
}

#else

static int read1(struct inpinfo* info, volatile int* adr)
{
    int res;
    lseek(info->path,(int)adr,SEEK_SET);
    read(info->path,&res,sizeof(int));
    return(res);
}

static void read4(struct inpinfo* info, volatile int* adr, int* buf)
{
    lseek(info->path,(int)adr,SEEK_SET);
    read(info->path,buf,4*sizeof(int));
}

static void write1(struct inpinfo* info, volatile int* adr, int* val)
{
    lseek(info->path,(int)adr,SEEK_SET);
    write(info->path,&val,sizeof(int));
}

#endif

void doreadout(callbackdata data)
{
  if (buffer_free)
  {
    int i;
#ifdef INPUT_BUFFER_HANDSHAKE
    int nirgends_daten;
    nirgends_daten=1;
#endif
    readout_active= -2;
    if (!es_wartet_noch_jemand)
    {
      eventcnt++;
    }
    es_wartet_noch_jemand=0;
    for (i=0;i<bufferanz;i++)
    {
      register struct inpinfo *inb;
      inb= &roinfo[i];
#define used inb->use_driver
#define polld inb->poll_driver
      if(inb->letztes_event==eventcnt){
	if(polld?read1(inb,inb->ip):*(inb->ip)){
	  struct eventheader evh;
	  PROFILE_START_2(PROF_RO,i);
	  if(polld){
	    read4(inb,inb->ip+1,(int*)&evh);
	  }else{
	    int i;
	    int *src,*dest;
	    src=(int*)inb->ip+1;
	    dest=(int*)&evh;
	    i=sizeof(struct eventheader)/sizeof(int); /*=4*/
	    while(i--)*dest++= *src++;
	  }
	  if(evh.len>-1){ /* gueltiges Event */
	    if(used){
	      gib_weiter_d(inb,inb->ip+1+
			   sizeof(struct eventheader)/sizeof(int),evh);
	      if(polld)
		write1(inb,inb->ip,0);
	      else
		*(inb->ip)=0;
	    }else{
	      gib_weiter(inb->ip+1+
			 sizeof(struct eventheader)/sizeof(int),evh);
	      *(inb->ip)=0;
	    }
	    inb->letztes_event++;
	    if((evh.info.evno!=inb->letztes_event)&&(evh.info.evno!=0)){
	      int buf[4];
	      PROFILE_END(PROF_RO);
	      printf("Fehler: Buffer %d erwartet %d, bekam %d, eventcnt=%d\n",
		     i,inb->letztes_event,evh.info.evno,eventcnt);
	      buf[0]=rtErr_Mismatch;
	      buf[1]=eventcnt;
	      buf[2]=i;
	      buf[3]=evh.info.evno;
	      send_unsolicited(Unsol_RuntimeError,buf,4);
	      readout_active= -3;
	      return;
	    }
	    inb->ip+=evh.len+2;
	  }else{ /* Bufferende oder Abbruch */
	    if(polld)
	      write1(inb,inb->ip,0);
	    else
	      *(inb->ip)=0;
	    if(evh.len==-2)
	      inb->letztes_event=(unsigned int)-1;
	    else
	      inb->ip=inb->bufstart;
	    es_wartet_noch_jemand=1;
	  }
	  PROFILE_END(PROF_RO);
#ifdef INPUT_BUFFER_HANDSHAKE
	  nirgends_daten=0;
#endif
	}else{
	  es_wartet_noch_jemand=1;
	}
	readout_active=1;
      } /* if ... erwarte Subevent */
    } /* for ... alle Inputs */
#ifdef INPUT_BUFFER_HANDSHAKE
    if(nirgends_daten)sleep_on_input();
#endif
  }
  else /* !buffer_free */
    schau_nach_speicher();
}
