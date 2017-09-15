/*
 * procs/fastbus/sis3100/sis3100sfi_dsp.c
 * created 2004-03-25 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100dsp.c,v 1.4 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <modulnames.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../../lowlevel/devices.h"
#include "fb_dspprocs.h"

#include "dev/pci/sis1100_var.h"
#include "dev/pci/sis3100_map.h"

#ifdef DMALLOC
#include dmalloc.h
#endif

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

#define get_fbdevice(crate) \
    (struct fastbus_dev*)get_gendevice(modul_fastbus, (crate))
#define ofs(what, elem) ((int)&(((what *)0)->elem))

#define DSP_MEM 0xC0000000

RCS_REGISTER(cvsid, "procs/fastbus/sis3100")

/*****************************************************************************/
static plerrcode
check_dsp_present(int crate)
{
    struct fastbus_dev* dev;
    struct fastbus_sis3100_sfi_info* sfi;
    struct dsp_procs* dsp_procs;
    plerrcode pres;

    if ((pres=find_odevice(modul_fastbus, crate, (struct generic_dev**)&dev)!=plOK)
        return pres;
    if (dev->fastbustype!=fastbus_sis3100_sfi)
        return BadModTyp;
    sfi=&dev->info.sis3100_sfi;
    dsp_procs=sfi->get_dsp_procs(sfi);
    if (!dsp_procs) {
        printf("sis3100dsp(fb)...: invalid controller\n");
        return plErr_BadModTyp;
    }
    if (dsp_procs->present(dev)<=0) {
        printf("sis3100dsp(fb)...: no DSP present\n");
        return plErr_HWTestFailed;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
check_mem_present(int crate)
{
    struct fastbus_dev* dev;
    struct fastbus_sis3100_sfi_info* sfi;
    struct mem_procs* mem_procs;
    plerrcode pres;

    if ((pres=find_odevice(modul_fastbus, crate, (struct generic_dev**)&dev)!=plOK)
        return pres;
    if (dev->fastbustype!=fastbus_sis3100_sfi)
        return BadModTyp;
    sfi=&dev->info.sis3100_sfi;
    mem_procs=sfi->get_mem_procs(sfi);
    if (!mem_procs) {
        printf("sis3100dsp(fb)...: invalid controller\n");
        return plErr_BadModTyp;
    }
    if (mem_procs->size(dev)<=0) {
        printf("sis3100dsp(fb)...: no memory present\n");
        return plErr_HWTestFailed;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
check_mem_and_dsp_present(int crate)
{
    struct fastbus_dev* dev;
    struct fastbus_sis3100_sfi_info* sfi;
    struct dsp_procs* dsp_procs;
    struct mem_procs* mem_procs;
    plerrcode pres;

    if ((pres=find_odevice(modul_fastbus, crate, (struct generic_dev**)&dev)!=plOK)
        return pres;
    if (dev->fastbustype!=fastbus_sis3100_sfi)
        return BadModTyp;
    sfi=&dev->info.sis3100_sfi;
    mem_procs=sfi->get_mem_procs(sfi);
    dsp_procs=sfi->get_dsp_procs(sfi);
    if (!mem_procs || !dsp_procs) {
        printf("sis3100dsp(fb)...: invalid controller\n");
        return plErr_BadModTyp;
    }
    if (dsp_procs->present(dev)<=0) {
        printf("sis3100dsp(fb)...: no DSP present\n");
        return plErr_HWTestFailed;
    }
    if (mem_procs->size(dev)<=0) {
        printf("sis3100dsp(bf)...: no memory present\n");
        return plErr_HWTestFailed;
    }
    return plOK;
}
/*****************************************************************************/
static int
find_dsp_proc(int modultype)
{
    struct fb_dspproc* dspproc=fb_dspprocs;
    while (dspproc->modultype && dspproc->modultype!=modultype) dspproc++;
    return dspproc->procid;
}
/*****************************************************************************/
static void
dump_mem(struct fastbus_dev* dev, int start, int size, const char* format)
{
    static u_int32_t* buf=0;
    static int bufsize=0;
    int l, n, i;
    struct mem_procs* mem_procs=dev->get_mem_procs(dev);

    size=(size+3)&~3;

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
dump_mem_f(struct fastbus_dev* dev, int start, int size, const char* format)
{
    static u_int32_t* buf=0;
    static int bufsize=0;
    int l, n, i;
    struct mem_procs* mem_procs=dev->get_mem_procs(dev);
    float x;

    size=(size+3)&~3;

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
    struct fastbus_dev* dev=get_fastbusdevice(p[1]);
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
    struct fastbus_dev* dev=get_fastbusdevice(p[1]);
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
    struct fastbus_dev* dev=get_fastbusdevice(p[1]);
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
char name_proc_sis3100sfi_dsp_init[] = "sis3100sfi_dsp_init";
int ver_proc_sis3100sfi_dsp_init = 1;
/*
 * p[0]: argcount>=3
 * p[1]: index in dsp-triggerlist
 * p[2]: module (for crate, pa and type)
 * p[3]: number of arguments
 * p[4...]: arguments
 */
plerrcode proc_sis3100sfi_dsp_init(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[2]);
    struct fastbus_dev* dev=module->address.fastbus.dev;
    struct dsp_procs* dsp_procs=dev->get_dsp_procs(dev);
    struct mem_procs* mem_procs=dev->get_mem_procs(dev);
    struct dsp_info dsp_info;
    struct descr_block descr_block;
    u_int32_t list[2];
    u_int32_t listpos, descrpos;
    int procid, modultype

    liststart=sizeof(struct dsp_info); /* start of descriptor blocks */
    descrstart=liststart+sizeof(list); /* start of descriptor blocks */
    listentry=liststart+2*p[1];
    descrentry=descrstart+sizeof(struct descr_block)*p[1];

    modultype=module->modultype;
    procid=find_dsp_proc(modultype);
    if (procid<0) {
        printf("sis3100sfi_dsp_init: no procedure for module %s\n",
                modulname(modultype, 0));
        return plErr_BadModTyp;
    }

    dsp_info.triggerlist[p[1]]=address of list[2*p[1]];

    descrentry->ID=p[1];
    descrentry->procedure=procid;
    switch (modultype) {
    case PH_ADC_10C2:
        if (p[3]!=1) return plErr_ArgNum;
        descrentry->extra.pa=module->address.fastbus.pa;
        descrentry->extra.num=p[4];
        break;
    case LC_ADC_1881:
        if (p[3]!=3) return plErr_ArgNum;
        descrentry->extra.pa=module->address.fastbus.pa;
        descrentry->extra.pat=p[4];
        descrentry->extra.bc=p[4];
        descrentry->extra.num=p[4];
        break;
    case LC_TDC_1877:
        break;
    case LC_TDC_1875A:
        break;
    }

    list[0]=(descrentry>>2)+DSP_MEM;
    list[1]=0;
    mem_procs->write(dev, list, 8, liststart+p[1]*8);


    mem_procs->write(dev, (u_int32_t*)&descr_block, sizeof(struct descr_block),
            descrentry);

    dsp_info.in_progress=-1;
    dsp_info.stopcode=0;
    dsp_info.eventcounter=-1;
    dsp_info.bufferstart=0x1000+DSP_MEM;
    dsp_info.triggermask=0x7f;
    dsp_info.debugbuffer=0x10000+DSP_MEM;
    for (i=0; i<128; i++) dsp_info.triggerlist[i]=((liststart>>2)+2*i)+DSP_MEM;
    mem_procs->write(dev, (u_int32_t*)&dsp_info, sizeof(struct dsp_info), 0);

    mem_procs->write(dev, list, sizeof(list), listpos);

    mem_procs->sis3100_mem_set_bufferstart(dev, dsp_info.bufferstart,
        ofs(struct dsp_info, bufferstart));

    return plOK;
}

plerrcode test_proc_sis3100sfi_dsp_init(ems_u32* p)
{
    plerrcode pres;

    if (p[0]<3) return plErr_ArgNum;
    if (!memberlist[0]) return plErr_BadISModList;

    pres=check_mem_and_dsp_present(ModulEnt(1)->address.fastbus.crate);
    if (pres!=plOK) return pres;

    wirbrauchen=0;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis3100sfi_dsp_readout[] = "sis3100sfi_dsp_readout";
int ver_proc_sis3100sfi_dsp_readout = 1;
/*
 * p[0]: argcount==2
 * p[1]: crate ID
 * p[2]: send trigger
 */
plerrcode proc_sis3100sfi_dsp_readout(ems_u32* p)
{
    struct fastbus_dev* dev=get_fastbusdevice(p[1]);
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
}

plerrcode test_proc_sis3100sfi_dsp_readout(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2) return plErr_ArgNum;
    pres=check_mem_and_dsp_present(p[1]);
    if (pres!=plOK) return pres;

    wirbrauchen=-1; /* unknown */
    return plOK;
}
/*****************************************************************************/
char name_proc_sis3100sfi_dsp_readmem1[] = "sis3100sfi_dsp_readmem";
int ver_proc_sis3100sfi_dsp_readmem1 = 1;
/*
 * p[0]: argcount==3
 * p[1]: crate ID
 * p[2]: start (wordaddress)
 * p[3]: size  (number of words)
 */
plerrcode proc_sis3100sfi_dsp_readmem1(ems_u32* p)
{
    struct fastbus_dev* dev=get_fastbusdevice(p[1]);

    dump_mem(dev, p[2], p[3], "  %08x");
    return plOK;
}

plerrcode test_proc_sis3100sfi_dsp_readmem1(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3) return plErr_ArgNum;
    pres=check_mem_present(p[1]);
    if (pres!=plOK) return pres;

    wirbrauchen=0;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis3100sfi_dsp_readmem2[] = "sis3100sfi_dsp_readmem";
int ver_proc_sis3100sfi_dsp_readmem2 = 2;
/*
 * p[0]: argcount==3
 * p[1]: crate ID
 * p[2]: start (wordaddress)
 * p[3]: size  (number of words)
 */
plerrcode proc_sis3100sfi_dsp_readmem2(ems_u32* p)
{
    struct fastbus_dev* dev=get_fastbusdevice(p[1]);
    struct mem_procs* mem_procs=dev->get_mem_procs(dev);

    *outptr++=p[3];
    mem_procs->read(dev, outptr, p[3]<<2, p[2]<<2);
    outptr+=p[3];

    return plOK;
}

plerrcode test_proc_sis3100sfi_dsp_readmem2(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3) return plErr_ArgNum;
    pres=check_mem_present(p[1]);
    if (pres!=plOK) return pres;

    wirbrauchen=p[3]+1;
    return plOK;
}

/*****************************************************************************/
char name_proc_sis3100sfi_dsp_writemem[] = "sis3100sfi_dsp_writemem";
int ver_proc_sis3100sfi_dsp_writemem = 1;
/*
 * p[0]: argcount
 * p[1]: crate ID
 * p[2]: start (wordaddress)
 * p[3...]: data
 */
plerrcode proc_sis3100sfi_dsp_writemem(ems_u32* p)
{
    int size;
    struct fastbus_dev* dev=get_fastbusdevice(p[1]);
    struct mem_procs* mem_procs=dev->get_mem_procs(dev);

    size=p[0]-2;

    mem_procs->write(dev, p+3, size<<2, p[2]<<2);

    return plOK;
}

plerrcode test_proc_sis3100sfi_dsp_writemem(ems_u32* p)
{
    plerrcode pres;

    if (p[0]<2) return plErr_ArgNum;
    pres=check_mem_present(p[1]);
    if (pres!=plOK) return pres;

    wirbrauchen=0;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis3100sfi_dsp_fillmem[] = "sis3100sfi_dsp_fillmem";
int ver_proc_sis3100sfi_dsp_fillmem = 1;
/*
 * p[0]: argcount
 * p[1]: crate ID
 * p[2]: start (wordaddress)
 * p[3]: num
 * p[4]: data
 */
plerrcode proc_sis3100sfi_dsp_fillmem(ems_u32* p)
{
    int i;
    struct fastbus_dev* dev=get_fastbusdevice(p[1]);
    struct mem_procs* mem_procs=dev->get_mem_procs(dev);

    for (i=0; i<p[3]; i++) {
        mem_procs->write(dev, p+4, 4, (p[2]+i)<<2);
    }

    return plOK;
}

plerrcode test_proc_sis3100sfi_dsp_fillmem(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=4) return plErr_ArgNum;
    pres=check_mem_present(p[1]);
    if (pres!=plOK) return pres;

    wirbrauchen=0;
    return plOK;
}
/*****************************************************************************/
char name_proc_sis3100sfi_dsp_memsize[] = "sis3100sfi_dsp_memsize";
int ver_proc_sis3100sfi_dsp_memsize = 1;
/*
 * p[0]: argcount==1
 * p[1]: crate ID
 */
plerrcode proc_sis3100sfi_dsp_memsize(ems_u32* p)
{
    struct fastbus_dev* dev=get_fastbusdevice(p[1]);
    struct mem_procs* mem_procs=dev->get_mem_procs(dev);

    if (mem_procs)
        *outptr++=mem_procs->size(dev);
    else
        *outptr++=0;
    return plOK;
}

plerrcode test_proc_sis3100sfi_dsp_memsize(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=check_mem_present(p[1]))!=plOK)
        return pres;
    wirbrauchen=1;
    return plOK;
}
/*****************************************************************************/
/*****************************************************************************/
