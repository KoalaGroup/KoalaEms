/*
 * objects/is/is.c
 * created before 03.08.94
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: is.c,v 1.30 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdlib.h>
#include "is.h"
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../objectcommon.h"
#include <actiontypes.h>
#include <unsoltypes.h>
#include "../notifystatus.h"
#ifdef READOUT_CC
#include "../pi/readout.h"
#endif /* PI_READOUT */
#include "../domain/dom_ml.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

RCS_REGISTER(cvsid, "objects/is")

/*****************************************************************************/

extern ems_u32* outptr;
#ifdef ISVARS
extern ISV* isvar;
#endif /* ISVARS */
struct IS *is_list[MAX_IS];

/*****************************************************************************/

errcode is_init(void)
{
int i;

T(is_init)
for (i=0; i<MAX_IS; i++) is_list[i]=0;
return(OK);
}

/*****************************************************************************/
static void
free_readoutlist(struct readoutlist *readoutlist)
{
    free(readoutlist->list);
    free(readoutlist);
}
/*****************************************************************************/
static void
unlink_readoutlist(struct readoutlist *readoutlist)
{
    if (!--readoutlist->refcount)
        free_readoutlist(readoutlist);
}
/*****************************************************************************/
/*
 * delete_is deletes is[idx] with all assiciated ressources
 * the calling procedure has to guarantee that idx is valid
 */
static errcode
delete_is(int idx)
{
    struct IS *is=is_list[idx];
    int i;

    if (is->members)
        free(is->members);

#ifdef ISVARS
    if (is->isvar.var)
        free(is->isvar.var);
#endif /* ISVARS */

#if defined (ROVARS) && defined (READOUT_CC)
    for (i=0; i<MAX_TRIGGER; i++)
        if (is->rovar[i].var)
            free(is->rovar[i].var);
#endif /* ISVARS && READOUT_CC */

#ifdef READOUT_CC
    for (i=0; i<MAX_TRIGGER; i++) {
        if (is->readoutlist[i]) {
            unlink_readoutlist(is->readoutlist[i]);
            is->readoutlist[i]=0;
        }
    }
#endif /* READOUT_CC */

    free(is_list[idx]);
    is_list[idx]=(struct IS*)0;
    return OK;    
}
/*****************************************************************************/
errcode is_done(void)
{
    int idx;

    T(is_done)
    for (idx=0; idx<MAX_IS; idx++) {
        if (is_list[idx]) {
            delete_is(idx);
        }
    }
    return OK;
}
/*****************************************************************************/

errcode is_getnamelist(__attribute__((unused)) ems_u32* p,
    __attribute__((unused)) unsigned int num)
{
int i, j;

T(is_getnamelist)
D(D_REQ, printf("GetNameList: IS\n");)
j=0;
for (i=0; i<MAX_IS; i++) if (is_list[i]!=0) outptr[++j]=i;
outptr[0]=j;
outptr+=j+1;
return(OK);
}

