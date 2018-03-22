/*
 * procs/internals/cluster/clustproc.c
 * PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: clustproc.c,v 1.5 2017/10/20 23:20:52 wuestner Exp $";

#include <errorcodes.h>

#include <xdrstring.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../dataout/cluster/do_cluster.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/dataout")

/*****************************************************************************/
/*
 * p[0]: argcount==0|1
 * p[1]: maximum number of events in a cluster
 */
char name_proc_ClusterEvMax[]="ClusterEvMax";
int ver_proc_ClusterEvMax=1;

plerrcode proc_ClusterEvMax(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ems_i32 new, old;

    new=ip[0]>0?ip[1]:-1;
    set_max_ev_per_cluster(new, &old);

    *outptr++=old;
    return plOK;
}

plerrcode test_proc_ClusterEvMax(ems_u32* p)
{
    if (p[0]>1)
        return plErr_ArgNum;
    wirbrauchen=1;
    return plOK;
}
/*****************************************************************************/
/*
 * p[0]: argcount==0|1
 * p[1]: maximum elapsed time before a cluster is 'full' (in ms)
 */
char name_proc_ClusterTimeMax[]="ClusterTimeMax";
int ver_proc_ClusterTimeMax=1;

plerrcode proc_ClusterTimeMax(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    ems_i32 new, old;

    new=ip[0]>0?ip[1]:-1;
    set_max_time_per_cluster(new, &old);

    *outptr++=old;
    return plOK;
}

plerrcode test_proc_ClusterTimeMax(ems_u32* p)
{
    if (p[0]>1)
        return plErr_ArgNum;
    wirbrauchen=1;
    return plOK;
}
/*****************************************************************************/
#if 0
XXX
FlushCluster sollte dadurch ersetzt werden, dass bei stopreadout
automatisch die letzten Daten geflusht werden. 
Zusaetzlich sollte eine ems-Prozedur das jederzeit ausloesen koennen
(aber generell auf Daten, nicht auf Cluster bezogen).

char name_proc_FlushCluster[]="FlushCluster";
int ver_proc_FlushCluster=1;

plerrcode proc_FlushCluster(ems_u32* p)
{
    flush_current_cluster();
    return plOK;
}

plerrcode test_proc_FlushCluster(ems_u32* p)
{
    if (p[0])
        return plErr_ArgNum;
    return plOK;
}
#endif
/*****************************************************************************/
/*****************************************************************************/
