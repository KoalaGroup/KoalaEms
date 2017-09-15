/*
 * lowlevel/lvd/sis1100/lvd_read_async.c
 * created 12.Mar.2005 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis1100_lvd_read_async.c,v 1.31 2012/09/12 15:00:13 wuestner Exp $";

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
#include <math.h>
#ifndef HAVE_CGETCAP
#include <getcap.h>
#endif
#include <emsctypes.h>
#include <rcs_ids.h>
#include "../../../trigger/trigger.h"
#include "../../../commu/commu.h"
#include "../../../objects/is/is.h"
#include "../../../objects/pi/readout.h"
#include "dev/pci/sis1100_var.h"
#include "sis1100_lvd.h"
#include "sis1100_lvd_read.h"
#include "../lvd_map.h"
#include "../lvd_access.h"
#include "../lvd_sync_statist.h"

#ifdef __linux__
#define LINUX_LARGEFILE O_LARGEFILE
#else
#define LINUX_LARGEFILE 0
#endif

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#if 0
/* already in /usr/include/sys/param.h */
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

extern struct IS *current_IS;

RCS_REGISTER(cvsid, "lowlevel/lvd/sis1100")

/*****************************************************************************/
static long int
get_pagesize(void)
{
    long res;
    res=sysconf(_SC_PAGESIZE);
    if (res<0) {
        printf("lowlevel/lvd/sis1100/lvd_read_async.c: sysconf(PAGESIZE): %s\n",
            strerror(errno));
    }
    return res;
}
/*****************************************************************************/
static void __attribute__((unused))
dump_block(ems_u32 *block, int num)
{
    int i, j;
    for (j=0; j<num; j+=8) {
        printf("%3d", j);
        for (i=j; i<j+8&&i<num; i++)
            printf(" %08x", block[i]);
        printf("\n");
    }
}
/*****************************************************************************/
static void
sis1100_lvd_block_statist(struct lvd_dev* dev)
{
    struct lvd_sis1100_info* info=(struct lvd_sis1100_info*)dev->info;
    struct ra* ra=&info->ra;

    ra->statist.blocks++;

    if (dev->statist_interval>0) {
        struct timeval tv1;
        gettimeofday(&tv1, 0);
        if ((tv1.tv_sec-ra->tv0.tv_sec)>=dev->statist_interval) {
            sis1100_lvd_print_statist(dev, tv1);
            ra->old_statist=ra->statist;
            ra->tv0=tv1;
        }
    }
}
/*****************************************************************************/
static plerrcode
sis1100_lvd_release_block(struct lvd_sis1100_info* info, int block)
{
    struct lvd_sis1100_link *link=info->A;
    int next_block_to_be_released;
    struct ra* ra=&info->ra;

    /* sanity check */
    next_block_to_be_released=NEXTBLOCK(ra->last_released_block);
    if (block!=next_block_to_be_released) {
        printf("release_block: last block=%d current block=%d\n",
                ra->last_released_block, block);
    }

    if (ioctl(link->p_ctrl, SIS1100_DEMAND_DMA_MARK, &block)<0) {
        complain("lvd_release_block: SIS1100_DEMAND_DMA_MARK(block=%d): %s",
                block, strerror(errno));
        return plErr_System;
    }
    ra->last_released_block=block;
    ra->activeblock=-1;
    return plOK;
}
/*****************************************************************************/
/*
 * event_complete is called from fragment_complete whenever an event is
 * completely received.
 * event_complete calls the filter procedures (if any) and stores the event
 * to buffer.
 */
