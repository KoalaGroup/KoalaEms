/*
 * lowlevel/lvd/sis1100/sis1100_lvd_read_single.c
 * created 20.Jan.2004 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis1100_lvd_read_single.c,v 1.41 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifndef HAVE_CGETCAP
#include <getcap.h>
#endif
#include <rcs_ids.h>
#include <emsctypes.h>
#include <modultypes.h>
#include "conststrings.h"
#include "../../../objects/pi/readout.h"
#include "../../../commu/commu.h"
#include "dev/pci/sis1100_var.h"
#include "sis1100_lvd.h"
#include "sis1100_lvd_read.h"
#include "../lvd_map.h"
#include "../lvd_access.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

RCS_REGISTER(cvsid, "lowlevel/lvd/sis1100")

#define ofs(what, elem) ((size_t)&(((what*)0)->elem))

/*****************************************************************************/
plerrcode
sis1100_lvd_prepare_single(__attribute__((unused)) struct lvd_dev* dev)
{
    return plOK;
}
/*****************************************************************************/
plerrcode
sis1100_lvd_cleanup_single(__attribute__((unused)) struct lvd_dev* dev)
{
    return plOK;
}
/*****************************************************************************/
/*
 * read_one_fragment reads one fragment of an LVDS event.
 * maxnum is the size of the buffer, num returns the number of words read.
 * frag indicates the first fragment (frag==0) or following fragments.
 * It is possible that we have to wait a little bit for following fragments.
 */
static plerrcode
read_one_fragment(int p, ems_u32 *buffer, unsigned int maxnum, int *num, int frag)
{
    struct sis1100_vme_block_req breq;
    struct timeval start={0, 0}, now;
    unsigned int len, again;

    if (maxnum<3) {
        complain("lvd_readout_single: no space for header");
        return plErr_Overflow;
    }

// printf("read_one_fragment: buffer=%p maxnum=%x\n", buffer, maxnum);
    do {
        again=0;
        breq.size=4;
        breq.fifo=1;
        breq.num=3; /* size of header */
        breq.am=-1;
        breq.addr=ofs(struct lvd_bus1100, prim_controller.cc.ro_data);
        breq.data=(u_int8_t*)buffer;
        breq.error=0;
// printf("read header:\n");
// printf("breq.num =        %08lx\n", breq.num);
// printf("breq.addr=        %08x\n", breq.addr);
// printf("breq.data=%p\n", breq.data);
        if (ioctl(p, SIS3100_VME_BLOCK_READ, &breq)) {
            complain("lvd_readout_single: read header: %s", strerror(errno));
            return plErr_System;
        }
        if (breq.error==0x209) { /* fifo empty */
            if (frag==0) { /* first fragment */
                complain("lvd_readout_single: first fragment but no data");
                return plErr_HW;
            } else {
                if (start.tv_sec) {
                    gettimeofday(&now, 0);
                    if (now.tv_sec-start.tv_sec>1) { /* one or two seconds */
                        complain("lvd_readout_single: timeout");
                        return plErr_HW;
                    }
                } else {
                    gettimeofday(&start, 0);
                }
                again=1;
           }
        }
    } while (again);

    /* check header consistency */
    if ((buffer[0]&LVD_HEADBIT)!=LVD_HEADBIT) {
        complain("lvd_readout_single: illegal header 0x%08x", buffer[0]);
        return plErr_HW;
    }

    /*  OK, here we have the header, now we can read the data */
    len=(buffer[0]&LVD_FIFO_MASK)/4-3; /* subtract size of header */
    if (!len) { /* nothing to read */
        return plOK;
    }
    if (len>maxnum) {
        complain("lvd_readout_single: no space for %d additional words; "
                " remaining space: %d words",
                len, maxnum);
        return plErr_Overflow;
    }
    maxnum-=3;
    *num=3;
    buffer+=3;

    breq.size=4;
    breq.fifo=1;
    breq.num=len;
    breq.am=-1;
    breq.addr=ofs(struct lvd_bus1100, prim_controller.cc.ro_data);
    breq.data=(u_int8_t*)buffer;
    breq.error=0;

// printf("read data:\n");
// printf("breq.num =        %08lx\n", breq.num);
// printf("breq.addr=        %08x\n", breq.addr);
// printf("breq.data=%p\n", breq.data);
    if (ioctl(p, SIS3100_VME_BLOCK_READ, &breq)) {
        complain("lvd_readout_single: read data: %s", strerror(errno));
        return plErr_System;
    }
    if (breq.error) {
        complain("lvd_readout_single: read data: error=0x%x num=%llu",
                breq.error, (unsigned long long)breq.num);
        return plErr_HW;
    }
    if (breq.num!=len) {
        complain("lvd_readout_single: %lld words read, but %d requested",
            (long long)breq.num, len);
        return plErr_HW;
    }

    return plOK;
}
/*****************************************************************************/
/*
 * buffer   : destination for the data
 * len (in) : max. number of words to read
 * len (out): actual number of words read
 *
 * flags:
 *     0x80000000: make handshake with controller
 *     0x40000000: readout error is ignored           (currently ignored)
 *     0x20000000: lost triggers are fatal            (currently ignored)
 *     0x10000000: event number not received is fatal (currently ignored)
 */
