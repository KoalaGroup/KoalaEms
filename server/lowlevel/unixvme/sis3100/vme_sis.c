/*
 * lowlevel/unixvme/sis3100/vme_sis.c
 * created 09.Jan.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: vme_sis.c,v 1.55 2017/10/22 23:37:44 wuestner Exp $";

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
#include <rcs_ids.h>

#include "dev/pci/sis1100_var.h"
#include "../vme.h"
#include "../vme_address_modifiers.h"
#include "../unixvme_jtag.h"
#include "vme_sis.h"
#include "dsp_sis.h"
#include "mem_sis.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#define IRQDEBUG 0

RCS_REGISTER(cvsid, "lowlevel/unixvme/sis3100")

#define HV_3100 1
#define HV_3104 2

/*****************************************************************************/
#if 0
static int
set_datasize(struct vme_dev* dev, int size, int am)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct vmespace s;
    int c;

    if ((size==info->current_datasize) &&
        (am==info->current_am)) return 0;

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
#if 0
    printf("ioctl(vme, SETVMESPACE, am=%d, swap=%d, mindmalen=%lld, mapit=%d"
        ", datasize=%d)\n",
        s.am, s.swap, (long long)s.mindmalen, s.mapit, s.datasize);
#endif
    c = ioctl(info->p_vme, SETVMESPACE, &s);
    if (c < 0) {
        printf("lowlevel/unixvme/sis3100/vme_sis.c: SETVMESPACE: %s\n",
            strerror(errno));
        return c;
    } else {
        info->current_datasize=size;
        info->current_am=am;
    }
    return 0;
}
#endif
/*****************************************************************************/
static int
vme_mindmalen(struct vme_dev* dev, int *len_read, int *len_write)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    int mindmalen[2];

    mindmalen[0]=*len_read;
    mindmalen[1]=*len_write;
    if (ioctl(info->p_vme, SIS1100_MINDMALEN, mindmalen)) {
        printf("mindmalen: %s\n", strerror(errno));
        return -1;
    }
    *len_read=mindmalen[0];
    *len_write=mindmalen[1];
    return 0;
}
/*****************************************************************************/
static int
vme_minpipelen(struct vme_dev* dev, int *len_read, int *len_write)
{
#ifdef SIS1100_MINPIPELEN
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    int minpipelen[2];

    minpipelen[0]=*len_read;
    minpipelen[1]=*len_write;
    if (ioctl(info->p_vme, SIS1100_MINPIPELEN, minpipelen)) {
        printf("minpipelen: %s\n", strerror(errno));
        return -1;
    }
    *len_read=minpipelen[0];
    *len_write=minpipelen[1];
    return 0;
#else
    printf("vme_minpipelen: SIS1100_MINPIPELEN not defined\n");
    return -1;
#endif
}
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_read(struct vme_dev* dev, ems_u32 adr, int am, ems_u32* data,
    int size, int datasize, ems_u32** newdata)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_block_req req;
    int res;

    req.size=datasize;
    req.fifo=0;
    req.num=size/datasize;
    req.am=am;
    req.addr=adr;
    req.data=(u_int8_t*)data;
    req.error=0;

    if (ioctl(info->p_vme, SIS3100_VME_BLOCK_READ, &req)) {
        printf("vme_read: %s\n", strerror(errno));
        if (newdata) *newdata=data;
        return -1;
    }
    if (req.error && ((req.error&0x200)!=0x200)) {
        printf("vme_read(addr=%08x am=%02x size=%d): error=%x, num=%llu\n",
            adr, am, datasize, req.error, (unsigned long long)req.num);
        if (newdata)
            *newdata=data;
        return -1;
    }
#if 0
    printf("vme_read(%08x %02x %d): error=%x, num=%llu data=0x%08x\n",
            adr, am, datasize, req.error, (unsigned long long)req.num,
            *data);
