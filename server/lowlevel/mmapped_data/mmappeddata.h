/*
 * lowlevel/mmaped_data/mmappeddata.h
 * 
 * created: 2011-08-04 PW
 * 
 * $ZEL: mmappeddata.h,v 1.1 2011/08/13 19:27:47 wuestner Exp $
 */
#ifndef _mmappeddata_h_
#define _mmappeddata_h_

#include <stdio.h>
#include <sys/time.h>
#include <errorcodes.h>
#include <emsctypes.h>

plerrcode read_mmapped_data(ems_u32** ptr, ems_u32, int, int);
plerrcode mmap_data(char*, ems_u32*);
plerrcode munmap_data(ems_u32);

#endif