static plerrcode
sis1100_event_complete(
    struct lvd_dev* dev,
    ems_u32 *buffer,               /* destination (event stream) */
    int *num_saved,                /* number of saved words */
    struct fragmentcollector *event
)
{
    struct lvd_sis1100_info* info=(struct lvd_sis1100_info*)dev->info;
    struct ra* ra=&info->ra;
    struct datafilter *filter;

    filter=dev->datafilter;
    while (filter) {
        plerrcode pres;
        pres=filter->filter(filter, event->data, &event->num);
        if (pres!=plOK)
            return pres;
        filter=filter->next;
    }
    /* correct header */
    event->data[0]=(event->data[0]&0xc0000000)|(event->num*4);

    /* parsing is not allowed to change num */
    if (dev->parseflags) {
        if (sis1100_lvd_parse_event(dev, event->data, event->num)) {
            printf("error from sis1100_lvd_parse_event\n");
            return plErr_Program;
        }
    }

    /* update statistics */
    trigger.eventcnt=event->data[2];
    if (event->num>=ra->statist.max_evsize)
        ra->statist.max_evsize=event->num;
    //ra->statist.sum_evsize+=event->num;
    //ra->statist.num_ev++;
    ra->statist.events++;
    ra->statist.words+=event->num;

    /* finally copy data to buffer */
    memcpy(buffer, event->data, event->num*4);
#if 0
    printf("A %p %08x %08x %08x, num=%d\n",
            buffer, buffer[0], buffer[1], buffer[2], event->num);
#endif
    (*num_saved)+=event->num;
    event->num=0;
    event->hsize=0;

    return plOK;
}
/*****************************************************************************/
static plerrcode
sis1100_add_fragment_to_event(struct lvd_dev* dev)
{
    struct lvd_sis1100_info* info=(struct lvd_sis1100_info*)dev->info;
    struct ra* ra=&info->ra;

    if (!ra->event.num) { /* just switch the pointers */
        ems_u32 *help=ra->event.data;
        ra->event.data=ra->fragment.data;
        ra->event.hsize=ra->fragment.hsize;
        ra->event.num=ra->fragment.num;
        ra->fragment.data=help;
        ra->fragment.hsize=0;
        ra->fragment.num=0;
    } else {
        int sumnum=ra->event.num+ra->fragment.num-3;
        if (sumnum>event_max) {
            complain("lvd_read_async: fragmented event too large: %d words",
                   sumnum);
            return plErr_NoMem;
        }
        memcpy(ra->event.data+ra->event.num, ra->fragment.data,
            ra->fragment.num-3);
        ra->event.data[0]=0x80000000+sumnum*4;
        if (ra->fragment.data[0]&LVD_FRAGMENTED)
            ra->event.data[0]|=LVD_FRAGMENTED;
        ra->event.num=sumnum;
        ra->event.hsize=sumnum;
        ra->fragment.num=0;
        ra->fragment.hsize=0;
    }
    return plOK;
}
/*****************************************************************************/
/*
 * fragment_complete is called from save_event whenever a fragment is
 * completely received.
 * The fragment is merged with previous fragments if necessary.
 * If this fragment is the last (or only) part of an event
 * event_complete is called.
 */
static plerrcode
sis1100_fragment_complete(
    struct lvd_dev* dev,
    ems_u32 *buffer,               /* destination (event stream) */
    int *num_saved                 /* number of saved words */
)
{
    struct lvd_sis1100_info* info=(struct lvd_sis1100_info*)dev->info;
    struct ra* ra=&info->ra;
    plerrcode pres;

    if (ra->fragment.data[0]&LVD_FRAGMENTED || ra->event.num) {
        pres=sis1100_add_fragment_to_event(dev);
        if (pres!=plOK)
            return pres;
        if (!(ra->event.data[0]&LVD_FRAGMENTED)) {
            pres=sis1100_event_complete(dev, buffer, num_saved, &ra->event);
        }
    } else {
        pres=sis1100_event_complete(dev, buffer, num_saved, &ra->fragment);
    }

    return pres;
}
/*****************************************************************************/
/*
 * save_event is called from save_block.
 * 'block' is (part of) a DMA buffer received from a lvds crate.
 * save_event extracts one event of 'block'.
 * Uncomplete events (from the end of 'block') have to be stored locally.
 */
