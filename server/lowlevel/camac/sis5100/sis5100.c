/*
 * lowlevel/camac/sis5100/sis5100.c
 * created: 05.Apr.2004 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis5100.c,v 1.21 2017/10/21 23:12:04 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dev/pci/sis1100_var.h>
#include <dev/pci/sis1100_map.h>
#include <dev/pci/sis5100_map.h>

#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include <swap.h>
#include "../camac.h"
#include "../camac_init.h"
#include "sis5100camac.h"
#include "../../jtag/jtag_lowlevel.h"

#ifdef SIS5100_MMAPPED
#include <sys/mman.h>
#endif

RCS_REGISTER(cvsid, "lowlevel/camac/sis5100")

/*****************************************************************************/
#ifdef SIS5100_MMAPPED
static camadr_t
sis5100_CAMACaddr(int n, int a, int f)
{
    camadr_t addr;
    addr.sis5100.naf=(f<<9)+(n<<4)+(a); /* base is ems_u32* !! */
    return addr;
}
#else
static camadr_t
sis5100_CAMACaddr(int n, int a, int f)
{
    camadr_t addr;
    addr.sis5100.n=n;
    addr.sis5100.a=a;
    addr.sis5100.f=f;
    return addr;
}
#endif
/*****************************************************************************/
/*
 * WARNING!
 * return value ist statically allocated
 * it is only valid until the next call of CAMACaddrP
 */
#ifdef SIS5100_MMAPPED
static camadr_t*
sis5100_CAMACaddrP(int n, int a, int f)
{
    static camadr_t addr;
    addr.sis5100.naf=(f<<9)+(n<<4)+(a); /* base is ems_u32* !! */
    return &addr;
}
#else
static camadr_t*
sis5100_CAMACaddrP(int n, int a, int f)
{
    static camadr_t addr;
    addr.sis5100.n=n;
    addr.sis5100.a=a;
    addr.sis5100.f=f;
    return &addr;
}
#endif
/*****************************************************************************/
#ifdef SIS5100_MMAPPED
static void
sis5100_CAMACinc(camadr_t* addr)
{
    addr->sis5100.naf+=4;
}
#else
static void
sis5100_CAMACinc(camadr_t* addr)
{
    if(++addr->sis5100.a >= 16) {
	addr->sis5100.a = 0;
	++addr->sis5100.n;
    }
}
#endif

/*
 * QX:
 * read:
 *     in value:
 *         !Q: 0x80000000
 *         !X: 0x40000000
 * write and cntl:
 *     int error:
 *         !Q: 0x280
 *         !X: 0x240
 */

