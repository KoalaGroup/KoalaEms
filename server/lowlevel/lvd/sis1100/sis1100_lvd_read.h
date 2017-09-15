/*
 * lowlevel/lvd/sis1100/sis1100_lvd_read.h
 * created 14.Dec.2003 PW
 * $ZEL: sis1100_lvd_read.h,v 1.12 2009/12/08 15:05:02 wuestner Exp $
 */

#ifndef _sis1100_lvd_read_h_
#define _sis1100_lvd_read_h_

#include "../lvdbus.h"

plerrcode sis1100_lvd_read_block(struct lvd_dev*, int ci, int addr, int reg,
    int size, ems_u32* buffer, size_t* num);
plerrcode sis1100_lvd_write_block(struct lvd_dev*, int ci, int addr, int reg,
    int size, ems_u32* buffer, size_t* num);

int sis1100_lvd_flush(struct lvd_dev*);
/*
int sis1100_lvd_readout16(struct lvd_dev*, ems_u32* buffer, int* num, int* ev);
*/

plerrcode sis1100_lvd_prepare_single(struct lvd_dev*);
plerrcode sis1100_lvd_start_single(struct lvd_dev*, int selftrigger);
plerrcode sis1100_lvd_readout_single(struct lvd_dev*, ems_u32* buffer, int* num,
        int);
plerrcode sis1100_lvd_stop_single(struct lvd_dev*, int selftrigger);
plerrcode sis1100_lvd_cleanup_single(struct lvd_dev*);
plerrcode sis1100_lvd_stat_single(struct lvd_dev*);

plerrcode sis1100_lvd_prepare_async(struct lvd_dev*, int bufnum, size_t bufsize);
plerrcode sis1100_lvd_start_async(struct lvd_dev*, int selftrigger);
plerrcode sis1100_lvd_readout_async(struct lvd_dev*, ems_u32* buffer, int* num);
plerrcode sis1100_lvd_stop_async(struct lvd_dev*, int selftrigger);
plerrcode sis1100_lvd_cleanup_async(struct lvd_dev*);
plerrcode sis1100_lvd_stat_async(struct lvd_dev*);

int sis1100_lvd_ddma_stat(struct lvd_dev* dev, int level);
int sis1100_lvd_ddma_flush(struct lvd_dev* dev);

int sis1100_lvd_parse_event(struct lvd_dev* dev, ems_u32* event, int size);

#endif
