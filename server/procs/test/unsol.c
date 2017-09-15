/*
 * procs/test/unsol.c
 * 
 * created 04.11.94
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: unsol.c,v 1.16 2011/04/06 20:30:34 wuestner Exp $";


#include <sconf.h>

#if /*defined(unix) ||*/ defined(__unix__)
#include <stdlib.h>
#include <unistd.h>
#endif
#include <errorcodes.h>
#include <unsoltypes.h>
#include <debug.h>
#include <rcs_ids.h>
#ifdef OBJ_VAR
#include "../../objects/var/variables.h"
#endif
#include "../../commu/commu.h"
#ifdef OSK
#include <utime.h>
#else
#include <sys/time.h>
#endif
#include "../procprops.h"
#include "../procs.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

extern ems_u32* outptr;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/test")

/*****************************************************************************/
/*
SendUnsol
*/
plerrcode proc_SendUnsol(ems_u32* p)
{
    unsigned int num;
    int i;
    ems_u32* ptr=0;

    T(proc_SendUnsol)
    D(D_USER, printf("SendUnsol(");
        for (i=1; i<=p[0]; i++) printf("%d%s", p[i], i<p[0]?", ":")\n");)
    num=p[0]-1;
    if ((num>0) && ((ptr=(ems_u32*)malloc(num*sizeof(ems_u32)))==0)) {
        D(D_USER|D_MEM,
            printf("SendUnsol: no memory (try to allocate %d words).\n", num);)
        return(plErr_NoMem);
    }
    for (i=0; i<num; i++) ptr[i]=p[i+2];
    send_unsolicited(p[1], ptr, num);
    if(ptr)free(ptr);
    return(plOK);
}

plerrcode test_proc_SendUnsol(ems_u32* p)
{
T(test_proc_SendUnsol)
if (p[0]<1) return(plErr_ArgNum);
wirbrauchen=0;
return(plOK);
}
#ifdef PROCPROPS
static procprop SendUnsol_prop={0, 0, "<type> ...", 0};

procprop* prop_proc_SendUnsol(void)
{
return(&SendUnsol_prop);
}
#endif
char name_proc_SendUnsol[]="SendUnsol";
int ver_proc_SendUnsol=1;

/*****************************************************************************/
/*
SendUnsolLoop
*/
plerrcode proc_SendUnsolLoop(ems_u32* p)
{
int i;
ems_u32 *ptr=0;

T(proc_SendUnsolLoop)
D(D_USER, printf("SendUnsolLoop(");
  for (i=1; i<=p[0]; i++) printf("%d%s", p[i], i<p[0]?", ":")\n");)
if ((p[0]>1)&&((ptr=(ems_u32*)calloc(p[0]-1, sizeof(ems_u32)))==0))
  {
  D(D_USER|D_MEM,
    printf("SendUnsolLoop: no memory (try to allocate %d words).\n", p[0]);)
  return(plErr_NoMem);
  }
for (i=0; i<p[1]; i++)
  {
  int j;
  for (j=0; j<p[0]-1; j++) ptr[j]=p[j+2];
  send_unsolicited(Unsol_Test, ptr, p[0]-1);
  }
if(ptr)free(ptr);
return(plOK);
}

plerrcode test_proc_SendUnsolLoop(ems_u32* p)
{
T(test_proc_SendUnsolLoop)
if (p[0]<1) return(plErr_ArgNum);
wirbrauchen=0;
return(plOK);
}
#ifdef PROCPROPS
static procprop SendUnsolLoop_prop={0, 0, "<loops> ...", 0};

procprop* prop_proc_SendUnsolLoop(void)
{
return(&SendUnsolLoop_prop);
}
#endif
char name_proc_SendUnsolLoop[]="SendUnsolLoop";
int ver_proc_SendUnsolLoop=1;

