/*
 * procs/fastbus/fb_lecroy/fb_lc1881_read_seq.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fb_lc1881_read_seq.c,v 1.11 2017/10/09 21:25:37 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../../lowlevel/fastbus/sis3100_sfi/sis3100_sfifastbus.h"
#include "../../../lowlevel/fastbus/sis3100_sfi/sis3100_sfi.h"
#if PERFSPECT
#include "../../../lowlevel/perfspect/perfspect.h"
#endif
#include "../../../objects/domain/dom_ml.h"
#include "../../../objects/pi/readout.h"
#include "../../procs.h"
#include "../../procprops.h"
#include "../fastbus_verify.h"
#include "dev/vme/sfi_map.h"

#define MAX_WC  65                    /* max number of words for single event */
static const ems_u32 mist=0x0815;

extern ems_u32* outptr;
extern int *memberlist;

#if PERFSPECT
extern int perfbedarf;
#endif

RCS_REGISTER(cvsid, "procs/fastbus/fb_lecroy")

/******************************************************************************/
char name_proc_fb_lc1881_load_sequencer[]="fb_lc1881_load_seq";
int ver_proc_fb_lc1881_load_sequencer=1;

/*
Readout of Le Croy ADC 1881
===========================

parameters:
-----------
  p[0]:                 2 (number of parameters)
  p[1]:                 sequencer address / 0x100
  p[2]:                 broadcast class for LNE
*/
/******************************************************************************/
plerrcode proc_fb_lc1881_load_sequencer(ems_u32* p)
{
    struct fastbus_dev* dev;
    struct fastbus_sis3100_sfi_info* sfi_info;
    struct sficommand comm[20];
    int max_wc;
    int i, pa;

    dev=ModulEnt(1)->address.fastbus.dev;
    sfi_info=(struct fastbus_sis3100_sfi_info*)dev->info;

    max_wc=memberlist[0]*(MAX_WC+1)+1;

/* find module with largest pa (==primary link) */
    pa=0;
    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        int pa_=module->address.fastbus.pa;
        if (pa_>pa) pa=pa_;
    }

    i=0;

/* !!! Test for conversion completion can not be done here !!! */

/* load next event */
    comm[i].cmd=seq_prim_csrm;     comm[i++].data=0x5|(p[2]<<4);
    comm[i].cmd=seq_rndm_w_dis;    comm[i++].data=0x400;

/* blockread */
/*
  address will be written later
    comm[i].cmd=seq_load_address;
      comm[i++].data=(u_int32_t)((caddr_t)lowbuf_extrabuffer()
                 -sfi.backbase+sfi.dma_vmebase);
*/
    comm[i].cmd=seq_prim_dsr;      comm[i++].data=pa;
    comm[i].cmd=seq_dma_r_clearwc; comm[i++].data=(max_wc & 0xffffff) | 0xa<<24;
    comm[i].cmd=seq_discon;        comm[i++].data=mist;
    comm[i].cmd=seq_store_statwc;  comm[i++].data=mist;
    comm[i].cmd=seq_disable_ram;   comm[i++].data=0;

    if (sfi_info->load_list(dev, p[1], i, comm)<0) {
        printf("fb_lc1881_load_sequencer: load failed\n");
        return plErr_HW;
    }

    return plOK;
}
/******************************************************************************/
plerrcode test_proc_fb_lc1881_load_sequencer(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={LC_ADC_1881M, 0};
    struct fastbus_dev* dev;

    T(test_proc_fb_lc1881_init_sequencer)

    {
        int i;
        printf("lc1881_load_sequencer: p[0]=%d\n", p[0]);
        for (i=1; i<=p[0]; i++) printf("%d ", p[i]);
        printf("\n");
    }

    if (p[0]!=2) return plErr_ArgNum;

    if ((res=test_proc_fastbus(memberlist, mtypes))!=plOK) return res;

    dev=ModulEnt(1)->address.fastbus.dev;
    if (dev->fastbustype!=fastbus_sis3100_sfi) {
        printf("lc1881_init_sequencer requires sis3100_sfi\n");
        return plErr_Other;
    }

    if ((unsigned int)p[2]>3) {
        printf("lc1881_init_sequencer: bc=%d\n", p[2]);
        *outptr++=p[1]; *outptr++=1;
        return plErr_ArgRange;
    }

    wirbrauchen=0;
#if PERFSPECT
    perfbedarf=0;
#endif

    return plOK;
}
/******************************************************************************/

char name_proc_fb_lc1881_readout_sequencer[]="fb_lc1881_readout_seq";
int ver_proc_fb_lc1881_readout_sequencer=1;

