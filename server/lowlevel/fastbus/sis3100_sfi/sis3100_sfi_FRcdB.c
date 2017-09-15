/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_FRcdB.c
 * created Oct.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_FRcdB.c,v 1.12 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <unistd.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"
#include "../../../objects/pi/readout.h"
#include "../../perfspect/perfspect.h"

static volatile ems_u32 mist=0x0815;
extern ems_u8 sis3100_sfi_deadbeef_prompt[];
extern ems_u8 sis3100_sfi_deadbeef_delayed[];

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

#undef PERFSPECT_FRcdB
#ifdef PERFSPECT_FRcdB

#define PERFSPECT_NEEDS 4

static char* PERFNAMES[]={
    "FRdcB-DMAstart",
    "FRdcB-DMAend",
    "FRdcB-FIFOa",
    "FRdcB-FIFOb"
};
/*
 * FRdcB_read_fifoword is cloned from sis3100_sfi_read_fifoword
 * only perfspect_record_time is added
 */
static int
FRdcB_read_fifoword(struct fastbus_dev* dev, ems_u32* data, int* ss)
{
    ems_u32 flags;
    int res;

SIS3100_CHECK_BALANCE(dev, "read_fifoword_0");
    if ((res=sis3100_sfi_wait_sequencer(dev, ss))==0) {
/*3*/   perfspect_record_time("FRdcB3");
SIS3100_CHECK_BALANCE(dev, "read_fifoword_1");
        if (SFI_R(dev, fifo_flags, &flags), flags&0x10) { /* FIFO empty */
            volatile ems_u32 d;
            SFI_R(dev, read_seq2vme_fifo, &d); /* dummy read */
            (void)d;
            SFI_R(dev, fifo_flags, &flags);
            if (flags&0x10) { /* FIFO empty */
                printf("sis3100_sfi_read_fifoword: FIFO empty\n");
                return -1;
            }
        }
/*4*/   perfspect_record_time("FRdcB4a");
        SFI_R(dev, read_seq2vme_fifo, data);
    } else {
        perfspect_record_time("FRdcB3b");
        perfspect_record_time("FRdcB4b");
    }
    return res;
}
#define PERFSPECT_RECORD_TIME(name) perfspect_record_time(name);

#else /*PERFSPECT_FRcdB*/

#define PERFSPECT_NEEDS 0
#define PERFSPECT_RECORD_TIME(name) do {} while(0)
static char** PERFNAMES=0;
static int
FRdcB_read_fifoword(struct fastbus_dev* dev, ems_u32* data, ems_u32* ss)
{
    return sis3100_sfi_read_fifoword(dev, data, ss);
}

#endif /*PERFSPECT_FRcdB*/

