/*
 * procs/camac/logic/lc4508.c
 * created 2010-12-18 PW
 *
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lc4508.c,v 1.7 2012/09/10 22:45:16 wuestner Exp $";

#include <errorcodes.h>

#include <sconf.h>
#include <debug.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "../../../lowlevel/camac/camac.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../objects/var/variables.h"
#include "../../procs.h"
#include "../../procprops.h"
#include "../camac_verify.h"

extern ems_u32* outptr;
extern int *memberlist, wirbrauchen;

RCS_REGISTER(cvsid, "procs/camac/logic")

/*
 * F(0)*A(0): read 1st section input pattern
 * F(0)*A(1): read 2nd section input pattern
 * 
 * F(0)*A(2): read 1st section memory at given address
 * F(0)*A(3): read 2nd section memory at given address
 * 
 * F(2)*A(0): read 1st section input pattern and reset
 * F(2)*A(1): read 2nd section input pattern and reset
 * 
 * F(2)*A(2): read 1st section memory at given address and increment address
 * F(2)*A(3): read 2nd section memory at given address and increment address
 * 
 * F(9)*A(0): clear 1st section pattern
 * F(9)*A(1): clear 2nd section pattern
 * 
 * F(9)*A(2): reset memory address in both sections
 * F(9)*A(3): reset memory address in both sections
 * 
 * F(16)*A(0): write 1st section memory at given address
 * F(16)*A(1): write 2nd section memory at given address
 * 
 * F(16)*A(2): write 1st section memory at address given in W9..W16
 * F(16)*A(3): write 2nd section memory at address given in W9..W16
 * 
 * F(18)*A(0): write 1st section memory at given address and increment address
 * F(18)*A(1): write 2nd section memory at given address and increment address
 * 
 * F(18)*A(2): write both section memories at address given in W9..W16
 * F(18)*A(3): write both section memories at address given in W9..W16
 */

/*****************************************************************************/
/*
 * p[0]: argcount==2;
 * p[1]: memberidx
 * p[2]: section; 1: 1st, 2: 2nd, 3: both
 */

plerrcode proc_lc4508_read(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;
    ems_u32 val;

    if (p[2]&1) {
        if (dev->CAMACread(dev, dev->CAMACaddrP(N, 0, 0), &val)<0) {
            complain("lc4508_read(%d) failed", N);
            return plErr_System;
        }
        if (!dev->CAMACgotQ(val)) {
            complain("lc4508_read(%d): no Q", N);
            return plErr_HW;
        }
        *outptr++=val&0xffffff;
    }
    if (p[2]&2) {
        if (dev->CAMACread(dev, dev->CAMACaddrP(N, 1, 0), &val)<0) {
            complain("lc4508_read(%d) failed", N);
            return plErr_System;
        }
        if (!dev->CAMACgotQ(val)) {
            complain("lc4508_read(%d): no Q", N);
            return plErr_HW;
        }
        *outptr++=val&0xffffff;
    }
    return plOK;
}

plerrcode test_proc_lc4508_read(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=2)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac, 0))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=LC_PLU_4508)
        return plErr_BadModTyp;
    wirbrauchen=0;
    if (p[2]&1)
        wirbrauchen++;
    if (p[2]&2)
        wirbrauchen++;
    return plOK;
}

