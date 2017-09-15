/*
 * dataout/ringbuffer/ringbuffer.c
 * created before 10.10.94
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: ringbuffer.c,v 1.22 2011/04/06 20:30:22 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <strings.h>

#ifdef NEED_STRDUP
char *strdup();
#endif

#include <errorcodes.h>
#include <rcs_ids.h>
#include "../dataout.h"
#include "../../objects/domain/dom_dataout.h"
#include "../../objects/pi/readout.h"
#include "../../lowlevel/profile/profile.h"
#include "../../lowlevel/oscompat/oscompat.h"

dataout_info dataout[1];

int *next_databuf;
int buffer_free;
extern ems_u32* outptr;
static volatile int *sp, *lp;
static int *last_databuf=0;

#if defined(EVENT_BUFBEG) && defined (EVENT_BUFSIZE)
#if (EVENT_BUFBEG==0)
#define USE_MODUL
#else
#ifdef OSK
#define CONST_BUFADR
#endif
#endif /* BUFBEG=0 */
#define BUFADR_GIVEN
#endif /* defined(EVENT_BUFBEG) ... */

#ifdef CONST_BUFADR
#define bufbeg (int*)EVENT_BUFBEG
#define bufend ((int*)EVENT_BUFBEG+EVENT_BUFSIZE/4)
#else
static int *bufbeg, *bufend;
#endif

#ifdef USE_MODUL
static modresc resc;
#else
static mmapresc mapping;
#endif

RCS_REGISTER(cvsid, "dataout/ringbuffer")

/*****************************************************************************/

errcode dataout_init(arg)
char *arg;
{
  char *name;
  int size,res;

  T(dataout_init)

#ifdef USE_MODUL

  name=EVENTBUFNAME;
  size=EVENT_BUFSIZE;
  res= -1;
  if(arg){
    res=cgetstr(arg, "obmn", &name);
    cgetnum(arg,"obms",&size);
  }
  bufbeg=init_datamod(name,size,&resc);
  if (res>=0) free(name);
  if (!bufbeg)
  {
    *outptr++=errno;
    return(Err_System);
  }
  bufend=bufbeg+size/4;
  dataout[0].addrtyp=Addr_Modul;
  dataout[0].addr.modulname=strdup(name);

#else /* kein modul -> mapping */

#ifdef CONST_BUFADR /* nur in OS-9, keine Translation */
  size=EVENT_BUFSIZE;
  if(!map_memory(bufbeg,size/4,&mapping)){
    printf("kann auf output-buffer nicht zugreifen\n");
    return(Err_System);
  }
  dataout[0].addr.addr=bufbeg;
#else /* nicht konstant -> setze Variablen */
#ifdef BUFADR_GIVEN
  bufbeg=(int*)EVENT_BUFBEG;
  size=EVENT_BUFSIZE;
#else
  res= -1;
  if(arg){
    res=cgetnum(arg,"bufbeg",&bufbeg);
    if(res!=-1)res=cgetnum(arg,"bufsize",&size);
  }
  if(res==-1){
    printf("Datenpuffer nicht angegeben!\n");
    return(Err_ArgNum);
  }
#endif /* BUFADR_GIVEN */
  dataout[0].addr.addr=bufbeg;
  bufbeg=map_memory(bufbeg,size/4,&mapping);
  if(!bufbeg){
    printf("kann auf output-buffer nicht zugreifen\n");
    return(Err_System);
  }
  bufend=bufbeg+size/4;
#endif /* CONST_BUFADR */
  dataout[0].addrtyp=Addr_Raw;

#endif /* modul */

  dataout[0].buffsize=size;
  dataout[0].wieviel=0;
  dataout[0].bufftyp=InOut_Ringbuffer;
  D(D_MEM, printf("init: Buffer ab %x bis %x\n", bufbeg, bufend);)
  *(volatile int*)bufbeg=0;  /* invalid */
  return(OK);
}

/*****************************************************************************/

errcode dataout_done()
{
  T(dataout_done)
#ifdef USE_MODUL
  done_datamod(&resc);
  if(dataout[0].addr.modulname)free(dataout[0].addr.modulname);
#else
  unmap_memory(&mapping);
#endif
  return(OK);
}

/*****************************************************************************/

errcode start_dataout()
{
sp=lp=bufbeg;
*sp=0;  /* invalid */
buffer_free=1;
next_databuf=(int*)sp+2;
return(OK);
}

/*****************************************************************************/

errcode stop_dataout()
{
*(sp+1)= -2;
*sp=1;
return(OK);
}

/*****************************************************************************/

#ifdef OSK
int* get_dataout_addr()
{
return(bufbeg);
}
#endif

/*****************************************************************************/

static int diff;

void schau_nach_speicher()
{
int len;

DV(D_MEM, printf("z.Zt. %d Worte frei\n", diff);)
while (diff<event_max)
  {
  int xxx= *lp;
  if (xxx)
    {
    DV(D_MEM, printf("warte bei %x\n",lp);)
#ifdef OUTPUT_BUFFER_HANDSHAKE
    if(buffer_free)signal_datadest();
#endif
    buffer_free=0;
    return;
    }else len= *(lp+1);
  DV(D_MEM, printf("Buffer bei %x frei, Laenge %d\n", lp, len);)
  if (len>-1)
    {
    diff+=len+2;
    lp+=len+2;
    }
  else
    {
    diff=event_max;
    lp=bufbeg;
    }
  }
buffer_free=1;
PROFILE_END(PROF_OUT);
}

/*****************************************************************************/

static void beschaffe_speicher()
{
sp+= (*(sp+1))+2;
if ((bufend-sp)>=event_max)
  {
  diff=lp-sp;
  DV(D_MEM, printf("naechster Buffer bei %x\n", sp);)
  if(diff>0){
    schau_nach_speicher();
  }else{
      buffer_free=1;
      PROFILE_END(PROF_OUT);
  }
}
else
  {
  PROFILE_START(PROF_OUTWARP);
  *(sp+1)= -1;*sp=1;
  DV(D_MEM, printf("springe an Anfang (%x)\n", bufbeg);)
  sp=lp=bufbeg;
  diff=0;
  schau_nach_speicher();
  }
}

/*****************************************************************************/

void flush_databuf(p)
volatile int *p;
{
T(flush_databuf)
PROFILE_START(PROF_OUT);
*p=0;
*(sp+1)=p-next_databuf;
*sp=1;
last_databuf=next_databuf;
DV(D_MEM, printf("Buffer %x gueltig, Laenge %d\n", sp, *sp);)
beschaffe_speicher();
next_databuf=(int*)sp+2;
}

/*****************************************************************************/

errcode get_last_databuf()
{
register int *ptr=last_databuf;
T(get_last_databuf)
if (ptr)
  {
  register int len= *(ptr-1);
  while (len--) *outptr++= *ptr++;
  }
else
  return(Err_NoDomain);
return(OK);
}

/*****************************************************************************/
/*****************************************************************************/
