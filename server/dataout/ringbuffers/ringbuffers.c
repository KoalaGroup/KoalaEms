/*
 * dataout/ringbuffers/ringbuffers.c
 * created before 22.07.93
 */

#include <errno.h>
#if defined(unix) || defined(__unix__)
#include <stdlib.h>
#endif

#include <sconf.h>
#include <errorcodes.h>
#include <debug.h>
#include "../../objects/pi/readout.h"
#include "../../lowlevel/profile/profile.h"
#include "../../lowlevel/oscompat/oscompat.h"

#include "multiringbuffer.h"
#include "ringbuffers.h"

extern ems_u32* outptr;

/*****************************************************************************/

void _init_dataout(dto)
DATAOUT *dto;
{
T(_init_dataout)
dto->sp=dto->lp=dto->bufbeg;
*(dto->sp)=0;  /* invalid */
dto->buffer_free=1;
dto->next_databuf=(int*)dto->sp+2;
dto->last_databuf=(int*)0;
}

void _done_dataout(dto)
DATAOUT *dto;
{
T(_done_dataout)
*(dto->sp+1)= -2;
*(dto->sp)=1;
}

DATAOUT *init_dataoutmod(name, size)
char *name;
int size;
{
  DATAOUT *dto;

  T(init_dataoutmod)
  if((dto=(DATAOUT*)malloc(sizeof(DATAOUT)))==0)return(0);
  dto->bufbeg=init_datamod(name,size,&(dto->ref));
  if(!(dto->bufbeg)){
    int help;
    help=errno;
    free(dto);
    errno=help;
    return(0);
  }
  dto->bufend=(int*)((char*)dto->bufbeg+size);
  D(D_MEM, printf("init: Buffer ab %x bis %x\n", dto->bufbeg, dto->bufend);)
  _init_dataout(dto);
  return(dto);
}

/*****************************************************************************/

DATAOUT *init_dataoutraw(addr,size)
int *addr,size;
{
  DATAOUT *dto;

  T(init_dataoutraw)
  if((dto=(DATAOUT*)malloc(sizeof(DATAOUT)))==0)return(0);
  dto->bufbeg=addr;
  dto->bufend=(int*)((char*)dto->bufbeg+size);
  D(D_MEM, printf("init: Buffer ab %x bis %x\n", dto->bufbeg, dto->bufend);)
  _init_dataout(dto);
  return(dto);
}

/*****************************************************************************/

void done_dataoutmod(dto)
DATAOUT *dto;
{
  T(done_dataoutmod)
  _done_dataout(dto);
  done_datamod(&(dto->ref));
  free(dto);
}

/*****************************************************************************/

void done_dataoutraw(dto)
DATAOUT *dto;
{
T(done_dataoutraw)
_done_dataout(dto);
free(dto);
}

/*****************************************************************************/

#ifdef OSK
int* _get_dataout_addr(dto)
DATAOUT *dto;
{
return(dto->bufbeg);
}
#endif

/*****************************************************************************/

void _schau_nach_speicher(dto)
DATAOUT *dto;
{
int len;

T(_schau_nach_speicher)
DV(D_MEM, printf("z.Zt. %d Worte frei\n", dto->diff);)
while (dto->diff<event_max)
  {
  int xxx= *(dto->lp);
  if (xxx)
    {
    DV(D_MEM, printf("warte bei %x\n",dto->lp);)
    dto->buffer_free=0;
    return;
    }
  else
    len= *(dto->lp+1);
  DV(D_MEM, printf("Buffer bei %x frei, Laenge %d\n", dto->lp, len);)
  if (len>-1)
    {
    dto->diff+=len+2;
    dto->lp+=len+2;
    }
  else
    {
    dto->diff=event_max;
    dto->lp=dto->bufbeg;
    }
  }
dto->buffer_free=1;
PROFILE_END_2(PROF_OUT,dto->xref);
}

/*****************************************************************************/

static void beschaffe_speicher(DATAOUT* dto)
{
  T(beschaffe_speicher)
  dto->sp+=(*(dto->sp+1))+2;
  if((dto->bufend-dto->sp)>=event_max){
    dto->diff=dto->lp-dto->sp;
    DV(D_MEM,printf("naechster Buffer bei %x\n",dto->sp);)
    if(dto->diff>0){
      _schau_nach_speicher(dto);
    }else{
      dto->buffer_free=1;
      PROFILE_END_2(PROF_OUT,dto->xref);
    }
  }else{
    PROFILE_START_2(PROF_OUTWARP,dto->xref);
    *(dto->sp+1)= -1;
    *(dto->sp)=1;
    DV(D_MEM,printf("springe an Anfang (%x)\n",dto->bufbeg);)
    dto->sp=dto->lp=dto->bufbeg;
    dto->diff=0;
    _schau_nach_speicher(dto);
  }
}

/*****************************************************************************/

void _flush_databuf(dto,p)
DATAOUT *dto;
VOLATILE int *p;
{
  T(_flush_databuf)
  PROFILE_START_2(PROF_OUT,dto->xref);
  *p=0;
  *(dto->sp+1)=p-dto->next_databuf;
  *(dto->sp)=1;
  dto->last_databuf=dto->next_databuf;
  DV(D_MEM,printf("Buffer %x gueltig, Laenge %d\n",dto->sp,*(dto->sp+1));)
  beschaffe_speicher(dto);
  dto->next_databuf=(int*)dto->sp+2;
}

/*****************************************************************************/

errcode _get_last_databuf(dto)
DATAOUT *dto;
{
register int *ptr;

T(_get_last_databuf)
ptr=dto->last_databuf;
if (ptr)
  {
  register int len= *(ptr-1);
  while (len--)*outptr++= *ptr++;
  }
else
  return(Err_NoDomain);
return(OK);
}

/*****************************************************************************/
/*****************************************************************************/

