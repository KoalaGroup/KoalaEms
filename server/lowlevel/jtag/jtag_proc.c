/*
 * lowlevel/jtag/jtag_proc.c
 * created 2006-Jul-06 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: jtag_proc.c,v 1.7 2017/10/22 22:09:36 wuestner Exp $";

/*
 * This file contains the procedures declared in jtag_proc.h.
 * These procedures are the interface to the the ems procedures in
 * server/proc/jtag.
 * This file must not contain any real jtag actions. (they are provided in
 * jtac_actions.c)
 */

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <conststrings.h>
#include <rcs_ids.h>
#include "../devices.h"

#include "jtag_proc.h"
#include "jtag_chipdata.h"

#ifdef JTAG_DEBUG
int jtag_traced=0;
#endif

RCS_REGISTER(cvsid, "lowlevel/jtag")

/****************************************************************************/
plerrcode
jtag_trace(__attribute__((unused)) struct generic_dev* dev,
        __attribute__((unused)) ems_u32 on,
        __attribute__((unused)) ems_u32* old)
{
#ifdef JTAG_DEBUG
    *old=jtag_traced;
    jtag_traced=on;
    return plOK;
#else
    return plErr_NoSuchProc;
#endif
}
/****************************************************************************/
static plerrcode
jtag_init_dev(struct generic_dev* dev, struct jtag_dev* jdev, int ci, int addr)
{
    plerrcode pres;

    jdev->dev=dev;
    jdev->ci=ci;
    jdev->addr=addr;
    jdev->opaque=0;

    if (ci!=2 && !dev->generic.online)
        return plErr_Offline;

    if (dev->generic.init_jtag_dev)
        pres=dev->generic.init_jtag_dev(dev, jdev);
    else
        pres=plErr_BadModTyp;
    if (pres!=plOK) {
        if (pres==plErr_BadModTyp)
            printf("init_jtag_dev: jtag for this device (class %d) not supported\n",
                    dev->generic.class);
        else
            printf("init_jtag_dev: dev->generic.init_jtag_dev returned %s\n",
                    PL_errstr(pres));
    }
    return pres;
}
/****************************************************************************/
void
//jtag_sleep(struct jtag_dev* jdev, int us)
jtag_sleep(struct jtag_chain* chain, int us)
{
    double time=0;
    double target=(double)us*1000.;

    do {
        //jdev->jt_sleep(&chain->jtag_dev);
        jtag_action(chain, /*tms=*/0, /*tdi=*/0, /*tdo=*/0);
        time+=chain->jtag_dev.sleeptime;
    } while (time<target);
}
/****************************************************************************/
static int
//jtag_calibrate_sleep_(struct jtag_dev* jdev, int usec)
jtag_calibrate_sleep_(struct jtag_chain* chain, int usec)
{
    struct jtag_dev* jdev=&chain->jtag_dev;
    struct timeval a, b;
    float t, help; /* time in us */
    int count=0, fail=1;

    /* we try to sleep usec us +-1% */
    do {
        gettimeofday(&a, 0);
        jtag_sleep(chain, usec);
        gettimeofday(&b, 0);
        t=(b.tv_sec-a.tv_sec)*1000000+(b.tv_usec-a.tv_usec);

        /* we wanted to sleep usec us, but we slept t us */
        /* correcting sleepmagic: */
        help=jdev->sleeptime*(t/usec);
        if (count<10) {
            printf("a=(%ld, %ld)\n", a.tv_sec, a.tv_usec);
            printf("b=(%ld, %ld)\n", b.tv_sec, b.tv_usec);
            printf("t=%f us, magic: %f --> %f\n", t, jdev->sleeptime, help);
        }
        jdev->sleeptime=help;
        fail=(t/usec<.990)||(t/usec>1.100);
    } while ((count++<20)&&fail);
    printf("jt_calibrate_sleep: magic=%f (after %d loops)\n",
            jdev->sleeptime, count);
    return fail;
}

