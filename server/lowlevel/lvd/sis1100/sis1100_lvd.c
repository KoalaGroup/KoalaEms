/*
 * lowlevel/lvd/sis1100/sis1100_lvd.c
 * created 10.Dec.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis1100_lvd.c,v 1.54 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <errorcodes.h>
#include <xprintf.h>
#include <rcs_ids.h>
#include "../../../main/nowstr.h"
#include "../../../commu/commu.h"
//#include "dev/pci/sis1100_var.h"
#include "dev/pci/sis1100_map.h"
#include "dev/pci/plx9054reg.h"
#include "../../../objects/pi/readout.h"
#include "../../../trigger/trigger.h"
#include "../lvdbus.h"
#include "../lvd_access.h"
#include "../datafilter/datafilter.h"
#include "sis1100_lvd_open.h"
#include "sis1100_lvd_read.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif
#define LVD_TRACE


struct lvd_sis1100_link *linkroot;

RCS_REGISTER(cvsid, "lowlevel/lvd/sis1100")

/*****************************************************************************/
#ifdef LVD_SIS1100_MMAPPED
static suseconds_t
tvdiff(struct timeval *t1, struct timeval *t0)
{
    time_t sdiff;
    suseconds_t usdiff;
    sdiff=t1->tv_sec-t0->tv_sec;
    usdiff=t1->tv_usec-t0->tv_usec;
    return usdiff+sdiff*1000000;
}
#endif
/*****************************************************************************/
#ifdef LVD_SIS1100_MMAPPED
static int
sis1100_lvd_write(struct lvd_dev* dev, u_int32_t addr, int size, ems_u32 data)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=info->B?info->B:info->A;
    struct sis1100_reg *pci_ctrl_base=link->pci_ctrl_base;
    struct lvd_bus1100 *remote_base=link->remote_base;
    u_int32_t prot_error;

    if (LVDTRACE_ACTIVE(dev->trace)) {
        printf("lvd_write (%s:0x%04x) %d ", dev->pathname, addr, size);
        if (size==4)
            printf("0x%08x ", data);
        else
            printf("    0x%04x ", data);
        lvd_decode_addr(dev, addr, size);
        printf("\n");
    }

    pci_ctrl_base->p_balance=0;
    prot_error=pci_ctrl_base->prot_error;

    switch (size) {
    case 4:
        ((ems_u32*)remote_base)[addr/4]=data;
        break;
    case 2:
        ((ems_u16*)remote_base)[addr/2]=data;
        break;
    default:
        printf("sis1100_lvd_write: illegal size %d\n", size);
        return -1;
    }

    prot_error=pci_ctrl_base->prot_error;
    if (prot_error) {
        if (prot_error==sis1100_e_dlock) {
            struct timeval tv_0, tv_1;
            suseconds_t udiff=0;
            gettimeofday(&tv_0, 0);
            do {
                gettimeofday(&tv_1, 0);
                udiff=tvdiff(&tv_1, &tv_0);
                prot_error=pci_ctrl_base->prot_error;
            } while (prot_error==sis1100_e_dlock && udiff<1000000);
        }
        if (prot_error==sis1100_e_dlock) {
            ems_u32 p_balance, prot_error_nw;
            p_balance=pci_ctrl_base->p_balance;
            prot_error_nw=pci_ctrl_base->prot_error_nw;
            pci_ctrl_base->p_balance=0;
            printf("sis1100_lvd_write: prot_error=sis1100_e_dlock\n"
                   "                   p_balance=%d error_nw=0x%x\n",
                    p_balance, prot_error_nw);
        }
        if (!dev->silent_errors) {
            printf("sis1100_lvd_write  (%s:0x%04x) size=%d 0x%x: error=0x%x ",
                    dev->pathname, addr, size, data, prot_error);
            lvd_decode_addr(dev, addr, size);
            printf("\n");
        }
        return -1;
    }
    return 0;
}
#else
static int
sis1100_lvd_write(struct lvd_dev* dev, u_int32_t addr, int size, ems_u32 data)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=info->B?info->B:info->A;
    struct sis1100_vme_req req;
    int res;

    if (LVDTRACE_ACTIVE(dev->trace)) {
        printf("lvd_write (%s:0x%04x) %d ", dev->pathname, addr, size);
        if (size==4)
            printf("0x%08x ", data);
        else
            printf("    0x%04x ", data);
        lvd_decode_addr(dev, addr, size);
        printf("\n");
    }

    req.size=size;
    req.am=-1;
    req.addr=addr;
    req.data=data;
    res=ioctl(link->p_remote, SIS3100_VME_WRITE, &req);
    if (res) {
        if (!dev->silent_errors) {
            printf("sis1100_lvd_write (%s:0x%04x) size=%d 0x%x: %s ",
                    dev->pathname, addr, size, data, strerror(errno));
            lvd_decode_addr(dev, addr, size);
            printf("\n");
        }
        return -1;
    }
    if (req.error) {
        if (!dev->silent_errors) {
            printf("sis1100_lvd_write  (%s:0x%04x) size=%d 0x%x: error=0x%x ",
                    dev->pathname, addr, size, data, req.error);
            lvd_decode_addr(dev, addr, size);
            printf("\n");
        }
        return -1;
    }

    return 0;
}
#endif
/*****************************************************************************/
/*
    sis1100_lvd_write_blind does the same as sis1100_lvd_write but
    does not expect or recognize any response (neither acknowledge nor error)
*/
static int
sis1100_lvd_write_blind(struct lvd_dev* dev, u_int32_t addr, int size,
        ems_u32 data)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=info->B?info->B:info->A;
    struct sis1100_vme_req req;
    int res;

    req.size=size;
    req.am=-1;
    req.addr=addr;
    req.data=data;
    res=ioctl(link->p_remote, SIS3100_VME_WRITE_BLIND, &req);
    if (res) {
        if (!dev->silent_errors) {
            printf("sis1100_lvd_write_blind (%s:0x%04x) size=%d 0x%x: %s ",
                    dev->pathname, addr, size, data, strerror(errno));
            lvd_decode_addr(dev, addr, size);
            printf("\n");
        }
        return -1;
    }
    return 0;
}
/*****************************************************************************/
#ifdef LVD_SIS1100_MMAPPED
static int
sis1100_lvd_read(struct lvd_dev* dev, u_int32_t addr, int size, u_int32_t* data)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=info->B?info->B:info->A;
    struct sis1100_reg *pci_ctrl_base=link->pci_ctrl_base;
    struct lvd_bus1100 *remote_base=link->remote_base;
    u_int32_t prot_error;

    pci_ctrl_base->p_balance=0;
    prot_error=pci_ctrl_base->prot_error;

    switch (size) {
    case 4:
        *data=((ems_u32*)remote_base)[addr/4];
        break;
    case 2:
        *data=((ems_u16*)remote_base)[addr/2];
        break;
    default:
        printf("sis1100_lvd_read: illegal size %d\n", size);
        return -1;
    }

    while ((prot_error=pci_ctrl_base->prot_error)==sis1100_e_dlock);
    if (prot_error!=0) {
        ems_u32 hdr;
        if (prot_error!=sis1100_le_dlock) {
            printf("lvd_read: error=0x%x\n", prot_error);
            return -1;
        }
        hdr=pci_ctrl_base->tc_hdr;
        if ((hdr&0x300)==0x300) {
            printf("lvd_read: error=0x%x (after deadlock)\n", 0x200|(hdr>>24));
            return -1;
        }
        *data=pci_ctrl_base->tc_dal;
    }

    if (LVDTRACE_ACTIVE(dev->trace)) {
        printf("lvd_read  (%s:0x%04x) %d ", dev->pathname, addr, size);
        if (size==4)
            printf("0x%08x ", *data);
        else
            printf("    0x%04x ", *data);
        lvd_decode_addr(dev, addr, size);
        printf("\n");
    }
    return 0;
}
#else
static int
sis1100_lvd_read(struct lvd_dev* dev, u_int32_t addr, int size, u_int32_t* data)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=info->B?info->B:info->A;
    struct sis1100_vme_req req;
    int res;

    req.size=size;
    req.am=-1;
    req.addr=addr;
    req.data=0;
    res=ioctl(link->p_remote, SIS3100_VME_READ, &req);
    if (res) {
        if (!dev->silent_errors) {
            printf("lvd_read (%s:0x%04x) size=%d: %s ",
                    dev->pathname, addr, size, strerror(errno));
            lvd_decode_addr(dev, addr, size);
            printf("\n");
        }
        return -1;
    }
    if (req.error) {
        if (!dev->silent_errors) {
            printf("lvd_read (%s:addr=0x%04x) size=%d: error=0x%x ",
                    dev->pathname, addr, size, req.error);
            lvd_decode_addr(dev, addr, size);
            printf("\n");
        }
        return -1;
    }

    if (LVDTRACE_ACTIVE(dev->trace)) {
        printf("lvd_read  (%s:0x%04x) %d ", dev->pathname, addr, size);
        if (size==4)
            printf("0x%08x ", req.data);
        else
            printf("    0x%04x ", req.data);
        lvd_decode_addr(dev, addr, size);
        printf("\n");
    }
    *data=req.data;
    return 0;
}
#endif
/*****************************************************************************/
static int
sis1100_lvd_localreset(struct lvd_dev* dev)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=info->B?info->B:info->A;
    int res;

    res=ioctl(link->p_remote, SIS1100_RESET, 0);
    if (res) {
        printf("sis1100_lvd_localreset: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}
/*****************************************************************************/
static plerrcode
sis1100_lvd_plxwrite(struct lvd_sis1100_link *link, ems_u32 addr, ems_u32 data)
{
    struct sis1100_ctrl_reg req;
    int res;

    req.offset=addr;
    req.val=data;
    res=ioctl(link->p_ctrl, SIS1100_PLX_WRITE, &req);
    if (res) {
        printf("lvd_set_plxreg(0x%08x, 0x%08x): %s\n", addr, data,
                strerror(errno));
        return plErr_System;
    }
    if (req.error) {
        printf("lvd_set_plxreg(0x%08x, 0x%08x): error=0x%x\n", addr,
                data, req.error);
        return plErr_System;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
sis1100_lvd_plxread(struct lvd_sis1100_link *link, u_int32_t addr,
        u_int32_t* data)
{
    struct sis1100_ctrl_reg req;
    int res;

    req.offset=addr;
    req.val=0;
    res=ioctl(link->p_ctrl, SIS1100_PLX_READ, &req);
    if (res) {
        printf("lvd_get_plxreg(0x%08x): %s\n", addr, strerror(errno));
        return plErr_System;
    }
    if (req.error) {
        printf("lvd_get_plxreg(0x%08x): error=0x%x\n", addr, req.error);
        return plErr_System;
    }
    *data=req.val;
    return plOK;
}
/*****************************************************************************/
#ifdef LVD_SIS1100_MMAPPED
static plerrcode
sis1100_lvd_siswrite(struct lvd_sis1100_link *link, u_int32_t addr,
        ems_u32 data)
{
    struct sis1100_reg *pci_ctrl_base=link->pci_ctrl_base;

    if (addr>0x800) {
        printf("sis1100_lvd_siswrite: addr=0x%x\n", addr);
        return plErr_ArgRange;
    }

    ((ems_u32*)pci_ctrl_base)[addr/4]=data;
    return plOK;
}
#else
static plerrcode
sis1100_lvd_siswrite(struct lvd_sis1100_link *link, u_int32_t addr,
        ems_u32 data)
{
    struct sis1100_ctrl_reg req;
    int res;

    req.offset=addr;
    req.val=data;
    res=ioctl(link->p_ctrl, SIS1100_CTRL_WRITE, &req);
    if (res) {
        printf("lvd_set_sisreg(0x%08x, 0x%08x): %s\n", addr, data,
                strerror(errno));
        return plErr_System;
    }
    if (req.error) {
        printf("lvd_set_sisreg(0x%08x, 0x%08x): error=0x%x\n", addr,
                data, req.error);
        return plErr_System;
    }
    return plOK;
}
#endif
/*****************************************************************************/
#ifdef LVD_SIS1100_MMAPPED
static plerrcode
sis1100_lvd_sisread(struct lvd_sis1100_link *link, u_int32_t addr,
        ems_u32* data)
{
    struct sis1100_reg *pci_ctrl_base=link->pci_ctrl_base;

    if (addr>0x800) {
        printf("sis1100_lvd_sisread: addr=0x%x\n", addr);
        return plErr_ArgRange;
    }

    *data=((ems_u32*)pci_ctrl_base)[addr/4];
    return plOK;
}
#else
static plerrcode
sis1100_lvd_sisread(struct lvd_sis1100_link *link, u_int32_t addr,
        u_int32_t* data)
{
    struct sis1100_ctrl_reg req;
    int res;

    req.offset=addr;
    req.val=0;
    res=ioctl(link->p_ctrl, SIS1100_CTRL_READ, &req);
    if (res) {
        printf("lvd_get_sisreg(0x%08x): %s\n", addr, strerror(errno));
        return plErr_System;
    }
    if (req.error) {
        printf("lvd_get_sisreg(0x%08x): error=0x%x\n", addr, req.error);
        return plErr_System;
    }
    *data=req.val;
    return plOK;
}
#endif
/*****************************************************************************/
#ifdef LVD_SIS1100_MMAPPED
static plerrcode
sis1100_lvd_mcwrite(struct lvd_sis1100_link *link, u_int32_t addr,
        ems_u32 data)
{
    struct sis1100_reg *pci_ctrl_base=link->pci_ctrl_base;
    struct lvd_reg *lvd_ctrl_base=link->lvd_ctrl_base;
    u_int32_t prot_error;
    int deadcount=-1;

    pci_ctrl_base->p_balance=0;
    prot_error=pci_ctrl_base->prot_error;

    ((ems_u32*)lvd_ctrl_base)[addr/4]=data;

    do {
        prot_error=pci_ctrl_base->prot_error;
        deadcount++;
    } while (prot_error==sis1100_e_dlock);
    if (prot_error==sis1100_le_dlock) {
        ems_u32 hdr;
        if (prot_error!=sis1100_le_dlock) {
            printf("lvd_mcwrite: error=0x%x\n", prot_error);
            return plErr_System;
        }
        hdr=pci_ctrl_base->tc_hdr;
        if ((hdr&0x300)==0x300) {
            printf("lvd_mcwrite: error=0x%x (after deadlock)\n",
                    0x200|(hdr>>24));
            return plErr_System;
        }
    }
    if (prot_error) {
        printf("sis1100_lvd_mcwrite(0x%08x, 0x%08x): error=0x%x, "
                "deadcount=%d\n",
                addr, data, prot_error, deadcount);
        return plErr_System;
    }

    return plOK;
}
#else
static plerrcode
sis1100_lvd_mcwrite(struct lvd_sis1100_link *link, u_int32_t addr,
        ems_u32 data)
{
    struct sis1100_ctrl_reg req;
    int res;

    req.offset=addr;
    req.val=data;
    req.error=0;
    res=ioctl(link->p_remote, SIS1100_CTRL_WRITE, &req);
    if (res) {
        printf("sis1100_lvd_mcwrite(0x%08x, 0x%08x): %s\n", addr, data,
                strerror(errno));
        return plErr_System;
    }
    if (req.error) {
        printf("sis1100_lvd_mcwrite(0x%08x, 0x%08x): error=0x%x\n", addr,
                data, req.error);
        return plErr_System;
    }
    return plOK;
}
#endif
/*****************************************************************************/
#ifdef LVD_SIS1100_MMAPPED
static plerrcode
sis1100_lvd_mcread(struct lvd_sis1100_link *link, u_int32_t addr,
        u_int32_t* data)
{
    struct sis1100_reg *pci_ctrl_base=link->pci_ctrl_base;
    struct lvd_reg *lvd_ctrl_base=link->lvd_ctrl_base;
    u_int32_t prot_error;
    int deadcount=-1;

    pci_ctrl_base->p_balance=0;
    prot_error=pci_ctrl_base->prot_error;

    *data=((ems_u32*)lvd_ctrl_base)[addr/4];

    do {
        prot_error=pci_ctrl_base->prot_error;
        deadcount++;
    } while (prot_error==sis1100_e_dlock);
    if (prot_error==sis1100_le_dlock) {
        ems_u32 hdr;
        if (prot_error!=sis1100_le_dlock) {
            printf("lvd_mcread: error=0x%x\n", prot_error);
            return plErr_System;
        }
        hdr=pci_ctrl_base->tc_hdr;
        if ((hdr&0x300)==0x300) {
            printf("lvd_mcread: error=0x%x (after deadlock)\n",
                    0x200|(hdr>>24));
            return plErr_System;
        }
        *data=pci_ctrl_base->tc_dal;
        prot_error=(pci_ctrl_base->tc_hdr&0xff)|0x200;
    }
    if (prot_error) {
        printf("sis1100_lvd_mcread(0x%08x): error=0x%x, deadcount=%d\n",
                addr, prot_error, deadcount);
        return plErr_System;
    }

    return plOK;
}
#else
static plerrcode
sis1100_lvd_mcread(struct lvd_sis1100_link *link, u_int32_t addr,
        u_int32_t* data)
{
    struct sis1100_ctrl_reg req;
    int res;

    req.offset=addr;
    req.val=0;
    req.error=0;
    res=ioctl(link->p_remote, SIS1100_CTRL_READ, &req);
    if (res) {
        printf("sis1100_lvd_mcread(0x%08x): %s\n", addr, strerror(errno));
        return plErr_System;
    }
    if (req.error) {
        printf("sis1100_lvd_mcread(0x%08x): error=0x%x\n", addr, req.error);
        return plErr_System;
    }
    *data=req.val;
    return plOK;
}
#endif
/*****************************************************************************/
static plerrcode
sis1100_lvd_sisread_(struct lvd_dev* dev, int lnk, u_int32_t addr,
        ems_u32* data)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=(lnk&&info->B)?info->B:info->A;
    return sis1100_lvd_sisread(link, addr, data);
}
static plerrcode
sis1100_lvd_siswrite_(struct lvd_dev* dev, int lnk, u_int32_t addr,
        ems_u32 data)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=(lnk&&info->B)?info->B:info->A;
    return sis1100_lvd_siswrite(link, addr, data);
}
static plerrcode
sis1100_lvd_mcread_(struct lvd_dev* dev, int lnk, u_int32_t addr,
        u_int32_t* data)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=(lnk&&info->B)?info->B:info->A;
    return sis1100_lvd_mcread(link, addr, data);
}
static plerrcode
sis1100_lvd_mcwrite_(struct lvd_dev* dev, int lnk, u_int32_t addr,
        ems_u32 data)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=(lnk&&info->B)?info->B:info->A;
    return sis1100_lvd_mcwrite(link, addr, data);
}
static plerrcode
sis1100_lvd_plxread_(struct lvd_dev* dev, int lnk, u_int32_t addr,
        u_int32_t* data)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=(lnk&&info->B)?info->B:info->A;
    return sis1100_lvd_plxread(link, addr, data);
}
static plerrcode
sis1100_lvd_plxwrite_(struct lvd_dev* dev, int lnk, u_int32_t addr,
        ems_u32 data)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=(lnk&&info->B)?info->B:info->A;
    return sis1100_lvd_plxwrite(link, addr, data);
}
/*****************************************************************************/
static plerrcode
sis1100_plx_stat(struct lvd_dev* dev, int lnk, void *xp, __attribute__((unused)) int level)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=lnk?info->B:info->A;
    ems_u32 data=0x5a5a;
    int a;

    if (!link)
        return plErr_ArgRange;

    xprintf(xp, "sis1100_pci (PLX):\n");

    a=offsetof(struct plx9054reg, L2PDBELL);
    sis1100_lvd_plxread(link, a, &data);
    xprintf(xp, "  [%02x] L2PDBELL  =0x%08x\n", a, data);

    a=offsetof(struct plx9054reg, INTCSR);
    sis1100_lvd_plxread(link, a, &data);
    xprintf(xp, "  [%02x] INTCSR    =0x%08x\n", a, data);

    a=offsetof(struct plx9054reg, DMAMODE0);
    sis1100_lvd_plxread(link, a, &data);
    xprintf(xp, "  [%02x] DMAMODE0  =0x%08x\n", a, data);

    a=offsetof(struct plx9054reg, DMAPADR0);
    sis1100_lvd_plxread(link, a, &data);
    xprintf(xp, "  [%02x] DMAPADR0  =0x%08x\n", a, data);

    a=offsetof(struct plx9054reg, DMALADR0);
    sis1100_lvd_plxread(link, a, &data);
    xprintf(xp, "  [%02x] DMALADR0  =0x%08x\n", a, data);

    a=offsetof(struct plx9054reg, DMASIZ0);
    sis1100_lvd_plxread(link, a, &data);
    xprintf(xp, "  [%02x] DMASIZ0   =0x%08x\n", a, data);

    a=offsetof(struct plx9054reg, DMADPR0);
    sis1100_lvd_plxread(link, a, &data);
    xprintf(xp, "  [%02x] DMADPR0   =0x%08x\n", a, data);

    a=offsetof(struct plx9054reg, DMACSR0_DMACSR1);
    sis1100_lvd_plxread(link, a, &data);
    xprintf(xp, "  [%02x] DMACSR0/1 =0x%08x\n", a, data);

    return plOK;
}
/*****************************************************************************/
static plerrcode
sis1100_sis_stat(struct lvd_dev* dev, int lnk, void *xp, __attribute__((unused)) int level)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=lnk?info->B:info->A;
    ems_u32 data=0x5a5a;
    int a;

    if (!link)
        return plErr_ArgRange;

    xprintf(xp, "sis1100_pci (addon):\n");

    a=offsetof(struct sis1100_reg, ident);
    sis1100_lvd_sisread(link, a, &data);
    xprintf(xp, "  [%02x] ident     =0x%08x\n", a, data);

    a=offsetof(struct sis1100_reg, sr);
    sis1100_lvd_sisread(link, a, &data);
    xprintf(xp, "  [%02x] sr        =0x%08x\n", a, data);

    a=offsetof(struct sis1100_reg, cr);
    sis1100_lvd_sisread(link, a, &data);
    xprintf(xp, "  [%02x] cr        =0x%08x\n", a, data);

    a=offsetof(struct sis1100_reg, doorbell);
    sis1100_lvd_sisread(link, a, &data);
    xprintf(xp, "  [%02x] doorbell  =0x%08x\n", a, data);

    a=offsetof(struct sis1100_reg, p_balance);
    sis1100_lvd_sisread(link, a, &data);
    xprintf(xp, "  [%02x] p_balance =0x%08x\n", a, data);

    a=offsetof(struct sis1100_reg, prot_error);
    sis1100_lvd_sisread(link, a, &data);
    xprintf(xp, "  [%02x] prot_error=0x%08x\n", a, data);

    a=offsetof(struct sis1100_reg, d0_bc);
    sis1100_lvd_sisread(link, a, &data);
    xprintf(xp, "  [%02x] d0_bc     =0x%08x\n", a, data);

    a=offsetof(struct sis1100_reg, d0_bc_buf);
    sis1100_lvd_sisread(link, a, &data);
    xprintf(xp, "  [%02x] d0_bc_buf =0x%08x\n", a, data);

    a=offsetof(struct sis1100_reg, d0_bc_blen);
    sis1100_lvd_sisread(link, a, &data);
    xprintf(xp, "  [%02x] d0_bc_blen=0x%08x\n", a, data);

    return plOK;
}
/*****************************************************************************/
static plerrcode
sis1100_mc_stat(struct lvd_dev* dev, int lnk, void *xp, __attribute__((unused)) int level)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=lnk?info->B:info->A;
    ems_u32 data=0x5a5a;
    int a;

    if (!link)
        return plErr_ArgRange;

    xprintf(xp, "controller master:\n");

    if (!link->online) {
        xprintf(xp, "  link offline\n");
        return plOK;
    }

    a=offsetof(struct lvd_reg, ident);
    sis1100_lvd_mcread(link, a, &data);
    xprintf(xp, "  [%02x] ident       =0x%08x\n", a, data);

    a=offsetof(struct lvd_reg, sr);
    sis1100_lvd_mcread(link, a, &data);
    xprintf(xp, "  [%02x] sr          =0x%08x", a, data);
    if (data&LVD_SR_SPEED_0)    xprintf(xp, " FAST_0");
    if (data&LVD_SR_SPEED_1)    xprintf(xp, " FAST_1");
    if (data&LVD_SR_TX_SYNCH_0) xprintf(xp, " TX_0");
    if (data&LVD_SR_TX_SYNCH_1) xprintf(xp, " TX_1");
    if (data&LVD_SR_RX_SYNCH_0) xprintf(xp, " RX_0");
    if (data&LVD_SR_RX_SYNCH_1) xprintf(xp, " RX_1");
    if (data&LVD_SR_LINK_B)     xprintf(xp, " LINK_B");
    if (data&LVD_DBELL_LVD_ERR) xprintf(xp, " LVD_ERR");
    if (data&LVD_DBELL_EVENTPF) xprintf(xp, " EVENTPF");
    if (data&LVD_DBELL_EVENT)   xprintf(xp, " EVENT");
    if (data&LVD_DBELL_INT)     xprintf(xp, " INT");
    xprintf(xp, "\n");

    a=offsetof(struct lvd_reg, cr);
    sis1100_lvd_mcread(link, a, &data);
    xprintf(xp, "  [%02x] cr          =0x%08x", a, data);
    if (data&LVD_CR_DDTX)   xprintf(xp, " DDTX");
    if (data&LVD_CR_SCAN)   xprintf(xp, " SCAN");
    if (data&LVD_CR_SINGLE) xprintf(xp, " SINGLE");
    if (data&LVD_CR_LED0)   xprintf(xp, " LED0");
    if (data&LVD_CR_LED1)   xprintf(xp, " LED1");
    xprintf(xp, "\n");

    a=offsetof(struct lvd_reg, ddt_counter);
    sis1100_lvd_mcread(link, a, &data);
    xprintf(xp, "  [%02x] ddt_count   =0x%08x\n", a, data);

    a=offsetof(struct lvd_reg, ddt_blocksz);
    sis1100_lvd_mcread(link, a, &data);
    xprintf(xp, "  [%02x] ddt_intsize =0x%08x\n", a, data);

    return plOK;
}
/*****************************************************************************/
static plerrcode
sis1100_lvd_rw_block(struct lvd_dev* dev, int ci, int addr, int reg,
    int size, ems_u32* buffer, size_t* num, int rw)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=info->B?info->B:info->A;
    struct sis1100_vme_block_req breq;
    int res;
    int dmalen[2]={1, 1};
    DEFINE_LVDTRACE(trace);

    lvd_settrace(dev, &trace, 0);

    res=ioctl(link->p_remote, SIS1100_MINDMALEN, dmalen);
    if (res) {
        printf("SIS1100_MINDMALEN: %s\n", strerror(errno));
    }

    breq.size=size;
    breq.fifo=1;
    breq.num=*num;
    breq.am=-1;
    if (ci) {
        breq.addr=ofs(struct lvd_bus1100, prim_controller)+reg;
    } else {
        breq.addr=ofs(struct lvd_bus1100, in_card[addr&0xf])+reg;
    }
    breq.data=(u_int8_t*)buffer;
    breq.error=0;
