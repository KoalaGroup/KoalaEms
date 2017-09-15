/*
 * objects/pi/readout_em/piops.c
 * created before 02.08.94
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: piops.c,v 1.17 2011/04/06 20:30:29 wuestner Exp $";

#include <errorcodes.h>
#include <sconf.h>
#include <debug.h>
#include <xdrstring.h>
#include <objecttypes.h>
#include <strings.h>
#include <stdlib.h>
#ifdef OSK
#include <strdup.h>
#endif
#include <rcs_ids.h>
#include "../readout.h"

#include "../../../dataout/dataout.h"
#include "../../../main/scheduler.h"
#include "../../../main/nowstr.h"

#include "../pireadoutobj.h"
#include "datains.h"
#include "readevents.h"

RCS_REGISTER(cvsid, "objects/pi/readout_em")

extern ems_u32* outptr;
extern ems_u32 eventcnt;
extern int bufferanz;

static taskdescr *t;

InvocStatus readout_active;

static char *owner;
static int perms[]={
  2,
  0,
  OP_CHSTATUS
};

/*****************************************************************************/

errcode pi_readout_init()
{
  readout_active=Invoc_notactive;
  eventcnt=0;
  owner=0;
  t=0;
  return(OK);
}

/*****************************************************************************/

errcode pi_readout_done()
{
    return(OK);
}

/*****************************************************************************/

static errcode createreadout(int* p, int num)
{
    if(num!=1)return(Err_ArgNum);
    return(Err_ObjDef);
}

/*****************************************************************************/

static errcode deletereadout(int* p, int num)
{
    if(num!=1)return(Err_ArgNum);
    return(Err_ObjNonDel);
}

/*****************************************************************************/

static errcode getreadoutattr(int* p, int num)
{
    if(num!=1)return(Err_ArgNum);
    *outptr++=readout_active;
    *outptr++=eventcnt;
    return(OK);
}

/*****************************************************************************/

static errcode startreadout(int* p, int num)
{
int i;
errcode res;

T(startreadout)

printf("%s startreadout; readout_active=%d\n", nowstr(), readout_active);
if (readout_active) return(Err_PIActive);

if (res=start_dataout())
  {
  D(D_REQ, printf("startreadout in piops.c: errcode=%d (start_dataout)\n", res);)
  return(res);
  }
bufferanz=0;
res=OK;
for (i=0;(!res)&&(i<MAX_DATAIN);) res=datain_init(i++);
if (res)
  {
  for (i--;i>0;) datain_done(--i);
  D(D_REQ, printf("startreadout in piops.c: errcode=%d (datain_init)\n", res);)
  return(res);
  }
if(!bufferanz)return(Err_NoDomain);
initevreader();
t=sched_insert_poll_task(doreadout,0,200,0,"readout");
readout_active=Invoc_active;
D(D_REQ, printf("startreadout in piops.c: OK\n");)
return(OK);
}

/*****************************************************************************/

static errcode resetreadout(int* p, int num)
{
  int i;

  if(num<1) return(Err_ArgNum);
  printf("%s resetreadout; readout_active=%d\n", nowstr(), readout_active);
  if(readout_active==Invoc_notactive) return(Err_PINotActive);

  if (t!=0) sched_remove_task(t); /* kann ja schon gestoppt sein */
  t=0;
  stop_dataout();
  for(i=0;i<MAX_DATAIN;i++)datain_done(i);
  readout_active=Invoc_notactive;
  eventcnt=0;
  return(OK);
}

/*****************************************************************************/

static errcode stopreadout(int* p, int num)
{
  if(num<1) return(Err_ArgNum);
  printf("%s stopreadout; readout_active=%d\n", nowstr(), readout_active);
  if(readout_active!=Invoc_active) return(Err_PINotActive);

  sched_remove_task(t);
  t=0;

  readout_active=Invoc_stopped;
  return(OK);
}

/*****************************************************************************/

static errcode resumereadout(int* p, int num)
{
  if (num<1) return(Err_ArgNum);

  printf("%s resumereadout; readout_active=%d\n", nowstr(), readout_active);
  if(readout_active!=Invoc_stopped){
    if(readout_active==Invoc_active)
      return(Err_PIActive);
    else
      return(Err_PINotActive);
  }

  t=sched_insert_poll_task(doreadout,0,200,0,"readout");

  readout_active=Invoc_active;
  return(OK);
}

/*****************************************************************************/
/* static piobj *lookup_pi_readout(int* id, int idlen, int* remlen) */
static objectcommon* lookup_pi_readout(int* id, int idlen, int* remlen)
{
  T(lookup_pi_readout)
  if(idlen>0){
    if(id[0]!=0)return(0);
    *remlen=idlen;
    return((objectcommon*)&pi_readout_obj);
  }else{
    *remlen=0;
    return((objectcommon*)&pi_readout_obj);
  }
}

/*****************************************************************************/

static int *dir_pi_readout(int* ptr)
{
printf("dir_pi_readout\n");
*ptr++=0;
return(ptr);
}

/*****************************************************************************/

static int setprot_pi_readout(int* idx, int idxlen, char* newowner, int* newperm)
{
  T(setprot_pi_readout)
  if(newperm)return(-1);
  if(newowner){
    if(owner)free(owner);
    owner=strdup(newowner);
  }
  return(0);
}

/*****************************************************************************/

static int getprot_pi_readout(int* idx, int idxlen, char** ownerp, int** permp)
{
  T(getprot_pi_readout)
  if(ownerp)*ownerp=owner;
  if(permp)*permp=perms;
  return(0);
}

/*****************************************************************************/

piobj pi_readout_obj={
  {
    pi_readout_init,
    pi_readout_done,
    lookup_pi_readout,
    dir_pi_readout,
    0,	/* ref */
    setprot_pi_readout,
    getprot_pi_readout
  },
  createreadout,
  deletereadout,
  startreadout,
  resetreadout,
  stopreadout,
  resumereadout,
  getreadoutattr,
  0
};

/*****************************************************************************/
/*****************************************************************************/
