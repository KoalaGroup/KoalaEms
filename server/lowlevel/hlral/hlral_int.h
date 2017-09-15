/*
 * lowlevel/hlral/hlral_int.h
 * $ZEL: hlral_int.h,v 1.2 2008/07/11 13:15:39 wuestner Exp $
 */

#ifndef _hlral_int_h_
#define _hlral_int_h_

#include <emsctypes.h>
#include "../devices.h"
#include "../rawmem/rawmem.h"

struct hlral {
/* this first element matches struct generic_dev */
    struct dev_generic generic;
    int (*DUMMY)(void);

    int p;                        /* path to hotlink device */
    char* pathname;
    int chip_type;
    int dma_mask;                 /* bitmap of available DMA types */
    int chip_version;
    int driverversion;
    int boardversion;
    struct rawmem rawmem;
    int dma_type;                 /* none/rawmem/scatter_gather/kernel */
    void(*dma_unmap)(struct hlral*);
    unsigned char*      dma_buf;
    size_t              dma_len;
    int buffermode;
    int signal;
    ems_u8 wbuf[8192];
    int wbufidx;
};

int hlral_reset(struct hlral*);
int hlral_real_reset(struct hlral*);
int hlral_countchips(struct hlral*, int, int);
int hlral_datapathtest(struct hlral*, int, int);
int hlral_loadregs(struct hlral*, int col, int which, ems_u8* data, int len);
int hlral_loadregs_XXX(struct hlral*, int col, int which, ems_u8* data,
        int len, ems_u8* dest);
int hlral_read_old_data(struct hlral*, int col, int which, ems_u8* dest);
plerrcode hlral_buffermode(struct hlral*, int);
plerrcode hlral_loaddac_2(struct hlral*, int, int, int, int*, int);
ems_u32 *hlral_testreadout(struct hlral*, ems_u32 *);
ems_u32 *hlral_readout(struct hlral*, ems_u32 *);
void hlral_startreadout(struct hlral*, int);
int number_of_ralhotlinks(void);

int pcihotlink_eoddelay(struct hlral*, ems_u32* eoddelay);
int pcihotlink_write(struct hlral*, ems_u8* buffer, int size);
int pcihotlink_read(struct hlral*, ems_u8* buffer, int size);
int pcihotlink_start(struct hlral*);

#define HLRAL_ANKE 1111
#define HLRAL_ATRAP 2222

#endif
