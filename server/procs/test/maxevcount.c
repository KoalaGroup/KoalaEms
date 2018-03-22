/*
 * procs/test/maxevcount.c
 * created 04.11.94 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: maxevcount.c,v 1.10 2017/10/25 21:13:02 wuestner Exp $";

#include <errorcodes.h>
#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "../procprops.h"
#include "../procs.h"

extern ems_u32* outptr;
#ifdef MAXEVCOUNT
extern unsigned int maxevcount;
extern unsigned int maxevinc;
#endif

RCS_REGISTER(cvsid, "procs/test")

/*****************************************************************************/
/*
 * MaxevCount
 * p[0]: argcount (0||1)
 * [p[1]: maximum event count]
 */
plerrcode proc_MaxevCount(__attribute__((unused)) ems_u32* p)
{
#ifdef MAXEVCOUNT
    *outptr++=maxevcount;
    if (p[0]>0)
        maxevcount=p[1];
#endif
    return plOK;
}

plerrcode test_proc_MaxevCount(__attribute__((unused)) ems_u32* p)
{
    plerrcode res=plOK;
#ifdef MAXEVCOUNT
    switch (p[0]) {
        case 0: break;
        case 1: break;
        default: res=plErr_ArgNum; break;
    }

#else
    res=plErr_NoSuchProc;
#endif
    return res;
}

#ifdef PROCPROPS
static procprop MaxevCount_prop={0, 1, "void", 0};

procprop* prop_proc_MaxevCount(void)
{
    return &MaxevCount_prop;
}
#endif
char name_proc_MaxevCount[]="MaxevCount";
int ver_proc_MaxevCount=1;

/*****************************************************************************/
/*
 * MaxevInc
 * p[0]: argcount (0||1)
 * [p[1]: maximum event count]
 */
plerrcode proc_MaxevInc(__attribute__((unused)) ems_u32* p)
{
#ifdef MAXEVCOUNT
    *outptr++=maxevinc;
    if (p[0]>0)
        maxevinc=p[1];
#endif
    return plOK;
}

plerrcode test_proc_MaxevInc(__attribute__((unused)) ems_u32* p)
{
    plerrcode res=plOK;
#ifdef MAXEVCOUNT
    switch (p[0]) {
        case 0: break;
        case 1: break;
        default: res=plErr_ArgNum; break;
    }

#else
    res=plErr_NoSuchProc;
#endif
    return res;
}

#ifdef PROCPROPS
static procprop MaxevInc_prop={0, 1, "void", 0};

procprop* prop_proc_MaxevInc(void)
{
    return &MaxevInc_prop;
}
#endif
char name_proc_MaxevInc[]="MaxevInc";
int ver_proc_MaxevInc=1;

/*****************************************************************************/
/*****************************************************************************/