static int
//jtag_calibrate_sleep(struct jtag_dev* jdev)
jtag_calibrate_sleep(struct jtag_chain* chain)
{
    /* sleeptime should contain the time (in ns) needed for one basic
            sleep loop */
    /* the initialisation value (1000 ns) is a very wild guess */
    chain->jtag_dev.sleeptime=1000.;
    /* initial calibration with an interval of 1 ms */
    if (jtag_calibrate_sleep_(chain, 1000))
        return -1;
    /* better calibration with an interval of 1 s */
    if (jtag_calibrate_sleep_(chain, 1000000))
        return -1;
chain->jtag_dev.sleeptime/=5;
    return 0;
}

static int
//jtag_check_sleep(struct jtag_dev* jdev)
jtag_check_sleep(struct jtag_chain* chain)
{
    struct timeval a, b;
    double t;

    printf("sleep check:\n");
    gettimeofday(&a, 0);
    jtag_sleep(chain, 1); /* sleep 1 us */
    gettimeofday(&b, 0);
    t=(double)(b.tv_sec-a.tv_sec)*1000000.+(b.tv_usec-a.tv_usec);
    printf("1 us --> %f us\n", t);

    gettimeofday(&a, 0);
    jtag_sleep(chain, 1); /* sleep 1 us */
    gettimeofday(&b, 0);
    t=(double)(b.tv_sec-a.tv_sec)*1000000.+(b.tv_usec-a.tv_usec);
    printf("1 us --> %f us\n", t);

    gettimeofday(&a, 0);
    jtag_sleep(chain, 10); /* sleep 10 us */
    gettimeofday(&b, 0);
    t=(double)(b.tv_sec-a.tv_sec)*1000000.+(b.tv_usec-a.tv_usec);
    printf("10 us --> %f us\n", t);

    gettimeofday(&a, 0);
    jtag_sleep(chain, 1000); /* sleep 1 ms */
    gettimeofday(&b, 0);
    t=(double)(b.tv_sec-a.tv_sec)*1000000.+(b.tv_usec-a.tv_usec);
    printf("1 ms --> %f ms\n", t/1000.);

    gettimeofday(&a, 0);
    jtag_sleep(chain, 1000000); /* sleep 1 s */
    gettimeofday(&b, 0);
    t=(double)(b.tv_sec-a.tv_sec)*1000000.+(b.tv_usec-a.tv_usec);
    printf("1 s --> %f s\n", t/1000000.);

    return 0;
}
/****************************************************************************/
static void
jtag_free_chain(struct jtag_chain* chain)
{
    chain->jtag_dev.jt_free(&chain->jtag_dev);
    free(chain->chips);
    chain->chips=0;
}
/****************************************************************************/
plerrcode
jtag_loopcal(struct generic_dev* dev, int ci, int addr)
{
    struct jtag_chain chain;
    plerrcode pres;

    chain.chips=0;
    pres=jtag_init_dev(dev, &chain.jtag_dev, ci, addr);
    if (pres!=plOK)
        return pres;

    if (jtag_calibrate_sleep(&chain))
        return plErr_HWTestFailed;

    if (jtag_check_sleep(&chain))
        return plErr_HWTestFailed;

    return plOK;
}
/****************************************************************************/
static plerrcode
jtag_init_chain(struct jtag_chain* chain, struct generic_dev* dev,
        int ci, int addr)
{
    int ii, bits, chain_valid;
    unsigned int i;
    ems_u32* ids;
    plerrcode pres;

    chain->chips=0;
    if ((pres=jtag_init_dev(dev, &chain->jtag_dev, ci, addr))!=plOK) {
        printf("jtag_init_chain: jtag_init_dev failed\n");
        return pres;
    }

    chain->state=0;
    chain_valid=0;
#ifdef JTAG_DEBUG
    chain->jstate_count=0;
    chain->ichain=0;
    chain->ochain=0;
#endif

    if (jtag_force_TLR(chain))
        return plErr_System;

    if (jtag_calibrate_sleep(chain))
        return plErr_HWTestFailed;

    if (jtag_read_ids(chain, &ids, &chain->num_chips))
        return plErr_System;

    chain_valid=1;
    chain->chips=malloc(sizeof(struct jtag_chip)*chain->num_chips);
    for (i=0; i<chain->num_chips; i++) {
        struct jtag_chip* chip=chain->chips+i;
        chip->chain=chain;
        chip->id=ids[i];
        chip->version=(ids[i]>>28)&0xf;
        find_jtag_chipdata(chip, 1);
        if (chip->chipdata==0)
            chain_valid=0;
    }

    if (!chain_valid) {
        printf("jtag_init_chain: chain not valid\n");
        free(ids);
        return plErr_BadModTyp;
    }

    for (i=0, bits=0; i<chain->num_chips; i++) {
        struct jtag_chip* chip=chain->chips+i;
        chip->pre_d_bits=chain->num_chips-i-1;
        chip->after_d_bits=i;
        chip->after_c_bits=bits;
        bits+=chip->chipdata->ir_len;
    }
    for (ii=chain->num_chips-1, bits=0; ii>=0; ii--) {
        struct jtag_chip* chip=chain->chips+ii;
        chip->pre_c_bits=bits;
        bits+=chip->chipdata->ir_len;
    }

    free(ids);
    return plOK;
}
/****************************************************************************/
static void
jtag_dump_chain(struct jtag_chain* chain)
{
    unsigned int i;

    for (i=0; i<chain->num_chips; i++) {
        struct jtag_chip* chip=chain->chips+i;
        printf("jtag ID 0x%08x ", chip->id);
        if (chip->chipdata!=0) {
            printf("  %s ir_len=%d ver=%d\n", chip->chipdata->name,
                chip->chipdata->ir_len, chip->version);
#ifdef JTAG_DEBUG
            printf("pre_d=%d after_d=%d pre_c=%d after_c=%d ir=%d %s\n",
                chip->pre_d_bits, chip->after_d_bits,
                chip->pre_c_bits, chip->after_c_bits,
                chip->chipdata->ir_len,
                chip->chipdata->name);
#endif
        } else {
            printf("  chip not known\n");
        }
    }
}
/****************************************************************************/
plerrcode
jtag_list(struct generic_dev* dev, int ci, int addr)
{
    struct jtag_chain chain;
    plerrcode pres=plOK;

    if ((pres=jtag_init_chain(&chain, dev, ci, addr))!=plOK)
        goto error;

    jtag_dump_chain(&chain);

error:
    jtag_free_chain(&chain);
    return pres;
}
/****************************************************************************/
plerrcode
jtag_check(struct generic_dev* dev, int ci, int addr)
{
    struct jtag_chain chain;
    plerrcode pres=plOK;
    unsigned int i;
    int res, irlen;

    if ((pres=jtag_init_chain(&chain, dev, ci, addr))!=plOK)
        goto error;

    jtag_dump_chain(&chain);

    irlen=0;
    for (i=0; i<chain.num_chips; i++) {
        irlen+=chain.chips[i].chipdata->ir_len;
    }

    res=jtag_irlen(&chain);
    if (res!=irlen) {
        printf("jtag_check: irlen is %d, not %d\n", res, irlen);
        return plErr_HW;
    }
    res=jtag_datalen(&chain);
    if (res<0 || (unsigned)res!=chain.num_chips) {
        printf("jtag_check: datalen is %d, not %d\n", res, chain.num_chips);
        return plErr_HW;
    }

error:
    jtag_free_chain(&chain);
    return pres;
}
/****************************************************************************/
plerrcode
jtag_read(struct generic_dev* dev, int ci, int addr, unsigned int idx,
    const char* name, ems_u32* usercode)
{
    struct jtag_chain chain;
    struct jtag_chip* chip;
    ssize_t size=0;
    void* data=0;
    plerrcode pres=plOK;
    int p=-1, res;

    if ((pres=jtag_init_chain(&chain, dev, ci, addr))!=plOK)
        goto error;

    /*jtag_dump_chain(&chain);*/

    if (idx>=chain.num_chips) {
        printf("jtag_read: chip idx %d too large\n", idx);
        pres=plErr_ArgRange;
        goto error;
    }

    chip=chain.chips+idx;
    size=chip->chipdata->mem_size;
    if (size<=0) {
        printf("jtag_read: chip %s is not a known memory device\n",
            chain.chips[idx].chipdata->name);
        pres=plErr_ArgRange;
        goto error;
    }

    if (!chip->chipdata->read) {
        printf("jtag_read: no read function for %s available\n",
                chip->chipdata->name);
        pres=plErr_ArgRange;
        goto error;
    }

    data=malloc(size);
    if (data==0) {
        printf("jtag_read: malloc(%lld): %s\n", (unsigned long long)size,
            strerror(errno));
        pres=plErr_NoMem;
        goto error;
    }

    res=chip->chipdata->read(chip, data, usercode);
    if (res) {
        pres=plErr_System;
        goto error;
    }

    p=open(name, O_RDWR|O_CREAT|O_TRUNC/*|O_EXCL*/, 0644);
    if (p<0) {
        printf("jtag_read: open \"%s\": %s\n", name, strerror(errno));
        pres=plErr_System;
        goto error;
    }

    if (write(p, data, size)!=size) {
        printf("jtag_read: write: %s\n", strerror(errno));
        pres=plErr_System;
        goto error;
    }

error:
    close(p);
    free(data);
    jtag_free_chain(&chain);
    return pres;
}
/****************************************************************************/
plerrcode
jtag_usercode(struct generic_dev* dev, int ci, int addr, unsigned int idx,
    ems_u32* usercode)
{
    struct jtag_chain chain;
    struct jtag_chip* chip;
    plerrcode pres=plOK;
    int res;

    if ((pres=jtag_init_chain(&chain, dev, ci, addr))!=plOK)
        goto error;

    if (idx>=chain.num_chips) {
        printf("jtag_usercode: chip idx %d too large\n", idx);
        pres=plErr_ArgRange;
        goto error;
    }

    chip=chain.chips+idx;

    if (!chip->chipdata->read) {
        printf("jtag_usercode: no read function for %s available\n",
                chip->chipdata->name);
        pres=plErr_ArgRange;
        goto error;
    }

    res=chip->chipdata->read(chip, 0, usercode);
    if (res) {
        pres=plErr_System;
        goto error;
    }

error:
    jtag_free_chain(&chain);
    return pres;
}
/****************************************************************************/
plerrcode
jtag_write(struct generic_dev* dev, int ci, int addr, unsigned int idx,
    const char* name, int use_usercode, ems_u32 usercode)
{
    struct jtag_chain chain;
    struct jtag_chip* chip;
    ssize_t size=0;
    void *data_w=0, *data_r=0;
    ems_u32 old_usercode, new_usercode;
    plerrcode pres=plOK;
    int res;

    if ((pres=jtag_init_chain(&chain, dev, ci, addr))!=plOK)
        goto error;

    /*jtag_dump_chain(&chain);*/

    if (idx>=chain.num_chips) {
        printf("jtag_write: chip idx %d too large\n", idx);
        pres=plErr_ArgRange;
        goto error;
    }

    chip=chain.chips+idx;
    size=chip->chipdata->mem_size;
    if (size<=0) {
        printf("jtag_write: chip %s is not a known memory device\n",
            chain.chips[idx].chipdata->name);
        pres=plErr_ArgRange;
        goto error;
    }

    if (!chip->chipdata->write || !chip->chipdata->read) {
        printf("jtag_write: no read or write function for %s available\n",
                chip->chipdata->name);
        pres=plErr_ArgRange;
        goto error;
    }

    data_r=malloc(size);
    data_w=malloc(size);
    if (data_r==0 || data_w==0) {
        printf("jtag_write: malloc(%lld): %s\n", (unsigned long long)size,
                strerror(errno));
        pres=plErr_NoMem;
        goto error;
    }

    if ((pres=jtag_read_data(name, data_w, size))!=plOK)
        goto error;

    res=chip->chipdata->read(chip, data_r, &old_usercode);
    if (res==-2) /* security bit set */
        goto write_anyway;
    if (res) {
        pres=plErr_System;
        goto error;
    }
    if (!(use_usercode && (old_usercode!=usercode))) {
        if (memcmp(data_r, data_w, size)==0) {
            printf("jtag_write: old and new codes are identical\n");
            pres=plOK;
            goto error;
        } else {
            printf("jtag_write: old and new codes are different\n");
        }
    } else {
        printf("jtag_write: usercodes are different:\n"
            "  use_usercode=%d old_usercode=%d usercode=%d\n",
            use_usercode, old_usercode, usercode);
    }
    if (!use_usercode)
        usercode=old_usercode;

write_anyway:
    res=chip->chipdata->write(chip, data_w, usercode);
    if (res) {
        pres=plErr_System;
        goto error;
    }

    /* verify */
    res=chip->chipdata->read(chip, data_r, &new_usercode);
    if (res) {
        pres=plErr_System;
        goto error;
    }
#if 1
    if (memcmp(data_r, data_w, size)!=0) {
        printf("jtag_write: verify failed!\n");
        pres=plErr_Verify;
        goto error;
    }
#endif
    if (new_usercode!=usercode) {
        printf("jtag_write: verify of usercode failed!\n");
        pres=plErr_Verify;
        goto error;
    }
    printf("jtag_write successfull.\n");

error:
    free(data_r);
    free(data_w);
    jtag_free_chain(&chain);
    return pres;
}
/****************************************************************************/
plerrcode
jtag_verify(struct generic_dev* dev, int ci, int addr, unsigned int idx,
    const char* name)
{
    struct jtag_chain chain;
    struct jtag_chip* chip;
    ssize_t size=0;
    void *data_w=0, *data_r=0;
    plerrcode pres=plOK;
    ems_u32 usercode;
    int res;

    if ((pres=jtag_init_chain(&chain, dev, ci, addr))!=plOK)
        goto error;

    if (idx>=chain.num_chips) {
        printf("jtag_verify: chip idx %d too large\n", idx);
        pres=plErr_ArgRange;
        goto error;
    }

    chip=chain.chips+idx;
    size=chip->chipdata->mem_size;
    if (size<=0) {
        printf("jtag_verify: chip %s is not a known memory device\n",
            chain.chips[idx].chipdata->name);
        pres=plErr_ArgRange;
        goto error;
    }

    if (!chip->chipdata->read) {
        printf("jtag_read: no read function for %s available\n",
                chip->chipdata->name);
        pres=plErr_ArgRange;
        goto error;
    }

    data_r=malloc(size);
    data_w=malloc(size);
    if (data_r==0 || data_w==0) {
        printf("jtag_verify: malloc(%lld): %s\n", (unsigned long long)size,
                strerror(errno));
        pres=plErr_NoMem;
        goto error;
    }

    if ((pres=jtag_read_data(name, data_w, size))!=plOK)
        goto error;

    res=chip->chipdata->read(chip, data_r, &usercode);
    if (res) {
        pres=plErr_System;
        goto error;
    }
    if (memcmp(data_r, data_w, size)!=0) {
        printf("jtag_verify: verify failed!\n");
        pres=plErr_Verify;
        goto error;
    }

error:
    free(data_r);
    free(data_w);
    jtag_free_chain(&chain);
    return pres;
}
/****************************************************************************/
/****************************************************************************/
