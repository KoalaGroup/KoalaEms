/*
 * lowlevel/unixvme/vme.h
 * $ZEL: vme.h,v 1.22 2009/03/12 13:25:13 wuestner Exp $
 * created 23.Jan.2001 PW
 */

#ifndef _vme_h_
#define _vme_h_

#include <sconf.h>
#include <emsctypes.h>
#include "../devices.h"
#include "dspprocs.h"

struct vme_dev;

struct vmeirq_callbackdata {
    ems_u32 mask;
    int level;
    ems_u32 vector;
    struct timespec time;
};

typedef void (*vmeirq_callback)(struct vme_dev*,
        const struct vmeirq_callbackdata*,
        void* data);

enum vmetypes {vme_none, vme_drv, vme_sis3100};
/*
 *   all read/write functions return number of BYTES read/written or
 *   (-errno) in case of error
 *   zero bytes read or written is NOT considered an error
 *
 *   probe returns 0 or errno
 *
 */
struct vme_dev {
/* this first element matches struct generic_dev */
    struct dev_generic generic;

    struct dsp_procs*  (*get_dsp_procs)(struct vme_dev*);
    struct mem_procs*  (*get_mem_procs)(struct vme_dev*);

    int (*berrtimer)   (struct vme_dev*, int); /* nsec*/
    int (*arbtimer)    (struct vme_dev*, int); /* nsec*/

    int (*read)        (struct vme_dev*, ems_u32 addr, int am,
            ems_u32* data, int size, int datasize, ems_u32** newdata);
    int (*write)       (struct vme_dev*, ems_u32 addr, int am,
            ems_u32* data, int size, int datasize);

    int (*read_a32)    (struct vme_dev*, ems_u32, ems_u32*, int, int, ems_u32**);
    int (*write_a32)   (struct vme_dev*, ems_u32, ems_u32*, int, int);
    int (*read_a24)    (struct vme_dev*, ems_u32, ems_u32*, int, int, ems_u32**);
    int (*write_a24)   (struct vme_dev*, ems_u32, ems_u32*, int, int);

    int (*read_a32_fifo)(struct vme_dev*, ems_u32 addr, ems_u32* data,
            int size, int datasize, ems_u32** newdata);
    int (*write_a32_fifo)(struct vme_dev*, ems_u32 addr, ems_u32* data,
            int size, int datasize);

    int (*read_a32d32) (struct vme_dev*, ems_u32, ems_u32*);
    int (*write_a32d32)(struct vme_dev*, ems_u32, ems_u32);
    int (*read_a32d16) (struct vme_dev*, ems_u32, ems_u16*);
    int (*write_a32d16)(struct vme_dev*, ems_u32, ems_u16);
    int (*read_a32d8)  (struct vme_dev*, ems_u32, ems_u8*);
    int (*write_a32d8) (struct vme_dev*, ems_u32, ems_u8);

    int (*read_a24d32) (struct vme_dev*, ems_u32, ems_u32*);
    int (*write_a24d32)(struct vme_dev*, ems_u32, ems_u32);
    int (*read_a24d16) (struct vme_dev*, ems_u32, ems_u16*);
    int (*write_a24d16)(struct vme_dev*, ems_u32, ems_u16);
    int (*read_a24d8)  (struct vme_dev*, ems_u32, ems_u8*);
    int (*write_a24d8) (struct vme_dev*, ems_u32, ems_u8);

    int (*read_aXd32)  (struct vme_dev*, int am, ems_u32, ems_u32*);
    int (*write_aXd32) (struct vme_dev*, int am, ems_u32, ems_u32);
    int (*read_aXd16)  (struct vme_dev*, int am, ems_u32, ems_u16*);
    int (*write_aXd16) (struct vme_dev*, int am, ems_u32, ems_u16);
    int (*read_aXd8)   (struct vme_dev*, int am, ems_u32, ems_u8*);
    int (*write_aXd8)  (struct vme_dev*, int am, ems_u32, ems_u8);

    int (*read_pipe)   (struct vme_dev*, int am, int num, int width,
            ems_u32* addr, ems_u32* data);

    int (*register_vmeirq) (struct vme_dev*, ems_u32 mask, ems_u32 vector,
        vmeirq_callback callback, void *data);
    int (*unregister_vmeirq) (struct vme_dev*, ems_u32 mask, ems_u32 vector);
    int (*acknowledge_vmeirq) (struct vme_dev*, ems_u32 mask, ems_u32 vector);

    int (*read_controller) (struct vme_dev*, int domain, ems_u32, ems_u32*);
    int (*write_controller) (struct vme_dev*, int domain, ems_u32, ems_u32);
    int (*irq_ack) (struct vme_dev*, ems_u32 level, ems_u32* vector, ems_u32* error);
    int (*mindmalen) (struct vme_dev*, int *len_read, int *len_write);
    int (*minpipelen) (struct vme_dev*, int *len_read, int *len_write);
    int (*front_io) (struct vme_dev*, int domain, ems_u32, ems_u32*);
    int (*front_setup) (struct vme_dev*, int domain, ems_u32*);

/*
 * DUMMY must be the last 'function'; it is used to check whether all other
 * function pointers are initialised.
 */
    int (*DUMMY)(void);

    enum vmetypes vmetype;
    char* pathname;
    void* info;
};

#endif
