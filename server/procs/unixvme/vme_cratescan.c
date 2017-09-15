/*
 * procs/unixvme/vme_cratescan.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: vme_cratescan.c,v 1.15 2011/08/13 20:21:58 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/unixvme/vme.h"
#include "../../lowlevel/devices.h"
#include "vme_verify.h"

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

#define get_device(class, crate) \
    (struct vme_dev*)get_gendevice((class), (crate))

RCS_REGISTER(cvsid, "procs/unixvme")

/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_VMEverifylist(ems_u32* p)
{
    int errors=0, i;

    if (memberlist) {
        for (i=memberlist[0]; i>0; i--) {
            ml_entry* module=ModulEnt_m(i);
            printf("VMEverifylist() i=%d\n", i);
            if (verify_vme_module(module, 1)) {
                *outptr++=i;
                errors++;
            }
        }
    } else {
        for (i=0; i<modullist->modnum; i++) {
            ml_entry* module=modullist->entry+i;
            if (module->modulclass==modul_vme) {
                if (verify_vme_module(module, 1)) {
                    *outptr++=i;
                    errors++;
                }
            }
        }
    }
    if (errors) return plErr_HWTestFailed;
    return plOK;
}

plerrcode test_proc_VMEverifylist(ems_u32* p)
{
    if (p[0]) return plErr_ArgNum;
    return plOK;
}

char name_proc_VMEverifylist[] = "VMEverifylist";
int ver_proc_VMEverifylist = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: module
 */
plerrcode proc_VMEverify(ems_u32* p)
{
    return verify_vme_module(ModulEnt(p[1]), 1);
}

plerrcode test_proc_VMEverify(ems_u32* p)
{
    if (p[0]!=1) return plErr_ArgNum;
    return plOK;
}

char name_proc_VMEverify[] = "VMEverify";
int ver_proc_VMEverify = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==4
 * p[1]: crate
 * p[2]: start addr
 * p[3]: increment
 * p[4]: number of addresses to be tested
 */
/*
 * output:
 *   number of modules found
 *     ems_type (-1 if not known)
 *     serial number (-1 if not known)
 *     base addr
 *     ems_type...
 */
plerrcode proc_vmecratescan(ems_u32* p)
{
    struct vme_dev* dev=get_device(modul_vme, p[1]);
    int num;

    num=scan_vme_crate(dev, p[2], p[3], p[4], outptr+1);
    if (num>=0) {
        *outptr++=num;
        outptr+=3*num;
    }
    return plOK;
}

plerrcode test_proc_vmecratescan(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_vme, p[1], 0))!=plOK)
        return pres;
    return plOK;
}

char name_proc_vmecratescan[] = "vmecratescan";
int ver_proc_vmecratescan = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: crate
 * p[2]: granularity
 * p[3]: am
 * p[4]: size
 */
plerrcode proc_vmecratemap(ems_u32* p)
{
    struct vme_dev* dev=get_device(modul_vme, p[1]);
    ems_u64 addr, start=0;
    ems_u32 val;
    int berr, gran, stat=0, res;
    int am=p[3], size=p[4];

    berr=dev->berrtimer(dev, 5000);
    gran=1<<p[2];
    if (gran<size)
        gran=size;

    for (addr=0; addr<0x100000000ULL; addr+=gran) {
        res=dev->read(dev, addr, am, &val, size, size, 0);
        if (res==size) {
            if (stat==0) {
                stat=1;
                start=addr;
            }
        } else {
            if (stat==1) {
                ems_u64 end=addr-gran;
                stat=0;
                if (start==end) {
                    printf("               0x%08llx\n",
                            (unsigned long long)start);
                } else {
                    printf("0x%08llx ... 0x%08llx\n",
                            (unsigned long long)start,
                            (unsigned long long)end);
                }
            }
        }
    }
    if (stat==1) {
        printf("0x%08llx ... end\n", (unsigned long long)start);
    }

    if (berr>=0) dev->berrtimer(dev, berr);
    return plOK;
}

plerrcode test_proc_vmecratemap(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_vme, p[1], 0))!=plOK)
        return pres;
    return plOK;
}

char name_proc_vmecratemap[] = "vmecratemap";
int ver_proc_vmecratemap = 1;
/*****************************************************************************/
/*****************************************************************************/