/*****************************************************************************/
/*
CreateIS
p[0] : IS-index
p[1] : id (fuer Tape)
*/
errcode CreateIS(ems_u32* p, unsigned int num)
{
    struct IS *is;
    ems_u32 idx;
    int i;

    T(CreateIS)
    D(D_REQ, printf("CreateIS\n");)
    if (num!=2)
        return Err_ArgNum;
    D(D_REQ, printf("  Index: %d\n", p[0]);)
    D(D_REQ, printf("  Ident(fuer tape): %d\n", p[1]);)
    idx=p[0];
    if ((idx>=MAX_IS) || (idx==0))
        return Err_IllIS;
    if (is_list[idx])
        return Err_ISDef;

    if (!(is=(struct IS*)malloc(sizeof(struct IS))))
        return Err_NoMem;
    is->members=0;

#ifdef ISVARS
    is->isvar.var=0;
#endif

#if defined (ROVARS) && defined (READOUT_CC)
    for (i=0; i<MAX_TRIGGER; i++)
        is->rovar[i].var=0;
#endif

#ifdef READOUT_CC
    for (i=0; i<MAX_TRIGGER; i++)
        is->readoutlist[i]=0;
#endif
    is->enabled=0;
    is->id=p[1];
    is->importance=0;
    is->decoding_hints=0;

    is_list[idx]=is;
    notifystatus(status_action_create, Object_is, 1, &idx);
    return OK;
}
/*****************************************************************************/
/*
DeleteIS
p[0] : IS-index
*/
errcode DeleteIS(ems_u32* p, unsigned int num)
{
    ems_u32 idx;

    T(DeleteIS)
    D(D_REQ, printf("DeleteIS\n");)
    if (num!=1)
        return Err_ArgNum;
#ifdef READOUT_CC
    if (readout_active)
        return Err_PIActive;
#endif /* READOUT */
    D(D_REQ, printf("  Index: %d\n", p[0]);)
    idx=p[0];
    if (idx>=MAX_IS)
        return Err_IllIS;
    if (!is_list[idx])
        return Err_NoIS;

    delete_is(idx);

    notifystatus(status_action_delete, Object_is, 1, &idx);
    return OK;
}
/*****************************************************************************/
/*
DownloadISModulList
p[0] : IS-index
p[1] : number of modules
p[2] : addr
...
*/
#ifdef DOM_ML
errcode DownloadISModulList(ems_u32* p, unsigned int num)
{
    ems_u32 idx;
    unsigned int i;
    unsigned int len;

    T(DownloadISModulList)
    D(D_REQ, printf("DownloadISModulList");)

    if ((num<2) || (num!=p[1]+2))
        return Err_ArgNum;
#ifdef READOUT_CC
    if (readout_active)
        return Err_PIActive;
#endif

    D(D_REQ, printf("  index: %d\n", p[0]);)
    D(D_REQ, printf("  Modulanzahl: %d\n", p[1]);)
    D(D_REQ, for (i=0; i<p[1]; i++) printf("  idx %d\n", p[i+2]);)
    idx=p[0];
    len=p[1]+1;
    if (idx>=MAX_IS) return Err_IllIS;
    if (!is_list[idx]) return Err_NoIS;
    if (!modullist) return Err_NoDomain;
    for (i=1; i<len; i++) {
        if (p[i+1]>=modullist->modnum) return Err_ArgRange;
    }
    if (is_list[idx]->members) free(is_list[idx]->members);
    if (!(is_list[idx]->members=
            (unsigned int*)calloc(len, sizeof(unsigned int))))
        return Err_NoMem;
    for (i=0; i<len; i++) is_list[idx]->members[i]=p[i+1];
    notifystatus(status_action_change, Object_is, 1, &idx);
    return OK;
}
#endif
/*****************************************************************************/
/*
UploadISModulList
p[0] : IS-index
*/
#ifdef DOM_ML
errcode UploadISModulList(ems_u32* p, unsigned int num)
{
unsigned int *ptr;
int i, len;

T(UploadISModulList)
D(D_REQ, printf("UploadISModulList");)
if (num!=1) return(Err_ArgNum);
D(D_REQ, printf("  index: %d\n", p[0]);)
if (p[0]>=MAX_IS) return(Err_IllIS);
if (!is_list[p[0]]) return(Err_NoIS);
ptr=is_list[p[0]]->members;
if (ptr==0) {
  *outptr++=0;
} else {
  len=ptr[0];
  for (i=0; i<=len; i++) *outptr++=ptr[i];
}
return(OK);
}/* UploadISModulList */
#endif
/*****************************************************************************/
/*
DeleteISModulList
p[0] : IS-index
*/
#ifdef DOM_ML
errcode DeleteISModulList(ems_u32* p, unsigned int num)
{
T(DeleteISModulList)
D(D_REQ, printf("DeleteISModulList");)
if (num!=1) return(Err_ArgNum);
D(D_REQ, printf("  index: %d\n", p[0]);)
if (p[0]>=MAX_IS) return(Err_IllIS);
if (!is_list[p[0]]) return(Err_NoIS);
if (is_list[p[0]]->members==0) return(Err_NoISModulList);
free(is_list[p[0]]->members);
is_list[p[0]]->members=0;
notifystatus(status_action_change, Object_is, 1, p);
return(OK);
}/* DeleteISModulList */
#endif
/*****************************************************************************/
/*
 * trigg is the trigger pattern to be checked
 * p is a pointer to a trigger pattern description
 * p[0]: either 0 or the first trigger pattern
 * if (p[0]==0) {
 *     trigger is accepted if ((p[1] & trigg) && !(p[2] & trigg))
 * } else {
 *     p[0..] contains a plain list of trigger patterns
 * }
 * num is the number of words in p[]
 *
 * it returns 1 if a trigg would be accepted
 */