static plerrcode
sis1100_lvd_save_event(
    struct lvd_dev* dev,
    ems_u32 *buffer,               /* destination (event stream) */
    int *num_saved,                /* number of saved words */
    ems_u32 *block,                /* source (part of DMA block) */
    int size,                      /* number of words in block */
    int *num_used                  /* number of words used */
    )
{
    struct lvd_sis1100_info* info=(struct lvd_sis1100_info*)dev->info;
    struct ra* ra=&info->ra;
    int words;

    *num_saved=0;
    *num_used=0;

    /* at the beginning ra->fragment and ra->event are empty
       new data are stored in ra->fragment until fragment is complete
       if fragment is a complete event (not fragmented) it is moved to
       buffer
       if fragment is really a fragment it is moved to ra->event, new
       fragments are added to ra->event until event is complete
       then ra->event is moved to buffer
     */

    if (ra->fragment.num<3) { /* we need a header */
        words=MIN(size, 3-ra->fragment.num);
        memcpy(ra->fragment.data+ra->fragment.num, block, words*4);
        ra->fragment.num+=words;
        block+=words;
        size-=words;
        (*num_used)+=words;
        if (ra->fragment.num>3) {
            complain("lvd_save_event: header has %d words", ra->fragment.num);
            return plErr_Program;
        }
        if (ra->fragment.num==3) {
static int valid_events=0;
            /* check consistency */
            if (ra->fragment.data[0]==0xeeeeeeee) {
                ra->statist.eeeeeeee_events++;
                ra->fragment.data[0]=ra->fragment.data[1];
                ra->fragment.data[1]=ra->fragment.data[2];
                ra->fragment.num--;
                printf("0xeeeeeeee received\n");
                return plOK;
            }
            if ((ra->fragment.data[0]&0x9ffc0000)!=0x80000000) {
                complain("lvd_readout_async: illegal header "
                        "0x%08x 0x%08x 0x%08x",
                    ra->fragment.data[0],
                    ra->fragment.data[1],
                    ra->fragment.data[2]);
                complain("after %d valid events", valid_events);
                //return plErr_HW;
            }
            /* extract length from header */
            ra->fragment.hsize=(ra->fragment.data[0]&LVD_FIFO_MASK)/4;
            if (ra->fragment.hsize>event_max) {
                complain("lvd_readout_async: fragment too large: %d words",
                    ra->fragment.hsize);
                return plErr_NoMem;
            }
            valid_events++;
        }
        if (!size) {
            //printf("save_event: size=0\n");
            return plOK;
        }
    }

    words=MIN(size, ra->fragment.hsize-ra->fragment.num);
    memcpy(ra->fragment.data+ra->fragment.num, block, words*4);
    ra->fragment.num+=words;
    (*num_used)+=words;
    /* no need to update block and size, we will not need them further */

    /* is this fragment complete? */
    if (ra->fragment.num<ra->fragment.hsize) /* No */
        return plOK;

    return sis1100_fragment_complete(dev, buffer, num_saved);
}
/*****************************************************************************/
static void
check_for_eeeeeeee(ems_u32* block, int size)
{
    int i, j;
    for (i=0; i<size; i++) {
        if (block[i]==0xeeeeeeee) {
            if (i==size-1 || (block[i+1]&0xbffc0000)!=0x80000000) {
                printf("found eeee[%d]:", i);
                for (j=i; j<i+4 && j<size; j++)
                    printf(" %08x", block[j]);
                printf("\n");
            }
        }
    }
}
/*****************************************************************************/
/*
 * Save_block is called from readout_async whenever a complete DMA block is
 * received.
 * Part of the block is presented to save_event until the block is exhausted.
 * Then release_block is called.
 * save_event is resposible to handle events crossing the event bounderies
 */
static plerrcode
sis1100_lvd_save_block(struct lvd_dev* dev,
        ems_u32* buffer, int *saved,
        ems_u32* block, int size)
{
    int num_saved, num_used=0;
    plerrcode pres;

    check_for_eeeeeeee(block, size);

    while (size>0) {
        pres=sis1100_lvd_save_event(dev, buffer, &num_saved, block, size,
            &num_used);
        if (pres!=plOK)
            return pres;

        block+=num_used;
        size-=num_used;
        buffer+=num_saved;
        *saved+=num_saved;
    }
    if (size<0) {
        complain("sis1100_lvd_save_block: fatal program error: size=%d", size);
        return plErr_Program;
    }
    sis1100_lvd_block_statist(dev);

    return plOK;
}
/*****************************************************************************/
/*
 * readout_async is called whenever a DMA block is full.
 * buffer is the destination for event data (inside a 'cluster', defined and
 * managed elsewhere).
 * #if 0
 * It is guaranteed that there is space in 'buffer' for
 * event_max words. More than event_max must not be written to 'buffer',
 * events larger than event_max are a fatal error.
 * #else
 * event_max has to be large enough for all data of this event. But we have no
 * information about other readout procedures.
 * But because the readout is asynchronous there are probably no other
 * readout procedures called.
 * #endif
 * *saved is the amount of words written to buffer
 * (event_max is declared in objects/pi/readout.h)
 * readout_async is responsible for the DMA block management only.
 */
