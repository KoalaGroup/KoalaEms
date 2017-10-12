/*
 * sis1100_F1_tools.c
 * $ZEL$
 * created 12.Aug.2003 PW
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include "sis1100_F1_tools.h"

static void
straw_close_path(struct straw_path* path)
{
    if (path->map) {
        munmap((void*)path->map, path->mapsize);
    }
    close (path->p);
}

void
straw_close(struct straw_pathes* pathes)
{
    straw_close_path(&pathes->straw);
    straw_close_path(&pathes->ctrl);
}

static int
straw_open_path(struct straw_path* path,
    const char* path_stub, enum sis1100_subdev subdev)
{
    char name[1024];
    const char* suffix;
    enum sis1100_subdev mysubdev;
    
    switch (subdev) {
    case sis1100_subdev_remote: suffix="remote"; break;
    case sis1100_subdev_ram: suffix="ram"; break;
    case sis1100_subdev_ctrl: suffix="ctrl"; break;
    case sis1100_subdev_dsp: suffix="dsp"; break;
    default: suffix=0;
    };

    snprintf(name, 1024, "%s%s", path_stub, suffix);

    path->p=open(name, O_RDWR, 0);
    if (path->p<0) {
        printf("open \"%s\": %s\n", name, strerror(errno));
        return -1;
    }

    if (ioctl(path->p, SIS1100_DEVTYPE, &mysubdev)<0) {
        printf("ioctl(%s, SIS1100_DEVTYPE): %s\n", name, strerror(errno));
        return -1;
    }
    if (mysubdev!=subdev) {
        printf("%s is not the expected type of subdevice (%d instead of %d)\n",
            name, mysubdev, subdev);
        close(path->p);
        return -1;
    }

    if (ioctl(path->p, SIS1100_MAPSIZE, &path->mapsize)<0) {
        printf("ioctl(%s, SIS1100_MAPSIZE): %s\n", name, strerror(errno));
        close(path->p);
        return -1;
    }

    if (path->mapsize) {
        printf("%s mapsize=%d\n", name, path->mapsize);
        path->map=mmap(0, path->mapsize, PROT_READ|PROT_WRITE,
                /*MAP_FILE|*/MAP_SHARED/*|MAP_VARIABLE*/, path->p, 0);
        if (path->map==MAP_FAILED) {
            printf("mmap(%s, 0x%x): %s\n", name, path->mapsize,
                strerror(errno));
            path->map=0;
            path->mapsize=0;
        }
    }
    return 0;
}

int
straw_open(struct straw_pathes* pathes, const char* path_stub)
{
    int version;

    if (straw_open_path(&pathes->ctrl, path_stub, sis1100_subdev_ctrl)<0)
        return -1;
    if (straw_open_path(&pathes->straw, path_stub, sis1100_subdev_remote)<0) {
        straw_close_path(&pathes->ctrl);
        return -1;
    }
    if (pathes->ctrl.map) {
        fprintf(stderr, "ctrl-device: %d bytes mapped\n", pathes->ctrl.mapsize);
    }
    if (pathes->straw.map) {
        fprintf(stderr, "straw-device: %d bytes mapped\n", pathes->straw.mapsize);
    }

    if (ioctl(pathes->ctrl.p, SIS1100_DRIVERVERSION, &version)<0) {
        fprintf(stderr, "ioctl(%s, SIS1100_DRIVERVERSION): %s\n",
                path_stub, strerror(errno));
        version=0;
    }
    pathes->majorversion=(version>>16)&0xffff;
    pathes->minorversion=version&0xffff;
    printf("%s: driverversion is %d.%d\n", path_stub,
            pathes->majorversion, pathes->minorversion);
    if (pathes->majorversion!=SIS1100_MajVersion) {
        printf("%s: Version mismatch:\n", path_stub);
        printf("required major version is %d\n", SIS1100_MajVersion);
        straw_close_path(&pathes->straw);
        straw_close_path(&pathes->ctrl);
        return -1;
    }
    if (pathes->minorversion!=SIS1100_MinVersion) {
        printf("%s: Version mismatch:\n", path_stub);
        printf("minor driver version (%d) is less than expected (%d)\n",
            pathes->minorversion, SIS1100_MinVersion);
    }
    pathes->units=-1;
    return 0;
}