char name_proc_lc4508_read[]="lc4508_read";
int ver_proc_lc4508_read=1;
/*****************************************************************************/
static plerrcode
write_mem(struct camac_dev* dev, int N, int section, ems_u32 *data)
{
    camadr_t addr;
    ems_u32 qx;
    int i;

    /* reset memory address in both sections */
    if (dev->CAMACcntl(dev, dev->CAMACaddrP(N, 2+section, 9), &qx)<0) {
        complain("lc4508_load_mem(%d): reset address failed", N);
        return plErr_System;
    }
    if (!dev->CAMACgotQ(qx)) {
        complain("lc4508_load_mem(%d): reset address: no Q", N);
        return plErr_HW;
    }

    addr=dev->CAMACaddr(N, 0+section, 18);
    for (i=0; i<256; i++) {
        if (dev->CAMACwrite(dev, &addr, data[i])<0) {
            complain("lc4508_load_mem(%d): write failed", N);
            return plErr_System;
        }
    }

    return plOK;
}
/*****************************************************************************/
static plerrcode
read_mem(struct camac_dev* dev, int N, int section, ems_u32 *data)
{
    camadr_t addr;
    ems_u32 qx;
    int i;

    /* reset memory address in both sections */
    if (dev->CAMACcntl(dev, dev->CAMACaddrP(N, 2+section, 9), &qx)<0) {
        complain("lc4508_load_mem(%d): reset address failed", N);
        return plErr_System;
    }
    if (!dev->CAMACgotQ(qx)) {
        complain("lc4508_load_mem(%d): reset address: no Q", N);
        return plErr_HW;
    }

    addr=dev->CAMACaddr(N, 2+section, 2);
    for (i=0; i<256; i++) {
        if (dev->CAMACread(dev, &addr, data+i)<0) {
            complain("lc4508_read_mem(%d): read failed", N);
            return plErr_System;
        }
        data[i]&=0xff;
    }

    return plOK;
}
/*****************************************************************************/
/*
 * p[0]: argcount>=2;
 * p[1]: memberidx
 * p[2]: section; 1: 1st, 2: 2nd, 3: both
 * p[3..258]: data
 */

plerrcode proc_lc4508_load_mem(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;
    plerrcode pres;
    ems_u32* pp=p+3;
    int i;

    if (p[2]&1) { /* 1st section */
        pres=write_mem(dev, N, 0, pp);
        if (pres)
            return pres;
    }
    if (p[2]&2) { /* 2nd section */
        pres=write_mem(dev, N, 1, pp);
        if (pres)
            return pres;
    }

    /* verify */
    if (p[2]&1) { /* 1st section */
        pres=read_mem(dev, N, 0, outptr);
        if (pres)
            return pres;
        for (i=0; i<256; i++) {
            if (outptr[i]!=pp[i]) {
                complain("lc4508_load(%d)[%d,%3d]: 0x%02x --> 0x%02x",
                    N, 0, i, pp[i], outptr[i]);
                return plErr_HW;
            }
        }
    }
    if (p[2]&2) { /* 2nd section */
        pres=read_mem(dev, N, 1, outptr);
        if (pres)
            return pres;
        for (i=0; i<256; i++) {
            if (outptr[i]!=pp[i]) {
                complain("lc4508_load(%d)[%d,%3d]: 0x%02x --> 0x%02x",
                    N, 1, i, pp[i], outptr[i]);
                return plErr_HW;
            }
        }
    }

    return plOK;
}

plerrcode test_proc_lc4508_load_mem(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=258)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac, 0))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=LC_PLU_4508)
        return plErr_BadModTyp;
    wirbrauchen=256; /* used for verify */
    return plOK;
}

char name_proc_lc4508_load_mem[]="lc4508_load_mem";
int ver_proc_lc4508_load_mem=1;
/*****************************************************************************/
/*
 * similar to lc4508_load_mem but data come from a variable
 * p[0]: argcount>3;
 * p[1]: memberidx
 * p[2]: section; 1: 1st, 2: 2nd, 3: both
 * p[3]: ems variable containing the selector
 * p[4..]: variable for data, selected by p[3]
 */
