/*
 * $ZEL: caennet.h,v 1.4 2007/08/09 20:29:18 wuestner Exp $
 * lowlevel/caenet/caenet.h
 * 
 * created: 2007-Mar-20 PW
 */

#ifndef _caennet_h_
#define _caennet_h_

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <emsctypes.h>
#include "../devices.h"

enum caentypes {caen_none, caen_a1303};

#define CAENET_BLOCKSIZE 4096

struct caenet_dev {
/* this first element matches struct generic_dev */
    struct dev_generic generic;

    plerrcode (*read)(struct caenet_dev*, ems_u8 *buf, int *size);
    plerrcode (*write)(struct caenet_dev*, ems_u8 *buf, int size);
    plerrcode (*write_read)(struct caenet_dev*, void *wbuf, int wsize,
            void *rbuf, int *rsize);
    plerrcode (*reset)(struct caenet_dev*);
    plerrcode (*echo)(struct caenet_dev*, int* num);
    plerrcode (*timeout)(struct caenet_dev*, int);
    plerrcode (*led)(struct caenet_dev*);
    plerrcode (*buffer)(struct caenet_dev*, ems_u8** buf);

/*
 * DUMMY must be the last 'function'; it is used to check whether all other
 * function pointers are initialised.
 */
    int (*DUMMY)(void);

    enum caentypes caentype;
    char* pathname;
    void* info;
};

#endif