static int
trig_is_accepted(ems_u32 trigg, ems_u32 *p, int num)
{
    ems_u32 a, b;
    int i;

    if (!trigg || !num) /* trivial case */
        return 0;

    if (p[0])
        goto list_version;

    /* the following matching logic is not stable yet and may change
       therefore it is more save to call this procedure instead of doing
       the logic elsewhere
    */

    if (num<2) /* error */
        return 0;
    a=p[1];
    b=num>2?p[2]:0;

    return (a & trigg) && !(b & trigg);


list_version:
    /* it is a very bad idea to use this code here; it is much better to use
       a simple loop over the list of pattern in the calling procedure (and
       not to call this procedure)
    */
    for (i=0; i<num; i++) {
        if (p[i]==trigg)
            return 1;
    }    
    return 0;
}
/*****************************************************************************/
#if 0
#ifdef READOUT_CC
/*
DownloadReadoutList
p[0] : IS-index
p[1] : No. of triggerpatterns
p[2] : trigger
...
p[?] : priority
p[?] : procedure list
...
*/
errcode DownloadReadoutList(ems_u32* p, unsigned int num)
{
    ems_u32 *ptr, idx;
    unsigned int numtrig;
    int i, prior;

    T(DownloadReadoutList)
    D(D_REQ, printf("DownloadReadoutlist\n");)
    if (num<1)
        return(Err_ArgNum);
    if (readout_active)
        return(Err_PIActive);
    idx= *p++;
    numtrig= *p++;
    D(D_REQ, printf("  IS: %d\n", idx);)
    D(D_REQ, printf("  Anzahl der Triggerpattern: %d\n", numtrig);)
    if ((!idx)||(idx>=MAX_IS))
        return(Err_IllIS);
    if (!is_list[idx])
        return(Err_NoIS);
    if ((numtrig>MAX_TRIGGER-1) || (numtrig<1))
        return(Err_ArgRange);
    for (i=1; i<numtrig; i++) {
        int j;
        DV(D_REQ, printf("  test trigger %d\n", p[i]);)
        if ((unsigned int)p[i]>=MAX_TRIGGER)
            return(Err_IllTrigPatt);
        for (j=i+1; j<numtrig; j++) {
            DV(D_REQ, printf("  test trigger %d<-->%d\n", p[i], p[j]);)
            if (p[i]==p[j]) return(Err_ArgRange);
        }
    }
    DV(D_REQ, printf("  alloziere %d integers\n", num-(numtrig+3));)
    if (!(ptr=(ems_u32*)calloc(num-=(numtrig+3), sizeof(ems_u32))))
        return(Err_NoMem);
    prior= *(p+numtrig);
    for (i=0; i<numtrig; i++) {
        ems_u32* old_list;
        int cnt, j, pat;
        pat= *p++;
        DV(D_REQ, printf("  bearbeite Trigger %d\n", pat);)
        old_list=is_list[idx]->readoutlist[pat];
        is_list[idx]->readoutlist[pat]=ptr;
        is_list[idx]->prioritaet[pat]=prior;
        is_list[idx]->listenlaenge[pat]=num;
        for (j=0,cnt=0;j<MAX_TRIGGER;j++)
            if (is_list[idx]->readoutlist[j]==old_list) cnt++;
        if (!cnt)
            free(old_list);
    }
    D(D_REQ, printf("Prioritaet: %d\n",*p);)
    DV(D_REQ, printf("  Liste:");)
    DV(D_REQ, for (i=0; i<num; i++) printf("%d%s", p[i], i+1<num?", ":"\n");)
    while (num--)
        *ptr++= *++p;
    is_list[idx]->enabled=1;
    notifystatus(status_action_change, Object_is, 1, &idx);
    return OK;
}
#endif /* READOUT_CC */
#else
/*
DownloadReadoutList
p[0]         : IS-index
p[1]         : No. of triggerpatterns
p[2...]      : triggerlist
p[2+n_trigg] : priority
p[3+n_trigg] : procedure list
...
*/
errcode DownloadReadoutList(ems_u32* p, unsigned int num)
{
    struct IS *is;
    struct readoutlist *readoutlist;
    ems_u32 idx, *list, *tlist;
    unsigned int listlen, priority;
    unsigned int numtrig, i;

    if (num<2)
        return Err_ArgNum;
    if (readout_active)
        return Err_PIActive;
    idx = p[0];
    numtrig = p[1];
    tlist = p+2; /* triggerlist */
    priority = p[2+numtrig];
    list = p+3+numtrig; /* readoutlist */
    listlen = num-3-numtrig;

    if (!idx || idx>=MAX_IS)
        return Err_IllIS;
    if (!is_list[idx])
        return Err_NoIS;
    is=is_list[idx];

    if (!numtrig)
        return Err_ArgNum;

    readoutlist = malloc(sizeof(struct readoutlist));
    if (!readoutlist)
        return Err_NoMem;
    readoutlist->list=malloc((listlen)*sizeof(ems_u32));
    if (!readoutlist->list) {
        free(readoutlist);
        return Err_NoMem;
    }
    readoutlist->length=listlen;
    readoutlist->priority=priority;
    readoutlist->refcount=0;
    for (i=0; i<listlen; i++) {
        readoutlist->list[i]=list[i];
    }

    if (tlist[0]) { /* tlist is a list of trigger IDs to be accepted */
        for (i=0; i<numtrig; i++) {
            if (tlist[i]>=MAX_TRIGGER) {
                free_readoutlist(readoutlist);
                return Err_IllTrigPatt;
            }
        }
        for (i=0; i<numtrig; i++) {
            if (is->readoutlist[tlist[i]])
                unlink_readoutlist(is->readoutlist[tlist[i]]);
            is->readoutlist[tlist[i]]=readoutlist;
            readoutlist->refcount++;
        }
    } else { /* for the logic see trig_is_accepted */
        for (i=0; i<MAX_TRIGGER; i++) {
            if (trig_is_accepted(i, tlist, numtrig)) {
                if (is->readoutlist[i])
                    unlink_readoutlist(is->readoutlist[i]);
                is->readoutlist[i]=readoutlist;
                readoutlist->refcount++;
            }
        }
    }

    if (!readoutlist->refcount)
        free_readoutlist(readoutlist);

    is_list[idx]->enabled=1;
    notifystatus(status_action_change, Object_is, 1, &idx);

    return OK;
}

