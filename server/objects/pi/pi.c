/*
 * objects/pi/pi.c
 * created before 14.04.94
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: pi.c,v 1.19 2011/04/06 20:30:29 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <objecttypes.h>
#include <rcs_ids.h>

#include "../objectcommon.h"
#include "pi.h"
#include "piobj.h"

#ifdef PI_READOUT
#include "pireadoutobj.h"
#include "readout.h"
#endif
#ifdef PI_LAM
#include "lam/pilamobj.h"
#include "lam/lam.h"
#endif

void moncontrol(int);

extern ems_u32* outptr;
int event_max=EVENT_MAX;

RCS_REGISTER(cvsid, "objects/pi")

/*****************************************************************************/

errcode pi_init(void)
{
errcode res;

T(pi_init)
#ifdef PI_READOUT
if ((res=pi_readout_init())!=OK) return(res);
#endif
#ifdef PI_LAM
if ((res=pi_lam_init())!=OK) return(res);
#endif
return(OK);
}

/*****************************************************************************/

errcode pi_done(void)
{
errcode rr, res;

T(pi_done)
res=OK;
#ifdef PI_READOUT
rr=pi_readout_done();
if (res==OK) res=rr;
#endif
#ifdef PI_LAM
rr=pi_lam_done();
if (res==OK) res=rr;
#endif
return(res);
}

/*****************************************************************************/
/* static piobj *lookup_pi(int* id, int idlen, int* remlen) */
static objectcommon* lookup_pi(ems_u32* id, unsigned int idlen,
    unsigned int* remlen)
{
/*
 * int i;
 * printf("lookup_pi({");
 * for (i=0; i<idlen; i++)
 *   printf("%d%s", id[i], i+1<idlen?", ":"");
 * printf("}, %d, %p)\n", idlen, remlen);
 */

T(lookup_pi)
if(idlen>0)
  {
  piobj *obj;
  switch(id[0])
    {
    case Invocation_null:
      obj=&pi_obj;
      break;
#ifdef PI_READOUT
    case Invocation_readout:
      obj= &pi_readout_obj;
      break;
#endif
#ifdef PI_LAM
    case Invocation_LAM:
      obj= &pi_lam_obj;
      break;
#endif
    default:
      return(0);
    }
  /* return((piobj*)obj->common.lookup(id+1, idlen-1, remlen)); */
  return(obj->common.lookup(id+1, idlen-1, remlen));
  }
else
  {
  *remlen=0;
  return((objectcommon*)&pi_obj);
  }
}
/*****************************************************************************/
/*
 CreateProgramInvocation
 p[0] : Invocation-type
 [p[1] : Invocation-id]
 ...
 */
errcode CreateProgramInvocation(ems_u32* p, unsigned int num)
{
  piobj *obj;
  unsigned int remlen;

  T(CreateProgramInvocation)
  D(D_REQ, printf("CreateProgramInvocation\n");)

/*
 * printf("CreateProgramInvocation({)");
 * for (i=0; i<num; i++)
 *   printf("%d%s", p[i], i+1<num?", ":"");
 * printf("}, %d)\n", num);
 */

  obj=(piobj*)lookup_pi(p, num, &remlen);
/*printf("CreateProgramInvocation: remlen=%d\n", remlen);*/
  if(!obj)return(Err_IllPIType);

/*
 * {
 * int* pp=p+num-remlen;
 * int nn=remlen;
 * printf("call obj->create({)");
 * for (i=0; i<nn; i++)
 *   printf("%d%s", pp[i], i+1<nn?", ":"");
 * printf("}, %d)\n", nn);
 * }
 */
  return(obj->create(p+num-remlen, remlen));
}

/*****************************************************************************/
/*
 DeleteProgramInvocation
 p[0] : Invocation-type
 [p[1] : Invocation-id]
 */
errcode DeleteProgramInvocation(ems_u32* p, unsigned int num)
{
  piobj *obj;
  unsigned int remlen;

  T(DeleteProgramInvocation)
  D(D_REQ, printf("DeleteProgramInvocation\n");)

  obj=(piobj*)lookup_pi(p, num, &remlen);
  if(!obj)return(Err_IllPIType);
  return(obj->delete(p+num-remlen, remlen));
}

/*****************************************************************************/
/*
 StartProgramInvocation
 p[0] : Invocation-type
 [p[1] : Invocation-id]
 */
errcode StartProgramInvocation(ems_u32* p, unsigned int num)
{
  errcode res;
  piobj *obj;
  unsigned int remlen;

  T(StartProgramInvocation)
  D(D_REQ, printf("StartProgramInvocation\n");)

  obj=(piobj*)lookup_pi(p, 2, &remlen);
  if(!obj)return(Err_IllPIType);

  res=obj->start(p+2-remlen, num-2+remlen);
  if(res)return(res);
#if 0
  if(num>2){
    char *id;
    xdrstrcdup(&id, p+2);	/* XXXXXXX */
    obj->common.setprot(p+2-remlen, num-2+remlen, id, 0);
    free(id);
  }
#endif
  moncontrol(1);
  return(OK);
}

