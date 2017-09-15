/*
 * lowlevel/jtag/jtag_lowlevel.h
 * created 2006-Jul_05 PW
 * $ZEL: jtag_lowlevel.h,v 1.4 2008/03/11 20:26:08 wuestner Exp $
 */

#ifndef _jtag_lowlevel_h_
#define _jtag_lowlevel_h_

#include <sconf.h>
#include <emsctypes.h>

/*
 * this is the only information which is exported to other lowlevel libraries
 */

struct jtag_dev {
    struct generic_dev* dev;
    int ci; /* 0: acq card, 1: (crate)controller 2: local interface */
    int addr;
    double sleeptime; /* time (in ns) needed for one basic sleep call */

    int  (*jt_data)(struct jtag_dev* jdev, ems_u32* v);
    int  (*jt_action)(struct jtag_dev* jdev, int tms, int tdi, int* tdo);
    void (*jt_free)(struct jtag_dev* jdev);
    int  (*jt_sleep)(struct jtag_dev* jdev);
    void (*jt_mark)(struct jtag_dev* jdev, int pat);

    void* opaque;
};

#endif
