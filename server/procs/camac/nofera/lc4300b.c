/*
 * camac/nofera/lc4300B.c
 * 
 * created: 2010-dec-15 PW
 * $ZEL: lc4300b.c,v 1.6 2015/04/06 21:33:28 wuestner Exp $
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lc4300b.c,v 1.6 2015/04/06 21:33:28 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../../lowlevel/camac/camac.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../procs.h"
#include "../../procprops.h"
#include "../camac_verify.h"

extern ems_u32* outptr;
extern int *memberlist, wirbrauchen;

RCS_REGISTER(cvsid, "procs/camac/nofera")

/*
 * status register:
 * bit 0..7 : VSN
 * bit 8..15: readout modes
 *     8 EPS: enable pedestal subtraction for ECL port
 *     9 ECE: enable zero suppression for ECL port
 *    10 EEN: ECL port enable
 *    11 CPS: enable pedestal subtraction for CAMAC
 *    12 CCE: enable zero suppression for CAMAC
 *    13 CSR: enable CAMAC sequential readout
 *    14 CLE: enable LAM
 *    15 OFS: enable overflow suppression
 */
#define EPS (1<< 8)
#define ECE (1<< 9)
#define EEN (1<<10)
#define CPS (1<<11)
#define CCE (1<<12)
#define CSR (1<<13)
#define CLE (1<<14)
#define OFS (1<<15)

/*****************************************************************************/
/*
 * p[0]: number of arguments
 * p[1]: slot
 * p[2]: VSN
 * p[3]: enable pedestal subtraction
 * p[4]: enable zero suppression
 * p[5]: enable overflow suppression
 */
plerrcode
proc_lc4300b_init(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;
    ems_u32 mode, val, qx;

    /* clear */
    if (dev->CAMACcntl(dev, dev->CAMACaddrP(N, 0, 9), &qx)<0) {
        complain("lc4300b_init(%d): clear failed", N);
        return plErr_System;
    }

    /* write status register */
    mode=p[2]|CSR;
    if (p[3])
        mode|=CPS;
    if (p[4])
        mode|=CCE;
    if (p[5])
        mode|=OFS;
    if (dev->CAMACwrite(dev, dev->CAMACaddrP(N, 0, 16), mode)<0) {
        complain("lc4300b_init(%d): write status failed", N);
        return plErr_System;
    }

    /* read status register */
    if (dev->CAMACread(dev, dev->CAMACaddrP(N, 0, 0), &val)<0) {
        complain("lc4300b_init(%d): read status failed", N);
        return plErr_System;
    }
    if (!dev->CAMACgotQ(val)) {
        complain("lc4300b_init(%d): read status: no Q", N);
        return plErr_HW;
    }
    if ((val&0xffff) != mode) {
        complain("lc4300b_init(%d): status: 0x%04x --> 0x%08x", N, mode, val);
        return plErr_HW;
    }

    return plOK;
}

plerrcode
test_proc_lc4300b_init(ems_u32* p)
{
    ml_entry* module;

    /* number of arguments */
    if (p[0]!=5)
        return plErr_ArgNum;
    /* value of arguments */
    if (p[2]>255)
        return plErr_ArgRange;
    /* LC4300B?*/
    if (!valid_module(p[1], modul_camac))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=FERA_ADC_4300B && module->modultype!=FERA_TDC_4300B)
        return plErr_BadModTyp;

    wirbrauchen=0;
    return plOK;
}

char name_proc_lc4300b_init[]="lc4300b_init";
int   ver_proc_lc4300b_init=1;
/*****************************************************************************/
/*
 * p[0]: number of arguments (==17 or 1)
 * p[1]: slot
 * p[2..17]: pedestals (0..255)
 */
