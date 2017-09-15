/*
 * lowlevel/fastbus/sis3100_sfi/dspprocs.h
 * 
 * created 2004-03-25 PW
 *
 * $ZEL: dspprocs.h,v 1.1 2004/04/02 13:50:44 wuestner Exp $
 */

#ifndef _dspprocs_h_
#define _dspprocs_h_

#include <sys/types.h>

struct fastbus_dev;

struct dsp_procs {
    int (*present)(struct fastbus_dev*);
    int (*reset)(struct fastbus_dev*);
    int (*start)(struct fastbus_dev*);
    int (*load)(struct fastbus_dev*, const char*);
};

struct mem_procs {
    int (*size)(struct fastbus_dev*);
    int (*write)(struct fastbus_dev*, u_int32_t* buffer, int size, u_int32_t addr);
    int (*read)(struct fastbus_dev*, u_int32_t* buffer, int size, u_int32_t addr);
    int (*read_delayed)(struct fastbus_dev*, u_int32_t* buffer, int size,
            u_int32_t addr, u_int32_t nextbuffer);
    void (*sis3100_mem_set_bufferstart)(struct fastbus_dev*, u_int32_t, u_int32_t);
};

#endif
