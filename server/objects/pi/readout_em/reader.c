/******************************************************************************
*                                                                             *
* reader.c                                                                    *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 15.04.94                                                                    *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: reader.c,v 1.23 2011/04/06 20:30:29 wuestner Exp $";

/* TriggerID wird nicht getestet!!! */

#include <module.h>
#include <sconf.h>
#include <debug.h>
#include <unsoltypes.h>
#include <errorcodes.h>
#include <errno.h>
#include <rcs_ids.h>
#include "../../domain/dom_datain.h"
#ifdef OSK
#include "../../../lowlevel/vicvsb/vic.h"
#endif
#include "../readout.h"

#ifndef NOVOLATILE
#define VOLATILE
#else
#define VOLATILE volatile
#endif

RCS_REGISTER(cvsid, "objects/pi/readout_em")

#ifdef _PROFILE
#include <utime.h>
static int max_event, min_event, lastevent, sum_words;
static struct timeval lasttime;
static int eventsize;
#endif /* _PROFILE */

/*****************************************************************************/

typedef union
  {
  mod_exec *mod;
  int path;
  } resourceinfo;

extern datain_info datain[MAX_DATAIN];

extern ems_u32* outptr;
ems_u32 eventcnt;
extern int *next_databuf, buffer_free;

static int *buffers[MAX_DATAIN];
static int bufferanz, nochraus, neues_event;
static int letztes_event[MAX_DATAIN];
static VOLATILE int *ip[MAX_DATAIN];
static int es_wartet_noch_jemand;
static resourceinfo resc[MAX_DATAIN];
static int rtp[MAX_DATAIN], space[MAX_DATAIN], rtsp[MAX_DATAIN];
static int use_driver[MAX_DATAIN];
static int poll_driver[MAX_DATAIN], *base[MAX_DATAIN];

/*****************************************************************************/

errcode pi_readout_init()
{
T(pi_readout_init)
readout_active=0;
eventcnt=0;
#ifdef _PROFILE
max_event=0;
min_event=0x7fffffff;
lastevent=eventcnt;
sum_words=0;
#endif /* _PROFILE */
return(OK);
}

/*****************************************************************************/

errcode pi_readout_done()
{
T(pi_readout_done)
return(OK);
}

/*****************************************************************************/
/*
p[0] : object (==Object_pi)
p[1] : invocation (==Invocation_readout)
*/
errcode pi_readout_getnamelist(p, num)
int *p, num;
{
T(pi_readout_getnamelist)
if (num!=2) return(Err_ArgNum);
*outptr++=1;
*outptr++=0;
return(OK);
}


/*****************************************************************************/
/*
createreadout
p[0] : pi-type (==Invocation_readout)
p[1] : id (ignoriert)
*/
errcode createreadout(p, num)
int *p, num;
{
T(createreadout)
if (num!=2) return(Err_ArgNum);
return(Err_ObjDef);
}

/*****************************************************************************/
/*
deletereadout
p[0] : pi-type (==Invocation_readout)
p[1] : id (ignoriert)
*/
errcode deletereadout(p, num)
int *p, num;
{
T(deletereadout)
if (num!=2) return(Err_ArgNum);
return(Err_ObjNonDel);
}

/*****************************************************************************/
/*
getreadoutattr
p[0] : pi-type (==Invocation_readout)
p[1] : id (ignoriert)
*/
errcode getreadoutattr(p, num)
int *p, num;
{
T(getreadoutattr)
if (num!=2) return(Err_ArgNum);
*outptr++=readout_active;
*outptr++=eventcnt;
return(OK);
}

/*****************************************************************************/
/*
getreadoutparams
p[0] : pi-type (==Invocation_readout)
p[1] : id (ignoriert)
*/
errcode getreadoutparams(int* p, int num)
{
T(getreadoutparams)
if (num!=2) return(Err_ArgNum);
/*
*outptr++=readout_active;
*outptr++=eventcnt;
*/
return(OK);
}

/*****************************************************************************/

