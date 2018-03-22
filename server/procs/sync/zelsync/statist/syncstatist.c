/*
 * procs/pci/trigger/statist/syncstatist.c
 * created 15.10.97 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: syncstatist.c,v 1.9 2016/05/12 21:49:49 wuestner Exp $";

#include <debug.h>
#include <errno.h>
#include <errorcodes.h>
#include <sys/types.h>
#include <rcs_ids.h>
#include "../../../../lowlevel/sync/pci_zel/sync_pci_zel.h"
#include "../../../../trigger/pci/zelsync/zelsynctrigger.h"
#include "../../../../trigger/trigger.h"
#include "../../../procs.h"
#include "../../../procprops.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/sync/zelsync/statist")

/*****************************************************************************/
#if 0 /* more work needed to make it compatible multiple triggers */
/*
 * clear u. requested:
 * 0 nichts
 * 1 ldt
 * 2 tdt
 * 4 gaps
 * 
 * p[0] == 4
 * p[1] requested (0: nur clear (oder nop))
 * p[2] clear
 * p[3] maxchan (-1: kein Histogramm; 0: alle Kanaele)
 * p[4] scale (0==1)
 */
/*
 * return:
 * int   Gesamtlaenge
 * int   Eventcount
 * int64 rejected Triggers
 * int   Number of following Structures
 *   int   typ (1: ldt, 2: tdt, 3: gaps)
 *   int   Entries
 *   int   Overflows
 *   int   Minimum
 *   int   Maximum
 *   int64 Summe
 *   int64 Summe**2
 *   int   Histogrammsize
 *   int   Histogrammscale (original; nicht p[4])
 *   int   Anzahl der Histogrammwerte, die gesendet werden sollen
 *   [int   Anzahl der fuehrenden Nullen (sie werden nicht gesendet)]
 *   [int[...] Histogrammwerte]
 */
  
plerrcode proc_GetSyncStatist(ems_u32* p)
{
    int l, m, n;
    ems_u32 *hlp0, *hlp1;
    struct syncstatistics* stat;
    signed int p3=(signed int)p[3];
    ems_u64 zelsyncrejected;

    T(proc_GetSyncStatist)

/*
 *     if (!syncstatist_ok)
 *         return plErr_Other;
 */
    if (get_syncstatist(&trigger, &zelsyncrejected)<0)
        return plErr_BadModTyp;


    hlp0=outptr++;
    *outptr++=trigger.eventcnt;
    *outptr++=(zelsyncrejected>>32)&0xffffffffL;
    *outptr++=zelsyncrejected&0xffffffffL;
    hlp1=outptr++;
    n=0;
    for (l=0, m=1; l<3; m<<=1, l++) {
        if ((m&p[1]) && ((stat=get_syncstatist_ptr(&trigger, m))!=0)) {
            n++;
            *outptr++=m;
            *outptr++=stat->entries;
            *outptr++=stat->ovl;
            *outptr++=stat->min;
            *outptr++=stat->max;
            *outptr++=(stat->sum>>32)&0xffffffffL;
            *outptr++=stat->sum&0xffffffffL;
            *outptr++=(stat->qsum>>32)&0xffffffffL;
            *outptr++=stat->qsum&0xffffffffL;
            *outptr++=stat->histsize;
            *outptr++=stat->histscale;
            if (p3>=0) {
                int wanted, scaledwanted, leer, scaledleer, i, j, k;
                wanted=p3?p[4]?p3*p[4]:p3:stat->histsize;
                if (wanted>stat->histsize) wanted=stat->histsize;
                for (leer=0; (leer<wanted) && (stat->hist[leer]==0); leer++);
                if (p[4]==0) {
                    *outptr++=wanted;
                    *outptr++=leer;
                    for (i=leer; i<wanted; i++) *outptr++=stat->hist[i];
                } else {
                    scaledwanted=wanted/p[4];
                    scaledleer=leer/p[4];
                    *outptr++=scaledwanted;
                    *outptr++=scaledleer;
                    for (i=scaledleer, j=scaledleer*p[4]; i<scaledwanted; i++) {
                        int s=stat->hist[j++];
                        for (k=1; k<p[4]; k++) s+=stat->hist[j++];
                        *outptr++=s;
                    }
                }
            } else {
                *outptr++=0;
            }
        }
    }
    if (p[2])
        clear_syncstatist(&trigger, p[2]);
    *hlp1=n;
    *hlp0=outptr-hlp0-1;
    return plOK;
}

plerrcode test_proc_GetSyncStatist(ems_u32* p)
{
    if (p[0]!=4) return plErr_ArgNum;
    return plOK;
}
#ifdef PROCPROPS
static procprop GetSyncStatist_prop={
    1,
    -1,
    "<mask for request> <mask for clear> maxchan scale(0==1)",
    "gibt Statistik des ZEL-PCI-Triggers zurueck"
};

procprop* prop_proc_GetSyncStatist()
{
    return(&GetSyncStatist_prop);
}
#endif
char name_proc_GetSyncStatist[]="GetSyncStatist";
int ver_proc_GetSyncStatist=1;

#endif
/*****************************************************************************/
#if 0 /* more work needed to make it compatible multiple triggers */
/*
 * clear:
 * 0 nichts
 * 1 ldt
 * 2 tdt
 * 4 gaps
 * 
 * p[0] == 1
 * p[1] clear
 */

plerrcode proc_ClearSyncStatist(ems_u32* p)
{
    T(proc_ClearSyncStatist)
    //if (!syncstatist_ok) return plErr_Other;
    clear_syncstatist(&trigger, p[1]);
    return plOK;
}

plerrcode test_proc_ClearSyncStatist(ems_u32* p)
{
    if (p[0]!=1)
        return plErr_ArgNum;
    return plOK;
}
#ifdef PROCPROPS
static procprop ClearSyncStatist_prop={
    1,
    -1,
    "<mask for clear>",
    "loescht Statistik des ZEL-PCI-Triggers"
};

procprop* prop_proc_ClearSyncStatist()
{
    return(&ClearSyncStatist_prop);
}
#endif
char name_proc_ClearSyncStatist[]="ClearSyncStatist";
int ver_proc_ClearSyncStatist=1;

#endif
/*****************************************************************************/
#if 0 /* more work needed to make it compatible multiple triggers */
/*
 * selected:
 * 0 nichts (Unsinn)
 * 1 ldt
 * 2 tdt
 * 4 gaps
 * 
 * p[0] == 3
 * p[1] selected 
 * p[2] histsize
 * p[3] histscale
 */
/*
 * return: nix
 */
  
plerrcode proc_SetSyncStatist(ems_u32* p)
{
    T(proc_GetSyncStatist)

    //if (!syncstatist_ok) return plErr_Other;

    return set_syncstatist(&trigger, p[1], p[2], p[3]);
}

plerrcode test_proc_SetSyncStatist(ems_u32* p)
{
    if (p[0]!=3)
        return plErr_ArgNum;
    return plOK;
}
#ifdef PROCPROPS
static procprop SetSyncStatist_prop={
    1,
    -1,
    "<mask for selection> size scale(0==1)",
    ""
};

procprop* prop_proc_SetSyncStatist()
{
    return(&SetSyncStatist_prop);
}
#endif
char name_proc_SetSyncStatist[]="SetSyncStatist";
int ver_proc_SetSyncStatist=1;

#endif
/*****************************************************************************/
/*****************************************************************************/
