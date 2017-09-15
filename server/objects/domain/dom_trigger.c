/*
 * objects/domain/dom_trigger.c
 * created before 02.02.94
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: dom_trigger.c,v 1.16 2011/04/06 20:30:29 wuestner Exp $";

#include <stdlib.h>
#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "dom_trigger.h"
#include "domtrigobj.h"
#include "../pi/readout.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

triginfo trigdata;
extern ems_u32* outptr;

RCS_REGISTER(cvsid, "objects/domain")

/*****************************************************************************/

errcode dom_trigger_init()
{
T(dom_trigger_init)
trigdata=(triginfo)0;
return(OK);
}

/*****************************************************************************/

errcode dom_trigger_done()
{
T(dom_trigger_done)
if (trigdata!=0) free(trigdata);
trigdata=(triginfo)0;
return(OK);
}

/*****************************************************************************/
/*
downloadtrigger
p[0] : id (ignoriert)
p[1] : Triggerprozedur
p[2] : Parameteranzahl
p[3]... : Parameter
*/
static errcode downloadtrigger(ems_u32* p, unsigned int num)
{
int i;

T(downloadtrigger)
D(D_REQ, printf("downloadtrigger\n");)
if (num<3) return(Err_ArgNum);
if (p[2]!=num-3) return(Err_ArgNum);
if (trigdata!=0) return(Err_DomainDef);
if (readout_active) return(Err_PIActive);
if (!(trigdata=(triginfo)calloc(num-1, sizeof(int)))) return(Err_NoMem);
trigdata->proc=p[1];
for (i=0; i<num-2; i++) trigdata->param[i]=p[i+2];
return(OK);
}

/*****************************************************************************/
/*
uploadtrigger
p[0] : id (ignoriert)
*/
static errcode uploadtrigger(ems_u32* p, unsigned int num)
{
int i;

T(uploadtrigger)
if (num!=1) return(Err_ArgNum);
if (!trigdata) return(Err_NoDomain);
*outptr++=trigdata->proc;
for (i=0; i<=trigdata->param[0]; i++) *outptr++=trigdata->param[i];
return(OK);
}

/*****************************************************************************/
/*
deletetrigger
p[0] : id (ignoriert)
*/
static errcode deletetrigger(ems_u32* p, unsigned int num)
{
T(deletetrigger)
if (num!=1) return(Err_ArgNum);
if (readout_active) return(Err_PIActive);
if (!trigdata) return(Err_NoDomain);
free(trigdata);
trigdata=(triginfo)0;
return(OK);
}

/*****************************************************************************/
/*static domobj *lookup_dom_trig(ems_u32* id, unsigned int idlen, unsigned int* remlen)*/
static objectcommon* lookup_dom_trig(ems_u32* id, unsigned int idlen, unsigned int* remlen)
{
  T(lookup_dom_trig)
  if(idlen>0){
    *remlen=idlen;
    return((objectcommon*)&dom_trig_obj);
  }else{
    *remlen=0;
    return((objectcommon*)&dom_trig_obj);
  }
}

static ems_u32 *dir_dom_trig(ems_u32* ptr)
{
  T(dir_dom_trig)
  if (trigdata) *ptr++=0;
  return(ptr);
}

domobj dom_trig_obj={
  {
    0,0,
    lookup_dom_trig,
    dir_dom_trig,
    0
  },
  downloadtrigger,
  uploadtrigger,
  deletetrigger
};
