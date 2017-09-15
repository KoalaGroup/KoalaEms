/******************************************************************************
*                                                                             *
* interface.c                                                                 *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 22.08.94                                                                    *
*                                                                             *
$ZEL: interface.c,v 1.19 2005/02/11 17:55:43 wuestner Exp $
*                                                                             *
******************************************************************************/

#include <stdio.h>
#include <errno.h>

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <objecttypes.h>

#include "multiringbuffer.h"
#include "ringbuffers.h"
#include "../dataout.h"
#include "../../objects/domain/dom_dataout.h"
#include "../../lowlevel/profile/profile.h"
#include "../../objects/pi/readout.h"

int *next_databuf, buffer_free;

extern ems_u32* outptr;

static DATAOUT *outputs[MAX_DATAOUT];
static int bufferanz,direkt;
static int wieviel[MAX_DATAOUT],xref[MAX_DATAOUT];

/*****************************************************************************/

int printuse_output(FILE* outfilepath)
{
return 0;
}

/*****************************************************************************/

errcode dataout_init(char* args)
{
int i;
T(dataout_init)
bufferanz=0;
direkt= -1;
for(i=0;i<MAX_DATAOUT;i++)xref[i]= -1;
return(OK);
}

errcode dataout_done(){
T(dataout_done)
return(OK);
}

/*****************************************************************************/

static errcode setup_dataouthandler(int i)
{
T(setup_dataouthandler)
if (dataout[i].bufftyp==InOut_Ringbuffer)
  {
  switch (dataout[i].addrtyp)
    {
    case Addr_Raw:
      outputs[bufferanz]=
          init_dataoutraw(dataout[i].addr.addr, dataout[i].buffsize);
      if (!(outputs[bufferanz])) return(Err_System);
      break;
    case Addr_Modul:
      outputs[bufferanz]=
          init_dataoutmod(dataout[i].addr.modulname, dataout[i].buffsize);
      if (!(outputs[bufferanz])) return(Err_System);
      break;
#ifdef OSK
    case Addr_Socket:{
      char tmpname[20];
      int res;

      snprintf(tmpname, 20,"sockout%d.%d",i,getpid());
#ifdef EVENT_BUFBEG
      if(!i)outputs[bufferanz]=
          init_dataoutraw(EVENT_BUFBEG,EVENT_BUFEND-EVENT_BUFBEG);
      else
#endif
        outputs[bufferanz]=init_dataoutmod(tmpname,dataout[i].buffsize);
      if(!(outputs[bufferanz])){
        *outptr++=errno;
        return(Err_System);
      }
      if(res=startsockhndlr(i, dataout[i].addr.inetsock.host,
                             dataout[i].addr.inetsock.port, tmpname)){
#ifdef EVENT_BUFBEG
        if(!i)done_dataoutraw(outputs[bufferanz]);
        else
#endif
          done_dataoutmod(outputs[bufferanz]);
        *outptr++=res;
        return(Err_System);
      }
      break;
      }
    case Addr_LocalSocket:{
      char tmpname[20];
      int res;

      snprintf(tmpname, 20,"locsockout%d.%d",i,getpid());
      outputs[bufferanz]=init_dataoutmod(tmpname,dataout[i].buffsize);
      if (!(outputs[bufferanz])) return(Err_System);
      if(res=startlocalsockhndlr(i, dataout[i].addr.filename, tmpname))
        {
        done_dataoutmod(outputs[bufferanz]);
        *outptr++=res;
        return(Err_System);
        }
      break;
      }
    case Addr_File:{
      char tmpname[20];
      int res;

      snprintf(tmpname, 20,"fileout%d.%d",i,getpid());
      outputs[bufferanz]=init_dataoutmod(tmpname,dataout[i].buffsize);
      if (!(outputs[bufferanz])) return(Err_System);
      if (res=startfilehndlr(i, dataout[i].addr.filename, tmpname))
        {
        done_dataoutmod(outputs[bufferanz]);
        *outptr++=res;
        return(Err_System);
        }
      break;
      }
    case Addr_Tape:{
      char tmpname[20];
      int res;

      snprintf(tmpname, 20,"tapeout%d.%d",i,getpid());
      outputs[bufferanz]=init_dataoutmod(tmpname,dataout[i].buffsize);
      if (!(outputs[bufferanz])) return(Err_System);
      if (res=starttapehndlr(i, dataout[i].addr.filename, tmpname))
        {
        done_dataoutmod(outputs[bufferanz]);
        *outptr++=res;
        return(Err_System);
        }
      break;
      }
    case Addr_Null:{
        char tmpname[20];
        int res;
        snprintf(tmpname, 20,"nullout%d.%d",i,getpid());
        outputs[bufferanz]=init_dataoutmod(tmpname,dataout[i].buffsize);
        if(!(outputs[bufferanz]))return(Err_System);
        if(res=startnullhndlr(i,tmpname)){
            done_dataoutmod(outputs[bufferanz]);
            *outptr++=res;
            return(Err_System);
        }
      break;
      }
#endif /* OSK */
    default:
      return(Err_NoDomain);
    }
  wieviel[bufferanz]=dataout[i].wieviel;
  xref[i]=bufferanz;
#ifdef PROFILE
  outputs[bufferanz]->xref=i;
#endif
  bufferanz++;
  }
else
  return(Err_ArgRange);
return(OK);
}

