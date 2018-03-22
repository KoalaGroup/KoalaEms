/*
 * lowlevel/camac/pcicamac/pcicamac.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: pcicamac.c,v 1.34 2017/10/21 23:21:35 wuestner Exp $";

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

#ifndef HAVE_CGETCAP
#include <getcap.h>
#endif
#include <swap.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../../main/signals.h"
/*#include "../../../trigger/trigger.h"*/
#include "../../../objects/pi/readout.h"
#include "pcicamaddr.h"
#include "../camac.h"
#include "../camac_init.h"
#include "../../rawmem/rawmem.h"

#ifdef PCICAM_MMAPPED
#include <sys/mman.h>
#define MAPSIZE (64*1024)
#endif

#include <sys/camacio.h>
#include <dev/pci/pcicamvar.h>
#include <dev/pci/pcicam_map.h>

struct camac_pci_info {
    int p;
    int driverversion;
#ifdef PCICAM_MMAPPED
    volatile ems_u32* base;
#endif
    int delay;
    ems_u32 lammask;
    int statsignal;
    int lamsignal;
#ifdef ZELFERA
    int fera_present;
    struct rawmem rawmem;
    int dmasize;
    int pos;
    int failcount;
#endif
};

RCS_REGISTER(cvsid, "lowlevel/camac/pcicamac")

/*****************************************************************************/
#ifdef PCICAM_MMAPPED
static camadr_t
zel_CAMACaddr(int n, int a, int f)
{
    camadr_t addr;
    /*addr.pcicamac.naf=(f<<11)+(n<<6)+(a<<2);*/
    addr.pcicamac.naf=(f<<9)+(n<<4)+(a); /* base is ems_u32* !! */
    return addr;
}
#else
static camadr_t
zel_CAMACaddr(int n, int a, int f)
{
    camadr_t addr;
    addr.pcicamac.n=n;
    addr.pcicamac.a=a;
    addr.pcicamac.f=f;
    return addr;
}
#endif
/*****************************************************************************/
/*
 * WARNING!
 * return value ist statically allocated
 * it is only valid until the next call of CAMACaddrP
 */
#ifdef PCICAM_MMAPPED
static camadr_t*
zel_CAMACaddrP(int n, int a, int f)
{
    static camadr_t addr;
    /*addr.pcicamac.naf=(f<<11)+(n<<6)+(a<<2);*/
    addr.pcicamac.naf=(f<<9)+(n<<4)+(a); /* base is ems_u32* !! */
    return &addr;
}
#else
static camadr_t*
zel_CAMACaddrP(int n, int a, int f)
{
    static camadr_t addr;
    addr.pcicamac.n=n;
    addr.pcicamac.a=a;
    addr.pcicamac.f=f;
    return &addr;
}
#endif
/*****************************************************************************/
#ifdef PCICAM_MMAPPED
static void
zel_CAMACinc(camadr_t* addr)
{
    addr->pcicamac.naf++;
}
#else
static void
zel_CAMACinc(camadr_t* addr)
{
    if(++addr->pcicamac.a >= 16) {
	addr->pcicamac.a = 0;
	++addr->pcicamac.n;
    }
}
#endif
/*****************************************************************************/
static ems_u32
zel_CAMACval(ems_u32 val)
{
    return val&0xffffff;
}
/*****************************************************************************/
static ems_u32
zel_CAMACgotQ(ems_u32 val)
{
    return val&0x80000000;
}
/*****************************************************************************/
static ems_u32
zel_CAMACgotX(ems_u32 val)
{
    return val&0x40000000;
}
/*****************************************************************************/
static ems_u32
zel_CAMACgotQX(ems_u32 val)
{
    return val&0xC0000000;
}
/*****************************************************************************/
#ifdef PCICAM_MMAPPED
static int
_zel_CAMACread(struct camac_dev* dev, camadr_t* addr, ems_u32* val)
{
    struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
    *val=*(info->base+addr->pcicamac.naf);
#if 0
    printf("zel_CAMACread(mapped): addr=%04x val=%08x\n",
            addr->pcicamac.naf, *val);
#endif
    return 0;
}
#else
static int
_zel_CAMACread(struct camac_dev* dev, camadr_t* addr, ems_u32* val)
{
    struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
    struct camactransfer c;
    int res;

    c.n = addr->pcicamac.n;
    c.a = addr->pcicamac.a;
    c.f = addr->pcicamac.f;
    res=ioctl(info->p, NAFREAD, &c);
#if 0
    printf("zel_CAMACread: naf=(%d %d %d) val=%08x res=%d\n",
            c.n, c.a, c.f, c.u.val, res);
#endif
    *val=c.u.val;
    return res;
}
#endif