/*****************************************************************************/
/*
SendLoopTime
*/
plerrcode proc_SendLoopTime(ems_u32* p)
{
int i, start;
ems_u32 buf[2];
struct timeval tv;
struct timezone tz;

T(proc_SendLoopTime)
gettimeofday(&tv, &tz);
start=tv.tv_sec*100+tv.tv_usec/10000;
for (i=0; i<p[1]; i++)
  {
  buf[0]=i;
  gettimeofday(&tv, &tz);
  buf[1]=tv.tv_sec*100+tv.tv_usec/10000-start;
  send_unsolicited(Unsol_Test, buf, 2);
  }
return(plOK);
}

plerrcode test_proc_SendLoopTime(ems_u32* p)
{
T(test_proc_SendLoopTime)
if (p[0]!=1) return(plErr_ArgNum);
wirbrauchen=0;
return(plOK);
}
#ifdef PROCPROPS
static procprop SendLoopTime_prop={0, 0, "<loops>", 0};

procprop* prop_proc_SendLoopTime(void)
{
return(&SendLoopTime_prop);
}
#endif
char name_proc_SendLoopTime[]="SendLoopTime";
int ver_proc_SendLoopTime=1;

/*****************************************************************************/
/*
SendLoopCount
*/
plerrcode proc_SendLoopCount(ems_u32* p)
{
int i;

T(proc_SendLoopCount)
for (i=0; i<p[1]; i++)
  {
  ems_u32 j;
  j=i;
  send_unsolicited(Unsol_Test, &j, 1);
  }
return(plOK);
}

plerrcode test_proc_SendLoopCount(ems_u32* p)
{
T(test_proc_SendLoopCount)
if (p[0]!=1) return(plErr_ArgNum);
wirbrauchen=0;
return(plOK);
}
#ifdef PROCPROPS
static procprop SendLoopCount_prop={0, 0, "<loops>", 0};

procprop* prop_proc_SendLoopCount(void)
{
return(&SendLoopCount_prop);
}
#endif
char name_proc_SendLoopCount[]="SendLoopCount";
int ver_proc_SendLoopCount=1;

/*****************************************************************************/
/*
SULC
p[0] = num
p[1] = count
p[2] = sleep
*/
plerrcode proc_SULC(ems_u32* p)
{
    int i, count;
    unsigned int sl;
    ems_u32* ptr;

    T(proc_SULC)
    D(D_USER, printf("SULC(");
        for (i=1; i<=p[0]; i++) printf("%d%s", p[i], i<p[0]?", ":")\n");)
    count=p[1]; sl=p[2];
    if ((ptr=(ems_u32*)calloc(p[0]-1, sizeof(ems_u32)))==0) {
        D(D_USER|D_MEM,
        printf("SULC: no memory (try to allocate %d words).\n", p[0]);)
        return(plErr_NoMem);
    }
    for (i=0; i<count; i++) {
        int j;
        ptr[0]=i;
        for (j=1; j<p[0]-1; j++) ptr[j]=p[j+2];
        send_unsolicited(Unsol_Test, ptr, p[0]-1);
        if (sl) sleep(sl);
    }
    free(ptr);
    return(plOK);
}

plerrcode test_proc_SULC(ems_u32* p)
{
T(test_proc_SULC)
if (p[0]<2) return(plErr_ArgNum);
wirbrauchen=0;
return(plOK);
}
#ifdef PROCPROPS
static procprop SULC_prop={0, 0, "<loops> <sleep> ...", 0};

procprop* prop_proc_SULC(void)
{
return(&SULC_prop);
}
#endif
char name_proc_SULC[]="SULC";
int ver_proc_SULC=1;

/*****************************************************************************/

