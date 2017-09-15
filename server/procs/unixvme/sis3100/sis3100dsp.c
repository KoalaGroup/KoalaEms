/*
 * procs/unixvme/sis3100/sis3100dsp.c
 * created 25.Jun.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100dsp.c,v 1.17 2011/04/06 20:30:35 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <modulnames.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/unixvme/vme.h"
#include "../../../lowlevel/devices.h"
#if PERFSPECT
#include "../../../lowlevel/perfspect/perfspect.h"
#endif
#include "../vme_verify.h"
#include "vme_dspprocs.h"

#include "dev/pci/sis1100_var.h"
#include "dev/pci/sis3100_map.h"

#ifdef DMALLOC
#include dmalloc.h
#endif

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

#define get_vmedevice(crate) \
    (struct vme_dev*)get_gendevice(modul_vme, (crate))
#define ofs(what, elem) ((size_t)&(((what *)0)->elem))

#define DSP_MEM 0xC0000000

RCS_REGISTER(cvsid, "procs/unixvme/sis3100")

/*****************************************************************************/
static plerrcode
check_dsp_present(unsigned int crate)
{
    struct vme_dev* dev;
    struct dsp_procs* dsp_procs;
    plerrcode pres;

    if ((pres=find_odevice(modul_vme, crate, (struct generic_dev**)&dev))!=plOK)
        return pres;
    dsp_procs=dev->get_dsp_procs(dev);
    if (!dsp_procs) {
        printf("sis3100dsp...: invalid controller\n");
        return plErr_BadModTyp;
    }
    if (dsp_procs->present(dev)<=0) {
        printf("sis3100dsp...: no DSP present\n");
        return plErr_HWTestFailed;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
check_mem_present(unsigned int crate)
{
    struct vme_dev* dev;
    struct mem_procs* mem_procs;
    plerrcode pres;

    if ((pres=find_odevice(modul_vme, crate, (struct generic_dev**)&dev))!=plOK)
        return pres;
    mem_procs=dev->get_mem_procs(dev);
    if (!mem_procs) {
        printf("sis3100dsp...: invalid controller\n");
        return plErr_BadModTyp;
    }
    if (mem_procs->size(dev)<=0) {
        printf("sis3100dsp...: no memory present\n");
        return plErr_HWTestFailed;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
check_mem_and_dsp_present(unsigned int crate)
{
    struct vme_dev* dev;
    struct dsp_procs* dsp_procs;
    struct mem_procs* mem_procs;
    plerrcode pres;

    if ((pres=find_odevice(modul_vme, crate, (struct generic_dev**)&dev))!=plOK)
        return pres;
    mem_procs=dev->get_mem_procs(dev);
    dsp_procs=dev->get_dsp_procs(dev);
    if (!mem_procs || !dsp_procs) {
        printf("sis3100dsp...: invalid controller\n");
        return plErr_BadModTyp;
    }
    if (dsp_procs->present(dev)<=0) {
        printf("sis3100dsp...: no DSP present\n");
        return plErr_HWTestFailed;
    }
    if (mem_procs->size(dev)<=0) {
        printf("sis3100dsp...: no memory present\n");
        return plErr_HWTestFailed;
    }
    return plOK;
}
/*****************************************************************************/
static int
find_dsp_proc(int modultype)
{
    struct vme_dspproc* dspproc=vme_dspprocs;
    while (dspproc->modultype && dspproc->modultype!=modultype) dspproc++;
    return dspproc->procid;
}
/*****************************************************************************/
static void
dump_mem(struct vme_dev* dev, unsigned int start, unsigned int size,
        const char* format)
{
    static u_int32_t* buf=0;
    static int bufsize=0;
    int l, n, i;
    struct mem_procs* mem_procs=dev->get_mem_procs(dev);

    size=(size+3)&(unsigned int)~3;

    if (bufsize<size) {
        free(buf);
        buf=malloc(size<<2);
        bufsize=size;
    }
    mem_procs->read(dev, buf, size<<2, start<<2);

    printf("\n");
    for (l=0, n=0; l<size/4; l++) {
        printf("%08x: ", start+n);
        for (i=0; i<4; i++, n++) printf(format, buf[n]);
        printf("\n");
    }
}
/*****************************************************************************/
static void
dump_mem_f(struct vme_dev* dev, unsigned int start, unsigned int size,
        const char* format)
{
    static u_int32_t* buf=0;
    static int bufsize=0;
    int l, n, i;
    struct mem_procs* mem_procs=dev->get_mem_procs(dev);
    float x;

    size=(size+3)&(unsigned int)~3;

    if (bufsize<size) {
        free(buf);
        buf=malloc(size<<2);
        bufsize=size;
    }
    mem_procs->read(dev, buf, size<<2, start<<2);

    printf("\n");
    for (l=0, n=0; l<size/4; l++) {
        printf("%08x", start+n);
        for (i=0; i<4; i++, n++) {
            x=*(float*)(buf+n);
            printf(format, x);
        }
        printf("\n");
    }
}
/*****************************************************************************/
char name_proc_sis3100dsp_reset[] = "sis3100dsp_reset";
int ver_proc_sis3100dsp_reset = 1;
/*
 * p[0]: argcount==1
 * p[1]: crate ID
 */
plerrcode proc_sis3100dsp_reset(ems_u32* p)
{
    struct vme_dev* dev=get_vmedevice(p[1]);
    struct dsp_procs* dsp_procs=dev->get_dsp_procs(dev);

    dsp_procs->reset(dev);
    return plOK;
}

plerrcode test_proc_sis3100dsp_reset(ems_u32* p)
{
    if (p[0]!=1) return plErr_ArgNum;
    wirbrauchen=0;
    return check_dsp_present(p[1]);
}
/*****************************************************************************/
char name_proc_sis3100dsp_start[] = "sis3100dsp_start";
int ver_proc_sis3100dsp_start = 1;
/*
 * p[0]: argcount==1
 * p[1]: crate ID
 */
plerrcode proc_sis3100dsp_start(ems_u32* p)
{
    struct vme_dev* dev=get_vmedevice(p[1]);
    struct dsp_procs* dsp_procs=dev->get_dsp_procs(dev);

    dsp_procs->start(dev);
    return plOK;
}

plerrcode test_proc_sis3100dsp_start(ems_u32* p)
{
    if (p[0]!=1) return plErr_ArgNum;
    wirbrauchen=0;
    return check_dsp_present(p[1]);
}
/*****************************************************************************/
char name_proc_sis3100dsp_load[] = "sis3100dsp_load";
int ver_proc_sis3100dsp_load = 1;
/*
 * p[0]: argcount
 * p[1]: crate ID
 * p[2...]: filename
 */
plerrcode proc_sis3100dsp_load(ems_u32* p)
{
    struct vme_dev* dev=get_vmedevice(p[1]);
    struct dsp_procs* dsp_procs=dev->get_dsp_procs(dev);
    char* filename;

    xdrstrcdup(&filename, p+2);
    if (dsp_procs->load(dev, filename)) return plErr_System;
    return plOK;
}

plerrcode test_proc_sis3100dsp_load(ems_u32* p)
{
    plerrcode pres;
    if (p[0]<2) return plErr_ArgNum;
    pres=check_dsp_present(p[1]);
    if (pres!=plOK) return pres;

    if ((xdrstrlen(p+2))!=p[0]-1) return plErr_ArgNum;

    wirbrauchen=0;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis3100dsp_init[] = "sis3100dsp_init";
int ver_proc_sis3100dsp_init = 1;
/*
 * p[0]: argcount==0
 */
plerrcode proc_sis3100dsp_init(ems_u32* p)
{
/* XXX all modules must reside in the same crate */
/* XXX trigger ID can not be used yet */
    struct dsp_info dsp_info;
    struct descr_block descr_block;
    u_int32_t list[32];
    u_int32_t listpos, descrpos;
    unsigned int N;
    int n, i;

    struct vme_dev* dev=ModulEnt(1)->address.vme.dev;
    struct mem_procs* mem_procs=dev->get_mem_procs(dev);

    memset(&dsp_info, 0x5a, sizeof(struct dsp_info));
    memset(&list, 0x5a, sizeof(list));
    listpos=sizeof(struct dsp_info); /* start of descriptor blocks */
    descrpos=listpos+sizeof(list); /* start of descriptor blocks */

    for (i=1, n=0; i<=memberlist[0]; i++) {
        ml_entry* module=modullist->entry+memberlist[i];
        ems_u32 addr=module->address.vme.addr;
        int modultype=module->modultype;

        int procid=find_dsp_proc(modultype);
        if (procid<0) {
	  printf("sis3100dsp_init: no procedure for module %s (member %d)\n",
		 modulname(modultype, 0), i);
	  return plErr_BadModTyp;
        }
        if (!procid) continue; /* this module does not produce data */
        descr_block.ID=0;
        descr_block.procedure=procid;
        descr_block.addr=addr;
        N=3;
        switch (modultype) {
        case CAEN_V550:
            descr_block.procedure=procid;
            descr_block.extra.v550common.ident=
                    (i<<12)|(module->address.vme.crate<<18);
            N++;
            if (module->property_length==0) {
            } else if (module->property_length==6) {
                descr_block.procedure=CAEN_V550_function_common_mode;
                descr_block.extra.v550common.upper_threshold_0=module->property_data[0];
                descr_block.extra.v550common.pedestal_0=module->property_data[1];
                descr_block.extra.v550common.lower_threshold_0=module->property_data[2];
                descr_block.extra.v550common.upper_threshold_1=module->property_data[3];
                descr_block.extra.v550common.pedestal_1=module->property_data[4];
                descr_block.extra.v550common.lower_threshold_1=module->property_data[5];
                N+=6;
            } else {
                printf("sis3100dsp_init: invalid property_length for module "
                    "CAEN_V550 (addr=0x%x): %d instead of 0 or 6\n",
                    addr, module->property_length);
                return plErr_ArgRange;
            }
            break;
        case CAEN_V775:
            descr_block.extra.v775.ident=
                    (i<<12)|(module->address.vme.crate<<18);
            N++;
            break;
        case SIS_3800:
            descr_block.extra.sis3800.ident=
                    (i<<12)|(module->address.vme.crate<<18);
            N++;
            if (module->property_length>0) {
                descr_block.extra.sis3800.read_addr=
                        addr+module->property_data[0];
            } else {
                descr_block.extra.sis3800.read_addr=
                        addr+0x200; /* read shadow */
            }
            N++;
            break;
        }
        list[n++]=(descrpos>>2)+DSP_MEM;
        mem_procs->write(dev, (u_int32_t*)&descr_block, N*4, descrpos);
        descrpos+=N*4;
    }
    list[n]=0;
    dsp_info.in_progress=-1;
    dsp_info.stopcode=0;
    dsp_info.eventcounter=-1;
    dsp_info.bufferstart=0x1000+DSP_MEM;
    dsp_info.triggermask=0x7f;
    dsp_info.debugbuffer=0x10000+DSP_MEM;
    for (i=0; i<128; i++) dsp_info.triggerlist[i]=(listpos>>2)+DSP_MEM;
    mem_procs->write(dev, (u_int32_t*)&dsp_info, sizeof(struct dsp_info), 0);

    mem_procs->write(dev, list, sizeof(list), listpos);

    mem_procs->sis3100_mem_set_bufferstart(dev, dsp_info.bufferstart,
        ofs(struct dsp_info, bufferstart));

    return plOK;
}

plerrcode test_proc_sis3100dsp_init(ems_u32* p)
{
    plerrcode pres=plOK;
    /* really, CAEN_V1290 is not in DSP procedures!!! ,17.11.2006 */
    ems_u32 mtypes[]={CAEN_V550, CAEN_V551B, CAEN_V775, SIS_3800, SIS_3801, CAEN_V1290,0};
    if (p[0]) return plErr_ArgNum;
    if (!memberlist[0]) return plErr_BadISModList;
    if ((pres=test_proc_vme(memberlist, mtypes))!=plOK) return pres; 

    pres=check_mem_and_dsp_present(ModulEnt(1)->address.vme.crate);
    if (pres!=plOK) return pres;

    wirbrauchen=0;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis3100dsp_readout[] = "sis3100dsp_readout";
int ver_proc_sis3100dsp_readout = 1;
/*
 * p[0]: argcount==2
 * p[1]: crate ID
 * p[2]: send trigger
 */
plerrcode proc_sis3100dsp_readout(ems_u32* p)
{
#ifdef DELAYED_READ
    struct vme_dev* dev=get_vmedevice(p[1]);
    struct mem_procs* mem_procs=dev->get_mem_procs(dev);
    struct dsp_info dsp_info;
    u_int32_t val, start;
    int count=0;
    static int cc=10;

    if (p[2]) {
        mem_procs->write(dev, p+2, 4, ofs(struct dsp_info, testtrigger));
    }

    /* wait for DSP to finish */
    do {
        mem_procs->read(dev, &val, 4, ofs(struct dsp_info, in_progress));
        count++;
    } while (val /*&& (count++<10000)*/);
    if (val) {
        printf("sis3100dsp_readout: timeout\n");
        dump_mem(dev, 0, 16, "  %08x");
        return plErr_Program;
    }
    if (cc) {
        printf("sis3100dsp_readout: count=%d\n", count);
        cc--;
    }

    mem_procs->read(dev, (u_int32_t*)&dsp_info, 4*5, 0);
/*
    printf("event %d start 0x%x size 0x%x\n",
            dsp_info.eventcounter,
            dsp_info.bufferstart,
            dsp_info.buffersize);
*/
    if (dsp_info.stopcode) {
        printf("sis3100dsp_readout: stopcode=%d\n", dsp_info.stopcode);
        dump_mem(dev, 0, 16, "  %08x");
        return plErr_HW;
    }
    start=(dsp_info.bufferstart-DSP_MEM)<<2;
    if (mem_procs->read_delayed(dev, outptr, dsp_info.buffersize<<2, start,
            dsp_info.bufferstart+dsp_info.buffersize)<0)
        return plErr_System;

    outptr+=dsp_info.buffersize;
    return plOK;
#else
    printf("sis3100dsp_readout can only be used with DELAYED_READ enabled\n");
    /*
    this is an error in the configuration system
    proc_sis3100dsp_readout shoud not exist without DELAYED_READ
    */
    return plErr_Program;
#endif
}

plerrcode test_proc_sis3100dsp_readout(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2) return plErr_ArgNum;
    pres=check_mem_and_dsp_present(p[1]);
    if (pres!=plOK) return pres;

    wirbrauchen=-1; /* unknown */
    return plOK;
}

/*****************************************************************************/
char name_proc_sis3100dsp_readmem1[] = "sis3100dsp_readmem";
int ver_proc_sis3100dsp_readmem1 = 1;
/*
 * p[0]: argcount==3
 * p[1]: crate ID
 * p[2]: start (wordaddress)
 * p[3]: size  (number of words)
 */
plerrcode proc_sis3100dsp_readmem1(ems_u32* p)
{
    struct vme_dev* dev=get_vmedevice(p[1]);

    dump_mem(dev, p[2], p[3], "  %08x");
    return plOK;
}

plerrcode test_proc_sis3100dsp_readmem1(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3) return plErr_ArgNum;
    pres=check_mem_present(p[1]);
    if (pres!=plOK) return pres;

    wirbrauchen=0;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis3100dsp_readmem2[] = "sis3100dsp_readmem";
int ver_proc_sis3100dsp_readmem2 = 2;
/*
 * p[0]: argcount==3
 * p[1]: crate ID
 * p[2]: start (wordaddress)
 * p[3]: size  (number of words)
 */
plerrcode proc_sis3100dsp_readmem2(ems_u32* p)
{
    struct vme_dev* dev=get_vmedevice(p[1]);
    struct mem_procs* mem_procs=dev->get_mem_procs(dev);

#if PERFSPECT
    perfspect_record_time("proc_sis3100_mem2a");
#endif
    *outptr++=p[3];
    mem_procs->read(dev, outptr, p[3]<<2, p[2]<<2);
    outptr+=p[3];
#if PERFSPECT
    perfspect_record_time("proc_sis3100_mem2b");
#endif

    return plOK;
}

plerrcode test_proc_sis3100dsp_readmem2(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3) return plErr_ArgNum;
    pres=check_mem_present(p[1]);
    if (pres!=plOK) return pres;

    wirbrauchen=p[3]+1;
#if PERFSPECT
    perfbedarf=2;
#endif
    return plOK;
}

/*****************************************************************************/
char name_proc_sis3100dsp_readmem_float1[] = "sis3100dsp_readmem_float";
int ver_proc_sis3100dsp_readmem_float1 = 1;
/*
 * p[0]: argcount==3
 * p[1]: crate ID
 * p[2]: start (wordaddress)
 * p[3]: size  (number of words)
 */
plerrcode proc_sis3100dsp_readmem_float1(ems_u32* p)
{
    struct vme_dev* dev=get_vmedevice(p[1]);

    dump_mem_f(dev, p[2], p[3], "  %f");
    return plOK;
}

plerrcode test_proc_sis3100dsp_readmem_float1(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3) return plErr_ArgNum;
    pres=check_mem_present(p[1]);
    if (pres!=plOK) return pres;

    wirbrauchen=0;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis3100dsp_writemem[] = "sis3100dsp_writemem";
int ver_proc_sis3100dsp_writemem = 1;
/*
 * p[0]: argcount
 * p[1]: crate ID
 * p[2]: start (wordaddress)
 * p[3...]: data
 */
plerrcode proc_sis3100dsp_writemem(ems_u32* p)
{
    unsigned int size;
    struct vme_dev* dev=get_vmedevice(p[1]);
    struct mem_procs* mem_procs=dev->get_mem_procs(dev);

    size=p[0]-2;

    mem_procs->write(dev, p+3, size<<2, p[2]<<2);

    return plOK;
}

plerrcode test_proc_sis3100dsp_writemem(ems_u32* p)
{
    plerrcode pres;

    if (p[0]<2) return plErr_ArgNum;
    pres=check_mem_present(p[1]);
    if (pres!=plOK) return pres;

    wirbrauchen=0;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis3100dsp_fillmem[] = "sis3100dsp_fillmem";
int ver_proc_sis3100dsp_fillmem = 1;
/*
 * p[0]: argcount
 * p[1]: crate ID
 * p[2]: start (wordaddress)
 * p[3]: num
 * p[4]: data
 */
plerrcode proc_sis3100dsp_fillmem(ems_u32* p)
{
    int i;
    struct vme_dev* dev=get_vmedevice(p[1]);
    struct mem_procs* mem_procs=dev->get_mem_procs(dev);

    for (i=0; i<p[3]; i++) {
        mem_procs->write(dev, p+4, 4, (p[2]+i)<<2);
    }

    return plOK;
}

plerrcode test_proc_sis3100dsp_fillmem(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=4) return plErr_ArgNum;
    pres=check_mem_present(p[1]);
    if (pres!=plOK) return pres;

    wirbrauchen=0;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis3100dsp_memsize[] = "sis3100dsp_memsize";
int ver_proc_sis3100dsp_memsize = 1;
/*
 * p[0]: argcount==1
 * p[1]: crate ID
 */
plerrcode proc_sis3100dsp_memsize(ems_u32* p)
{
    struct vme_dev* dev=get_vmedevice(p[1]);
    struct mem_procs* mem_procs=dev->get_mem_procs(dev);

    if (mem_procs)
        *outptr++=mem_procs->size(dev);
    else
        *outptr++=0;
    return plOK;
}

plerrcode test_proc_sis3100dsp_memsize(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_vme, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=1;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis3100dsp_divide[] = "sis3100dsp_divide";
int ver_proc_sis3100dsp_divide = 1;
/*
 * p[0]: argcount==3
 * p[1]: crate ID
 * p[2]: numerator
 * p[3]: denominator
 */
plerrcode proc_sis3100dsp_divide(ems_u32* p)
{
    struct vme_dev* dev=get_vmedevice(p[1]);
    struct mem_procs* mem_procs=dev->get_mem_procs(dev);
    /*struct dsp_procs* dsp_procs=dev->get_dsp_procs(dev);*/
    u_int32_t eins=1;

    mem_procs->write(dev, p+2, 16, 4);
    mem_procs->write(dev, &eins, 4, 0);
    do {
        mem_procs->read(dev, &eins, 4, 0);
    } while (eins);
    dump_mem(dev, 0, 4, "  %08x");
    dump_mem_f(dev, 0, 4, "  %f");
    return plOK;
}

plerrcode test_proc_sis3100dsp_divide(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3) return plErr_ArgNum;
    pres=check_mem_and_dsp_present(p[1]);
    if (pres!=plOK) return pres;

    wirbrauchen=1;
    return plOK;
}
/*****************************************************************************/
/*****************************************************************************/
