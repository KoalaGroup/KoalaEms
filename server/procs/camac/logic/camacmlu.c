/*
 * procs/camac/mlu/camacmlu.c
 * 
 * programming of CAMAC MLUs LC2373 and CAEN C8[56]
 * 
 * created 2004-04-04 PW
 *
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: camacmlu.c,v 1.5 2011/04/06 20:30:30 wuestner Exp $";

#include <errorcodes.h>

#include <sconf.h>
#include <debug.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../../lowlevel/camac/camac.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../procprops.h"
#include "../camac_verify.h"

extern ems_u32* outptr;
extern int *memberlist, wirbrauchen;

/*
  (R/W line 8765 4321)
       2373 0100 0000
     strobe 0100 0000 0x40
transparent 0100 1000 0x48
    inhibit 0101 0000 0x50
      pulse 0110 0000 0x60
*/
enum LC2373_modes {LC2373_inhibit=0x50, LC2373_transparent=0x48,
        LC2373_strobe=0x40, LC2373_pulse=0x60};

#define LC2373_SIZE (1<<16)
#define C85_SIZE    (1<<8)

#define MAX(a, b) ((a)>(b)?(a):(b))

RCS_REGISTER(cvsid, "procs/camac/logic")

/*****************************************************************************/
static plerrcode
LC2373_set_mode(struct camac_dev* dev, int N, int mode, ems_u32* oldmode,
        char* caller)
{
    camadr_t caddr_read_mode=dev->CAMACaddr(N, 2, 0);
    camadr_t caddr_write_mode=dev->CAMACaddr(N, 2, 16);
    ems_u32 status, val;

    if (oldmode) {
        ems_u32 old;
        /* read current mode */
        if (dev->CAMACread(dev, &caddr_read_mode, &old)) {
            printf("%s: read old mode failed: %s\n", caller, strerror(errno));
            return plErr_System;
        }
        /* neither X nor Q! */
        *oldmode=dev->CAMACval(old);
    }

    /* set mode */
    if (dev->CAMACwrite(dev, &caddr_write_mode, mode)) {
        printf("%s: set mode 0x%x: %s\n", caller, mode, strerror(errno));
        return plErr_System;
    }
    if (dev->CAMACstatus(dev, &status)) {
        printf("%s: read status after set mode: %s\n", caller, strerror(errno));
        return plErr_System;
    }
    if (!dev->CAMACgotX(status)) {
        printf("%s: set mode 0x%x: no X\n", caller, mode);
        return plErr_HW;
    }
    /* read new mode back (just to be sure) */
    if (dev->CAMACread(dev, &caddr_read_mode, &val)) {
        printf("%s: read new mode failed: %s\n", caller, strerror(errno));
        return plErr_System;
    }
    /* neither X nor Q! */
    if (dev->CAMACval(val)!=mode) {
        printf("%s: mode=0x%x, not 0x%x\n", caller, val, mode);
        return plErr_HW;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode
LC2373_set_address(struct camac_dev* dev, int N, int addr, char* caller)
/* module must be in 'inhibit' mode */
{
    camadr_t caddr_read_addr=dev->CAMACaddr(N, 1, 0);
    camadr_t caddr_write_addr=dev->CAMACaddr(N, 1, 16);
    ems_u32 val;

    /* write address */
    if (dev->CAMACwrite(dev, &caddr_write_addr, addr)) {
        printf("%s: write address: %s\n", caller, strerror(errno));
        return plErr_System;
    }
    if (dev->CAMACstatus(dev, &val)) {
        printf("%s: read status after set addr: %s\n", caller, strerror(errno));
        return plErr_System;
    }
    if (!dev->CAMACgotX(val)) {
        printf("%s: set addr: no X\n", caller);
        return plErr_HW;
    }

    /* read new addr back (just to be sure) */
    if (dev->CAMACread(dev, &caddr_read_addr, &val)) {
        printf("%s: read new mode failed: %s\n", caller, strerror(errno));
        return plErr_System;
    }
    if (dev->CAMACval(val)!=addr) {
        printf("%s: new addr=0x%x, not 0x%x\n", caller, val, addr);
        return plErr_HW;
    }
    return plOK;
}
/*****************************************************************************/
/*
 * p[0]: argcount
 * p[1]: memberidx
 * p[2]: startindex
 * p[3]: number of values to be written
 * p[4...]: values
 */

plerrcode proc_camac_mlu_load(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;
    camadr_t caddr_write_mem=dev->CAMACaddr(N, 0, 16);
    camadr_t caddr_read_mem=dev->CAMACaddr(N, 0, 0);
    int mtype=module->modultype;
    plerrcode pres;
    ems_u32 buf[MAX(LC2373_SIZE, C85_SIZE)];
    ems_u32* q=p+4;
    int i, j;

    switch (mtype) {
    case LC_MLU_2373: {
        ems_u32 oldmode;
        if ((p[2]>=LC2373_SIZE) || (p[2]+p[3]>LC2373_SIZE)) {
            return plErr_ArgRange;
        }

        /* read old mode and set mode to 'inhibit' */
        pres=LC2373_set_mode(dev, N, LC2373_inhibit, &oldmode,
                "proc_camac_mlu_load");
        if (pres) return pres;

        /* write new data */
        pres=LC2373_set_address(dev, N, p[2], "proc_camac_mlu_load");
        if (pres) return pres;
        if (dev->CAMACblwrite(dev, &caddr_write_mem, p[3], q)) {
            printf("proc_camac_mlu_load: CAMACblwrite failed: %s\n",
                    strerror(errno));
            return plErr_System;
        }

        /* read all data back and compare */
        pres=LC2373_set_address(dev, N, 0, "proc_camac_mlu_load");
        if (pres) return pres;
        if (dev->CAMACblread(dev, &caddr_read_mem, LC2373_SIZE, buf)) {
            printf("proc_camac_mlu_load: CAMACblread failed: %s\n",
                    strerror(errno));
            return plErr_System;
        }
        for (i=0; i<LC2373_SIZE; i++) buf[i]&=0xffffff;
        for (i=p[2]+p[3]-1, j=p[3]-1; j>=0; i--, j--) {
            if (q[j]!=buf[i]) {
                printf("proc_camac_mlu_load: value mismatch: "
                        "[0x%x]: 0x%x --> 0x%x\n", i, q[j], buf[i]);
                return plErr_HW;
            }
        }

        /* restore old mode */
        pres=LC2373_set_mode(dev, N, oldmode, 0, "proc_camac_mlu_load");
        if (pres) return pres;
        }
        break;
    case CAEN_MLU_C85:
    case CAEN_MLU_C86:
        if ((p[2]>=C85_SIZE) || (p[2]+p[3]>C85_SIZE)) {
            return plErr_ArgRange;
        }
        break;
    };

    return plOK;
}

plerrcode test_proc_camac_mlu_load(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_MLU_C85, CAEN_MLU_C86, LC_MLU_2373, 0};

    if (p[0]<3) return plErr_ArgNum;
    if (p[0]!=p[3]+3) return plErr_ArgNum;
    if (!memberlist) return plErr_NoISModulList;
    if ((p[1]<1)||(p[1]>memberlist[0])) return plErr_IllModAddr;
    if ((res=test_proc_camac(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_camac_mlu_load[]="camac_mlu_load";
int ver_proc_camac_mlu_load=1;
/*****************************************************************************/
/*
 * p[0]: argcount
 * p[1]: memberidx
 * p[2]: startindex
 * p[3]: number of values to be read
 */

plerrcode proc_camac_mlu_read(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;
    camadr_t caddr_read_mem=dev->CAMACaddr(N, 0, 0);
    int mtype=module->modultype;
    int i;
    plerrcode pres;

    switch (mtype) {
    case LC_MLU_2373: {
        ems_u32 oldmode;
        if ((p[2]>=LC2373_SIZE) || (p[2]+p[3]>LC2373_SIZE)) {
            return plErr_ArgRange;
        }

        /* read old mode and set mode to 'inhibit' */
        pres=LC2373_set_mode(dev, N, LC2373_inhibit, &oldmode,
                "proc_camac_mlu_read");
        if (pres) return pres;

        /* read data */
        pres=LC2373_set_address(dev, N, p[2], "proc_camac_mlu_read");
        if (pres) return pres;
        if (dev->CAMACblread(dev, &caddr_read_mem, p[3], outptr)) {
            printf("proc_camac_mlu_read: CAMACblread failed: %s\n",
                    strerror(errno));
            return plErr_System;
        }
        for (i=0; i<p[3]; i++) outptr[i]&=0xffffff;
        outptr+=p[3];

        /* restore old mode */
        pres=LC2373_set_mode(dev, N, oldmode, 0, "proc_camac_mlu_load");
        if (pres) return pres;
        }
        break;
    case CAEN_MLU_C85:
    case CAEN_MLU_C86:
        if ((p[2]>=C85_SIZE) || (p[2]+p[3]>C85_SIZE)) {
            return plErr_ArgRange;
        }
        break;
    };

    return plOK;
}

plerrcode test_proc_camac_mlu_read(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={CAEN_MLU_C85, CAEN_MLU_C86, LC_MLU_2373, 0};

    if (p[0]!=3) return plErr_ArgNum;
    if (!memberlist) return plErr_NoISModulList;
    if ((p[1]<1)||(p[1]>memberlist[0])) return plErr_IllModAddr;
    if ((res=test_proc_camac(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=p[3];
    return plOK;
}

char name_proc_camac_mlu_read[]="camac_mlu_read";
int ver_proc_camac_mlu_read=1;
/*****************************************************************************/
/*
 * p[0]: argcount==2;
 * p[1]: memberidx
 * p[2]: mode
 */

plerrcode proc_camac_mlu_mode(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;
    plerrcode pres;

    pres=LC2373_set_mode(dev, N, p[2], outptr, "proc_camac_mlu_mode");
    if (pres) return pres;
    outptr++;
    return plOK;
}

plerrcode test_proc_camac_mlu_mode(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={LC_MLU_2373, 0};

    if (p[0]!=2) return plErr_ArgNum;
    if (!memberlist) return plErr_NoISModulList;
    if ((p[1]<1)||(p[1]>memberlist[0])) return plErr_IllModAddr;
    if ((res=test_proc_camac(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=1;
    return plOK;
}

char name_proc_camac_mlu_mode[]="camac_mlu_mode";
int ver_proc_camac_mlu_mode=1;
/*****************************************************************************/
/*
 * p[0]: argcount==1;
 * p[1]: memberidx
 */
enum LC2373_modes modes[4]={
    LC2373_transparent,
    LC2373_strobe,
    LC2373_pulse,
    LC2373_inhibit};

plerrcode proc_camac_mlu_test(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct camac_dev* dev=module->address.camac.dev;
    int N=module->address.camac.slot;
    camadr_t read_mode=dev->CAMACaddr(N, 2, 0);
    camadr_t write_mode=dev->CAMACaddr(N, 2, 16);
    camadr_t read_addr=dev->CAMACaddr(N, 1, 0);
    camadr_t write_addr=dev->CAMACaddr(N, 1, 16);
    camadr_t read_val=dev->CAMACaddr(N, 0, 0);
    camadr_t write_val=dev->CAMACaddr(N, 0, 16);
    ems_u32 val;
    int i;

/* set mode */
    for (i=0; i<4; i++) {
        int mode=modes[i];
        dev->CAMACwrite(dev, &write_mode, mode);
        dev->CAMACread(dev, &read_mode, &val);
        if (!dev->CAMACgotX(val)) {
            printf("read mode 0x%x: no X\n", mode);
            return plErr_HW;
        }
        if (dev->CAMACval(val)!=mode) {
            printf("read mode 0x%x: 0x%08x\n", mode, val);
            return plErr_HW;
        }
    }
    printf("modes OK.\n");

/* write addr */
    for (i=0; i<65536; i++) {
        dev->CAMACwrite(dev, &write_addr, i);
        dev->CAMACread(dev, &read_addr, &val);
        if (!dev->CAMACgotX(val)) {
            printf("read addr 0x%x: no X\n", i);
            return plErr_HW;
        }
        if (dev->CAMACval(val)!=i) {
            printf("read addr 0x%x: 0x%08x\n", i, val);
            return plErr_HW;
        }
    }
    printf("addr OK.\n");

/* write data 0xffff */
    dev->CAMACwrite(dev, &write_addr, 0);
    for (i=0; i<65536; i++) {
        dev->CAMACwrite(dev, &write_val, 0xffff);
        dev->CAMACread(dev, &read_addr, &val);
        if (!dev->CAMACgotX(val)) {
            printf("read addr 0x%x: no X\n", i+1);
            return plErr_HW;
        }
        if (dev->CAMACval(val)!=((i+1)&0xffff)) {
            printf("read addr 0x%x: 0x%08x\n", i+1, val);
            return plErr_HW;
        }
    }
    printf("write 0xffff OK.\n");

/* read data 0xffff */
    dev->CAMACwrite(dev, &write_addr, 0);
    for (i=0; i<65536; i++) {
        dev->CAMACread(dev, &read_val, &val);
        if ((dev->CAMACgotQX(val)!=0xc0000000) && (i<0xffff)) {
            printf("read val_f 0x%x: no QX: 0x%08x\n", i, val);
            return plErr_HW;
        }
        if (dev->CAMACval(val)!=0xffff) {
            printf("read val_f 0x%x: 0x%08x\n", i, val);
            return plErr_HW;
        }
        dev->CAMACread(dev, &read_addr, &val);
        if (!dev->CAMACgotX(val)) {
            printf("read addr 0x%x: no X\n", i+1);
            return plErr_HW;
        }
        if (dev->CAMACval(val)!=((i+1)&0xffff)) {
            printf("read addr 0x%x: 0x%08x\n", i+1, val);
            return plErr_HW;
        }
    }
    printf("read 0xffff OK.\n");

/* write data 0x0000 */
    dev->CAMACwrite(dev, &write_addr, 0);
    for (i=0; i<65536; i++) {
        dev->CAMACwrite(dev, &write_val, 0x0000);
        dev->CAMACread(dev, &read_addr, &val);
        if (!dev->CAMACgotX(val)) {
            printf("read addr 0x%x: no X\n", i+1);
            return plErr_HW;
        }
        if (dev->CAMACval(val)!=((i+1)&0xffff)) {
            printf("read addr 0x%x: 0x%08x\n", i+1, val);
            return plErr_HW;
        }
    }
    printf("write 0x0000 OK.\n");

/* read data 0xffff */
    dev->CAMACwrite(dev, &write_addr, 0);
    for (i=0; i<65536; i++) {
        dev->CAMACread(dev, &read_val, &val);
        if ((dev->CAMACgotQX(val)!=0xc0000000) && (i<0xffff)) {
            printf("read val_0 0x%x: no QX: 0x%08x\n", i, val);
            return plErr_HW;
        }
        if (dev->CAMACval(val)!=0x0000) {
            printf("read val_0 0x%x: 0x%08x\n", i, val);
            return plErr_HW;
        }
        dev->CAMACread(dev, &read_addr, &val);
        if (!dev->CAMACgotX(val)) {
            printf("read addr 0x%x: no X\n", i+1);
            return plErr_HW;
        }
        if (dev->CAMACval(val)!=((i+1)&0xffff)) {
            printf("read addr 0x%x: 0x%08x\n", i+1, val);
            return plErr_HW;
        }
    }
    printf("read 0x0000 OK.\n");

/* write data 0x???? */
    dev->CAMACwrite(dev, &write_addr, 0);
    for (i=0; i<65536; i++) {
        dev->CAMACwrite(dev, &write_val, i);
        dev->CAMACread(dev, &read_addr, &val);
        if (!dev->CAMACgotX(val)) {
            printf("read addr 0x%x: no X\n", i+1);
            return plErr_HW;
        }
        if (dev->CAMACval(val)!=((i+1)&0xffff)) {
            printf("read addr 0x%x: 0x%08x\n", i+1, val);
            return plErr_HW;
        }
    }
    printf("write 0x???? OK.\n");

/* read data 0xffff */
    dev->CAMACwrite(dev, &write_addr, 0);
    for (i=0; i<65536; i++) {
        dev->CAMACread(dev, &read_val, &val);
        if ((dev->CAMACgotQX(val)!=0xc0000000) && (i<0xffff)) {
            printf("read val_* 0x%x: no QX: 0x%08x\n", i, val);
            return plErr_HW;
        }
        if (dev->CAMACval(val)!=i) {
            printf("read val_* 0x%x: 0x%08x\n", i, val);
            return plErr_HW;
        }
        dev->CAMACread(dev, &read_addr, &val);
        if (!dev->CAMACgotX(val)) {
            printf("read addr 0x%x: no X\n", i+1);
            return plErr_HW;
        }
        if (dev->CAMACval(val)!=((i+1)&0xffff)) {
            printf("read addr 0x%x: 0x%08x\n", i+1, val);
            return plErr_HW;
        }
    }
    printf("read 0x???? OK.\n");

    return plOK;
}

plerrcode test_proc_camac_mlu_test(ems_u32* p)
{
    plerrcode res;
    ems_u32 mtypes[]={LC_MLU_2373, 0};

    if (p[0]!=1) return plErr_ArgNum;
    if (!memberlist) return plErr_NoISModulList;
    if ((p[1]<1)||(p[1]>memberlist[0])) return plErr_IllModAddr;
    if ((res=test_proc_camac(memberlist, mtypes))!=plOK) return res;

    wirbrauchen=0;
    return plOK;
}

char name_proc_camac_mlu_test[]="camac_mlu_test";
int ver_proc_camac_mlu_test=1;
/*****************************************************************************/
/*****************************************************************************/