/*****************************************************************************/
/*
ResetProgramInvocation
p[0] : Invocation-type
[p[1] : Invocation-id]
 */
errcode
ResetProgramInvocation(ems_u32* p, unsigned int num)
{
    piobj *obj;
    unsigned int remlen;

    T(ResetProgramInvocation)
    D(D_REQ, printf("ResetProgramInvocation\n");)

    obj=(piobj*)lookup_pi(p, num, &remlen);
    if (!obj)
        return Err_IllPIType;
#if 0
    if (num>2) {
        char *id;
        int ok;
        xdrstrcdup(&id, p+2);	/* XXXXXXXX */
        setid(id);	/* XXXXXXXX */
        ok=checkperm((objectcommon*)obj, p+num-remlen, remlen, OP_CHSTATUS);
        free(id);
        if(!ok)
            return(Err_NotOwner);
    }
#endif
    moncontrol(0);
    return obj->reset(p+num-remlen, remlen);
}
/*****************************************************************************/
/*
 StopProgramInvocation
 p[0] : Invocation-type
 [p[1] : Invocation-id]
 */
errcode
StopProgramInvocation(ems_u32* p, unsigned int num)
{
    piobj *obj;
    unsigned int remlen;

    T(StopProgramInvocation)
    D(D_REQ, printf("StopProgramInvocation\n");)

    obj=(piobj*)lookup_pi(p, num, &remlen);
    if (!obj)
        return Err_IllPIType;
#if 0
    if (num>2) {
        char *id;
        int ok;
        xdrstrcdup(&id, p+2);	/* XXXXXXXX */
        setid(id);	/* XXXXXXXX */
        ok=checkperm((objectcommon*)obj, p+num-remlen, remlen, OP_CHSTATUS);
        free(id);
        if (!ok)
            return Err_NotOwner;
    }
#endif
    return obj->stop(p+num-remlen, remlen);
}
/*****************************************************************************/
/*
 ResumeProgramInvocation
 p[0] : Invocation-type
 [p[1] : Invocation-id]
 */
errcode
ResumeProgramInvocation(ems_u32* p, unsigned int num)
{
    piobj *obj;
    unsigned int remlen;

    T(ResumeProgramInvocation)
    D(D_REQ, printf("ResumeProgramInvocation\n");)

    obj=(piobj*)lookup_pi(p, num, &remlen);
    if (!obj)
        return Err_IllPIType;
#if 0
    if (num>2) {
        char *id;
        int ok;
        xdrstrcdup(&id, p+2);	/* XXXXXXXX */
        setid(id);	/* XXXXXXXX */
        ok=checkperm((objectcommon*)obj, p+num-remlen, remlen, OP_CHSTATUS);
        free(id);
        if (!ok)
            return Err_NotOwner;
    }
#endif
    return obj->resume(p+num-remlen, remlen);
}
/*****************************************************************************/
/*
 GetProgramInvocationAttr
 p[0] : Invocation-type
 [p[1] : Invocation-id]
 */
errcode
GetProgramInvocationAttr(ems_u32* p, unsigned int num)
{
    piobj *obj;
    unsigned int remlen;

    T(GetProgramInvocationAttr)
    D(D_REQ, printf("GetProgramInvocationAttr\n");)

    obj=(piobj*)lookup_pi(p, num, &remlen);
    if (!obj)
        return Err_IllPIType;

    return obj->getattr(p+num-remlen, remlen);
}
/*****************************************************************************/
/*
 GetProgramInvocationParams
 p[0] : Invocation-type
 [p[1] : Invocation-id]
 */
errcode
GetProgramInvocationParams(ems_u32* p, unsigned int num)
{
    piobj *obj;
    unsigned int remlen;

    T(GetProgramInvocationParams)
    D(D_REQ, printf("GetProgramInvocationParams\n");)

    obj=(piobj*)lookup_pi(p, num, &remlen);
    if (!obj)
        return Err_IllPIType;

    if (obj->getparam!=0)
        return obj->getparam(p+num-remlen, remlen);
    else
    return Err_NotImpl;
}
/*****************************************************************************/
static ems_u32*
dir_pi(ems_u32* ptr)
{
    T(dir_pi)
#ifdef PI_READOUT
    *ptr++=Invocation_readout;
#endif
#ifdef PI_LAM
    *ptr++=Invocation_LAM;
#endif
    return ptr;
}
/*****************************************************************************/
piobj pi_obj={
    {
        pi_init,
        pi_done,
        /*(lookupfunc)*/lookup_pi,
        dir_pi,
        0
    },
    0
};
/*****************************************************************************/
/*****************************************************************************/
