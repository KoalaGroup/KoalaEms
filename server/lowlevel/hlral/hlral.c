/*
 * lowlevel/hlral/hlral.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: hlral.c,v 1.17 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <cdefs.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <rcs_ids.h>
#include <errorcodes.h>
#include <swap.h>
#ifndef HAVE_CGETCAP
#include <getcap.h>
#endif

#include "../devices.h"
#include "../../main/signals.h"
#include "../../commu/commu.h"

#define NO_HLRAL_PLX
#ifdef NO_HLRAL_PLX
/*#error NO HLRAL PLX*/
/* we have only the AMCC driver */
#   include <dev/pci/pcihotlinkvar.h>
#else /* PLX */
/* hlplx_var.h has all definitions of pcihotlinkvar.h */
/* the AMCC version is missing some features, but we can fix that at runtime */
#   include <dev/pci/hlplx_var.h>
#endif

#include "../rawmem/rawmem.h"
#include "hlralreg.h"
#include "hlral_int.h"
#include "hlral.h"

#ifdef HLMEMTYPE_PHYSICAL /* we are using the AMCC version */
/*#error HLMEMTYPE PHYSICAL*/
#undef HLMEMTYPE_PHYSICAL
enum hlmemtype {
    HLMEMTYPE_INVALID=0,
    HLMEMTYPE_PHYSICAL=666,
    HLMEMTYPE_SCATTERED
};
#endif

#ifndef HLRAL_SG_SIZE
#define HLRAL_SG_SIZE 1048576
#endif
#define HLRAL_KERN_SIZE 1048576

#undef HLDEBUG

/* _RPC is 'Registers Per Card */
#if (HLRAL_VERSION==HLRAL_ATRAP)
#define MAX_RAL_RPC 16
#else
# if (HLRAL_VERSION==HLRAL_ANKE)
# define MAX_RAL_RPC 4
# else
# error HLRAL_VERSION must be defined
# endif
#endif

#if ! defined (HLPLX_Version) && ! defined (HLVERSION)
enum hl_dma_type {hl_dma_none=0, hl_dma_rawmem=1, hl_dma_sg=2, hl_dma_kernel=4};
#endif

RCS_REGISTER(cvsid, "lowlevel/hlral")

