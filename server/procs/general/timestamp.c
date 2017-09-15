/*
 * procs/general/timestamp.c
 * created 04.11.94   
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: timestamp.c,v 1.11 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>

#if /*defined(unix) ||*/ defined(__unix__) || defined(Lynx) || defined(__Lynx__)
#include <sys/time.h>
#endif

#include <errorcodes.h>
#include <rcs_ids.h>
#include "../procprops.h"
#include "../procs.h"
#include "../../objects/pi/readout.h"
#include "../../trigger/trigger.h"

extern ems_u32* outptr;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/general")

/*****************************************************************************/
/*
 * p[0]: argcount==0|1
 * [p[1]: 0: OS9 format (default) 1: UNIX format ]
 */
plerrcode proc_Timestamp(ems_u32* p)
{
int sekunden, tage, hundertstel;
#ifdef OSK
short wochentag;
#endif

if ((p[0]==0) || (p[1]==0)) /* OS9-Format */
  {
#if /*defined(unix) ||*/ defined(__unix__) || defined(Lynx) || defined(__Lynx__)
  struct timeval t;
  struct timezone tz;
  gettimeofday(&t,&tz);
  hundertstel=t.tv_usec/10000;
  sekunden=t.tv_sec;
  tage=t.tv_sec/86400;
#else
#ifdef OSK
  _sysdate(3, &sekunden, &tage, &wochentag, &hundertstel);
#else
  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#endif
#endif
  *outptr++=tage;
  *outptr++=sekunden;
  *outptr++=hundertstel;
  }
else /* unix-Format */
  {
#ifdef OSK
  return plErr_ArgRange;
#else
  struct timeval t;
  gettimeofday(&t, 0);
  *outptr++=t.tv_sec;
  *outptr++=t.tv_usec;
#endif
  }
return(plOK);
}

plerrcode test_proc_Timestamp(ems_u32* p)
{
if (p[0]>1) return(plErr_ArgNum);
if ((p[0]>0) && ((unsigned int)p[1]>1)) return plErr_ArgRange;
if ((p[0]>0) && (p[1]==1))
  wirbrauchen=2;
else
  wirbrauchen=3;
return(plOK);
}
#ifdef PROCPROPS
static procprop Timestamp_prop={0, 3, "void", 0};

procprop* prop_proc_Timestamp()
{
return(&Timestamp_prop);
}
#endif
char name_proc_Timestamp[]="Timestamp";
int ver_proc_Timestamp=1;

/*****************************************************************************/
/*
 * p[0]: argcount==0|1
 * [p[1]: 0: sec/usec (default) 1: sec/nsec]
 */
char name_proc_Triggertime[]="Triggertime";
int ver_proc_Triggertime=1;

plerrcode proc_Triggertime(ems_u32* p)
{
#ifdef READOUT_CC
    if (!inside_readout)
        return plErr_PINotActive;

    *outptr++=trigger.time.tv_sec;
    if (p[0]>0 && p[1]) {
        *outptr++=trigger.time.tv_nsec;
    } else {
        *outptr++=trigger.time.tv_nsec/1000;
    }
    return plOK;
#else
    return plErr_NotImpl;
#endif
}

plerrcode test_proc_Triggertime(ems_u32* p)
{
#ifdef READOUT_CC
    if (p[0]>1)
        return plErr_ArgNum;
    return plOK;
#else
    return plErr_NotImpl;
#endif
}
/*****************************************************************************/
/*****************************************************************************/