plerrcode
proc_lc4300b_pedestals(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;
    ems_u32 qx, *pp=p+2;
    int i;
    plerrcode pres=plOK;

    /* clear */
    if (dev->CAMACcntl(dev, dev->CAMACaddrP(N, 0, 9), &qx)<0) {
        complain("lc4300b_init(%d): clear failed", N);
        return plErr_System;
    }

    if (p[0]>1) {
        /* write pedestals (0..255) */
        for (i=0; i<16; i++) {
            if (dev->CAMACwrite(dev, dev->CAMACaddrP(N, i, 17), pp[i])<0) {
                complain("pedestals(%d, %d): write pedestal failed", N, i);
                return plErr_System;
            }
        }
    }

    /* read back pedestals */
    for (i=0; i<16; i++) {
        if (dev->CAMACread(dev, dev->CAMACaddrP(N, i, 1), outptr+i)<0) {
            complain("pedestals(%d, %d): read pedestal failed", N, i);
            return plErr_System;
        }
        if (!dev->CAMACgotQ(outptr[i])) {
            complain("lc4300b_init(%d): read pedestal: no Q", N);
            return plErr_HW;
        }
        outptr[i]&=0xff;
    }

    if (p[0]>1) {
        for (i=0; i<16; i++) {
            if (outptr[i]!=pp[i]) {
                complain("lc4300b_pedestals n=%d A=%d 0x%02x --> 0x%02x",
                    N, i, pp[i], outptr[i]);
                pres=plErr_HW;
            }
        }
    } else {
        outptr+=16;
    }

    return pres;
}

plerrcode
test_proc_lc4300b_pedestals(ems_u32* p)
{
    ml_entry* module;
    ems_u32 *pp=p+2;
    int i;

    /* number of arguments */
    if (p[0]!=1 && p[0]!=17)
        return plErr_ArgNum;
    /* LC4300B?*/
    if (!valid_module(p[1], modul_camac))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=FERA_ADC_4300B && module->modultype!=FERA_TDC_4300B)
        return plErr_BadModTyp;

    if (p[0]==1) {
        wirbrauchen=16;
        return plOK;
    }

    for (i=0; i<16; i++) {
        if (pp[i]>255)
            return plErr_ArgRange;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_lc4300b_pedestals[]="lc4300b_pedestals";
int   ver_proc_lc4300b_pedestals=1;
/*****************************************************************************/
/*
 * p[0]: number of arguments (==1)
 * p[1]: slot
 */
plerrcode
proc_lc4300b_start(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;
    ems_u32 qx;

    /* clear */
    if (dev->CAMACcntl(dev, dev->CAMACaddrP(N, 0, 9), &qx)<0) {
        complain("lc4300b_init(%d): clear failed", N);
        return plErr_System;
    }

    return plOK;
}

plerrcode
test_proc_lc4300b_start(ems_u32* p)
{
    ml_entry* module;

    /* number of arguments */
    if (p[0]!=1)
        return plErr_ArgNum;
    /* LC4300B?*/
    if (!valid_module(p[1], modul_camac))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=FERA_ADC_4300B && module->modultype!=FERA_TDC_4300B)
        return plErr_BadModTyp;

    wirbrauchen=0;
    return plOK;
}

char name_proc_lc4300b_start[]="lc4300b_start";
int   ver_proc_lc4300b_start=1;
/*****************************************************************************/
/*
 * p[0]: number of arguments (==1)
 * p[1]: slot
 */
plerrcode
proc_lc4300b_read(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;
    ems_u32 qx;
    int res, i;

    /* we expect max. 17 words */
    res=dev->CAMACblreadQstop(dev, dev->CAMACaddrP(N, 0, 2), 18, outptr);
    if (res<0) {
        complain("lc4300b_read(%d) failed", N);
        return plErr_System;
    }
    if (res>17) {
        complain("lc4300b_read(%d) %d words read (max. 17 expected)", N, res);
    }

    /* clear */
    if (dev->CAMACcntl(dev, dev->CAMACaddrP(N, 0, 9), &qx)<0) {
        complain("lc4300b_read(%d): clear failed", N);
        return plErr_System;
    }

    for (i=0; i<res; i++)
        outptr[i]&=~0xc0000000;
    outptr+=res;

    return plOK;
}

plerrcode
test_proc_lc4300b_read(ems_u32* p)
{
    ml_entry* module;

    /* number of arguments */
    if (p[0]!=1)
        return plErr_ArgNum;
    /* LC4300B?*/
    if (!valid_module(p[1], modul_camac))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=FERA_ADC_4300B && module->modultype!=FERA_TDC_4300B)
        return plErr_BadModTyp;

    wirbrauchen=18; /* header word, values */
    return plOK;
}

char name_proc_lc4300b_read[]="lc4300b_read";
int   ver_proc_lc4300b_read=1;
/*****************************************************************************/
/*****************************************************************************/