#if 0
    printf("lvd_rw_block: size=%d num=%lld addr=0x%x data=%p\n",
            breq.size, (unsigned long long)breq.num, breq.addr, breq.data);
printf("breq.size=%d breq.fifo=%d breq.am=%d\n", breq.size, breq.fifo, breq.am);
printf("breq.num=%lu\n", breq.num);
printf("breq.data=%p\n", breq.data);
printf("breq.addr=0x%08x\n", breq.addr);
#endif

    res=ioctl(link->p_remote,
            rw?SIS3100_VME_BLOCK_WRITE:SIS3100_VME_BLOCK_READ, &breq);
    if (res) {
        printf("sis1100_lvd_rw_block: %s\n", strerror(errno));
        lvd_settrace(dev, 0, trace);
        return plErr_System;
    }
#if 0
    printf("num=%lld error=0x%x\n", (unsigned long long)breq.num, breq.error);
#endif
    if (breq.error) {
        printf("sis1100_lvd_rw_block: error=0x%x\n", breq.error);
        if ((breq.error!=0x208) && (breq.error!=0x209)) {
            lvd_settrace(dev, 0, trace);
            return plErr_HW;
        }
    }
#if 0
for (i=0; i<10; i++) {
    printf("[%2d]: 0x%08x\n", i, buffer[i]);
}
#endif
    *num=breq.num;

    lvd_settrace(dev, 0, trace);

    return plOK;
}
/*****************************************************************************/
plerrcode
sis1100_lvd_read_block(struct lvd_dev* dev, int ci, int addr, int reg,
    int size, ems_u32* buffer, size_t* num)
{
    return sis1100_lvd_rw_block(dev, ci, addr, reg, size, buffer, num, 0);
}