int
_w_straw(struct straw_pathes* pp, u_int32_t addr, int size, u_int16_t data)
{
    struct sis1100_vme_req req;
    int res;

    req.size=size;
    req.am=-1;
    req.addr=addr;
    req.data=data;
    res=ioctl(pp->straw.p, SIS3100_VME_WRITE, &req);
    if (pp->debug) debug_w_straw(&req);
    if (res) {
        fprintf(stderr, "w%d_straw(addr=0x%08x): %s\n", size<<3, addr,
            strerror(errno));
        return -1;
    }
    if (req.error) {
        fprintf(stderr, "w%d_straw(addr=0x%08x): error=0x%x\n", size<<3, addr,
            req.error);
        return -1;
    }
    return 0;
}

int
_r_straw(struct straw_pathes* pp, u_int32_t addr, int size, u_int32_t* data)
{
    struct sis1100_vme_req req;
    int res;
    
    req.size=size;
    req.am=-1;
    req.addr=addr;
    req.data=0;
    res=ioctl(pp->straw.p, SIS3100_VME_READ, &req);
    if (pp->debug) debug_r_straw(&req);
    if (res) {
        fprintf(stderr, "r%d_straw(addr=0x%08x): %s\n", size<<3, addr,
            strerror(errno));
        return -1;
    }
    if (req.error) {
        fprintf(stderr, "r%d_straw(addr=0x%08x): error=0x%x\n", size<<3, addr,
            req.error);
        return -1;
    }
    *data=req.data;
    return 0;
}

int
w32_ctrl(struct straw_pathes* pp, u_int32_t addr, u_int32_t data)
{
    struct sis1100_ctrl_reg req;
    int res;
    
    req.offset=addr;
    req.val=data;
    res=ioctl(pp->ctrl.p, SIS1100_LOCAL_CTRL_WRITE, &req);
    /*debug_w32_ctrl(&req);*/
    if (res) {
        fprintf(stderr, "w32_ctrl(addr=0x%08x): %s\n", addr, strerror(errno));
        return -1;
    }
    return 0;
}

int
r32_ctrl(struct straw_pathes* pp, u_int32_t addr, u_int32_t* data)
{
    struct sis1100_ctrl_reg req;
    int res;
    
    req.offset=addr;
    res=ioctl(pp->ctrl.p, SIS1100_LOCAL_CTRL_READ, &req);
    /*debug_r32_ctrl(&req);*/
    if (res) {
        fprintf(stderr, "r32_ctrl(addr=0x%08x): %s\n", addr, strerror(errno));
        return -1;
    }
    *data=req.val;
    return 0;
}

int
w32_remote(struct straw_pathes* pp, u_int32_t addr, u_int32_t data)
{
    struct sis1100_ctrl_reg req;
    int res;
    
    req.offset=addr;
    req.val=data;
    res=ioctl(pp->ctrl.p, SIS1100_REMOTE_CTRL_WRITE, &req);
    if (pp->debug) debug_w32_remote(&req);
    if (res) {
        fprintf(stderr, "w32_remote(addr=0x%08x): %s\n", addr, strerror(errno));
        return -1;
    }
    if (req.error) {
        fprintf(stderr, "w32_straw(addr=0x%08x): error=0x%x\n", addr, req.error);
        return -1;
    }
    return 0;
}


int
r32_remote(struct straw_pathes* pp, u_int32_t addr, u_int32_t* data)
{
    struct sis1100_ctrl_reg req;
    int res;
    
    req.offset=addr;
    res=ioctl(pp->ctrl.p, SIS1100_REMOTE_CTRL_READ, &req);
    if (pp->debug) debug_r32_remote(&req);
    if (res) {
        fprintf(stderr, "r32_remote(addr=0x%08x): %s\n", addr, strerror(errno));
        return -1;
    }
    if (req.error) {
        fprintf(stderr, "r32_straw(addr=0x%08x): error=0x%x\n", addr, req.error);
        return -1;
    }
    *data=req.val;
    return 0;
}