/*****************************************************************************/
int
hlral_low_printuse(outfilepath)
	FILE *outfilepath;
{
	fprintf(outfilepath, "  [:pcihlp=<pcihlpath>[:rmemp=<rawmempath>]]\n");
	return 1;
}
/*****************************************************************************/
static void
identify_driver(struct hlral* hl)
{
    hl->chip_type=hl_chip_unknown;
    hl->dma_mask=hl_dma_none;
    hl->dma_type=hl_dma_none;
    hl->dma_unmap=0;
    hl->chip_version=0;
    hl->driverversion=-1;
    hl->boardversion=-1;
#if defined (HLPLX_Version)
    printf("hlral[%s]: compiled with HLPLX driver %d.%d\n",
        hl->pathname, HLPLX_MajVersion, HLPLX_MinVersion);
#elif defined (PCIHL_Version)
    printf("hlral[%s]: compiled with PCIHL driver %d.%d\n",
        hl->pathname, PCIHL_MajVersion, PCIHL_MinVersion);
#else
    printf("hlral[%s]: compiled with old PCIHL driver\n", hl->pathname);
    hl->dma_mask=hl_dma_rawmem;
#endif

#ifdef HLVERSION
    {
        struct hlversion version;
        if (ioctl(hl->p, HLVERSION, &version)<0) {
            printf("hlral[%s]: HLVERSION(identify): %s\n",
                    hl->pathname, strerror(errno));
            return;
        }

        hl->chip_type=version.chip_type;
        hl->dma_mask=version.dma_type;
        hl->chip_version=version.chip_version;
        hl->driverversion=version.driverversion;
        hl->boardversion=version.boardversion;

        printf("hlral[%s]: chip_type=", hl->pathname);
        switch (hl->chip_type) {
        case hl_chip_amcc: printf("AMCC "); break;
        case hl_chip_plx: printf("PLX "); break;
        default: printf("unknown chip 0x%x", hl->chip_type);
        }
        printf("0x%x\n", hl->chip_version);
        printf("hlral[%s]: dma_type=%d\n", hl->pathname, hl->dma_mask);
        printf("hlral[%s]: driverversion=%d.%02d\n",
                hl->pathname,
                hl->driverversion>>16,
                hl->driverversion&0xffff);
        printf("hlral[%s]: boardversion =%d\n", hl->pathname, hl->boardversion);
    }
#else
    printf("hlral[%s]: HLVERSION is not defined\n", hl->pathname);
#endif
}
/*****************************************************************************/
static enum hl_dma_type
check_dma_type(struct hlral* hl)
{
    identify_driver(hl);

    /*
     * dma_type is a bitmap of all available DMA methods,
     * we will leave only one bit set in order to return an unambigous value
     */
    if (hl->rawmem.len>0 && (hl->dma_mask & hl_dma_rawmem))
        return hl_dma_rawmem;
    if (hl->dma_mask & hl_dma_sg)
        return hl_dma_sg;
    if (hl->dma_mask & hl_dma_kernel)
        return hl_dma_kernel;
    printf("hlral[%s]: dma_type 0x%x not usable\n", hl->pathname, hl->dma_mask);
    return hl_dma_none;
}
/*****************************************************************************/
static int
prepare_dma_none(struct hlral *hl)
{
    printf("hlral[%s]: DMA=hl_dma_none\n", hl->pathname);
    hl->dma_buf = 0;
    hl->dma_len = 0;
    hl->dma_unmap=0;
    return 0;
}
/*****************************************************************************/
static void
dma_unmap_rawmem(struct hlral *hl)
{
    rawmem_free(&hl->rawmem);
}
/*****************************************************************************/
static int
prepare_dma_rawmem(struct hlral *hl)
{
    struct hlmem hlmem;
    printf("hlral[%s]: DMA=hl_dma_rawmem\n", hl->pathname);
    hl->dma_buf = (unsigned char*)hl->rawmem.buf;
    hl->dma_len = hl->rawmem.len;
    hlmem.type  = HLMEMTYPE_PHYSICAL;
    hlmem.base  = hl->rawmem.paddr;
    hlmem.len   = hl->dma_len;
    if (ioctl(hl->p, HLSETMEM, &hlmem) < 0) {
        printf("hlral[%s]: setmem: %s\n", hl->pathname, strerror(errno));
        return -1;
    }
    hl->dma_unmap=dma_unmap_rawmem;
    return 0;
}
/*****************************************************************************/
static void
dma_unmap_sg(struct hlral *hl)
{
    struct hlmem hlmem;
    hlmem.type=HLMEMTYPE_INVALID;
    if (ioctl(hl->p, HLSETMEM, &hlmem)<0) {
        printf("hlral[%s]: ioctl(HLSETMEM(INVALID)): %s\n",
                hl->pathname, strerror(errno));
    } else {
        /* the next line causes a kernel panic if the
           ioctl above fails*/
        free (hl->dma_buf);
    }
}
/*****************************************************************************/
static int
prepare_dma_sg(struct hlral *hl)
{
    struct hlmem hlmem;
    printf("hlral[%s]: DMA=hl_dma_sg\n", hl->pathname);
    hl->dma_len  = HLRAL_SG_SIZE;
    hl->dma_buf = malloc(hl->dma_len);
    if (!hl->dma_buf) {
        printf("hlral[%s]: malloc(%llu): %s\n",
                hl->pathname, 
                (unsigned long long)hl->dma_len,
                strerror(errno));
        /* XXX free rawmem and other buffers !!! */
        return -1;
    }
    printf("hlral[%s]: base=0x%0lx, len=%ld\n", hl->pathname,
            hlmem.base, hlmem.len);
    hlmem.type=HLMEMTYPE_SCATTERED;
    hlmem.base=(unsigned long)hl->dma_buf;
    hlmem.len=hl->dma_len;
    if (ioctl(hl->p, HLSETMEM, &hlmem) < 0) {
        printf("hlral[%s]: setmem: %s\n", hl->pathname, strerror(errno));
	return -1;
    }
    hl->dma_unmap=dma_unmap_sg;
    return 0;
}
/*****************************************************************************/
#if 0
static void
dma_unmap_kernel(struct hlral *hl)
{
    struct hlmem hlmem;
    munmap(hl->dma_buf, hl->dma_len);
    hlmem.type=HLMEMTYPE_INVALID;
    if (ioctl(hl->p, HLSETMEM, &hlmem)<0)
        printf("hlral[%s]: ioctl(HLSETMEM(INVALID)): %s\n",
                hl->pathname, strerror(errno));
}
#endif
/*****************************************************************************/
static int
prepare_dma_kernel(struct hlral *hl)
{
    struct hlmem hlmem;

    hl->dma_len = HLRAL_KERN_SIZE;
    hlmem.type = HLMEMTYPE_KERNEL;
    hlmem.base = 0;
    hlmem.len = hl->dma_len;
    if (ioctl(hl->p, HLSETMEM, &hlmem) < 0) {
        printf("hlral[%s]: setmem: %s\n", hl->pathname, strerror(errno));
        return -1;
    }

#if 0
    hl->dma_buf=mmap(0, hl->dma_len, PROT_READ|PROT_WRITE, MAP_SHARED, hl->p, 0);
    if (hl->dma_buf==MAP_FAILED) {
        printf("hlral[%s]: mmap: %s\n", hl->pathname, strerror(errno));
        return -1;
    }
    printf("hlral[%s]: mmap: dma_buf=%p\n", hl->pathname, hl->dma_buf);
    hl->dma_unmap=dma_unmap_kernel;
#else
    hl->dma_buf=0;
    hl->dma_unmap=0;
#endif
    return 0;
}
/*****************************************************************************/
static int
prepare_dma(struct hlral *hl)
{
    hl->dma_unmap=0;
    hl->dma_type=check_dma_type(hl);

    switch (hl->dma_type) {
    case hl_dma_none:
        return prepare_dma_none(hl);
    case hl_dma_rawmem:
        return prepare_dma_rawmem(hl);
    case hl_dma_sg:
        return prepare_dma_sg(hl);
    case hl_dma_kernel:
        return prepare_dma_kernel(hl);
    default:
        printf("hlral[%s]: unhandled dma type 0x%x\n",
                hl->pathname, hl->dma_type);
        return -1;
    }
}
/*****************************************************************************/
static void
hlral_stathandler(union SigHdlArg arg, int signal)
{
    struct hlral* hl=(struct hlral*)arg.ptr;
    struct hlstatus status;
    int old_online=hl->generic.online;

    if (ioctl(hl->p, HLSTATUS, &status)<0) {
        printf("hlral[%s]: ioctl(HLSTATUS): %s\n",
                hl->pathname, strerror(errno));
        return;
    }
    hl->generic.online=!!(status.statctl&0x10);
    printf("hlral[%s]: carrier %s\n", hl->pathname,
            hl->generic.online?"detected":"lost");
    if (hl->generic.online!=old_online) {
        send_unsol_var(Unsol_DeviceDisconnect, 4, modul_pcihl,
            hl->generic.dev_idx, hl->generic.online?1:-1, 0);
    }
}
/*****************************************************************************/
static void
release_sighandler(struct hlral *hl)
{
printf("hlral release_sighandler, sig=%d\n", hl->signal);
    if (hl->signal!=-1)
        remove_signalhandler(hl->signal);
}
/*****************************************************************************/
static int
install_sighandler(struct hlral *hl)
{
    union SigHdlArg a;
    int sig;

    a.ptr=hl;

    hl->signal=sig=install_signalhandler(hlral_stathandler, a, "hlstathandler");
    if (hl->signal==-1) {
        printf("hlral[%s]: installing of stathandler failed\n", hl->pathname);
        return -1;
    }
    if (ioctl(hl->p, HLSIGNAL, &sig)<0) {
        printf("hlral[%s]: ioctl(HLSIGNAL): %s\n",
                hl->pathname, strerror(errno));
        return -1;
    }
    return 0;
}
/*****************************************************************************/
static errcode
hlral_done(struct generic_dev* gdev)
{
    struct hlral *hl=(struct hlral*)gdev;

printf("hlral_done\n");
    release_sighandler(hl);
    if (hl->dma_unmap)
        hl->dma_unmap(hl);
    close(hl->p);
    return OK;
}
/*****************************************************************************/
static errcode
hlral_init(struct hlral *hl, char* rawmempart)
{
    struct hlstatus status;
    int zero = 0;
    errcode res;

    hl->signal=-1;
    hl->dma_type=hl_dma_none;
    hl->dma_unmap=0;

    hl->p = open(hl->pathname, O_RDWR, 0);
    if (hl->p < 0) {
        printf("open PCI-Hotlink at %s: %s\n",
                hl->pathname, strerror(errno));
        return Err_System;
    }

    if (fcntl(hl->p, F_SETFD, FD_CLOEXEC)<0) {
        printf("hlral[%s]: fcntl(FD_CLOEXEC): %s\n",
                hl->pathname, strerror(errno));
        res=Err_System;
        goto errout;
    }

    if (rawmempart) {
        printf("hlral[%s]: rawmempart given as \"%s\"\n",
                hl->pathname, rawmempart);
        if (rawmem_alloc_part(rawmempart, &hl->rawmem, "hlral")<0) {
            printf("hlral[%s]: alloc %s of rawmem failed\n",
                    hl->pathname, rawmempart);
            res=Err_NoMem;
            goto errout;
        } else {
            if (hl->rawmem.len==0) {
                printf("hlral[%s]: %s of rawmem requested but size is zero\n",
                        hl->pathname, rawmempart);
                res=Err_NoMem;
                goto errout;
           }
        }
    } else {
        printf("hlral[%s]: no rawmempart given\n", hl->pathname);
        hl->rawmem.len=0;
        hl->rawmem.paddr=0;
        hl->rawmem.buf=0;
    }

    hl->wbufidx=0;

    if (prepare_dma(hl)<0) {
        res=Err_System;
        goto errout;
    }

    if (ioctl(hl->p, HLSTATUS, &status)<0) {
        printf("hlral[%s]: ioctl(HLSTATUS): %s, p=%d\n",
                hl->pathname, strerror(errno), hl->p);
        /* we have no information from an old driver */
        hl->generic.online=1;
    } else {
        hl->generic.online=!!(status.statctl&0x10);
        install_sighandler(hl);
    }
    printf("hlral[%s]: %s\n",
            hl->pathname, hl->generic.online?"carrier found":"no carrier");

    if (ioctl(hl->p, HLLOOPBACK, &zero)<0) {
        printf("hlral[%s]: ioctl(HLLOOPBACK): %s\n",
                hl->pathname, strerror(errno));
        res=Err_System;
        goto errout;
    }

    if (hl->generic.online) {
        if (hlral_real_reset(hl)<0) {
	    printf("hlral[%s]: hlral_real_reset failed\n", hl->pathname);
            res=Err_System;
            goto errout;
        }
    }

    hl->generic.done=hlral_done;

#if 0
		res = hlral_reset(i);
		if (res < 0) {
			printf("hlral_reset failed\n");
			goto errout;
		}
#endif

    return OK;

errout:
    release_sighandler(hl);
    if (hl->dma_unmap)
        hl->dma_unmap(hl);
    close(hl->p);
    return res;
}
/*****************************************************************************/
errcode
hlral_low_init(char *arg)
{
	char *hlpath, *help;
	int i, res;

	T(hlral_low_init)

	if ((!arg) || (cgetstr(arg, "pcihlp", &hlpath) < 0)) {
		printf("no PCI Hotlink device given\n");
		return (OK);
	}
	D(D_USER, printf("PCI-Hotlinks are %s\n", hlpath); )

	help = hlpath;
	res = 0; i=0;
	while (help) {
                struct hlral* dev;
		char *comma, *semi;

		comma = strchr(help, ',');
		if (comma)
			*comma = '\0';
                if (!strcmp(help, "none")) {
                    printf("No device for hlral %d\n", i);
                    register_device(modul_pcihl, 0, "hlral");
                } else {
		    semi = strrchr(help, ';');
		    if (semi)
			    *semi++ = '\0';

                    dev=calloc(1, sizeof(struct hlral));
                    init_device_dummies(&dev->generic);
                    dev->pathname=strdup(help);
                    res=hlral_init(dev, semi);
                    if (res==OK) {
                        register_device(modul_pcihl, (struct generic_dev*)dev,
                                "hlral");
                    } else {
                        if (dev->pathname)
                            free(dev->pathname);
                        free(dev);
                        break;
                    }
                }

                i++;
		if (comma)
			help = comma + 1;
		else
			help = 0;
	}

	free(hlpath);

        if (res==OK)
            return OK;

        for (i=0; i<num_devices(modul_camac); i++) {
            struct generic_dev* dev;
            if (find_device(modul_pcihl, i, &dev)==OK) {
                dev->generic.done(dev);
                destroy_device(modul_pcihl, i);
            }
        }

	return res;
}
/*****************************************************************************/
int number_of_ralhotlinks()
{
    return num_devices(modul_pcihl);
}
/*****************************************************************************/
errcode
hlral_low_done(void)
{
    /* devices_done will do all work */
    return OK;
}
/*****************************************************************************/
#ifdef HLDEBUG
static ems_u8 storage[1048576];
static int storage_idx=0;