plerrcode proc_lc4508_load_var(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;
    int selector, p_idx, v_idx, i;
    ems_i32 *ip=(ems_i32*)p;
    ems_u32 *pp;
    plerrcode pres;

    if (ip[3]>=0) {
        selector=var_list[p[3]].var.val;
    } else {
        selector=0;
    }
#if 0
    printf("lc4508_load_var: selector=%d\n", selector);
#endif
    p_idx=selector+4;
    if (p[0]<p_idx) {
        complain("lc4508_load: selector=%d, %d vars given", selector, p[0]-3); 
        return plErr_Overflow;
    }
    v_idx=p[p_idx];
#if 0
    printf("lc4508_load_var: load from=%d\n", v_idx);
#endif
    if (v_idx>MAX_VAR) {
        complain("lc4508_load: selector=%d, var=%d, MAX_VAR=%d",
            selector, v_idx, MAX_VAR);
        return plErr_IllVar;
    }
    if (!var_list[v_idx].len)
        return plErr_NoVar;
    if (var_list[v_idx].len!=256)
        return plErr_IllVarSize;
    pp=var_list[v_idx].var.ptr;

    if (p[2]&1) { /* 1st section */
        pres=write_mem(dev, N, 0, pp);
        if (pres)
            return pres;
    }
    if (p[2]&2) { /* 2nd section */
        pres=write_mem(dev, N, 1, pp);
        if (pres)
            return pres;
    }

    /* verify */
    if (p[2]&1) { /* 1st section */
        pres=read_mem(dev, N, 0, outptr);
        if (pres)
            return pres;
        for (i=0; i<256; i++) {
            if (outptr[i]!=pp[i]) {
                complain("lc4508_load(%d)[%d,%3d]: 0x%02x --> 0x%02x",
                    N, 0, i, pp[i], outptr[i]);
                return plErr_HW;
            }
        }
    }
    if (p[2]&2) { /* 2nd section */
        pres=read_mem(dev, N, 1, outptr);
        if (pres)
            return pres;
        for (i=0; i<256; i++) {
            if (outptr[i]!=pp[i]) {
                complain("lc4508_load(%d)[%d,%3d]: 0x%02x --> 0x%02x",
                    N, 1, i, pp[i], outptr[i]);
                return plErr_HW;
            }
        }
    }

    return plOK;
}

plerrcode test_proc_lc4508_load_var(ems_u32* p)
{
    ml_entry* module;
    ems_i32 *ip=(ems_i32*)p;

    if (p[0]<=3)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac, 0))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=LC_PLU_4508)
        return plErr_BadModTyp;
    if (ip[3]>=0) {
        if (p[3]>MAX_VAR) {
            complain("lc4508_load: illegal variable for idx (%d)", p[3]);
            return plErr_IllVar;
        }
        if (!var_list[p[3]].len) {
            complain("lc4508_load: no variable for idx (%d)", p[3]);
            return plErr_NoVar;
        }
        if (var_list[p[3]].len!=1) {
            complain("lc4508_load: wrong variable size for idx (%d/%d)",
                    p[3], var_list[p[3]].len);
            return plErr_IllVarSize;
        }
    }
    wirbrauchen=256; /* used for verify */
    return plOK;
}

char name_proc_lc4508_load_var[]="lc4508_load_var";
int ver_proc_lc4508_load_var=1;
/*****************************************************************************/
/*
 * p[0]: argcount==2;
 * p[1]: memberidx
 * p[2]: section; 1: 1st, 2: 2nd
 */

plerrcode proc_lc4508_read_mem(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;
    plerrcode pres;

    /* in the test proc we checked that p[2] is 1 or 2 */
    pres=read_mem(dev, N, p[2]-1, outptr);
    if (pres)
        return pres;
    outptr+=256;

    return plOK;
}

plerrcode test_proc_lc4508_read_mem(ems_u32* p)
{
    ml_entry* module;

    if (p[0]!=2)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac, 0))
        return plErr_ArgRange;
    module=ModulEnt(p[1]);
    if (module->modultype!=LC_PLU_4508)
        return plErr_BadModTyp;
    if (p[2]!=1 && p[2]!=2)
        return plErr_ArgRange;
    wirbrauchen=256;
    return plOK;
}

char name_proc_lc4508_read_mem[]="lc4508_read_mem";
int ver_proc_lc4508_read_mem=1;
/*****************************************************************************/
/*****************************************************************************/