/*****************************************************************************/

static errcode remove_dataouthandler(int i)
{
T(remove_dataouthandler)
D(D_REQ, printf("remove_dataouthandler(%d)\n", i); sleep(2);)
if (dataout[i].bufftyp==InOut_Ringbuffer)
  {
  switch (dataout[i].addrtyp)
    {
    case Addr_Raw:
      done_dataoutraw(outputs[xref[i]]);
      break;
    case Addr_Modul:
      done_dataoutmod(outputs[xref[i]]);
      break;
#ifdef OSK
    case Addr_Socket:
#ifdef EVENT_BUFBEG
      if(!i)done_dataoutraw(outputs[xref[i]]);
      else
#endif
        done_dataoutmod(outputs[xref[i]]);
      if (stophndlr(i, 10)) return(Err_DataoutBusy);
      D(D_REQ, printf("stophndlr ok\n"); sleep(2);)
      break;
    case Addr_File:
      done_dataoutmod(outputs[xref[i]]);
      if(stophndlr(i,10))return(Err_DataoutBusy);
      break;
    case Addr_Tape:
      done_dataoutmod(outputs[xref[i]]);
      if(stophndlr(i,150))return(Err_DataoutBusy);
      break;
    case Addr_Null:
      done_dataoutmod(outputs[xref[i]]);
      if(stophndlr(i,2))return(Err_DataoutBusy);
      break;
#endif
    default:
      printf("dataout_done: Da war doch ein falscher Adresstyp.\n");
    }
    return(OK);
  }
else
  return(Err_Program);
}

/*****************************************************************************/

errcode start_dataout()
{
int i;

T(start_dataout)
direkt= -1;
for (i=0; i<bufferanz; i++)
  {
  if ((!(wieviel[i]))&&(direkt==-1))
    {
    direkt=i;
    D(D_MEM, printf("direkter buffer ist %d\n", i);)
    }
  }
if (direkt==-1)
  {
  D(D_REQ, printf("start_dataout errcode=Err_NoDomain\n");)
  return(Err_NoDomain);
  }
for (i=0; i<bufferanz; i++)
  {
  _init_dataout(outputs[i]);
  }
next_databuf=outputs[direkt]->next_databuf;
D(D_MEM, printf("init_dataout: %d buffer,  erste Adresse %x\n",
    bufferanz, next_databuf);)
#ifdef OSK
for (i=0; i<MAX_DATAOUT; i++)
  {
  if (dataout[i].bufftyp==InOut_Ringbuffer)
    {
    switch (dataout[i].addrtyp)
      {
      case Addr_Socket:
      case Addr_File:
      case Addr_Tape:
      case Addr_Null:
        if (starthndlr(i))
          {
          D(D_REQ, printf("start_dataout errcode=Err_DataoutBusy\n");)
          return(Err_DataoutBusy);
          }
      }
    }
  }
#endif
buffer_free=1;
D(D_REQ, printf("start_dataout OK\n");)
return(OK);
}