static void
store_bytes1(ems_u8 v)
{
    storage[storage_idx++]=v;
}
/*
 * static void
 * store_bytes2(ems_u16 v)
 * {
 *     storage[storage_idx++]=v&0xff;
 *     storage[storage_idx++]=(v>>8)&0xff;
 * }
 */
static void
store_bytes4(ems_u32 v)
{
    storage[storage_idx++]=v&0xff;
    storage[storage_idx++]=(v>>8)&0xff;
    storage[storage_idx++]=(v>>16)&0xff;
    storage[storage_idx++]=(v>>24)&0xff;
}
#endif /* HLDEBUG */

static void
dump_bytes_buf(ems_u8* buf, int len)
{
    char oldline[50];
    char line[50];
    int idx=0, rep=0;

    oldline[0]=0;
    while (idx<len) {
        int rest=len-idx, i;
        if (rest>16) rest=16;
        for (i=0; i<50; i++) line[i]=' ';
        line[49]=0;
        for (i=0; i<rest; idx++, i++) {
            int v=buf[idx];
            char c=((v>>4)&0xf)+'0';
            if (c>'9') c+='A'-'9'-1;
            line[i*3]=c;
            c=(v&0xf)+'0';
            if (c>'9') c+='A'-'9'-1;
            line[i*3+1]=c;
        }
        for (i=0; i<50; i++) if (line[i]!=oldline[i]) break;
        if (i==50) {
            rep++;
        } else {
            if (rep) {
                printf("last line repeated %d times\n", rep);
                rep=0;
            }
            printf("%s\n", line);
            for (i=0; i<50; i++) oldline[i]=line[i];
        }
    }
    if (rep)
        printf("last line repeated %d times\n", rep);
}

