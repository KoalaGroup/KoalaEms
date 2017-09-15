/*
 * objects/pi/lam/lam.c
 * created before 22.09.93
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lam.c,v 1.21 2011/04/06 20:30:29 wuestner Exp $";

/*
 * LAMs are triggered by an arbitrary trigger procedure.
 * If triggered a procedure list in the context of an IS (or IS 0) is 
 * executed. The output of this procedure list can be sent to the data stream
 * (if readout is active), it can generate an unsolicited message or can be
 * ignored.
 */

#include <sconf.h>
#include <debug.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <errorcodes.h>
#include <objecttypes.h>
#include <unsoltypes.h>
#include <xdrstring.h>
#include <rcs_ids.h>

#include "lam.h"
#include "../../is/is.h"
#include "../../../main/scheduler.h"
#include "../../../commu/commu.h"
#include "../../../procs/proclist.h"

#include "pilamobj.h"

#ifdef DMALLOC
#include dmalloc.h
#endif

extern ems_u32 *outptr, *outbuf;
struct LAM *lam_list=0; /* linked list */

extern int *memberlist;

RCS_REGISTER(cvsid, "objects/pi/lam")

/*****************************************************************************/
static void
free_lam(struct LAM *lam)
{
    if (lam==lam_list) {
        lam_list=lam->next;
    } else {
        lam->prev->next=lam->next;
    }
    if (lam->next)
        lam->next->prev=lam->prev;
    free(lam->trigargs);
    free(lam->proclist);
    free(lam);
}
/*****************************************************************************/
static void
insert_lam(struct LAM *lam)
{
    if (lam_list)
        lam_list->prev=lam;
    lam->prev=0;
    lam->next=lam_list;
    lam_list=lam;
}
/*****************************************************************************/
struct LAM*
get_lam(int idx)
{
    struct LAM *lam=lam_list;
    while (lam && lam->idx!=idx)
        lam=lam->next;
    return lam;
}
/*****************************************************************************/
/*
createlam
p[0] : LAM-Index
p[1] : LAM-id (for unsol. messages)
p[2] : instrumentation system (or 0)
p[3] : index of the trigger procedure
p[4...] : arguments of the trigger procedure
*/
errcode createlam(ems_u32* p, unsigned int num)
{
    struct LAM *lam;
    int i;

    /* check number of arguments */
    if (num<5 || p[4]!=num-5)
        return Err_ArgNum;

    /* index already in use? */
    if (get_lam(p[0]))
        return Err_PIDef;

    lam=calloc(1, sizeof(struct LAM));
    if (!lam)
        return Err_NoMem;
    if (p[4]) {
        lam->trigargs=malloc((p[4]+1)*sizeof(ems_u32));
        if (!lam->trigargs) {
            free(lam);
            return Err_NoMem;
        }
    } else {
        lam->trigargs=0;
    }

    lam->idx=p[0];
    lam->id=p[1];
    lam->is=p[2];
    lam->output=0;
    lam->cnt=0;
    lam->trigproc=p[3];
    for (i=0; i<=p[4]; i++)
        lam->trigargs[i]=p[4+i];
    lam->proclistlen=0;
    lam->proclist=0;
    lam->trinfo.state=trigger_idle;
    lam->trinfo.tinfo=0;

    insert_lam(lam);

    return plOK;
}
/*****************************************************************************/
/*
deletelam
p[0] : LAM-Index
*/
errcode deletelam(ems_u32* p, unsigned int num)
{
    struct LAM *lam;

    T(deletelam)
    if (num!=1)
        return Err_ArgNum;
    if ((lam=get_lam(p[0]))==0)
        return Err_NoPI;

    if (lam->trinfo.tinfo)
        return Err_PIActive;
    free_lam(lam);
    return OK;
}
/*****************************************************************************/
static void lam_callback(void* data)
{
    struct LAM* lam=(struct LAM*)data;
    ems_u32* help;
    errcode res;

printf("objects/pi/lam/lam.c::lam_callback called\n");
    if (is_list[lam->is])
        memberlist=is_list[lam->is]->members;
    else
        memberlist=0;

    if (lam->output) {
        *outptr++=lam->id;
        res=scan_proclist(lam->proclist);
    } else {
        help=outptr;
        res=scan_proclist(lam->proclist);
        outptr=help; /* restore old outptr; no output is generated */
    }
    lam->cnt++;
    if (res!=OK) {
        printf("lam %d: error %d\n", lam->idx, res);
        /* XXX hier fehlt weitere Behandlung (z.B. dump von outptr) */
        suspend_trigger_task(&lam->trinfo);
    }

    /* XXX lam->id should be inserted here */
    if (outptr!=outbuf)
        send_unsolicited(Unsol_LAM, outbuf, outptr-outbuf);
}
/*****************************************************************************/
static errcode test_lam_proc(struct LAM *lam)
{
    errcode res;
    int limit=0;

    if (!lam->proclist)
        return Err_NoReadoutList;

    if (is_list[lam->is])
        memberlist=is_list[lam->is]->members;
    else
        memberlist=0;

    res=test_proclist(lam->proclist, lam->proclistlen, &limit);
    printf("test_lam_proc: res=%d limit=%d\n", res, limit);
    return res;
}
/*****************************************************************************/
/*
startlam
p[0] : LAM-Index
p[1...]: string owner (obsolete and to be deleted)
*/
errcode
startlam(ems_u32* p, unsigned int num)
{
    struct LAM *lam;
    errcode res;

    T(startlam)
printf("startlam: num=%d p[0]=%d p[1]=%d\n", num, p[0], p[1]);

    if (num<1)
        return Err_ArgNum;
    /* XXX to be deleted after removing owner from all requests */
    if (num>0) {
        char* owner;
        xdrstrcdup(&owner, p+1);
        printf("startlam: request contains 'owner': %s\n", owner);
        free(owner);
        num=1;
    }

    if (num!=1)
        return Err_ArgNum;
    if ((lam=get_lam(p[0]))==0)
        return Err_NoPI;
    if (lam->trinfo.tinfo)
        return Err_PIActive;

    res=test_lam_proc(lam);
    if (res!=OK)
        return res;

    res=init_trigger(&lam->trinfo, lam->trigproc, lam->trigargs);
    if (res) {
        printf("startlam[%d]: init_trigger failed\n", p[0]);
        return res;
    }

    lam->trinfo.cb_proc=lam_callback;
    lam->trinfo.cb_data=lam;
    res=insert_trigger_task(&lam->trinfo);
    if (res) {
        printf("startlam[%d]: insert_trigger failed\n", p[0]);
        return res;
    }

    return OK;
}
/*****************************************************************************/
/*
stoplam
p[0] : LAM-Index
p[1...]: string owner (obsolete and to be deleted)
*/
errcode resetlam(ems_u32* p, unsigned int num)
{
    struct LAM *lam;
    errcode res;

    T(resetlam)
printf("resetlam: num=%d p[0]=%d p[1]=%d\n", num, p[0], p[1]);

    if (num<1)
        return Err_ArgNum;
    /* XXX to be deleted after removing owner from all requests */
    if (num>0) {
        char* owner;
        xdrstrcdup(&owner, p+1);
        printf("resetlam: request contains 'owner': %s\n", owner);
        free(owner);
        num=1;
    }

    if (num!=1)
        return Err_ArgNum;
    if ((lam=get_lam(p[0]))==0)
        return Err_NoPI;
    if (!lam->trinfo.tinfo)
        return Err_PINotActive;

    res=done_trigger(&lam->trinfo);
    if (res) {
        printf("resetlam[%d]: done_trigger failed\n", p[0]);
        return res;
    }
    return OK;
}
/*****************************************************************************/
errcode stoplam(ems_u32* p, unsigned int num) {
    struct LAM *lam;

    T(stoplam)
    if (num<1)
        return Err_ArgNum;
    /* XXX to be deleted after removing owner from all requests */
    if (num>0) {
        char* owner;
        xdrstrcdup(&owner, p+1);
        printf("stoplam: request contains 'owner': %s\n", owner);
        free(owner);
        num=1;
    }

    if (num!=1)
        return Err_ArgNum;
    if ((lam=get_lam(p[0]))==0)
        return Err_NoPI;
    if (!lam->trinfo.tinfo || !(lam->trinfo.state&trigger_active))
        return Err_PINotActive;

    suspend_trigger_task(&lam->trinfo);
    return OK;
}
/*****************************************************************************/
errcode resumelam(ems_u32* p, unsigned int num)
{
    struct LAM *lam;

    T(stoplam)
    if (num<1)
        return Err_ArgNum;
    /* XXX to be deleted after removing owner from all requests */
    if (num>0) {
        char* owner;
        xdrstrcdup(&owner, p+1);
        printf("resumelam: request contains 'owner': %s\n", owner);
        free(owner);
        num=1;
    }

    if (num!=1)
        return Err_ArgNum;
    if ((lam=get_lam(p[0]))==0)
        return Err_NoPI;
    if (!lam->trinfo.tinfo || !(lam->trinfo.state&trigger_inserted))
        return Err_NoPI;
    if (lam->trinfo.state&trigger_active)
        return Err_PIActive;

    reactivate_trigger_task(&lam->trinfo);
    return OK;
}
/*****************************************************************************/
/*
 * deactivate all LAMs (called from shutdown_server and ResetVED)
 * In principle pi_lam_done should be enough but the order of the
 * *_done procedures is not clear
 */
