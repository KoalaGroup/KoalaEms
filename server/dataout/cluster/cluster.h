/*
 * dataout/cluster/cluster.h
 * created      10.04.97
 * 
 * $ZEL: cluster.h,v 1.12 2011/08/18 22:30:42 wuestner Exp $
 */

#ifndef _cluster_h_
#define _cluster_h_

#include <sconf.h>
#include <errorcodes.h>
#include <sys/types.h>
#include <emsctypes.h>

/*
 * Aufbau eines Clusters: (Element 'data' in struct Cluster)
 * 
 * [0] size (Anzahl aller folgenden Worte)
 * [1] endientest (==0x12345678)
 * [2] type (0: events; 1...: userrecords; 0x10000000: no_more_data)
 * [3] optionsize  (z.B. 4)
 * [4, wenn optionsize!=0] optionflags (1: timestamp 2: checksum 4: ...)
 * [5, wenn vorhanden] timestamp (struct timeval)
 * [5 o. 7, wenn vorhanden] checksum
 * ab hier nur fuer events gueltig
 * [4+optsize] flags (fragmentiert...)
 * [5+optsize] ved_id
 * [6+optsize] fragment_id
 * [7+optsize] num_events
 * [8+optsize] size of first event
 * [9+optsize] first event_id
 * ...
 */
/*
 * final cluster:
 * [0] size=3
 * [1] endientest (==0x12345678)
 * [2] type=no_more_data
 * [3] optionsize= 0
 * oder
 * [0] size=5
 * [1] endientest (==0x12345678)
 * [2] type=no_more_data
 * [3] optionsize= 3
 * [4] optionflags =timestamp
 * [5] tv.tv_sec
 * [6] tv.tv_usec
 */
/*
 * VED cluster: (alte Version)
 * [0] size=6+isnum
 * [1] endientest (==0x12345678)
 * [2] type=ved_info
 * [3] optionsize
 * ... options
 * [4+optsize] number of VEDs
 * [5+optsize] number of ISs
 * [] is-ID ...
 */
/*
 * VED cluster: (neue Version)
 * [0] size=6+isnum
 * [1] endientest (==0x12345678)
 * [2] type=ved_info
 * [3] optionsize
 * ... options
 * [4+optsize] versionnumber (bit 31=1)
 * [5+optsize] number of VEDs
 * [6+optsize] VEDid
 * [7+optsize] number of ISs
 * [         ] is-ID ...
 * [         ] VEDid
 * [         ] number of ISs
 * [         ] is-ID ...
 * ...
 */

#include <clusterformat.h> /* in ems/common */

struct Cluster {
    struct Cluster* prev;
    struct Cluster* next;
#ifdef READOUT_CC
    ems_u32 triggers;
#else
    /*int di_idx;*/
#endif
    int numcostumers;
    int costumers[MAX_DATAOUT];
    int predefined_costumers;
    /* size ist hier einschliesslich Laengenwort gemeint */
    int size, type, flags, firstevent, numevents, swapped, optsize, ved_id;
    ems_u32 data[1]; /* eigentlich variable Laenge */
};
#define HEADSIZE (sizeof(struct Cluster)/sizeof(ems_u32))

extern struct Cluster* clusters;
extern int cluster_max;

errcode clusters_init(char*);
errcode clusters_reset(void);
#ifdef READOUT_CC
struct Cluster* clusters_create(size_t, const char* text);
#else
struct Cluster* clusters_create(size_t, int, const char* text);
#endif
void clusters_recycle(struct Cluster*);
void clusters_deposit(struct Cluster*);
void notify_do_done(int);
/*int get_clustersize(void);*/

#endif

/*****************************************************************************/
/*****************************************************************************/
