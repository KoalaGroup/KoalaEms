/*
 * trigger/pci/sharedzelsync/sh_zelsynctrigger.h
 * $ZEL$
 * created 2011-03-17 PW
 */

#ifndef _zelsynctrigger_h_
#define _zelsynctrigger_h_
#include <sconf.h>
#include <debug.h>
#include <emsctypes.h>
#include "../../../trigger/trigger.h"
#include <dev/pci/pcisyncvar.h>

/*****************************************************************************/

int print_sharedzelsync_status(struct triggerinfo*, ems_u32**, int);
int get_pci_sharedtrigdata(struct triggerinfo*,
        struct trigstatus*, struct trigstatus*);
int get_sharedsyncstatist(struct triggerinfo* trinfo,
        ems_u64* zelsyncrejected);

#ifdef SYNCSTATIST
enum syncstatist_types {syncstatist_ldt=1, syncstatist_tdt=2, syncstatist_gap=4}
    syncstatist_types;
struct syncstatistics {
  ems_u32 entries;
  ems_u32 ovl;
  ems_u32 max;
  ems_u32 min;
  ems_u64 sum;
  ems_u64 qsum;
  ems_u32 histsize;
  ems_u32 histscale;
  int* hist;
};

struct syncstatistics* get_sharedsyncstatist_ptr(struct triggerinfo*,
        enum syncstatist_types);
void clear_sharedsyncstatist(struct triggerinfo* trinfo,
        enum syncstatist_types);
plerrcode set_sharedsyncstatist(struct triggerinfo* trinfo,
        enum syncstatist_types,
        int size, int scale);
#endif

#ifdef PCISYNC_PROFILE
/*
(defined in sys/dev/pci/pcisyncvar.h)
struct trigprofdata {
        int flagnum;
        unsigned int flags[4];
        int timenum;
        unsigned int times[7];
};
*/
extern struct trigprofdata sharedpcisync_profdata;
#endif

#endif

/*****************************************************************************/
/*****************************************************************************/
