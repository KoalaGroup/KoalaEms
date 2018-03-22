/*
 * procs/unixvme/vme_irq.c
 * created: 2013-03-01 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: vme_irq.c,v 1.4 2017/10/23 00:01:51 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/unixvme/vme.h"
#include "../../lowlevel/unixvme/vme_address_modifiers.h"
#include "../../lowlevel/devices.h"

RCS_REGISTER(cvsid, "procs/unixvme")

/*****************************************************************************/
static plerrcode
test_vmeparm(ems_u32* p, unsigned int n)
{
    if (p[0]!=n)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme))
        return plErr_ArgRange;
    wirbrauchen=0;
    return plOK;
}
/*****************************************************************************/
/*
(defined in server/lowlevel/unixvme/vme.h)
struct vmeirq_callbackdata {
    ems_u32 mask;
    int level;
    ems_u32 vector;
    struct timespec time;
};
*/
static void
callback(__attribute__((unused)) struct vme_dev* dev,
        const struct vmeirq_callbackdata* cbd,
        __attribute__((unused)) void* data)
{
    printf("IRQ received.");
    printf("  mask   %08x\n", cbd->mask);
    printf("  level         %d\n", cbd->level);
    printf("  vector %08x\n", cbd->vector);
}
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module (used to select the controller only)
 * p[2]: mask
 * p[3]: vector
 */
plerrcode proc_VMEenableirq(ems_u32* p)
{
    int res;
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;

    res=dev->register_vmeirq(dev, p[2], p[3], callback, 0);

    return res?plErr_System:plOK;
}

plerrcode test_proc_VMEenableirq(ems_u32* p)
{
    return test_vmeparm(p, 3);
}

char name_proc_VMEenableirq[]="VMEenableirq";
int ver_proc_VMEenableirq=1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module (used to select the controller only)
 * p[2]: mask
 * p[3]: vector
 */
plerrcode proc_VMEdisableirq(ems_u32* p)
{
    int res;
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;

    res=dev->unregister_vmeirq(dev, p[2], p[3]);

    return res?plErr_System:plOK;
}

plerrcode test_proc_VMEdisableirq(ems_u32* p)
{
    return test_vmeparm(p, 3);
}

char name_proc_VMEdisableirq[]="VMEdisableirq";
int ver_proc_VMEdisableirq=1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module (used to select the controller only)
 * p[2]: mask
 * p[3]: vector
 */
plerrcode proc_VMEacknowledgeirq(ems_u32* p)
{
    int res;
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;

    res=dev->acknowledge_vmeirq(dev, p[2], p[3]);

    return res?plErr_System:plOK;
}

plerrcode test_proc_VMEacknowledgeirq(ems_u32* p)
{
    return test_vmeparm(p, 3);
}

char name_proc_VMEacknowledgeirq[]="VMEacknowledgeirq";
int ver_proc_VMEacknowledgeirq=1;
/*****************************************************************************/
/*****************************************************************************/