static errcode datain_init(i)
int i;
{
T(datain_init)
if (datain[i].bufftyp==InOut_Ringbuffer)
  {
  use_driver[bufferanz]=0;
  poll_driver[bufferanz]=0;
  switch(datain[i].addrtyp)
    {
    case Addr_Raw:
      buffers[bufferanz]=datain[i].addr.addr;
      break;
    case Addr_Modul:
      {
      mod_exec* head;
      head=(mod_exec*)modlink(datain[i].addr.modulname,
          mktypelang(MT_DATA, ML_ANY));
      if (head==(mod_exec*)-1)
        {
        *outptr++=errno;
        return(Err_System);
        }
      buffers[bufferanz]=(int*)((char*)head+head->_mexec);
      break;
      }
    case Addr_Driver_syscall:
      poll_driver[bufferanz]=1;
    case Addr_Driver_mixed:
      use_driver[bufferanz]=1;
    case Addr_Driver_mapped:
      {
      int path;
      D(D_REQ, printf("open Driver %s\n", datain[i].addr.driver.path);)
      if ((path=resc[i].path=rtp[bufferanz]=
           open(datain[i].addr.driver.path, 3))==-1)
        {
        *outptr++=errno;
        close(path);
        D(D_REQ, printf("cannot open, errno=%d\n", *(outptr-1));)
        return(Err_System);
        }
      rtsp[bufferanz]=datain[i].addr.driver.space;
      if (poll_driver[bufferanz])
        {
        _setstat(path,ss_setSpace,datain[i].addr.driver.space);
        }
      else
        {
        struct vic_opts x;
        _gs_opt(path,&x);
/*      if (x.NormalTyp!=datain[i].addr.driver.space)
          {
          close(path);
          return(Err_System);
          }*/
/*vorlaeufig, wegen Konfigurierungsproblemen im Watzlawik-Client*/
        }
      if (datain[i].addr.driver.option==1)
        {
        int s;
        if ((s=_getstat(path, ss_CSR, 0))==-1)
          {
          *outptr++=errno;
          close(path);
          D(D_REQ, printf("cannot access vic-slave, errno=%d\n", *(outptr-1));)
          return(Err_System);
          }
        _setstat(path, ss_CSR, 0, s|0x1006);
        }
      if (poll_driver[bufferanz])
        base[bufferanz]=0;
      else
        {
        if ((base[bufferanz]=(int*)_getstat(path, ss_getbase))==(int*)-1)
          {
          *outptr++=errno;
          close(path);
          D(D_REQ, printf("cannot get base address, errno=%d\n", errno);)
          return(Err_System);
          }
        }
      buffers[bufferanz]=
          (int*)((char*)base[bufferanz]+datain[i].addr.driver.offset);
      D(D_REQ, printf("use address 0x%08x\n", buffers[bufferanz]);)
      break;
      }
    default: return(Err_Program);
    }
  letztes_event[bufferanz]=0;
  ip[bufferanz]=buffers[bufferanz];
  bufferanz++;
  }
return(OK);
}

/*****************************************************************************/

static errcode datain_done(i)
int i;
{
T(datain_done)
if (datain[i].bufftyp==InOut_Ringbuffer)
  {
  switch(datain[i].addrtyp)
    {
    case Addr_Raw:
      break;
    case Addr_Modul:
      {
      munlink(resc[i].mod);
      break;
      }
    case Addr_Driver_syscall:
    case Addr_Driver_mixed:
    case Addr_Driver_mapped:
      D(D_REQ, printf("close Driver %d\n", resc[i].path);)
      close(resc[i].path);
      break;
    default: return(Err_Program);
    }
  }
  return(OK);
}

/*****************************************************************************/
/*
startreadout
p[0] : pi-type (==Invocation_readout)
p[1] : id (ignoriert)
*/
errcode startreadout(p, num)
int *p, num;
{
int i;
errcode res;

T(startreadout)
if (num!=2) return(Err_ArgNum);
if (readout_active) return(Err_PIActive);
if (res=start_dataout())
  {
  D(D_REQ, printf("startreadout in reader.c: errcode=%d\n", res);)
  return(res);
  return(res);
  }
bufferanz=0;
res=OK;

for (i=0; (res==OK) && (i<MAX_DATAIN);)
  res=datain_init(i++);

if (res!=OK)
  {
  for (i--; i>0;) datain_done(--i);
  return(res);
  }

es_wartet_noch_jemand=1;
eventcnt=0;
nochraus=bufferanz;neues_event=1;
readout_active=1;
D(D_REQ, printf("READOUT!!\n");)
return(OK);
}

/*****************************************************************************/
/*
resetreadout
p[0] : pi-type (==Invocation_readout)
p[1] : id (ignoriert)
*/
errcode resetreadout(p, num)
int *p, num;
{
T(resetreadout)
if (num!=2) return(Err_ArgNum);
if (readout_active)
  {
  int i;
  stop_dataout();
  for (i=0; i<MAX_DATAIN; i++) datain_done(i);
  D(D_REQ, printf("readout gestoppt!!\n");)
  }
else
  return(Err_PINotActive);
readout_active=0;
eventcnt=0;
return(OK);
}

/*****************************************************************************/

errcode stopreadout(p,num)
int *p, num;
{
T(stopreadout)
if (num!=2) return(Err_ArgNum);
if (readout_active==1)
  readout_active= -1;
else
  return(Err_PINotActive);
return(OK);
}