#endif
    res=req.num*datasize;
    if (newdata)
        *newdata=((ems_u32*)(((unsigned long)data+res+3)&~3));

    return res;
}
#endif
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
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_read_a32_fifo(struct vme_dev* dev, ems_u32 adr, ems_u32* data,
    int size, int datasize, ems_u32** newdata)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_block_req req;

    req.size=datasize;
    req.fifo=1;
    req.num=size/datasize;
    req.am=0x0b;
    req.addr=adr;
    req.data=(u_int8_t*)data;
    req.error=0;

    if (ioctl(info->p_vme, SIS3100_VME_BLOCK_READ, &req)) {
        printf("vme_read_a32_fifo: %s\n", strerror(errno));
        if (newdata)
            *newdata=data;
        return -1;
    }
    if (req.error && ((req.error&0x200)!=0x200)) {
        printf("vme_read_a32_fifo: error=%x\n", req.error);
        if (newdata)
            *newdata=data;
        return -1;
    }
    if (newdata)
        *newdata=data+req.num;
    return req.num*datasize;
}
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_write_a32_fifo(struct vme_dev* dev, ems_u32 adr, ems_u32* data,
    int size, int datasize)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_block_req req;

    req.size=datasize;
    req.fifo=1;
    req.num=size/datasize;
    req.am=0x0b;
    req.addr=adr;
    req.data=(u_int8_t*)data;
    req.error=0;

    if (ioctl(info->p_vme, SIS3100_VME_BLOCK_WRITE, &req)) {
        printf("vme_write_a32_fifo: %s\n", strerror(errno));
        return -1;
    }
    if (req.error && ((req.error&0x200)!=0x200)) {
        printf("vme_write_a32_fifo: error=%x\n", req.error);
        return -1;
    }
    return req.num*datasize;
}
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
#if 0
static int
vme_write(struct vme_dev* dev, ems_u32 adr, int am, ems_u32* data,
    int size, int datasize)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    int res;

    if ((res=set_datasize(dev, datasize, am)))
        return -res;
    res=pwrite(info->p_vme, data, size, adr);
    if (res>=0) {
        return res;
    } else {
        printf("lowlevel/unixvme/sis3100/vme_sis.c: vme_write to 0x%08x "
            "(am=0x%x size=%d datasize=%d): %s\n",
            adr, am, size, datasize, strerror(errno));
        return -errno;
    }
}
#else
static int
vme_write(struct vme_dev* dev, ems_u32 adr, int am, ems_u32* data,
    int size, int datasize)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_block_req req;

    req.size=datasize;
    req.fifo=0;
    req.num=size/datasize;
    req.am=am;
    req.addr=adr;
    req.data=(u_int8_t*)data;
    req.error=0;

    if (ioctl(info->p_vme, SIS3100_VME_BLOCK_WRITE, &req)) {
        printf("vme_write: %s\n", strerror(errno));
        return -1;
    }
    if (req.error && ((req.error&0x200)!=0x200 || !req.num)) {
        printf("vme_write(addr=%08x am=%02x size=%d): error=%x, num=%llu\n",
            adr, am, datasize, req.error, (unsigned long long)req.num);
        return -1;
    }
    return req.num*datasize;
}
#endif
#endif
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
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_read_aXd32(struct vme_dev* dev, int am, ems_u32 adr,
    ems_u32* data)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_req req;
    int res;

    req.size=4;
    req.am=am;
    req.addr=adr;

    res=ioctl(info->p_vme, SIS3100_VME_READ, &req);
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
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_read_a32d32(struct vme_dev* dev, ems_u32 adr, ems_u32* data)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_req req;
    int res;

    req.size=4;
    req.am=AM_a32_data;
    req.addr=adr;

    res=ioctl(info->p_vme, SIS3100_VME_READ, &req);
    if (res) {
        printf("vme_read_a32d32(%s, 0x%08x): %s\n", dev->pathname,
                adr, strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_read_a32d32(%s, 0x%08x): error=0x%x\n", dev->pathname,
                adr, req.error);
        return -EIO;
    }
    *data=req.data;
    return 4;
}
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_read_a24d32(struct vme_dev* dev, ems_u32 adr, ems_u32* data)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_req req;
    int res;

    req.size=4;
    req.am=AM_a24_data;
    req.addr=adr;

    res=ioctl(info->p_vme, SIS3100_VME_READ, &req);
    if (res) {
        printf("vme_read_a24d32(%s): %s\n", dev->pathname, strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_read_24d32(%s, 0x%08x): error=0x%x\n", dev->pathname,
                adr, req.error);
        return -EIO;
    }
    *data=req.data;
    return 4;
}
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_read_aXd16(struct vme_dev* dev, int am, ems_u32 adr,
    ems_u16* data)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_req req;
    int res;

    req.size=2;
    req.am=am;
    req.addr=adr;

    res=ioctl(info->p_vme, SIS3100_VME_READ, &req);
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
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_read_a32d16(struct vme_dev* dev, ems_u32 adr, ems_u16* data)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_req req;
    int res;

    req.size=2;
    req.am=AM_a32_data;
    req.addr=adr;

    res=ioctl(info->p_vme, SIS3100_VME_READ, &req);
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
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_read_a24d16(struct vme_dev* dev, ems_u32 adr, ems_u16* data)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_req req;
    int res;

    req.size=2;
    req.am=AM_a24_data;
    req.addr=adr;

    res=ioctl(info->p_vme, SIS3100_VME_READ, &req);
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
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_read_aXd8(struct vme_dev* dev, int am, ems_u32 adr, ems_u8* data)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_req req;
    int res;

    req.size=1;
    req.am=am;
    req.addr=adr;

    res=ioctl(info->p_vme, SIS3100_VME_READ, &req);
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
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_read_a32d8(struct vme_dev* dev, ems_u32 adr, ems_u8* data)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_req req;
    int res;

    req.size=1;
    req.am=AM_a32_data;
    req.addr=adr;

    res=ioctl(info->p_vme, SIS3100_VME_READ, &req);
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
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_read_a24d8(struct vme_dev* dev, ems_u32 adr, ems_u8* data)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_req req;
    int res;

    req.size=1;
    req.am=AM_a24_data;
    req.addr=adr;

    res=ioctl(info->p_vme, SIS3100_VME_READ, &req);
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
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_write_aXd32(struct vme_dev* dev, int am, ems_u32 adr, ems_u32 data)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_req req;
    int res;

    req.size=4;
    req.am=am;
    req.addr=adr;
    req.data=data;

    res=ioctl(info->p_vme, SIS3100_VME_WRITE, &req);
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
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_write_a32d32(struct vme_dev* dev, ems_u32 adr, ems_u32 data)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_req req;
    int res;

    req.size=4;
    req.am=AM_a32_data;
    req.addr=adr;
    req.data=data;

    res=ioctl(info->p_vme, SIS3100_VME_WRITE, &req);
    if (res) {
        printf("vme_write_a32d32(%s): %s\n", dev->pathname, strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_write_a32d32(%s, 0x%08x, 0x%08x): error=0x%x\n",
                dev->pathname, adr, data, req.error);
        return -EIO;
    }
    return 4;
}
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_write_a24d32(struct vme_dev* dev, ems_u32 adr, ems_u32 data)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_req req;
    int res;

    req.size=4;
    req.am=AM_a24_data;
    req.addr=adr;
    req.data=data;

    res=ioctl(info->p_vme, SIS3100_VME_WRITE, &req);
    if (res) {
        printf("vme_write_a24d32(%s): %s\n", dev->pathname, strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("vme_write_a24d32(%s, 0x%08x, 0x%08x): error=0x%x\n",
                dev->pathname, adr, data, req.error);
        return -EIO;
    }
    return 4;
}
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_write_aXd16(struct vme_dev* dev, int am, ems_u32 adr, ems_u16 data)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_req req;
    int res;

    req.size=2;
    req.am=am;
    req.addr=adr;
    req.data=data;

    res=ioctl(info->p_vme, SIS3100_VME_WRITE, &req);
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
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_write_a32d16(struct vme_dev* dev, ems_u32 adr, ems_u16 data)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_req req;
    int res;

    req.size=2;
    req.am=AM_a32_data;
    req.addr=adr;
    req.data=data;

    res=ioctl(info->p_vme, SIS3100_VME_WRITE, &req);
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
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_write_a24d16(struct vme_dev* dev, ems_u32 adr, ems_u16 data)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_req req;
    int res;

    req.size=2;
    req.am=AM_a24_data;
    req.addr=adr;
    req.data=data;

    res=ioctl(info->p_vme, SIS3100_VME_WRITE, &req);
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
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_write_aXd8(struct vme_dev* dev, __attribute__((unused)) int am,
        ems_u32 adr, ems_u8 data)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_req req;
    int res;

    req.size=1;
    req.am=AM_a32_data;
    req.addr=adr;
    req.data=data;

    res=ioctl(info->p_vme, SIS3100_VME_WRITE, &req);
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
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_write_a32d8(struct vme_dev* dev, ems_u32 adr, ems_u8 data)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_req req;
    int res;

    req.size=1;
    req.am=AM_a32_data;
    req.addr=adr;
    req.data=data;

    res=ioctl(info->p_vme, SIS3100_VME_WRITE, &req);
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
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_write_a24d8(struct vme_dev* dev, ems_u32 adr, ems_u8 data)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_vme_req req;
    int res;

    req.size=1;
    req.am=AM_a24_data;
    req.addr=adr;
    req.data=data;

    res=ioctl(info->p_vme, SIS3100_VME_WRITE, &req);
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
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_read_pipe(struct vme_dev* dev, int am, int num, int width, ems_u32* addr,
    ems_u32* data)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_pipelist list[32];
    struct sis1100_pipe pipe;
    int i, res;

    if (width!=4)
        return -EINVAL;
    if (num>32) return -ENOMEM;

    for (i=0; i<num; i++) {
        list[i].head=0x0f010000;
        list[i].am=am;
        list[i].addr=addr[i];
    }
    pipe.num=num;
    pipe.list=list;
    pipe.data=data;
    pipe.error=0;

    res=ioctl(info->p_vme, SIS1100_PIPE, &pipe);
    if (res) {
        printf("vme_read_pipe: %s\n", strerror(errno));
        return -errno;
    }

    if (pipe.error) {
        printf("vme_read_pipe: error=0x%x\n", pipe.error);
        return -EIO;
    }
    return pipe.num;
}
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
vme_berrtimer(struct vme_dev* dev, int to)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_ctrl_reg reg;
    int berr_code, berr=0;

    reg.offset=0x100;
    if (ioctl(info->p_vme, SIS1100_CTRL_READ, &reg)<0) {
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
    if (ioctl(info->p_vme, SIS1100_CTRL_WRITE, &reg)<0) {
        printf("ioctl(SIS1100_CTRL_WRITE(0x100)): %s\n", strerror(errno));
        return -1;
    }
    if (reg.error) {
        printf("ioctl(SIS1100_CTRL_WRITE(0x100)): error=0x%x\n", reg.error);
        return -1;
    }
    return berr;
}
#endif
/*****************************************************************************/
static int
vme_arbtimer(__attribute__((unused)) struct vme_dev* dev,
        __attribute__((unused)) int to)
{
    return 0;
}
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
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
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
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
#endif
/*****************************************************************************/
#ifdef DELAYED_READ
static void
vme_reset_delayed_read(struct generic_dev* gdev)
{
    struct vme_dev* dev=(struct vme_dev*)gdev;
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;

printf("sis3100dsp_reset_delayed_read\n");
    info->num_hunks=0;
}
#endif
/*****************************************************************************/
#ifdef DELAYED_READ
static int
vme_read_delayed(struct generic_dev* gdev)
{
    struct vme_dev* dev=(struct vme_dev*)gdev;
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct delayed_hunk* hunks=info->hunk_list;
    int i, res, hunksum;
    ems_u8* delay_buffer;

    if (!info->num_hunks) return 0;
    hunksum=hunks[info->num_hunks-1].src+
         hunks[info->num_hunks-1].size-
         hunks[0].src;

    if (info->delay_buffer_size<hunksum) {
        printf("buffer_size=%lld, hunksum=%d, buffer=%p\n",
                (unsigned long long)info->delay_buffer_size,
                hunksum,
                info->delay_buffer);
        free(info->delay_buffer);
        info->delay_buffer=calloc(hunksum, 1);
        if (!info->delay_buffer) {
            printf("sis3100_read_delayed: malloc(%d):%s\n",
                hunksum, strerror(errno));
            return -1;
        }
        printf("new buffer=%p\n", info->delay_buffer);
        info->delay_buffer_size=hunksum;
    }
/*
printf("read_delayed: pread(%p, %d, %08llx)\n", info->delay_buffer,
        hunksum, info->hunk_list[0].src);
*/
    res=pread(info->p_mem, info->delay_buffer,
            hunksum, info->hunk_list[0].src);
    if (res!=hunksum) {
        printf("sis3100_read_delayed: pread: size=%d res=%d\n",
                hunksum, res);
        return -1;
    }

    delay_buffer=info->delay_buffer-info->hunk_list[0].src;
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
#endif
/*****************************************************************************/
#ifdef DELAYED_READ
int
sis3100_vme_delay_read(struct vme_dev* dev,
        off_t src, void* dst, size_t size)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
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
#endif
/*****************************************************************************/
#ifdef DELAYED_READ
static int
vme_enable_delayed_read(struct generic_dev* gdev, int val)
{
    int old;

    old=gdev->generic.delayed_read_enabled;
    gdev->generic.delayed_read_enabled=val;
    printf("delayed read for VME %sabled.\n", val?"en":"dis");
    return old;
}
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
sis3100_read_controller(struct vme_dev* dev, int domain, ems_u32 addr,
    ems_u32* val)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_ctrl_reg reg;
    int res=0;

    reg.offset=addr;
    switch (domain) {
    case 0: /* sis1100 register */
        res=ioctl(info->p_ctrl, SIS1100_CTRL_READ, &reg);
        break;
    case 1: /* sis3100 register */
        res=ioctl(info->p_vme, SIS1100_CTRL_READ, &reg);
        break;
    default:
        printf("sis3100_read_controller: illegal domain %d\n", domain);
        return -1;
    }
    if (res) {
        printf("sis3100_read_controller(%d, 0x%x): %s\n",
            domain, addr, strerror(errno));
    } else if (reg.error) {
        printf("sis3100_read_controller(%d, 0x%x): 0x%x\n",
            domain, addr, reg.error);
        res=-1;
    } else {
        *val=reg.val;
    }
    return res;
}
#endif
/*****************************************************************************/
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
sis3100_write_controller(struct vme_dev* dev, int domain, ems_u32 addr,
    ems_u32 val)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_ctrl_reg reg;
    int res=0;

    reg.offset=addr;
    reg.val=val;

    switch (domain) {
    case 0: /* sis1100 register */
        res=ioctl(info->p_ctrl, SIS1100_CTRL_WRITE, &reg);
        break;
    case 1: /* sis3100 register */
        res=ioctl(info->p_vme, SIS1100_CTRL_WRITE, &reg);
        break;
    default:
        printf("sis3100_write_controller: illegal domain %d\n", domain);
        return -1;
    }
    if (res) {
        printf("sis3100_write_controller(%d, 0x%x, 0x%x): %s\n",
            domain, addr, val, strerror(errno));
    } else if (reg.error) {
        printf("sis3100_write_controller(%d, 0x%x, 0x%x): 0x%x\n",
            domain, addr, val, reg.error);
        res=-1;
    }
    return res;
}
#endif
/*****************************************************************************/
/*
 * sis3100_front_rw writes and reads the front IO register of the VME
 * controller.
 * Access to the IOs of the PCI card is not (yet) implemented. Because the
 * "Optical Control/Status Register" can only be written as a whole help of
 * the driver is necessary.
 * The procedure writes the (allowed bits of) the register and reads it back.
 * Writing a 0 has no effect, only the read is carried out.
 * If 'latched' is set the latch IO register is used.
 */
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
sis3100_front_io(struct vme_dev* dev, int domain, ems_u32 latched,
    ems_u32* val)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_ctrl_reg reg;
    int res=0;

    switch (domain) {
    case 0: /* sis1100 register */
        /* not implemented (yet) */
        printf("sis3100_front_io: illegal domain %d\n", domain);
        return -1;
    case 1: /* sis3100 register */
        if (latched) {
            reg.offset=0x84;
            switch (info->ident.remote.hw_version) {
            case HV_3100:
                reg.val=*val&0x7f000000;
                break;
            case HV_3104:
                reg.val=*val&0x0f000000;
                break;
            default:
                printf("sis3100_front_io: unknown controller type %d\n",
                        info->ident.remote.hw_version);
                return -1;
            }
        } else {
            /* write of the 'unlatched' register is unrestricted for both
               controller types */
            reg.offset=0x80;
            reg.val=*val;
        }
        if (reg.val) { /* no action if value is 0 */
            res=ioctl(info->p_vme, SIS1100_CTRL_WRITE, &reg);
            if (res || reg.error)
                break;
        }
        res=ioctl(info->p_ctrl, SIS1100_CTRL_READ, &reg);
        break;
    default:
        printf("sis3100_front_read: illegal domain %d\n", domain);
        return -1;
    }

    if (res) {
        printf("sis3100_front_io(%d, %d, 0x%x): %s\n",
            domain, latched, *val, strerror(errno));
    } else if (reg.error) {
        printf("sis3100_front_io(%d, %d, 0x%x): 0x%x\n",
            domain, latched, *val, reg.error);
        res=-1;
    } else {
        *val=reg.val;
    }
    return res;
}
#endif
/*****************************************************************************/
/*
 * sis3100_front_setup writes and reads the front IO level control register
 * of the SIS3104 VME controller.
 * Access to the IOs of the PCI card is not implemented (there is nothing to
 * set up).
 * The procedure writes the (allowed bits of) the register and reads it back.
 * Writing a 0 has no effect, only the read is carried out.
 */
