/*
 * readout_eventfilter/piops.c
 * 
 * created: 2007-Jun-09 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: piops.c,v 1.1 2017/10/12 21:44:32 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <objecttypes.h>
#include "../../../main/nowstr.h"
#include "datains.h"
#include "../readout.h"
#include "../pireadoutobj.h"
#include "../../../dataout/cluster/do_cluster.h"
#include "../../../objects/domain/dom_dataout.h"
#include "../../../main/scheduler.h"
#include "../../notifystatus.h"
#include "../../../dataout/dataout.h"
#include "../../domain/dom_datain.h"

#if 0
#define set_max(a, b) ((a)=(a)<(b)?(b):(a))
#endif

extern ems_u32* outptr;

InvocStatus readout_active;

/*****************************************************************************/
#if 0
void
fatal_readout_error()
{
    T(readout_eventfilter/piops.c:fatal_readout_error)

    printf("Fatal readout error!!\n");
    freeze_datains();
    freeze_dataouts();
    readout_active=Invoc_error;
    {
        ems_u32 obj[]={Invocation_readout, 0};
        notifystatus(status_action_change, Object_pi, 2, obj);
    }
}
#endif
/*****************************************************************************/
errcode
pi_readout_init()
{
    T(readout_eventfilter/piops.c:pi_readout_init)

    printf("readout_eventfilter/piops.c:pi_readout_init: HIER MUSS WAS GEMACHT WERDEN!\n");
    return OK;
}
/*****************************************************************************/
errcode
pi_readout_done()
{
    T(readout_eventfilter/piops.c:pi_readout_done)

    printf("readout_eventfilter/piops.c:pi_readout_done: HIER MUSS AUFGERAEUMT WERDEN!\n");
    return OK;
}
/*****************************************************************************/
static objectcommon*
lookup_pi_readout(ems_u32* id, unsigned int idlen, unsigned int* remlen)
{
    T(readout_eventfilter/piops.c:lookup_pi_readout)
    if (idlen>0) {
        if (id[0]!=0) return 0;
        *remlen=idlen;
        return (objectcommon*)&pi_readout_obj;
    } else {
        *remlen=0;
        return (objectcommon*)&pi_readout_obj;
    }
}
/*****************************************************************************/
static ems_u32 *
dir_pi_readout(ems_u32* ptr)
{
    T(readout_eventfilter/piops.c:dir_pi_readout)

    *ptr++=0;
    return ptr;
}
/*****************************************************************************/
#if 0
static int setprot_pi_readout(ems_u32* idx, unsigned int idxlen,
        char* newowner, int* newperm)
{
    T(readout_eventfilter/piops.c:setprot_pi_readout)
    if (newperm)
        return -1;
    if (newowner) {
        if (owner)
            free(owner);
        owner=strdup(newowner);
    }
    return 0;
}
#endif
/*****************************************************************************/
#if 0
static int getprot_pi_readout(ems_u32* idx, unsigned int idxlen,
        char** ownerp, int** permp)
{
    T(readout_eventfilter/piops.c:getprot_pi_readout)

    if (ownerp)
        *ownerp=owner;
    if (permp)
        *permp=perms;
    return 0;
}
#endif
/*****************************************************************************/
static errcode
createreadout(ems_u32* p, unsigned int num)
{
    T(readout_eventfilter/piops.c:createreadout)
    if (num!=1)
        return Err_ArgNum;
    return Err_ObjDef;
}
/*****************************************************************************/
static errcode
deletereadout(ems_u32* p, unsigned int num)
{
    T(readout_eventfilter/piops.c:deletereadout)
    if (num!=1)
        return Err_ArgNum;
    return Err_ObjNonDel;
}
/*****************************************************************************/
static errcode
startreadout(ems_u32* p, unsigned int num)
{
    ems_u32 obj[]={Invocation_readout, 0};

    T(readout_eventfilter/piops.c:startreadout)

    printf("%s startreadout; readout_active=%d\n", nowstr(), readout_active);
    if (readout_active)
        return Err_PIActive;

    printf("readout_eventfilter/piops.c:startreadout: HIER FEHLT CODE!\n");

#if 0
    errcode res;
    int i, j;

    ved_info_sent=0;
    if ((res=start_dataout())!=OK)
        return res;

    for (i=0; i<MAX_DATAIN; i++) {
        if (datain[i].bufftyp!=-1) {
            res=datain_cl[i].procs.start(i);
            if (res!=OK) {
                printf("readout_eventfilter/piops::startreadout():\n"
                        "  cannot initialise datain %d\n", i);
                for (j=0; j<i; j++)
                    datain_cl[j].procs.reset(j);
                for (j=0; j<MAX_DATAOUT; j++)
                    dataout_cl[j].procs.reset(j);
                return res;
            }
        }
    }

#endif
    readout_active=Invoc_active;
    notifystatus(status_action_start, Object_pi, 2, obj);
    D(D_REQ, printf("startreadout in piops.c: OK\n");)
    return OK;
}
/*****************************************************************************/
static errcode
resetreadout(ems_u32* p, unsigned int num)
{
    ems_u32 obj[]={Invocation_readout, 0};

    T(readout_eventfilter/piops.c:resetreadout)

    if (num<1)
        return Err_ArgNum;
    printf("%s resetreadout; readout_active=%d\n", nowstr(), readout_active);
    if (readout_active==Invoc_notactive)
        return Err_PINotActive;

    printf("readout_eventfilter/piops.c:resetreadout: HIER FEHLT CODE!\n");

#if 0
    int i;
    errcode res;

    for (i=0; i<MAX_DATAIN; i++) {
        if (datain[i].bufftyp!=-1) {
            datain_cl[i].procs.reset(i);
        }
    }
    if ((res=stop_dataouts()))
        return(res);
    /*eventcnt=0;*/

#endif
    readout_active=Invoc_notactive;
    notifystatus(status_action_reset, Object_pi, 2, obj);

    return OK;
}
/*****************************************************************************/
static errcode
stopreadout(ems_u32* p, unsigned int num)
{
    ems_u32 obj[]={Invocation_readout, 0};
    T(readout_eventfilter/piops.c:stopreadout)

    if (num<1)
        return Err_ArgNum;
    printf("%s stopreadout; readout_active=%d\n", nowstr(), readout_active);
    if (readout_active!=Invoc_active)
        return Err_PINotActive;

    printf("readout_eventfilter/piops.c:stopreadout: HIER FEHLT CODE!\n");

#if 0
    int i;

    for (i=0; i<MAX_DATAIN; i++) {
        if (datain[i].bufftyp!=-1) datain_cl[i].procs.stop(i);
    }

#endif
    readout_active=Invoc_stopped;
    notifystatus(status_action_stop, Object_pi, 2, obj);
    return OK;
}
/*****************************************************************************/
static errcode
resumereadout(ems_u32* p, unsigned int num)
{
    ems_u32 obj[]={Invocation_readout, 0};
    T(readout_eventfilter/piops.c:resumereadout)

    if (num<1)
        return Err_ArgNum;
    printf("%s resumereadout; readout_active=%d\n", nowstr(), readout_active);

    if (readout_active!=Invoc_stopped) {
        if (readout_active==Invoc_active)
            return Err_PIActive;
        else
            return Err_PINotActive;
    }

    printf("readout_eventfilter/piops.c:stopreadout: HIER FEHLT CODE!\n");

#if 0
    int i;

    for (i=0; i<MAX_DATAIN; i++) {
        if (datain[i].bufftyp!=-1) datain_cl[i].procs.resume(i);
    }

#endif
    readout_active=Invoc_active;
    notifystatus(status_action_resume, Object_pi, 2, obj);
    return OK;
}
/*****************************************************************************/
#if 0
void
check_ro_done(int idx)
{
    int i;
    static ems_u32 id[2]={Invocation_readout, 0};
    T(readout_eventfilter/piops.c:check_ro_done)

    printf("%s datain %d finished.\n", nowstr(), idx);
    if (readout_active!=Invoc_active) {
        printf("check_ro_done: readout not active!\n");
        return;
    }
    for (i=0; i<MAX_DATAIN; i++) {
        if ((datain[i].bufftyp!=-1) && datain_cl[i].status!=Invoc_alldone)
            return;
    }
    notifystatus(status_action_finish, Object_pi, 2, id);
    readout_active=Invoc_alldone;
    printf("all datains finished.\n");
}
#endif
/*****************************************************************************/
/*
getreadoutparams
p[0] : id (ignoriert)
*/
static errcode
getreadoutparams(ems_u32* p, unsigned int num)
{
    if (num!=1)
        return Err_ArgNum;

    printf("readout_eventfilter/piops.c:getreadoutparams: HIER FEHLT CODE\n");
#if 0
    *outptr++=0;
#endif
    return OK;
}
/*****************************************************************************/
/*
getreadoutattr
p[0] : id (ignoriert)
[p[1] : Format fuer Timestamp (1: unix; 2: OSK)
[p[2] : Format fuer Zusatzdaten (1: nur readout; 2: datains; 4: buffer)]]
*/
static errcode
getreadoutattr(ems_u32* p, unsigned int num)
{
    T(readout_eventfilter/piops.c:getreadoutattr)

    if ((num<1) | (num>3))
        return(Err_ArgNum);

    printf("readout_eventfilter/piops.c:getreadoutattr: HIER FEHLT CODE\n");

#if 0
    int i, evcnt=-1;

    *outptr++=readout_active;
    for (i=0; i<MAX_DATAIN; i++) {
    if (datain[i].bufftyp!=-1) {
        int j;
        for (j=0; j<datain_cl[i].vedinfos.num_veds; j++)
            set_max(evcnt, datain_cl[i].vedinfos.info[j].events);
        }
    }

    *outptr++=evcnt;
    if (num>1) {
        *outptr++=p[1];
        switch (p[1]) {
        case 1: { /* unix-format */
#if /*defined (unix) ||*/ defined (__unix__)
            struct timeval t;
            gettimeofday(&t, 0);
            *outptr++=t.tv_sec;
            *outptr++=t.tv_usec;
#else
#ifdef OSK
            int sekunden, tage, hundertstel;
            short wochentag;
            _sysdate(3, &sekunden, &tage, &wochentag, &hundertstel);
      
            *outptr++=sekunden+86400*tage; /* sec */
            *outptr++=hundertstel*10000;   /* usec */
#else
       xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#endif
#endif
            }
            break;
        case 2: { /* OS/9-format */
#if /*defined (unix) ||*/ defined (__unix__)
            struct timeval t;
            gettimeofday(&t,0);
            *outptr++=t.tv_sec/86400;  /* tage */
            *outptr++=t.tv_sec%86400;  /* sekunden */
            *outptr++=t.tv_usec/10000; /* hundertstel */
#else
#ifdef OSK
            int sekunden, tage, hundertstel;
            short wochentag;
            _sysdate(3, &sekunden, &tage, &wochentag, &hundertstel);
            *outptr++=tage;
            *outptr++=sekunden;
            *outptr++=hundertstel;
#else
      xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#endif
#endif
            }
            break;
        default:
            return(Err_ArgRange);
            break;
        }
    }

    if (num>2) {
        ems_u32* extraptr=outptr++;
        int extras=0;
        if (p[2]&1) {
            extras++;
            *outptr++=1; /* key fuer readout data */
            *outptr++=0; /* number of data */
            /* es gibt noch keine */
        }
        if (p[2]&2) {
            ems_u32* countptr;
            ems_u32* numptr;
            int donum=0;
            extras++;
            *outptr++=2; /* key fuer datain data */
            countptr=outptr++;
            numptr=outptr++;
            for (i=0; i<MAX_DATAIN; i++) {
                if (datain[i].bufftyp!=-1) {
                    donum++;
                    *outptr++=i;
                    *outptr++=7; /* number of data */
                    *outptr++=datain_cl[i].status;
                    *outptr++=datain_cl[i].error;
                    *outptr++=datain_cl[i].suspended;
                    *outptr++=datain_cl[i].suspensions;
                    *outptr++=datain_cl[i].clusters;
                    /* *outptr++=datain_cl[i].eventcnt; */
                    *outptr++=0;
                    *outptr++=datain_cl[i].words;
                }
            }
            *numptr=donum;
            *countptr=outptr-countptr-1;
        }
        if (p[2]&4) {
            ems_u32* countptr;
            extras++;
            *outptr++=4; /* key fuer buffer data */
            countptr=outptr++;
#ifdef EXTRA_DO_DATA
            extra_do_data();
#endif
            *countptr=outptr-countptr-1;
        }
        *extraptr=extras;
    }
#endif
    return OK;
}
/*****************************************************************************/
piobj pi_readout_obj={ /* ???? */
    {
        pi_readout_init,
        pi_readout_done,
        /*(lookupfunc)*/lookup_pi_readout,
        dir_pi_readout,
        0,	/* ref */
#if 0
        setprot_pi_readout,
        getprot_pi_readout,
#endif
    },
    createreadout,
    deletereadout,
    startreadout,
    resetreadout,
    stopreadout,
    resumereadout,
    getreadoutattr,
    getreadoutparams
};
/*****************************************************************************/
/*****************************************************************************/
