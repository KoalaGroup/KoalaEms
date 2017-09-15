/*
 * lowlevel/unixvme/drv/vme_drv.c
 * created 23.Jan.2001 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: vme_drv.c,v 1.7 2011/04/06 20:30:28 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <rcs_ids.h>

#include <dev/vme/vmevar.h>
#include <dev/vme/vmegeneric_var.h>

#include "../vme.h"
#include "unixvme_drv.h"

#ifdef DMALLOC
#include dmalloc.h
#endif

#ifdef VMESWAP
#define VME2Hl(x) ntohl(x)
#define H2VMEl(x) ntohl(x)
#define VME2Hs(x) ntohs(x)
#define H2VMEs(x) ntohs(x)
#else
#define VME2Hl(x) (x)
#define H2VMEl(x) (x)
#define VME2Hs(x) (x)
#define H2VMEs(x) (x)
#endif

RCS_REGISTER(cvsid, "lowlevel/unixvme/drv")

/*****************************************************************************/
static int set_datasize(struct vme_dev* dev, int size)
{
struct vmespace s;
int c;

if (size != dev->info.drv.current_datasize)
  {
  switch (size)
    {
    case 1: /* nobreak */
    case 2: /* nobreak */
    case 4:
      s.datasize = size;
      break;
    default:
      printf("invalid datasize %d\n", size);
      return -1;
    }
  s.am = -1; /* no change */
  s.swap = 0;
  s.mindmalen = 0;
  s.mapit = 1;
  /*
  printf("ioctl(vme, SETVMESPACE, am=%d, swap=%d, mindmalen=%d, mapit=%d"
      ", datasize=%d)\n",
      s.am, s.swap, s.mindmalen, s.mapit, s.datasize);
  */
  c = ioctl(dev->info.drv.handle, SETVMESPACE, &s);
  if (c < 0)
    {
    printf("set VME params: %s\n", strerror(errno));
    return -1;
    }
  else
    dev->info.drv.current_datasize=size;
  }
return 0;
}
/*****************************************************************************/
static int
vme_read(struct vme_dev* dev, ems_u32 adr, ems_u32* data,
    int size, int datasize, ems_u32** newdata)
{
    int res;

    if ((res=set_datasize(dev, datasize))) {
        if (newdata) *newdata=data;
        return -res;
    }

    if (lseek(dev->info.drv.handle, adr, SEEK_SET)!=adr) {
        printf("lowlevel/unixvme/drv/vme_drv.c: lseek(0x%08x): %s\n",
            adr, strerror(errno));
        if (newdata) *newdata=data;
        return -errno;
    }

#ifdef VMESWAP
    switch (datasize) {
    case 4: {
        int* dp;
        int isize;
        isize=(size+3)/4;
        res = read(dev->info.drv.handle, data, size);
        for (dp=data; isize; isize--, dp++) *dp=VME2Hl(*dp);
        }
        break;
    case 2: {
        short* dp;
        int isize;
        isize=(size+1)/2;
        res = read(dev->info.drv.handle, data, size);
        for (dp=(short*)data; isize; isize--, dp++) *dp=VME2Hs(*dp);
        }
        break;
    default /* 1 */: res = read(dev->info.drv.handle, data, size);
    }
#else
    res = read(dev->info.drv.handle, data, size);
#endif
    if (res>=0) {
        if (newdata) *newdata=((int*)(((unsigned long)data + res + 3) & ~3));
        return res;
    } else {
        printf("lowlevel/unixvme/drv/vme_drv.c: vme_read from 0x%08x: %s\n",
            adr, strerror(errno));
        if (newdata) *newdata=data;
        return -errno;
    }
}
/*****************************************************************************/
static int
vme_read_aX(struct vme_dev* dev, int am, ems_u32 adr, ems_u32* data,
    int size, int datasize, ems_u32** newdata)
{
    return vme_read(dev, adr, data, size, datasize, newdata);
}
/*****************************************************************************/
static int
vme_read_d32(struct vme_dev* dev, ems_u32 adr, ems_u32* data)
{
    int res;

    if ((res=set_datasize(dev, 4))) return -res;

    lseek(dev->info.drv.handle, adr, SEEK_SET);

    res=read(dev->info.drv.handle, data, 4);
    if (res!=4) {
        printf("VME read_d32: %s\n", strerror(errno));
        return -errno;
    }

#ifdef VMESWAP
    *data=VME2Hl(*data);
#endif
    return res;
}
/*****************************************************************************/
static int
vme_read_aXd32(struct vme_dev* dev, int am, ems_u32 adr, ems_u32* data)
{
    return vme_read_d32(dev, adr, data);
}
/*****************************************************************************/
static int
vme_read_a32_fifo(struct vme_dev* dev, ems_u32 adr, ems_u32* data,
    int size, int datasize, ems_u32** newdata)
{
    int res, i;

    if ((res=set_datasize(dev, datasize))) {
        if (newdata) *newdata=data;
        return -res;
    }

    for (i=0; i<size/datasize; i++) {
        lseek(dev->info.drv.handle, adr, SEEK_SET);

        res=read(dev->info.drv.handle, data, datasize);
        if (res!=datasize) {
            printf("VME vme_read_a32_fifo: %s\n", strerror(errno));
            if (newdata) *newdata=data;
            return -errno;
        }
#ifdef VMESWAP
        switch (datasize) {
        case 4: *data=VME2Hl(*data); break;
        case 2: *data=VME2Hs(*data); break;
        }
#endif
        data++;
    }
    if (newdata) *newdata=data;
    return size;
}
/*****************************************************************************/
static int
vme_read_d16(struct vme_dev* dev, ems_u32 adr, ems_u16* data)
{
    int res;

    if ((res=set_datasize(dev, 2))) return -res;

    lseek(dev->info.drv.handle, adr, SEEK_SET);

    res=read(dev->info.drv.handle, data, 2);
    if (res!=2) {
        printf("VME read_d16: %s\n", strerror(errno));
        return -errno;
    }

#ifdef VMESWAP
    *data=VME2Hs(*data);
#endif
    return res;
}
/*****************************************************************************/
static int
vme_read_aXd16(struct vme_dev* dev, int am, ems_u32 adr, ems_u16* data)
{
    return vme_read_d16(dev, adr, data);
}
/*****************************************************************************/
static int
vme_read_d8(struct vme_dev* dev, ems_u32 adr, ems_u8* data)
{
    int res;

    if ((res=set_datasize(dev, 1))) return -res;

    lseek(dev->info.drv.handle, adr, SEEK_SET);

    res=read(dev->info.drv.handle, data, 1);
    if (res!=1) {
        printf("VME read_d8: %s\n", strerror(errno));
        return -errno;
    }
    return res;
}
/*****************************************************************************/
static int
vme_read_aXd8(struct vme_dev* dev, int am, ems_u32 adr, ems_u8* data)
{
    return vme_read_d8(dev, adr, data);
}
/*****************************************************************************/
static int
vme_write(struct vme_dev* dev, ems_u32 adr, ems_u32* data,
    int size, int datasize)
{
    int res;
#ifdef VMESWAP
    int isize;
    static void* tmp=0;
    static int tmplen=0;
#endif

    if ((res=set_datasize(dev, datasize))) return -res;

    if (lseek(dev->info.drv.handle, adr, SEEK_SET)!=adr) {
        printf("lowlevel/unixvme/drv/vme_drv.c: lseek(0x%08x): %s\n",
            adr, strerror(errno));
        return -errno;
    }

#ifdef VMESWAP
    switch (datasize) {
    case 4: {
        int *src, *dst;
        if (tmplen<size) {
            free(tmp);
            tmp=malloc(size);
            tmplen=size;
        }
        isize=(size+3)/4;
        for (dst=(int*)tmp, src=data; isize; isize--) *dst++=H2VMEl(*src++);
        res = write(dev->info.drv.handle, (int*)tmp, size);
        }
        break;
    case 2: {
        short *src, *dst;
        if (tmplen<size) {
            free(tmp);
            tmp=malloc(size);
            tmplen=size;
        }
        isize=(size+1)/2;
        for (dst=(short*)tmp, src=(short*)data; isize; isize--)
                *dst++=H2VMEs(*src++);
        res = write(dev->info.drv.handle, (int*)tmp, size);
        }
        break;
    default /* 1 */: res = write(dev->info.drv.handle, data, size);
    }
#else
    res = write(dev->info.drv.handle, data, size);
#endif
    if (res>=0) {
        return res;
    } else {
        printf("lowlevel/unixvme/drv/vme_drv.c: vme_write to 0x%08x: %s\n",
            adr, strerror(errno));
        return -errno;
    }
}
/*****************************************************************************/
static int
vme_write_d32(struct vme_dev* dev, ems_u32 adr, ems_u32 data)
{
    int res;

    if ((res=set_datasize(dev, 4))) return -res;

    lseek(dev->info.drv.handle, adr, SEEK_SET);

#ifdef VMESWAP
    data=H2VMEl(data);
#endif

    res=write(dev->info.drv.handle, &data, 4);
    if (res!=4) {
        printf("VME write_d32: %s\n", strerror(errno));
        return -errno;
    } else {
        return res;
    }
}
/*****************************************************************************/
static int
vme_write_aXd32(struct vme_dev* dev, int am, ems_u32 adr, ems_u32 data)
{
    return vme_write_d32(dev, adr, data);
}
/*****************************************************************************/
static int
vme_write_d16(struct vme_dev* dev, ems_u32 adr, ems_u16 data)
{
    int res;

    if ((res=set_datasize(dev, 2))) return -res;

    lseek(dev->info.drv.handle, adr, SEEK_SET);

#ifdef VMESWAP
    data=H2VMEs(data);
#endif
    res=write(dev->info.drv.handle, &data, 2);
    if (res!=2) {
        printf("VME write_d16: %s\n", strerror(errno));
        return -errno;
    } else {
        return res;
    }
}
/*****************************************************************************/
static int
vme_write_aXd16(struct vme_dev* dev, int am, ems_u32 adr, ems_u16 data)
{
    return vme_write_d16(dev, adr, data);
}
/*****************************************************************************/
static int
vme_write_d8(struct vme_dev* dev, ems_u32 adr, ems_u8 data)
{
    int res;

    if ((res=set_datasize(dev, 1))) return -res;

    lseek(dev->info.drv.handle, adr, SEEK_SET);

    res=write(dev->info.drv.handle, &data, 1);
    if (res!=1) {
        printf("VME write_d8: %s\n", strerror(errno));
        return -errno;
    } else {
        return res;
    }
}
/*****************************************************************************/
static int
vme_write_aXd8(struct vme_dev* dev, int am, ems_u32 adr, ems_u8 data)
{
    return vme_write_d8(dev, adr, data);
}
/*****************************************************************************/
/*
static int vme_probe(struct vme_dev* dev, unsigned int adr, int datasize)
{
if (set_datasize(dev, datasize)<0) return -1;
printf("ioctl(vme, VME_PROBE, adr=0x%x)\n", adr);
if (ioctl(dev->info.drv.handle, VME_PROBE, &adr))
  return errno?errno:-1;
else
  return 0;
}
*/
/*****************************************************************************/
static int
vme_berrtimer(struct vme_dev* dev, int to){return 0;}
static int
vme_arbtimer(struct vme_dev* dev, int to){return 0;}
static void
vme_reset_delayed_read(struct generic_dev* dev) {}
static int
vme_read_delayed(struct generic_dev* dev) {return 0;}
static int
vme_enable_delayed_read(struct generic_dev* dev, int val) {return -1;}
/*****************************************************************************/
static struct dsp_procs* get_dsp_procs(struct vme_dev* dev) {return 0;}
static struct mem_procs* get_mem_procs(struct vme_dev* dev) {return 0;}
/*****************************************************************************/
static errcode vme_done_drv(struct generic_dev* gdev)
{
struct vme_dev* dev=(struct vme_dev*)gdev;
if (dev->info.drv.handle>=0)
  {
  printf("close(vme_drv)\n");
  close(dev->info.drv.handle);
  }
/*dev->valid=0;*/
return OK;
}
/*****************************************************************************/
errcode vme_init_drv(char* pathname, struct vme_dev* dev)
{
    printf("vme_init_drv: open(path=%s)\n", pathname);
    dev->info.drv.handle=open(pathname, O_RDWR, 0);
    if(dev->info.drv.handle<0) {
        printf("open VME (drv) %s: %s\n", pathname, strerror(errno));
        return Err_System;
    }
    if (fcntl(dev->info.drv.handle, F_SETFD, FD_CLOEXEC)<0) {
        printf("fcntl(vme %s, FD_CLOEXEC): %s\n", pathname, strerror(errno));
    }
    dev->info.drv.current_datasize=-1;

    dev->generic.done=vme_done_drv;
    dev->generic.online=1;

    dev->generic.reset_delayed_read=vme_reset_delayed_read;
    dev->generic.read_delayed=vme_read_delayed;
    dev->generic.enable_delayed_read=vme_enable_delayed_read;

    dev->get_dsp_procs=get_dsp_procs;
    dev->get_mem_procs=get_mem_procs;

    dev->berrtimer=vme_berrtimer;
    dev->arbtimer=vme_arbtimer;

    dev->read_a32=vme_read;
    dev->write_a32=vme_write;
    dev->read_a24=vme_read;
    dev->write_a24=vme_write;
    dev->read_aX=vme_read_aX;
    dev->read_a32_fifo=vme_read_a32_fifo;

    dev->read_a32d32=vme_read_d32;
    dev->write_a32d32=vme_write_d32;
    dev->read_a32d16=vme_read_d16;
    dev->write_a32d16=vme_write_d16;
    dev->read_a32d8=vme_read_d8;
    dev->write_a32d8=vme_write_d8;

    dev->read_a24d32=vme_read_d32;
    dev->write_a24d32=vme_write_d32;
    dev->read_a24d16=vme_read_d16;
    dev->write_a24d16=vme_write_d16;
    dev->read_a24d8=vme_read_d8;
    dev->write_a24d8=vme_write_d8;

    dev->read_aXd32=vme_read_aXd32;
    dev->write_aXd32=vme_write_aXd32;
    dev->read_aXd16=vme_read_aXd16;
    dev->write_aXd16=vme_write_aXd16;
    dev->read_aXd8=vme_read_aXd8;
    dev->write_aXd8=vme_write_aXd8;

    /*dev->probe=vme_probe;*/
    /*dev->valid=1;*/
    return OK;
}
/*****************************************************************************/
errcode unixvme_drv_low_init(char* mist) {return OK;}
errcode unixvme_drv_low_done(void) {return OK;}
errcode unixvme_drv_low_printuse(FILE* mist) {return OK;}
/*****************************************************************************/
/*****************************************************************************/