/*
Readout of Le Croy ADC 1881
===========================

parameters:
-----------
  p[0]:                 1 (number of parameters)
  p[1]:                 sequencer address/0x100

outptr:
-------
  ptr[ 0]:               number of read out data words (n_words)

 data words for all modules in IS:
  ptr[1...n_words]:      header & data words

 structure for each read out module (start: n=1):
  ptr[ n]:               header word (containing word count wc)
  ptr[(n+1)...(n+wc)]:   data words
*/
/******************************************************************************/
plerrcode proc_fb_lc1881_readout_sequencer(ems_u32* p)
{
    struct fastbus_dev* dev;
    struct fastbus_sis3100_sfi_info* sfi_info;
    int wc, max_wc;
    ems_u32 wc_ss, ss;
    int res=0;

    dev=ModulEnt(1)->address.fastbus.dev;
    sfi_info=(struct fastbus_sis3100_sfi_info*)dev->info;

    max_wc=memberlist[0]*(MAX_WC+1)+1;
    if (sfi_info->ram_size-sfi_info->ram_free<max_wc) {
        printf("lc1881_readout_sequencer: count=%d but available ramsize only %Ld\n",
            max_wc, (long long int)(sfi_info->ram_size-sfi_info->ram_free));
        return plErr_HW;
    }
                                              /* E10094 <- 84000000+ram_free */
    SEQ_W(dev, seq_load_address, SFI_VME_MEM+sfi_info->ram_free);
    SEQ_W(dev, seq_enable_ram+(p[1]>>2), mist);

    res=sis3100_sfi_read_fifoword(dev, &wc_ss, &ss);
    if (res==0) {
        int bc, ssr;
        ssr=wc_ss>>24; /* ss | FB timeout | VME timeout */
        wc=wc_ss&0xffffff;
        if (wc>=max_wc) {
            printf("lc1881_readout_seq: max_wc=%d, wc=%d\n", max_wc, wc);
            return plErr_HW;
        }
        if (ssr&0x30) { /* VME or FB timeout */
            if (ssr&0x10) printf("lc1881_readout_seq: FB timeout\n");
            if (ssr&0x20) printf("lc1881_readout_seq: VME timeout\n");
            return plErr_HW;
        }
        ss=ssr&7;
        if (ss!=2) {
            printf("lc1881_readout_sequencer: ss=%d\n", ss);
            return plErr_HW;
        }
        wc--; /* last word is dummy */
        bc=wc*4;

#ifdef DELAYED_READ
        if (inside_readout && dev->generic.delayed_read_enabled) {
            if (sis3100_sfi_delay_read(dev, sfi_info->ram_free,
                    (ems_u8*)outptr, bc)) {
                return plErr_HW;
            }
            sfi_info->ram_free+=bc;
        } else
#endif
        {
            res=pread(sfi_info->p_mem, outptr, bc, sfi_info->ram_free);
            if (res!=bc) {
                if (res<0) {
                    printf("lc1881_readout_seq: read: %s\n", strerror(errno));
                } else {
                    printf("lc1881_readout_seq: read: wc=%d res=%d\n", wc, res);
                }
                return plErr_HW;
            }
        }
        if (!(ssr&0x08)) wc++; /* DMA not stopped by limit counter */
    } else { /* fatal error */
        printf("lc1881_readout_seq: error from sis3100_sfi_read_fifoword\n");
        return plErr_HW;
    }

    outptr+=wc;
    return plOK;
}
/******************************************************************************/

plerrcode test_proc_fb_lc1881_readout_sequencer(ems_u32* p)
  /* test subroutine for "fb_lc1881_readout_sequencer" */
{
    plerrcode res;
    ems_u32 mtypes[]={LC_ADC_1881M, 0};
    struct fastbus_dev* dev;

    T(test_proc_fb_lc1881_readout_sequencer)
    {
        int i;
        printf("lc1881_readout_sequencer: p[0]=%d\n", p[0]);
        for (i=1; i<=p[0]; i++) printf("%d ", p[i]);
        printf("\n");
    }

    if (p[0]!=1) return plErr_ArgNum;

    if ((res=test_proc_fastbus(memberlist, mtypes))!=plOK) return res;

    dev=ModulEnt(1)->address.fastbus.dev;
    if (dev->fastbustype!=fastbus_sis3100_sfi) {
        printf("lc1881_readout_sequencer requires sis3100_sfi\n");
        return plErr_Other;
    }

    wirbrauchen=memberlist[0]*(MAX_WC+1);
#if PERFSPECT
    perfbedarf=0;
#endif

    return plOK;
}

/******************************************************************************/