#endif
/*****************************************************************************/
#ifdef READOUT_CC
/*
UploadReadoutList
p[0] : IS-index
p[1] : triggernummer
*/
errcode UploadReadoutList(ems_u32* p, unsigned int num)
{
    struct IS *is;
    struct readoutlist *readoutlist;
    int i, cnt;
    ems_u32 *list;

    T(UploadReadoutList)
    D(D_REQ, printf("UploadReadoutlist\n");)
    if (num!=2)
        return Err_ArgNum;
    D(D_REQ, printf("  IS: %d\n", p[0]);)
    D(D_REQ, printf("  Trigger: %d\n", p[1]);)
    if (p[0]>=MAX_IS)
        return Err_IllIS;
    is=is_list[p[0]];
    if (!is)
        return Err_NoIS;
    if (p[1]>=MAX_TRIGGER)
        return Err_IllTrigPatt;
    readoutlist=is->readoutlist[p[1]];
    if (!readoutlist)
        return Err_NoReadoutList;
    list=readoutlist->list;
    *outptr++=readoutlist->priority;
    *outptr++=cnt= *list++;        /* Anzahl der Prozeduren */
    DV(D_REQ, printf("  Prozeduren:%d\n", cnt);)
    while (cnt--) {
        *outptr++= *list++;        /* Prozedur-id */
        *outptr++=i= *list++;      /* Anzahl der Parameter */
        while (i--) *outptr++= *list++; /* Parameter */
    }
    return OK;
}
#endif /* READOUT_CC */
/*****************************************************************************/
#ifdef READOUT_CC
/*
old version:
DeleteReadoutList
p[0] : IS-index
p[1] : trigger pattern
new version:
DeleteReadoutList
p[0] : IS-index
p[1...] : list of trigger pattern
          or: p[1]==0: see trig_is_accepted
*/
errcode DeleteReadoutList(ems_u32* p, unsigned int num)
{
    struct IS* is;
    unsigned int i;

    T(DeleteReadoutList)
    D(D_REQ, printf("DeleteReadoutList\n");)
    if (num<2)
        return Err_ArgNum;
    if (readout_active)
        return Err_PIActive;
    D(D_REQ, printf("  IS: %d\n", p[0]);)
    if (p[0]>=MAX_IS)
        return Err_IllIS;
    is=is_list[p[0]];
    if (!is)
        return Err_NoIS;

    if (p[1]) {
        for (i=1; i<num; i++) {
            if (p[i]>=MAX_TRIGGER)
                return Err_IllTrigPatt;
        }
        for (i=1; i<num; i++) {
            if (is->readoutlist[p[i]])
                unlink_readoutlist(is->readoutlist[p[i]]);
        }
    } else {
        if (num<3)
            return Err_ArgNum;
        for (i=0; i<MAX_TRIGGER; i++) {
            if (trig_is_accepted(i, p+1, num-1)) {
                if (is->readoutlist[i]) {
                    unlink_readoutlist(is->readoutlist[i]);
                    is->readoutlist[i]=0;
                }
            }
        }
    }

    notifystatus(status_action_change, Object_is, 1, p);
    return OK;
}
#endif  /* READOUT_CC */
/*****************************************************************************/
#ifdef READOUT_CC
/*
EnableIS
p[0] : IS-index
*/
errcode EnableIS(ems_u32* p, unsigned int num)
{
T(EnableIS)
D(D_REQ, printf("EnableIS\n");)
if (num!=1) return(Err_ArgNum);
D(D_REQ, printf("  IS: %d\n", p[0]);)
if (*p>=MAX_IS) return(Err_IllIS);
if (!is_list[*p]) return(Err_NoIS);
is_list[*p]->enabled=1;
notifystatus(status_action_enable, Object_is, 1, p);
return(OK);
}
#endif /* READOUT_CC */
/*****************************************************************************/
#ifdef READOUT_CC
/*
DisableIS
p[0] : IS-index
*/
errcode DisableIS(ems_u32* p, unsigned int num)
{
T(DisableIS)
D(D_REQ, printf("DisableIS\n");)
if (num!=1) return(Err_ArgNum);
D(D_REQ, printf("  IS: %d\n", p[0]);)
if (*p>=MAX_IS) return(Err_IllIS);
if (!is_list[*p]) return(Err_NoIS);
is_list[*p]->enabled=0;
notifystatus(status_action_disable, Object_is, 1, p);
return(OK);
}
#endif /* READOUT_CC */
/*****************************************************************************/
#ifdef READOUT_CC
/*
GetISStatus
p[0] : IS-index
*/
errcode GetISStatus(ems_u32* p, unsigned int num)
{
    ems_u32* help;
    int i;
    struct IS* is;

    T(GetISStatus)
    D(D_REQ, printf("GetISStatus\n");)
    if (num!=1) return(Err_ArgNum);
    D(D_REQ, printf("  IS: %d\n", p[0]);)
    if (*p>=MAX_IS) return(Err_IllIS);
    is=is_list[p[0]];
    if (!is) return(Err_NoIS);
    *outptr++=is->id;
    *outptr++=(is->enabled?1:0);
    *outptr++=(is->members?1:0);
    help=outptr++;
    for (i=0; i<MAX_TRIGGER; i++)
        if (is->readoutlist[i]) *outptr++=i;
    *help=outptr-help-1;
    return(OK);
}
#endif /* READOUT_CC */
/*****************************************************************************/
/*
GetISParams
p[0] : IS-index
*/
/* scheint zu nichts gut zu sein. */
errcode GetISParams(ems_u32* p, unsigned int num)
{
struct IS* is;

T(GetISParams)
D(D_REQ, printf("GetISParams\n");)
if (num!=1) return(Err_ArgNum);
D(D_REQ, printf("  IS: %d\n", p[0]);)
if (*p>=MAX_IS) return(Err_IllIS);
if ((is=is_list[p[0]])==0) return(Err_NoIS);
return(Err_NotImpl);
}

