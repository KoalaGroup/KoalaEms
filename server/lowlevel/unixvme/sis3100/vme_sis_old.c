/*
 * lowlevel/unixvme/sis3100/vme_sis.c
 * $ZEL: vme_sis.c,v 1.21 2004/06/18 20:00:14 wuestner Exp $
 * created 09.Jan.2002 PW
 * 07.Sep.2002 PW code for A24
 */

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include "../../../commu/commu.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "dev/pci/sis1100_var.h"
#include "../vme.h"
#include "../vme_address_modifiers.h"
#include "unixvme_sis3100.h"
#include "dsp_sis.h"
#include "mem_sis.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

static ems_u8* sis3100_vme_delay_buffer;
static size_t sis3100_vme_delay_buffer_size;

/*****************************************************************************/
static int
set_datasize(struct vme_dev* dev, int size, int am)
{
    struct vmespace s;
    int c;

    if ((size==dev->info.sis3100.current_datasize) &&
        (am==dev->info.sis3100.current_am)) return 0;

    switch (size) {
    case 1: /* nobreak */
    case 2: /* nobreak */
    case 4:
        s.datasize = size;
        break;
    default:
        printf("invalid datasize %d\n", size);
        return EINVAL;
    }
    s.am = am;
    s.swap = 0;
    s.mindmalen = -1; /* no change */
    s.mapit = 0; /* ignored anyway */
/*
    printf("ioctl(vme, SETVMESPACE, am=%d, swap=%d, mindmalen=%d, mapit=%d"
        ", datasize=%d)\n",
        s.am, s.swap, s.mindmalen, s.mapit, s.datasize);
*/
    c = ioctl(dev->info.sis3100.p_vme, SETVMESPACE, &s);
    if (c < 0) {
        printf("lowlevel/unixvme/sis3100/vme_sis.c: SETVMESPACE: %s\n",
            strerror(errno));
        return c;
    } else {
        dev->info.sis3100.current_datasize=size;
        dev->info.sis3100.current_am=am;
    }
    return 0;
}
/*****************************************************************************/
static int
vme_read(struct vme_dev* dev, ems_u32 adr, int am, ems_u32* data,
    int size, int datasize, ems_u32** newdata)
{
    int res;

    if ((res=set_datasize(dev, datasize, am))) {
        if (newdata) *newdata=data;
        return -res;
    }
    res = pread(dev->info.sis3100.p_vme, data, size, adr);
    if (res>=0) {
/*
        int i;
        printf("vme_read(addr=0x%x am=0x%02x size=%d datasize=%d): ",
            adr, am, size, datasize);
        for (i=0; i<res; i++) {
            printf("%02x ", ((ems_u8*)data)[i]);
        }
        printf("\n");
*/
        if (newdata) *newdata=((int*)(((unsigned long)data + res + 3) & ~3));
        return res;
    } else {
        printf("lowlevel/unixvme/sis3100/vme_sis.c: vme_read from 0x%08x "
            "(am=0x%x size=%d datasize=%d): %s\n",
            adr, am, size, datasize, strerror(errno));
        if (newdata) *newdata=data;
        return -errno;
    }
}
/*****************************************************************************/
static int
vme_read_a32(struct vme_dev* dev, ems_u32 adr, ems_u32* data,
    int size, int datasize, ems_u32** newdata)
{
    int am, res;

    am=datasize==size?AM_a32_data:AM_a32_block;
    res=vme_read(dev, adr, am, data, size, datasize, newdata);
    return res;
}
/*****************************************************************************/
static int
vme_read_a32_fifo(struct vme_dev* dev, ems_u32 adr, ems_u32* data,
    int size, int datasize, ems_u32** newdata)
{
    int res;
    struct sis1100_vme_block_req req;

    req.size=datasize;
    req.fifo=1;
    req.num=size/datasize;
    req.am=0x0b;
    req.addr=adr;
    req.data=(u_int8_t*)data;
    req.error=0;
/*
printf("vme_read_a32_fifo: size=  %d\n", req.size);
printf("                   fifo=  %d\n", req.fifo);
printf("                    num=  %d\n", req.num);
printf("                     am=0x%x\n", req.am);
printf("                   addr=0x%x\n", req.addr);
printf("                   data=%p\n", req.data);
*/
    res=ioctl(dev->info.sis3100.p_vme, SIS3100_VME_BLOCK_READ, &req);
    if (res) {
        printf("vme_read_a32_fifo: %s\n", strerror(errno));
        if (newdata) *newdata=data;
        return -1;
    }
    if (req.error && ((req.error&0x200)!=0x200)) {
        printf("vme_read_a32_fifo: error=%x\n", req.error);
        if (newdata) *newdata=data;
        return -1;
    }
    if (newdata) *newdata=data+req.num;
    return req.num*datasize;
}
/*****************************************************************************/
static int
vme_write(struct vme_dev* dev, ems_u32 adr, int am, ems_u32* data,
    int size, int datasize)
{
    int res;

    if ((res=set_datasize(dev, datasize, am))) return -res;
    res=pwrite(dev->info.sis3100.p_vme, data, size, adr);
    if (res>=0) {
        return res;
    } else {
        printf("lowlevel/unixvme/sis3100/vme_sis.c: vme_write to 0x%08x "
            "(am=0x%x size=%d datasize=%d): %s\n",
            adr, am, size, datasize, strerror(errno));
        return -errno;
    }
}
/*****************************************************************************/
static int
vme_write_a32(struct vme_dev* dev, ems_u32 adr, ems_u32* data,
    int size, int datasize)
{
    int am, res;

    am=datasize?AM_a32_data:AM_a32_block;
    res=vme_write(dev, adr, am, data, size, datasize);
    return res;
}
/*****************************************************************************/
static int
vme_read_a24(struct vme_dev* dev, ems_u32 adr, ems_u32* data,
    int size, int datasize, ems_u32** newdata)
{
    int am, res;

    am=datasize?AM_a24_data:AM_a24_block;
    res=vme_read(dev, adr, am, data, size, datasize, newdata);
    return res;
}
/*****************************************************************************/
static int
vme_write_a24(struct vme_dev* dev, ems_u32 adr, ems_u32* data,
    int size, int datasize)
{
    int am, res;

    am=datasize?AM_a24_data:AM_a24_block;
    res=vme_write(dev, adr, am, data, size, datasize);
    return res;
}
/*****************************************************************************/
static int
vme_read_aXd32(struct vme_dev* dev, int am, ems_u32 adr,
    ems_u32* data)
{
    struct sis1100_vme_req req;
    int res;

    req.size=4;
    req.am=am;
    req.addr=adr;

    res=ioctl(dev->info.sis3100.p_vme, SIS3100_VME_READ, &req);
    if (res) {
        printf("vme_read_aXd32: %s\n", strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_read_aXd32: error=0x%x\n", req.error);
        return -EIO;
    }
    *data=req.data;
    return 4;
}
/*****************************************************************************/
static int
vme_read_a32d32(struct vme_dev* dev, ems_u32 adr, ems_u32* data)
{
    struct sis1100_vme_req req;
    int res;

    req.size=4;
    req.am=AM_a32_data;
    req.addr=adr;

    res=ioctl(dev->info.sis3100.p_vme, SIS3100_VME_READ, &req);
    if (res) {
        printf("vme_read_a32d32: %s\n", strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_read_a32d32: error=0x%x\n", req.error);
        return -EIO;
    }
    *data=req.data;
    return 4;
}
/*****************************************************************************/
static int
vme_read_a24d32(struct vme_dev* dev, ems_u32 adr, ems_u32* data)
{
    struct sis1100_vme_req req;
    int res;

    req.size=4;
    req.am=AM_a24_data;
    req.addr=adr;

    res=ioctl(dev->info.sis3100.p_vme, SIS3100_VME_READ, &req);
    if (res) {
        printf("vme_read_a24d32: %s\n", strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_read_24d32: error=0x%x\n", req.error);
        return -EIO;
    }
    *data=req.data;
    return 4;
}
/*****************************************************************************/
static int
vme_read_aXd16(struct vme_dev* dev, int am, ems_u32 adr,
    ems_u16* data)
{
    struct sis1100_vme_req req;
    int res;

    req.size=2;
    req.am=am;
    req.addr=adr;

    res=ioctl(dev->info.sis3100.p_vme, SIS3100_VME_READ, &req);
    if (res) {
        printf("vme_read_aXd16: %s\n", strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_read_aXd16: error=0x%x\n", req.error);
        return -EIO;
    }
    *data=req.data;
    return 2;
}
/*****************************************************************************/
static int
vme_read_a32d16(struct vme_dev* dev, ems_u32 adr, ems_u16* data)
{
    struct sis1100_vme_req req;
    int res;

    req.size=2;
    req.am=AM_a32_data;
    req.addr=adr;

    res=ioctl(dev->info.sis3100.p_vme, SIS3100_VME_READ, &req);
    if (res) {
        printf("vme_read_a32d16: %s\n", strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_read_a32d16: error=0x%x\n", req.error);
        return -EIO;
    }
    *data=req.data;
    return 2;
}
/*****************************************************************************/
static int
vme_read_a24d16(struct vme_dev* dev, ems_u32 adr, ems_u16* data)
{
    struct sis1100_vme_req req;
    int res;

    req.size=2;
    req.am=AM_a24_data;
    req.addr=adr;

    res=ioctl(dev->info.sis3100.p_vme, SIS3100_VME_READ, &req);
    if (res) {
        printf("vme_read_a24d16: %s\n", strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_read_a24d16: error=0x%x\n", req.error);
        return -EIO;
    }
    *data=req.data;
    return 2;
}
/*****************************************************************************/
static int
vme_read_aXd8(struct vme_dev* dev, int am, ems_u32 adr, ems_u8* data)
{
    struct sis1100_vme_req req;
    int res;

    req.size=1;
    req.am=am;
    req.addr=adr;

    res=ioctl(dev->info.sis3100.p_vme, SIS3100_VME_READ, &req);
    if (res) {
        printf("vme_read_aXd8: %s\n", strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_read_aXd8: error=0x%x\n", req.error);
        return -EIO;
    }
    *data=req.data;
    return 1;
}
/*****************************************************************************/
static int
vme_read_a32d8(struct vme_dev* dev, ems_u32 adr, ems_u8* data)
{
    struct sis1100_vme_req req;
    int res;

    req.size=1;
    req.am=AM_a32_data;
    req.addr=adr;

    res=ioctl(dev->info.sis3100.p_vme, SIS3100_VME_READ, &req);
    if (res) {
        printf("vme_read_a32d8: %s\n", strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_read_a32d8: error=0x%x\n", req.error);
        return -EIO;
    }
    *data=req.data;
    return 1;
}
/*****************************************************************************/
static int
vme_read_a24d8(struct vme_dev* dev, ems_u32 adr, ems_u8* data)
{
    struct sis1100_vme_req req;
    int res;

    req.size=1;
    req.am=AM_a24_data;
    req.addr=adr;

    res=ioctl(dev->info.sis3100.p_vme, SIS3100_VME_READ, &req);
    if (res) {
        printf("vme_read_a24d8: %s\n", strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_read_a24d8: error=0x%x\n", req.error);
        return -EIO;
    }
    *data=req.data;
    return 1;
}
/*****************************************************************************/
static int
vme_write_aXd32(struct vme_dev* dev, int am, ems_u32 adr, ems_u32 data)
{
    struct sis1100_vme_req req;
    int res;

    req.size=4;
    req.am=am;
    req.addr=adr;
    req.data=data;

    res=ioctl(dev->info.sis3100.p_vme, SIS3100_VME_WRITE, &req);
    if (res) {
        printf("vme_write_aXd32: %s\n", strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_write_aXd32: error=0x%x\n", req.error);
        return -EIO;
    }
    return 4;
}
/*****************************************************************************/
static int
vme_write_a32d32(struct vme_dev* dev, ems_u32 adr, ems_u32 data)
{
    struct sis1100_vme_req req;
    int res;

    req.size=4;
    req.am=AM_a32_data;
    req.addr=adr;
    req.data=data;
#if 0
    printf("write_a32d32(addr=0x%08x size=%d am=0x%02x val=0x%08x)\n",
            req.addr, req.size, req.am, req.data);
#endif
    res=ioctl(dev->info.sis3100.p_vme, SIS3100_VME_WRITE, &req);
    if (res) {
        printf("vme_write_a32d32: %s\n", strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_write_a32d32: error=0x%x\n", req.error);
        return -EIO;
    }
    return 4;
}
/*****************************************************************************/
static int
vme_write_a24d32(struct vme_dev* dev, ems_u32 adr, ems_u32 data)
{
    struct sis1100_vme_req req;
    int res;

    req.size=4;
    req.am=AM_a24_data;
    req.addr=adr;
    req.data=data;

    res=ioctl(dev->info.sis3100.p_vme, SIS3100_VME_WRITE, &req);
    if (res) {
        printf("vme_write_a24d32: %s\n", strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_write_a24d32: error=0x%x\n", req.error);
        return -EIO;
    }
    return 4;
}
/*****************************************************************************/
static int
vme_write_aXd16(struct vme_dev* dev, int am, ems_u32 adr, ems_u16 data)
{
    struct sis1100_vme_req req;
    int res;

    req.size=2;
    req.am=am;
    req.addr=adr;
    req.data=data;

    res=ioctl(dev->info.sis3100.p_vme, SIS3100_VME_WRITE, &req);
    if (res) {
        printf("vme_write_aXd16: %s\n", strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_write_aXd16: error=0x%x\n", req.error);
        return -EIO;
    }
    return 2;
}
/*****************************************************************************/
static int
vme_write_a32d16(struct vme_dev* dev, ems_u32 adr, ems_u16 data)
{
    struct sis1100_vme_req req;
    int res;

    req.size=2;
    req.am=AM_a32_data;
    req.addr=adr;
    req.data=data;

    res=ioctl(dev->info.sis3100.p_vme, SIS3100_VME_WRITE, &req);
    if (res) {
        printf("vme_write_a32d16: %s\n", strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_write_a32d16: error=0x%x\n", req.error);
        return -EIO;
    }
    return 2;
}
/*****************************************************************************/
static int
vme_write_a24d16(struct vme_dev* dev, ems_u32 adr, ems_u16 data)
{
    struct sis1100_vme_req req;
    int res;

    req.size=2;
    req.am=AM_a24_data;
    req.addr=adr;
    req.data=data;

    res=ioctl(dev->info.sis3100.p_vme, SIS3100_VME_WRITE, &req);
    if (res) {
        printf("vme_write_a24d16: %s\n", strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_write_a24d16: error=0x%x\n", req.error);
        return -EIO;
    }
    return 2;
}
/*****************************************************************************/
static int
vme_write_aXd8(struct vme_dev* dev, int am, ems_u32 adr, ems_u8 data)
{
    struct sis1100_vme_req req;
    int res;

    req.size=1;
    req.am=AM_a32_data;
    req.addr=adr;
    req.data=data;

    res=ioctl(dev->info.sis3100.p_vme, SIS3100_VME_WRITE, &req);
    if (res) {
        printf("vme_write_aXd8: %s\n", strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_write_aXd8: error=0x%x\n", req.error);
        return -EIO;
    }
    return 1;
}
/*****************************************************************************/
static int
vme_write_a32d8(struct vme_dev* dev, ems_u32 adr, ems_u8 data)
{
    struct sis1100_vme_req req;
    int res;

    req.size=1;
    req.am=AM_a32_data;
    req.addr=adr;
    req.data=data;

    res=ioctl(dev->info.sis3100.p_vme, SIS3100_VME_WRITE, &req);
    if (res) {
        printf("vme_write_a32d8: %s\n", strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_write_a32d8: error=0x%x\n", req.error);
        return -EIO;
    }
    return 1;
}
/*****************************************************************************/
static int
vme_write_a24d8(struct vme_dev* dev, ems_u32 adr, ems_u8 data)
{
    struct sis1100_vme_req req;
    int res;

    req.size=1;
    req.am=AM_a24_data;
    req.addr=adr;
    req.data=data;

    res=ioctl(dev->info.sis3100.p_vme, SIS3100_VME_WRITE, &req);
    if (res) {
        printf("vme_write_a24d8: %s\n", strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_write_a24d8: error=0x%x\n", req.error);
        return -EIO;
    }
    return 1;
}
/*****************************************************************************/
static int
vme_berrtimer(struct vme_dev* dev, int to)
{
    struct sis1100_ctrl_reg reg;
    int berr_code, berr=0;

    reg.offset=0x100;
    if (ioctl(dev->info.sis3100.p_vme, SIS1100_CTRL_READ, &reg)<0) {
        printf("ioctl(SIS1100_CTRL_READ(0x100)): %s\n", strerror(errno));
        return -1;
    }
    if (reg.error) {
        printf("ioctl(SIS1100_CTRL_READ(0x100)): error=0x%x\n", reg.error);
        return -1;
    }
    berr_code=(reg.val>>14)&3;
    switch (berr_code) {
        case 0: berr=1250; break;
        case 1: berr=6250; break;
        case 2: berr=12500; break;
        case 3: berr=100000; break;
    }
    printf("berr was %d ns\n", berr);

    berr_code=3;
    if (to<=12500) berr_code=2;
    if (to<=6250) berr_code=1;
    if (to<=1250) berr_code=0;
    reg.val=(berr_code<<14)|((~berr_code&3)<<30);
    if (ioctl(dev->info.sis3100.p_vme, SIS1100_CTRL_WRITE, &reg)<0) {
        printf("ioctl(SIS1100_CTRL_WRITE(0x100)): %s\n", strerror(errno));
        return -1;
    }
    if (reg.error) {
        printf("ioctl(SIS1100_CTRL_WRITE(0x100)): error=0x%x\n", reg.error);
        return -1;
    }
    return berr;
}
/*****************************************************************************/
static int
vme_arbtimer(struct vme_dev* dev, int to)
{
    return 0;
}
/*****************************************************************************/
static int
sis3100_set_user_led(struct vme_sis_info* info, int on)
{
    struct sis1100_ctrl_reg req;
    int res;

    req.offset=0x100;
    req.val=0x80;
    if (!on) req.val<<=16;

    res=ioctl(info->p_vme, SIS1100_CTRL_WRITE, &req);
    if (res) {
        printf("switch user LED: %s\n", strerror(errno));
        return -1;
    }
    if (req.error) {
        printf("switch user LED: error=0x%x\n", req.error);
        return -1;
    }
    return 0;
}
/*****************************************************************************/
static int
sis3100_get_user_led(struct vme_sis_info* info, int* on)
{
    struct sis1100_ctrl_reg req;
    int res;

    req.offset=0x100;

    res=ioctl(info->p_vme, SIS1100_CTRL_READ, &req);
    *on=!!(req.val&0x80);
    if (res) {
        printf("read user LED: %s\n", strerror(errno));
        return -1;
    }
    if (req.error) {
        printf("read user LED: error=0x%x\n", req.error);
        return -1;
    }
    return 0;
}
/*****************************************************************************/
static void
vme_reset_delayed_read(struct generic_dev* gdev)
{
    struct vme_dev* dev=(struct vme_dev*)gdev;
    struct vme_sis_info* info=&dev->info.sis3100;

printf("sis3100dsp_reset_delayed_read\n");
    info->num_hunks=0;
}

/*****************************************************************************/
static int
vme_read_delayed(struct generic_dev* gdev)
{
    struct vme_dev* dev=(struct vme_dev*)gdev;
    struct vme_sis_info* info=&dev->info.sis3100;
    struct delayed_hunk* hunks=info->hunk_list;
    int i, res, hunksum;
    ems_u8* delay_buffer;

    if (!info->num_hunks) return 0;
    hunksum=hunks[info->num_hunks-1].src+
         hunks[info->num_hunks-1].size-
         hunks[0].src;

    if (sis3100_vme_delay_buffer_size<hunksum) {
        printf("buffer_size=%lld, hunksum=%d, buffer=%p\n",
                (unsigned long long)sis3100_vme_delay_buffer_size,
                hunksum,
                sis3100_vme_delay_buffer);
        free(sis3100_vme_delay_buffer);
        sis3100_vme_delay_buffer=calloc(hunksum, 1);
        if (!sis3100_vme_delay_buffer) {
            printf("sis3100_read_delayed: malloc(%d):%s\n",
                hunksum, strerror(errno));
            return -1;
        }
        printf("new buffer=%p\n", sis3100_vme_delay_buffer);
        sis3100_vme_delay_buffer_size=hunksum;
    }
/*
printf("read_delayed: pread(%p, %d, %08llx)\n", sis3100_vme_delay_buffer,
        hunksum, info->hunk_list[0].src);
*/
    res=pread(info->p_mem, sis3100_vme_delay_buffer,
            hunksum, info->hunk_list[0].src);
    if (res!=hunksum) {
        printf("sis3100_read_delayed: pread: size=%d res=%d\n",
                hunksum, res);
        return -1;
    }

    delay_buffer=sis3100_vme_delay_buffer-info->hunk_list[0].src;
    for (i=0; i<info->num_hunks; i++) {
        struct delayed_hunk* hunk=info->hunk_list+i;
/*
printf("read_delayed: memcpy(%p, %p (0x%llx), %d)\n", hunk->dst,
    delay_buffer+hunk->src, hunk->src, hunk->size);
*/
        memcpy(hunk->dst, delay_buffer+hunk->src, hunk->size);
    }

    info->num_hunks=0;
    if (pwrite(info->p_mem, &info->bufferstart, 4, info->bufferaddr)<0) {
        printf("vme_read_delayed: pwrite: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}
/*****************************************************************************/
int
sis3100_vme_delay_read(struct vme_dev* dev,
        off_t src, void* dst, size_t size)
{
    struct vme_sis_info* info=&dev->info.sis3100;
    struct delayed_hunk* hunk;

/*
printf("delay_read: src=0x%08llx dst=%p size=%d\n", src, dst, size);
*/
    if (info->hunk_list_size==info->num_hunks) {
        int i;
        struct delayed_hunk* help;
printf("delay_read: allocating hunk %d\n", info->num_hunks);
        help=malloc((info->num_hunks+100)*sizeof(struct delayed_hunk));
        if (!help) {
            printf("sis3100_vme_delay_read: alloc %d hunks: %s\n",
                    info->num_hunks+100, strerror(errno));
            return -1;
        }
        for (i=info->num_hunks-1; i>=0; i--) help[i]=info->hunk_list[i];
        free(info->hunk_list);
        info->hunk_list=help;
        info->hunk_list_size+=100;
    }
    hunk=info->hunk_list+info->num_hunks;
    hunk->src=src;
    hunk->dst=dst;
    hunk->size=size;
    info->num_hunks++;
    return 0;
}
/*****************************************************************************/
static int
vme_enable_delayed_read(struct generic_dev* gdev, int val)
{
    struct vme_dev* dev=(struct vme_dev*)gdev;
    struct vme_sis_info* info=&dev->info.sis3100;
    int old;

    old=info->delayed_read_enabled;
    info->delayed_read_enabled=val;
    printf("delayed read for VME %sabled.\n", val?"en":"dis");
    return old;
}
/*****************************************************************************/
static struct dsp_procs dsp_procs={
    sis3100_dsp_present,
    sis3100_dsp_reset,
    sis3100_dsp_start,
    sis3100_dsp_load,
};

static struct mem_procs mem_procs={
    sis3100_mem_size,
    sis3100_mem_write,
    sis3100_mem_read,
    sis3100_mem_read_delayed,
    sis3100_mem_set_bufferstart,
};

static struct dsp_procs*
get_dsp_procs(struct vme_dev* dev)
{
    return &dsp_procs;
}

static struct mem_procs*
get_mem_procs(struct vme_dev* dev)
{
    return &mem_procs;
}
/*****************************************************************************/
static errcode
vme_done_sis(struct generic_dev* gdev)
{
    struct vme_dev* dev=(struct vme_dev*)gdev;

    if (dev->info.sis3100.st) {
        sched_delete_select_task(dev->info.sis3100.st);
    }

    if (dev->info.sis3100.p_vme>=0) {
        printf("close(vme_sis/vme)\n");
        close(dev->info.sis3100.p_vme);
    }
    if (dev->info.sis3100.p_ctrl>=0) {
        printf("close(vme_sis/ctrl)\n");
        close(dev->info.sis3100.p_ctrl);
    }
    if (dev->info.sis3100.p_mem>=0) {
        printf("close(vme_sis/mem)\n");
        close(dev->info.sis3100.p_mem);
    }
    if (dev->info.sis3100.p_dsp>=0) {
        printf("close(vme_sis/dsp)\n");
        close(dev->info.sis3100.p_dsp);
    }
    return OK;
}
/*****************************************************************************/
static const char *sis1100names[]={"vme", "ram", "ctrl", "dsp"};

static int
open_dev(const char* pathname, const char* suffix)
{
    char* pname;
    int p, len;

    len=strlen(pathname)+strlen(suffix)+1;
    pname=malloc(len);
    if (!pname) {
        printf("open VME: malloc: %s\n", strerror(errno));
        return -1;
    }
    snprintf(pname, len, "%s%s", pathname, suffix);
    printf("vme_init_sis3100: open(%s)\n", pname);
    p=open(pname, O_RDWR, 0);
    if (p<0) {
        printf("open VME (sis3100) %s: %s\n", pname, strerror(errno));
        free(pname);
        return -1;
    }
    if (fcntl(p, F_SETFD, FD_CLOEXEC)<0) {
        printf("fcntl(VME %s, FD_CLOEXEC): %s\n", pname, strerror(errno));
    }
    free(pname);
    return p;
}
/*****************************************************************************/
static void
sis3100_link_up_down(int p, select_types selected, callbackdata data)
{
    struct generic_dev* dev=(struct generic_dev*)data.p;
    struct vme_dev* vmedev=(struct vme_dev*)dev;
    struct vme_sis_info* info=&vmedev->info.sis3100;
    struct sis1100_irq_get get;
    int res;

    res=read(p, &get, sizeof(struct sis1100_irq_get));
    if (res!=sizeof(struct sis1100_irq_get)) {
        printf("sis3100_link_up_down: res=%d errno=%s\n", res, strerror(errno));
        return;
    }
    if (get.remote_status) {
        int crate=find_device_index(modul_vme, dev);
        int initialised;
        int on, up;

        up=get.remote_status>0;
        printf("sis3100: VME crate %d is %s\n", crate, up?"up":"down");
        initialised=0;
        if (up) {
            int res;
            res=sis3100_get_user_led(info, &on);
            if (!res && on) initialised=1;
            printf("sis3100: it is %s initialised\n",
                    initialised?"still":"not any more"); 
        }
        send_unsol_var(Unsol_DeviceDisconnect, 4, modul_vme,
            crate, get.remote_status, initialised);
    } else {
        printf("sis3100_link_up_down: status not changed\n");
    }
}
/*****************************************************************************/
errcode
vme_init_sis3100(char* pathname, struct vme_dev* dev)
{
    struct vme_sis_info* info=&dev->info.sis3100;
    struct sis1100_irq_ctl ctl;

    printf("vme_init_sis3100: path=%s\n", pathname);

    if ((info->p_ctrl=
            open_dev(pathname, sis1100names[sis1100_subdev_ctrl]))<0) {
        return Err_System;
    }

    if ((info->p_vme=
            open_dev(pathname, sis1100names[sis1100_subdev_remote]))<0) {
        close(info->p_ctrl);
        return Err_System;
    }

    if ((info->p_mem=
            open_dev(pathname, sis1100names[sis1100_subdev_ram]))<0) {
        close(info->p_ctrl);
        close(info->p_vme);
        return Err_System;
    }

    if ((info->p_dsp=
            open_dev(pathname, sis1100names[sis1100_subdev_dsp]))<0) {
        close(info->p_ctrl);
        close(info->p_vme);
        close(info->p_mem);
        return Err_System;
    }

    info->st=0;

    if (fcntl(info->p_ctrl, F_SETFL, O_NDELAY)<0) {
        printf("vme_init_sis3100: fcntl(p_ctrl, F_SETFL): %s\n", strerror(errno));
    }

    ctl.irq_mask=0;
    ctl.signal=-1;
    if (ioctl(info->p_ctrl, SIS1100_IRQ_CTL, &ctl)) {
        printf("vme_init_sis3100: SIS1100_IRQ_CTL: %s\n", strerror(errno));
    } else {
        callbackdata cbdata;
        cbdata.p=dev;
        info->st=sched_insert_select_task(sis3100_link_up_down, cbdata,
            "sis3100_link", info->p_ctrl, select_read
    #ifdef SELECT_STATIST
            , 1
    #endif
            );
        if (!info->st) {
            printf("vme_init_sis3100: cannot install callback for link_up_down\n");
        }
    }

    info->current_datasize=-1;

    info->hunk_list=0;
    info->hunk_list_size=0;
    info->delayed_read_enabled=1;
    vme_reset_delayed_read((struct generic_dev*)dev);

    dev->generic.done=vme_done_sis;

    dev->generic.reset_delayed_read=vme_reset_delayed_read;
    dev->generic.read_delayed=vme_read_delayed;
    dev->generic.enable_delayed_read=vme_enable_delayed_read;

    dev->get_dsp_procs=get_dsp_procs;
    dev->get_mem_procs=get_mem_procs;

    dev->berrtimer=vme_berrtimer;
    dev->arbtimer=vme_arbtimer;

    dev->read=vme_read;
    dev->write=vme_write;

    dev->read_a32=vme_read_a32;
    dev->write_a32=vme_write_a32;
    dev->read_a24=vme_read_a24;
    dev->write_a24=vme_write_a24;
    /*dev->read_aX=vme_read_aX;*/
    /*dev->write_aX=vme_write_aX;*/

    dev->read_a32_fifo=vme_read_a32_fifo;

    dev->read_a32d32=vme_read_a32d32;
    dev->write_a32d32=vme_write_a32d32;
    dev->read_a32d16=vme_read_a32d16;
    dev->write_a32d16=vme_write_a32d16;
    dev->read_a32d8=vme_read_a32d8;
    dev->write_a32d8=vme_write_a32d8;

    dev->read_a24d32=vme_read_a24d32;
    dev->write_a24d32=vme_write_a24d32;
    dev->read_a24d16=vme_read_a24d16;
    dev->write_a24d16=vme_write_a24d16;
    dev->read_a24d8=vme_read_a24d8;
    dev->write_a24d8=vme_write_a24d8;

    dev->read_aXd32=vme_read_aXd32;
    dev->write_aXd32=vme_write_aXd32;
    dev->read_aXd16=vme_read_aXd16;
    dev->write_aXd16=vme_write_aXd16;
    dev->read_aXd8=vme_read_aXd8;
    dev->write_aXd8=vme_write_aXd8;

    sis3100_set_user_led(info, 1);

    return OK;
}
/*****************************************************************************/
volatile int x;
errcode unixvme_sis3100_low_init(char* nix) {
x=*(volatile int*)0;
    sis3100_vme_delay_buffer=0;
    sis3100_vme_delay_buffer_size=0;
    return OK;
}

errcode unixvme_sis3100_low_done(void) {
    if (sis3100_vme_delay_buffer) free(sis3100_vme_delay_buffer);
    sis3100_vme_delay_buffer=0;
    sis3100_vme_delay_buffer_size=0;
    return OK;
}
errcode unixvme_sis3100_low_printuse(FILE* nix) {return OK;}
/*****************************************************************************/
/*****************************************************************************/
