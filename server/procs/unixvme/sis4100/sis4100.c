/*
 * procs/unixvme/sis4100/sis4100.c
 * created 18.Sep.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis4100.c,v 1.6 2011/04/06 20:30:35 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/unixvme/vme.h"
#include "../../../lowlevel/devices.h"

#include "sis4100_map.h"

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

#define get_vmedevice(crate) \
    (struct vme_dev*)get_gendevice(modul_vme, (crate))
#define ofs(what, elem) ((off_t)&(((what *)0)->elem))

#define ngfread(module, offs, val)                                     \
({                                                                     \
    struct vme_dev* dev=module->address.vme.dev;                       \
    dev->read_a24d32(dev, (module)->address.vme.addr+offs, val);       \
})

#define ngfwrite(module, offs, val)                                    \
({                                                                     \
    struct vme_dev* dev=module->address.vme.dev;                       \
    dev->write_a24d32(dev, (module)->address.vme.addr+offs, val);      \
})

#define ngfwritereg(module, elem, val)                                 \
    ngfwrite(module, ofs(struct sfi_w, elem), val)

#define ngfreadreg(module, elem, val)                                  \
    ngfread(module, ofs(struct sfi_r, elem), val)

#define dummy 0x0815

RCS_REGISTER(cvsid, "procs/unixvme/sis4100")

/*****************************************************************************/
static void
sis4100_seq_error(ml_entry* module)
{
    u_int32_t status, fb1, fb2;
    int res;

    res=ngfreadreg(module, sequencer_status, &status);
    if (res<0) goto fehler;
    switch (status&0x00f0) {
    case 0x10: printf("sis4100_sequencer: invalid command\n"); break;
    case 0x20:
        ngfreadreg(module, fastbus_status_1, &fb1);
        if (fb1&0xf0) {
            printf("sis4100_sequencer: ss=%d during pa-cycle\n", (fb1>>4)&7);
        } else {
            printf("sis4100_sequencer: fb1=0x%x\n", fb1);
        }
        break;
    case 0x40: /* error on data cycle */
    case 0x80: /* DMA error */
        ngfreadreg(module, fastbus_status_2, &fb2);
        printf("sis4100_sequencer: ss=%d during data-cycle\n", (fb1>>4)&7);
        break;
    default: printf("sis4100_sequencer: status=0x%x\n", status);
    }
    return;

fehler:
    printf("decode sis4100_seq_error: %s\n", strerror(-res));
}
/*****************************************************************************/
char name_proc_sis4100_reset[] = "sis4100_reset";
int ver_proc_sis4100_reset = 1;
/*
 * p[0]: argcount==1
 * p[1]: Module
 */
plerrcode proc_sis4100_reset(ems_u32* p)
{
    ml_entry* module;
    int res;

    module=ModulEnt(p[1]);

    res=ngfwritereg(module, reset_register_group_lca2, dummy);
    if (res<0) goto fehler;
    res=ngfwritereg(module, sequencer_reset, dummy);
    if (res<0) goto fehler;
    return plOK;
fehler:
    printf("proc_sis4100_reset: %s\n", strerror(-res));
    *outptr++=-res;
    return plErr_System;
}

plerrcode test_proc_sis4100_reset(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=1) return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0)) return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if ((module->modultype!=SIS_4100) &&
        (module->modultype!=STR_340))
            return plErr_BadModTyp;

    wirbrauchen=0;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis4100_writereg[] = "sis4100_writereg";
int ver_proc_sis4100_writereg = 1;
/*
 * p[0]: argcount==3
 * p[1]: Module
 * p[2]: addr
 * p[3]: val
 */
plerrcode proc_sis4100_writereg(ems_u32* p)
{
    int res;

    res=ngfwrite(ModulEnt(p[1]), p[2], p[3]);
    if (res<0) goto fehler;
    return plOK;
fehler:
    printf("proc_sis4100_writereg: %s\n", strerror(-res));
    *outptr++=-res;
    return plErr_System;
}