plerrcode
sis1100_lvd_readout_async(struct lvd_dev* dev, ems_u32* buffer, int* saved)
{
    struct lvd_sis1100_info* info=(struct lvd_sis1100_info*)dev->info;
    struct ra* ra=&info->ra;
    plerrcode pres;
    int block;

    *saved=0;

    if (info->ra.dma_newblock==-1) {
        complain("sis1100_lvd_readout_async: received block -1");
        return plErr_Program;
    }

    /* sanity check */
    block=NEXTBLOCK(info->ra.dma_oldblock);
    if (info->ra.dma_newblock!=block) {
        complain("sis1100_lvd_readout_async: illegal block %d after %d",
            info->ra.dma_newblock, info->ra.dma_oldblock);
        return plErr_Program;
    }

#if 0
u_int32_t *data;
printf("sis1100_lvd_readout_async called\n");
printf("ra->dma_size=%d\n", ra->dma_size);
data=ra->dma_buf+block*ra->dma_size;
printf("data: %08x %08x %08x %08x\n", data[0], data[1], data[2], data[3]);
#endif

    ra->activeblock=block;
    pres=sis1100_lvd_save_block(dev,
            buffer, saved, ra->dma_buf+block*ra->dma_size, ra->dma_size);
    sis1100_lvd_release_block(info, block);

    if (pres!=plOK)
        return pres;
    info->ra.dma_oldblock=info->ra.dma_newblock;

    return plOK;
}
/*****************************************************************************/
static int
alloc_dmabuf(struct lvd_dev* dev, int bufnum, size_t bufsize)
{
    struct lvd_sis1100_info* info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=info->A;
    struct sis1100_ddma_map map;
    ssize_t pagesize, pagemask;
    int i;

    pagesize=get_pagesize();
    if (pagesize<0)
        return -1;
    pagemask=pagesize-1;

    /* round up to full pagesize */
    bufsize=(bufsize+pagemask)&~pagemask;
    bufsize/=4;

    if ((info->ra.dma_buf!=0) &&
            (info->ra.dma_size==bufsize) &&
            (info->ra.dma_num==bufnum)) {
        return 0;
    }

    /* free old DMA mapping */
    map.addr=0;
    map.size=0;
    map.num=0;
    if (ioctl(link->p_ctrl, SIS1100_DEMAND_DMA_MAP, &map)<0) {
        complain("sis1100_lvd_prepare_async: SIS1100_DEMAND_DMA_MAP(0): %s",
                strerror(errno));
        return -1;
    }
    /* free old buffer */
    if (info->ra._dma_buf!=0)
        free(info->ra._dma_buf);
    info->ra._dma_buf=0;
    info->ra.dma_buf=0;

    /* alloc new buffer */
    info->ra._dma_buf=calloc(bufsize*bufnum*4+pagesize, 1);
    if (info->ra._dma_buf==0) {
        complain("sis1100_lvd_prepare_async: malloc(%lld): %s",
                (long long)(bufsize*bufnum*4+pagesize), strerror(errno));
        return -1;
    }
    /* make it page aligned */
    info->ra.dma_buf=(ems_u32*)(((ssize_t)info->ra._dma_buf+pagemask)&~pagemask);
    info->ra.dma_size=bufsize;
    info->ra.dma_num=bufnum;

    printf("sis1100_lvd_prepare_async: bufsize=%lld words, buffer=%p, num=%d\n",
            (unsigned long long)info->ra.dma_size, info->ra.dma_buf,
            info->ra.dma_num);

    /* map buffer for DMA */
    map.addr=(char*)info->ra.dma_buf;
    map.size=info->ra.dma_size*4;
    map.num=info->ra.dma_num;

    printf("alloc_dmabuf: %d buffers of size %lld at %p\n",
            map.num, (unsigned long long)map.size, map.addr);
    for (i=0; i<map.num; i++)
        printf("[%d]: %p .. %p\n", i, map.addr+map.size*i, map.addr+map.size*(i+1)-4);
    if (ioctl(link->p_ctrl, SIS1100_DEMAND_DMA_MAP, &map)<0) {
        complain("sis1100_lvd_prepare_async: SIS1100_DEMAND_DMA_MAP: %s",
                strerror(errno));
        free(info->ra._dma_buf);
        info->ra._dma_buf=0;
        info->ra.dma_buf=0;
        return -1;
    }

    return 0;
}
/*****************************************************************************/
plerrcode
sis1100_lvd_cleanup_async(struct lvd_dev* dev)
{
    struct lvd_sis1100_info* info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=info->A;
    struct sis1100_ddma_map map;
    plerrcode pres=plOK;

    printf("sis1100_lvd_cleanup_async\n");

    if (current_IS)
        current_IS->decoding_hints&=~decohint_lvd_async;

    if (info->ra.dma_buf==0)
        return plOK;

    /* free DMA mapping */
    map.addr=0;
    map.size=0;
    map.num=0;
    if (ioctl(link->p_ctrl, SIS1100_DEMAND_DMA_MAP, &map)<0) {
        complain("sis1100_lvd_cleanup_async: SIS1100_DEMAND_DMA_MAP(0): %s",
                strerror(errno));
        pres=plErr_System;
    }
    free(info->ra._dma_buf);
    info->ra._dma_buf=0;
    info->ra.dma_buf=0;

    free(info->ra.event.data);
    info->ra.event.data=0;
    free(info->ra.fragment.data);
    info->ra.fragment.data=0;

    return pres;
}
/*****************************************************************************/
/*
 * space considerations:
 * In the worst case an partial event is already stored and only the last word
 * is missing. The actual DMA buffer contains this last word and the last event
 * in this buffer ends exactly at the end of the buffer.
 * Thus we need space for 2*event_max.
 * Consequence:
 *     event_max+bufsize must not be larger than cluster_max.
 *     it is only guaranteed that event_max can be stored in the cluster
 * (event_max and cluster_max are measured in words, bufsize is measured in
 *     bytes)
 * BUT!: event_max is the maximum size for the complete lvds superevent, not
 *     the 'normal' events which constitutes this superevent
 */
