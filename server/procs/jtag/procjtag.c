/*
 * procs/jtag/jtag.c
 * created 2006-Jul-05 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: procjtag.c,v 1.9 2017/10/09 21:25:38 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../lowlevel/jtag/jtag_proc.h"
#include "../../lowlevel/devices.h"

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/jtag")

/*****************************************************************************/
/*
 * p[0]: argcount
 * p[1...]: devicetype (String)
 *         camac
 *         fastbus
 *         vme
 *         lvd
 *         perf
 *         can
 * p[pp+0]: branch
 * p[pp+1]: trace
 */
plerrcode proc_jtag_trace(ems_u32* p)
{
    struct generic_dev* dev;
    char* devname;
    enum Modulclass mclass;
    ems_u32* pp;
    int branch, trace;
    plerrcode pres;

    pp=xdrstrcdup(&devname, p+1);
    branch=pp[0];
    trace=pp[1];

    mclass=find_devclass(devname);
    if ((mclass<=modul_generic) || (mclass>=modul_invalid)) {
        printf("proc_jtag_trace: \"%s\" is not a usable device class\n",
            devname);
        pres=plErr_ArgRange;
        goto error;
    }

    if ((pres=find_odevice(mclass, branch, &dev))!=plOK)
        goto error;

    pres=jtag_trace(dev, trace, outptr);
    if (pres==plOK)
        outptr++;

error:
    free(devname);
    return pres;
}

plerrcode test_proc_jtag_trace(ems_u32* p)
{
    if (p[0]<1)
        return plErr_ArgNum;
    if ((xdrstrlen(p+1)+2)!=p[0])
        return plErr_ArgNum;
    wirbrauchen=1;
    return plOK;
}

char name_proc_jtag_trace[] = "jtag_trace";
int ver_proc_jtag_trace = 1;
/*****************************************************************************/
/*
 * p[0]: argcount
 * p[1...]: devicetype (String)
 *         camac
 *         fastbus
 *         vme
 *         lvd
 *         perf
 *         can
 * p[pp+0]: branch
 * p[pp+1]: ci 0: daq module, 1: crate controller 2: local interface
 * p[pp+2]: addr
 */
plerrcode proc_jtag_loopcal(ems_u32* p)
{
    struct generic_dev* dev;
    char* devname;
    enum Modulclass mclass;
    ems_u32* pp;
    int branch, ci, addr;
    plerrcode pres;

    pp=xdrstrcdup(&devname, p+1);
    branch=pp[0];
    ci=pp[1];
    addr=pp[2];

    mclass=find_devclass(devname);
    if ((mclass<=modul_generic) || (mclass>=modul_invalid)) {
        printf("proc_jtag_loopcal: \"%s\" is not a usable device class\n",
            devname);
        pres=plErr_ArgRange;
        goto error;
    }

    if (ci<2) {
        if ((pres=find_odevice(mclass, branch, &dev))!=plOK)
            goto error;
    } else {
        if ((pres=find_device(mclass, branch, &dev))!=plOK)
            goto error;
    }

    pres=jtag_loopcal(dev, ci, addr);

error:
    free(devname);
    return pres;
}

plerrcode test_proc_jtag_loopcal(ems_u32* p)
{
    if (p[0]<1)
        return plErr_ArgNum;
    if ((xdrstrlen(p+1)+3)!=p[0])
        return plErr_ArgNum;
    wirbrauchen=0;
    return plOK;
}

char name_proc_jtag_loopcal[] = "jtag_loopcal";
int ver_proc_jtag_loopcal = 1;
/*****************************************************************************/
/*
 * p[0]: argcount
 * p[1...]: devicetype (String)
 * p[pp+0]: branch
 * p[pp+1]: ci 0: daq module, 1: crate controller 2: local interface
 * p[pp+2]: addr
 */
