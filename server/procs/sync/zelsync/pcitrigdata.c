/*
 * pcitrigdata.c
 * created 04.11.96 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: pcitrigdata.c,v 1.9 2011/04/06 20:30:34 wuestner Exp $";

#include <debug.h>
#include <errno.h>
#include <errorcodes.h>
#include <sys/time.h>
#include <sys/types.h>
#include <rcs_ids.h>
#include "../../../lowlevel/sync/pci_zel/sync_pci_zel.h"
#include "../../../trigger/trigger.h"
#include "../../../trigger/pci/zelsync/zelsynctrigger.h"
#include "../../procs.h"
#include "../../procprops.h"

extern ems_u32* outptr;
extern int wirbrauchen;
/*extern int wrote_cluster;*/
extern int suspensions;
static int old_suspensions=0, after_suspension=0;

RCS_REGISTER(cvsid, "procs/sync/zelsync")

/*****************************************************************************/
plerrcode proc_GetPCITrigData(ems_u32* p)
{
    struct trigstatus pci_trigdata, pci_oldtrigdata;
    int i, n;

    T(proc_GetPCITrigData)
    if (get_pci_trigdata(&trigger, &pci_trigdata, &pci_oldtrigdata)<0)
        return plErr_BadModTyp;
    n=sizeof(struct trigstatus)/sizeof(unsigned int);
    for (i=0; i<n; i++)
        *outptr++=((unsigned int*)&pci_oldtrigdata)[i];
#ifdef PROFILE
    n=sizeof(struct trigprofdata_u)/sizeof(unsigned int);
    for (i=0; i<n; i++)
        *outptr++=((unsigned int*)&pci_oldprofdata)[i];
#else
    *outptr++=0;
    *outptr++=0;
#endif
    *outptr++=3;
    *outptr++=/*wrote_cluster++*/ 0;
    *outptr++=suspensions;
    if (old_suspensions!=suspensions) {
        after_suspension=0;
        old_suspensions=suspensions;
    }
    *outptr++=after_suspension++;
    return plOK;
}

plerrcode test_proc_GetPCITrigData(ems_u32* p)
{
    if (p[0]!=0) return plErr_ArgNum;
    return plOK;
}

#ifdef PROCPROPS
static procprop GetPCITrigData_prop={0,
    sizeof(struct trigstatus)/sizeof(unsigned int)
#ifdef PROFILE
      +sizeof(struct trigprofdata_u)/sizeof(unsigned int)
#endif
    , "void",
    "gibt Statistik des ZEL-PCI-Triggers zurueck"};

procprop* prop_proc_GetPCITrigData()
{
    return(&GetPCITrigData_prop);
}
#endif

char name_proc_GetPCITrigData[]="GetPCITrigData";
int ver_proc_GetPCITrigData=1;
/*****************************************************************************/
plerrcode proc_GetPCITrigTime(ems_u32* p)
{
    struct trigstatus pci_trigdata, pci_oldtrigdata;
    struct timeval t;

    T(proc_GetPCITrigTime)
    gettimeofday(&t, 0);
    if (get_pci_trigdata(&trigger, &pci_trigdata, &pci_oldtrigdata)<0)
        return plErr_BadModTyp;

    *outptr++=t.tv_sec;
    *outptr++=t.tv_usec;

    *outptr++=pci_oldtrigdata.tdt;
    *outptr++=pci_oldtrigdata.reje;

    return plOK;
}

plerrcode test_proc_GetPCITrigTime(ems_u32* p)
{
    if (p[0]!=0) return plErr_ArgNum;
    wirbrauchen=4;
    return plOK;
}
#ifdef PROCPROPS
static procprop GetPCITrigTime_prop={0,
    sizeof(struct trigstatus)/sizeof(unsigned int)
    , "void",
    "gibt DeadTime und Uhrzeit zurueck"};

procprop* prop_proc_GetPCITrigTime()
{
    return(&GetPCITrigTime_prop);
}
#endif
char name_proc_GetPCITrigTime[]="GetPCITrigTime";
int ver_proc_GetPCITrigTime=1;
/*****************************************************************************/
plerrcode proc_GetPCITrigTime_2(ems_u32* p)
{
    struct trigstatus pci_trigdata, pci_oldtrigdata;
struct timeval t;

T(proc_GetPCITrigTime)
gettimeofday(&t, 0);
    if (get_pci_trigdata(&trigger, &pci_trigdata, &pci_oldtrigdata)<0)
        return plErr_BadModTyp;

*outptr++=t.tv_sec;
*outptr++=t.tv_usec;

*outptr++=pci_oldtrigdata.tdt;
*outptr++=pci_oldtrigdata.reje;

return plOK;
}

plerrcode test_proc_GetPCITrigTime_2(ems_u32* p)
{
if (p[0]!=0) return plErr_ArgNum;
wirbrauchen=4;
return plOK;
}
#ifdef PROCPROPS
static procprop GetPCITrigTime_2_prop={0,
    sizeof(struct trigstatus)/sizeof(unsigned int)
    , "void",
    "gibt DeadTime und Uhrzeit zurueck"};

procprop* prop_proc_GetPCITrigTime_2()
{
return(&GetPCITrigTime_2_prop);
}
#endif
char name_proc_GetPCITrigTime_2[]="GetPCITrigTime";
int ver_proc_GetPCITrigTime_2=2;
/*****************************************************************************/
/*****************************************************************************/