/*****************************************************************************/

errcode stop_dataout()
{
int i;

T(stop_dataout)
for (i=0; i<bufferanz; i++) _done_dataout(outputs[i]);
return(OK);
}

/*****************************************************************************/

errcode insert_dataout(i)
int i;
{
    int res;

    T(insert_dataout)
    if(readout_active&&!dataout[i].wieviel)return(Err_PIActive);
    if(res=setup_dataouthandler(i))return(res);
#ifdef OSK
    if (readout_active){
        if (dataout[i].bufftyp==InOut_Ringbuffer)
        {
            switch (dataout[i].addrtyp)
            {
                case Addr_Socket:
                case Addr_File:
                case Addr_Tape:
                case Addr_Null:
                    if(starthndlr(i))return(Err_DataoutBusy);
            }
        }
    }
#endif
    return(OK);
}

/*****************************************************************************/

errcode remove_dataout(i)
int i;
{
errcode res;
int j;

T(remove_dataout)
if(readout_active&&!dataout[i].wieviel)return(Err_PIActive);
if(res=remove_dataouthandler(i))return(res);

bufferanz--;

if(xref[i]!=bufferanz){
    outputs[xref[i]]=outputs[bufferanz];
    wieviel[xref[i]]=wieviel[bufferanz];
    for(j=0;j<MAX_DATAOUT;j++)if(xref[j]==bufferanz){
        D(D_REQ,printf("Dataout %d rueckt nach\n",j);)
        xref[j]=xref[i];
    }
}
if(xref[i]==direkt){
    D(D_REQ,printf("\"direkt\" wird ungueltig\n");)
    direkt= -1;
}
xref[i]= -1;
return(OK);
}

/*****************************************************************************/

#ifdef OSK
int* get_dataout_addr(int i)
{
    return(_get_dataout_addr(outputs[xref[i]]));
}
#endif

/*****************************************************************************/
void schau_nach_speicher()
{
int i;

T(schau_nach_speicher)
buffer_free=1;
for (i=0; i<bufferanz; i++)
  {
  if (!(outputs[i]->buffer_free))
    {
    _schau_nach_speicher(outputs[i]);
    if((!(outputs[i]->buffer_free))&&(wieviel[i]>=0)) buffer_free=0;
    }
  }
#ifdef PROFILE
if(buffer_free)PROFILE_END(PROF_OUTALL);
#endif
}

void flush_databuf(p)
int *p;
{
int i;

T(flush_databuf)
PROFILE_START(PROF_OUTALL);
for(i=0;i<bufferanz;i++){
    if (outputs[i]->buffer_free){
        if((i!=direkt)&&(wieviel[i]>=-1)){
            memcpy(outputs[i]->next_databuf, next_databuf,
                   (p-next_databuf)*sizeof(int));
            _flush_databuf(outputs[i],
                           outputs[i]->next_databuf+(p-next_databuf));
            if((wieviel[i]>0)&&(!(--wieviel[i])))wieviel[i]= -2;
        }
    }
}
/*if(!wieviel[direkt]){*/
    _flush_databuf(outputs[direkt],p);
    next_databuf=outputs[direkt]->next_databuf;
/*}*/
buffer_free=1;
for (i=0; i<bufferanz; i++){
    if(!(outputs[i]->buffer_free)){
        if(wieviel[i]>=0)
            buffer_free=0;
        else
            _schau_nach_speicher(outputs[i]);
    }
}
#ifdef PROFILE
if(buffer_free)PROFILE_END(PROF_OUTALL);
#endif
D(D_MEM, printf("next_databuf ist %x\n", next_databuf);)
}

/*****************************************************************************/

errcode get_last_databuf()
{
T(get_last_databuf)
if (direkt==-1) return(Err_NoDomain);
return(_get_last_databuf(outputs[direkt]));
}