plerrcode proc_jtag_list(ems_u32* p)
{
    struct generic_dev* dev;
    char* devname;
    enum Modulclass mclass;
    ems_u32* pp;
    int branch, ci, addr;
    plerrcode pres;

    pp=xdrstrcdup(&devname, p+1);
    branch=pp[0];
    ci=pp[1];
    addr=pp[2];

    mclass=find_devclass(devname);
    if ((mclass<=modul_generic) || (mclass>=modul_invalid)) {
        printf("proc_jtag_trace: \"%s\" is not a usable device class\n",
            devname);
        pres=plErr_ArgRange;
        goto error;
    }

    if ((pres=find_device(mclass, branch, (struct generic_dev**)&dev))!=plOK)
        goto error;

    pres=jtag_list(dev, ci, addr);

error:
    free(devname);
    return pres;
}

plerrcode test_proc_jtag_list(ems_u32* p)
{
    if (p[0]<1)
        return plErr_ArgNum;
    if ((xdrstrlen(p+1)+3)!=p[0])
        return plErr_ArgNum;
    wirbrauchen=-1;
    return plOK;
}

char name_proc_jtag_list[] = "jtag_list";
int ver_proc_jtag_list = 1;
/*****************************************************************************/
/*
 * p[0]: argcount
 * p[1...]: devicetype (String)
 * p[pp+0]: branch
 * p[pp+1]: ci 0: daq module, 1: crate controller 2: local interface
 * p[pp+2]: addr
 */
plerrcode proc_jtag_check(ems_u32* p)
{
    struct generic_dev* dev;
    char* devname;
    enum Modulclass mclass;
    ems_u32* pp;
    int branch, ci, addr;
    plerrcode pres;

    pp=xdrstrcdup(&devname, p+1);
    branch=pp[0];
    ci=pp[1];
    addr=pp[2];

    mclass=find_devclass(devname);
    if ((mclass<=modul_generic) || (mclass>=modul_invalid)) {
        printf("proc_jtag_trace: \"%s\" is not a usable device class\n",
            devname);
        pres=plErr_ArgRange;
        goto error;
    }

    if ((pres=find_device(mclass, branch, (struct generic_dev**)&dev))!=plOK)
        goto error;

    pres=jtag_check(dev, ci, addr);

error:
    free(devname);
    return pres;
}

plerrcode test_proc_jtag_check(ems_u32* p)
{
    if (p[0]<1)
        return plErr_ArgNum;
    if ((xdrstrlen(p+1)+3)!=p[0])
        return plErr_ArgNum;
    wirbrauchen=-1;
    return plOK;
}

char name_proc_jtag_check[] = "jtag_check";
int ver_proc_jtag_check = 1;
/*****************************************************************************/
/*
 * p[0]: argcount
 * p[1...]: devicetype (String)
 * p[pp+0]: branch
 * p[pp+1]: ci 0: daq module, 1: crate controller 2: local interface
 * p[pp+2]: addr
 * p[pp+3]: chip_idx
 * p[pp+4...]: filename
 */
plerrcode proc_jtag_read(ems_u32* p)
{
    struct generic_dev* dev;
    char* devname;
    enum Modulclass mclass;
    ems_u32 *pp, usercode;
    int branch, ci, addr, idx;
    char* filename;
    plerrcode pres;

    pp=xdrstrcdup(&devname, p+1);
    branch=pp[0];
    ci=pp[1];
    addr=pp[2];
    idx=pp[3];

    mclass=find_devclass(devname);
    if ((mclass<=modul_generic) || (mclass>=modul_invalid)) {
        printf("proc_jtag_trace: \"%s\" is not a usable device class\n",
            devname);
        pres=plErr_ArgRange;
        goto error;
    }

    if ((pres=find_device(mclass, branch, (struct generic_dev**)&dev))!=plOK)
        goto error;

    xdrstrcdup(&filename, pp+4);
    pres=jtag_read(dev, ci, addr, idx, filename, &usercode);
    free(filename);
    if (pres==plOK)
        *outptr++=usercode;

error:
    free(devname);
    return pres;
}

plerrcode test_proc_jtag_read(ems_u32* p)
{
    ems_u32 *pp;

    if (p[0]<1)
        return plErr_ArgNum;
    pp=p+1;
    pp+=xdrstrlen(pp);
    pp+=4;
    if (pp-p>p[0]+1)
        return plErr_ArgNum;
    pp+=xdrstrlen(pp);
    if (pp-p>p[0]+1)
        return plErr_ArgNum;

    wirbrauchen=1;
    return plOK;
}

