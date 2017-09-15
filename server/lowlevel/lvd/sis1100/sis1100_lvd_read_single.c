/*
 * lowlevel/lvd/sis1100/sis1100_lvd_read_single.c
 * created 20.Jan.2004 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis1100_lvd_read_single.c,v 1.38 2012/09/12 14:56:03 wuestner Exp $";

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
#include "../../../trigger/trigger.h"
#include "../../../commu/commu.h"
#include "dev/pci/sis1100_var.h"
#include "sis1100_lvd.h"
#include "sis1100_lvd_read.h"
#include "../lvd_map.h"
#include "../lvd_access.h"
//#include "../tdc/f1.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

RCS_REGISTER(cvsid, "lowlevel/lvd/sis1100")

#define ofs(what, elem) ((size_t)&(((what*)0)->elem))

static int errorcount;
//static ems_u32 word_store[16];
static int stored_words;

/*****************************************************************************/
plerrcode
sis1100_lvd_prepare_single(struct lvd_dev* dev)
{
    return plOK;
}
/*****************************************************************************/
plerrcode
sis1100_lvd_cleanup_single(struct lvd_dev* dev)
{
    return plOK;
}
/*****************************************************************************/
#if 0
static int
sis1100_lvd_readout_transient_errors(struct lvd_dev* dev, ems_u32 status,
        int verbose)
{
    ems_u32 sr, pat;

    if (verbose) {
        printf("printing crate status because of readout errors\n");
        lvd_cratestat(dev, 2, -1);
    }
    if (status&0x40) {
        int addr;
        /* read error pattern */
        lvd_ib_r(dev, error, &pat);
        if (verbose)
            printf("error pattern: 0x%x\n", pat);
        for (addr=0; addr<16; addr++) {
            if ((1<<addr)&pat) {
                int idx=lvd_find_acard(dev, addr);
                struct lvd_acard *acard=dev->acards+idx;

                if (idx<0) {
                    printf("program error in sis1100_lvd_readout_single: "
                        "nonexistent card at address 0x%x\n", addr);
                    goto error;
                }
                if (acard->clear_card) {
                    if (acard->clear_card(dev, acard)<0)
                        goto error;
                }
            }
        }
    }
    lvd_cc_w(dev, ctrl, LVD_SYNC_RES);
    printf("sis1100_lvd_readout_single: LVD_SYNC_RES for crate %s at event %d "
            "executed\n",
            dev->pathname, trigger.eventcnt);

    if (lvd_cc_w(dev, sr, status)<0) {
        printf("sis1100_lvd_readout_single: error clearing status bits\n");
        goto error;
    }
    if (lvd_cc_r(dev, sr, &sr)<0) {
        printf("sis1100_lvd_readout_single: error reading status\n");
        goto error;
    }
    if (sr&0x40c0) {
        printf("sis1100_lvd_readout_single: status after clear: 0x%04x\n", sr);
        goto error;
    }

    return 0;

error:
    if (!verbose) {
        printf("printing crate status because of readout errors after "
                "recovery\n");
        lvd_cratestat(dev, 2, -1);
    }
    return 1;
}
#endif
/*****************************************************************************/
static plerrcode
read_header(int p, ems_u32 *headbuf, int frag)
{
    struct sis1100_vme_block_req breq;

    breq.size=4;
    breq.fifo=1;
    breq.num=3; /* size of header */
    breq.am=-1;
    breq.addr=ofs(struct lvd_bus1100, prim_controller.cc.ro_data);
    breq.data=(u_int8_t*)headbuf;
    breq.error=0;
    if (ioctl(p, SIS3100_VME_BLOCK_READ, &breq)) {
        complain("lvd_readout_single: read header: %s", strerror(errno));
        return plErr_System;
    }

    if (breq.error==0x209) { /* fifo empty */
        struct timeval start, now;
        //printf("read_header: error==0x209 frag=%d\n", frag);
        if (frag==0) { /* should never happen */
            complain("lvd_readout_single: first fragment but no data");
                return plErr_HW;
        }
        gettimeofday(&start, 0);
        while (breq.error==0x209) {
            gettimeofday(&now, 0);
            if (now.tv_sec-start.tv_sec>1) { /* one or two seconds */
                complain("lvd_readout_single: timeout");
                return plErr_HW;
            }
            breq.size=4;
            breq.fifo=1;
            breq.num=3;
            breq.am=-1;
            breq.addr=ofs(struct lvd_bus1100, prim_controller.cc.ro_data);
            breq.data=(u_int8_t*)headbuf;
            breq.error=0;
            if (ioctl(p, SIS3100_VME_BLOCK_READ, &breq)) {
                complain("lvd_readout_single: read header: %s",
                        strerror(errno));
                return plErr_System;
            }
        }
    }

#if 0
    if (breq.error || breq.num!=3) {
        complain("lvd_readout_single: header: error=0x%x num=%llu,frag=%d ev=%d",
            breq.error, (unsigned long long)breq.num,
            frag, trigger.eventcnt);
        return plErr_HW;
    }
#else
    if (breq.error || breq.num<3 || breq.num>19) {
        complain("lvd_readout_single: header: error=0x%x num=%llu,frag=%d ev=%d",
            breq.error, (unsigned long long)breq.num,
            frag, trigger.eventcnt);
        return plErr_HW;
    } else if (breq.num>3) {
        stored_words=breq.num-3;
    } /* else: OK */

#endif

    /* check header consistency */
#if 0
    printf("[ev=%d] head: %08x %08x %08x\n", trigger.eventcnt,
            headbuf[0], headbuf[1], headbuf[2]);
#endif
    if ((headbuf[0]&LVD_HEADBIT)!=LVD_HEADBIT) {
        complain("lvd_readout_single: illegal header 0x%08x", headbuf[0]);
        return plErr_HW;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
read_data(int p, ems_u32 *databuf, int len, int frag)
{
    struct sis1100_vme_block_req breq;

    if (stored_words) {
        databuf+=stored_words;
        len-=stored_words;
    }

    breq.size=4;
    breq.fifo=1;
    breq.num=len;
    breq.am=-1;
    breq.addr=ofs(struct lvd_bus1100, prim_controller.cc.ro_data);
    breq.data=(u_int8_t*)databuf;
    breq.error=0;

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
 * headbuf   : destination for the header
 * databuf   : destination for the data
 * maxdatalen: maximim number of data words to read
 * datalen   : actual number of data words read
 */
static plerrcode
read_one_fragment(int p, ems_u32 *headbuf, ems_u32 *databuf,
    int maxdatalen, int *datalen, int frag)
{
    plerrcode pres;
    int len;

    /* sanity check only, must never be true */
    if (maxdatalen<=0) {
        complain("lvd_readout_single: strange length requested: %d",
                maxdatalen);
        return plErr_Program;
    }

    stored_words=0;
    if ((pres=read_header(p, headbuf, frag))!=plOK)
        return pres;

    len=(headbuf[0]&LVD_FIFO_MASK)/4-3; /* subtract size of header */
    if (!len) { /* nothing to read */
        *datalen=0;
        return plOK;
    }
    if (len>maxdatalen) {
        complain("lvd_readout_single: no space for %lld additional words; "
                " remaining space: %d words",
                (long long)len, maxdatalen);
        return plErr_Overflow;
    }

    if ((pres=read_data(p, databuf, len, frag))!=plOK)
        return pres;
    *datalen=len;
    
    return pres;
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
    //struct datafilter *filter;
    plerrcode pres=plOK;
    int fragments_complete=0, fragment=0;
    ems_u32 headbuffer[3]; /* for fragmented events */
    ems_u32 *headbuf=buffer, *databuf=buffer+3; /* three words for header */
    int maxlen=*len-3, num=3;                   /* three words for header */
    int xnum=0;
    DEFINE_LVDTRACE(trace);

    lvd_settrace(dev, &trace, 0);

    while (!fragments_complete) {
        /* read one fragment */
        pres=read_one_fragment(link->p_remote, headbuf, databuf, maxlen, &xnum,
                fragment);
        if (pres!=plOK) {
            if (pres==plErr_Overflow) {
                complain("lvd_readout_single: %d available words "
                        "and %d words read so far in %d fragment%s",
                        *len-3, xnum, fragment, fragment==1?"":"s");
            }
            goto error;
        }

        num+=xnum;
        maxlen-=xnum;

        if (!(headbuf[0]&LVD_FRAGMENTED)) { /* event complete */
            fragments_complete=1;
        } else {                        /* fragmented */
complain("fragmented events are currently not supported!");
pres=plErr_Overflow;
goto error;
            headbuf=headbuffer;
            databuf+=xnum;
            //printf("[%2d] fragment %d incomplete\n",
            //        trigger.eventcnt, fragment);
        }
        fragment++;
    } 

#if 0
    /* DON'T correct header! */
    buffer[0]=0x80000000|(num*4);
    But now fragmented events cannot be used.
    Fuer fragmentierte Events koennte man einen Superhaeder einfuehren, der
    28 Bit fuer die Laenge hat, aber keine Zeiterweitrung
#endif

#if 0
temporarily disabled because of unclear logic
filter should receive and return the actual length explicitely
it must not use buffer[0]&LVD_FIFO_MASK because of possible overflow

    filter=dev->datafilter;
    while (filter) {
        pres=filter->filter(filter, buffer, len);
        if (pres!=plOK)
            goto error;
        filter=filter->next;
    }
#endif

    *len=num;

    ///* clear error flags (but not handshake flag) */
    //lvd_cc_w(dev, sr, sr_clear);

    /* handshake */
    if (flags&0x80000000) {
        lvd_cc_w(dev, sr, LVD_MSR_READY);
    }

    /* toggle the LEDs every 1024 events */
    if ((trigger.eventcnt&0x3ff)==0) {
        ems_u32 cr;
        info->mcread(dev, 0, ofs(struct lvd_reg, cr), &cr);
        cr=(cr&~0x18)|(((trigger.eventcnt>>10)&3)<<3);
        info->mcwrite(dev, 0, ofs(struct lvd_reg, cr), cr);
    }

#if 0
temporarily disabled because of unclear logic
see filter

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
//printf("[ev=%d] fragment=%d event complete\n", trigger.eventcnt, fragment);
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
    errorcount=0;
    stored_words=0;

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
