/*
 * lowlevel/lvd/sis1100/lvd_read2.c
 * created 12.Mar.2005 PW
 * $ZEL: sis1100_lvd_read_async.c,v 1.2 2005/11/20 21:04:16 wuestner Exp $
 */

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/param.h>
#ifndef HAVE_CGETCAP
#include <getcap.h>
#endif
#include <emsctypes.h>
#include "../../../trigger/trigger.h"
#include "dev/pci/sis1100_var.h"
#include "../lvdbus.h"
#include "../lvd_map.h"
#include "../lvd_access.h"
#include "sis1100_lvd_read.h"
/*#include "../../sync/pci_zel/sync_pci_zel.h"*/

#ifdef __linux__
#define LINUX_LARGEFILE O_LARGEFILE
#else
#define LINUX_LARGEFILE 0
#endif

#ifdef DMALLOC
#include <dmalloc.h>
#endif

/*****************************************************************************/
static long int
get_pagesize(void)
{
    long res;
    res=sysconf(_SC_PAGESIZE);
    if (res<0) {
        printf("lowlevel/lvd/sis1100/lvd_read2.c: sysconf(PAGESIZE): %s\n",
            strerror(errno));
    }
    return res;
}
/*****************************************************************************/
#if 0
static void
dump_block(struct lvd_sis1100_info* info, int block, int idx, int end)
{
    ems_u32* start;

    printf("dump of block %d\n", block);
    start=info->demand_dma_buf+block*info->demand_dma_size;
    if (idx<0)
        idx=0;
    if (end>=info->demand_dma_size)
        end=info->demand_dma_size;

    for (; idx<end; idx++) {
        printf("  %8x %08x\n", idx, start[idx]);
    }
}
#endif
/*****************************************************************************/
static int
sis1100_lvd_readblock_async(struct lvd_sis1100_info* info,
    unsigned int* buffer, int* num, int block, int seq)
{
    ems_u32* start;
    int res;
#if 0
    int ofs, expected_event, event=0;
#endif

    start=info->ra.demand_dma_buf+
        (block%info->ra.demand_dma_num)*info->ra.demand_dma_size;

#if 0
    /* copy data to dataout */
    memcpy(buffer, start, info->demand_dma_size);
    *num=info->demand_dma_size;
#endif

#if 0
    /* sanity check */
    ofs=info->demand_dma_ofs;
    expected_event=info->demand_dma_lastevent+1;
    while (ofs<info->demand_dma_size) {
        int len;
        len=(start[ofs]&0xffff)/4;
        if (ofs+2>=info->demand_dma_size) {
            /*
            printf("lvd_readout2: block=%d seq=%d eventnr %d guessed\n",
                    block, seq, expected_event);
            */
            event=expected_event;
        } else {
            event=start[ofs+2];
        }
        if (event!=expected_event) {
            printf("lvd_readout2: block=%d seq=%d event=0x%x, expected=0x%x\n",
                    block, seq, event, expected_event);
            printf("ofs=0x%x demand_dma_ofs=0x%x demand_dma_lastevent=0x%x\n",
                    ofs, info->demand_dma_ofs, info->demand_dma_lastevent);
            printf("demand_dma_size=0x%llx\n", (unsigned long long)info->demand_dma_size);
            dump_block(info, block, ofs-50, ofs+50);
            return -1;
        }
        expected_event=event+1;
        ofs+=len;
    }
    info->demand_dma_lastevent=event;
    info->demand_dma_ofs=ofs-info->demand_dma_size;
#endif

    /* write data to plain file */
    res=write(info->ra.demand_dma_path, start, info->ra.demand_dma_size*4);
    if (res!=info->ra.demand_dma_size*4) {
        printf("lvd_readblock2: res=%d; errno=%s\n", res, strerror(errno));
        return -1;
    }

    /* release buffer */
    if (ioctl(info->p_ctrl, SIS1100_DEMAND_DMA_MARK, &block)<0) {
        printf("lvd_readout2: SIS1100_DEMAND_DMA_MARK(0): %s\n",
                strerror(errno));
        return -1;
    }
    /*printf("block %d marked\n", block);*/
    return 0;
}
/*****************************************************************************/
int
sis1100_lvd_readout_async(struct lvd_dev* dev, unsigned int* buffer, int* num,
    int block)
{
    struct lvd_sis1100_info* info=&dev->info.sis1100;
    int res=0, nextblock, count=0, trace=0;
    static int seq=0;

    *num=0;
    nextblock=info->ra.demand_dma_lastblock+1;
    /*if (nextblock>=info->demand_dma_num)
        nextblock=0;*/

    if (block<nextblock) {
        printf("lvd_readout2: block=%d < expected=%d, seq=%d\n", block,
                nextblock, seq);
        return 0;
    }
    if (block!=nextblock) {
        /*trace=1;
        printf("lvd_readout2: block=%d, expected=%d, seq=%d\n", block,
                nextblock, seq);*/
        do {
            /*printf("read nextblock %d\n", nextblock);*/
            res=sis1100_lvd_readblock_async(info, buffer, &count, nextblock, seq);
            buffer+=count;
            *num+=count;
            nextblock++;
            /*if (nextblock>=info->demand_dma_num)
                nextblock=0;*/
        } while ((nextblock!=block) && !res);
    }

    if (trace)
        printf("read block %d\n", block);
    res=sis1100_lvd_readblock_async(info, buffer, &count, block, seq);
    *num+=count;

    info->ra.demand_dma_lastblock=block;
    seq++;

    return res;
}
/*****************************************************************************/
static int
alloc_dmabuf(struct lvd_dev* dev, int bufnum, size_t bufsize)
{
    struct lvd_sis1100_info* info=&dev->info.sis1100;
    struct sis1100_ddma_map map;
    ssize_t pagesize, pagemask;
    
    pagesize=get_pagesize();
    if (pagesize<0)
        return -1;
    pagemask=pagesize-1;

    /* round up to full pagesize */
    bufsize=(bufsize+pagemask)&~pagemask;
    bufsize/=4;

    if ((info->ra.demand_dma_buf!=0) &&
            (info->ra.demand_dma_size==bufsize) &&
            (info->ra.demand_dma_num==bufnum)) {
        return 0;
    }

    /* free DMA mapping */
    map.addr=0;
    map.size=0;
    map.num=0;
    if (ioctl(info->p_ctrl, SIS1100_DEMAND_DMA_MAP, &map)<0) {
        printf("sis1100_lvd_start2: SIS1100_DEMAND_DMA_MAP(0): %s\n",
                strerror(errno));
        return -1;
    }
    /* free buffer */
    if (info->ra._demand_dma_buf!=0)
        free(info->ra._demand_dma_buf);
    info->ra._demand_dma_buf=0;
    info->ra.demand_dma_buf=0;

    /* alloc new buffer */
    info->ra._demand_dma_buf=calloc(bufsize*bufnum*4+pagesize, 1);
    if (info->ra._demand_dma_buf==0) {
        printf("sis1100_lvd_start2: malloc(%lld): %s\n",
                (long long)(bufsize*bufnum*4+pagesize), strerror(errno));
        return -1;
    }
    info->ra.demand_dma_buf=(ems_u32*)(((ssize_t)info->ra._demand_dma_buf+pagemask)&~pagemask);
    info->ra.demand_dma_size=bufsize;
    info->ra.demand_dma_num=bufnum;

    printf("start2: bufsize=%lld Words, buffer=%p, num=%d\n",
            (unsigned long long)info->ra.demand_dma_size, info->ra.demand_dma_buf,
            info->ra.demand_dma_num);

    /* map buffer for DMA */
    map.addr=(char*)info->ra.demand_dma_buf;
    map.size=info->ra.demand_dma_size*4;
    map.num=info->ra.demand_dma_num;

    if (ioctl(info->p_ctrl, SIS1100_DEMAND_DMA_MAP, &map)<0) {
        printf("sis1100_lvd_start2: SIS1100_DEMAND_DMA_MAP: %s\n",
                strerror(errno));
        free(info->ra._demand_dma_buf);
        info->ra._demand_dma_buf=0;
        info->ra.demand_dma_buf=0;
        return -1;
    }

    return 0;
}
/*****************************************************************************/
/* XXX only for RSS test! should be removed */
static int
open_path(struct lvd_sis1100_info* info)
{
    struct timeval tv;
    struct tm* tm;
    time_t tt;
    int p;
    char help[MAXPATHLEN+1];
    char filename[]="data/%Y-%m-%d_%H:%M:%S";

    gettimeofday(&tv, 0);
    tt=tv.tv_sec;
    tm=gmtime(&tt);
    /* recommended time part of filename: %Y-%m-%d_%H:%M:%S */
    strftime(help, MAXPATHLEN, filename, tm);

    p=open(help, O_WRONLY|O_CREAT|O_EXCL|LINUX_LARGEFILE, 0644);
    if (p<0) {
        printf("sis1100_lvd_start2: can't open file \"%s\": %s\n",
                filename, strerror(errno));
        return -1;
    }
    info->ra.demand_dma_path=p;
    return 0;
}
/*****************************************************************************/
int
sis1100_lvd_prepare_async(struct lvd_dev* dev, int bufnum, size_t bufsize)
{
    struct lvd_sis1100_info* info=&dev->info.sis1100;

    printf("sis1100_lvd_prepare2\n");

    if (open_path(info)<0) /* XXX only for RSS test! should be removed */
        return -1;

    if (alloc_dmabuf(dev, bufnum, bufsize)<0)
        return -1;

    return 0;
}
/*****************************************************************************/
int
sis1100_lvd_cleanup_async(struct lvd_dev* dev)
{
    struct lvd_sis1100_info* info=&dev->info.sis1100;
    struct sis1100_ddma_map map;

    printf("sis1100_lvd_cleanup_async\n");

/* XXX only for RSS test! should be removed */
    if (info->ra.demand_dma_path>=0)
        close(info->ra.demand_dma_path);

    if (info->ra.demand_dma_buf==0)
        return 0;

    /* free DMA mapping */
    map.addr=0;
    map.size=0;
    map.num=0;
    if (ioctl(info->p_ctrl, SIS1100_DEMAND_DMA_MAP, &map)<0) {
        printf("sis1100_lvd_cleanup_async: SIS1100_DEMAND_DMA_MAP(0): %s\n",
                strerror(errno));
    }
    free(info->ra._demand_dma_buf);
    info->ra._demand_dma_buf=0;
    info->ra.demand_dma_buf=0;

    return 0;
}
/*****************************************************************************/
int
sis1100_lvd_start_async(struct lvd_dev* dev)
{
    struct lvd_sis1100_info* info=&dev->info.sis1100;

    printf("sis1100_lvd_start2\n");

    info->ra.demand_dma_lastblock=-1;
#if 0
    info->demand_dma_lastevent=-1;
    info->demand_dma_ofs=0;
#endif

    if (ioctl(info->p_ctrl, SIS1100_DEMAND_DMA_START)<0) {
        printf("sis1100_lvd_start2: SIS1100_DEMAND_DMA_START: %s\n",
                strerror(errno));
        return -1;
    }

    info->mcwrite(dev, ofs(struct lvd_reg, cr), LVD_CR_DDTX/*|LVD_CR_SCAN*/, 0);
    lvd_c_w(dev, 16, sr, 0x200); /* clear EV_READY bit */

    if (lvd_start(dev)<0) {
        struct sis1100_ddma_stop stop;
        printf("sis1100_lvd_start2: lvd_start failed\n");
        if (ioctl(info->p_ctrl, SIS1100_DEMAND_DMA_STOP, &stop)<0) {
            printf("sis1100_lvd_start2: SIS1100_DEMAND_DMA_STOP: %s\n",
                    strerror(errno));
        }
        return -1;
    }
    return 0;
}
/*****************************************************************************/
int
sis1100_lvd_stop_async(struct lvd_dev* dev)
{
    struct lvd_sis1100_info* info=&dev->info.sis1100;
    struct sis1100_ddma_stop stop;
    int res=0;

printf("sis1100_lvd_stop2\n");

    close(info->ra.demand_dma_path); /* XXX only for RSS test! should be removed */
    info->ra.demand_dma_path=-1;

    if (ioctl(dev->info.sis1100.p_ctrl, SIS1100_RESET)<0) {
        printf("sis1100_lvd_stop2: SIS1100_RESET: %s\n",
                strerror(errno));
    }

    res|=info->mcwrite(dev, ofs(struct lvd_reg, cr), 0, 0);
    res|=lvd_stop(dev);

    if (ioctl(dev->info.sis1100.p_ctrl, SIS1100_DEMAND_DMA_STOP, &stop)<0) {
        printf("sis1100_lvd_stop2: SIS1100_DEMAND_DMA_STOP: %s\n",
                strerror(errno));
        res=-1;
    } else {
        printf("lvd_stop2: num=size=%llu idx=%d\n",
            (unsigned long long)stop.num, stop.idx);
    }
    return res?-1:0;
}
/*****************************************************************************/
#include "../../../objects/is/is.h"
int
sis1100_lvd_stat_async(struct lvd_dev* dev)
{
#if 0
int i;
    int p=dev->info.sis1100.p_ctrl;
    struct sis1100_ctrl_reg req;
    struct sis1100_dma_stat dma;

    req.offset=0x10;
    ioctl(p, SIS1100_CTRL_READ, &req);
    printf("doorbell=0x%x\n", req.val);

    req.offset=0xb0;
    ioctl(p, SIS1100_CTRL_READ, &req);
    printf("d0_bc=%d\n", req.val);

    ioctl(p, SIS1100_DMA_STAT, &dma);
    printf("dmacsr0: 0x%x\n", dma.dmacsr0);
    printf("sr     : 0x%x\n", dma.sr);

    for (i=0; i<dev->info.sis1100.demand_dma_size/4; i++) {
        if (dev->info.sis1100.demand_dma_buf[i]!=0x5a5a5a5a)
            printf("[%3d]: 0x%08x\n", i, dev->info.sis1100.demand_dma_buf[i]);
    }
/*
    for (i=0; i<32; i++) {
        if (is_list[i]!=0)
            printf("is[%2d]=%p\n", i, is_list[i]);
    }
*/
#endif
    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