plerrcode
sis1100_lvd_prepare_async(struct lvd_dev* dev, int bufnum, size_t bufsize)
{
    struct lvd_sis1100_info* info=(struct lvd_sis1100_info*)dev->info;

    printf("sis1100_lvd_prepare_async: bufnum%d, bufsize=%llu\n",
            bufnum, (unsigned long long)bufsize);

    /* only 30 bits of first header word can be used for size */
    if (event_max*4>0x3fffffff) {
        complain("lvd_prepare_async: event_max is too large");
        return plErr_NoMem;
    }

    if (bufsize>event_max*4) {
        printf("sis1100_lvd_prepare_async: bufsize (%llu) must not be "
            "larger than event_max*4 (%llu)\n",
            (unsigned long long)bufsize,
            (unsigned long long)(event_max*4));
        return plErr_ArgRange;
    }

    if (current_IS)
        current_IS->decoding_hints|=decohint_lvd_async;
    if (alloc_dmabuf(dev, bufnum, bufsize)<0)
        return plErr_System;
    info->ra.fragment.data=0;
    info->ra.event.data=0;
    if (!(info->ra.fragment.data=malloc(event_max*4))) {
        sis1100_lvd_cleanup_async(dev);
        return plErr_NoMem;
    }
    if (!(info->ra.event.data=malloc(event_max*4))) {
        sis1100_lvd_cleanup_async(dev);
        return plErr_NoMem;
    }

    return plOK;
}
/*****************************************************************************/
plerrcode
sis1100_lvd_start_async(struct lvd_dev* dev, int selftrigger)
{
    struct lvd_sis1100_info* info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=info->A;
    plerrcode pres;

    printf("sis1100_lvd_start_async\n");

    /* clear statistics */
    init_ra_statist(&info->ra.statist);
    init_ra_statist(&info->ra.old_statist);

    info->ra.dma_oldblock=-1;
    info->ra.dma_newblock=-1;
    info->ra.last_released_block=-1;
    info->ra.activeblock=-1;
    info->ra.fragment.hsize=0;
    info->ra.fragment.num=0;
    info->ra.event.hsize=0;
    info->ra.event.num=0;

    if (ioctl(link->p_ctrl, SIS1100_DEMAND_DMA_START, 0)<0) {
        complain("sis1100_lvd_start_async: SIS1100_DEMAND_DMA_START: %s",
                strerror(errno));
        return plErr_System;
    }
    info->ddma_active=1;

    info->mcwrite(dev, 0, ofs(struct lvd_reg, cr), LVD_CR_DDTX/*|LVD_CR_SCAN*/);
    lvd_cc_w(dev, sr, LVD_MSR_READY); /* clear EV_READY bit */

    if ((pres=lvd_start(dev, selftrigger))!=plOK) {
        struct sis1100_ddma_stop stop;
        printf("sis1100_lvd_start_async: lvd_start failed\n");
        if (ioctl(link->p_ctrl, SIS1100_DEMAND_DMA_STOP, &stop)<0) {
            printf("sis1100_lvd_start_async: SIS1100_DEMAND_DMA_STOP: %s\n",
                    strerror(errno));
        }
        return pres;
    }
    return plOK;
}
/*****************************************************************************/
#if 0
plerrcode
sis1100_lvd_stop_async(struct lvd_dev* dev, int selftrigger)
{
    struct lvd_sis1100_info* info=(struct lvd_sis1100_info*)dev->info;
    struct sis1100_ddma_stop stop;
    plerrcode pres=plOK, presx;

    printf("sis1100_lvd_stop_async\n");

    if (ioctl(link->p_ctrl, SIS1100_RESET, 0)<0) {
        printf("sis1100_lvd_stop_async: SIS1100_RESET: %s\n",
                strerror(errno));
        pres=plErr_System;
    }
    sleep(1); /* reset takes some time */

    pres=info->mcwrite(dev, ofs(struct lvd_reg, cr), 0);
    if ((presx=lvd_stop(dev))!=plOK) {
        if (pres==plOK) pres=presx;
    }
    if (ioctl(link->p_ctrl, SIS1100_DEMAND_DMA_STOP, &stop)<0) {
        printf("sis1100_lvd_stop_async: SIS1100_DEMAND_DMA_STOP: %s\n",
                strerror(errno));
        if (pres==plOK) pres=plErr_System;
    } else {
        /*
            XXX wenn kein Trigger gekommen war sind num und idx
            nicht initialisiert und voellig unsinnig
        */
        printf("sis1100_lvd_stop_async: num=size=%llu idx=%d\n",
            (unsigned long long)stop.num, stop.idx);
    }
    info->ddma_active=0;
    return pres;
}
#else
plerrcode
sis1100_lvd_stop_async(struct lvd_dev* dev, int selftrigger)
{
    struct lvd_sis1100_info* info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=info->A;
    struct sis1100_ddma_stop stop;
    plerrcode pres=plOK, presx;

    printf("sis1100_lvd_stop_async (new version)\n");

    if (ioctl(link->p_ctrl, SIS1100_DEMAND_DMA_STOP, &stop)<0) {
        complain("sis1100_lvd_stop_async: SIS1100_DEMAND_DMA_STOP: %s",
                strerror(errno));
        pres=plErr_System;
    }

#if 0
    if (lvd_cc_w(dev, cr, 0)<0)
        printf("lvd_cc_w cr <-- 0 failed\n");
#else
    if ((presx=lvd_stop(dev, selftrigger))!=plOK) {
        if (pres==plOK)
            pres=presx;
    }
#endif

    info->ddma_active=0;
    return pres;
}
#endif
/*****************************************************************************/
/*****************************************************************************/
