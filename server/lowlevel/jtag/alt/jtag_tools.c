/*
 * lowlevel/jtag_tools.c
 * created 12.Aug.2005 PW
 * $ZEL: jtag_tools.c,v 1.1 2006/09/15 17:54:01 wuestner Exp $
 */

#include <sconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "../devices.h"

#include "jtag_tools.h"
#include "jtag_low.h"
#include "jtag_int.h"

#ifdef LOWLEVEL_UNIXVME
#include "../unixvme/vme.h"
#endif
#ifdef LOWLEVEL_LVD
#include "../lvd/lvdbus.h"
#endif

int jtag_traced=0;

/****************************************************************************/
static plerrcode
jtag_init_dev(struct generic_dev* dev, struct jtag_dev* jdev, int ci, int addr)
{
    plerrcode pres;

    jdev->dev=dev;
    jdev->ci=ci;
    jdev->addr=addr;
    jdev->jt_data=0;
    jdev->jt_action=0;
    jdev->opaque=0;
    switch (dev->generic.class) {
#ifdef LOWLEVEL_UNIXVME
    case modul_vme: {
            struct vme_dev* vme_dev=(struct vme_dev*)dev;
            pres=vme_dev->init_jtag_dev(vme_dev, jdev);
        } break;
#endif
#ifdef LOWLEVEL_LVD
    case modul_lvd: {
            struct lvd_dev* lvd_dev=(struct lvd_dev*)dev;
            pres=lvd_dev->init_jtag_dev(lvd_dev, jdev);
        } break;
#endif
    default:
        printf("init_jtag_dev: jtag for this device (class %d) not supported\n",
                dev->generic.class);
        pres=plErr_BadModTyp;
    }
    return pres;
}
/****************************************************************************/
plerrcode
jtag_trace(struct generic_dev* dev, ems_u32 on, ems_u32* old)
{
#ifdef LVD_JTAG_TRACE
    *old=jtag_traced;
    jtag_traced=on;
    return plOK;
#else
    return plErr_NoSuchProc;
#endif
}
/****************************************************************************/
plerrcode
jtag_list(struct generic_dev* dev, int ci, int addr)
{
    struct jtag_chain chain;
    plerrcode pres=plOK;

    if ((pres=jtag_init_dev(dev, &chain.jtag_dev, ci, addr))!=plOK)
        return pres;

    if ((pres=jtag_init_chain(&chain))!=plOK)
        goto error;

    jtag_dump_chain(&chain);

error:
    jtag_free_chain(&chain);
    chain.jtag_dev.jt_free(&chain.jtag_dev);
    return pres;
}
/****************************************************************************/
plerrcode
jtag_check(struct generic_dev* dev, int ci, int addr)
{
    struct jtag_chain chain;
    plerrcode pres=plOK;
    int i, res, irlen;

    if ((pres=jtag_init_dev(dev, &chain.jtag_dev, ci, addr))!=plOK)
        return pres;

    if ((pres=jtag_init_chain(&chain))!=plOK)
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
    if (res!=chain.num_chips) {
        printf("jtag_check: datalen is %d, not %d\n", res, chain.num_chips);
        return plErr_HW;
    }

error:
    jtag_free_chain(&chain);
    chain.jtag_dev.jt_free(&chain.jtag_dev);
    return pres;
}
/****************************************************************************/
#ifdef NEVER
plerrcode jtag_read_id(struct generic_dev* dev, int ci, int addr,
    unsigned int chip, ems_u32* id)
{
    int trace_;
    struct jtag_chain chain;
    plerrcode pres=plOK;

    trace_=jtag_traced;
    jtag_traced=0;
    if ((pres=jtag_init_dev(dev, &chain.jtag_dev, ci, addr))!=plOK)
        return pres;

    if ((pres=jtag_init_chain(&chain))!=plOK)
        goto error;

    jtag_dump_chain(&chain);

    if (chip>=chain.num_chips) {
        pres=plErr_ArgRange;
        goto error;
    }

    jtag_traced=trace_;
    if (jtag_id(chain.chips+chip, id))
        pres=plErr_System;

error:
    jtag_free_chain(&chain);
    chain.jtag_dev.jt_free(&chain.jtag_dev);
    return pres;
}
#endif
/****************************************************************************/
plerrcode
jtag_read(struct generic_dev* dev, int ci, int addr, unsigned int idx,
    const char* name)
{
    struct jtag_chain chain;
    struct jtag_chip* chip;
    ssize_t size=0;
    void* data=0;
    plerrcode pres=plOK;
    int p=-1, res, found, i;

    if ((pres=jtag_init_dev(dev, &chain.jtag_dev, ci, addr))!=plOK)
        return pres;

    if ((pres=jtag_init_chain(&chain))!=plOK)
        goto error;

    jtag_dump_chain(&chain);

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

    data=malloc(size);
    if (data==0) {
        printf("jtag_read: malloc(%lld): %s\n", (unsigned long long)size,
            strerror(errno));
        pres=plErr_NoMem;
        goto error;
    }

    found=0; res=0;
    switch (chip->chipdata->vendor_id) {
    case VID_XILINX:
        switch (chip->chipdata->part_id) {
        case DID_XC18V01a:
        case DID_XC18V01b:
        case DID_XC18V02a:
        case DID_XC18V02b:
        case DID_XC18V04a:
        case DID_XC18V04b:
            found=1;
            res=jtag_read_XC18V00(chip, data);
            break;
        }
        break;
    }
    if (!found) {
        printf("jtag_read: no read function for %s available\n",
                chip->chipdata->name);
        pres=plErr_ArgRange;
        goto error;
    }
    if (res) {
        pres=plErr_System;
        goto error;
    }

    for (i=0; i<10; i++) {
        printf("%08x", ((ems_u32*)data)[i]);
    }
    printf("\n");

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
    chain.jtag_dev.jt_free(&chain.jtag_dev);
    return pres;
}
/****************************************************************************/
plerrcode
jtag_write(struct generic_dev* dev, int ci, int addr, unsigned int idx,
    const char* name)
{
    struct jtag_chain chain;
    struct jtag_chip* chip;
    struct stat statbuf;
    ssize_t size=0;
    void* data=0;
    plerrcode pres=plOK;
    int p=-1, res, found, i;

    if ((pres=jtag_init_dev(dev, &chain.jtag_dev, ci, addr))!=plOK)
        return pres;

    if ((pres=jtag_init_chain(&chain))!=plOK)
        goto error;

    jtag_dump_chain(&chain);

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

    data=malloc(size);
    if (data==0) {
        printf("jtag_write: malloc(%lld): %s\n", (unsigned long long)size,
                strerror(errno));
        pres=plErr_NoMem;
        goto error;
    }

    p=open(name, O_RDWR, 0);
    if (p<0) {
        printf("jtag_write: open \"%s\": %s\n", name, strerror(errno));
        pres=plErr_System;
        goto error;
    }

    if (fstat(p, &statbuf)<0) {
        printf("jtag_write: fstat \"%s\": %s\n", name, strerror(errno));
        pres=plErr_System;
        goto error;
    }

    if (statbuf.st_size!=size) {
        printf("jtag_write: file %s has not the required size "
            "(file: %lld %s: %d)\n",
            name, (unsigned long long)statbuf.st_size,
            chain.chips[idx].chipdata->name,
            chip->chipdata->mem_size);
        pres=plErr_ArgRange;
        goto error;
    }

    if (read(p, data, size)!=size) {
        printf("jtag_write: read: %s\n", strerror(errno));
        pres=plErr_System;
        goto error;
    }

    for (i=0; i<10; i++)
        printf("%08x\n", ((ems_u32*)data)[i]);

    found=0; res=0;
    switch (chip->chipdata->vendor_id) {
    case VID_XILINX:
        switch (chip->chipdata->part_id) {
        case DID_XC18V01a:
        case DID_XC18V01b:
        case DID_XC18V02a:
        case DID_XC18V02b:
        case DID_XC18V04a:
        case DID_XC18V04b:
            found=1;
            res=jtag_write_XC18V00(chip, data);
            break;
        }
        break;
    }
    if (!found) {
        printf("jtag_write: no write function for %s available\n",
                chip->chipdata->name);
        pres=plErr_ArgRange;
        goto error;
    }
    if (res) {
        pres=plErr_System;
        goto error;
    }

error:
    close(p);
    free(data);
    jtag_free_chain(&chain);
    chain.jtag_dev.jt_free(&chain.jtag_dev);
    return pres;
}
/****************************************************************************/
/****************************************************************************/