errcode stoplams(void)
{
    struct LAM *lam=lam_list;
    T(stoplams)
    while (lam) {
        if (lam_list->trinfo.tinfo && (lam_list->trinfo.state&trigger_inserted))
            remove_trigger_task(&lam_list->trinfo);
        lam=lam->next;
    }
    return OK;
}
/*****************************************************************************/
errcode pi_lam_init(void)
{
    T(pi_lam_init)
    lam_list=0;

    return OK;
}
/*****************************************************************************/
errcode pi_lam_done(void)
{
    T(pi_lam_done)
    while (lam_list) {
        if (lam_list->trinfo.tinfo) {
            if (lam_list->trinfo.state&trigger_inserted)
                remove_trigger_task(&lam_list->trinfo);
            done_trigger(&lam_list->trinfo);
        }
        free_lam(lam_list);
    }
    return OK;
}
/*****************************************************************************/
/*
getlamattr
p[0] : LAM-Index
*/
errcode getlamattr(ems_u32* p, unsigned int num)
{
    struct LAM *lam;
    int i;

    T(getlamattr)
    if (num!=1)
        return Err_ArgNum;
    if ((lam=get_lam(p[0]))==0)
        return Err_NoPI;

    *outptr++=!!(lam->trinfo.state&trigger_active);
    *outptr++=lam->cnt;
    *outptr++=lam->id;
    *outptr++=lam->is;
    *outptr++=lam->trigproc;
    for (i=0; i<=lam->trigargs[0]; i++)
        *outptr++=lam->trigargs[i];
    return OK;
}
/*****************************************************************************/
/*
getlamparams
p[0] : LAM-Index
*/
/* ist alles in getlamattr drin */
errcode getlamparams(ems_u32* p, unsigned int num)
{
    struct LAM *lam;

    T(getlamparams)
    if (num!=1)
        return Err_ArgNum;
    if ((lam=get_lam(p[0]))==0)
        return Err_NoPI;

    return Err_NotImpl;
}
/*****************************************************************************/
/*
p[0] : object (==Object_pi)
p[1] : invocation (==Invocation_LAM)
*/
static ems_u32 *dir_pi_lam(ems_u32* p)
{
    struct LAM *lam=lam_list;

    T(dir_pi_lam)
    while (lam) {
        *p++=lam->idx;
        lam=lam->next;
    }
    return(p);
}
/*****************************************************************************/
static objectcommon* lookup_pi_lam(ems_u32* id, unsigned int idlen,
        unsigned int* remlen)
{
T(lookup_pi_lam)
if (idlen>0)
  {
  *remlen=idlen;
  return((objectcommon*)&pi_lam_obj);
  }
else
  {
  *remlen=0;
  return((objectcommon*)&pi_lam_obj); /* XXX */
  }
}
/*****************************************************************************/

piobj pi_lam_obj={
	{
		pi_lam_init,
		pi_lam_done,
		lookup_pi_lam,
		dir_pi_lam,
		0,	/* ref */
#if 0
		0, /* setprot */
		0, /* getprot */
#endif
	},
	createlam,
	deletelam,
	startlam,
	resetlam,
	stoplam,
	resumelam,
	getlamattr,
        0
};
/*****************************************************************************/
/*****************************************************************************/