static
int FRdcB(struct fastbus_dev* dev, ems_u32 pa, ems_u32 sa, int mod, int use_sa,
        unsigned int count, ems_u32* dest, ems_u32* ss, const char* caller)
{
    int res=0, read_is_delayed=0;
    unsigned int wc;
    ems_u32 ssr;
    struct fastbus_sis3100_sfi_info* sfi_info=
            (struct fastbus_sis3100_sfi_info*)dev->info;

    if (sfi_info->ram_size-sfi_info->ram_free<count) {
        printf("sis3100_sfi_FRdcB: count=%d but available ramsize only %Ld\n",
            count, (long long int)(sfi_info->ram_size-sfi_info->ram_free));
        errno=ENOMEM;
        return -1;
    }

#if defined(DELAYED_READ)
    if (inside_readout && dev->generic.delayed_read_enabled)
        read_is_delayed=1;
#endif

#if defined(SIS3100_SFI_DEADBEEF)
    if (!read_is_delayed) {
        int r, n=count<<2;
        r=pwrite(sfi_info->p_mem, sis3100_sfi_deadbeef_prompt, n,
                sfi_info->ram_free);
        if (r!=n) {
            printf("FRdcB: pwrite deadbeef_prompt failed: %s\n",
                    strerror(errno));
            return -1;
        }
    }
#endif

    SIS3100_CHECK_BALANCE(dev, "FRdcB_0");
    res|=SEQ_W(dev, seq_prim_dsr|mod, pa);             /* E10014 <- pa       */
    if (use_sa) res|=SEQ_W(dev, seq_secad_w, sa);      /* E10244 <- sa       */
                                              /* E10094 <- 84000000+ram_free */
    res|=SEQ_W(dev, seq_load_address, SFI_VME_MEM+sfi_info->ram_free);
                                                       /* E108A4 <- count-1  */
    res|=SEQ_W(dev, seq_dma_r_clearwc, ((count-1) & 0xffffff) | 0xa<<24);
    res|=SEQ_W(dev, seq_discon, mist);                 /* E10024 <- mist     */
/*1*/   PERFSPECT_RECORD_TIME("FRdcB1");
        sis3100_sfi_wait_sequencer(dev, &ssr);
/*2*/   PERFSPECT_RECORD_TIME("FRdcB2");
    res|=SEQ_W(dev, seq_store_statwc, mist);           /* E100E4 <- mist     */
    if (res) {
        printf("FRdcB: error loading sequencer\n");
        return -1;
    }

    res=FRdcB_read_fifoword(dev, &wc, ss);

    if (res==0) {
        int bc;
        ssr=wc>>24; /* ss | FB timeout | VME timeout */
        wc&=0xffffff;
        if (wc>count) {
            printf("FRdcB: count=%d, wc=%d\n", count, wc);
            return -1;
        }
        if (ssr&0x30) { /* VME or FB timeout */
            if (ssr&0x10) printf("FRdcB: FB timeout\n");
            if (ssr&0x20) printf("FRdcB: VME timeout\n");
            return -1;
        }
        *ss=ssr&7;
        bc=wc*4;

        SIS3100_CHECK_BALANCE(dev, "FRdcB_1");

#ifdef DELAYED_READ
        if (read_is_delayed) {
            if (sis3100_sfi_delay_read(dev, sfi_info->ram_free, (ems_u8*)dest,
                    bc)) {
                return -1;
            }
            sfi_info->ram_free+=bc;
        } else
#endif
               {
#ifdef SIS3100_SFI_DEADBEEF
            memset(dest, SIS3100_SFI_BEEF_MAGIC_LOCAL, bc);
#endif
            res=pread(sfi_info->p_mem, dest, bc, sfi_info->ram_free);
            SIS3100_CHECK_BALANCE(dev, "FRdcB_2");
            if (res!=bc) {
                if (res<0) {
                    printf("sis3100_sfi_FRdcB: read: %s\n", strerror(errno));
                } else {
                    printf("sis3100_sfi_FRdcB: read: wc=%d res=%d\n", wc, res);
                }
                return -1;
            }
        }
        if (!(ssr&0x08)) wc++; /* DMA not stopped by limit counter */
    } else if (res>0) { /* sequencer was disabled; but not fatal */
        wc=0;
    } else /*if (res<0)*/ { /* fatal error */
        printf("FRdcB: error reading wordcount\n");
        return -1;
    }

    return wc;
}

int sis3100_sfi_FRDB(struct fastbus_dev* dev, ems_u32 pa, ems_u32 sa,
        unsigned int count, ems_u32* dest, ems_u32* ss, const char* caller)
{
    return FRdcB(dev, pa, sa, 0, 1, count, dest, ss, caller);
}
void sis3100_sfi_FRDB_perfspect_needs(struct fastbus_dev* dev, int* needs,
        char* names[])
{
    int i;
    for (i=0; i<PERFSPECT_NEEDS; i++) names[i]=PERFNAMES[i];
    *needs=PERFSPECT_NEEDS;
}

int sis3100_sfi_FRDB_S(struct fastbus_dev* dev, ems_u32 pa,
        unsigned int count, ems_u32* dest, ems_u32* ss, const char* caller)
{
    return FRdcB(dev, pa, 0, 0, 0, count, dest, ss, caller);
}
void sis3100_sfi_FRDB_S_perfspect_needs(struct fastbus_dev* dev, int* needs,
        char* names[])
{
    int i;
    for (i=0; i<PERFSPECT_NEEDS; i++) names[i]=PERFNAMES[i];
    *needs=PERFSPECT_NEEDS;
}


int sis3100_sfi_FRCB(struct fastbus_dev* dev, ems_u32 pa, ems_u32 sa,
        unsigned int count, ems_u32* dest, ems_u32* ss, const char* caller)
{
    return FRdcB(dev, pa, sa, SFI_MS0, 1, count, dest, ss, caller);
}
/*****************************************************************************/
/*****************************************************************************/