#ifdef HLDEBUG
static void
dump_bytes_written(char* text)
{
    printf("written to hotlink(%s): %d bytes\n", text, storage_idx);
    dump_bytes_buf(storage, storage_idx);
    storage_idx=0;
}

#endif /* HLDEBUG */
static void
dump_bytes_read(ems_u8* buf, int len, char* text)
{
    printf("read from hotlink(%s): %d bytes\n", text, len);
    dump_bytes_buf(buf, len);
}
/*****************************************************************************/
static int
hlsend(struct hlral* hl, ems_u8* data, int num)
{
        int res;

        res = write(hl->p, data, num);
	return res;
}
/*****************************************************************************/
static int
hlsend1(struct hlral* hl, ems_u8 data)
{

#ifdef HLDEBUG
        store_bytes1(data);
#endif
#if 0
        int res;
        res = write(hl->p, &data, 1);
        if (res != 1) {
                printf("hlral[%s]: hlsend1: %s\n",
                        hl->pathname, strerror(errno));
                return (-1);
        }
#else
        if (hl->wbufidx>=sizeof(hl->wbuf)) {
            printf("hlsend1: wbuf overflow\n");
            return -1;
        }
        hl->wbuf[hl->wbufidx++]=data;
#endif
	return (0);
}
/*****************************************************************************/
/*
 * static int
 * hlsend2(struct hlral* hl, ems_u16 data)
 * {
 *         int res;
 *
 *         store_bytes2(data);
 *         res = write(hl->p, &data, 2);
 *         if (res != 2)
 *                 return (-1);
 * 	   return (0);
 * }
 */