/*****************************************************************************/
static ems_u32
sis5100_CAMACval(ems_u32 val)
{
    return val&0xffffff;
}
/*****************************************************************************/
static ems_u32
sis5100_CAMACgotQ(ems_u32 val)
{
    return val&0x80000000;
}
/*****************************************************************************/
static ems_u32
sis5100_CAMACgotX(ems_u32 val)
{
    return val&0x40000000;
}
/*****************************************************************************/
static ems_u32
sis5100_CAMACgotQX(ems_u32 val)
{
    return val&0xC0000000;
}
/*****************************************************************************/
#ifdef SIS5100_MMAPPED
static int
sis5100_CAMACread(struct camac_dev* dev, camadr_t* addr, ems_u32* val)
{
    struct camac_sis5100_info *info=(struct camac_sis5100_info*)dev->info;
    ems_u32 error;

    *val=info->camacstatus=*(info->camac_base+addr->sis5100.naf)^0xc0000000;
    error=info->ctrl_base->prot_error;
    if (!error)
        return 0;

    while (error==sis1100_e_dlock) {
        error=info->ctrl_base->prot_error;
    }

    if (error==sis1100_le_dlock) { /* PCI timeout */
        ems_u32 head;
        head=info->ctrl_base->tc_hdr;
        if ((head&0x300)!=0x300) { /* not an error confirmation */
            *val=info->camacstatus=info->ctrl_base->tc_dal^0xc0000000;
            return 0;
        }
        error=((head>>24)&0xff)|0x200;
    }

    if (error==sis1100_le_to)
        info->ctrl_base->p_balance=0;
    printf("sis5100_CAMACread: game over! error=0x%x\n", error);
    return -1;
}
#else
static int
sis5100_CAMACread(struct camac_dev* dev, camadr_t* addr, ems_u32* val)
{
    struct camac_sis5100_info *info=(struct camac_sis5100_info*)dev->info;
    struct sis1100_camac_req c;
    int res;

    c.N = addr->sis5100.n;
    c.A = addr->sis5100.a;
    c.F = addr->sis5100.f;
    res=ioctl(info->camac_path, SIS5100_CNAF, &c);

    *val=info->camacstatus=c.data^0xc0000000;
#if 0
    printf("CAMACread(%2d %2d %2d): error=0x%03x camacstatus=0x%08x val=0x%08x\n",
            c.N, c.A, c.F,
            c.error,
            info->camacstatus,
            *val);
#endif
    return res;
}
#endif
/*****************************************************************************/
#ifdef SIS5100_MMAPPED
static int
sis5100_CAMACcntl(struct camac_dev* dev, camadr_t* addr, ems_u32* stat)
{
    struct camac_sis5100_info* info=(struct camac_sis5100_info*)dev->info;
    ems_u32 error;

    *(info->camac_base+addr->sis5100.naf)=0;
    do {
        error=info->ctrl_base->prot_error;
    } while (error==sis1100_e_dlock);

    if (!error || (error&0x200)) {
        *stat=info->camacstatus=(error<<24)^0xc0000000;
        return 0;
    }

    if (error==sis1100_le_to)
        info->ctrl_base->p_balance=0;
    printf("sis5100_CAMACcntl: game over! error=0x%x\n", error);
    return -1;
}
#else
static int
sis5100_CAMACcntl(struct camac_dev* dev, camadr_t* addr, ems_u32* stat)
{
    struct camac_sis5100_info* info=(struct camac_sis5100_info*)dev->info;
    struct sis1100_camac_req c;
    int res;

    c.N = addr->sis5100.n;
    c.A = addr->sis5100.a;
    c.F = addr->sis5100.f;
    res=ioctl(info->camac_path, SIS5100_CNAF, &c);
    *stat=info->camacstatus=(c.error<<24)^0xc0000000;
#if 0
    printf("CAMACcntl(%d %d %d): error=0x%03x camacstatus=0x%08x stat=0x%08x\n",
            c.N, c.A, c.F,
            c.error,
            info->camacstatus,
            *stat);
#endif
    return res;
}
#endif
/*****************************************************************************/
#ifdef SIS5100_MMAPPED
static int
sis5100_CAMACwrite(struct camac_dev* dev, camadr_t* addr, ems_u32 val)
{
    struct camac_sis5100_info* info=(struct camac_sis5100_info*)dev->info;
    ems_u32 error;

    *(info->camac_base+addr->sis5100.naf)=val;
    do {
        error=info->ctrl_base->prot_error;
    } while (error==sis1100_e_dlock);

    if (!error || (error&0x200)) {
        info->camacstatus=(error<<24)^0xc0000000;
        return 0;
    }

    if (error==sis1100_le_to)
        info->ctrl_base->p_balance=0;
    printf("sis5100_CAMACwrite: game over! error=0x%x\n", error);
    return -1;
}
#else
static int
sis5100_CAMACwrite(struct camac_dev* dev, camadr_t* addr, ems_u32 val)
{
    struct camac_sis5100_info* info=(struct camac_sis5100_info*)dev->info;
    struct sis1100_camac_req c;
    int res;

    c.N = addr->sis5100.n;
    c.A = addr->sis5100.a;
    c.F = addr->sis5100.f;
    c.data = val;
    res=ioctl(info->camac_path, SIS5100_CNAF, &c);
    info->camacstatus=(c.error<<24)^0xc0000000;
#if 0
    printf("CAMACwrite(%d %d %d): error=0x%03x camacstatus=0x%08x\n",
            c.N, c.A, c.F,
            c.error,
            info->camacstatus);
#endif
    return res;
}
#endif
/*****************************************************************************/
static plerrcode
sis5100_camcontrol(struct camac_dev* dev, int space, int rw, ems_u32 offs,
            ems_u32 *val)
{
    struct camac_sis5100_info* info=(struct camac_sis5100_info*)dev->info;
    struct sis1100_ctrl_reg r;
    int res=0;

    r.offset=offs;
    r.val=*val;
    r.error=0;

    switch (space) {
    case 0: /* PLX */
        if (rw&2)
            res=ioctl(info->ctrl_path, SIS1100_PLX_WRITE, &r);
        if (rw&1)
            res|=ioctl(info->ctrl_path, SIS1100_PLX_READ, &r);
        break;
    case 1: /* ADD_ON */
        if (rw&2)
            res=ioctl(info->ctrl_path, SIS1100_CTRL_WRITE, &r);
        if (rw&1)
            res|=ioctl(info->ctrl_path, SIS1100_CTRL_READ, &r);
        break;
    case 2: /* CAMAC */
        if (rw&2)
            res=ioctl(info->camac_path, SIS1100_CTRL_WRITE, &r);
        if (rw&1)
            res|=ioctl(info->camac_path, SIS1100_CTRL_READ, &r);
        break;
    default:
        return plErr_ArgRange;
    }

    *val=r.val;

    return res?plErr_System:plOK;
}
/*****************************************************************************/
#ifdef SIS5100_MMAPPED
static int
sis5100_CCCC(struct camac_dev* dev)
{
    ems_u32 stat;

    camadr_t addr=sis5100_CAMACaddr(28, 8, 26);
    return sis5100_CAMACcntl(dev, &addr, &stat);
}
#else
static int
sis5100_CCCC(struct camac_dev* dev)
{
    struct camac_sis5100_info* info=(struct camac_sis5100_info*)dev->info;
    return ioctl(info->camac_path, SIS5100_CCCC);
}
#endif
/*****************************************************************************/
#ifdef SIS5100_MMAPPED
static int
sis5100_CCCZ(struct camac_dev* dev)
{
    ems_u32 stat;

    camadr_t addr=sis5100_CAMACaddr(28, 9, 26);
    return sis5100_CAMACcntl(dev, &addr, &stat);
}
#else
static int
sis5100_CCCZ(struct camac_dev* dev)
{
    struct camac_sis5100_info* info=(struct camac_sis5100_info*)dev->info;
    return ioctl(info->camac_path, SIS5100_CCCZ);
}
#endif
/*****************************************************************************/
#ifdef SIS5100_MMAPPED
static int
sis5100_CCCI(struct camac_dev* dev, int val)
{
    struct camac_sis5100_info* info=(struct camac_sis5100_info*)dev->info;
    info->ctrl_base_rem->camac_sc=val?1:0x10000;
    return 0;
}
#else
static int
sis5100_CCCI(struct camac_dev* dev, int val)
{
    struct camac_sis5100_info* info=(struct camac_sis5100_info*)dev->info;
    return ioctl(info->camac_path, SIS5100_CCCI, &val);
}
#endif
/*****************************************************************************/
#ifdef SIS5100_MMAPPED
static int
sis5100_CAMAClams(struct camac_dev* dev, ems_u32* val)
{
    struct camac_sis5100_info* info=(struct camac_sis5100_info*)dev->info;
    /* CAMACread(n=30, a=0, f=0) */
    *val=*(info->camac_base+(30<<6)+(0<<2)+(0<<11));
    return 0;
}
#else
static int
sis5100_CAMAClams(struct camac_dev* dev, ems_u32* val)
{
/*    struct camactransfer c;
    int res;

    c.n = 30;
    c.a = 0;
    c.f = 0;
    res=ioctl(info->p, NAFREAD, &c);
    *val=c.u.val;
    return res;
*/
    return 0;
}
#endif
/*****************************************************************************/
static int
sis5100_CAMACenable_lams(__attribute__((unused)) struct camac_dev* dev,
        __attribute__((unused)) int idx)
{
/*
    info->lammask |= 1<<(idx-1);
*/
    return 0;
}
/*****************************************************************************/
static int
sis5100_CAMACdisable_lams(__attribute__((unused)) struct camac_dev* dev,
        __attribute__((unused)) int idx)
{
/*
    info->lammask &= ~(1<<(idx-1));
*/
    return 0;
}
/*****************************************************************************/
static int
sis5100_CAMACstatus(struct camac_dev* dev, ems_u32* val)
{
    struct camac_sis5100_info* info=(struct camac_sis5100_info*)dev->info;
    *val=info->camacstatus;
    return 0;
}
/*****************************************************************************/
static int
sis5100_CAMACblread(struct camac_dev* dev, camadr_t* addr, int num, ems_u32* dest)
{
    int res=0;
    T(CAMACblread)

    while (!res && num--) {
        res=sis5100_CAMACread(dev, addr, dest++);
    }
    return res;
}
/*****************************************************************************/
static int
sis5100_CAMACblread16(struct camac_dev* dev, camadr_t* addr, int num, ems_u32* dest)
{
    int res=0;
    T(CAMACblread)

    while (!res && num--) {
        res=sis5100_CAMACread(dev, addr, dest++);
    }
    return /*res*/-1;
}
/*****************************************************************************/
static int
sis5100_CAMACblreadQstop(struct camac_dev* dev, camadr_t* addr,
        int num, ems_u32* dest)
{
    int res=0, n=0;
    T(CAMACblreadQstop)

    while (!res && num--) {
        ems_u32 val;
        res=sis5100_CAMACread(dev, addr, &val);
        if (sis5100_CAMACgotQ(val)) {
            *dest++=val;
            n++;
        } else {
            break;
        }
    }
    return res?-1:n;
}
/*****************************************************************************/
static int
sis5100_CAMACblreadAddrScan(struct camac_dev* dev, camadr_t* addr,
        int num, ems_u32* dest)
{
    int res=0;
    T(CAMACblreadAddrScan)

    while (!res && num--) {
	res=sis5100_CAMACread(dev, addr, dest++);
    	sis5100_CAMACinc(addr);
    }
    return res;
}
/*****************************************************************************/
static int
sis5100_CAMACblwrite(struct camac_dev* dev, camadr_t* addr, int num, ems_u32* src)
{
    int res=0;
    T(CAMACblwrite)

    while (!res && num--) {
        res=sis5100_CAMACwrite(dev, addr, *src++);
    }
    return res;
}
/*****************************************************************************/
#if 0
static int
sis5100_CAMACblread16(struct camac_dev* dev, camadr_t addr, int num, int* dest)
/* Byteordering inkonsistent ??? */
{
    int *save;
    int dummy;

    T(CAMACblread16)
    save = dest;

    dummy = num;
    while(num--)
	*((short*)dest)++ = (short)sis5100_CAMACread(dev, addr);
    if(dummy & 1) *((short*)dest)++ = 0;

    swap_words_falls_noetig(save, dest - save);
    return num;
}
#endif
/*****************************************************************************/
static int
sis5100_set_user_led(struct camac_sis5100_info* info, int on)
{
    struct sis1100_ctrl_reg req;
    int res;

    req.offset=0x100;
    req.val=0x80;
    req.error=0;
    if (!on)
        req.val<<=16;

    res=ioctl(info->camac_path, SIS1100_CTRL_WRITE, &req);
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
sis5100_get_user_led(struct camac_sis5100_info* info, int* on)
{
    struct sis1100_ctrl_reg req;
    int res;

    req.offset=0x100;

    res=ioctl(info->camac_path, SIS1100_CTRL_READ, &req);
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
static int
sis5100_camacdelay(struct camac_dev* dev, int delay, int* olddelay)
{
    *olddelay=dev->camdelay;
    if (delay>=0)
        dev->camdelay=delay;
    return 0;
}
/*****************************************************************************/
#ifdef DELAYED_READ
static int
sis5100_enable_delayed_read(__attribute__((unused)) struct generic_dev* dev,
        __attribute__((unused)) int val)
{
    return 0;
}

static void sis5100_reset_delayed_read(
        __attribute__((unused)) struct generic_dev* gdev)
{}

static int
sis5100_read_delayed(__attribute__((unused)) struct generic_dev* gdev)
{
    return -1;
}
#endif
/*****************************************************************************/
static int
sis1100_remote_write(struct camac_dev* dev, int size,
            int am, ems_u32 addr, ems_u32 data, ems_u32* error)
{
    struct camac_sis5100_info* info=(struct camac_sis5100_info*)dev->info;
    struct sis1100_vme_req req;
    int res;

    req.size=size;
    req.am=am;
    req.addr=addr;
    req.data=data;
    req.error=0xa5a5a5a5;

    printf("sis5100_remote_write(addr=0x%08x size=%d am=0x%02x val=0x%08x)",
            req.addr, req.size, req.am, req.data);

    res=ioctl(info->camac_path, SIS3100_VME_WRITE, &req);
    if (res) {
        printf(": %s\n", strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf(": error=0x%x\n", req.error);
    } else {
        printf("\n");
    }
    *error=req.error;
    return 0;
}
/*****************************************************************************/
static int
sis1100_remote_read(struct camac_dev* dev, int size,
            int am, ems_u32 addr, ems_u32* data, ems_u32* error)
{
    struct camac_sis5100_info* info=(struct camac_sis5100_info*)dev->info;
    struct sis1100_vme_req req;
    int res;

    req.size=size;
    req.am=am;
    req.addr=addr;
    req.error=0xa5a5a5a5;

    printf("sis5100_remote_read(addr=0x%08x size=%d am=0x%02x): ",
            req.addr, req.size, req.am);
    res=ioctl(info->camac_path, SIS3100_VME_READ, &req);
    if (res) {
        printf("%s\n", strerror(errno));
        return -errno;
    }
    if (req.error) {
        printf("error 0x%x\n", req.error);
        return -EIO;
    } else {
        printf("0x%x\n", req.data);
    }
    *data=req.data;
    *error=req.error;
    return 0;
}
/*****************************************************************************/
static int
sis1100_local_write(struct camac_dev* dev, int offset,
            ems_u32 val, ems_u32* error)
{
    struct camac_sis5100_info* info=(struct camac_sis5100_info*)dev->info;
    struct sis1100_ctrl_reg reg;
    int res;

    reg.offset=offset;
    reg.val=val;
    reg.error=0xa5a5a5a5;

    printf("sis5100_local_write(offset=0x%08x val=0x%08x)",
            reg.offset, reg.val);

    res=ioctl(info->ctrl_path, SIS1100_CTRL_WRITE, &reg);
    if (res) {
        printf(": %s\n", strerror(errno));
        return -errno;
    }
    if (reg.error) {
        printf(": error=0x%x\n", reg.error);
    } else {
        printf("\n");
    }
    *error=reg.error;
    return 0;
}
/*****************************************************************************/
static int
sis1100_local_read(struct camac_dev* dev, int offset,
            ems_u32* val, ems_u32* error)
{
    struct camac_sis5100_info* info=(struct camac_sis5100_info*)dev->info;
    struct sis1100_ctrl_reg reg;
    int res;

    reg.offset=offset;
    reg.error=0xa5a5a5a5;

    printf("sis5100_local_read(offset=0x%08x): ", reg.offset);

    res=ioctl(info->ctrl_path, SIS1100_CTRL_READ, &reg);
    if (res) {
        printf("%s\n", strerror(errno));
        return -errno;
    }
    if (reg.error) {
        printf("error=0x%x\n", reg.error);
    } else {
        printf("0x%08x\n", reg.val);
    }
    *val=reg.val;
    *error=reg.error;
    return 0;
}
/*****************************************************************************/
static struct camac_raw_procs sis5100_raw_procs={
    sis1100_remote_write,
    sis1100_remote_read,
    sis1100_local_write,
    sis1100_local_read,
};

static struct camac_raw_procs*
sis5100_get_raw_procs(__attribute__((unused)) struct camac_dev* dev)
{
    return &sis5100_raw_procs;
}

#ifdef ZELFERA
static struct FERA_procs*
sis5100_get_FERA_procs(__attribute__((unused)) struct camac_dev* dev)
{
    return 0;
}
#endif
/*****************************************************************************/
static void
sis5100_camacirq(__attribute__((unused)) struct camac_dev* dev,
        __attribute__((unused)) struct sis1100_irq_get* get)
{
    printf("SIS5100 IRQ (not yet implemeted)\n");
}
/*****************************************************************************/
static int
sis5100_check_ident(struct camac_dev* dev)
{
    struct camac_sis5100_info* info=(struct camac_sis5100_info*)dev->info;
    struct sis1100_ident ident;

    if (ioctl(info->ctrl_path, SIS1100_IDENT, &ident)<0) {
        printf("sis5100_check_ident: fcntl(p_ctrl, IDENT): %s\n",
                strerror(errno));
        dev->generic.online=0;
        return -1;
    }
    printf("camac_init_sis5100 local ident:\n");
    printf("  hw_type   =%d\n", ident.local.hw_type);
    printf("  hw_version=%d\n", ident.local.hw_version);
    printf("  fw_type   =%d\n", ident.local.fw_type);
    printf("  fw_version=%d\n", ident.local.fw_version);
    if (ident.remote_ok) {
        printf("camac_init_sis5100 remote ident:\n");
        printf("  hw_type   =%d\n", ident.remote.hw_type);
        printf("  hw_version=%d\n", ident.remote.hw_version);
        printf("  fw_type   =%d\n", ident.remote.fw_type);
        printf("  fw_version=%d\n", ident.remote.fw_version);
    }

    if (ident.remote_online) {
        if (ident.remote.hw_type==sis1100_hw_camac) {
            dev->generic.online=1;
        } else {
            printf("camac_init_sis5100: Wrong hardware connected!\n");
            dev->generic.online=0;
        }
    } else {
        printf("camac_init_sis5100: NO remote Interface!\n");
        dev->generic.online=0;
    }
    return 0;
}
/*****************************************************************************/
static void
sis5100_link_up_down(struct camac_dev* dev, struct sis1100_irq_get* get)
{
    struct camac_sis5100_info* info=(struct camac_sis5100_info*)dev->info;
    int crate=dev->generic.dev_idx;
    int initialised=-1;
    int up;

    up=get->remote_status>0;
    printf("sis1100: CAMAC crate %d is %s\n",dev->generic.dev_idx, up?"up":"down");
    initialised=0;
    if (up) {
        sis5100_check_ident(dev);
        if (dev->generic.online) {
            int on, res;
            res=sis5100_get_user_led(info, &on);
            if (!res && on)
                initialised=1;
            printf("sis5100: it is %s initialised\n",
                initialised?"still":"not any more");
        }
    } else {
        dev->generic.online=0;
    }
    send_unsol_var(Unsol_DeviceDisconnect, 4, modul_camac,
        crate, get->remote_status, initialised);
}
/*****************************************************************************/
static void
sis5100_irq(int p, __attribute__((unused)) enum select_types selected,
        union callbackdata data)
{
    struct generic_dev* gendev=(struct generic_dev*)data.p;
    struct camac_dev* dev=(struct camac_dev*)gendev;
    /*struct camac_sis5100_info* info=(struct camac_sis5100_info*)dev->info;*/
    struct sis1100_irq_get get;
    int res;

    res=read(p, &get, sizeof(struct sis1100_irq_get));
    if (res!=sizeof(struct sis1100_irq_get)) {
        printf("camac/sis5100/sis5100.c:sis5100_irq: res=%d errno=%s\n",
                res, strerror(errno));
        return;
    }

    if (get.remote_status) {
        sis5100_link_up_down(dev, &get);
    }

    if (get.irqs) {
        sis5100_camacirq(dev, &get);
    }

#if 0
    {
        static int max=10;
        if (max>0) {
            max--;
        } else {
            struct vme_sis_info* info=&vmedev->info.sis3100;
            struct sis1100_irq_ctl ctl;
            ctl.irq_mask=0xffffffff;
            ctl.signal=0;
            printf("sis3100_irq: disabling IRQs\n");
            if (ioctl(info->p_ctrl, SIS1100_IRQ_CTL, &ctl))
                printf("SIS1100_IRQ_CTL: %s\n", strerror(errno));
        } 
    }
#endif
}
/*****************************************************************************/
static errcode done_sis5100(struct generic_dev* gdev)
{
    struct camac_dev* dev=(struct camac_dev*)gdev;
    struct camac_sis5100_info* info=(struct camac_sis5100_info*)dev->info;
    printf("done_sis5100\n");

    if (info->st) {
        sched_delete_select_task(info->st);
    }

#ifdef SIS5100_MMAPPED
    if (info->camac_base)
            munmap((void*)(info->camac_base), 0x10000);
    if (info->ctrl_base)
            munmap((void*)(info->ctrl_base), 0x1000);
#endif

    close(info->camac_path);
    close(info->ctrl_path);
    close(info->ram_path);
    close(info->dsp_path);

    if (dev->pathname)
        free(dev->pathname);
    dev->pathname=0;

    free(info);
    dev->info=0;
    return OK;
}
/*****************************************************************************/
errcode camac_init_sis5100(struct camac_dev* dev,
        __attribute__((unused)) char* rawmempart)
{
    struct camac_sis5100_info* info;
    struct sis1100_irq_ctl ctl;

    T(camac_init_sis5100)
    printf("camac_init_sis5100(%s)\n", dev->pathname);

    info=(struct camac_sis5100_info*)malloc(sizeof(struct camac_sis5100_info));
    if (!info) {
        printf("camac_init_sis5100: calloc(info): %s\n", strerror(errno));
        return Err_NoMem;
    }
    dev->info=info;

    info->camac_path=-1;
    info->ram_path=-1;
    info->ctrl_path=-1;
    info->dsp_path=-1;

#ifdef SIS5100_MMAPPED
    info->mem_size=0;
    info->dsp_size=0;
    info->pipe_size=0;
#endif

    info->st=0;

    if (sis5100_open_devices(info, dev->pathname)<0) {
        done_sis5100((struct generic_dev*)dev);
        return Err_System;
    }

    if (info->camac_path<0) {
        printf("no CAMAC path for %s open\n", dev->pathname);
        done_sis5100((struct generic_dev*)dev);
        return Err_System;
    }
    if (info->ctrl_path<0) {
        printf("no CONTROL path for %s open\n", dev->pathname);
        done_sis5100((struct generic_dev*)dev);
        return Err_System;
    }

    ctl.irq_mask=0;
    ctl.signal=-1;
    if (ioctl(info->ctrl_path, SIS1100_IRQ_CTL, &ctl)) {
        printf("camac_init_sis5100: SIS1100_IRQ_CTL: %s\n", strerror(errno));
        done_sis5100((struct generic_dev*)dev);
        return Err_System;
    } else {
        union callbackdata cbdata;
        cbdata.p=dev;
        info->st=sched_insert_select_task(sis5100_irq, cbdata,
            "sis5100_irq", info->ctrl_path, select_read
    #ifdef SELECT_STATIST
            , 1
    #endif
            );
        if (!info->st) {
            printf("vme_init_sis3100: cannot install callback for sis3100_irq\n");
            done_sis5100((struct generic_dev*)dev);
            return Err_System;
        }
    }

    dev->generic.done                 =done_sis5100;
#ifdef DELAYED_READ
    dev->generic.reset_delayed_read   =sis5100_reset_delayed_read;
    dev->generic.read_delayed         =sis5100_read_delayed;
    dev->generic.enable_delayed_read  =sis5100_enable_delayed_read;
    dev->generic.delayed_read_enabled=0;
#endif
    dev->CAMACaddr=sis5100_CAMACaddr;
    dev->CAMACaddrP=sis5100_CAMACaddrP;
    dev->CAMACval=sis5100_CAMACval;
    dev->CAMACgotQ=sis5100_CAMACgotQ;
    dev->CAMACgotX=sis5100_CAMACgotX;
    dev->CAMACgotQX=sis5100_CAMACgotQX;
    dev->CAMACinc=sis5100_CAMACinc;
    dev->CCCC=sis5100_CCCC;
    dev->CCCZ=sis5100_CCCZ;
    dev->CCCI=sis5100_CCCI;
    dev->CAMACstatus=sis5100_CAMACstatus;
    dev->CAMACread=sis5100_CAMACread;
    dev->CAMACwrite=sis5100_CAMACwrite;
    dev->CAMACcntl=sis5100_CAMACcntl;
    dev->CAMACblread=sis5100_CAMACblread;
    dev->CAMACblread16=sis5100_CAMACblread16;
    dev->CAMACblwrite=sis5100_CAMACblwrite;
    dev->CAMACblreadQstop=sis5100_CAMACblreadQstop;
    dev->CAMACblreadAddrScan=sis5100_CAMACblreadAddrScan;
    dev->CAMAClams=sis5100_CAMAClams;
    dev->CAMACenable_lams=sis5100_CAMACenable_lams;
    dev->CAMACdisable_lams=sis5100_CAMACdisable_lams;
    dev->camacdelay=sis5100_camacdelay;

#if 0
#ifdef DELAYED_READ
    info->hunk_list=0;
    info->hunk_list_size=0;
    info->delay_buffer=0;
    info->delay_buffer_size=0;
    sis5100_reset_delayed_read((struct generic_dev*)dev);
#endif
#endif
    dev->camcontrol=sis5100_camcontrol;
#ifdef ZELFERA
    dev->get_FERA_procs=sis5100_get_FERA_procs;
#endif
    dev->get_raw_procs=sis5100_get_raw_procs;

    sis5100_check_ident(dev);

    if (dev->generic.online) {
        sis5100_CCCC(dev);
        sis5100_CCCZ(dev);
        sis5100_CCCI(dev, 0);
        sis5100_set_user_led(info, 1);
    }

    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