plerrcode test_proc_sis4100_writereg(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=3) return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0)) return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if ((module->modultype!=SIS_4100) &&
        (module->modultype!=STR_340))
            return plErr_BadModTyp;
    if ((unsigned int)p[2]>0x20000) return plErr_ArgRange;

    wirbrauchen=1;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis4100_readreg[] = "sis4100_readreg";
int ver_proc_sis4100_readreg = 1;
/*
 * p[0]: argcount==2
 * p[1]: Module
 * p[2]: addr
 */
plerrcode proc_sis4100_readreg(ems_u32* p)
{
    u_int32_t val;
    int res;

    res=ngfread(ModulEnt(p[1]), p[2], &val);
    if (res<0) goto fehler;
    *outptr++=val;
    return plOK;
fehler:
    printf("proc_sis4100_readreg: %s\n", strerror(-res));
    *outptr++=-res;
    return plErr_System;
}

plerrcode test_proc_sis4100_readreg(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=2) return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0)) return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if ((module->modultype!=SIS_4100) &&
        (module->modultype!=STR_340))
            return plErr_BadModTyp;
    if ((unsigned int)p[2]>0x20000) return plErr_ArgRange;

    wirbrauchen=1;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis4100_seq_status[] = "sis4100_seq_status";
int ver_proc_sis4100_seq_status = 1;
/*
 * p[0]: argcount==1
 * p[1]: Module
 */
plerrcode proc_sis4100_seq_status(ems_u32* p)
{
    u_int32_t val;
    int res;

    res=ngfreadreg(ModulEnt(p[1]), sequencer_status, &val);
    if (res<0) goto fehler;
    *outptr++=val;
    return plOK;
fehler:
    printf("proc_sis4100_seq_status: %s\n", strerror(-res));
    *outptr++=-res;
    return plErr_System;
}

plerrcode test_proc_sis4100_seq_status(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=1) return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0)) return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if ((module->modultype!=SIS_4100) &&
        (module->modultype!=STR_340))
            return plErr_BadModTyp;

    wirbrauchen=1;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis4100_seq_enable[] = "sis4100_seq_enable";
int ver_proc_sis4100_seq_enable = 1;
/*
 * p[0]: argcount==2
 * p[1]: Module
 * p[2]: 0: disable 1: enable
 */
plerrcode proc_sis4100_seq_enable(ems_u32* p)
{
    int res;
    u_int32_t offs;

    offs=p[2]?ofs(struct sfi_w, sequencer_enable):ofs(struct sfi_w, sequencer_disable);
    res=ngfwrite(ModulEnt(p[1]), offs, dummy);
    if (res<0) goto fehler;
    return plOK;
fehler:
    printf("proc_sis4100_seq_enable: %s\n", strerror(-res));
    *outptr++=-res;
    return plErr_System;
}

plerrcode test_proc_sis4100_seq_enable(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=2) return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0)) return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if ((module->modultype!=SIS_4100) &&
        (module->modultype!=STR_340))
            return plErr_BadModTyp;

    wirbrauchen=1;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis4100_seq_write[] = "sis4100_seq_write";
int ver_proc_sis4100_seq_write = 1;
/*
 * p[0]: argcount==3
 * p[1]: Module
 * p[2]: command
 * p[3]: data
 */
plerrcode proc_sis4100_seq_write(ems_u32* p)
{
    int res;

    res=ngfwritereg(ModulEnt(p[1]), seq[p[2]], p[3]);
    if (res<0) goto fehler;
    return plOK;
fehler:
    printf("proc_sis4100_seq_write: %s\n", strerror(-res));
    *outptr++=-res;
    return plErr_System;
}

plerrcode test_proc_sis4100_seq_write(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=3) return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0)) return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if ((module->modultype!=SIS_4100) &&
        (module->modultype!=STR_340))
            return plErr_BadModTyp;

    wirbrauchen=1;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis4100_seq_read[] = "sis4100_seq_read";
int ver_proc_sis4100_seq_read = 1;
/*
 * p[0]: argcount==1
 * p[1]: Module
 */
