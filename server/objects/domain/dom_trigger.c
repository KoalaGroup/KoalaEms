/*
 * objects/domain/dom_trigger.c
 * created before 02.02.94
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: dom_trigger.c,v 1.18 2017/10/20 23:21:31 wuestner Exp $";

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

struct triginfo *triginfo;
extern ems_u32* outptr;

RCS_REGISTER(cvsid, "objects/domain")

/*****************************************************************************/
errcode
dom_trigger_init(void)
{
    T(dom_trigger_init)
    triginfo=0;
    return OK;
}
/*****************************************************************************/
errcode
dom_trigger_done(void)
{
    T(dom_trigger_done)

    while (triginfo) {
        struct triginfo *help=triginfo;
        triginfo=triginfo->next;
        free(help->trinfo);
        free(help);
    }
    return OK;
}
/*****************************************************************************/
/*
downloadtrigger
p[0] : id
p[1] : Triggerprozedur
p[2] : Parameteranzahl
p[3]... : Parameter
*/
static errcode
downloadtrigger(ems_u32* p, unsigned int num)
{
    struct triginfo *info;
    unsigned int i;

    T(downloadtrigger)
    D(D_REQ, printf("downloadtrigger\n");)

    if (num<3)
        return Err_ArgNum;
    if (p[2]!=num-3)
        return Err_ArgNum;

printf("downloadtrigger: idx=%d\n", p[0]);

    info=triginfo;
    while (info) {
        if (info->idx==p[0])
            return Err_DomainDef;
        info=info->next;
    }

    if (readout_active)
        return Err_PIActive;

    info=calloc(num-1, sizeof(struct triginfo)+p[2]*sizeof(ems_u32));
    if (!info)
        return Err_NoMem;

    info->idx=p[0];
    info->proc=p[1];
    for (i=0; i<=p[2]; i++)
        info->param[i]=p[i+2];
    info->prev=0;
    info->next=triginfo;
    triginfo=info;

    return OK;
}
/*****************************************************************************/
/*
uploadtrigger
p[0] : id
*/
static errcode
uploadtrigger(ems_u32* p, unsigned int num)
{
    struct triginfo *info=triginfo;
    unsigned int i;

    T(uploadtrigger)
    if (num!=1)
        return Err_ArgNum;

printf("uploadtrigger: idx=%d, triginfo=%p\n", p[0], triginfo);
    while (info) {
        printf("info=%p idx=%d\n", info, info->idx);
        if (info->idx==p[0])
            break;
        info=info->next;
    };
    if (!info)
        return Err_NoDomain;

    *outptr++=info->proc;
    for (i=0; i<=info->param[0]; i++)
        *outptr++=info->param[i];

    return OK;
}
/*****************************************************************************/
static void
unlink_trigger(struct triginfo *info)
{
    if (info->next)
        info->next->prev=info->prev;
    if (info->prev)
        info->prev->next=info->next;
    else
        triginfo=info->next;
    free(info->trinfo);
    free(info);
}

/*
deletetrigger
p[0] : idx
*/
static errcode
deletetrigger(ems_u32* p, unsigned int num)
{
    struct triginfo *info=triginfo;

    T(deletetrigger)
    if (num!=1)
        return Err_ArgNum;
    if (readout_active)
        return Err_PIActive;

printf("deletetrigger: idx=%d, triginfo=%p\n", p[0], triginfo);
    while (info) {
        printf("info=%p idx=%d\n", info, info->idx);
        if (info->idx==p[0]) {
            unlink_trigger(info);
            return OK;
        }
        info=info->next;
    };

    return Err_NoDomain;
}
/*****************************************************************************/
/*static domobj *lookup_dom_trig(ems_u32* id, unsigned int idlen,
    unsigned int* remlen)*/
static objectcommon* lookup_dom_trig(__attribute__((unused)) ems_u32* id,
    unsigned int idlen, unsigned int* remlen)
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

static ems_u32*
dir_dom_trig(ems_u32* ptr)
{
    struct triginfo *info=triginfo;

    T(dir_dom_trig)
    while (info) {
        *ptr++=info->idx;
        info=info->next;
    };
    return ptr;
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
/*****************************************************************************/
/*****************************************************************************/