/*****************************************************************************/

errcode resumereadout(p,num)
int *p, num;
{
T(resumereadout)
if (num!=2) return(Err_ArgNum);
if (readout_active==-1)
  readout_active=1;
else if (readout_active<=0)
  return(Err_PINotActive);
else
  return(Err_PIActive);
return(OK);
}

/*****************************************************************************/
static VOLATILE int *doutptr, *iscnt;

static void gib_weiter(buf, len, evno)
VOLATILE int *buf;
int len, evno;
{
register VOLATILE int *source;
register int i;

T(gib_weiter)
#ifdef DEBUG
if (evno!=(eventcnt+1))
  {
  printf("g.w.: evno %d eventcnt %d\n",evno,eventcnt);
  return;
  }
#endif
if (evno==neues_event)
  {
#ifdef DEBUG
  if (nochraus!=bufferanz)
      printf("nochraus=%d, eventcnt %d, evno %d\n", nochraus, eventcnt, evno);
#endif
  doutptr=next_databuf;
  iscnt=doutptr+2;
  neues_event++;
  source=buf;
  i=len;
  }
else
  {
#ifdef DEBUG
  if (nochraus==bufferanz)
    {
    printf("nochraus=%d, eventcnt %d, evno %d\n", nochraus, eventcnt, evno);
    }
#endif
  DV(D_TRIG, printf("gebe Event %d noch nicht weiter\n", evno);)
  DV(D_MEM, printf("ring: read iscnt from 0x%08x \n", buf+2);)
  *iscnt+=*(buf+2);
  DV(D_MEM, printf("ring: iscnt=%d\n", *iscnt);)
  source=buf+3;
  i=len-3;
  }
DV(D_MEM, printf("ring: copy %d words from 0x%08x to 0x%08x\n", i, source,
    doutptr);)
while (i--) *doutptr++= *source++;

if (!(--nochraus))
  {
  flush_databuf(doutptr);
  nochraus=bufferanz;
  }
}

/*****************************************************************************/

static void gib_weiter_d(path, space, buf, len, evno)
int path, space, *buf, len, evno;
{
register int *source, i;

T(gib_weiter_d)
#ifdef DEBUG
if (evno!=(eventcnt+1))
  {
  printf("g.w.: evno %d eventcnt %d\n",evno,eventcnt);
  return;
  }
#endif
if (evno==neues_event)
  {
#ifdef DEBUG
  if (nochraus!=bufferanz)
  printf("nochraus=%d, eventcnt %d, evno %d\n", nochraus, eventcnt, evno);
#endif
  doutptr=next_databuf;
  iscnt=doutptr+2;
  neues_event++;
  source=buf;
  i=len;
  }
else
  {
#ifdef DEBUG
  if (nochraus==bufferanz)
     printf("nochraus=%d, eventcnt %d, evno %d\n", nochraus, eventcnt, evno);
#endif
  *iscnt+=_getstat(path, ss_singleTransfer, space, buf+2);
  source=buf+3;
  i=len-3;
  }
lseek(path, source, 0);
read(path, doutptr, i*sizeof(int));
doutptr+=i;
if (!(--nochraus))
  {
  flush_databuf(doutptr);
  nochraus=bufferanz;
  }
}

/*****************************************************************************/

static void gib_weiter_m(path, buf, len, offset, evno)
int path;
VOLATILE int *buf;
int len, offset, evno;
{
register VOLATILE int *source;
int i;

T(gib_weiter_m)
#ifdef DEBUG
if (evno!=(eventcnt+1))
  {
  printf("g.w.: evno %d eventcnt %d\n", evno, eventcnt);
  return;
  }
#endif
if (evno==neues_event)
  {
#ifdef DEBUG
  if (nochraus!=bufferanz)
  printf("nochraus=%d, eventcnt %d, evno %d\n", nochraus, eventcnt, evno);
#endif
  doutptr=next_databuf;
  iscnt=doutptr+2;
  neues_event++;
  source=buf;
  i=len;
  }
else
  {
#ifdef DEBUG
  if (nochraus==bufferanz)
  printf("nochraus=%d, eventcnt %d, evno %d\n", nochraus, eventcnt, evno);
#endif
  *iscnt+= *(buf+2);
  source=buf+3;
  i=len-3;
  }
lseek(path, (int*)((int)source-offset), 0);
read(path, doutptr, i*sizeof(int));
doutptr+=i;
if (!(--nochraus))
  {
  flush_databuf(doutptr);
  nochraus=bufferanz;
  }
}

/*****************************************************************************/