plerrcode proc_sis4100_seq_read(ems_u32* p)
{
    ml_entry* module;
    u_int32_t status, flags, val;
    int *help, res;

    module=ModulEnt(p[1]);

    help=outptr++;
/* wait until sequencer is idle */
    res=ngfreadreg(module, sequencer_status, &status);
    if (res<0) goto fehler;
    if (status==0) {
        printf("sis4100_seq_read: sequencer never started\n");
        return plErr_HW;
    }
    while ((status&0x8000)==0) ngfreadreg(module, sequencer_status, &status);
    if ((status&0x8001)!=0x8001) { /* some error occured */
        sis4100_seq_error(module);
        return plErr_HW;
    }

/* if fifo empty read dummy word */
    ngfreadreg(module, fifo_flags, &flags);
    if (flags&0x10) ngfreadreg(module, read_seq2vme_fifo, &val);

/* read fifo until empty */
    do {
        ngfreadreg(module, fifo_flags, &flags);
        if (!(flags&0x10)) { /* fifo not empty */
            ngfreadreg(module, read_seq2vme_fifo, &val);
            *outptr++=val;
        }
    } while (!(flags&0x10));
    *help=outptr-help-1;
    return plOK;
fehler:
    printf("proc_sis4100_seq_read: %s\n", strerror(-res));
    outptr=help;
    *outptr++=-res;
    return plErr_System;
}

plerrcode test_proc_sis4100_seq_read(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=1) return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0)) return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if ((module->modultype!=SIS_4100) &&
        (module->modultype!=STR_340))
            return plErr_BadModTyp;

    wirbrauchen=1;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis4100_seq_out[] = "sis4100_seq_out";
int ver_proc_sis4100_seq_out = 1;
/*
 * p[0]: argcount==2
 * p[1]: Module
 * p[2]: val
 */
plerrcode proc_sis4100_seq_out(ems_u32* p)
{
    int res;

    res=ngfwritereg(ModulEnt(p[1]), seq[seq_out], p[3]);
    if (res<0) goto fehler;
    return plOK;
fehler:
    printf("proc_sis4100_seq_out: %s\n", strerror(-res));
    *outptr++=-res;
    return plErr_System;
}

plerrcode test_proc_sis4100_seq_out(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=2) return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0)) return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if ((module->modultype!=SIS_4100) &&
        (module->modultype!=STR_340))
            return plErr_BadModTyp;

    wirbrauchen=1;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis4100_seq_cmd[] = "sis4100_seq_cmd";
int ver_proc_sis4100_seq_cmd = 1;
/*
 * p[0]: argcount==2
 * p[1]: Module
 * p[2]: 0: reset vme_out_flag 1: set vme_out_flag
 */
plerrcode proc_sis4100_seq_cmd(ems_u32* p)
{
    int res;

    if (p[2]) {
        res=ngfwritereg(ModulEnt(p[1]), seq[seq_set_cmd_flag], dummy);
        if (res<0) goto fehler;
    } else {
        res=ngfwritereg(ModulEnt(p[1]), clear_sequencer_cmd_flag, dummy);
        if (res<0) goto fehler;
    }
    return plOK;
fehler:
    printf("proc_sis4100_seq_cmd: %s\n", strerror(-res));
    *outptr++=-res;
    return plErr_System;
}

plerrcode test_proc_sis4100_seq_cmd(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=2) return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0)) return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if ((module->modultype!=SIS_4100) &&
        (module->modultype!=STR_340))
            return plErr_BadModTyp;

    wirbrauchen=1;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis4100_vme_out[] = "sis4100_vme_out";
int ver_proc_sis4100_vme_out = 1;
/*
 * p[0]: argcount==2
 * p[1]: Module
 * p[2]: val
 */
plerrcode proc_sis4100_vme_out(ems_u32* p)
{
    int res;

    res=ngfwritereg(ModulEnt(p[1]), write_vme_out_signal, p[2]);
    if (res<0) goto fehler;
    return plOK;
fehler:
    printf("proc_sis4100_vme_out: %s\n", strerror(-res));
    *outptr++=-res;
    return plErr_System;
}

plerrcode test_proc_sis4100_vme_out(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=2) return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0)) return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if ((module->modultype!=SIS_4100) &&
        (module->modultype!=STR_340))
            return plErr_BadModTyp;

    wirbrauchen=1;
    return plOK;
}
/*****************************************************************************/
/*****************************************************************************/