/*****************************************************************************/
#ifdef ISVARS
plerrcode allocisvar(int size)
{
T(allocisvar)
D(D_USER, printf("allocisvar(%d)\n", size);)
if (!isvar) return(plErr_Other);
if (isvar->var) return(plErr_VarDef);
isvar->var=(int*)malloc(size*sizeof(int));
if (isvar->var==0) return(plErr_NoMem);
isvar->size=size;
return(plOK);
}
#endif /* ISVARS */
/*****************************************************************************/
#ifdef ISVARS
plerrcode reallocisvar(int size)
{
T(reallocisvar)
D(D_USER, printf("reallocisvar(%d)\n", size);)
if (!isvar) return(plErr_Other);
if (isvar->var) free(isvar->var);
isvar->var=(int*)malloc(size*sizeof(int));
if (isvar->var==0) return(plErr_NoMem);
isvar->size=size;
return(plOK);
}
#endif /* ISVARS */
/*****************************************************************************/
#ifdef ISVARS
plerrcode freeisvar(void)
{
T(freeisvar)
D(D_USER, printf("freeisvar()\n");)
if (!isvar) return(plErr_Other);
if (isvar->var) free(isvar->var);
return(plOK);
}
#endif /* ISVARS */
/*****************************************************************************/
#ifdef ISVARS
int isvarsize(void)
{
T(isvarsize)
D(D_USER, printf("isvarsize()\n");)
if (!isvar) return(plErr_Other);
if (isvar->var)
  return(isvar->size);
else
  return(0);
return(OK);
}
#endif /* ISVARS */
/*****************************************************************************/
extern objectcommon is_obj; /* forward */

objectcommon *lookup_is(ems_u32* id, unsigned int idlen, unsigned int* remlen)
{
if (idlen>0)
  {
  if ((unsigned int)id[0]<MAX_IS)
    {
    *remlen=idlen;
    return(&is_obj);
    }
  else
    return(0);
  }
*remlen=0;
return(&is_obj);
}
/*****************************************************************************/
static ems_u32*
dir_is(ems_u32* ptr)
{
int i;
for (i=0; i<MAX_IS; i++)
  if(is_list[i]!=0) *ptr++=i;
return(ptr);
}
/*****************************************************************************/
objectcommon is_obj={
  0,
  0,
  lookup_is,
  dir_is,
  0
};
/*****************************************************************************/
/*****************************************************************************/