#ifdef SIS3100_MMAPPED
#error mapped access of SIS3100 not yet implemented
#else
static int
sis3100_front_setup(struct vme_dev* dev, int domain, ems_u32 *val)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_ctrl_reg reg;
    int res=0;

    switch (domain) {
    case 0: /* sis1100 register */
        printf("sis3100_front_setup: illegal domain %d\n", domain);
        return -1;
    case 1: /* sis3100 register */
        if (info->ident.remote.hw_version!=HV_3104) {
            printf("sis3100_front_setup: controller is not a SIS3104\n");
            return -1;
        }
        reg.offset=0x88;
        reg.val=*val;
        if (reg.val) { /* no action if value is 0 */
            res=ioctl(info->p_vme, SIS1100_CTRL_WRITE, &reg);
            if (res || reg.error)
                break;
        }
        res=ioctl(info->p_ctrl, SIS1100_CTRL_READ, &reg);
        break;
    default:
        printf("sis3100_front_setup: illegal domain %d\n", domain);
        return -1;
    }
    if (res) {
        printf("sis3100_front_setup(%d, 0x%x): %s\n",
            domain, *val, strerror(errno));
    } else if (reg.error) {
        printf("sis3100_front_setup(%d, 0x%x): 0x%x\n",
            domain, *val, reg.error);
        res=-1;
    }
    return res;
}
#endif
/*****************************************************************************/
static int
sis3100_irq_ack(struct vme_dev* dev, ems_u32 level, ems_u32* vector,
    ems_u32* error)
{
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    struct sis1100_ctrl_reg reg;

    reg.offset=0x80; /* t_hdr */
    reg.val=level&1?0x0c010802:0x03010802;
    if (ioctl(info->p_ctrl, SIS1100_CTRL_WRITE, &reg)<0)
        goto error;

    reg.offset=0x84; /* t_am */
    reg.val=(1<<14)|0x3f;
    if (ioctl(info->p_ctrl, SIS1100_CTRL_WRITE, &reg)<0)
        goto error;

    reg.offset=0x88; /* t_adl */
    reg.val=level<<1;
    if (ioctl(info->p_ctrl, SIS1100_CTRL_WRITE, &reg)<0)
        goto error;

    reg.offset=0xac; /* prot_error */
    if (ioctl(info->p_ctrl, SIS1100_CTRL_READ, &reg)<0)
        goto error;
    *error=reg.val;

    reg.offset=0xa0; /* tc_dal */
    if (ioctl(info->p_ctrl, SIS1100_CTRL_WRITE, &reg)<0)
        goto error;
    *vector=reg.val;
    
    return plOK;

error:
    printf("sis3100_irq_ack:ioctl: %s\n", strerror(errno));
    return plErr_System;
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
#ifdef DELAYED_READ
    sis3100_mem_read_delayed,
#endif
    sis3100_mem_set_bufferstart,
};