/*
 * normal header word 0:
 *     0x80000000: normal event or last fragment of fragmented event
 *     0xc0000000: fragmented event, more fragments follow
 * #if 0
 * modified header words:
 *     if event_size!=size_read
 *         synthetic header: head={0xa000000c, trigger_time, event_nr}
 *     if flags&0x40000000
 *         head[1]=trigger_time
 *     bits 2, 4, 6, 7, 14 of sr copied to bits 24, 25, 26, 27, 28 of head[0]
 * #endif
 *
 * len must not exceed 2^28 words
 */
plerrcode
sis1100_lvd_readout_single(struct lvd_dev* dev, ems_u32* buffer, int* len,
    int flags)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=info->A;
    plerrcode pres=plOK;
    int incomplete;
    int num=0, maxnum=*len;
    int frag=0;
    DEFINE_LVDTRACE(trace);

// printf("sis1100_lvd_readout_single: buffer=%p len=%x\n", buffer, *len);
    lvd_settrace(dev, &trace, 0);

    do {
        int xnum;

        /* read one fragment */
        pres=read_one_fragment(link->p_remote, buffer, maxnum, &xnum, frag);
        if (pres!=plOK) {
            if (pres==plErr_Overflow) {
                complain("lvd_readout_single: %d available words "
                        "and %d words read so far in %d fragment%s",
                        maxnum, num, frag, frag==1?"":"s");
            }
            goto error;
        }
        incomplete=buffer[0]&LVD_FRAGMENTED;

        if (dev->datafilter) {
            /*
             * Filter can modify data and data size.
             * After adding data *num has to be increased and *maxnum decreased.
             * After removing data *num has to be decreased and *maxnum
             * increased.
             * The event header has to be corrected too.
             * It is not allowed to remove the event header.
             * Of course *num must not go below 3 (event header) and *maxnum
             * must not become negative.
             */
            struct datafilter *filter=dev->datafilter;
            while (filter) {
                pres=filter->filter(filter, buffer, &xnum, &maxnum);
                if (pres!=plOK)
                    goto error;
                filter=filter->next;
            }
        }

        if (dev->parseflags) {
            /* Parser is not allowed to modify data. */
            if (sis1100_lvd_parse_event(dev, buffer, xnum)) {
                printf("error from sis1100_lvd_parse_event\n");
                pres=plErr_Program;
                goto error;
            }
        }

        num+=xnum;
        maxnum-=xnum;
        buffer+=xnum;
    } while (incomplete);

    ///* clear error flags (but not handshake flag) */
    //lvd_cc_w(dev, sr, sr_clear);

    /* handshake */
    if (flags&0x80000000) {
        lvd_cc_w(dev, sr, LVD_MSR_READY);
    }

    /* toggle the LEDs every 1024 events */
    if ((global_evc.ev_count&0x3ff)==0) {
        ems_u32 cr;
        info->mcread(dev, 0, ofs(struct lvd_reg, cr), &cr);
        cr=(cr&~0x18)|(((global_evc.ev_count>>10)&3)<<3);
        info->mcwrite(dev, 0, ofs(struct lvd_reg, cr), cr);
    }

#if 0
//temporarily disabled because of unclear logic
//see filter

    if (dev->parseflags) {
        if (sis1100_lvd_parse_event(dev, buffer, *len)) {
            printf("error from sis1100_lvd_parse_event\n");
            pres=plErr_Program;
            goto error;
        }
    }
#endif

error:
    lvd_settrace(dev, 0, trace);
//printf("[ev=%d] fragment=%d event complete\n", trigger.eventcnt, frag);
    return pres;
}
/*****************************************************************************/
plerrcode
sis1100_lvd_start_single(struct lvd_dev* dev, int selftrigger)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    ems_u32 cr;
    plerrcode pres;

#if 1
    printf("sis1100_lvd_start_single\n");
#endif

    if ((pres=lvd_start(dev, selftrigger))!=plOK) {
        printf("sis1100_lvd_start_single: lvd_start failed, pres=%s\n",
                PL_errstr(pres));
        return pres;
    }

    if ((pres=info->mcread(dev, 1, ofs(struct lvd_reg, cr), &cr))!=plOK) {
        printf("sis1100_lvd_start_single: mcread failed, pres=%s\n",
                PL_errstr(pres));
        return pres;
    }
    cr&=LVD_CR_LED0|LVD_CR_LED1;
    cr|=LVD_CR_SINGLE;
    if ((pres=info->mcwrite(dev, 1, ofs(struct lvd_reg, cr), cr))!=plOK) {
        printf("sis1100_lvd_start_single: mcwrite failed, pres=%s\n",
                PL_errstr(pres));
        return pres;
    }

    return plOK;
}
/*****************************************************************************/
plerrcode
sis1100_lvd_stop_single(struct lvd_dev* dev, int selftrigger)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    plerrcode pres;
    int res=0;

#if 1
    printf("sis1100_lvd_stop_single\n");
#endif

    pres=info->mcwrite(dev, 1, ofs(struct lvd_reg, cr),
        LVD_CR_LED0|LVD_CR_LED1);
    res|=lvd_stop(dev, selftrigger);
    
    return res?plErr_System:pres;
}
/*****************************************************************************/
/*****************************************************************************/