char name_proc_jtag_read[] = "jtag_read";
int ver_proc_jtag_read = 1;
/*****************************************************************************/
/*
 * p[0]: argcount
 * p[1...]: devicetype (String)
 * p[pp+0]: branch
 * p[pp+1]: ci 0: daq module, 1: crate controller 2: local interface
 * p[pp+2]: addr
 * p[pp+3]: chip_idx
 */
plerrcode proc_jtag_usercode(ems_u32* p)
{
    struct generic_dev* dev;
    char* devname;
    enum Modulclass mclass;
    ems_u32 *pp, usercode;
    int branch, ci, addr, idx;
    plerrcode pres;

    pp=xdrstrcdup(&devname, p+1);
    branch=pp[0];
    ci=pp[1];
    addr=pp[2];
    idx=pp[3];

    mclass=find_devclass(devname);
    if ((mclass<=modul_generic) || (mclass>=modul_invalid)) {
        printf("proc_jtag_trace: \"%s\" is not a usable device class\n",
            devname);
        pres=plErr_ArgRange;
        goto error;
    }

    if ((pres=find_device(mclass, branch, (struct generic_dev**)&dev))!=plOK)
        goto error;

    pres=jtag_usercode(dev, ci, addr, idx, &usercode);
    if (pres==plOK)
        *outptr++=usercode;

error:
    free(devname);
    return pres;
}

plerrcode test_proc_jtag_usercode(ems_u32* p)
{
    ems_u32 *pp;

    if (p[0]<1)
        return plErr_ArgNum;
    pp=p+1;
    pp+=xdrstrlen(pp);
    pp+=4;
    if (pp-p>p[0]+1)
        return plErr_ArgNum;

    wirbrauchen=1;
    return plOK;
}

char name_proc_jtag_usercode[] = "jtag_usercode";
int ver_proc_jtag_usercode = 1;
/*****************************************************************************/
/*
 * p[0]: argcount
 * p[1...]: devicetype (String)
 * p[pp+0]: branch
 * p[pp+1]: ci 0: daq module, 1: crate controller 2: local interface
 * p[pp+2]: addr
 * p[pp+3]: chip_idx
 * p[pp+4...]: filename
 * [p[pp+0]: usercode]
 */
plerrcode proc_jtag_write(ems_u32* p)
{
    struct generic_dev* dev;
    char* devname;
    enum Modulclass mclass;
    ems_u32 *pp, usercode;
    int branch, ci, addr, idx, change_usercode;
    char* filename;
    plerrcode pres;

    pp=xdrstrcdup(&devname, p+1);
    branch=pp[0];
    ci=pp[1];
    addr=pp[2];
    idx=pp[3];
    usercode=pp[4];

    mclass=find_devclass(devname);
    if ((mclass<=modul_generic) || (mclass>=modul_invalid)) {
        printf("proc_jtag_trace: \"%s\" is not a usable device class\n",
            devname);
        pres=plErr_ArgRange;
        goto error;
    }

    if ((pres=find_device(mclass, branch, (struct generic_dev**)&dev))!=plOK)
        goto error;

    pp=xdrstrcdup(&filename, pp+4);
    if (pp-p<p[0]+1) {
        usercode=*pp++;
        change_usercode=1;
    } else {
        usercode=-1;
        change_usercode=0;
    }
    pres=jtag_write(dev, ci, addr, idx, filename, change_usercode, usercode);
    free(filename);

error:
    free(devname);
    return pres;
}

plerrcode test_proc_jtag_write(ems_u32* p)
{
    ems_u32 *pp;

    if (p[0]<1)
        return plErr_ArgNum;
    pp=p+1;
    pp+=xdrstrlen(pp);
    pp+=4;
    if (pp-p>p[0]+1)
        return plErr_ArgNum;
    pp+=xdrstrlen(pp);
    if (pp-p>p[0]+1)
        return plErr_ArgNum;

    wirbrauchen=0;
    return plOK;
}