static struct dsp_procs*
get_dsp_procs(__attribute__((unused)) struct vme_dev* dev)
{
    return &dsp_procs;
}

static struct mem_procs*
get_mem_procs(__attribute__((unused)) struct vme_dev* dev)
{
    return &mem_procs;
}
/*****************************************************************************/
static errcode
vme_done_sis(struct generic_dev* gdev)
{
    struct vme_dev* dev=(struct vme_dev*)gdev;
    struct vme_sis_info *info=(struct vme_sis_info*)dev->info;
    
    if (info->st) {
        sched_delete_select_task(info->st);
    }

    if (info->p_vme>=0) {
        printf("close(vme_sis/vme)\n");
        close(info->p_vme);
    }
    if (info->p_ctrl>=0) {
        printf("close(vme_sis/ctrl)\n");
        close(info->p_ctrl);
    }
    if (info->p_mem>=0) {
        printf("close(vme_sis/mem)\n");
        close(info->p_mem);
    }
    if (info->p_dsp>=0) {
        printf("close(vme_sis/dsp)\n");
        close(info->p_dsp);
    }

#ifdef DELAYED_READ
    if (info->delay_buffer)
        free(info->delay_buffer);
    info->delay_buffer=0;
    info->delay_buffer_size=0;
#endif

    if (dev->pathname)
        free(dev->pathname);

    free(info);
    dev->info=0;
    return OK;
}
/*****************************************************************************/
static const char *sis1100names[]={"remote", "ram", "ctrl", "dsp"};

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
static int
sis3100_check_ident(struct vme_dev* dev)
{
    struct vme_sis_info* info=(struct vme_sis_info*)dev->info;

    if (ioctl(info->p_ctrl, SIS1100_IDENT, &info->ident)<0) {
        printf("sis3100_check_ident: fcntl(p_ctrl, IDENT): %s\n",
                strerror(errno));
        dev->generic.online=0;
        return -1;
    }
    printf("vme_init_sis3100 local ident:\n");
    printf("  hw_type   =%d\n", info->ident.local.hw_type);
    printf("  hw_version=%d\n", info->ident.local.hw_version);
    printf("  fw_type   =%d\n", info->ident.local.fw_type);
    printf("  fw_version=%d\n", info->ident.local.fw_version);
    if (info->ident.remote_ok) {
        printf("vme_init_sis3100 remote ident:\n");
        printf("  hw_type   =%d\n", info->ident.remote.hw_type);
        printf("  hw_version=%d\n", info->ident.remote.hw_version);
        printf("  fw_type   =%d\n", info->ident.remote.fw_type);
        printf("  fw_version=%d\n", info->ident.remote.fw_version);
        switch (info->ident.remote.hw_version) {
        case 1:
            printf("    controller is SIS3100\n");
            break;
        case 2:
            printf("    controller is SIS3104\n");
            break;
        default:
            printf("    controller type is unknown\n");
            break;
        }
    }

    if (info->ident.remote_online) {
        if (info->ident.remote.hw_type==sis1100_hw_vme) {
            dev->generic.online=1;
        } else {
            printf("vme_init_sis3100: Wrong hardware connected!\n");
            dev->generic.online=0;
        }
    } else {
        printf("vme_init_sis3100: NO remote Interface!\n");
        dev->generic.online=0;
    }
    return 0;
}
/*****************************************************************************/
static int
vme_register_irq(struct vme_dev* dev, ems_u32 mask, ems_u32 vector,
        vmeirq_callback callback, void* data)
{
/*
vme_register_irq installs an interrupt which can be generated by the sis3100
VME-Controller.
It can be a real VME interrupt, a sis3100 front panel IRQ or a s1100 LEMO IRQ.
The bits in mask select the type of IRQ (or the IRQ level) (see sis3100 driver)
for details.
callback (if !=NULL) is called if the IRQ occures.
If signal is !=NULL a signal is requested and the signal number returned
via *signal.
If neither callback nor signal is given the interrupt is installed but no
action is taken if the IRQ occures. The caller of this function may poll for
it (using SIS1100_IRQ_GET).

signal is not implemented yet.
*/

    struct vme_sis_info* info=(struct vme_sis_info*)dev->info;
    struct vme_irq_callback_3100 *new_cb, *cb;
    struct sis1100_irq_ctl ctl;
    ems_u32 cmask;

#if 1
    printf("vme_register_irq mask=0x%x vector=0x%x callback=%p\n",
            mask, vector, callback);
#endif
/*
 *     new mask must not overlap with current mask for non-VME IRQs
 *     new mask may share bits with current mask if the VME-IRQ vectors
 *     are different
 */
    cmask=0;
    cb=info->irq_callbacks;
    while (cb) {
        if (mask&cb->mask&~SIS3100_VME_IRQS) {
            printf("vme_register_irq: masks for non VME-IRQs overlap\n");
            return -1;
        }
        if ((mask&cb->mask&SIS3100_VME_IRQS) && (vector==cb->vector)) {
            printf("vme_register_irq: identical level and vector\n");
            return -1;
        }
        cmask|=cb->mask;
        cb=cb->next;
    }
    /*printf("current mask=0x%x\n", cmask);*/

    /* if signal!=NULL register signal */

    /* register irq with driver */
    ctl.irq_mask=mask&~cmask;
    if (ctl.irq_mask) {
        ctl.signal=-1; /* install IRQ but do not send a signal */
        if (ioctl(info->p_ctrl, SIS1100_IRQ_CTL, &ctl)) {
            printf("vme_register_irq: SIS1100_IRQ_CTL: %s\n", strerror(errno));
            return -1;
        }
    }

    /* find old matching entry (callback and vector and data) */
    cb=info->irq_callbacks;
    while (cb) {
        if ((cb->callback==callback) && (cb->vector==vector) && (cb->data==data)) {
            break;
        }
        cb=cb->next;
    }
    if (cb) { /* usable old entry found */
        /*printf("old entry found, old mask=0x%x\n", cb->mask);*/
        cb->mask|=mask;
        return 0;
    }

    /* create new entry */
    new_cb=malloc(sizeof(struct vme_irq_callback_3100));
    new_cb->callback=callback;
    new_cb->mask=mask;
    new_cb->vector=vector;
    new_cb->data=data;

    if (info->irq_callbacks)
        info->irq_callbacks->prev=new_cb;
    new_cb->prev=0;
    new_cb->next=info->irq_callbacks;
    info->irq_callbacks=new_cb;

    return 0;
}
/*****************************************************************************/
static int
vme_unregister_irq(struct vme_dev* dev, ems_u32 mask, ems_u32 vector)
{
    struct vme_sis_info* info=(struct vme_sis_info*)dev->info;
    struct vme_irq_callback_3100 *cb, *next_cb;
    struct sis1100_irq_ctl ctl;
    ems_u32 oldmask, newmask;

#if 1
    printf("vme_unregister_irq mask=0x%x vector=0x%x\n", mask, vector);
#endif
    /* calculate old mask */
    oldmask=0;
    cb=info->irq_callbacks;
    while (cb) {
        oldmask|=cb->mask;
        cb=cb->next;
    }

    /* find matching entry (vector and callback) */
    cb=info->irq_callbacks;
    while (cb) {
        next_cb=cb->next;
        if (cb->vector==vector) {
            /*printf("found entry %p old mask=0x%x", cb, cb->mask);*/
            cb->mask&=~mask;
            /*printf(" new mask==0x%x\n", cb->mask);*/
            if (!cb->mask) {
                /*printf("deleting %p\n", cb);*/
                if (cb->prev)
                    cb->prev->next=cb->next;
                if (cb->next)
                    cb->next->prev=cb->prev;
                if (cb==info->irq_callbacks)
                    info->irq_callbacks=cb->next;
                free(cb);
            }
            break;
        }
        cb=next_cb;
    }

    /* calculate new mask */
    newmask=0;
    cb=info->irq_callbacks;
    while (cb) {
        newmask|=cb->mask;
        cb=cb->next;
    }

    /* deregister IRQs */
    ctl.irq_mask=oldmask&~newmask;
    if (ctl.irq_mask) {
        ctl.signal=0;
        if (ioctl(info->p_ctrl, SIS1100_IRQ_CTL, &ctl)) {
            printf("vme_unregister_irq: SIS1100_IRQ_CTL: %s\n", strerror(errno));
            return -1;
        }
    }

    return 0;
}
/*****************************************************************************/
static int
vme_acknowledge_irq(struct vme_dev* dev, ems_u32 mask,
        __attribute__((unused)) ems_u32 vector)
{
    struct vme_sis_info* info=(struct vme_sis_info*)dev->info;
    struct sis1100_irq_ack ack;

#if 0
    printf("vme_acknowledge_irq(%s) mask=0x%x vector 0x%x\n",
                dev->pathname, mask, vector);
#endif
    ack.irq_mask=mask;
    if (ioctl(info->p_ctrl, SIS1100_IRQ_ACK, &ack)) {
        printf("vme_acknowledge_irq(%s): %s\n", dev->pathname, strerror(errno));
        return -1;
    }

    return 0;
}
/*****************************************************************************/
static void
sis3100_link_up_down(struct vme_dev* dev, struct sis1100_irq_get2* get)
{
    struct vme_sis_info* info=(struct vme_sis_info*)dev->info;
    int crate=dev->generic.dev_idx;
    int initialised=-1;
    int up;

    up=get->remote_status>0;
    printf("sis1100: VME crate %d is %s\n", dev->generic.dev_idx, up?"up":"down");
    initialised=0;
    if (up) {
        sis3100_check_ident(dev);
        if (dev->generic.online) {
            int on, res;
            res=sis3100_get_user_led(info, &on);
            if (!res && on)
                initialised=1;
            printf("sis3100: it is %s initialised\n",
                initialised?"still":"not any more");
        }
    } else {
        dev->generic.online=0;
    }
    send_unsol_var(Unsol_DeviceDisconnect, 4, modul_vme,
        crate, get->remote_status, initialised);
}
/*****************************************************************************/
static void
sis3100_vmeirq(struct vme_dev* dev, struct sis1100_irq_get2* get)
{
    struct vme_sis_info* info=(struct vme_sis_info*)dev->info;
    ems_u32 irqs;

#if IRQDEBUG
    static int irqnum=0;
    irqnum++;
    if (irqnum>40) {
        printf("NOTAUS\n");
        exit(0);
    }
#endif

#if IRQDEBUG
    printf("sis3100_vmeirq(%s): got irqs 0x%08x level=%d vector 0x%x\n",
                dev->pathname, get->irqs, get->level, get->vector);
#endif
    /* loop over non-VME interrupts */
    irqs=get->irqs&SIS3100_EXT_IRQS;
#if IRQDEBUG
    printf("non-VME irqs: 0x%08x\n", irqs);
#endif

    if (irqs) {
        struct vme_irq_callback_3100* cb=info->irq_callbacks;
        int irq_handled=0;
        while (irqs && cb) {
            if (cb->mask&irqs) {
                struct vmeirq_callbackdata vme_data;
#if IRQDEBUG
                printf("found callback %p, mask 0x%08x\n", cb->callback, cb->mask);
#endif
                vme_data.mask=cb->mask&irqs;
                vme_data.level=0;
                vme_data.vector=0;
                vme_data.time.tv_sec=get->irq_sec;
                vme_data.time.tv_nsec=get->irq_nsec;
                cb->callback(dev, &vme_data, cb->data);
                irqs&=~cb->mask;
                irq_handled=1;
            }
            cb=cb->next;
        }
        /* looks strange but is correct:
           It is enough to handle at least one bit in irqs.
           The remaining bits are handled (or disabled) after the next select.
        */
        if (!irq_handled) {
            struct sis1100_irq_ctl ctl;
            printf("sis3100_vmeirq: IRQs 0x%x not handled; disabled\n", irqs);
            ctl.irq_mask=irqs;
            ctl.signal=0;
            if (ioctl(info->p_ctrl, SIS1100_IRQ_CTL, &ctl))
                printf("SIS1100_IRQ_CTL: %s\n", strerror(errno));
        }
    }

    /* deliver the highest level VME interrupt */
    irqs=get->irqs&SIS3100_VME_IRQS;
#if IRQDEBUG
    printf("    VME irqs: 0x%08x\n", irqs);
#endif
    if (irqs) {
        ems_u32 irqbit=1<<get->level;
        int irq_handled=0;
        int inum=0;
        if (!(irqbit&irqs)) {
            static int maxcount=20;
            complain("VME: IRQ logic confused: no LEVEL %d in bits 0x%x",
                    get->level, get->irqs&SIS3100_VME_IRQS);
            /* try to acknowledge dangling IRQs */
            vme_acknowledge_irq(dev, irqs, 0);
            if (--maxcount<=0) {
                complain("exiting because of confused IRQ logic");
                exit(0);
            }
        } else {
            struct vme_irq_callback_3100* cb=info->irq_callbacks;
#if 0
            printf("sis3100_vmeirq: irqs=0x%x, vector=0x%x eventcnt=%d\n",
                irqs, get->vector, eventcnt);
#endif
            while (irqs && cb && !inum) {
                if (cb->mask&irqs && (int)cb->vector==get->vector) {
#if IRQDEBUG
                    printf("found callback %p, mask 0x%08x\n", cb->callback, cb->mask);
#endif
                    inum++;
                    if (cb->callback) {
                        struct vmeirq_callbackdata vme_data;
                        vme_data.mask=cb->mask&irqs;
                        vme_data.level=get->level;
                        vme_data.vector=get->vector;
                        vme_data.time.tv_sec=get->irq_sec;
                        vme_data.time.tv_nsec=get->irq_nsec;
                        cb->callback(dev, &vme_data, cb->data);
                        irqs&=~irqbit;
                    }
                    irq_handled=1;
                }
                cb=cb->next;
            }
            /* Wir muessen hier darauf vertrauen, dass cb->callback()
               die Interruptquelle deaktiviert. Passiert das nicht, wird
               sis3100_irq vom Scheduler immer wieder aufgerufen und wir
               enden in einer Endlosschleife (das Murmeltier gruesst).
               Wenn der Interrupt von einer Triggerprozedur installiert
               wurde muss die Quelle normalerweise von einer aufgerufenen
               Readout-Prozedur deaktiviert werden.
            */
        }
        /* will we get the other VME IRQs with the next select? */
#if 1
        if (!irq_handled) {
            struct sis1100_irq_ctl ctl;
            printf("sis3100_vmeirq: IRQs 0x%x not handled; disabled\n", irqs);
            ctl.irq_mask=irqs;
            ctl.signal=0;
            if (ioctl(info->p_ctrl, SIS1100_IRQ_CTL, &ctl))
                printf("SIS1100_IRQ_CTL: %s\n", strerror(errno));
        }
#endif
    }
}
/*****************************************************************************/
static void
sis3100_irq(int p, __attribute__((unused)) enum select_types selected,
        union callbackdata data)
{
    struct generic_dev* dev=(struct generic_dev*)data.p;
    struct vme_dev* vmedev=(struct vme_dev*)dev;
    struct sis1100_irq_get2 get;
    int res;

