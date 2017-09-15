/*
 * procs/unixvme/caen/v729a.c
 * created 11.Sep.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: v729a_n.c,v 1.2 2011/04/06 20:30:35 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/unixvme/vme.h"
#include "../../../lowlevel/devices.h"
#include "../vme_verify.h"
#include "../../../objects/var/variables.h"

extern ems_u32 *outptr;
extern int wirbrauchen;
extern int *memberlist;

RCS_REGISTER(cvsid, "procs/unixvme/caen")

/*
 * V729a 4-channel 12-bit 40 MHz ADC
 * A32D16 (registers) A32D32 (output buffers)
 * reserves 65536 Byte
 */
/*
 * 0x00: output buffer ch. 0/1 (D32/BLT32)
 * 0x04: output buffer ch. 2/3 (D32/BLT32)
 * 0x08: sample register
 * 0x0A: accepted event counter
 * 0x0C: rejected event counter
 * 0x0E: control/status
 * 0x10: FIFO setting register
 * 0x12: update FIFO set register
 * 0x14: reset
 * 0x16: stop
 * 0x18: DAC ofset ch.0 +
 * 0x1A: DAC ofset ch.0 -
 * 0x1C: DAC ofset ch.1 +
 * 0x1E: DAC ofset ch.1 -
 * 0x20: DAC ofset ch.2 +
 * 0x22: DAC ofset ch.2 -
 * 0x24: DAC ofset ch.3 +
 * 0x26: DAC ofset ch.3 -
 * 0xFA: 0xFAF5 (const)
 * 0xFC: <15:10>Manufacturer(==0x2) <9:0>ModuleType(==0x48)
 * 0xFE: <15:12>Version <11:0>Serial No.
 */

/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_v729pzread(ems_u32* p)
{
    int i, res;

    *outptr++=memberlist[0];
    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 base=module->address.vme.addr;
        ems_u32 ob01=base;
        ems_u32 ob23=base+4;
        ems_u16 data, samples, nbytes;
        ems_u32* help;

        /* accepted event counter */
        res=dev->read_a32d16(dev, base+0x0a, &data);
        if (res!=2) return plErr_System;

/*
 *         if (data>1) {
 *             printf("v729pread: accepted event counter=%d\n", data);
 *             return plErr_Other;
 *         }
 */
     if (var_list[p[1]].var.val==data) {
        *outptr++=0;

        return plOK;
     } else {
        *outptr++=data-var_list[p[1]].var.val;
        var_list[p[1]].var.val=data;

     }
        /* rejected event counter */
        res=dev->read_a32d16(dev, base+0x0c, &data);
        if (res!=2) return plErr_System;
/*
 *         *outptr++=data;
 */
        if (data) {
            printf("v729pzread: rejected event counter=%d\n", data);
            return plErr_Other;
        }
        /* sample register */
        res=dev->read_a32d16(dev, base+0x08, &samples);
        if (res!=2) return plErr_System;

        nbytes=(samples+1)*4;

        /* read output buffers */
        *outptr++=samples+1;
        help=outptr;
        res=dev->read_a32_fifo(dev, ob01, outptr, nbytes, 4, (ems_u32**)&outptr);
        if (res!=nbytes) {
            printf("v729pzread: read_a32_fifo(ob01, %d): res=%d\n", nbytes, res);
            return plErr_System;
        }
    
        if ((help[0]&0xe0000000)!=0xe0000000) {
            printf("v729pzread: read_a32_fifo(ob01): first word is not a header "
                    "(0x%08x)\n", help[0]);
            return plErr_Other;
        }
        if ((help[samples-1]&0x60000000)!=0x60000000) {
            printf("v729pzread: read_a32_fifo(ob01): last word not valid "
                    "(0x%08x)\n", help[samples-1]);
            return plErr_Other;
        }
        if (help[samples]&0x20000000) {
            printf("v729pzread: read_a32_fifo(ob01): too many valid words "
                    "(0x%08x)\n", help[samples]);
            return plErr_Other;
        }
        help=outptr;
        res=dev->read_a32_fifo(dev, ob23, outptr, nbytes, 4, (ems_u32**)&outptr);
        if (res!=nbytes) {
            printf("v729pzread: read_a32_fifo(ob23, %d): res=%d\n", nbytes, res);
            return plErr_System;
        }
        outptr=help;
       }
    
    return plOK;
}
/*****************************************************************************/
plerrcode test_proc_v729pzread(ems_u32* p)
{
    plerrcode pres;
    ems_u32 mtypes[]={CAEN_V729A, 0};
    ems_u16 data;
    int i, res;

    if (p[0]!=1) return plErr_ArgNum;
    if (p[1]>MAX_VAR) return(plErr_IllVar);
    if (!var_list[p[1]].len) return(plErr_NoVar);
    if (var_list[p[1]].len!=1) return(plErr_IllVarSize);
    if ((pres=test_proc_vme(memberlist, mtypes))!=plOK) return pres;

    for (i=1; i<=memberlist[0]; i++) {
        ml_entry* module=ModulEnt(i);
        struct vme_dev* dev=module->address.vme.dev;
        ems_u32 base=module->address.vme.addr;

        res=dev->read_a32d16(dev, base+0x0e, &data);
        if (res!=2) return plErr_System;
        if (data&0x30) {
            printf("v729pzread: not in acquisition mode\n");
            return plErr_HWTestFailed;
        }
    }

    wirbrauchen=memberlist[0]*(4096+1+1)*2;
    return plOK;
}

char name_proc_v729pzread[] = "v729pzread";
int ver_proc_v729pzread = 1;

/*****************************************************************************/
/*****************************************************************************/