void doreadout(int anz)
{
int res;
register int i;

T(read_ringbuffers)
while (anz--)
  {
  if (buffer_free)
    {
    readout_active= -2;
    if (!es_wartet_noch_jemand)
      {
      eventcnt++;
#ifdef _PROFILE
      if (eventcnt>1)
        {
        if (min_event>eventsize) min_event=eventsize;
        if (max_event<eventsize) max_event=eventsize;
        sum_words+=eventsize;
        }
      eventsize=0;
#endif /* _PROFILE */
      }
    es_wartet_noch_jemand=0;
    for (i=0; i<bufferanz; i++)
      {
      if (letztes_event[i]==eventcnt)
        {
        DV(D_MEM, printf("ring %d: read flag from 0x%08x\n", i, ip[i]);)
        if (res=(poll_driver[i]?_getstat(rtp[i], ss_singleTransfer, rtsp[i],
            ip[i]):*(ip[i])))
          {
          int len, evno;
          DV(D_MEM, printf("ring: flag=%d, read len from 0x%08x\n", res,
              ip[i]+1);)
          if (poll_driver[i])
            {
            len=_getstat(rtp[i], ss_singleTransfer, rtsp[i], ip[i]+1);
            DV(D_MEM, printf("ring: len=%d, read evno from 0x%08x\n", len,
                ip[i]+2);)
            evno=_getstat(rtp[i], ss_singleTransfer, rtsp[i], ip[i]+2);
            }
          else
            {
            len= *(ip[i]+1);
            DV(D_MEM, printf("ring: len=%d, read evno from 0x%08x\n", len,
                ip[i]+2);)
            evno= *(ip[i]+2);
            }
          DV(D_MEM, printf("ring: evno=%d\n", evno);)
          if (len>-1)
            {
#ifdef _PROFILE
            eventsize+=len;
#endif /* _PROFILE */
            if (use_driver[i])
              {
              if (poll_driver[i])
                {
                gib_weiter_d(rtp[i], rtsp[i], (int)(ip[i]+2)-(int)base[i],
                    len, evno);
                _setstat(rtp[i], ss_singleTransfer, rtsp[i], ip[i], 0);
                }
              else
                {
                gib_weiter_m(rtp[i], ip[i]+2, len, base[i], evno);
                *(ip[i])=0;
                }
              }
            else
              {
              gib_weiter(ip[i]+2, len, evno);
              *(ip[i])=0;
              }
            DV(D_MEM, printf("ring: wrote flag\n");)
            letztes_event[i]++;
            if ((evno!=letztes_event[i]) && (evno!=0))
              {
              int buf[4];
              printf("Fehler: Buffer %d erwartet %d, bekam %d, eventcnt=%d\n",
                  i, letztes_event[i], evno, eventcnt);
              buf[0]=rtErr_Mismatch;
              buf[1]=eventcnt;
              buf[2]=i;
              buf[3]=evno;
              send_unsolicited(Unsol_RuntimeError,buf,4);
              readout_active= -3;
              return;
              }
            ip[i]+=len+2;
            }
          else
            {
            DV(D_MEM, printf("ring: write flag to 0x%08x\n", ip[i]);)
            if (poll_driver[i])
              _setstat(rtp[i], ss_singleTransfer, rtsp[i], ip[i], 0);
            else
              *(ip[i])=0;
            DV(D_MEM, printf("ring: wrote flag\n");)
            if (len==-2)
              letztes_event[i]=(unsigned int)-1;
            else
              ip[i]=buffers[i];
            es_wartet_noch_jemand=1;
            }
          }
        else
          {
          DV(D_MEM, printf("ring: flag=%d\n", res);)
          es_wartet_noch_jemand=1;
          }
        readout_active=1;
        }
      }
    }
  else
    schau_nach_speicher();
  }
}

/*****************************************************************************/
#ifdef _PROFILE
/*
p[0] : ro|lam (==ro)
p[1] : ID     (belanglos)
p[2] : clear
*/
errcode ProfilePI_Readout(p, num)
int *p, num;
{
struct timeval jetzt;
struct timezone tz;

T(ProfilePI_Readout)
D(D_REQ, printf("ProfilePI_Readout\n");)
gettimeofday(&jetzt, &tz);
*outptr++=Invocation_readout;
*outptr++=jetzt.tv_sec;
*outptr++=lasttime.tv_sec;
*outptr++=min_event;
*outptr++=max_event;
*outptr++=eventcnt-lastevent;
*outptr++=sum_words;
if (p[2])
  {
  max_event=0;
  min_event=0x7fffffff;
  sum_words=0;
  lastevent=eventcnt;
  lasttime.tv_sec=jetzt.tv_sec;
  lasttime.tv_usec=jetzt.tv_usec;
  }
return(OK);
}
#endif /* _PROFILE */
/*****************************************************************************/
/*****************************************************************************/