/*****************************************************************************/
static int
hlsend4(struct hlral* hl, ems_u32 data)
{

#ifdef HLDEBUG
        store_bytes4(data);
#endif
#if 0
        int res;
        res = write(hl->p, &data, 4);
        if (res != 4)
                return (-1);
#else
        if (hl->wbufidx+4>sizeof(hl->wbuf)) {
            printf("hlsend4: wbuf overflow\n");
            return -1;
        }
        bcopy(&data, hl->wbuf+hl->wbufidx, 4);
        hl->wbufidx+=4;
#endif
        return (0);
}
/*****************************************************************************/
static int
hlflush(struct hlral* hl)
{
    int res=0;
    if (hl->wbufidx) {
        res=write(hl->p, hl->wbuf, hl->wbufidx);
        if (res!=hl->wbufidx) {
            printf("hlflush: %s\n", strerror(errno));
        }
        hl->wbufidx=0;
    }
    return res;
}
/*****************************************************************************/
static int
hlread(struct hlral* hl, ems_u8* data, int num)
{
        if (hlflush(hl)<0)
            return -1;
        return read(hl->p, data, num);
}
/*****************************************************************************/
static int
selectcol(struct hlral* hl, int col)
{
	int res=0, i;

	res=hlsend1(hl, STP_FCLK);
	for (i = 0; i < col; i++) {
		res|=hlsend1(hl, STP_FCLK | CTL_LD_DAC);
		res|=hlsend1(hl, STP_FCLK);
	}
        hlflush(hl);
        return res;
}
/*****************************************************************************/
int
hlral_real_reset(struct hlral* hl)
{
	int res=0;

#ifdef HLRESET
/* the AMCC HotLink driver does not have HLRESET */
	res = ioctl(hl->p, HLRESET);
        printf("hlral: HLRESET: %s\n", strerror(errno));
	if (res<0 && errno!=ENOTTY) {
                printf("hlral[%s]: HLRESET: returning -1\n", hl->pathname);
		return -1;
        }
#endif

	res|=hlsend1(hl, CTL_RESET);
	res|=hlsend1(hl, 0);
        hlflush(hl);
	if (res)
		return (-1);

	res = ioctl(hl->p, HLCLEARRCV);
	if (res)
		return (-1);

	return (0);
}
/*****************************************************************************/
int
hlral_reset(struct hlral* hl)
{
	int res=0;

	res|=hlsend1(hl, CTL_RESET);
	res|=hlsend1(hl, 0);
        hlflush(hl);
	if (res)
		return (-1);

	res = ioctl(hl->p, HLCLEARRCV);
	if (res)
		return (-1);

	return (0);
}
/*****************************************************************************/
int
hlral_countchips(struct hlral *hl, int col, int testregs)
{
        enum hl_readmode readmode=hl_readmode_simple;
	ems_u8 buf[HLRAL_MAX_ROWS * MAX_RAL_RPC + 4];
	int res, i, chips;
	int RAL_RPC;
        int pat_5, pat_a;
        if (testregs) {
            RAL_RPC=4;
            pat_5=0x05;
            pat_a=0x0a;
        } else {
#if (HLRAL_VERSION==HLRAL_ATRAP)
            RAL_RPC=16;
            pat_5=0x0d;
            pat_a=0x0e;
#else
# if (HLRAL_VERSION==HLRAL_ANKE)
            RAL_RPC=1;
            pat_5=0x05;
            pat_a=0x0a;
# endif
#endif
        }

	hlral_reset(hl);
        if (ioctl(hl->p, HLREADMODE, &readmode)<0) {
            printf("hlral[%s]: hlral_countchips: readmode: %s\n",
                    hl->pathname, strerror(errno));
            return -1;
        }
#ifdef HLDEBUG
        storage_idx=0;
#endif
	selectcol(hl, col);

	if (testregs)
	    hlsend1(hl, STP_FCLK | CTL_LD_TST);
	else
	    hlsend1(hl, STP_FCLK | CTL_LD_DAC);

	for (i = 0; i < HLRAL_MAX_ROWS * RAL_RPC; i++) {
		hlsend1(hl, SET_DREG | /*0x5*/pat_5); /* DAC value */
		hlsend1(hl, CLOCK);
	}

#ifdef HLDEBUG
        dump_bytes_written("A");
#endif
	res = hlread(hl, buf, sizeof(buf) & (-4));
#ifdef HLDEBUG
        dump_bytes_read(buf, res);
#endif
	if (res != HLRAL_MAX_ROWS * RAL_RPC) {
		printf("hlral[%s]: hlral_countchips: sent %d, got %d\n",
                        hl->pathname,
		        HLRAL_MAX_ROWS * RAL_RPC, res);
		return (-1);
	}




	for (i = 0; i < HLRAL_MAX_ROWS * RAL_RPC; i++) {
		hlsend1(hl, SET_DREG | /*0xa*/pat_a); /* DAC value */
		hlsend1(hl, CLOCK);
	}
	hlsend1(hl, STP_FCLK);

#ifdef HLDEBUG
        dump_bytes_written("B");
#endif
	res = hlread(hl, buf, sizeof(buf) & (-4));
#ifdef HLDEBUG
        dump_bytes_read(buf, res);
#endif
	if (res != HLRAL_MAX_ROWS * RAL_RPC) {
		printf("hlral[%s]: hlral_countchips: sent %d, got %d\n",
                        hl->pathname,
		        HLRAL_MAX_ROWS * RAL_RPC, res);
		return (-1);
	}
	for (i = 0; i < HLRAL_MAX_ROWS * RAL_RPC; i++) {
		if ((buf[i] & 0x0f) != pat_5)
			break;
	}
	if (i == 0) {
		//printf("hlral_countchips: no chips in col %d\n", col);
		return (0);
	}
	if (i % RAL_RPC) {
		printf("hlral[%s]: hlral_countchips: %d registers???\n",
                        hl->pathname, i);
		return (-1);
	}
	chips = i / RAL_RPC;
	for (; i < HLRAL_MAX_ROWS * RAL_RPC; i++) {
		if ((buf[i] & 0x0f) != pat_a) {
			printf("hlral[%s]: hlral_countchips: got %x at %d\n",
			       hl->pathname, buf[i], i);
			return (-2);
		}
	}
	//printf("hlral_countchips: %d chips in col %d\n", chips, col);
	return (chips);
}
/*****************************************************************************/
static plerrcode
hlral_sendline(struct hlral* hl, int col, int num, ems_u8* buf_a, int which)
{
    ems_u8 buf_b[HLRAL_MAX_ROWS*16+4];
    int i, j, res;

    D(D_DUMP, {
      printf("hlral_sendline: col=%d num=%d which=%d:\n", col, num, which);
      if (which)
        for (i=0; i<num; i++) printf("%X", ~buf_a[i]&0x3);
      else
        for (i=0; i<num; i++) printf("%X", buf_a[i]&0xf);
      printf("\n");})
      for (j=0; j<2; j++) {
        for (i=0; i<num; i++) {

          hlsend1(hl, SET_DREG|buf_a[i]);
          if (which)
            hlsend4(hl, CLOCK*0x01010101);
          else
            {
            hlsend1(hl, CLOCK);
            }
        }
        res=hlread(hl, buf_b, num);
        if (res!=num) {
          printf("hlral_sendline: sent %d, got %d\n", num, res);
          return plErr_HW;
        }
      }
      if (which) {
        for (i=0; i<num; i++) {
          if ((buf_b[i]&0x3)!=buf_a[i]) {
            printf("hlral_sendline: column %d; idx %d: expected %d, got %d\n",
                col, i, buf_a[i], buf_b[i]&0x3);
            printf("buf_a:\n");
            for (i=0; i<num; i++) printf("%X", buf_a[i]&0x3);
            printf("\n");
            printf("buf_b:\n");
            for (i=0; i<num; i++) printf("%X", buf_b[i]&0x3);
            printf("\n");
            return plErr_HW;
          }
        }
      } else {
        for (i=0; i<num; i++) {
          if ((buf_b[i]&0xf)!=buf_a[i]) {
            printf("hlral_sendline: column %d; idx %d: expected %d, got %d\n",
                col, i, buf_a[i], buf_b[i]&0xf);
            printf("buf_a:\n");
            for (i=0; i<num; i++) printf("%X", buf_a[i]&0xf);
            printf("\n");
            printf("buf_b:\n");
            for (i=0; i<num; i++) printf("%X", buf_b[i]&0xf);
            printf("\n");
            return plErr_HW;
          }
        }
      }
    return plOK;
}
/*****************************************************************************/
plerrcode
hlral_buffermode(struct hlral *hl, int mode)
{
    plerrcode pres;
    ems_u8 buf_a[HLRAL_MAX_ROWS*16+4];
    int i, j, col, chips, command;

#if (HLRAL_VERSION!=HLRAL_ATRAP)
    printf("hlral_buffermode only works for ATRAP\n");
    return plErr_Other;
#endif

    command=STP_FCLK|CTL_LD_DAC;
    col=0;
    chips=hlral_countchips(hl, col, 1);
    if (chips<0) return plErr_HW;
    while (chips>0) {
        hlral_reset(hl);
        selectcol(hl, col);
        hlsend1(hl, command);

        for (i=0; i<chips*16; i++) buf_a[i]=3;
        for (i=0; i<chips; i++) buf_a[i*16+8]=0;
        if (mode) {
            for (i=0; i<chips; i++) {
                for (j=10; j<16; j++) buf_a[i*16+j]=0;
            }
        } else {
            /* nothing */
        }
        if ((pres=hlral_sendline(hl, col, chips*16, buf_a, 1))!=plOK)
                return pres;
        col++;
        chips=hlral_countchips(hl, col, 1);
    }
    hlral_reset(hl);
    return plOK;
}
/*****************************************************************************/
plerrcode
hlral_loaddac_2(struct hlral *hl, int col, int row, int channel, int* data, int num)
{
    ems_u8 buf_a[HLRAL_MAX_ROWS*16+4];
    int i, j, k;
    int chips;
    plerrcode pres;

    #if (HLRAL_VERSION!=HLRAL_ATRAP)
    printf("hlral_loaddac_2 only works for ATRAP\n");
    return plErr_Other;
    #endif

    chips=hlral_countchips(hl, col, 1); /* only testregs possible */
    if (chips<0) return plErr_HW;
    hlral_reset(hl);
    if (!chips) {
        printf("column %d contains no chips(?).\n", col);
        return plOK;
    }
    selectcol(hl, col);
    if (row>=chips) {
        printf("hlral_loaddac_2: row %d requested but only %d rows available\n",
                row, chips);
        return (plErr_ArgRange);
    }

    hlsend1(hl, STP_FCLK|CTL_LD_DAC);

    if (num==1) {
        for (i=0; i<chips*16; i++) buf_a[i]=3;
        if (channel>=0) { /* only one channel */
            if (channel<8) {
                buf_a[row*16+7-channel]&=~1;
                for (i=0; i<8; i++) buf_a[row*16+15-i]&=*data>>i|~1;
            } else {
                buf_a[row*16+7-(channel-8)]&=~2;
                for (i=0; i<8; i++) buf_a[row*16+15-i]&=*data>>i<<1|~2;
            }
        } else if (row>=0) { /* all 16 channels of one row (board) */
            for (i=0; i<8; i++) buf_a[row*16+i]=0;
            for (i=0; i<8; i++) buf_a[row*16+15-i]=(*data>>i&1)*3;
        } else { /* all channels of all boards */
            for (j=0; j<chips; j++) {
                for (i=0; i<8; i++) buf_a[j*16+i]=0;
                for (i=0; i<8; i++) buf_a[j*16+15-i]=(*data>>i&1)*3;
            }
        }
        if ((pres=hlral_sendline(hl, col, chips*16, buf_a, 1))!=plOK)
                return pres;
    } else {
        if (channel>=0) { /*chan>=0*/
            printf("hlral_loaddac_2: only one channel but %d data\n", num);
            return (plErr_ArgNum);
        } else if (row>=0) { /*chan<0; row>=0; all 16 channels of one row (board) */
            if (num!=16) {
                printf("hlral_loaddac_2: row=%d and channel=-1 but num=%d (not 16)\n",
                        row, num);
                return (plErr_ArgNum);
            }
            for (j=0; j<8; j++) {
                for (i=0; i<chips*16; i++) buf_a[i]=3;
                buf_a[row*16+8+j]=0;
                for (i=0; i<8; i++) {
                    buf_a[row*16+i]&=(data[j]>>i|~1)&(data[j+8]>>i<<1|~2);
                }
                if ((pres=hlral_sendline(hl, col, chips*16, buf_a, 1))!=plOK) return pres;
            }
        } else { /*chan<0; row<0; all channels of all boards */
            if (num!=chips*16) {
                printf("hlral_loaddac_2: column=%d with %d chips but num=%d\n",
                        col, chips, num);
                return (plErr_ArgNum);
            }
            for (j=0; j<8; j++) {
                for (i=0; i<chips*16; i++) buf_a[i]=3;
                for (k=0; k<chips; k++) {
                    buf_a[k*16+8+j]=0;
                    for (i=0; i<8; i++) {
                        buf_a[k*16+i]&=(data[k*16+j]>>i|~1)&(data[k*16+j+8]>>i<<1|~2);
                    }
                }
                if ((pres=hlral_sendline(hl, col, chips*16, buf_a, 1))!=plOK)
                        return pres;
            }
        }
    }
    hlral_reset(hl);
    return plOK;
}
/*****************************************************************************/
static int
datapathtest(struct hlral* hl, int col, int testregs, int chips, int val)
{
    ems_u8 *buf;
    int RAL_RPC, res=0, r, mask, i, fail;

    if (testregs) {
        RAL_RPC=4;
        mask=0xf;
    } else {
#if (HLRAL_VERSION==HLRAL_ATRAP)
        RAL_RPC=16;
        mask=0x3;
#else
# if (HLRAL_VERSION==HLRAL_ANKE)
        RAL_RPC=1;
        mask=0xf;
# endif
#endif
    }
    buf=(ems_u8*)malloc(chips*RAL_RPC+4);
    if (!buf) {
        printf("datapathtest: cannot allocate %d bytes.\n", chips*RAL_RPC+4);
        return -1;
    }
    for (i=0; i<chips*RAL_RPC; i++) {
        hlsend1(hl, SET_DREG | (val & mask));
        hlsend1(hl, CLOCK);
    }
    r=hlread(hl, buf, chips*RAL_RPC);
    if (r!=chips*RAL_RPC) {
        printf("datapathtest: sent %d, got %d\n", chips*RAL_RPC, r);
        res=-1; goto raus;
    }
    for (i=0; i<chips*RAL_RPC; i++) {
        hlsend1(hl, SET_DREG | (~val&mask));
        hlsend1(hl, CLOCK);
    }
    r=hlread(hl, buf, chips*RAL_RPC);
    if (r!=chips*RAL_RPC) {
        printf("datapathtest: sent %d, got %d\n", chips*RAL_RPC, r);
        res=-1; goto raus;
    }
    fail=0;
    for (i=0; i<chips*RAL_RPC; i++) {
        if ((buf[i]^val)&mask) fail=1;
    }
    if (fail) {
        printf("datapathtest a %s(0x%x):\n", testregs?"testregs":"dacregs", val);
        for (i=0; i<chips*RAL_RPC; i++) printf("%x ", buf[i]);
        printf("\n");
        res=-1; goto raus;
    }

    for (i=0; i<chips*RAL_RPC; i++) {
        hlsend1(hl, SET_DREG|((i%2?val:~val)&mask));
        hlsend1(hl, CLOCK);
        }
    r=hlread(hl, buf, chips*RAL_RPC);
    if (r!=chips*RAL_RPC) {
        printf("datapathtest: sent %d, got %d\n", chips*RAL_RPC, r);
        res=-1; goto raus;
    }
    for (i=0; i<chips*RAL_RPC; i++) {
        hlsend1(hl, SET_DREG|(~val&mask));
        hlsend1(hl, CLOCK);
    }
    r=hlread(hl, buf, chips*RAL_RPC);
    if (r!=chips*RAL_RPC) {
        printf("datapathtest: sent %d, got %d\n", chips*RAL_RPC, r);
        res=-1; goto raus;
    }
    fail=0;
    for (i=0; i<chips*RAL_RPC; i++) {
        if ((buf[i]^(i%2?val:~val))&mask) fail=1;
    }
    if (fail) {
        printf("datapathtest b %s(0x%x):\n", testregs?"testregs":"dacregs", val);
        for (i=0; i<chips*RAL_RPC; i++) printf("%x ", buf[i]);
        printf("\n");
        res=-1; goto raus;
    }

    raus:
    free(buf);
    return res;
}
/*****************************************************************************/
int hlral_datapathtest(struct hlral *hl, int col, int testregs)
{
    int chips, i;
    chips=hlral_countchips(hl, col, testregs);
    if (chips<1) {
        printf("hlral_datapathtest: no chips found!\n");
        return -1;
    }
    hlral_reset(hl);
    selectcol(hl, col);

    if (testregs)
        hlsend1(hl, STP_FCLK | CTL_LD_TST);
    else
        hlsend1(hl, STP_FCLK | CTL_LD_DAC);

    for (i=0; i<0xf; i++) {
        if (datapathtest(hl, col, testregs, chips, i)<0) return -1;
    }

    return 0;
}
/*****************************************************************************/
int
hlral_loadregs(struct hlral *hl, int col, int which, ems_u8 *data, int len)
{
        enum hl_readmode readmode=hl_readmode_simple;
	ems_u8 buf[HLRAL_MAX_ROWS * 4 + 4];
	int res, i;

        if (len>(sizeof(buf)&(-4))) {
            printf("hlral_loadregs: len=%d buf=%llu\n",
                    len, (unsigned long long)sizeof(buf)&(-4));
            return -1;
        }

	hlral_reset(hl);
        if (ioctl(hl->p, HLREADMODE, &readmode)<0) {
            printf("hlral[%s]: hlral_startreadout: readmode: %s\n",
                    hl->pathname, strerror(errno));
            return -1;
        }

	selectcol(hl, col);

	if (which)
		hlsend1(hl, STP_FCLK | CTL_LD_DAC);
	else
		hlsend1(hl, STP_FCLK | CTL_LD_TST);

	for (i = 0; i < len; i++) {
		hlsend1(hl, SET_DREG | *data++);
		hlsend1(hl, CLOCK);
	}
	hlsend1(hl, STP_FCLK);

	res = hlread(hl, buf, sizeof(buf) & (-4));
	if (res != len) {
		printf("hlral_loadregs: sent %d, got %d\n",
		       len, res);
		return (-1);
	}

	return (0);
}
/*****************************************************************************/
int
hlral_read_old_data(struct hlral *hl, int col, int which, ems_u8* dest)
{
        enum hl_readmode readmode=hl_readmode_simple;
        int LEN=HLRAL_MAX_ROWS * MAX_RAL_RPC;
	int res, i;

	hlral_reset(hl);
        if (ioctl(hl->p, HLREADMODE, &readmode)<0) {
            printf("hlral[%s]: hlral_startreadout: readmode: %s\n",
                    hl->pathname, strerror(errno));
            return -1;
        }

	selectcol(hl, col);

	if (which)
		hlsend1(hl, STP_FCLK | CTL_LD_DAC);
	else
		hlsend1(hl, STP_FCLK | CTL_LD_TST);

        /* flush and read old content */
	for (i = 0; i < HLRAL_MAX_ROWS * MAX_RAL_RPC; i++) {
		hlsend1(hl, SET_DREG | 0);
		hlsend1(hl, CLOCK);
	}
	res = hlread(hl, dest, LEN);
	hlsend1(hl, STP_FCLK);
        hlflush(hl);
	if (res != LEN) {
		printf("hlral_read_old_data: sent %d, got %d\n", LEN, res);
		return -1;
	}
        /*dump_bytes_read(dest, dest, "read_old_data");*/

	return res;
}
/*****************************************************************************/
int
hlral_loadregs_XXX(struct hlral *hl, int col, int which, ems_u8 *data,
        int len, ems_u8 *dest)
{
        enum hl_readmode readmode=hl_readmode_simple;
	ems_u8 buf0[HLRAL_MAX_ROWS * MAX_RAL_RPC + 4];
	ems_u8 buf1[HLRAL_MAX_ROWS * MAX_RAL_RPC + 4];
	int res, i;
        static int n=0;

        if (len>(sizeof(buf0)&(-4))) {
            printf("hlral_loadregs: len=%d buf=%llu\n",
                    len, (unsigned long long)sizeof(buf0)&(-4));
            return -1;
        }

	hlral_reset(hl);
        if (ioctl(hl->p, HLREADMODE, &readmode)<0) {
            printf("hlral[%s]: hlral_startreadout: readmode: %s\n",
                    hl->pathname, strerror(errno));
            return -1;
        }

	selectcol(hl, col);

	if (which)
		hlsend1(hl, STP_FCLK | CTL_LD_DAC);
	else
		hlsend1(hl, STP_FCLK | CTL_LD_TST);

/* fill regs with '0' */
	for (i = 0; i < HLRAL_MAX_ROWS * MAX_RAL_RPC; i++) {
		hlsend1(hl, SET_DREG | ((i+n)&0xf));
		hlsend1(hl, CLOCK);
	}
	res = hlread(hl, buf1, sizeof(buf1) & (-4));
	if (res != HLRAL_MAX_ROWS * MAX_RAL_RPC) {
		printf("hlral_loadregs(1): sent %d, got %d\n", len, res);
		return (-1);
	}
        dump_bytes_read(buf1, res, "after fill");

/* fill in the data */
	for (i = 0; i < len; i++) {
		hlsend1(hl, SET_DREG | *data++);
		hlsend1(hl, CLOCK);
	}
	res = hlread(hl, buf1, sizeof(buf1) & (-4));
	if (res != len) {
		printf("hlral_loadregs(2): sent %d, got %d\n", len, res);
		return (-1);
	}
        dump_bytes_read(buf1, res, "after write");

/* and flush it again */
	for (i = 0; i < HLRAL_MAX_ROWS * MAX_RAL_RPC; i++) {
		hlsend1(hl, SET_DREG | 0x5);
		hlsend1(hl, CLOCK);
	}
	res = hlread(hl, buf1, sizeof(buf1) & (-4));
	if (res != HLRAL_MAX_ROWS * MAX_RAL_RPC) {
		printf("hlral_loadregs(3): sent %d, got %d\n", len, res);
		return (-1);
	}
        dump_bytes_read(buf1, res, "after flush");

	hlsend1(hl, STP_FCLK);
        hlflush(hl);

        n++;
	return (0);
}
/*****************************************************************************/
#if 0
plerrcode
hlral_loadtregs(int col, int* data, int len)
{
    ems_u8 buf[HLRAL_MAX_ROWS * 4 + 4];
    int chips, i;
    plerrcode pres;

    chips=hlral_countchips(col, 1 /*testregs*/);

    hlral_reset();
    selectcol(col);

    hlsend1(STP_FCLK | CTL_LD_TST);
    for (i=0; i<chips; i++) {
        for (j=0; j<4; j++)
                buf[4*i+j]=0;
        for (k=0; k<4; k++) {
            if (data[i]>>(4*j+k)&1) buf[4*i+j]|=1<<k;
        }
    }
    return ((hlral_sendline(col, chips*4, buf, 0 /*!daqreqs*/))!=plOK)
}
#endif
/*****************************************************************************/