static void
camdelay(struct camac_dev* dev)
{
    if (dev->camdelay) {
        ems_u32 dummy;
        camadr_t dummyaddr=zel_CAMACaddr(30, 0, 0);
        int i;
        for (i=0; i<dev->camdelay; i++) {
	    _zel_CAMACread(dev, &dummyaddr, &dummy);
        }
    }
}

static int
zel_CAMACread(struct camac_dev* dev, camadr_t* addr, ems_u32* val)
{
    camdelay(dev);
    return _zel_CAMACread(dev, addr, val);
}
/*****************************************************************************/
#ifdef PCICAM_MMAPPED
static int
_zel_CAMACcntl(struct camac_dev* dev, camadr_t* addr, ems_u32* stat)
{
    struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
    volatile ems_u32 stat_;
    stat_=*(info->base+addr->pcicamac.naf);
    *stat=zel_CAMACgotQX(stat_);
    return 0;
}
#else
static int
_zel_CAMACcntl(struct camac_dev* dev, camadr_t* addr, ems_u32* stat)
{
    struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
    struct camactransfer c;
    int res;

    c.n = addr->pcicamac.n;
    c.a = addr->pcicamac.a;
    c.f = addr->pcicamac.f;
    res=ioctl(info->p, NAFREAD, &c);
    *stat=zel_CAMACgotQX(c.u.val);
    return res;
}
#endif
static int
zel_CAMACcntl(struct camac_dev* dev, camadr_t* addr, ems_u32* stat)
{
    camdelay(dev);
    return _zel_CAMACcntl(dev, addr, stat);
}
/*****************************************************************************/
#ifdef PCICAM_MMAPPED
static int
_zel_CAMACwrite(struct camac_dev* dev, camadr_t* addr, ems_u32 val)
{
    struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
    *(info->base+addr->pcicamac.naf)=val;
    return 0;
}
#else
static int
_zel_CAMACwrite(struct camac_dev* dev, camadr_t* addr, ems_u32 val)
{
    struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
    struct camactransfer c;

    c.n = addr->pcicamac.n;
    c.a = addr->pcicamac.a;
    c.f = addr->pcicamac.f;
    c.u.val = val;
    return ioctl(info->p, NAFWRITE, &c);
}
#endif
static int
zel_CAMACwrite(struct camac_dev* dev, camadr_t* addr, ems_u32 val)
{
    camdelay(dev);
    return _zel_CAMACwrite(dev, addr, val);
}
/*****************************************************************************/
static int
zel_CCCC(struct camac_dev* dev)
{
    ems_u32 stat;

    camadr_t addr=zel_CAMACaddr(28, 9, 26);
    return zel_CAMACcntl(dev, &addr, &stat);
}
/*****************************************************************************/
static int
zel_CCCZ(struct camac_dev* dev)
{
    ems_u32 stat;

    camadr_t addr=zel_CAMACaddr(28, 8, 26);
    return zel_CAMACcntl(dev, &addr, &stat);
}
/*****************************************************************************/
static int
zel_CCCI(struct camac_dev* dev, int val)
{
    ems_u32 stat;

    camadr_t addr=zel_CAMACaddr(30, 9, val?26:24);
    return zel_CAMACcntl(dev, &addr, &stat);
}
/*****************************************************************************/
#ifdef PCICAM_MMAPPED
static int
zel_CAMAClams(struct camac_dev* dev, ems_u32* val)
{
    struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
    /* CAMACread(n=30, a=0, f=0) */
    *val=*(info->base+(30<<6)+(0<<2)+(0<<11));
    return 0;
}
#else
static int
zel_CAMAClams(struct camac_dev* dev, ems_u32* val)
{
    struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
    struct camactransfer c;
    int res;

    c.n = 30;
    c.a = 0;
    c.f = 0;
    res=ioctl(info->p, NAFREAD, &c);
    *val=c.u.val;
    return res;
}
#endif
/*****************************************************************************/
static int
zel_CAMACenable_lams(struct camac_dev* dev, int idx)
{
    struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
    info->lammask |= 1<<(idx-1);
    return 0;
}
/*****************************************************************************/
static int
zel_CAMACdisable_lams(struct camac_dev* dev, int idx)
{
    struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
    info->lammask &= ~(1<<(idx-1));
    return 0;
}
/*****************************************************************************/
static int
zel_CAMACstatus(struct camac_dev* dev, ems_u32* val)
{
    struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
    return ioctl(info->p, CAMSTATUS, val);
}
/*****************************************************************************/
#if 1
static int
zel_CAMACblread(struct camac_dev* dev, camadr_t* addr, int num, ems_u32* dest)
{
    int res=0, n=num;
    T(CAMACblread)

    while (!res && n--) {
        res=zel_CAMACread(dev, addr, dest++);
    }
    return res<0?-1:num;
}
#else
static int
zel_CAMACblread(struct camac_dev* dev, camadr_t* addr, int num, ems_u32* dest)
{
    struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
    struct camactransfer transfer;
    int res;

#ifdef PCICAM_MMAPPED
    transfer.n=(addr->pcicamac.naf>>4)&0x1f;
    transfer.a=addr->pcicamac.naf&0xf;
    transfer.f=(addr->pcicamac.naf>>9)&0x1f;
#else
    transfer.n=addr->pcicamac.n;
    transfer.a=addr->pcicamac.a;
    transfer.f=addr->pcicamac.f;
#endif
    transfer.u.b.buf=dest;
    transfer.u.b.buflen=num*4;

    res=ioctl(info->p, NAFREADBLK, &transfer);
    return res<0?-1:transfer.u.b.buflen/4;
}
#endif
/*****************************************************************************/
static int
zel_CAMACblreadQstop(struct camac_dev* dev, camadr_t* addr,
        int num, ems_u32* dest)
{
    int res=0, n=0;
    T(CAMACblreadQstop)

    while (!res && num--) {
        ems_u32 val;
        res=zel_CAMACread(dev, addr, &val);
        if (zel_CAMACgotQ(val)) {
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
zel_CAMACblreadAddrScan(struct camac_dev* dev, camadr_t* addr,
        int num, ems_u32* dest)
{
    int res=0;
    T(CAMACblreadAddrScan)

    while (!res && num--) {
	res=zel_CAMACread(dev, addr, dest++);
    	zel_CAMACinc(addr);
    }
    return res;
}
/*****************************************************************************/
static int
zel_CAMACblwrite(struct camac_dev* dev, camadr_t* addr, int num, ems_u32* src)
{
    int res=0;
    T(CAMACblwrite)

    while (!res && num--) {
        res=zel_CAMACwrite(dev, addr, *src++);
    }
    return res;
}
/*****************************************************************************/
static int
zel_CAMACblread16(struct camac_dev* dev, camadr_t* addr, int num, ems_u32* dest)
/* Byteordering inkonsistent ??? */
{
    ems_u32 *save, val;
    int dummy, res=0;
    ems_u16* sdest=(ems_u16*)dest;

    T(CAMACblread16)
    save = dest;

    dummy = num;
    while (!res && num--) {
	res|=zel_CAMACread(dev, addr, &val);
        *sdest++ = (ems_u16)val;
    }
    if (dummy & 1)
        *sdest++ = 0;

    dest=(ems_u32*)sdest;
    swap_words_falls_noetig(save, dest - save);
    return res?-1:num;
}
/*****************************************************************************/
#ifdef ZELFERA
static void
zel_FERAdisable(struct camac_dev* dev)
{
        struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
	struct ferasetup d;
	int b;

printf("FERAdisable\n");
	d.mode = PCICAMMODE_CAMBLT;

	b = ioctl(info->p, FERASETUP, &d);
	if (b < 0) printf("pcicamac: FERASETUP failed\n");
}
/*****************************************************************************/
static int
zel_FERAenable(struct camac_dev* dev, int to, int del, int sgl)
{
        struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
        struct ferasetup d;
        struct feraeventinfo feraevent;
        struct feramem e;

printf("FERAenable: timeout=%d delay=%d sgl=%d\n", to, del, sgl);

        d.mode = sgl ? PCICAMMODE_FERASGL : PCICAMMODE_FERAMULTI;
        d.reqdelay = del;
        d.timeout = to;

        if (ioctl(info->p, FERASETUP, &d)<0) {
	        printf("FERASETUP failed\n");
	        return -1;
        }

        if (info->rawmem.len) {
            e.type = FERAMEMTYPE_PHYSICAL;
            e.base = info->rawmem.paddr;
            e.len = info->rawmem.len;
        } else {
#ifdef FERAMEMTYPE_KERNEL
            e.type = FERAMEMTYPE_KERNEL;
            e.base = 0;
            e.len = info->dmasize;
#else
            printf("zel_FERAenable: rawmem.len==0 but FERAMEMTYPE_KERNEL "
                    "not available\n");
            return -1;
#endif
        }

        printf("camac[%s]:FERAenable FERASETMEM(type=%d base=%llu len=%llu)\n",
                dev->pathname, e.type, (unsigned long long)e.base, (unsigned long long)e.len);
        if (ioctl(info->p, FERASETMEM, &e)<0) {
	        printf("FERASETMEM failed\n");
	        return -1;
        }

        info->pos = 0;

        feraevent.flags = FERACLEAR;

        if (ioctl(info->p, FERAEVENT, &feraevent)<0) {
	        printf("FERAEVENT: clear failed\n");
	        return -1;
        }

        return 0;
}

static int
zel_FERA_dmasize(struct camac_dev* dev, int size)
{
    struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
    info->dmasize=size;
    return 0;
}
/*****************************************************************************/
static int
_zel_FERAreadout_phys(struct camac_dev* dev, ems_u32** bufp)
{
        struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
	struct feraeventinfo feraevent;
	int b, evlen, datalen, overflow = 0;
	struct feramem e;
        ems_u32* rawbuf=(ems_u32*)(info->rawmem.buf);
        int rawbuflen=info->rawmem.len/4;

	feraevent.flags = FERAWAIT | FERACLEAR;
	feraevent.timeout = 100;
	do {
		b = ioctl(info->p, FERAEVENT, &feraevent);
	} while (b < 0 && errno == EINTR && --feraevent.timeout > 0);
	if(b < 0) {
		struct feradiags fd;
		printf("FERAEVENT: wait failed (%d)\n", errno);
		if (ioctl(info->p, FERAGETDIAGS, &fd)<0) {
			printf("FERAGETDIAGS after timeout failed\n");
			return -1;
		}
		if (fd.statcntl & 0x0800) { /* "idle" */
			printf("idle after timeout - clearing\n");
			feraevent.flags = FERACLEAR;
			b = ioctl(info->p, FERAEVENT, &feraevent);
			if (b < 0) {
				printf("FERA clear after timeout failed\n");
				return(-1);
			}
		}
		return(-1);
	}

again:
	if (!rawbuf[info->pos] || (rawbuf[info->pos] & 0xfffee000)) {
		if ((rawbuf[info->pos] & 0xffffe000) == 0x30000) {
#if 1
			printf("FERAreadout: timeout\n");
#endif
		} else if (rawbuf[info->pos] == 0x80011000) {
#if 1
			printf("FERAreadout: overflow\n");
#endif
			overflow = 1;
		} else {
			printf("FERAreadout: unexpected len word (%x) "
                                "at %x/%x\n",
			       rawbuf[info->pos], info->pos, rawbuflen);
			return -1;
		}
	}
	evlen = rawbuf[info->pos] & 0xffff;
	datalen = evlen + 1;
	if(datalen > rawbuflen - info->pos)
		datalen = rawbuflen - info->pos;

	bcopy(rawbuf+info->pos, *bufp, datalen*4);
	*bufp += datalen;
	bzero(rawbuf+info->pos, datalen*4);

	info->pos += datalen;

	if (info->pos >= rawbuflen) {
		if (info->pos > rawbuflen) {
			printf("FERAreadout: buffer len botch\n");
			exit(1);
		}
		b = read(info->p, *bufp, (evlen + 1 - datalen) * 4);
		if (b != (evlen + 1 - datalen) * 4) {
			printf("FERAreadout: read failed\n");
			return -1;
		}
		*bufp += evlen + 1 - datalen;
		bzero(rawbuf, rawbuflen);

		e.type = FERAMEMTYPE_PHYSICAL;
		e.base = info->rawmem.paddr;
		e.len = info->rawmem.len;

                printf("camac[%s]:FERAreadout_phys FERASETMEM(type=%d base=%llu len=%llu)\n",
                        dev->pathname, e.type, (unsigned long long)e.base,
                        (unsigned long long)e.len);
		if (ioctl(info->p, FERASETMEM, &e)<0) {
			printf("FERASETMEM failed\n");
			return -1;
		}
		info->pos = 0;
	}

	if (overflow) {
		overflow = 0;
		goto again;
	}

	return 0;
}

#ifdef FERAMEMTYPE_KERNEL
static int
_zel_FERAreadout_kern(struct camac_dev* dev, ems_u32** bufp)
{
        struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
	struct feraeventinfo feraevent;
	struct feramem e;
        size_t buflen=info->dmasize;
        ssize_t res;
	int b;

	feraevent.flags = FERAWAIT | FERACLEAR;
	feraevent.timeout = 100;
	do {
		b = ioctl(info->p, FERAEVENT, &feraevent);
	} while (b < 0 && errno == EINTR && --feraevent.timeout > 0);
	if(b < 0) {
		struct feradiags fd;
		printf("FERAEVENT: wait failed (%d)\n", errno);
		if (ioctl(info->p, FERAGETDIAGS, &fd)<0) {
			printf("FERAGETDIAGS after timeout failed\n");
			return -1;
		}
		if (fd.statcntl & 0x0800) { /* "idle" */
			printf("idle after timeout - clearing\n");
			feraevent.flags = FERACLEAR;
			b = ioctl(info->p, FERAEVENT, &feraevent);
			if (b < 0) {
				printf("FERA clear after timeout failed\n");
				return(-1);
			}
		}
		return -1;
	}

        /* bisher sind keine Daten gelesen, wir wissen nur, dass sie
           verfuegbar sind */

        /* _alle_ Daten lesen (die vom DMA und der Rest aus dem FIFO) */
        res=read(info->p, *bufp, buflen);
        if (res<0) {
            printf("FERAreadout: %s\n", strerror(errno));
            return -1;
        } else if ((size_t)res==buflen) {
            printf("FERAreadout: buffer too short (%llu)\n",
                    (unsigned long long)buflen);
            return -1;
        } else {
            ems_u32 word0=**bufp;
#if 0
            int i;
            printf("read %lld words\n", (long long int)res/4);
            for (i=0; i<res/4; i++)
                printf("[%2d]: %08x\n", i, (*bufp)[i]);
#endif
            /* consitency check */
            if ((word0&0x7ff1e000)!=0x10000) {
                printf("FERAreadout: illegal 1st word %08x\n", word0);
                return -1;
            }
#if 0
            if (word0&0x20000) {
                printf("FERAreadout: timeout (%08x)\n", word0);
                return -1;
            }
#endif
            if (word0&0x10000000) {
                printf("FERAreadout: overflow (%08x)\n", word0);
                return -1;
            }
            if (((word0&0x1fff)+1)*4 != res) {
                int missing;
                missing=((word0&0x1fff)+1)-res/4;
                printf("FERAreadout: size mismatch: word0=%08x (%d) res=%lld "
                        "missing: %d\n"
                        "evc=%d\n",
                        word0, ((word0&0x1fff)+1)*4,
                        (unsigned long long)res,
                        missing,
                        global_evc.ev_count);
                printf("calling READFIFO(%d)\n", missing);
                if (ioctl(info->p, READFIFO, &missing)<0)
                    printf("READFIFO: %s\n", strerror(errno));
                //if (++info->failcount>1)
                    return -1;
                //else
                //    res=0;
            }
            *bufp += res/4;
        }

        /* reenable DMA */
        e.type=FERAMEMTYPE_REENABLE;
#if 0
        printf("camac[%s]:FERAreadout_kern FERASETMEM(type=%d base=%llu len=%llu)\n",
                dev->pathname, e.type, (unsigned long long)e.base,
                (unsigned long long)e.len);
#endif
        if (ioctl(info->p, FERASETMEM, &e)<0) {
            printf("FERAreadout: FERASETMEM: %s\n", strerror(errno));
            return -1;
        }

        return 0;
}
#endif

static int
zel_FERAreadout(struct camac_dev* dev, ems_u32** bufp)
{
        struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
        if (info->rawmem.len) /* FERAMEMTYPE_PHYSICAL */
                return _zel_FERAreadout_phys(dev, bufp);
#ifdef FERAMEMTYPE_KERNEL
        else /* FERAMEMTYPE_KERNEL */
                return _zel_FERAreadout_kern(dev, bufp);
#endif
        printf("FERAreadout: FERAMEMTYPE confusion\n");
        return -1;
}
/*****************************************************************************/
static void
zel_FERAgate(struct camac_dev* dev)
{
        struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
	struct feraeventinfo feraevent;

	feraevent.flags =  FERAGATE;

	if (ioctl(info->p, FERAEVENT, &feraevent)<0) {
		printf("FERAgate: FERAEVENT failed\n");
        }
}
/*****************************************************************************/
static int
zel_FERAstatus(struct camac_dev* dev, ems_u32** buf)
{
        struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
	struct feradiags fd;
	struct ferastats fs;
	int b;

	b = ioctl(info->p, FERAGETDIAGS, &fd);
	if (b < 0) {
		printf("FERAstatus: FERAGETDIAGS failed\n");
		return(-1);
	}

	b = ioctl(info->p, FERAGETSTATS, &fs);
	if (b < 0) {
		printf("FERAstatus: FERAGETSTATS failed\n");
		return(-1);
	}

#define put(x) *((*buf)++) = x
	put(fd.cntl);
	put(fd.statcntl);
	put(fd.camstat);
	put(fd.F_reqdelay);
	put(fd.F_rotimo);
	put(fd.event_pending);
	put(fd.imb4);
	put(fd.mwtc);
	put(fd.mcsr);
	put(fs.rejects);
	put(fs.timeouts);
	put(fs.maint_timer);
#undef put
	return(0);
}
/*****************************************************************************/
static int
zel_FERA_select_fd(struct camac_dev* dev)
{
        struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
        return info->p;
}
#endif /* FERA */
/*****************************************************************************/
static int
zel_camacdelay(struct camac_dev* dev, int delay, int* olddelay)
{
    *olddelay=dev->camdelay;
    if (delay>=0)
        dev->camdelay=delay;
    return 0;
}
/*****************************************************************************/
#ifdef DELAYED_READ
static int
zel_enable_delayed_read(__attribute__((unused)) struct generic_dev* dev,
        __attribute__((unused)) int val)
{
    return 0;
}

static void zel_reset_delayed_read(
        __attribute__((unused)) struct generic_dev* gdev) {}

static int
zel_read_delayed(__attribute__((unused)) struct generic_dev* gdev)
{
    return -1;
}
#endif
/*****************************************************************************/
#ifdef CAMCNTLRW
static plerrcode
pcicamac_camcontrol(struct camac_dev* dev, int space, int rw, ems_u32 offs,
            ems_u32 *val)
{
    struct camac_pci_info* info=(struct camac_pci_info*)dev->info;
    struct camcntl_rw r;
    int res;

    r.space=space;
    r.rw=rw;
    r.offs=offs;
    r.data=*val;

    if (rw&2)
        printf("write 0x%08x to   addr 0x%x of space %d\n", r.data, offs, space);
    res=ioctl(info->p, CAMCNTLRW, &r);

    *val=r.data;
    if (rw&1)
        printf("read  0x%08x from addr 0x%x of space %d\n", r.data, offs, space);

    return res?plErr_System:plOK;
}
#else
static plerrcode
pcicamac_camcontrol(struct camac_dev* dev, int space, int rw, ems_u32 offs,
            ems_u32 *val)
{
    return plErr_NotImpl;
}
#endif
/*****************************************************************************/
static int
check_camacdiags(struct camac_dev* dev)
{
    struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
    struct feradiags diags;
    int crate_online, camac_online, camac_inhibit, fera_present;
#ifdef FERAEXTSTATS
    struct feraextstats stats;
#endif

    if (info->driverversion==-1) {
        /* not implemented in older driver */
#ifdef PCICAMDRIVERVERSION
        if (ioctl(info->p, PCICAMDRIVERVERSION, &info->driverversion)<0) {
            printf("lowlevel/camac/pcicamac/pcicamac.c: ioctl(DRIVERVERSION): %s\n",
                strerror(errno));
            info->driverversion=0;
        }
#else
        info->driverversion=0;
#endif
        printf("driverversion=%d.%d\n",
                info->driverversion>>16,
                info->driverversion&0xffff);
    }

    if (ioctl(info->p, FERAGETDIAGS, &diags)<0) {
        printf("lowlevel/camac/pcicamac/pcicamac.c: ioctl(FERAGETDIAGS): %s\n",
                strerror(errno));
        return -1;
    }
#ifdef FERAEXTSTATS
    if (info->driverversion>=(2<<16)) {
        if (ioctl(info->p, FERAEXTSTATS, &stats)<0) {
            printf("lowlevel/camac/pcicamac/pcicamac.c: ioctl(FERAEXTSTATS): %s\n",
                    strerror(errno));
        }
    }
#endif

printf("imb4    =0x%08x\n", diags.imb4);
printf("statcntl=0x%08x\n", diags.statcntl);
printf("camstat =0x%08x\n", diags.camstat);
#ifdef FERAEXTSTATS
if (info->driverversion>=(2<<16))
    printf("stats   =0x%08x\n", stats.stat);
#endif

#ifdef FERAEXTSTATS
    if (info->driverversion>=(2<<16)) {
        crate_online=!!(stats.stat&feraext_on);
        camac_online=!!(stats.stat&feraext_online);
        fera_present=!!(stats.stat&feraext_ferapresent);
    } else
#endif
    {
        crate_online=!(diags.imb4 & MBX4_OFFLINE);
        camac_online=!(diags.statcntl & CAMAC_OFFLINE);
        fera_present=(diags.statcntl&0xffff0000)==0xffff0000;
    }
    camac_inhibit=!!(diags.camstat&CAMSTAT_INHIBIT);

    printf("pcicamac(%s): crate is o%sline\n",
            dev->pathname, crate_online?"n":"ff");
    if (crate_online!=dev->generic.online) {
        send_unsol_var(Unsol_DeviceDisconnect, 4, modul_camac,
            dev->generic.dev_idx, crate_online?1:-1, 0);
    }
    if (crate_online) {
        printf("CAMAC is o%sline\n", camac_online?"n":"ff");
        printf("CAMAC is%s inhibited\n", camac_inhibit?"":" not");
#ifdef ZELFERA
        printf("FERA  is%s present\n", fera_present?"":" not");
#endif
    }

    /* don't change status if old driver is used */
    if (info->driverversion>=(2<<16)) {
        dev->generic.online=crate_online;
        dev->camac_online=camac_online;
    }
#ifdef ZELFERA
    if (crate_online)
        info->fera_present=fera_present;
#endif

    return 0;
}
/*****************************************************************************/
#ifdef ZELFERA
static struct FERA_procs FERA_procs={
    zel_FERAenable,
    zel_FERAreadout,
    zel_FERAgate,
    zel_FERAstatus,
    zel_FERAdisable,
    zel_FERA_select_fd,
    zel_FERA_dmasize,
};

static struct FERA_procs*
pcicamac_get_FERA_procs(struct camac_dev* dev)
{
    struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
    if (info->fera_present)
        return &FERA_procs;

    if (info->driverversion<(2<<16)) {
        /* old driver and hardware was or is still offline */
        if (check_camacdiags(dev)<0)
            return 0;
        if (info->fera_present)
            return &FERA_procs;
    }
    return 0;
}
#else
static struct FERA_procs*
pcicamac_get_FERA_procs(struct camac_dev* dev)
{
    return 0;
}
#endif

static struct camac_raw_procs*
pcicamac_get_raw_procs(__attribute__((unused)) struct camac_dev* dev)
{
    return 0;
}
/*****************************************************************************/
static void
lamhandler(union SigHdlArg arg, int signal)
{
    struct camac_dev* dev=(struct camac_dev*)arg.ptr;
    printf("lamhandler called for %s, signal=%d\n", dev->pathname, signal);
}
/*****************************************************************************/
static void
stathandler(union SigHdlArg arg, __attribute__((unused)) int signal)
{
    struct camac_dev* dev=(struct camac_dev*)arg.ptr;

printf("stathandler called\n");
    check_camacdiags(dev);
}
/*****************************************************************************/
static void
release_sighandler(struct camac_dev* dev)
{
    struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
    if (info->lamsignal!=-1)
        remove_signalhandler(info->lamsignal);
    if (info->statsignal!=-1)
        remove_signalhandler(info->statsignal);
}
/*****************************************************************************/
static int
install_sighandler(struct camac_dev* dev)
{
    struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
    union SigHdlArg a;
    struct lamhandling lams;

    a.ptr=dev;
    info->statsignal=install_signalhandler(stathandler, a, "camstathandler");
    if (info->statsignal==-1) {
        printf("pcicamac %s: installing of stathandler failed\n",
                dev->pathname);
        return -1;
    }
    lams.selectmask=0;
    lams.sigmask=0xff000000;
    lams.sig=info->statsignal;
    if (ioctl(info->p, SETLAMHANDLING, &lams)<0) {
        printf("lowlevel/camac/pcicamac/pcicamac.c: ioctl(SETLAMHANDLING): %s\n",
                strerror(errno));
        remove_signalhandler(info->statsignal);
        return -1;
    } else {
        printf("SETLAMHANDLING(A) successfull\n");
    }

    info->lamsignal=install_signalhandler(lamhandler, a, "lamhandler");
    if (info->lamsignal==-1) {
        printf("pcicamac %s: installing of lamhandler failed\n", dev->pathname);
        return -1;
    }
    lams.selectmask=0;
    lams.sigmask=0xffffff;
    lams.sig=info->lamsignal;
    if (ioctl(info->p, SETLAMHANDLING, &lams)<0) {
        printf("lowlevel/camac/pcicamac/pcicamac.c: ioctl(SETLAMHANDLING): %s\n",
                strerror(errno));
        remove_signalhandler(info->lamsignal);
        return -1;
    } else {
        printf("SETLAMHANDLING(B) successfull\n");
    }

    return 0;
}
/*****************************************************************************/
static errcode done_pcicamac(struct generic_dev* gdev)
{
    struct camac_dev* dev=(struct camac_dev*)gdev;
    struct camac_pci_info *info=(struct camac_pci_info*)dev->info;
    printf("done_pcicamac\n");

    release_sighandler(dev);

#ifdef PCICAM_MMAPPED
    munmap((void*)(info->base), MAPSIZE);
#endif

    printf("close CAMAC path=%d\n", info->p);
    close(info->p);
#ifdef FERA
    rawmem_free(&info->rawmem);
#endif

    if (dev->pathname)
        free(dev->pathname);
    dev->pathname=0;

    free(info);
    dev->info=0;
    return OK;
}
/*****************************************************************************/
errcode
camac_init_pcicamac(struct camac_dev* dev, char* rawmempart)
{
    struct camac_pci_info* info;

    T(camac_pcicamac_low_init)
    printf("camac_init_pcicamac(%s, %s)\n", dev->pathname,
        rawmempart?rawmempart:"no rawmem given");

    info=(struct camac_pci_info*)malloc(sizeof(struct camac_pci_info));
    if (!info) {
        printf("camac_init_pcicamac: calloc(info): %s\n", strerror(errno));
        return Err_NoMem;
    }
    dev->info=info;

    info->delay=1;
    info->lammask=0;
    info->statsignal=-1;
    info->lamsignal=-1;
    info->failcount=0;

    info->dmasize=256*1024;

    info->p=open(dev->pathname, O_RDWR, 0);
    if (info->p<0) {
        printf("open CAMAC (pcicamac) %s: %s\n", dev->pathname, strerror(errno));
        return Err_System;
    }
    if (fcntl(info->p, F_SETFD, FD_CLOEXEC)<0) {
        printf("fcntl(CAMAC %s, FD_CLOEXEC): %s\n", dev->pathname, strerror(errno));
    }

#ifdef PCICAM_MMAPPED
# ifdef __linux__
    info->base=(ems_u32*)mmap(0, MAPSIZE, PROT_READ|PROT_WRITE, MAP_SHARED, 
# else /* __linux__ */
    info->base=(ems_u32*)mmap(0, MAPSIZE, PROT_READ|PROT_WRITE, MAP_FILE, 
# endif /* __linux__ */
        info->p, 0);
    if (info->base==(ems_u32*)-1) {
        printf("mmap CAMAC dev %s: %s\n", dev->pathname, strerror(errno));
        close(info->p);
        return Err_System;
    }
#endif

#ifdef FERA
    if (rawmempart) {
        if (rawmem_alloc_part(rawmempart, &info->rawmem, "pcicamac")<0) {
            printf("pcicamac: alloc %s of rawmem failed\n", rawmempart);
            return Err_NoMem;
        } else {
            printf("CAMAC/rawmem: len=0x%lx paddr=0x%lx buf=%p\n",
                    info->rawmem.len, info->rawmem.paddr, info->rawmem.buf);
            if (info->rawmem.len==0) {
                printf("pcicamac: %s of rawmem requested but size is zero\n",
                        rawmempart);
                return Err_NoMem;
            }
        }
    } else {
        printf("no rawmempart given for CAMAC path %s\n", dev->pathname);
        info->rawmem.len=0;
        info->rawmem.paddr=0;
        info->rawmem.buf=0;
    }
#endif /* FERA */

    dev->generic.done=done_pcicamac;
#ifdef DELAYED_READ
    dev->generic.reset_delayed_read=zel_reset_delayed_read;
    dev->generic.read_delayed=zel_read_delayed;
    dev->generic.enable_delayed_read=zel_enable_delayed_read;
    dev->generic.delayed_read_enabled=0;
#endif
    dev->CAMACaddr=zel_CAMACaddr;
    dev->CAMACaddrP=zel_CAMACaddrP;
    dev->CAMACval=zel_CAMACval;
    dev->CAMACgotQ=zel_CAMACgotQ;
    dev->CAMACgotX=zel_CAMACgotX;
    dev->CAMACgotQX=zel_CAMACgotQX;
    dev->CAMACinc=zel_CAMACinc;
    dev->CCCC=zel_CCCC;
    dev->CCCZ=zel_CCCZ;
    dev->CCCI=zel_CCCI;
    dev->CAMACstatus=zel_CAMACstatus;
    dev->CAMACread=zel_CAMACread;
    dev->CAMACwrite=zel_CAMACwrite;
    dev->CAMACcntl=zel_CAMACcntl;
    dev->CAMACblread=zel_CAMACblread;
    dev->CAMACblread16=zel_CAMACblread16;
    dev->CAMACblwrite=zel_CAMACblwrite;
    dev->CAMACblreadQstop=zel_CAMACblreadQstop;
    dev->CAMACblreadAddrScan=zel_CAMACblreadAddrScan;
    dev->CAMAClams=zel_CAMAClams;
    dev->CAMACenable_lams=zel_CAMACenable_lams;
    dev->CAMACdisable_lams=zel_CAMACdisable_lams;
    dev->camacdelay=zel_camacdelay;
#ifdef DELAYED_READ
    //dev->delay_read= gibt es nicht;
#endif
    dev->camcontrol=pcicamac_camcontrol;
#ifdef FERA
    dev->get_FERA_procs=pcicamac_get_FERA_procs;
#endif
    dev->get_raw_procs=pcicamac_get_raw_procs;

    /*
     * old driver will never announce Crate on/off and CAMAC online
     * so we have to set online=1 and camac_online=1
     * regardless of the actual crate status
     * 'check_camacdiags' will update these values if a new driver is used
     */
    dev->generic.online=1;
    dev->camac_online=1;
    info->driverversion=-1;
    info->fera_present=0;

    if (check_camacdiags(dev)<0) {
        close(info->p);
        return Err_System;
    }

    install_sighandler(dev);

    if (dev->generic.online && dev->camac_online) {
        zel_CCCC(dev);
        zel_CCCZ(dev);
        zel_CCCI(dev, 0);
    }

    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
