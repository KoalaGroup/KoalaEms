/*
 * objects/domain/dom_lam.c
 * created before 10.02.94
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: dom_lam.c,v 1.15 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdlib.h>
#include <unistd.h>
#include <errorcodes.h>
#include <rcs_ids.h>

#include "../pi/lam/lam.h"
#include "domlamobj.h"
#include "dom_lam.h"

#ifdef DMALLOC
#include dmalloc.h
#endif

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "objects/domain")

/*****************************************************************************/

errcode dom_lam_init()
{
T(dom_lam_init)
return(OK);
}

/*****************************************************************************/

errcode dom_lam_done()
{
/*
 * all necessary action should be in pi_lam_done
 */
#if 0
int i;
struct LAM *lam;

T(dom_lam_done)
for (i=0; i<MAX_LAM; i++)
  {
  lam=lam_list[i];
  if (lam!=0)
    {
    if (lam->active)
      {
      printf("Fehler in dom_lam_done: lam %d aktiv!!\n", i);
      return(-1);
      }
    if (lam->list!=0) free(lam->list);
    lam->list=(ems_u32*)0;
    lam->listlen=0;
    lam->output=0;
    }
  }
#endif
return(OK);
}

/*****************************************************************************/
/*
 downloadlamlist
 p[0] : Domain-id (=LAM-Index)
 p[1] : Ausgabemodus (unsolicited Message oder nicht)
 p[2] : Liste
 ...
 */
errcode downloadlamlist(ems_u32* p, unsigned int num)
{
    int i, len;
    struct LAM *lam;

    T(downloadlamlist)

    if (num<3)
        return Err_ArgNum;
    if ((lam=get_lam(p[0]))==0)
        return Err_NoPI;

    if (lam->trinfo.state!=trigger_idle)
        return Err_PIActive;

    /* delete old list */
    if (lam->proclist)
        free(lam->proclist);
    lam->proclistlen=0;

    /* allocate new list */
    len=num-2;
    if (!(lam->proclist=calloc(len, sizeof(ems_u32))))
        return Err_NoMem;
    lam->proclistlen=len;
    for (i=0; i<len; i++)
        lam->proclist[i]=p[i+2];
    lam->output=p[1];
    return OK;
}
/*****************************************************************************/
/*
uploadlamlist
p[0] : Domain-id
*/
errcode uploadlamlist(ems_u32* p, unsigned int num)
{
    int i;
    ems_u32 *ptr;
    struct LAM *lam;

    T(uploadlamlist)

    if (num!=1)
        return Err_ArgNum;
    if ((lam=get_lam(p[0]))==0)
        return Err_NoPI;

    if (!(ptr=lam->proclist))
        return Err_NoDomain;
    *outptr++=lam->output;
    i=lam->proclistlen;
    while (i--)
        *outptr++= *ptr++;
   return OK;
}
/*****************************************************************************/
/*
deletelamlist
p[0] : Domain-id
*/
errcode deletelamlist(ems_u32* p, unsigned int num)
{
    struct LAM *lam;

    T(deletelamlist)
    if (num!=1)
        return Err_ArgNum;
    if ((lam=get_lam(p[0]))==0)
        return Err_NoPI;

    if (lam->trinfo.state!=trigger_idle)
        return Err_PIActive;
    if (!(lam->proclist))
        return Err_NoDomain;
    free(lam->proclist);
    lam->proclist=0;
    lam->proclistlen=0;
    lam->output=0;
    return OK;
}
/*****************************************************************************/
static objectcommon* lookup_dom_lam(__attribute__((unused)) ems_u32* id,
    unsigned int idlen, unsigned int* remlen)
{
  T(lookup_dom_lam)
  if(idlen>0){
    *remlen=idlen;
    return((objectcommon*)&dom_lam_obj);
  }else{
    *remlen=0;
    return((objectcommon*)&dom_lam_obj);
  }
}
/*****************************************************************************/
static ems_u32 *dir_dom_lam(ems_u32* ptr)
{
  T(dir_dom_lam)
  return(ptr);
}
/*****************************************************************************/
domobj dom_lam_obj={
  {
    0,0,
    lookup_dom_lam,
    dir_dom_lam,
    0
  },
  downloadlamlist,
  uploadlamlist,
  deletelamlist
};
/*****************************************************************************/
/*****************************************************************************/
