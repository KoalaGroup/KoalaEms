/*
 * lowlevel/unixvme/dspprocs.h
 * 
 * created 25.Jun.2003 PW
 *
 * $ZEL: dspprocs.h,v 1.4 2005/04/12 18:39:50 wuestner Exp $
 */

#ifndef _dspprocs_h_
#define _dspprocs_h_

#include <sys/types.h>

struct vme_dev;

struct dsp_procs {
    int (*present)(struct vme_dev*);
    int (*reset)(struct vme_dev*);
    int (*start)(struct vme_dev*);
    int (*load)(struct vme_dev*, const char*);
};

struct mem_procs {
    int (*size)(struct vme_dev*);
    int (*write)(struct vme_dev*, ems_u32* buffer, unsigned int size,
            ems_u32 addr);
    int (*read)(struct vme_dev*, ems_u32* buffer, unsigned int size,
            ems_u32 addr);
#ifdef DELAYED_READ
    int (*read_delayed)(struct vme_dev*, ems_u32* buffer, unsigned int size,
            ems_u32 addr, ems_u32 nextbuffer);
#endif
    void (*sis3100_mem_set_bufferstart)(struct vme_dev*, ems_u32, ems_u32);
};

#endif
