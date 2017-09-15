/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_FRDB_multi.c
 * created 08.Nov.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_FRDB_multi.c,v 1.8 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <unistd.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"
#include "../../../objects/pi/readout.h"

static volatile ems_u32 mist=0xdeadbeef;
extern ems_u8 sis3100_sfi_deadbeef[];
static ems_u32* dest_ctrl=0;
static int dest_ctrl_size=0;

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

int
sis3100_sfi_FRDB_multi_init(struct fastbus_dev* dev, int seqaddr,
        unsigned int num_pa, ems_u32* pa, ems_u32 sa, unsigned int max)
{
    int i, res=0;

    if (num_pa>dest_ctrl_size) {
        free(dest_ctrl);
        dest_ctrl=malloc(num_pa*sizeof(ems_u32));
        dest_ctrl_size=num_pa;
    }

    res|=SFI_W(dev, sequencer_reset, mist);
    res|=SFI_W(dev, next_sequencer_ram_address, seqaddr<<8);
    res|=SFI_W(dev, sequencer_ram_load_enable, mist);

//    res|=SEQ_W(dev, seq_dma_r_clearwc, ((max-1)&0xffffff)|0xa<<24);
    for (i=0; i<num_pa; i++) {
        res|=SEQ_W(dev, seq_prim_dsr, pa[i]);
        if (sa>=0) res|=SEQ_W(dev, seq_secad_w, sa);
//#if 0
        if (i)
            res|=SEQ_W(dev, seq_dma_r, ((max-1)&0xffffff)|0xa<<24);
        else
            res|=SEQ_W(dev, seq_dma_r_clearwc, ((max-1)&0xffffff)|0xa<<24);
//#endif
        res|=SEQ_W(dev, seq_discon, mist);
#if 0
        res|=SEQ_W(dev, seq_store_statwc, mist);
#endif
    }
    res|=SEQ_W(dev, seq_store_statwc, mist);

    res|=SEQ_W(dev, seq_disable_ram, mist);
    res|=SFI_W(dev, sequencer_ram_load_disable, mist);
    res|=SFI_W(dev, sequencer_enable, mist);
    if (res) {
        printf("sis3100_sfi_FRDB_multi_init: load sequencer list failed\n");
        return -1;
    }
    return 0;
}

int
sis3100_sfi_FRDB_multi_read(struct fastbus_dev* dev, int seqaddr,
    ems_u32* dest, int* countp)
{
    struct fastbus_sis3100_sfi_info* sfi_info=(struct fastbus_sis3100_sfi_info*)dev->info;
    ems_u32 ss;
    int res=0, bcount/*, i, n*/;
    ems_u32 nn;

#ifdef DELAYED_READ
    int delayed_read=inside_readout && dev->generic.delayed_read_enabled;
#endif

    res|=SEQ_W(dev, seq_load_address, SFI_VME_MEM+sfi_info->ram_free);
    res|=SEQ_W(dev, seq_enable_ram|seqaddr<<6, mist);
    if (res) {
        printf("sis3100_sfi_FRDB_multi_read: list failed; res=%d\n", res);
        return -1;
    }
#if 0
    n=sis3100_sfi_read_fifo(dev, dest_ctrl, 100, &ss);
    if (n<0) {
        printf("sis3100_sfi_FRDB_multi_read: read_fifo failed\n");
        return -1;
    }

    bcount=(dest_ctrl[n-1]&0xffffff)<<2;
    for (i=0; i<n; i++) {
        if ((dest_ctrl[i]&0x38000000) /*limit reached; FASTBUS or VME timeout*/
                || ((dest_ctrl[i]&0x07000000)!=0x02000000)) /* ss!=2 */ {
            printf("FRDB_multi_read(%d, %d):\n", seqaddr, i);
            if (dest_ctrl[i]&0x08000000) printf("  Stopped by limit counter.\n");
            if (dest_ctrl[i]&0x10000000) printf("  FASTBUS timeout.\n");
            if (dest_ctrl[i]&0x20000000) printf("  VME timeout.\n");
            printf("  ss=%d\n", (dest_ctrl[i]>>24)&7);
            return -1;
        }
    }
#endif
    res=sis3100_sfi_read_fifoword(dev, &nn, &ss);
    if (res) {
        printf("sis3100_sfi_FRDB_multi_read: read_fifoword failed: %d\n", res);
        return -1;
    }
    if ((nn&0x38000000) /*limit reached; FASTBUS or VME timeout*/
            || ((nn&0x07000000)!=0x02000000)) /* ss!=2 */ {
        printf("FRDB_multi_read(%d):\n", seqaddr);
        if (nn&0x08000000) printf("  Stopped by limit counter.\n");
        if (nn&0x10000000) printf("  FASTBUS timeout.\n");
        if (nn&0x20000000) printf("  VME timeout.\n");
        printf("  ss=%d\n", (nn>>24)&7);
        return -1;
    }
    bcount=(nn&0xffffff)<<2;

#ifdef DELAYED_READ
    if (delayed_read) {
        sis3100_sfi_delay_read(dev, sfi_info->ram_free, (ems_u8*)dest, bcount);
        sfi_info->ram_free+=bcount;
    } else
#endif
    {
        res=pread(sfi_info->p_mem, dest, bcount, sfi_info->ram_free);
        if (res!=bcount) {
            printf("sis3100_sfi_FRDB_multi_read: pread failed: %s\n",
                    strerror(errno));
            return -1;
        }
    }

    *countp=bcount>>2;
    return 0;
}

/*****************************************************************************/
/*****************************************************************************/
