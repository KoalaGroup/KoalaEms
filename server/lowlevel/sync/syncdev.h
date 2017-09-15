/*
 * lowlevel/sync/syncdev.h
 * created 2007-Jul-02 PW
 * $ZEL: syncdev.h,v 1.2 2008/03/11 20:28:35 wuestner Exp $
 */

#ifndef _syncdev_h_
#define _syncdev_h_

#include <sconf.h>
#include <stdio.h>
#include <errorcodes.h>
#include <emsctypes.h>
#include "../devices.h"

enum synctypes {sync_none, sync_plx};

struct sync_dev {
/* this first element matches struct generic_dev */
    struct dev_generic generic;

    int (*reset)(struct sync_dev*);
    plerrcode (*get_eventcount)(struct sync_dev*, ems_u32* evc);
    plerrcode (*set_eventcount)(struct sync_dev*, ems_u32 evc);
    plerrcode (*write_reg)(struct sync_dev*, ems_u32 addr, int size, ems_u32 data);
    plerrcode (*read_reg)(struct sync_dev*, ems_u32 addr, int size, ems_u32* data);
    plerrcode (*module_state)(struct sync_dev*, int level);
    plerrcode (*write_plxreg)(struct sync_dev*, ems_u32 addr, ems_u32 data);
    plerrcode (*read_plxreg)(struct sync_dev*, ems_u32 addr, ems_u32* data);

/*
 * DUMMY must be the last 'function'; it is used to check whether all other
 * function pointers are initialised.
 */
    int (*DUMMY)(void);

    enum synctypes synctype;
    char* pathname;
    void* info;
};

plerrcode sync_init(struct sync_dev*, int reset, int num_ids, ems_u32* ids);
plerrcode sync_done(struct sync_dev* dev);

plerrcode sync_init_jtag_dev(struct generic_dev* dev, void* jdev);

#endif
