/*
 * objects/pi/readout_async/readout_async.c
 * created 2005-May-09 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL$";

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sconf.h>
#include <debug.h>
#include <objecttypes.h>
#include <unsoltypes.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <emsctypes.h>
#include "../../is/is.h"
#include "../../../trigger/trigger.h"
#include "../../../dataout/dataout.h"
#include "../../../main/scheduler.h"
#include "../../../lowlevel/devices.h"
#include "../../../lowlevel/profile/profile.h"
#include "../readout.h"

#include "../pireadoutobj.h"
#include "../../notifystatus.h"
#include "../../../procs/proclist.h"
#include "../../../main/server.h"
#include "../../../main/nowstr.h"
#include "../../../commu/commu.h"

#include "../../../lowlevel/perfspect/perfspect.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#ifdef VAR_READOUT_PRIOR
extern int readout_prior;
#endif

extern ems_u32* outptr;
extern int *memberlist;
#ifdef ISVARS
extern ISV *isvar;
#endif
#ifdef MAXEVCOUNT
int maxevcount=MAXEVCOUNT;
#endif

int event_max=EVENT_MAX;

#ifndef SCHED_TRIGGER
#error readout_async requieres SCHED_TRIGGER
#endif

int suspended;
int suspensions;
int wirhaben; /* used in dataout/cluster/cl_interface.c also */
int inside_readout;

/*****************************************************************************/

struct isreadoutinfo{
    int isidx;
    int isid;
    int *members;
#ifdef ISVARS
    ISV *isvar;
#endif
    int *readoutlist;
};

static struct isreadoutinfo *readoutinfo[MAX_TRIGGER];
static int readoutcnt[MAX_TRIGGER];

int onreadouterror;
InvocStatus readout_active;

static char *owner;
static int perms[]={
    2,
    0,
    OP_CHSTATUS
};
static int event_invalid;

/*****************************************************************************/
static void setowner(ems_u32* p)
{
    if (owner) free(owner);
    xdrstrcdup(&owner, p);
}
/*****************************************************************************/
static int testowner(ems_u32* p)
{
    char* help;
    int res;
    if (owner==0) return 0;
    xdrstrcdup(&help, p);
    res=-strcmp(help, owner);
    free(help);
    return res;
}
/*****************************************************************************/
static void resetowner(void)
{
    if (owner) free(owner);
    owner=0;
}
/*****************************************************************************/
errcode pi_readout_init()
{
    int i;

    T(pi_readout_init)
    readout_active=Invoc_notactive;
    eventcnt=0;
    onreadouterror=0;
    for(i=0; i<MAX_TRIGGER; i++){
        readoutinfo[i]=(isreadoutinfo*)0;
        readoutcnt[i]=0;
    }
    owner=0;
#ifdef MAXEVCOUNT
    maxevcount=MAXEVCOUNT;
#endif
    inside_readout=0;
    return(OK);
}
/*****************************************************************************/
errcode pi_readout_done()
{
    int i;

    T(pi_readout_done)
    for(i=0; i<MAX_TRIGGER; i++)
        if(readoutinfo[i]) free(readoutinfo[i]);
    resetowner();
    return(OK);
}
/*****************************************************************************/
/*
 createreadout
 p[0] : id (ignoriert)
 */
static errcode createreadout(ems_u32* p, unsigned int num)
{
    T(createreadout)

    if (num!=1) return(Err_ArgNum);
    return(Err_ObjDef);
}
/*****************************************************************************/
/*
 deletereadout
 p[0] : id (ignoriert)
 */