    res=read(p, &get, sizeof(get));
    if (res!=sizeof(get)) {
        struct sis1100_irq_ctl ctl;

        printf("unixvme/sis3100/sis1100_irq: res=%d errno=%s\n",
                res, strerror(errno));

        printf("sizeof(struct sis1100_irq_get2)=%llu\n",
                (unsigned long long)sizeof(struct sis1100_irq_get2));

        ctl.irq_mask=0xffffffff;
        ctl.signal=0;
        if (ioctl(p, SIS1100_IRQ_CTL, &ctl)) {
            printf("sis1100_irq: SIS1100_IRQ_CTL: %s\n", strerror(errno));
        }
        return;
    }
#if IRQDEBUG
    printf("sis3100_irq: irqs=0x%x, level=%u vector=0x%x sec=%u nsec=%u\n",
        get.irqs, get.level, get.vector, get.irq_sec, get.irq_nsec);
#endif
    if (get.remote_status) {
        sis3100_link_up_down(vmedev, &get);
    }

    if (get.irqs) {
        sis3100_vmeirq(vmedev, &get);
    }
}
/*****************************************************************************/
errcode
vme_init_sis3100(struct vme_dev* dev)
{
    struct vme_sis_info* info;
    struct sis1100_irq_ctl ctl;
    errcode res=OK;
    printf("vme_init_sis3100: path=%s\n", dev->pathname);

    info=(struct vme_sis_info*)malloc(sizeof(struct vme_sis_info));
    if (!info) {
        printf("vme_init_sis3100: calloc(info): %s\n", strerror(errno));
        return Err_NoMem;
    }
    dev->info=info;

    info->p_ctrl=-1;
    info->p_vme=-1;
    info->p_mem=-1;
    info->p_dsp=-1;
    info->irq_callbacks=0;

    if ((info->p_ctrl=
            open_dev(dev->pathname, sis1100names[sis1100_subdev_ctrl]))<0) {
        res=Err_System;
        goto error;
    }

    if ((info->p_vme=
            open_dev(dev->pathname, sis1100names[sis1100_subdev_remote]))<0) {
        res=Err_System;
        goto error;
    }

    if ((info->p_mem=
            open_dev(dev->pathname, sis1100names[sis1100_subdev_ram]))<0) {
        res=Err_System;
        goto error;
    }

    if ((info->p_dsp=
            open_dev(dev->pathname, sis1100names[sis1100_subdev_dsp]))<0) {
        res=Err_System;
        goto error;
    }

    info->st=0;

    if (fcntl(info->p_ctrl, F_SETFL, O_NDELAY)<0) {
        printf("vme_init_sis3100: fcntl(p_ctrl, F_SETFL): %s\n", strerror(errno));
    }

    ctl.irq_mask=0;
    //ctl.irq_mask=0xffffffff;
    ctl.signal=-1;
    if (ioctl(info->p_ctrl, SIS1100_IRQ_CTL, &ctl)) {
        printf("vme_init_sis3100: SIS1100_IRQ_CTL: %s\n", strerror(errno));
        res=Err_System;
        goto error;
    } else {
        union callbackdata cbdata;
        cbdata.p=dev;
        info->st=sched_insert_select_task(sis3100_irq, cbdata,
            "sis3100_irq", info->p_ctrl, select_read
    #ifdef SELECT_STATIST
            , 1
    #endif
            );
        if (!info->st) {
            printf("vme_init_sis3100: cannot install callback for sis3100_irq\n");
            res=Err_System;
            goto error;
        }
    }

    dev->generic.done=vme_done_sis;
    dev->generic.init_jtag_dev=unixvme_init_jtag_dev;

#ifdef DELAYED_READ
    dev->generic.reset_delayed_read=vme_reset_delayed_read;
    dev->generic.read_delayed=vme_read_delayed;
    dev->generic.enable_delayed_read=vme_enable_delayed_read;
#endif

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
    dev->write_a32_fifo=vme_write_a32_fifo;

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

    dev->read_pipe=vme_read_pipe;

    dev->register_vmeirq=vme_register_irq;
    dev->unregister_vmeirq=vme_unregister_irq;
    dev->acknowledge_vmeirq=vme_acknowledge_irq;

    dev->read_controller=sis3100_read_controller;
    dev->write_controller=sis3100_write_controller;
    dev->irq_ack=sis3100_irq_ack;
    dev->mindmalen=vme_mindmalen;
    dev->minpipelen=vme_minpipelen;
    dev->front_io=sis3100_front_io;
    dev->front_setup=sis3100_front_setup;

    info->current_datasize=-1;

#ifdef DELAYED_READ
    info->hunk_list=0;
    info->hunk_list_size=0;
    info->delay_buffer=0;
    info->delay_buffer_size=0;
    dev->generic.delayed_read_enabled=1;
    vme_reset_delayed_read((struct generic_dev*)dev);
#endif

    sis3100_check_ident(dev);

    if (dev->generic.online) {
        int timeouts[2]={12400, 9};
        if (ioctl(info->p_vme, SIS3100_TIMEOUTS, timeouts)<0) {
            printf("vme_init_sis3100: ioctl(TIMEOUTS): %s\n", strerror(errno));
        }
        printf("old timeouts: berr: %d ns, arb: %d ms\n",
                timeouts[0], timeouts[1]);
        timeouts[0]=-1;
        timeouts[1]=-1;
        if (ioctl(info->p_vme, SIS3100_TIMEOUTS, timeouts)<0) {
            printf("vme_init_sis3100: ioctl(TIMEOUTS): %s\n", strerror(errno));
        }
        printf("new timeouts: berr: %d ns, arb: %d ms\n",
                timeouts[0], timeouts[1]);

        sis3100_set_user_led(info, 1);
    }

    return OK;

error:
    if (info->p_ctrl>-1) close(info->p_ctrl);
    if (info->p_vme>-1) close(info->p_vme);
    if (info->p_mem>-1) close(info->p_mem);
    if (info->p_dsp>-1) close(info->p_mem);
    free(info);
    dev->info=0;
    return res;
}
/*****************************************************************************/
/*****************************************************************************/
