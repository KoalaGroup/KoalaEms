/*
 * lowlevel/lvd/sis1100/sis1100_lvd_open.h
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis1100_lvd_open.c,v 1.9 2011/04/06 20:30:26 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <rcs_ids.h>
#include "sis1100_lvd_open.h"

#ifdef LVD_SIS1100_MMAPPED
#include <dev/pci/sis1100_map.h>
#include <sys/mman.h>
#endif

RCS_REGISTER(cvsid, "lowlevel/lvd/sis1100")

static const char *sis1100names[]={"remote", "ram", "ctrl", "dsp"};

int
sis1100_lvd_open_dev(const char* pathname, enum sis1100_subdev subdev)
{
    int p, len;
    char* pname;

    len=strlen(pathname)+strlen(sis1100names[subdev])+1;
    pname=malloc(len);
    if (!pname) {
        printf("open LVD: malloc: %s\n", strerror(errno));
        return -1;
    }
    snprintf(pname, len, "%s%s", pathname, sis1100names[subdev]);
    printf("lvd_init_sis1100: open(%s)\n", pname);
    p=open(pname, O_RDWR, 0);
    if (p<0) {
        printf("open LVD (sis1100) %s: %s\n", pname, strerror(errno));
        free(pname);
        return -1;
    }
    if (fcntl(p, F_SETFD, FD_CLOEXEC)<0) {
        printf("fcntl(LVD %s, FD_CLOEXEC): %s\n", pname, strerror(errno));
    }
    free(pname);

    return p;
}

#ifdef LVD_SIS1100_MMAPPED
int
sis1100_lvd_mmap_init(struct lvd_sis1100_link *link)
{
    u_int32_t descr[4]={0xff010000, 0, 0, 0};
    u_int32_t mapsize;

    if (ioctl(link->p_ctrl, SIS1100_MAPSIZE, &mapsize)<0) {
        printf("lvd_mmap: ioctl(MAPSIZE(ctrl): %s\n", strerror(errno));
        return -1;
    }
    link->ctrl_size=mapsize;
    printf("lvd_mmap: size of control space: %lld KByte\n",
            (long long int)(link->ctrl_size>>10));

    if (ioctl(link->p_remote, SIS1100_MAPSIZE, &mapsize)<0) {
        printf("lvd_mmap: ioctl(MAPSIZE(remote): %s\n", strerror(errno));
        return -1;
    }
    link->remote_size=mapsize;
    printf("lvd_mmap: size of remote space: %lld MByte\n",
            (unsigned long long int)(link->remote_size>>20));
    link->remote_size=2<<13; /* we need 8 KByte for LVDS */

    printf("mmap(0, %lld, %d, %d, %d, 0)\n",
        (unsigned long long)link->ctrl_size, PROT_READ|PROT_WRITE, MAP_SHARED,
        link->p_ctrl);
    link->pci_ctrl_base=mmap(0, link->ctrl_size, PROT_READ|PROT_WRITE,
            MAP_SHARED, link->p_ctrl, 0);
    if (link->pci_ctrl_base==MAP_FAILED) {
        printf("sis1100_lvd_mmap_init: mmap ctrl: %s\n", strerror(errno));
        return -1;
    }
    link->lvd_ctrl_base=(struct lvd_reg*)((char*)(link->pci_ctrl_base)+0x800);

    link->remote_base=mmap(0, link->remote_size, PROT_READ|PROT_WRITE,
            MAP_SHARED, link->p_remote, 0);
    if (link->remote_base==MAP_FAILED) {
        printf("sis1100_lvd_mmap_init: mmap remote: %s\n", strerror(errno));
        munmap(link->pci_ctrl_base, link->ctrl_size);
        return -1;
    }

    link->pci_ctrl_base->sp1_descr[0].hdr=descr[0];
    link->pci_ctrl_base->sp1_descr[0].am =descr[1];
    link->pci_ctrl_base->sp1_descr[0].adl=descr[2];
    link->pci_ctrl_base->sp1_descr[0].adh=descr[3];

    return 0;
}
#else
int
sis1100_lvd_mmap_init(struct lvd_sis1100_link *link)
{
    return 0;
}
#endif /* LVD_SIS1100_MMAPPED */

#ifdef LVD_SIS1100_MMAPPED
void
sis1100_lvd_mmap_done(struct lvd_sis1100_link *link)
{
    munmap(link->remote_base, link->remote_size);
    munmap(link->pci_ctrl_base, link->ctrl_size);
    link->remote_base=0;
    link->pci_ctrl_base=0;
}
#else
void
sis1100_lvd_mmap_done(struct lvd_sis1100_link *link)
{}
#endif /* LVD_SIS1100_MMAPPED */