static errcode deletereadout(ems_u32* p, unsigned int num)
{
   T(deletereadout)

   if(num!=1) return(Err_ArgNum);
   return(Err_ObjNonDel);
}
/*****************************************************************************/
/*
getreadoutattr
p[0] : id (ignoriert)
[p[1] : Format fuer Timestamp (1: unix; 2: OSK)
[p[2] : Format fuer Zusatzdaten (1: readout; 2: datains (fuer EB); 4: buffer)]]
*/
static errcode getreadoutattr(ems_u32* p, unsigned int num)
{
    T(getreadoutattr)
    D(D_REQ, {int i; printf("getreadoutattr(num=%d, ", num);
        for (i=0; i<num; i++) printf("%d%s", p[i], i+1<num?", ":"");
        printf(")\n");}
    )

    if ((num<1) | (num>3))
        return Err_ArgNum;

    *outptr++=readout_active;
    *outptr++=eventcnt;
    if (num>1) {
        *outptr++=p[1];
        switch (p[1]) {
        case 1: { /* unix-format */
            struct timeval t;
            gettimeofday(&t, 0);
            *outptr++=t.tv_sec;
            *outptr++=t.tv_usec;
            }
            break;
        case 2: { /* OS/9-format */
            struct timeval t;
            gettimeofday(&t,0);
            *outptr++=t.tv_sec/86400;  /* tage */
            *outptr++=t.tv_sec%86400;  /* sekunden */
            *outptr++=t.tv_usec/10000; /* hundertstel */
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
            *outptr++=1; /* key fuer readout data */
            *outptr++=2; /* number of data */
            extras++;
            *outptr++=suspensions;
            *outptr++=suspended;
        }
        /*if (p[2]&2) nur fuer Eventbuilder */
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
    return OK;
}
/*****************************************************************************/
/*
getreadoutparams
p[0] : id (ignoriert)
*/
static errcode getreadoutparams(ems_u32* p, unsigned int num)
{
if (num!=1) return(Err_ArgNum);
if (owner)
  outptr=outstring(outptr, owner);
else
  *outptr++=0;
return(OK);
}
/*****************************************************************************/
/*
startreadout
p[0] : id (ignoriert)
*/
static errcode startreadout(ems_u32* p, unsigned int num)
{
    int trig;
    errcode res;
    int restlen,limit;

    T(startreadout)
    if (num!=1) {
        if (num<2) return(Err_ArgNum);
        if (num!=xdrstrlen(p+1)+1) return(Err_ArgNum);
    }

    printf("%s startreadout; readout_active=%d\n", nowstr(), readout_active);
    if (readout_active!=Invoc_notactive) return(Err_PIActive);
    printf("STARTREADOUT\n");

    resetowner();
    if (num>2) setowner(p+1);

    perfspect_reset();
    perfspect_set_state(perfspect_state_setup);

    /* Listen einsammeln und nach Prioritaet sortieren */
    for (trig=0; trig<MAX_TRIGGER; trig++) {
        int is, max_prior=0;
        unsigned int cnt=0;
        isreadoutinfo *ptr;

        /* hoechste Prioritaet und Anzahl der IS pro Triggertyp bestimmen */
        for (is=1; is<MAX_IS; is++) {
            if (is_list[is] && is_list[is]->enabled &&
                    (is_list[is]->readoutlist[trig])) {
                int tmp_prior;
                cnt++;
                tmp_prior=is_list[is]->prioritaet[trig];
                max_prior=(tmp_prior>max_prior?tmp_prior:max_prior);
            }
        }
        D(D_TRIG,printf("%d Listen fuer Trigger %d gefunden\n", cnt, trig);)

        /* allocate memory for readoutinfo */
        if (readoutinfo[trig]) free(readoutinfo[trig]);
        readoutinfo[trig]=ptr=(isreadoutinfo*)calloc(cnt, sizeof(isreadoutinfo));
        readoutcnt[trig]=cnt;
        restlen=event_max;

        /* Listen einsammeln, dabei naechsthoechste Prioritaet bestimmen */
        while (cnt) {
            int second_prior=0;
            for (is=1; is<MAX_IS; is++) {
                int *p, pri;
                if (is_list[is] && is_list[is]->enabled &&
                        (p=is_list[is]->readoutlist[trig])) {
                    pri=is_list[is]->prioritaet[trig];
                    if (pri==max_prior) {
                        int *help,res;
                        D(D_TRIG,printf("\tIS %d (prior %d)\n", is, pri);)
                        ptr->isidx=is;
                        ptr->isid=is_list[is]->id;
                        ptr->members=memberlist=is_list[is]->members;
#ifdef ISVARS
                        ptr->isvar=isvar=&(is_list[is]->isvar);
#endif
                        ptr++->readoutlist=p;
                        help=outptr;
                        outptr+=2;
                        perfspect_set_trigg_is(trig, is);
                        res=test_proclist(p, is_list[is]->listenlaenge[trig],
                                &limit);
                        if ((!res)&&(limit>restlen)) {
                            printf("startreadout: limit=%d > restlen=%d\n",
                                    limit, restlen);
                            res=Err_BufOverfl;
                        }
                        if (res) {
                            *help++=is;
                            *help=trig;
                            perfspect_set_state(perfspect_state_invalid);
                            return(res);
                        } else {
                            outptr=help;
                        }
                        if (limit>0) restlen-=limit;
                        cnt--;
                    } else {
                        if ((pri<max_prior) && (pri>second_prior))
                                second_prior=pri;
                    }
                }
            }
            max_prior=second_prior;
        }
    }
    eventcnt=0;

#ifdef PERFSPECT
    if (perfspect_realloc_arrays()!=plOK)
        return Err_NoMem;
#endif

#ifdef DELAYED_READ
    reset_delayed_read();
#endif

    if ((res=init_trigger())!=OK) {
        perfspect_set_state(perfspect_state_invalid);
        return res;
    }
    if ((res=start_dataout("startreadout"))!=OK) {
        perfspect_set_state(perfspect_state_invalid);
        return res;
    }

    suspensions=0;
    /* insert_readout_task() ist beim Trigger definiert */
    if ((res=insert_trigger_task())!=OK) {
        perfspect_set_state(perfspect_state_invalid);
        return res;
    }

    if (!buffer_free) {
        schau_nach_speicher();
        if (!buffer_free) {
/* readouttask muss von Bufferverwaltung wieder aktiviert werden */
            suspend_trigger_task();
            suspensions++;
/* schau_nach_speicher wird nur einmal ausgefuehrt; alles andere ist
   Sache der Bufferverwaltung */
        }
    } else {
        printf("startreadout: buffer_free=%d\n", buffer_free);
        panic();
    }
    readout_active=Invoc_active;
    {
        int obj[]={Invocation_readout, 0};
        notifystatus(status_action_start, Object_pi, 2, obj);
    }
    D(D_REQ, printf("READOUT!!\n");)
    perfspect_set_state(perfspect_state_ready);
    return(OK);
}
/*****************************************************************************/
/*
 resetreadout
 p[0] : pi-type (==Invocation_readout)
 p[1] : id (ignoriert)
 */
static errcode resetreadout(ems_u32* p, unsigned int num)
{
    errcode res;

    T(resetreadout)

    if (num<1) return(Err_ArgNum);
    printf("%s resetreadout; readout_active=%d\n", nowstr(), readout_active);
    if (readout_active==Invoc_notactive) return(Err_PINotActive);
    printf("RESETREADOUT\n");

    if (num!=1) {
        if (num!=xdrstrlen(p+1)+1) return(Err_ArgNum);
        if (testowner(p+1)!=0) return Err_NotOwner;
    }
    resetowner();

    remove_trigger_task();
    suspended=0;
    suspensions=0;
    perfspect_set_state(perfspect_state_inactive);
    if ((res=done_trigger())) return res;
    if ((res=stop_dataouts("resetreadout"))) return res;
    D(D_REQ, printf("readout gestoppt!!\n");)
    readout_active=Invoc_notactive;
  /* eventcnt=0; PW */

    {
        int obj[]={Invocation_readout, 0};
        notifystatus(status_action_reset, Object_pi, 2, obj);
        }
    return(OK);
}
/*****************************************************************************/
static errcode stopreadout(ems_u32* p, unsigned int num)
{
    T(stopreadout)
    if(num<1) return(Err_ArgNum);
    printf("%s stopreadout; readout_active=%d\n", nowstr(), readout_active);
    if (readout_active!=Invoc_active) return Err_PINotActive;
    if (num!=1) {
        if (num!=xdrstrlen(p+1)+1) return Err_ArgNum;
        if (testowner(p+1)!=0) return Err_NotOwner;
    }
    printf("stopreadout: suspended=%d\n", suspended);
    if (!suspended) suspend_trigger_task();
    readout_active=Invoc_stopped;
    {
        int obj[]={Invocation_readout, 0};
        notifystatus(status_action_stop, Object_pi, 2, obj);
    }
    return OK;
}
/*****************************************************************************/
static errcode resumereadout(ems_u32* p, unsigned int num)
{
    callbackdata calldata;
    T(resumereadout)

    calldata.p=(void*)0;
    if (num<1) return(Err_ArgNum);

    if (readout_active!=Invoc_stopped) {
        if (readout_active==Invoc_active)
            return(Err_PIActive);
        else
            return(Err_PINotActive);
    }
    if (num!=1) {
        if (num!=xdrstrlen(p+1)+1) return Err_ArgNum;
        if (testowner(p+1)!=0) return Err_NotOwner;
    }

    if (!suspended) reactivate_trigger_task();
    readout_active=Invoc_active;
    {
        int obj[]={Invocation_readout, 0};
        notifystatus(status_action_resume, Object_pi, 2, obj);
    }
    return(OK);
}
/*****************************************************************************/
void fatal_readout_error()
{
    T(readout_cc/readout.c:fatal_readout_error)

    printf("%s Fatal readout error!!\n", nowstr());
    if (readout_active!=Invoc_active) {
        printf("fatal_readout_error(); readout_active=%d\n", readout_active);
        return;
    }
    remove_trigger_task();

    readout_active=Invoc_error;
    {
        int obj[]={Invocation_readout, 0};
        notifystatus(status_action_change, Object_pi, 2, obj);
    }
}
/*****************************************************************************/
static void read_out(ems_u32 trig)
{
    isreadoutinfo *ptr;
    int cnt;
    ems_u32* current_trig_ptr;

    PROFILE_START_2(PROF_RO,trig);
    event_invalid=0;

/* TEST */
    if (next_databuf==0) {
        int buf[3];
        printf("readout.c:read_out: "
                "next_databuf=0, eventcnt=%d, buffer_free=%d\n", 
                    eventcnt, buffer_free);
        buf[0]=rtErr_Other;
        buf[1]=buffer_free;
        buf[2]=eventcnt;
        send_unsolicited(Unsol_RuntimeError, buf, 2);
        fatal_readout_error();
        return;
    }
    outptr=next_databuf;
    ptr=readoutinfo[trig];
    cnt=readoutcnt[trig];

    add_used_trigger(trig);

    *outptr++=eventcnt;
    current_trig_ptr=outptr;
    *outptr++=trig;
    *outptr++=cnt;
    wirhaben=0;
    inside_readout=1;
    perfspect_set_state(perfspect_state_active);
    perfspect_eventstart();
    DV(D_TRIG, printf("Event No. %d, Trigger %d, lese %d IS aus\n", eventcnt,
		  trig, cnt);)
    while(cnt--) {
        ems_u32* help;
        int res;

        memberlist=ptr->members;
#ifdef ISVARS
        isvar=ptr->isvar;
#endif
        perfspect_set_trigg_is(trig, ptr->isidx);
        *outptr++=ptr->isid;
        help=outptr++; /* space for wordcount */
        DV(D_TRIG, printf("   IS: %d\n",ptr->isid);)
        PROFILE_START(PROF_RO_SUB);
        if ((res=scan_proclist(ptr++->readoutlist))) {/* Error in readoutlist */
            PROFILE_END(PROF_RO_SUB);
            if(onreadouterror<=1){
                int *buf, i;
                unsigned int anz;
                anz=outptr-help-1;
                buf=(int*)calloc(anz+5, sizeof(int));
                if(!buf) { /* passiert hoffentlich nie */
	            int b[2];
	            b[0]=rtErr_Other;
	            b[1]=Err_NoMem;
	            send_unsolicited(Unsol_RuntimeError, b, 3);
                    inside_readout=0;
                    perfspect_set_state(perfspect_state_inactive);
	            fatal_readout_error();
	            return;
                }
                buf[0]=rtErr_ExecProcList;
                buf[1]=eventcnt;
                buf[2]=res;
                buf[3]=(ptr-1)->isid;
                buf[4]=trig;
                for (i=0; i<anz; i++) buf[i+5]= *(help+i+1);
                send_unsolicited(Unsol_RuntimeError, buf, anz+5);
                free(buf);
                if (!onreadouterror) {
                    inside_readout=0;
                    perfspect_set_state(perfspect_state_inactive);
	            fatal_readout_error();
	            return;
                }
            }
            outptr-=2;
        }
        PROFILE_END(PROF_RO_SUB);
        *help=outptr-help-1; /* wordcount */
        wirhaben+=*help;
        DV(D_TRIG, printf("ausgelesen: %d Worte von IS id %d\n",
		    *help, *(help-1));)
        }
    inside_readout=0;
    perfspect_set_state(perfspect_state_inactive);
    reset_trigger();
PROFILE_END(PROF_RO);
    if (event_invalid) *current_trig_ptr|=0x80000000;
    flush_databuf(outptr);
}
/*****************************************************************************/
void doreadout(ems_u32 trig)
{
#ifdef DEBUG
    if (!buffer_free) {
        printf("kein buffer frei zum readout, error\n");
        panic();
    }
#endif
    read_out(trig);

#ifdef MAXEVCOUNT
    if (maxevcount && (eventcnt==maxevcount)) {
        if (!suspended) suspend_trigger_task();
        readout_active=Invoc_stopped;
        printf("\007\007\007%d EVENTS READ; READOUT STOPPED\n", eventcnt);
        return;
    }
#endif /* MAXEVCOUNT */
/* 
readouttask muss von Bufferverwaltung (ueber 'outputbuffer_freed')
wieder aktiviert werden
*/
    if (!buffer_free) {
/*
 *   printf("doreadout: buffer_free=0, eventcnt=%d\n", eventcnt);
 */
        suspend_trigger_task();
        suspended=1;
        suspensions++;
    }
}
/*****************************************************************************/
void outputbuffer_freed(void)
{
if (!suspended) return;
schau_nach_speicher();
if (buffer_free)
  {
/*
 *   printf("outputbuffer_freed: eventcnt=%d, next_databuf=%p\n",
 *       eventcnt, next_databuf);
 */
  if (next_databuf==0) {buffer_free=0; return;}
  suspended=0;
  if (readout_active!=Invoc_stopped)
    {
    if (readout_active!=Invoc_active)
      {
      printf("Warning in outputbuffer_freed: readout_active==");
      switch (readout_active)
        {
        case Invoc_error: printf("Invoc_error"); break;
        case Invoc_alldone: printf("Invoc_alldone"); break;
        case Invoc_stopped: printf("Invoc_stopped"); break;
        case Invoc_notactive: printf("Invoc_notactive"); break;
        case Invoc_active: printf("Invoc_active"); break;
        default: printf("%d", readout_active); break;
        }
      printf("\n");
      fflush(stdout);
      }
    else
      reactivate_trigger_task();
    }
  }
}
/*****************************************************************************/
void invalidate_event()
{
    event_invalid=1;
}
/*****************************************************************************/
/*static piobj *lookup_pi_readout(int* id, int idlen, int* remlen)*/
static objectcommon* lookup_pi_readout(ems_u32* id, unsigned int idlen, unsigned int* remlen)
{
T(lookup_pi_readout)
if (idlen>0)
  {
  if (id[0]!=0) return(0);
  *remlen=idlen;
  return((objectcommon*)&pi_readout_obj);
  }
else
  {
  *remlen=0;
  return((objectcommon*)&pi_readout_obj);
  }
}
/*****************************************************************************/
static int *dir_pi_readout(int* ptr)
{
*ptr++=0;
return(ptr);
}
/*****************************************************************************/
static int setprot_pi_readout(ems_u32* idx, unsigned int idxlen, char* newowner, int* newperm)
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
static int getprot_pi_readout(ems_u32* idx, unsigned int idxlen, char** ownerp, int** permp)
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
    /*(lookupfunc)*/lookup_pi_readout,
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
  getreadoutparams
};
/*****************************************************************************/
/*****************************************************************************/
