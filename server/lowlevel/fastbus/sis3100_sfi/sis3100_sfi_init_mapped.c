/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_init_mapped.c
 * 
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_init_mapped.c,v 1.7 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <rcs_ids.h>

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

#ifdef SIS3100_SFI_MMAPPED

#include "sis3100_sfi.h"
#include <sys/mman.h>
#include "sis3100_sfi_open_dev.h"

/*
 * we assume a recent version of sis1100 with 4 MByte mapping per segment
 * with one segment we can map the whole FBspace
 * but  VME may be not mappable at all (virtual address space exhausted)
 */
errcode
sis3100_sfi_init_mapped(struct fastbus_dev* dev)
{
    ems_u32 descr[4]={0xff010800, 0x39, SFI_BASE, 0};
    struct fastbus_sis3100_sfi_info* info=&dev->info.sis3100_sfi;
    struct sis1100_ctrl_reg reg;
    int i, offs;
    ems_u32 mapsize, mapbase, mapoffs;
    struct sis1100_dma_alloc dma_alloc;

    if (ioctl(info->vme_path, SIS1100_MAPSIZE, &info->vme_size)<0) {
        printf("ioctl(MAPSIZE(vme): %s\n", strerror(errno));
        return Err_System;
    }
    printf("Size of vme space: %lld MByte\n",
            (unsigned long long int)(info->vme_size>>20));
    if (ioctl(info->ctrl_path, SIS1100_MAPSIZE, &info->ctrl_size)<0) {
        printf("ioctl(MAPSIZE(ctrl): %s\n", strerror(errno));
        return Err_System;
    }
    printf("Size of control space: %lld KByte\n",
            (long long int)(info->ctrl_size>>10));

    mapsize=0x400000; /* 4 MByte in recent cards */
    if (mapsize<SFI_MAPSIZE) {
        printf("fastbus_init_sis3100_sfi: size of one page is 0x%x.\n",
                mapsize);
        return Err_Program;
    }
    mapbase=SFI_BASE&~(mapsize-1);
    mapoffs=SFI_BASE&(mapsize-1);
    printf("VME map: mapsize=0x%x mapbase=0x%x mapoffs=0x%x\n",
            mapsize, mapbase, mapoffs);

    for (i=0, offs=0x400; i<4; i++, offs+=4) {
        reg.offset=offs;
        reg.val=descr[i];
        if (ioctl(info->ctrl_path, SIS1100_CTRL_WRITE, &reg)<0) {
            printf("ioctl SIS1100_CTRL_WRITE: %s\n", strerror(errno));
            return Err_System;
        }
        /* reg.error is never set for SIS1100_CTRL */
    }
    info->vme_base=mmap(0, SFI_MAPSIZE, PROT_READ|PROT_WRITE, 0,
            info->vme_path, mapoffs);
    printf("VME map: base=%p\n", info->vme_base);
    if (info->vme_base==MAP_FAILED) {
        printf("sis3100_sfi: mmap vme: %s\n", strerror(errno));
        return Err_System;
    }
    info->ctrl_base=mmap(0, info->ctrl_size, PROT_READ|PROT_WRITE, 0,
            info->ctrl_path, 0);
    printf("CTRL map: base=%p\n", info->ctrl_base);
    if (info->ctrl_base==MAP_FAILED) {
        printf("sis3100_sfi: mmap ctrl: %s\n", strerror(errno));
        munmap((void*)info->vme_base, SFI_MAPSIZE);
        return Err_System;
    }
    CTRL_W(dev, sp1_descr[0].hdr, descr[0]);
    CTRL_W(dev, sp1_descr[0].am, descr[1]);
    CTRL_W(dev, sp1_descr[0].adl, descr[2]);
    CTRL_W(dev, sp1_descr[0].adh, descr[3]);

    info->sr=&((struct sis1100_reg*)(info->ctrl_base))->sr;
    info->p_balance=&((struct sis1100_reg*)(info->ctrl_base))->p_balance;
    info->prot_error=&((struct sis1100_reg*)(info->ctrl_base))->prot_error;

    {
        ems_u32 balance, error;
        /*for (i=0; i<10; i++) info->vme_base[i]=i;*/
        error=*info->prot_error;
        balance=*info->p_balance;
        if (error) {
            printf("init_sis3100_sfi(B): ERROR=0x%x; BALANCE=%d\n", error,
                    balance);
        }
        *info->p_balance=0;
    }

    dma_alloc.size=sysconf(_SC_PAGESIZE);
    if (dma_alloc.size==(size_t)-1) {
        printf("sis3100_sfi: sysconf: %s\n", strerror(errno));
        dma_alloc.size=4096;
    }

    if (ioctl(info->ctrl_path, SIS1100_DMA_ALLOC, &dma_alloc)<0) {
        printf("ioctl SIS1100_DMA_ALLOC: %s\n", strerror(errno));
        munmap((void*)info->vme_base, SFI_MAPSIZE);
        munmap((void*)info->ctrl_base, info->ctrl_size);
        return Err_System;
    }
    printf("DMA_ALLOC: size=%lld offs=%lld dma=0x%08x\n",
            (unsigned long long int)dma_alloc.size,
            (unsigned long long int)dma_alloc.offset,
            dma_alloc.dma_addr);
    info->pipe_size=dma_alloc.size;
    info->pipe_offset=dma_alloc.offset;
    info->pipe_dma_addr=dma_alloc.dma_addr;
    info->pipe_base=mmap(0, info->pipe_size, PROT_READ|PROT_WRITE, 0,
            info->ctrl_path, info->pipe_offset);
    printf("PIPE map: base=%p\n", info->pipe_base);
    if (info->pipe_base==MAP_FAILED) {
        printf("sis3100_sfi: mmap pipe: %s\n", strerror(errno));
        munmap((void*)info->vme_base, SFI_MAPSIZE);
        munmap((void*)info->ctrl_base, info->ctrl_size);
        ioctl(info->ctrl_path, SIS1100_DMA_FREE, &dma_alloc);
        return Err_System;
    }

    return OK;    
}
#endif
/*****************************************************************************/
/*****************************************************************************/
