/*
 * lowlevel/camac/sis5100/sis5100_open.c
 * 
 * created: 05.Apr.2004 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis5100_open.c,v 1.7 2011/04/06 20:30:22 wuestner Exp $";

#include <sconf.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "sis5100camac.h"
#include <dev/pci/sis1100_var.h>
#include <dev/pci/sis1100_map.h>
#include <rcs_ids.h>

#ifdef SIS5100_MMAPPED

# ifdef __linux__
#  define MAP_SHARED_FILE MAP_SHARED
# else /* __linux__ */
#  define MAP_SHARED_FILE MAP_FILE
# endif /* __linux__ */

RCS_REGISTER(cvsid, "lowlevel/camac/sis5100")

static int
sis5100_mmap(int p, char* name, volatile void** mapbase, size_t mapsize)
{
    ems_u32 size;
    ems_u32* base;
    if (ioctl(p, SIS1100_MAPSIZE, &size)<0) {
        printf("sis5100_mmap: ioctl(%s, SIS1100_MAPSIZE): %s\n",
                name, strerror(errno));
        return -1;
    }
    if (size<mapsize) {
        printf("sis5100_mmap(%s): mapsize=%d (%lld required)\n",
            name, size, (unsigned long long)mapsize);
        return -1;
    }
    printf("sis5100_open: mapsize for %s is 0x%x\n", name, size);
    if (size>mapsize)
        printf("sis5100_mmap(%s): only 0x%llx will be mapped\n",
                name, (unsigned long long)mapsize);
    base=mmap(0, mapsize, PROT_READ|PROT_WRITE, MAP_SHARED_FILE, p, 0);
    if (base==MAP_FAILED) {
        printf("sis5100 mmap %s: %s\n", name, strerror(errno));
        return -1;
    }
    *mapbase=base;
    return 0;
}
#endif

static int
try_open(struct camac_sis5100_info* info, char* name)
{
    int p;
    enum sis1100_subdev type;

    p=open(name, O_RDWR, 0);
    if (p<0) {
        printf("open \"%s\": %s\n", name, strerror(errno));
        return -1;
    }
    if (fcntl(p, F_SETFD, FD_CLOEXEC)<0) {
        printf("fcntl(%s, FD_CLOEXEC): %s\n", name, strerror(errno));
    }
    if (ioctl(p, SIS1100_DEVTYPE, &type)<0) {
        printf("ioctl(%s, SIS1100_DEVTYPE): %s\n",
                name, strerror(errno));
        return -1;
    }
    switch (type) {
    case sis1100_subdev_remote:
        printf("sis5100: %s \tis <REMOTE>\n", name);
        info->camac_path=p;
        break;
    case sis1100_subdev_ram:
        printf("sis5100: %s \tis RAM\n", name);
        close(p);
        break;
    case sis1100_subdev_ctrl:
        printf("sis5100: %s \tis CTRL\n", name);
        info->ctrl_path=p;
        break;
    case sis1100_subdev_dsp:
        printf("sis5100: %s \tis DSP\n", name);
        close(p);
        break;
    default:
        printf("init_path: %s has unknown type %d\n", name, type);
    }
    return 0;
}

static int
ischardev(char* name)
{
    struct stat status;

    if (stat(name, &status)<0) {
        printf("sis5100_open_devices: stat(%s): %s\n", name, strerror(errno));
        return 0;
    }
    return S_ISCHR(status.st_mode);
}

int
sis5100_open_devices(struct camac_sis5100_info* info, const char* stub)
{
    char *bname, *dname;
    char *_bname, *_dname, *_stub;
    DIR *dir;
    struct dirent* ent;
    int len;

    _stub=strdup(stub); /* copy stub because dirname and basename may modify
                           their arguments */
    if (!_stub) {
        printf("sis5100_open_devices: strdup: %s\n", strerror(errno));
        return -1;
    }

    _dname=dirname(_stub);
    dname=strdup(_dname);
    strcpy(_stub, stub); /* restaurate copy of stub for future use */
    if (!dname) {
        printf("sis5100_open_devices: strdup: %s\n", strerror(errno));
        free(_stub);
        return -1;
    }

    _bname=basename(_stub);
    bname=strdup(_bname);
    strcpy(_stub, stub); /* restaurate copy of stub for future use */
    if (!bname) {
        printf("sis5100_open_devices: strdup: %s\n", strerror(errno));
        free(_stub);
        free(dname);
        return -1;
    }
    free(_stub);

    len=strlen(bname);

    dir=opendir(dname);
    if (!dir) {
        printf("sis5100_open_devices opendir(%s): %s\n", dname,
                strerror(errno));
        free(dname);
        free(bname);
        return -1;
    }

    ent=readdir(dir);
    while (ent) {
        if (!strncmp(ent->d_name, bname, len)) {
            char* name=malloc(strlen(dname)+strlen(ent->d_name)+2);
            if (!name) {
                printf("sis5100_open_devices: malloc: %s\n", strerror(errno));
                continue;
            }
            sprintf(name, "%s/%s", dname, ent->d_name);
            if (ischardev(name)) {
                try_open(info, name);
            }
        }
        ent=readdir(dir);
    };
    closedir(dir);
    free(dname);
    free(bname);

#ifdef SIS5100_MMAPPED
    if (info->ctrl_path>=0) {
        u_int8_t* base;
        if (sis5100_mmap(info->ctrl_path, "ctrl",
                (volatile void**)&base, 0x1000)<0) {
            return -1;
        }
#if 1
        printf("sis5100_open_devices: control mapped at %p\n", base);
#endif
        info->ctrl_base=(struct sis1100_reg*)base;
        info->ctrl_base_rem=(struct sis5100_reg*)(base+0x800);
    }
    if (info->camac_path>=0) {
        if (sis5100_mmap(info->camac_path, "camac",
                (volatile void**)&info->camac_base, 0x10000)<0) {
            return -1;
        }
#if 1
        printf("sis5100_open_devices: camac mapped at %p\n", info->camac_base);
#endif
        info->ctrl_base->sp1_descr[0].hdr=0x0f010000;
        info->ctrl_base->sp1_descr[0].am=0;
        info->ctrl_base->sp1_descr[0].adl=0;
        info->ctrl_base->sp1_descr[0].adh=0;
    }
#endif

    return 0;
}