plerrcode
sis1100_lvd_write_block(struct lvd_dev* dev, int ci, int addr, int reg,
    int size, ems_u32* buffer, size_t* num)
{
    return sis1100_lvd_rw_block(dev, ci, addr, reg, size, buffer, num, 1);
}
/*****************************************************************************/
static errcode
lvd_done_link(struct lvd_sis1100_link *link)
{
    printf("lvd_done_link(%s) link=%p\n", link->pathname, link);
    sis1100_lvd_mmap_done(link);

    if (link->p_remote>=0)
        close(link->p_remote);
    if (link->p_ctrl>=0)
        close(link->p_ctrl);
    link->p_remote=link->p_ctrl=-1;

    free(link->pathname);
    link->pathname=0;

    /* remove from linked list */
    if (link->next)
        link->next->prev=link->prev;
    if (link->prev)
        link->prev->next=link->next;
    else
        linkroot=link->next;

    return OK;
}
/*****************************************************************************/
static plerrcode
sis1100_link_stat_(struct lvd_sis1100_link *link, void *xp)
{
    xprintf(xp, "    path: %s, o%sline, priority=%d, dev=%p\n", link->pathname,
            link->online?"n":"ff", link->priority, link->dev);
    xprintf(xp, "    p_ctrl=%d p_remote=%d\n", link->p_ctrl, link->p_remote);
#ifdef LVD_SIS1100_MMAPPED
    xprintf(xp, "    pci_base=%p lvd_base=%p remote_base=%p\n",
        link->pci_ctrl_base, link->lvd_ctrl_base, link->remote_base);
    xprintf(xp, "    ctrl_size=%llu remote_size=%llu\n",
        (unsigned long long)link->ctrl_size,
        (unsigned long long)link->remote_size);
#endif
    xprintf(xp, "    seltaskdescr=%p\n", link->st);
    xprintf(xp, "    speed=%d\n", link->speed);
    return 0;
}
/*****************************************************************************/
static plerrcode
sis1100_link_stat(struct lvd_dev* dev, void *xp)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;

    xprintf(xp, "lvd sis1100 link state:\n");
    xprintf(xp, "  path: %s, o%sline, dev=%p\n", dev->pathname,
            dev->generic.online?"n":"ff", dev);
    xprintf(xp, "  controller: %04x/%04x\n", dev->ccard.id, dev->ccard.serial);
    xprintf(xp, "  irq_mask  : 0x%x\n", info->irq_mask);
    xprintf(xp, "  auto_mask : 0x%x\n", info->autoack_mask);
    if (!info->irq_callbacks) {
        xprintf(xp, "  no irq_callbacks\n");
    } else {
        struct lvd_irq_callback *callback=info->irq_callbacks;
        xprintf(xp, "  irq_callbacks:\n");
        while (callback) {
            xprintf(xp, "    mask=0x%x callback=%p\n",
                callback->mask, callback->callback);
            xprintf(xp, "    procname: %s\n", callback->procname);
            callback=callback->next;
        }
    }

    xprintf(xp, "  link A (%p)\n", info->A);
    if (info->A) {
        sis1100_link_stat_(info->A, xp);
    }
    xprintf(xp, "  link B (%p)\n", info->B);
    if (info->B) {
        sis1100_link_stat_(info->B, xp);
    }

    return 0;
}
/*****************************************************************************/
static errcode
lvd_done_sis1100(struct generic_dev* gdev)
{
    struct lvd_dev* dev=(struct lvd_dev*)gdev;
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;

    printf("== %s sis1100_lvd.c:lvd_done_sis1100 ==\n", nowstr());

    lvd_settrace(dev, 0, 0);

#if 0
    if (info->ddma_active) {
        /* close(info->p_ctrl) sollte das automatisch machen */
        sis1100_lvd_stop_async(dev);
    }
#endif

    lvdbus_done(dev);

printf("lvd_done_sis1100: A=%p B=%p\n", info->A, info->B);
printf("lvd_done_sis1100: A->st=%p\n", info->A->st);
    if (info->A->st)
        sched_delete_select_task(info->A->st);
    if (info->B && info->B->st) {
printf("lvd_done_sis1100: B->st=%p\n", info->B->st);
        sched_delete_select_task(info->B->st);
}

    lvd_done_link(info->A);
    if (info->B)
        lvd_done_link(info->B);

printf("try to free _dma_buf: %p\n", info->ra._dma_buf);
    if (info->ra._dma_buf!=0)
        free(info->ra._dma_buf);
    while (dev->datafilter) {
        struct datafilter *filter=dev->datafilter;
        dev->datafilter=dev->datafilter->next;
        lvd_free_datafilter(filter);
    }
    if (dev->pathname)
        free(dev->pathname);
    dev->pathname=0;

    free(info);
    dev->info=0;
    return OK;
}
/*****************************************************************************/
/* to be used only from sis1100_check_linktype! */
static int
lvd_read_simple(int p, int addr, u_int32_t* data)
{
    struct sis1100_vme_req req;
    req.size=2;
    req.am=-1;
    req.addr=addr;
    if (ioctl(p, SIS3100_VME_READ, &req)<0) {
        printf("lvd_read_simple: p=%d addr=0x%x: %s\n", p, addr, strerror(errno));
        return -1;
    }
    if (req.error) {
        printf("lvd_read_simple: p=%d addr=0x%x: 0x%x\n", p, addr, req.error);
        return -1;
    }
    *data=req.data;
    return 0;
}
/*****************************************************************************/
static int
sis1100_check_linktype(struct lvd_sis1100_link *link)
{
    plerrcode pres;
    ems_u32 mcsr;
    int dual;

    link->priority=1;

    /* controller with dual link? */
    dual=link->ident.remote.hw_version>=3;
    printf("sis1100(%s): controller with %s link.\n",
            link->pathname, dual?"dual":"single");

    pres=sis1100_lvd_mcread(link, 4, &mcsr); /* status register */
    if (pres!=plOK) {
        printf("sis1100_check_linktype: mcread failed\n");
        link->priority=0;
        return -1;
    }
    if (lvd_read_simple(link->p_remote,
            offsetof(struct lvd_controller_common, ident)+0x1800,
            &link->controller_id)<0)
        return -1;
    if (lvd_read_simple(link->p_remote,
            offsetof(struct lvd_controller_common, serial)+0x1800,
            &link->controller_serial)<0)
        return -1;

    if (mcsr&LVD_SR_LINK_B) {
/* it's the wrong place for this complaint! */
        complain("the link used (%s) is not usable for readout!",
                link->pathname);
        link->priority=0;
        link->speed=(mcsr>>31)&1;
    } else {
        link->speed=(mcsr>>30)&1;
    }
    printf("  link speed is %d GBit/s\n", link->speed+1);
    return 0;
}
/*****************************************************************************/
static int
sis1100_check_ident(struct lvd_sis1100_link *link)
{
    //struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    //struct lvd_sis1100_link *link=info->B?info->B:info->A;
    //struct sis1100_ident ident;
    u_int32_t serial[4];

    if (ioctl(link->p_ctrl, SIS1100_IDENT, &link->ident)<0) {
        printf("lvd_sis1100_check_ident(%s): fcntl(p_ctrl, IDENT): %s\n",
                link->pathname, strerror(errno));
        link->online=0;
        return -1;
    }

    printf("lvd_init_sis1100(%s) local ident:\n", link->pathname);
    printf("  hw_type   =%d\n", link->ident.local.hw_type);
    printf("  hw_version=%d\n", link->ident.local.hw_version);
    printf("  fw_type   =%d\n", link->ident.local.fw_type);
    printf("  fw_version=%d\n", link->ident.local.fw_version);
    if (ioctl(link->p_ctrl, SIS1100_SERIAL_NO, serial)<0) {
        printf("lvd_sis1100_check_ident(%s): fcntl(p_ctrl, SERIAL): %s\n",
                link->pathname, strerror(errno));
    } else {
        printf("  serial number: %d (", serial[1]);
        switch (serial[0]) {
        case 0: printf("PCI 5V"); break;
        case 1: printf("PCI 3.3V"); break;
        case 2: printf("PCI univ."); break;
        case 3: printf("PCIe+piggiback"); break;
        case 4: printf("PCIe single link"); break;
        case 5: printf("PCIe four links"); break;
        default: printf("unknown type %d", serial[0]);
        }
        printf(")\n");
    }
    if (link->ident.remote_ok) {
        printf("lvd_init_sis1100 remote ident:\n");
        printf("  hw_type   =%d\n", link->ident.remote.hw_type);
        printf("  hw_version=%d\n", link->ident.remote.hw_version);
        printf("  fw_type   =%d\n", link->ident.remote.fw_type);
        printf("  fw_version=%d\n", link->ident.remote.fw_version);
    }
    if (link->ident.remote_online) {
        if (link->ident.remote.hw_type==sis1100_hw_lvd) {
            //info->sis1100_ident_lvd.hw_version=ident.remote.hw_version;
            //info->sis1100_ident_lvd.fw_type=ident.remote.fw_type;
            //info->sis1100_ident_lvd.fw_version=ident.remote.fw_version;
            link->online=1;
            sis1100_check_linktype(link);
            printf("controller: %04x/%04x\n", link->controller_id,
                    link->controller_serial);
        } else {
            printf("lvd_init_sis1100: Wrong hardware connected!\n");
            printf("remote hardware code is %d ", link->ident.remote.hw_type);
            switch (link->ident.remote.hw_type) {
            case sis1100_hw_invalid: printf("(%s)\n", "invalid"); break;
            case sis1100_hw_pci:     printf("(%s)\n", "pci"); break;
            case sis1100_hw_vme:     printf("(%s)\n", "vme"); break;
            case sis1100_hw_camac:   printf("(%s)\n", "camac"); break;
            case sis1100_hw_lvd:     printf("(%s)\n", "lvd"); break;
            case sis1100_hw_pandapixel: printf("(%s)\n", "pandapixel"); break;
            default: printf("(%s)\n", "unknown");
            }
            link->online=0;
        }
    } else {
        printf("lvd_init_sis1100(%s): NO remote Interface!\n", link->pathname);
        link->online=0;
    }
    return 0;
}
/*****************************************************************************/
static void
print_offline(struct lvd_sis1100_link *link, char *text)
{
    struct sis1100_vme_req req;
    ems_u32 offline, online;
    int p=link->p_remote;

    req.size=2;
    req.am=-1;
    req.addr=offsetof(struct lvd_in_card_bc, card_offl);
    req.data=0xa5a5;
    printf("print_offline: addr=%x\n", req.addr);
    if (ioctl(p, SIS3100_VME_READ, &req))
        printf("print_offline: %s\n", strerror(errno));
    if (req.error)
        printf("print_offline: %x\n", req.error);
    offline=req.data;
    req.size=2;
    req.am=-1;
    req.addr=offsetof(struct lvd_in_card_bc, card_onl);
    req.data=0xa5a5;
    printf("print_offline: addr=%x (%s)\n", req.addr, text);
    ioctl(p, SIS3100_VME_READ, &req);
    if (ioctl(p, SIS3100_VME_READ, &req))
        printf("print_offline: %s\n", strerror(errno));
    if (req.error)
        printf("print_offline: %x\n", req.error);
    online=req.data;
    printf("  online=%04x offline=%04x\n", online, offline);
}
/*****************************************************************************/
/*
 * This is called at end of initialization and after (re)connection of the
 * optical link.
 * If the remote system is still initialized (no module is offline or the
 * user LEDs are active no further action is performed.
 * Otherwise a basic initialisation will occur.
 * The function returns +1 if crate seems to be still initialised.
 * It returns 0 if the crate seems to be freshly switched on.
 * It returns -1 if the necessary registers could not be read.
 */