ems_u32 *
hlral_testreadout(struct hlral *hl, ems_u32 *data)
{
        enum hl_readmode readmode=hl_readmode_dma;
	int res;
	ems_u8 *p;
	int got;

	p = (ems_u8*)(data + 1);
	got = 0;

	if (hlral_reset(hl)<0) {
            printf("hlral_testreadout(%s): hlral_reset failed\n", hl->pathname);
            return 0;
        }

	if (hl->dma_len) {
                if (ioctl(hl->p, HLREADMODE, &readmode)<0) {
                    printf("hlral[%s]: hlral_testreadout: readmode: %s\n",
                            hl->pathname, strerror(errno));
                    return 0;
                }
		res = ioctl(hl->p, HLSTARTDMA, 0);
		if (res < 0) {
			printf("hlral_testreadout: startdma: %s\n", strerror(errno));
                        return 0;
                }
	}

	hlsend1(hl, 0 | SET_DREG); /* no external strobe */
	hlsend1(hl, CTL_LD_TST);
	hlsend1(hl, STROBE);
	hlsend1(hl, 0);
	hlsend1(hl, ROUT_DIRECT);

	if (hl->dma_len) {
		struct hlstopdma sd;

		sd.wait = 1;
		res = ioctl(hl->p, HLSTOPDMA, &sd);
		if (res < 0) {
			printf("hlral_testreadout: stopdma: %s\n", strerror(errno));
                        return 0;
                }

		bcopy(hl->dma_buf, p, sd.len);
		p += sd.len;
		got = sd.len;
		if (sd.alldata)
			goto gotall;
	}

        res = hlread(hl, p, 1048576); /* XXX */
	if (res < 0)
            return (0);
	got += res;

gotall:
	if (got <= 0) /* must have something */
		return (0);
	*data = got; /* counter as header */

	return (data + (got + 3) / 4 + 1);
}
/*****************************************************************************/
/*static int ralmode[MAXHLRAL];*/

