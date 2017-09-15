/*
 * procs/proclist.c
 * created: 13.09.94
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: proclist.c,v 1.18 2011/04/06 20:30:29 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "listprocs.h"
#include "../objects/is/is.h"
#include <errorcodes.h>
#include "emsctypes.h"
#include "proclist.h"
#include "../lowlevel/profile/profile.h"
#if PERFSPECT
#include "../lowlevel/perfspect/perfspect.h"
#endif
#include "../trigger/trigger.h"
#include <stdio.h>

RCS_REGISTER(cvsid, "procs")

/*****************************************************************************/

extern ems_u32* outptr;
int *memberlist;
struct IS *current_IS;
#ifdef ISVARS
ISV *isvar;
#endif
int wirbrauchen;
#if PERFSPECT
int perfbedarf;
#define NUMPERFS 100
char* perfnames[NUMPERFS];
#endif

/*****************************************************************************/

errcode test_proclist(ems_u32* p, unsigned int len, int* limit)
{
    plerrcode res;
    ems_u32* max;
    ems_u32* help;
    int anz, i;
    int gesamtbedarf;

    T(test_proclist)
    gesamtbedarf=0;
    res=plOK;
    max=p+len;
    anz= *p++;

#if PERFSPECT
    perfspect_alloc_isdata(anz);
#endif
    for (i=0; i<anz; i++) {
#if PERFSPECT
        int j;
        for (j=0; j<NUMPERFS; j++)
            perfnames[j]=0;
#endif
        help=outptr;
        outptr+=3;
        if ((unsigned int)(*p)>=NrOfProcs) {
            *outptr++= *p;
            res=plErr_NoSuchProc;
        } else {
            if (p+*(p+1)+2>max) {
                *outptr++= *(p+1);
                res=plErr_BadArgCounter;
            } else {
                wirbrauchen= -1;
#if PERFSPECT
                perfbedarf= 0;
#endif
                res=Proc[*p].test_proc(p+1);
            }
        }
        if (res) {
            *help++=i+1;
            *help++=(int)res;
            *help=outptr-help-1;
            return(Err_BadProcList);
        } else {
            outptr=help;
        }
        if ((wirbrauchen!=-1)&&(gesamtbedarf!=-1))
            gesamtbedarf+=wirbrauchen;
        else
            gesamtbedarf= -1;
#if PERFSPECT
        perfspect_alloc_procdata(i, *p, Proc[*p].name_proc,
                perfbedarf, perfnames);
#endif
        p+= *(p+1)+2;
    }
    if (limit) *limit=gesamtbedarf;
    return(OK);
}

/*****************************************************************************/

#ifndef OPTIMIERT

errcode scan_proclist(ems_u32* p)
{
    register int i, num;
    plerrcode res;

    T(scan_proclist)
    i=num= *p++;
    DV(D_PL,printf("%d Aufrufe\n",i);)

    while (i--) {
        DV(D_PL,printf("Aufruf der Prozedur %d mit %d Parametern\n", *p, *(p+1));)
        PROFILE_START(PROF_PROC);
#if PERFSPECT
        perfspect_start_proc(num-1-i);
#endif
        res=Proc[*p].proc(p+1);
        if (res) {
            PROFILE_END(PROF_PROC);
            *outptr++=num-i;
            *outptr++=res;
            return(Err_ExecProcList);
        }
        PROFILE_END(PROF_PROC);
#if PERFSPECT
        perfspect_end_proc();
#endif
        p+= *(p+1)+2;
    }
    return(OK);
}

#endif

/*****************************************************************************/
/*****************************************************************************/