static int
sis1100_remote_init(struct lvd_dev* dev)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=info->A;
    ems_u32 cr;
    int code;

    if (lvdbus_init(dev, 0, 0, &code))
        return -1;

    /* switch on both user LEDs */
    if (sis1100_lvd_mcread(link, ofs(struct lvd_reg, cr), &cr)==plOK)
            sis1100_lvd_mcwrite(link, ofs(struct lvd_reg, cr),
                cr|LVD_CR_LED0|LVD_CR_LED1);

    return code;
}
/*****************************************************************************/
__attribute__((unused)) static const char*
link_is (struct lvd_sis1100_link *link)
{
    struct lvd_sis1100_info *info;
    info=link->dev?(struct lvd_sis1100_info*)link->dev->info:0;
    if (!info)
        return "dangling";
    if (link==info->A)
        return "A";
    if (link==info->B)
        return "B";
    return "strange";
}
/*****************************************************************************/
static int
register_sis1100irq(struct lvd_sis1100_link *link,
        ems_u32 mask, ems_u32 autoack_mask, int noclear)
{
    struct sis1100_irq_ctl2 ctl;

#if 0
    printf("register_sis1100irq(%s; %s) mask=0x%x auto_mask=0x%x\n",
        link->pathname, link_is(link), mask, autoack_mask);
#endif

    if (!mask) {
        printf("register_sis1100irq: mask empty\n");
        return 0;
    }
    ctl.irq_mask=mask;
    ctl.auto_mask=autoack_mask;
    ctl.flags=0;   /* if 0, Pause/Resume does no work with old driver */
    if (noclear)
        ctl.flags|=SIS1100_IRQCTL_NOCLEAR;
    ctl.signal=-1; /* install IRQ but do not send a signal */
    if (ioctl(link->p_ctrl, SIS1100_IRQ_CTL2, &ctl)) {
        printf("register_lvdirq: SIS1100_IRQ_CTL: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}
/*****************************************************************************/
static int
unregister_sis1100irq(struct lvd_sis1100_link *link, ems_u32 mask)
{
    struct sis1100_irq_ctl ctl;

#if 0
    printf("unregister_sis1100irq(%s; %s) mask=0x%x\n",
        link->pathname, link_is(link), mask);
#endif

    if (!mask) {
        printf("unregister_sis1100irq: mask empty\n");
        return 0;
    }
    ctl.signal=0;
    if (ioctl(link->p_ctrl, SIS1100_IRQ_CTL, &ctl)) {
        printf("unregister_lvdirq: SIS1100_IRQ_CTL: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}
/*****************************************************************************/
static int
register_lvdirq(struct lvd_dev* dev, ems_u32 mask, ems_u32 autoack_mask,
    lvdirq_callback callback, void *data, char *procname, int noclear)
{
/*
register_lvdirq installs an interrupt which can be generated by the sis1100
card or (via the doorbell register) initiated from the lvd crate controller.
callback (if !=NULL) is called if the IRQ occures.
*/

    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_irq_callback *cb;
    //ems_u32 cmask;

#if 0
    printf("register_lvdirq: mask=0x%x callback=%p data=%p\n",
            mask, callback, data);
#endif
    /* calculate current mask and check new mask */
    //cmask=0;
    //cb=info->irq_callbacks;
    //while (cb) {
    //    cmask|=cb->mask;
    //    cb=cb->next;
    //}
#if 0
    printf("current mask=0x%x\n", cmask);
#endif
#if 1
    if (mask&info->irq_mask) {
        printf("register_lvdirq: masks overlap\n");
        return -1;
    }
    if (autoack_mask&~mask) {
        printf("register_lvdirq: invalid autoack_mask\n");
        return -1;
    }
#else
    /* masks are allowed to overlap */
    /* when this happens all callbacks for each IRQbit are called */
#endif

    /* register irq with driver */
    if (info->A->priority>0) {
        /* register new bits only */
        register_sis1100irq(info->A, mask&~info->irq_mask,
                autoack_mask&~info->autoack_mask, noclear);
    } else {
        printf("register_lvdirq: no suitable link\n");
    }
    info->irq_mask|=mask;
    info->autoack_mask|=autoack_mask;

    /* find old matching entry (same callback and same data) */
    cb=info->irq_callbacks;
    while (cb) {
        if (cb->callback==callback && cb->data==data)
            break;
        cb=cb->next;
    }
    if (cb) { /* usable old entry found */
        /*printf("old entry found, old mask=0x%x\n", cb->mask);*/
        cb->mask|=mask;
        return 0;
    }

    /* create new entry */
    cb=malloc(sizeof(struct lvd_irq_callback));
    cb->callback=callback;
    cb->mask=mask;
    cb->data=data;
    cb->procname=strdup(procname);
    cb->prev=0;
    cb->next=info->irq_callbacks;
    if (info->irq_callbacks)
        info->irq_callbacks->prev=cb;
    info->irq_callbacks=cb;

    return 0;
}
/*****************************************************************************/
static int
unregister_lvdirq(struct lvd_dev* dev, ems_u32 mask, lvdirq_callback callback)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    //struct lvd_sis1100_link *link=info->A;
    struct lvd_irq_callback *cb;
    //struct sis1100_irq_ctl ctl;
    ems_u32 /*oldmask,*/ newmask;

#if 0
    printf("unregister_lvdirq mask=0x%x, callback %p\n", mask, callback);
#endif

    /* calculate old mask */
    //oldmask=0;
    //cb=info->irq_callbacks;
    //while (cb) {
    //    oldmask|=cb->mask;
    //    cb=cb->next;
    //}

    cb=info->irq_callbacks;
    while (cb) {
        if (cb->callback==callback) {
            cb->mask&=~mask;
            if (!cb->mask) {
                /* invalidate cb */
                cb->callback=0;
#if 0
                printf("unregister_lvdirq: callback %s invalidated\n",
                        cb->procname);
#endif
            }
        }
        cb=cb->next;
    }

    /* calculate new mask */
    newmask=0;
    cb=info->irq_callbacks;
    while (cb) {
#if 0
        printf("callback still active: %s with mask 0x%x\n",
                        cb->procname, cb->mask);
#endif
        newmask|=cb->mask;
        cb=cb->next;
    }

    /* deregister IRQs */
    unregister_sis1100irq(info->A, info->irq_mask&~newmask);

    //ctl.irq_mask=info->irq_mask&~newmask;
    //if (ctl.irq_mask) {
    //    ctl.signal=0;
    //    if (ioctl(link->p_ctrl, SIS1100_IRQ_CTL, &ctl)) {
    //        printf("unregister_lvdirq: SIS1100_IRQ_CTL: %s\n", strerror(errno));
    //        return -1;
    //    }
    //}
    info->irq_mask=newmask;
    info->autoack_mask&=newmask;

    return 0;
}
/*****************************************************************************/
static int
acknowledge_lvdirq(struct lvd_dev* dev, ems_u32 mask)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=info->A;
    struct sis1100_irq_ack ack;

    if (mask&info->autoack_mask) {
        printf("acknowledge_lvdirq mask=%08x auto=%08x\n",
                mask, info->autoack_mask);
        return -1;
    }

    ack.irq_mask=mask;
    if (ioctl(link->p_ctrl, SIS1100_IRQ_ACK, &ack)) {
        printf("acknowledge_lvdirq: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}
/*****************************************************************************/
/*
 * attach_link attaches a new link to dev.
 * info->A always exists but the new link might be 'better' than A.
 * In this case A ist moved to B.
 */
static void
attach_link(struct lvd_dev* dev, struct lvd_sis1100_link *link)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;

printf("attach_link: dev=%s link=%s\n", dev->pathname, link->pathname);

    if (info->B) {
        printf("#### #### #### lvd_attach_link: B is already attached\n");
        return;
    }

    if (!info->A->online) {
        /* we are online and will replace A */
        printf("attach_link: replacing %s by %s (removing the former)\n",
                info->A->pathname, link->pathname);
        info->A->dev=0;
        info->A=link;
        link->dev=dev;
        return;
    } else {
        /* A is online, are we better? */
        if (link->priority>info->A->priority) {
            printf("attach_link: replacing %s by %s (moving the former to B)\n",
                    info->A->pathname, link->pathname);
            info->B=info->A;
            info->A=link;
            link->dev=dev;
        } else {
            printf("attach_link: assigning %s as B (A is %s)\n",
                    link->pathname, info->A->pathname);
            info->B=link;
            link->dev=dev;
        }
    }

    /* now we are attached, enable interrupts? */
    if (link->priority>0) {
        register_sis1100irq(link, info->irq_mask, info->autoack_mask, 0);
    }
}
/*****************************************************************************/
/*
 * detach_link removes a link from dev.
 * Because info->A must always exist ^ is moved to A if A is removed.
 */
static void
detach_link(struct lvd_dev* dev, struct lvd_sis1100_link *link)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;

printf("detach_link: dev=%s link=%s\n", dev->pathname, link->pathname);

    if (!info->B) {/* we cannot remove the last link */
printf("  link was A and cannot be detached\n");
if (link!=info->A) 
    printf("#### #### #### detach_link: link is not A\n");
        return;
    }

    link->dev=0;
    if (link==info->B) {
        info->B=0;
    } else {
        if (link!=info->A) {
            printf("#### #### #### lvd_detach_link: link is neither A nor B\n");
            return;
        }
        info->A=info->B;
        info->B=0;
        /* because 'link' was A we have to disable the LVDS IRQs */
        unregister_sis1100irq(link, info->irq_mask);
    }
}
/*****************************************************************************/
static struct lvd_sis1100_link*
find_matching_link(struct lvd_dev *dev)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *l=linkroot;

    printf("search_matching_link for %s\n", dev->pathname);

    while (l) {
        if (dev->ccard.id==l->controller_id &&
            dev->ccard.serial==l->controller_serial) {
            if (l==info->A) {
                printf("  found myself!\n");
            } else {
                printf("  found %s\n", l->pathname);
                return l;
            }
        }
        l=l->next;
    }
    return 0;
}
/*****************************************************************************/
static struct lvd_dev*
find_matching_dev(struct lvd_sis1100_link *link)
{
    unsigned int i;

    printf("search_matching_dev for %s\n", link->pathname);

    for (i=0; i<numxdevs[modul_lvd]; i++) {
        struct generic_dev *gdev=devices[xdevices[modul_lvd][i]];
        struct lvd_dev *dev=(struct lvd_dev*)gdev;
        if (!dev)
            continue;
        if (dev->ccard.id==link->controller_id &&
                dev->ccard.serial==link->controller_serial) {
            printf("  found %s\n", dev->pathname);
            return dev;
        }
    }
    return 0;
}
/*****************************************************************************/
static void
sis1100_link_up(struct lvd_sis1100_link *link, __attribute__((unused)) struct sis1100_irq_get2* get)
{
    struct lvd_sis1100_info *info;

    info=link->dev?(struct lvd_sis1100_info*)link->dev->info:0;
    link->online=1;

    sis1100_check_ident(link); /* may change link->online */
    if (!link->online) {
        printf("link is up but !online\n");
        return;
    } else {
        printf("link is up and online\n");
    }

    if (link->dev) { /* wir sind A*/
        if (link!=info->A) {
            printf("#### #### #### sis1100_link_up: link is not A\n");
            return;
        }
        sis1100_remote_init(link->dev);
        if (link->priority>0)
            register_sis1100irq(link, info->irq_mask, info->autoack_mask, 0);
        /* here we have to check whether the crate was switched off or not */
    } else {
        struct lvd_dev *dev;
        dev=find_matching_dev(link);
        if (!dev)
            return;
        attach_link(dev, link);
    }
    if (link->online && link->dev)
        link->dev->generic.online=1;
}
/*****************************************************************************/
static void
sis1100_link_down(struct lvd_sis1100_link *link, __attribute__((unused)) struct sis1100_irq_get2* get)
{
    link->online=0;


    if (link->dev) {
        if (link->priority>0) {
            struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)link->dev->info;
            if (link!=info->A) {
                printf("#### #### #### sis1100_link_down: old primary link is not A\n");
            } else {
                unregister_sis1100irq(link, info->irq_mask);
            }
        }
        detach_link(link->dev, link);
    }
    if (!link->online && link->dev)
        link->dev->generic.online=0;
}
/*****************************************************************************/
static void
sis1100_link_up_down(struct lvd_sis1100_link *link,
        struct sis1100_irq_get2* get)
{
/* link_up_down is called from sis1100_irq if get->remote_status!=0
 * it should set link->online to 0
 * if link is up it should
 *     call check_ident
 *     (check_ident will set link->online to 1 if remote type is acceptable
 *     check whether we are the upper link
 * it should reassign info->A and info->B if necessary
 *     (if both links are up --> A will be the upper link)
 *     (if only one link is up it will be A)
 *     (if both links are down or only one link exists nothing is changed)
 * it should set dev->generic.online to A->online
 *
 *  value of code should be:
 *     -1: error
 *      0: power cycled or resetted
 *      1: still initialised
 *      2: different crate or configuration
 */

/*
 * If link is now up and it is an A-link and an assoziated crate is found
 * the A- and B-links of the crate will be switched here.
 * --> result of we_are_1st_link must not be chached!
 */

/*
 * (un)register_sis1100irq:
 * If a link gets up and is a primary link and is attached to a dev
 * then the irqmask of dev will be registered.
 * If a link gets down and was aprimary link and was attached to a dev
 * then the irqmask of dev will be unregistered.
 * 
 * priority==0: secondary link; priority>=1: primary link
 *   the relation priority <-> primary/secondary has to be reconsidered if
 *   more types of links are available
 * 
 * simple solution:
 *     if a link with priority 1 gets down --> unregister IRQs
 *     if a link with priority 1 is attached to a dev --> register IRQs
 */

printf("sis1100_link_up_down: link(%s) is now %s\n",
    link->pathname, get->remote_status>0?"up":"down");


    if (get->remote_status>0)
        sis1100_link_up(link, get);
    else
        sis1100_link_down(link, get);

#if 0


-- ende --

    if (we_are_1st_link(link)) {
        int code=0;

        link->dev->generic.online=link->online;
        if (link->online) {
            code=sis1100_remote_init(link->dev);
            if (code<0) {
                complain("sis1100_link_up_down: error initializing %s",
                        link->pathname);
                link->dev->generic.online=0;
            } else {
                printf("sis1100_link_up_down(%s): ", link->pathname);
                switch (code) {
                case 0:
                    printf("power cycled or resetted\n"); break;
                case 1:
                    printf("still initialised\n"); break;
                case 2:
                    printf("different crate or configuration\n"); break;
                default:
                    printf("unknown code %d\n", code);
                }
            }
        }
        send_unsol_var(Unsol_DeviceDisconnect, 4, modul_lvd,
                link->dev->generic.dev_idx, link->dev->generic.online, code);
    }

#endif
}
/*****************************************************************************/
/*
 * called if (doorbell&LVD_DBELL_LVD_ERR)
 * LVD_DBELL_LVD_ERR==0x3f000000
 */
static void
sis1100_doorbell_err(struct lvd_dev* dev,
        const struct lvdirq_callbackdata *lvd_data,
        __attribute__((unused)) void *data)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_sis1100_link *link=info->A;
    ems_u32 doorbell=lvd_data->mask;
    ems_u32 errpat;

    printf("sis1100_lvd.c: LVD error in %s; doorbell=0x%08x\n",
            dev->pathname, doorbell);

    if (info->ddma_active) {
        /*sis1100_lvd_stop_async(dev);*/
        ioctl(link->p_ctrl, SIS1100_RESET, 0);
    }

    if (doorbell&0x3f000000) { /* readout error */
        if (doorbell&0x3e000000) {
            printf("unexpected doorbell content: 0x%08x\n", doorbell);
        } else { /* must be 0x01000000 */
            if (lvd_ib_r(dev, error, &errpat)<0) {
                printf("    cannot scan crate for error pattern\n");
                errpat=0;
            } else {
                printf("    error pattern (modules): 0x%04x\n", errpat);
            }
        }
    }

#if 0
    lvd_cratestat(dev, 0, 2, -1);
#else
    lvd_cratestat(dev, 0, 3, errpat);
#endif

#if 0
    lvd_clear_status(dev);
#else
    /* disable controller */
    lvd_cc_w(dev, cr, 0); /* slave controllers ignored */
#endif

#if 1
    printf("will acknowledge_lvdirq mask=0x%x\n", doorbell&LVD_DBELL_LVD_ERR);
    acknowledge_lvdirq(dev, doorbell&LVD_DBELL_LVD_ERR);
#endif

#if 1
    if (readout_active==Invoc_active) {
        send_unsol_var(Unsol_RuntimeError, 1, rtErr_InDev);
        fatal_readout_error();
    }
#endif
}
/*****************************************************************************/
static void
sis1100_doorbell(struct lvd_dev* dev, struct sis1100_irq_get2* get)
{
    struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
    struct lvd_irq_callback* cb;
    struct lvdirq_callbackdata lvd_data;
    ems_u32 doorbell=get->irqs;

#if 0
    printf("sis1100_doorbell: doorbell=%08x\n", doorbell);
#endif

    lvd_data.mask=doorbell;
    lvd_data.seq=get->vector;
    lvd_data.time.tv_sec=get->irq_sec;
    lvd_data.time.tv_nsec=get->irq_nsec;

#if 1
    if (lvd_data.seq<0) {
        printf("seq     =%d\n", lvd_data.seq);
        printf("irq_mask=%08x\n", get->irq_mask);
        printf("status  =%08x\n", get->remote_status);
        printf("opt     =%08x\n", get->opt_status);
        printf("mbx0    =%08x\n", get->mbx0);
        printf("irqs    =%08x\n", get->irqs);
        printf("level   =%08x\n", get->level);
        printf("vector  =%08x\n", get->vector);
    }
#endif

    cb=info->irq_callbacks;
    while (cb) {
        struct lvd_irq_callback* next_cb=cb->next;
        if (doorbell&cb->mask) {
            if (cb->callback) {
#if 0
                printf("sis1100_doorbell: calling %s\n", cb->procname);
#endif
                cb->callback(dev, &lvd_data, cb->data);
            } else { /* entry was invalidated */
                if (cb->prev)
                    cb->prev->next=cb->next;
                if (cb->next)
                    cb->next->prev=cb->prev;
                if (cb==info->irq_callbacks)
                    info->irq_callbacks=cb->next;
                printf("sis1100_doorbell: callback %s removed\n", cb->procname);
                free(cb->procname);
                free(cb);
            }
        }
        cb=next_cb;
    }
}
/*****************************************************************************/
static void
sis1100_irq_old(int p)
{
    struct sis1100_irq_get get;
    if (read(p, &get, sizeof(get))==sizeof(get)) {
        printf("sis1100_irq: old driver detected\n");
    }
}
/*****************************************************************************/
#if 0
(definition in sis1100_var.h)
struct sis1100_irq_get2 {
    u_int32_t irq_mask;
    int32_t   remote_status; /* -1: down 1: up 0: unchanged */
    u_int32_t opt_status;
    u_int32_t mbx0;
    u_int32_t irqs;
    int32_t level;
    int32_t vector;
    /* the fields above this line have to match sis1100_irq_get */
    u_int32_t irq_sec;  /* time of last Interrupt (seconds)     */
    u_int32_t irq_nsec; /*                        (nanoseconds) */
};
#endif
static void
sis1100_irq(int p, __attribute__((unused)) enum select_types selected, union callbackdata data)
{
    //struct generic_dev* gdev=(struct generic_dev*)data.p;
    //struct lvd_dev* dev=(struct lvd_dev*)gdev;
    struct lvd_sis1100_link *link=(struct lvd_sis1100_link*)data.p;
    struct sis1100_irq_get2 get;
    int res;

    res=read(p, &get, sizeof(get));
    if (res!=sizeof(get)) {
        //struct lvd_sis1100_info *info=(struct lvd_sis1100_info*)dev->info;
        printf("lvd/sis1100lvd_irq: res=%d errno=%s\n", res, strerror(errno));
        /* check for old driver version */
        sis1100_irq_old(p);
printf("call sched_delete_select_task(%p)\n", link->st);
        sched_delete_select_task(link->st);
        link->st=0;
        printf("sis1100_irq(%s): IRQs removed\n", link->pathname);
        return;
    }

#if 0
    printf("sis1100_doorbell:\n");
    printf("  irq_mask     =0x%x\n", get.irq_mask);
    printf("  remote_status=  %d\n", get.remote_status);
    printf("  opt_status   =0x%x\n", get.opt_status);
    printf("  mbx0         =0x%x\n", get.mbx0);
    printf("  irqs         =0x%x\n", get.irqs);
    printf("  level        =0x%x\n", get.level);
    printf("  vector       =0x%x\n", get.vector);
    printf("  irq_sec      =  %d\n", get.irq_sec);
    printf("  irq_nsec     =  %d\n", get.irq_nsec);
#endif

    if (get.remote_status) {
        sis1100_link_up_down(link, &get);
    }

    if (get.irqs) {
        if (link->dev)
            sis1100_doorbell(link->dev, &get);
        else
            printf("sis1100_irq: dev=0\n");
    }
}
/*****************************************************************************/
void
init_ra_statist(struct ra_statist *ra_st)
{
    ra_st->blocks=0;
    ra_st->events=0;
    ra_st->fragments=0;
    ra_st->words=0;
    ra_st->max_evsize=0;
}
/*****************************************************************************/
static errcode
lvd_init_link(struct lvd_sis1100_link *link)
{
    struct sis1100_irq_ctl ctl;
    int dmalen[2]={1, 1};
    int res=OK;

    printf("lvd_init_link(%s) link=%p\n", link->pathname, link);

    link->p_ctrl=-1;
    link->p_remote=-1;

    if ((link->p_ctrl=sis1100_lvd_open_dev(link->pathname,
            sis1100_subdev_ctrl))<0) {
        res=Err_System;
        goto error;
    }

    if ((link->p_remote=sis1100_lvd_open_dev(link->pathname,
            sis1100_subdev_remote))<0) {
        res=Err_System;
        goto error;
    }
    print_offline(link, "lvd_init_link");

    if (sis1100_lvd_mmap_init(link)<0) {
        res=Err_System;
        goto error;
    }

    if (ioctl(link->p_remote, SIS1100_MINDMALEN, dmalen)) {
        printf("SIS1100_MINDMALEN: %s\n", strerror(errno));
        res=Err_System;
        goto error;
    }

    /* request IRQ for link up/down */
    ctl.irq_mask=0;
    ctl.signal=-1;
    if (ioctl(link->p_ctrl, SIS1100_IRQ_CTL, &ctl)) {
        printf("lvd_init_sis1100: SIS1100_IRQ_CTL: %s\n", strerror(errno));
        res=Err_System;
        goto error;
    } else {
        union callbackdata cbdata;
        cbdata.p=link;
        printf("lvd_init_sis1100: installing seltask %p cbdata.p=%p\n",
                sis1100_irq, cbdata.p);
        link->st=sched_insert_select_task(sis1100_irq, cbdata,
            "sis1100lvd_irq", link->p_ctrl, select_read
#ifdef SELECT_STATIST
            , 1
#endif
            );
        if (!link->st) {
            printf("lvd_init_sis1100: cannot install callback for "
                    "sis1100_irq\n");
            res=Err_System;
            goto error;
        }
    }

    sis1100_check_ident(link);

    /* link into linked list */
    if (linkroot)
        linkroot->prev=link;
    link->prev=0;
    link->next=linkroot;
    linkroot=link;

    return OK;

error:
    close(link->p_ctrl);
    close(link->p_remote);
    return res;
}
/*****************************************************************************/
errcode
lvd_init_sis1100(struct lvd_dev* dev)
{
    struct lvd_sis1100_info *info;
    errcode res;

    printf("== %s lvd_init_sis1100: path=%s ==\n", nowstr(), dev->pathname);

    if (lvd_decode_check_integrity()!=0) {
        printf("integrity check for lvd decode failed!\n");
        res=Err_Program;
        goto error;
    }

    info=(struct lvd_sis1100_info*)calloc(1, sizeof(struct lvd_sis1100_info));
    if (!info) {
        printf("lvd_init_sis1100: calloc(info): %s\n", strerror(errno));
        return Err_NoMem;
    }
    dev->info=info;
    info->ddma_active=0;
    info->irq_callbacks=0;
    info->irq_mask=0;
    info->autoack_mask=0;

    info->A=(struct lvd_sis1100_link*)calloc(1, sizeof(struct lvd_sis1100_link));
    if (!info->A) {
        printf("lvd_init_sis1100: calloc(link): %s\n", strerror(errno));
        return Err_NoMem;
    }
    info->A->pathname=strdup(dev->pathname);
    info->A->dev=dev;
    info->B=0;
    res=lvd_init_link(info->A);
    if (res!=OK)
        return res;
    dev->generic.online=info->A->online;
    if (dev->generic.online) {
        dev->ccard.id=info->A->controller_id;
        dev->ccard.serial=info->A->controller_serial;
    } else {
        dev->ccard.id=0;
        dev->ccard.serial=0;
    }

    lvd_settrace(dev, 0, 0);
    dev->silent_errors=0;
    dev->silent_regmanipulation=0;
    dev->parseflags=0;
    dev->statist_interval=0;
    //dev->num_headerwords=0;
    //dev->headerwords=0;
    dev->datafilter=0;

    dev->generic.done=lvd_done_sis1100;
    dev->generic.init_jtag_dev=lvd_init_jtag_dev;
    dev->write=sis1100_lvd_write;
    dev->read=sis1100_lvd_read;
    dev->write_block=sis1100_lvd_write_block;
    dev->read_block=sis1100_lvd_read_block;
    dev->localreset=sis1100_lvd_localreset;
    dev->write_blind=sis1100_lvd_write_blind;

    dev->prepare_single=sis1100_lvd_prepare_single;
    dev->start_single=sis1100_lvd_start_single;
    dev->readout_single=sis1100_lvd_readout_single;
    dev->stop_single=sis1100_lvd_stop_single;
    dev->cleanup_single=sis1100_lvd_cleanup_single;

    dev->prepare_async=sis1100_lvd_prepare_async;
    dev->start_async=sis1100_lvd_start_async;
    dev->readout_async=sis1100_lvd_readout_async;
    dev->stop_async=sis1100_lvd_stop_async;
    dev->cleanup_async=sis1100_lvd_cleanup_async;

    dev->statist=sis1100_lvd_statist;

    dev->register_lvdirq=register_lvdirq;
    dev->unregister_lvdirq=unregister_lvdirq;
    dev->acknowledge_lvdirq=acknowledge_lvdirq;

    info->plxwrite=sis1100_lvd_plxwrite_;
    info->plxread=sis1100_lvd_plxread_;
    info->siswrite=sis1100_lvd_siswrite_;
    info->sisread=sis1100_lvd_sisread_;
    info->mcwrite=sis1100_lvd_mcwrite_;
    info->mcread=sis1100_lvd_mcread_;

    info->sis_stat=sis1100_sis_stat;
    info->mc_stat=sis1100_mc_stat;
    info->plx_stat=sis1100_plx_stat;
    info->link_stat=sis1100_link_stat;

    init_ra_statist(&info->ra.statist);
    init_ra_statist(&info->ra.old_statist);

    if (dev->generic.online) {
        struct lvd_sis1100_link *link2;
    print_offline(info->A, "lvd_init_sis1100");
        link2=find_matching_link(dev);
        if (link2)
            attach_link(dev, link2); /* attach_link may swap A and B */

        printf("lvd_init_sis1100(%s): device online\n", dev->pathname);
        printf("  link A (%s) online\n", info->A->pathname);
        if (info->B)
            printf("  link B (%s) o%sline\n", info->B->pathname,
                    info->B->online?"n":"ff");
        else
            printf("  link B does not exist\n");

        if (sis1100_remote_init(dev)<0) {
            res=Err_System;
            goto error;
        }
    } else {
        printf("lvd_init_sis1100(%s): device offline\n", dev->pathname);
    }

    if (register_lvdirq(dev, LVD_DBELL_LVD_ERR, 0, sis1100_doorbell_err,
            0, "sis1100_doorbell_err", 0)<0) {
        printf("lvd_init_sis1100: Cannot install callback for lvd errors!\n");
        res=Err_System;
        goto error;
    }

    return OK;

error:
    lvd_done_sis1100((struct generic_dev*)dev);
    return res;
}
/*****************************************************************************/
errcode
lvd_init_sis1100X(struct lvd_dev* dev)
{
    struct lvd_sis1100_link *link;

    printf("lvd_init_sis1100X called with path %s\n", dev->pathname);

    /* dev will be deleted after return from this procedure
     * therefore we have to store the information somewhere else */

    link=(struct lvd_sis1100_link*)calloc(1, sizeof(struct lvd_sis1100_link));
    /* we can 'steal' pathname from dev because dev is deleted soon */
    link->pathname=dev->pathname;
    dev->pathname=0;
    link->dev=0;
    lvd_init_link(link);

//    find a 'partner' in the list of devices
//    if not found: put us in a linked list of helper objects

    /* if there is no link: park the information somewhere */

    return OK;
}
/*****************************************************************************/
void sis1100_init(void)
{
    linkroot=0;
}
/*****************************************************************************/
void sis1100_done(void)
{
    while (linkroot) {
        printf("deleting dangling link %s\n", linkroot->pathname);
        lvd_done_link(linkroot);
    }
}
/*****************************************************************************/
/*****************************************************************************/
