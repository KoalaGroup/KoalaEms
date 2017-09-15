/*
 * lowlevel/cosybeam/cosybeam.h
 * 
 * created: 06.Aug.2001 PW
 */
/*
 * $ZEL: cosybeam.h,v 1.5 2007/09/16 23:43:49 wuestner Exp $
 */
#ifndef _cosybeam_h_
#define _cosybeam_h_

#include <stdio.h>
#include <sys/time.h>
#include <errorcodes.h>
#include <emsctypes.h>

struct dataformat {
    int size;
    int version;
    struct timeval tv;
    int num;
    int vals[16]; /* XXX arbitrary size */
};

struct shmformat {
    int sequence;
    int valid;
    struct dataformat data;
};

extern volatile struct shmformat* shm;

errcode cosybeam_low_init(char*);
errcode cosybeam_low_done(void);
int cosybeam_low_printuse(FILE*);

plerrcode cosybeam_get_data(ems_u32** ptr, int, int, int);

#endif