plerrcode proc_SendVarUnsol(ems_u32* p)
{
#ifdef OBJ_VAR
int i;
unsigned int len;
ems_u32 *buf, *ptr;

T(proc_SendVarUnsol)
len=var_list[p[1]].len;
buf=(ems_u32*)calloc(len+2, sizeof(ems_u32));
if (buf==0)
  {
  D(D_USER|D_MEM,
    printf("SendVarUnsol: no memory (try to allocate %d words).\n", len+2);)
  return(plErr_NoMem);
  }
buf[0]=p[2];
buf[1]=len;
ptr=(len==1?&var_list[p[1]].var.val:var_list[p[1]].var.ptr);
for (i=0; i<len; i++) buf[i+2]=ptr[i];
send_unsolicited(Unsol_Data, buf, len+2);
free(buf);
#endif
return(plOK);
}

plerrcode test_proc_SendVarUnsol(ems_u32* p)
{
T(test_proc_SendVarUnsol)
#ifdef OBJ_VAR
if (p[0]!=2) return(plErr_ArgNum);
if ((unsigned)p[1]>MAX_VAR) return(plErr_IllVar);
if (!var_list[p[1]].len) return(plErr_NoVar);
wirbrauchen=0;
return(plOK);
#else
return(plErr_IllVar);
#endif
}
#ifdef PROCPROPS
static procprop SendVarUnsol_prop={0, 0, "<variable> <type>", 0};

procprop* prop_proc_SendVarUnsol(void)
{
return(&SendVarUnsol_prop);
}
#endif
char name_proc_SendVarUnsol[]="SendVarUnsol";
int ver_proc_SendVarUnsol=1;

/*****************************************************************************/
/*
 * p[0] number of arguments (>=2)
 * p[1] severity: 0: green 1: yellow 2: red 3: infradead
 * p[2] I_counter
 * p[3...2+I_counter] arbitrary integer information for automatic error handling
 * pairs of <flags string>
 *     flags:
 *         1: text to log
 *         2: sound to play
 *         4: picture to show
 *         8: program to execute
 *      0x10: addressee of mail
 *      0x20: text of mail
 */
plerrcode proc_SendUnsolAlarm(ems_u32* p)
{
#if 0
    int idx, i;
    ems_u32 *msg, *help;
    struct timeval tv;

    gettimeofday(&tv);
    nr_pairs=
    msg=malloc(p[0]+4);
    if (!msg) {
        return plErr_NoMem;
    }
    idx=0;
    msg[idx++]=1; /* version */
    msg[idx++]=tv.tv_sec;
    msg[idx++]=tv.tv_usec;
    msg[idx++]=p[1]; /* severity */
    msg[idx++]=p[2]; /* I_counter */
    for (i=0; i<p[2]; i++)
        msg[idx++]=p[i+2];
    help=msg+idx;
    *help=0; /* S_counter */
    pp=p+2+p[2];
    while (pp+2<=p+p[0]) {
        msg[idx++]=*pp++ /* flag */
        XXXXXXXXXXXXX
    }

    T(proc_SendUnsol)
    D(D_USER, printf("SendUnsol(");
        for (i=1; i<=p[0]; i++) printf("%d%s", p[i], i<p[0]?", ":")\n");)
    num=p[0]-1;
    if ((num>0) && ((ptr=(ems_u32*)malloc(num*sizeof(ems_u32)))==0)) {
        D(D_USER|D_MEM,
            printf("SendUnsol: no memory (try to allocate %d words).\n", num);)
        return(plErr_NoMem);
    }
    for (i=0; i<num; i++) ptr[i]=p[i+2];
    send_unsolicited(p[1], ptr, num);
    if(ptr)free(ptr);
    return(plOK);
error:
    free(msg);
    return pres;
#else
    return plOK;
#endif
}

plerrcode test_proc_SendUnsolAlarm(ems_u32* p)
{
    if (p[0]<2)
        return plErr_ArgNum;
    wirbrauchen=0;
    return plOK;
}
char name_proc_SendUnsolAlarm[]="SendUnsolAlarm";
int ver_proc_SendUnsolAlarm=1;

/*****************************************************************************/
/*****************************************************************************/
