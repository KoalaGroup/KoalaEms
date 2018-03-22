/*
 * objects/domain/domain.c
 * created before 30.05.94
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: domain.c,v 1.17 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <cdefs.h>
#include <objecttypes.h>
#include <errorcodes.h>
#include <actiontypes.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
#include "../notifystatus.h"

#include "domain.h"
#include "domobj.h"
#ifdef DOM_ML
#include "dommlobj.h"
#include "dom_ml.h"
#endif
#ifdef DOM_LAM
#include "domlamobj.h"
#include "dom_lam.h"
#endif
#ifdef DOM_TRIGGER
#include "domtrigobj.h"
#include "dom_trigger.h"
#endif
#ifdef DOM_EVENT
#include "domevobj.h"
#include "dom_event.h"
#endif
#ifdef DOM_DATAOUT
#include "domdoobj.h"
#include "dom_dataout.h"
#endif
#ifdef DOM_DATAIN
#include "domdiobj.h"
#include "dom_datain.h"
#endif

RCS_REGISTER(cvsid, "objects/domain")

/*****************************************************************************/

/* domobj *lookup_dom __P((int*, int, int*)); */

extern ems_u32* outptr;

errcode domain_init()
{
errcode res;

T(domain_init)
#ifdef DOM_ML
if ((res=dom_ml_init())!=OK) return(res);
#endif /* OBJ_IS */
#ifdef DOM_LAM
if ((res=dom_lam_init())!=OK) return(res);
#endif /* PI_LAM */
#ifdef DOM_TRIGGER
if ((res=dom_trigger_init())!=OK) return(res);
#endif /* TRIGGER */
#ifdef DOM_EVENT
if ((res=dom_event_init())!=OK) return(res);
#endif
#ifdef DOM_DATAOUT
if ((res=dom_dataout_init())!=OK) return(res);
#endif /* PI_READOUT */
#ifdef DOM_DATAIN
if ((res=dom_datain_init())!=OK) return(res);
#endif /* DATAIN */
return(OK);
}

/*****************************************************************************/

errcode domain_done()
{
errcode rr, res;

T(domain_done)
res=OK;
#ifdef DOM_ML
rr=dom_ml_done();
if (res==OK) res=rr;
#endif /* OBJ_IS */
#ifdef DOM_LAM
rr=dom_lam_done();
if (res==OK) res=rr;
#endif /* PI_LAM */
#ifdef DOM_TRIGGER
rr=dom_trigger_done();
if (res==OK) res=rr;
#endif /* TRIGGER */
#ifdef DOM_EVENT
rr=dom_event_done();
if (res==OK) res=rr;
#endif
#ifdef DOM_DATAOUT
rr=dom_dataout_done();
if (res==OK) res=rr;
#endif /* PI_READOUT */
#ifdef DOM_DATAIN
rr=dom_datain_done();
if (res==OK) res=rr;
#endif /* DATAIN */
return(res);
}

/*****************************************************************************/
/* domobj *lookup_dom(int* id, int idlen, int* remlen) */
static objectcommon* lookup_dom(ems_u32* id, unsigned int idlen,
    unsigned int* remlen)
{
  T(lookup_dom)
  if(idlen>0){
    domobj *obj;
    switch(id[0]){
      case Dom_null:
        obj= &dom_obj;
        break;
#ifdef DOM_ML
      case Dom_Modullist:
	obj= &dom_ml_obj;
	break;
#endif
#ifdef DOM_LAM
      case Dom_LAMproclist:
	obj= &dom_lam_obj;
	break;
#endif
#ifdef DOM_TRIGGER
      case Dom_Trigger:
	obj= &dom_trig_obj;
	break;
#endif
#ifdef DOM_EVENT
      case Dom_Event:
	obj= (domobj*)&dom_ev_obj;
	break;
#endif
#ifdef DOM_DATAOUT
      case Dom_Dataout:
	obj= &dom_do_obj;
	break;
#endif
#ifdef DOM_DATAIN
      case Dom_Datain:
	obj= &dom_di_obj;
	break;
#endif
      default:
	return(0);
    }
    return((obj->common.lookup(id+1, idlen-1, remlen)));
  }else
    {
    *remlen=0;
    return((objectcommon*)&dom_obj);
    }
}
/*****************************************************************************/
/*
 DownloadDomain
 p[0] : Domain-type
 p[1] : Domain-id
 p[2] : daten
 ...
 */
errcode DownloadDomain(ems_u32* p, unsigned int num)
{
domobj *obj;
unsigned int remlen=12345678;
errcode res;

T(DownloadDomain)
if (num<2) return(Err_ArgNum);
D(D_REQ, printf("DownloadDomain typ %d\n", p[0]);)

obj=(domobj*)lookup_dom(p, num, &remlen);
if (!obj)
  {
  num-=remlen;
  obj=(domobj*)lookup_dom(p, num, &remlen);
  if (!obj)
    return Err_IllDomainType;
  else
    return Err_IllDomain;
  }
res=obj->download(p+num-remlen, remlen);
if (res==OK) notifystatus(status_action_create, Object_domain, num-remlen+1, p);
return res;
}

/*****************************************************************************/
/*
UploadDomain
p[0] : Domain-type
p[1] : Domain_id
*/
errcode
UploadDomain(ems_u32* p, unsigned int num)
{
    domobj *obj;
    unsigned int remlen;

    T(UploadDomain)

    obj=(domobj*)lookup_dom(p, num, &remlen);
    if (!obj) {
        num--;
        obj=(domobj*)lookup_dom(p, num, &remlen);
        if (!obj)
            return Err_IllDomainType;
        else
            return Err_NoDomain;
    }
    return obj->upload(p+num-remlen, remlen);
}
/*****************************************************************************/
/*
DeleteDomain
p[0] : Domain-type
p[1] : Domain-id
*/
errcode DeleteDomain(ems_u32* p, unsigned int num)
{
domobj *obj;
unsigned int remlen;
errcode res;

T(DeleteDomain)
if (num<2) return(Err_ArgNum);
D(D_REQ,
  unsigned int i;
  printf("DeleteDomain(");
  for (i=0; i<num; i++) printf("%d%s", p[i], i+1<num?", ":")\n");
  )
obj=(domobj*)lookup_dom(p, num, &remlen);
if (!obj)
  {
  num-=remlen;
  obj=(domobj*)lookup_dom(p, num, &remlen);
  if (!obj)
    return Err_IllDomainType;
  else
    return Err_IllDomain;
  }

res=obj->delete(p+num-remlen, remlen);
if (res==OK) notifystatus(status_action_delete, Object_domain, num-remlen+1, p);
return res;
}
/*****************************************************************************/
static ems_u32 *dir_dom(ems_u32* ptr)
{
  T(dir_dom)
#ifdef DOM_ML
  *ptr++=Dom_Modullist;
#endif
#ifdef DOM_LAM
  *ptr++=Dom_LAMproclist;
#endif
#ifdef DOM_TRIGGER
  *ptr++=Dom_Trigger;
#endif
#ifdef DOM_EVENT
  *ptr++=Dom_Event;
#endif
#ifdef DOM_DATAOUT
  *ptr++=Dom_Dataout;
#endif
#ifdef DOM_DATAIN
  *ptr++=Dom_Datain;
#endif
  return(ptr);
}
/*****************************************************************************/
domobj dom_obj={
  {
    0,0,
    /*(lookupfunc)*/lookup_dom,
    dir_dom,
    0
  },
  0, 0, 0
};
/*****************************************************************************/
/*****************************************************************************/

