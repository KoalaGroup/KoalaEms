/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_wait_sequencer.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_wait_sequencer.c,v 1.7 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"

#undef DEBUG_DATACYCLE

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

/*
    sis3100_sfi_wait_sequencer waits until the sequencer is idle
      or disabled because of error
    
    in case of sequencer disabled the sequencer is cleared and restarted and
      +1 is returned
    for fatal errors -1 is returned
    otherwise 0 is returned
*/
int sis3100_sfi_wait_sequencer(struct fastbus_dev* dev, ems_u32* ss)
{
    ems_u32 status, ret=1;

    SFI_R(dev, sequencer_status, &status);
    if (status==0) {
        printf("sis3100_sfi_wait_sequencer: sequencer never started\n");
        return -1;
    }
    while ((status&0x8000)==0) SFI_R(dev, sequencer_status, &status);
    /*if (status&0x00f0) printf("sequencer status=0x%08x\n", status);*/

    if ((status&0x8001)==0x8001) {
        *ss=0;
        return 0;
    }

    /* Error */
    switch (status&0x00f0) {
    case 0x10:   /* invalid sequencer command */
        printf("wait_sequencer: invalid command\n");
        ret=-1;
        break;
    case 0x20: { /* error on arbitration or prim. address cycle */
        ems_u32 fb1;
        printf("Primary Address or Arbitration Error\n");
        SFI_R(dev, fastbus_status_1, &fb1);
        fb1&=0xfff;
        /*printf("fb1=0x%x\n", fb1);*/
        if (!(fb1&0xf0f))
            *ss=(fb1>>4)&0x7;
        else if (fb1==0x200) {
            ems_u32 pa;
            SFI_R(dev, last_primadr, &pa);
            /*printf("wait_sequencer: prim. address timeout (%d)\n", pa);*/
            *ss=8;
            ret=1;
        } else {
            printf("wait_sequencer: fb1=0x%x\n", fb1);
            ret=-1;
        }
        }
        break;
    case 0x40:   /* error on data cycle */
    case 0x80: { /* DMA error */
        ems_u32 fb2;
#ifdef DEBUG_DATACYCLE
        if ((status&0xf0)==0x80)
            printf("Blocktransfer Error\n");
        else
            printf("DataCycle Error\n");
#endif
        SFI_R(dev, fastbus_status_2, &fb2);
#ifdef DEBUG_DATACYCLE
        printf("fb2=0x%x\n", fb2);
        printf("ss=%d; DMA_ss=%d\n", (fb2>>4)&0x7, (fb2>>8)&0x7);
#endif
        *ss=(fb2>>4)&0x7;
        }
        break;
    default:
        printf("sis3100_sfi_wait_sequencer: multiple Errors???\n");
        ret=-1;
        break;
    }
    sis3100_sfi_restart_sequencer(dev);
    return ret;
}
/*****************************************************************************/