char name_proc_jtag_write[] = "jtag_write";
int ver_proc_jtag_write = 1;
/*****************************************************************************/
#if 0
/*
 * p[0]: argcount
 * p[1...]: devicetype (String)
 * p[pp+0]: branch
 * p[pp+1]: ci 0: daq module, 1: crate controller 2: local interface
 * p[pp+2]: addr
 * p[pp+3]: chip_idx
 * p[pp+4...]: filename
 * [p[pp+0]: usercode]
 */
plerrcode proc_jtag_writeX(ems_u32* p)
{
    struct generic_dev* dev;
    char* devname;
    enum Modulclass mclass;
    ems_u32* pp;
    int branch, ci, addr, idx, change_usercode, usercode;
    char* filename;
    plerrcode pres;

    pp=xdrstrcdup(&devname, p+1);
    branch=pp[0];
    ci=pp[1];
    addr=pp[2];
    idx=pp[3];
    usercode=pp[4];

    mclass=find_devclass(devname);
    if ((mclass<=modul_generic) || (mclass>=modul_invalid)) {
        printf("proc_jtag_trace: \"%s\" is not a usable device class\n",
            devname);
        pres=plErr_ArgRange;
        goto error;
    }

    if ((pres=find_device(mclass, branch, (struct generic_dev**)&dev))!=plOK)
        goto error;

    pp=xdrstrcdup(&filename, pp+4);
    if (pp-p<=p[0]+1) {
        usercode=*pp++;
        change_usercode=1;
    } else {
        usercode=-1;
        change_usercode=0;
    }
    pres=jtag_writeX(dev, ci, addr, idx, filename, change_usercode, usercode);
    free(filename);

error:
    free(devname);
    return pres;
}

plerrcode test_proc_jtag_writeX(ems_u32* p)
{
    ems_u32 *pp;

    if (p[0]<1)
        return plErr_ArgNum;
    pp=p+1;
    pp+=xdrstrlen(pp);
    pp+=4;
    if (pp-p>p[0]+1)
        return plErr_ArgNum;
    pp+=xdrstrlen(pp);
    if (pp-p>p[0]+1)
        return plErr_ArgNum;

    wirbrauchen=0;
    return plOK;
}

char name_proc_jtag_writeX[] = "jtag_writeX";
int ver_proc_jtag_writeX = 1;
#endif
/*****************************************************************************/
/*
 * p[0]: argcount
 * p[1...]: devicetype (String)
 * p[pp+0]: branch
 * p[pp+1]: ci 0: daq module, 1: crate controller 2: local interface
 * p[pp+2]: addr
 * p[pp+3]: chip_idx
 * p[pp+4...]: filename
 */
plerrcode proc_jtag_verify(ems_u32* p)
{
    struct generic_dev* dev;
    char* devname;
    enum Modulclass mclass;
    ems_u32* pp;
    int branch, ci, addr, idx;
    char* filename;
    plerrcode pres;

    pp=xdrstrcdup(&devname, p+1);
    branch=pp[0];
    ci=pp[1];
    addr=pp[2];
    idx=pp[3];

    mclass=find_devclass(devname);
    if ((mclass<=modul_generic) || (mclass>=modul_invalid)) {
        printf("proc_jtag_trace: \"%s\" is not a usable device class\n",
            devname);
        pres=plErr_ArgRange;
        goto error;
    }

    if ((pres=find_device(mclass, branch, (struct generic_dev**)&dev))!=plOK)
        goto error;

    xdrstrcdup(&filename, pp+4);
    pres=jtag_verify(dev, ci, addr, idx, filename);
    free(filename);

error:
    free(devname);
    return pres;
}

plerrcode test_proc_jtag_verify(ems_u32* p)
{
    ems_u32 *pp;

    if (p[0]<1)
        return plErr_ArgNum;
    pp=p+1;
    pp+=xdrstrlen(pp);
    pp+=4;
    if (pp-p>p[0]+1)
        return plErr_ArgNum;
    pp+=xdrstrlen(pp);
    if (pp-p>p[0]+1)
        return plErr_ArgNum;

    wirbrauchen=0;
    return plOK;
}

char name_proc_jtag_verify[] = "jtag_verify";
int ver_proc_jtag_verify = 1;
/*****************************************************************************/
/*****************************************************************************/