ems_u32 *
hlral_readout(struct hlral *hl, ems_u32 *data)
{
	int res;
	ems_u8 *p;
	int got;
	p = (ems_u8*)(data + 1);
	got = 0;

	if (hl->dma_len) {
		struct hlstopdma sd;
		sd.wait = 1;
                sd.len=12345;
		res = ioctl(hl->p, HLSTOPDMA, &sd);
		if (res < 0) {
			printf("hlral[%s]: hlral_readout: stopdma: %s\n",
                                hl->pathname, strerror(errno));
                        return 0;
                }

		bcopy(hl->dma_buf, p, sd.len);
		p += sd.len;
		got = sd.len;
		if (sd.alldata)
			goto gotall;
	}

	res = hlread(hl, p, 1048576); /* XXX */
	if (res < 0) {
		printf("hlral[%s]: hlral_readout: read %d: %s\n",
                        hl->pathname, res, strerror(errno));
		return (0);
	}
	got += res;
gotall:
	if (got <= 0) { /* must have something */
#if 1
		complain("hlral[%s]: hlral_readout: got %d\n",
                        hl->pathname, got);
		return (0);
#else
                if (got < 0) {
                        complain("hlral[%s]: hlral_readout: got %d",
                                hl->pathname, got);
                        return (0);
                } else {
                        complain("hlral[%s]: hlral_readout: no data",
                                hl->pathname);
                }
#endif
	}
	*data = got; /* counter as header */

	if (hl->dma_len) {
		res = ioctl(hl->p, HLSTARTDMA, 0);
		if (res < 0) {
			printf("hlral[%s]: hlral_readout: startdma: %s\n",
                                hl->pathname, strerror(errno));
                        return 0;
                }
	}

	if (hl->buffermode)
		hlsend1(hl, ROUT_ACT_TEST);
	else
		hlsend1(hl, ROUT_ACT_NORM);
        hlflush(hl);
	return (data + (got + 3) / 4 + 1);
}
/*****************************************************************************/
void
hlral_startreadout(struct hlral *hl, int mode)
{
        enum hl_readmode readmode=hl_readmode_dma;
	int res;

	hlral_reset(hl);

	if (hl->dma_len) {
                if (ioctl(hl->p, HLREADMODE, &readmode)<0) {
                    printf("hlral[%s]: hlral_startreadout: readmode: %s\n",
                            hl->pathname, strerror(errno));
                    return;
                }
		res = ioctl(hl->p, HLSTARTDMA, 0);
		if (res < 0) {
			printf("hlral[%s]: hlral_startreadout: startdma: %s\n",
                            hl->pathname, strerror(errno));
                        return;
                }
	}

	hlsend1(hl, 0 | SET_DREG); /* no external strobe */

	if (mode & 2)
		hlsend1(hl, CTL_LD_TST); /* use test registers */
	mode &= 1;

	if (mode)
		hlsend1(hl, ROUT_ACT_TEST); /* total readout */
	else
		hlsend1(hl, ROUT_ACT_NORM);
        hlflush(hl);
	hl->buffermode=mode;
}
/*****************************************************************************/
int
pcihotlink_eoddelay(struct hlral *hl, ems_u32* eoddelay)
{
#ifdef HLEODDELAY
    int res;

    res = ioctl(hl->p, HLEODDELAY, eoddelay);
    if (res < 0)
        printf("HLEODDELAY: %s\n", strerror(errno));
    return res;
#else
    printf("pcihotlink_eoddelay: HLEODDELAY not defined\n");
    return -1;
#endif
}
/*****************************************************************************/
int
pcihotlink_write(struct hlral *hl, ems_u8* buffer, int size)
{
    int res;

    res=hlsend(hl, buffer, size);
    return res;
}
/*****************************************************************************/
int
pcihotlink_read(struct hlral *hl, ems_u8* buffer, int size)
{
    int res;

    res=hlread(hl, buffer, size);
    return res;
}
/*****************************************************************************/
int
pcihotlink_start(struct hlral *hl)
{
    int res;

    if ((res=ioctl(hl->p, HLSTARTDMA, 0))<0)
        printf("pcihotlink_start: %s\n", strerror(errno));
    return res;
}
/*****************************************************************************/
/*****************************************************************************/
