/*
 * procs/camac/scaler/scaler.c
 * 
 * Programmierung der CAMAC scaler 2551 und 4434
 * 
 * created 28.09.93 R. Nellen
 * 
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: scaler.c,v 1.15 2012/09/10 22:46:28 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/camac/camac.h"
#include "../../../objects/var/variables.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../procs.h"
#include "../../procprops.h"

#undef OPTIMIERT

extern ems_u32* outptr;
extern int* memberlist;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/camac/scaler")

/*****************************************************************************/

struct int64 {
    ems_u32 upper;
    ems_u32 lower;
};

/*****************************************************************************/

#define NUMCHAN_4434 32
#define NUMCHAN_2551 12

#define argnum p[0]
#define variable p[1]
#define pattern p[2]
/*#define slot(i) memberlist[(i)+1]*/  /* slot beginnt mit 0 */

/*****************************************************************************/

static plerrcode test_it(ems_u32* p, int ttyp)
{
    plerrcode res;
    int num, n, numchan;
    unsigned int mask, vsize;

    if (argnum!=2) return plErr_ArgNum;

    if ((res=var_attrib(variable, &vsize))!=plOK) return res;

    if (memberlist==0) return plErr_NoISModulList;

    numchan=0;
    num=memberlist[0];

#if 0
    for (n=1; n<=num; n++) {
        struct ml_entry* entry= ModulEnt(n);
        printf("[n=%2d] type=0x%08x class=%d ", n, entry->modultype, entry->modulclass);
        switch (entry->modulclass) {
        case modul_none:
            break;
        case modul_unspec:
            break;
        case modul_generic:
            break;
        case modul_camac:
            printf("dev=%p ", entry->address.camac.dev);
            printf("crate=%d ", entry->address.camac.crate);
            printf("slot=%d\n", entry->address.camac.slot);
            break;
        case modul_fastbus:
            printf("dev=%p ", entry->address.fastbus.dev);
            printf("crate=%d ", entry->address.fastbus.crate);
            printf("pa=%d\n", entry->address.fastbus.pa);
            break;
        case modul_vme:
            printf("dev=%p ", entry->address.vme.dev);
            printf("crate=%d ", entry->address.vme.crate);
            printf("addr=0x%08x\n", entry->address.vme.addr);
            break;
        case modul_lvd:
            break;
        case modul_invalid:
            break;
        }
    }
#endif

#if 0
Why should it be a problem if the modules are in different crates?
    {
        struct camac_dev* dev=0;
        for (n=1; n<=num; n++) {
            if (!valid_module(n, modul_camac, 0)) {
                printf("procs/camac/scaler: no camac module (idx %d)\n", n);
                return plErr_ArgRange;
            }
            if (n==1) {
                dev=ModulEnt(n)->address.camac.dev;
                printf("set dev to %p\n", dev);
            } else {
                if (dev!=ModulEnt(n)->address.camac.dev) {
                    printf("dev=%p\n", dev);
                    printf("ModulEnt(n)->address.camac.dev=%p\n", ModulEnt(n)->address.camac.dev);
                    printf("camac/scaler: modules are in different crates\n");
                    return plErr_ArgRange;
                }
            }
        }
    }
#endif

    for (n=1; n<=num; n++) {
        struct ml_entry* m=ModulEnt(n);

        switch (m->modultype) {
        case LC_SCALER_4434:
            D(D_USER, printf("LC_SCALER_4434 in pos %d\n", n);)
            numchan+=NUMCHAN_4434;
            break;
        case LC_SCALER_2551:
            D(D_USER, printf("LC_SCALER_2551 in pos %d\n", n);)
            numchan+=NUMCHAN_2551;
            break;
        default:
            D(D_USER, printf("Typ %08x in pos %d\n", m->modultype, n);)
            break;
        }
    }

    D(D_USER, printf("%d kanaele gesamt, vsize=%d\n", numchan, vsize);)
    if (vsize<2*numchan)
        return plErr_IllVarSize;

    mask=pattern;
    if ((0xffffffff<<memberlist[0])&mask) {
        printf("procs/camac/scaler: zu viele Bits in Maske\n");
        printf("procs/camac/scaler: maske=%08x; 0xffffffff<<memberlist[0]=%08x\n",
                mask, 0xffffffff<<memberlist[0]);
        return plErr_ArgRange;
    }

    n=1;
    while (mask) {
        if (mask&1) {
            int typ;
            typ=ModulEnt(n)->modultype;
            if (typ!=ttyp) {
                printf("bit fuer pos %d: typ %08x statt %08x\n", n, typ, ttyp);
                return plErr_ArgRange;
            }
        }
        mask>>=1; n++;
    }
    return plOK;
}

/*****************************************************************************/

plerrcode proc_Scaler4434_2_ini(ems_u32* p)
{
    int n, k, res=0;
    struct int64* arr;
    unsigned int mask;

    for (mask=pattern, n=0, arr=(struct int64*)var_list[variable].var.ptr;
            mask;
            mask>>=1, n++) {
        ml_entry* m=ModulEnt(n+1);
        if (mask&1) {
            struct camac_dev* dev=m->address.camac.dev;
            camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), 0, 16);

            res|=dev->CAMACwrite(dev, &addr, 0x8000);
            for (k=0; k<NUMCHAN_4434; k++) {
                arr[k].lower=0;
                arr[k].upper=0;
            }
            arr+=NUMCHAN_4434;
        } else {
            switch (m->modultype) {
            case LC_SCALER_4434: arr+=NUMCHAN_4434; break;
            case LC_SCALER_2551: arr+=NUMCHAN_2551; break;
            }
        }
    }
    return res?plErr_System:plOK;
}

plerrcode test_proc_Scaler4434_2_ini(ems_u32* p)
{
    return test_it(p, LC_SCALER_4434);
}
#ifdef PROCPROPS
static procprop Scaler4434_2_ini_prop={0, 0, "int var, int pattern",
    "initialisiert scaler 4434"};

procprop* prop_proc_Scaler4434_2_ini()
{
    return(&Scaler4434_2_ini_prop);
}
#endif
char name_proc_Scaler4434_2_ini[]="Scaler4434_ini";
int ver_proc_Scaler4434_2_ini=1;

/*****************************************************************************/

plerrcode proc_Scaler2551_2_ini(ems_u32* p)
{
    int n, k, res=0;
    struct int64* arr;
    unsigned int mask;

    for (mask=pattern, n=0, arr=(struct int64*)var_list[variable].var.ptr;
            mask;
            mask>>=1, n++) {
        ml_entry* m=ModulEnt(n+1);
        if (mask&1) {
            struct camac_dev* dev=m->address.camac.dev;
            camadr_t addr;
            ems_u32 stat, dummy;
    
            addr=dev->CAMACaddr(CAMACslot_e(m), 0, 9);
            res|=dev->CAMACcntl(dev, &addr, &stat);
            addr=dev->CAMACaddr(CAMACslot_e(m), 0, 24);
            res|=dev->CAMACcntl(dev, &addr, &stat);
            addr=dev->CAMACaddr(CAMACslot_e(m), 11, 2);
            res|=dev->CAMACread(dev, &addr, &dummy);
            for (k=0; k<NUMCHAN_2551; k++) {
                arr[k].lower=0;
                arr[k].upper=0;
            }
            arr+=NUMCHAN_2551;
        } else {
            switch (m->modultype) {
            case LC_SCALER_4434: arr+=NUMCHAN_4434; break;
            case LC_SCALER_2551: arr+=NUMCHAN_2551; break;
            }
        }
    }
    return res?plErr_System:plOK;
}

plerrcode test_proc_Scaler2551_2_ini(ems_u32* p)
{
    return(test_it(p, LC_SCALER_2551));
}
#ifdef PROCPROPS
static procprop Scaler2551_2_ini_prop={0, 0, "int var, int pattern",
    "initialisiert scaler 2551"};

procprop* prop_proc_Scaler2551_2_ini()
{
    return(&Scaler2551_2_ini_prop);
}
#endif
char name_proc_Scaler2551_2_ini[]="Scaler2551_ini";
int ver_proc_Scaler2551_2_ini=1;

/*****************************************************************************/

plerrcode proc_Scaler4434_2_clear(ems_u32* p)
{
    int n, k, res=0;
    struct int64* arr;
    unsigned int mask;

    for (mask=pattern, n=0, arr=(struct int64*)var_list[variable].var.ptr;
            mask;
            mask>>=1, n++) {
        ml_entry* m=ModulEnt(n+1);
        if (mask&1) {
            struct camac_dev* dev=m->address.camac.dev;
            camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), 0, 16);

            res|=dev->CAMACwrite(dev, &addr, 0x1f40);
            for (k=0; k<NUMCHAN_4434; k++) {
                arr[k].lower=0;
                arr[k].upper=0;
            }
            arr+=NUMCHAN_4434;
        } else {
            switch (m->modultype) {
            case LC_SCALER_4434: arr+=NUMCHAN_4434; break;
            case LC_SCALER_2551: arr+=NUMCHAN_2551; break;
            }
        }
    }
    return res?plErr_System:plOK;
}

plerrcode test_proc_Scaler4434_2_clear(ems_u32* p)
{
    return test_it(p, LC_SCALER_4434);
}
#ifdef PROCPROPS
static procprop Scaler4434_2_clear_prop={0, 0, "int var, int pattern",
    "cleart scaler 4434"};

procprop* prop_proc_Scaler4434_2_clear()
{
    return(&Scaler4434_2_clear_prop);
}
#endif
char name_proc_Scaler4434_2_clear[]="Scaler4434_clear";
int ver_proc_Scaler4434_2_clear=1;

/*****************************************************************************/

plerrcode proc_Scaler2551_2_clear(ems_u32* p)
{
    int n, k, res=0;
    struct int64* arr;
    unsigned int mask;

    for (mask=pattern, n=0, arr=(struct int64*)var_list[variable].var.ptr;
            mask;
            mask>>=1, n++) {
        ml_entry* m=ModulEnt(n+1);
        if (mask&1) {
            struct camac_dev* dev=m->address.camac.dev;
            camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), 0, 9);
            ems_u32 stat;

            res|=dev->CAMACcntl(dev, &addr, &stat);
            for (k=0; k<NUMCHAN_2551; k++) {
                arr[k].lower=0;
                arr[k].upper=0;
            }
            arr+=NUMCHAN_2551;
        } else {
            switch (m->modultype) {
            case LC_SCALER_4434: arr+=NUMCHAN_4434; break;
            case LC_SCALER_2551: arr+=NUMCHAN_2551; break;
            }
        }
    }
    return res?plErr_System:plOK;
}

plerrcode test_proc_Scaler2551_2_clear(ems_u32* p)
{
    return(test_it(p, LC_SCALER_2551));
}
#ifdef PROCPROPS
static procprop Scaler2551_2_clear_prop={0, 0, "int var, int pattern",
    "cleart scaler 2551"};

procprop* prop_proc_Scaler2551_2_clear()
{
    return(&Scaler2551_2_clear_prop);
}
#endif
char name_proc_Scaler2551_2_clear[]="Scaler2551_clear";
int ver_proc_Scaler2551_2_clear=1;

/*****************************************************************************/

plerrcode proc_Scaler4434_2_start(ems_u32* p)
{
    int n, res=0;
    unsigned int mask;

    for (mask=pattern, n=0; mask; mask>>=1, n++) {
        if (mask&1) {
            ml_entry* m=ModulEnt(n+1);
            struct camac_dev* dev=m->address.camac.dev;
            camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), 0, 16);

            res|=dev->CAMACwrite(dev, &addr, 0x1f40);
        }
    }
    return res?plErr_System:plOK;
}

plerrcode test_proc_Scaler4434_2_start(ems_u32* p)
{
    return(test_it(p, LC_SCALER_4434));
}
#ifdef PROCPROPS
static procprop Scaler4434_2_start_prop={0, 0, "int var, int pattern",
    "startet scaler 4434"};

procprop* prop_proc_Scaler4434_2_start()
{
    return(&Scaler4434_2_start_prop);
}
#endif
char name_proc_Scaler4434_2_start[]="Scaler4434_start";
int ver_proc_Scaler4434_2_start=1;

/*****************************************************************************/

plerrcode proc_Scaler2551_2_start(ems_u32* p)
{
    int n, res=0;
    unsigned int mask;

    for (mask=pattern, n=0; mask; mask>>=1, n++) {
        if (mask&1) {
            ml_entry* m=ModulEnt(n+1);
            struct camac_dev* dev=m->address.camac.dev;
            camadr_t addr;
            ems_u32 stat, dummy;

            addr=dev->CAMACaddr(CAMACslot_e(m), 11, 2);
            res|=dev->CAMACread(dev, &addr, &dummy);
            addr=dev->CAMACaddr(CAMACslot_e(m), 0, 26);
            res|=dev->CAMACcntl(dev, &addr, &stat);
        }
    }
    return res?plErr_System:plOK;
}

plerrcode test_proc_Scaler2551_2_start(ems_u32* p)
{
    return(test_it(p, LC_SCALER_2551));
}
#ifdef PROCPROPS
static procprop Scaler2551_2_start_prop={0, 0, "int var, int pattern",
    "startet scaler 2551"};

procprop* prop_proc_Scaler2551_2_start()
{
    return(&Scaler2551_2_start_prop);
}
#endif
char name_proc_Scaler2551_2_start[]="Scaler2551_start";
int ver_proc_Scaler2551_2_start=1;

/*****************************************************************************/

plerrcode proc_Scaler4434_2_stop(ems_u32* p)
{
    int n, k;
    ems_u32 val;
    struct int64* arr;
    unsigned int mask;

    for (mask=pattern, n=0, arr=(struct int64*)var_list[variable].var.ptr;
            mask;
            mask>>=1, n++) {
        ml_entry* m=ModulEnt(n+1);
        if (mask&1) {
            struct camac_dev* dev=m->address.camac.dev;
            camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), 0, 16);

            dev->CAMACwrite(dev, &addr, 0x1fe0);
            addr=dev->CAMACaddr(CAMACslot_e(m), 0, 2);
            for (k=0; k<NUMCHAN_4434; k++) {
                dev->CAMACread(dev, &addr, &val);
                val&=0xffffff;
#ifdef OPTIMIERT
                addlong(arr+k, val);
#else
                arr[k].lower+=val;
                if (arr[k].lower<val) arr[k].upper++;
#endif
            }
            addr=dev->CAMACaddr(CAMACslot_e(m), 0, 16);
            dev->CAMACwrite(dev, &addr, 0x8000);
            arr+=NUMCHAN_4434;
        } else {
            switch (m->modultype) {
            case LC_SCALER_4434: arr+=NUMCHAN_4434; break;
            case LC_SCALER_2551: arr+=NUMCHAN_2551; break;
            }
        }
    }
    return plOK;
}

plerrcode test_proc_Scaler4434_2_stop(ems_u32* p)
{
    return(test_it(p, LC_SCALER_4434));
}
#ifdef PROCPROPS
static procprop Scaler4434_2_stop_prop={0, 0, "int var, int pattern",
    "stoppt scaler 4434"};

procprop* prop_proc_Scaler4434_2_stop()
{
    return(&Scaler4434_2_stop_prop);
}
#endif
char name_proc_Scaler4434_2_stop[]="Scaler4434_stop";
int ver_proc_Scaler4434_2_stop=1;

/*****************************************************************************/

plerrcode proc_Scaler2551_2_stop(ems_u32* p)
{
    int n, k;
    ems_u32 val, stat;
    struct int64* arr;
    unsigned int mask;

    for (mask=pattern, n=0, arr=(struct int64*)var_list[variable].var.ptr;
            mask;
            mask>>=1, n++) {
        ml_entry* m=ModulEnt(n+1);
        if (mask&1) {
            struct camac_dev* dev=m->address.camac.dev;
            camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), 0, 24);

            dev->CAMACcntl(dev, &addr, &stat);
            for (k=0; k<NUMCHAN_2551; k++) {
                addr=dev->CAMACaddr(CAMACslot_e(m), k, 2);
                dev->CAMACread(dev, &addr, &val);
                val&=0xffffff;
#ifdef OPTIMIERT
                addlong(arr+k, val);
#else
                arr[k].lower+=val;
                if (arr[k].lower<val) arr[k].upper++;
#endif
            }
            arr+=NUMCHAN_2551;
        } else {
            switch (m->modultype) {
            case LC_SCALER_4434: arr+=NUMCHAN_4434; break;
            case LC_SCALER_2551: arr+=NUMCHAN_2551; break;
            }
        }
    }
    return plOK;
}

plerrcode test_proc_Scaler2551_2_stop(ems_u32* p)
{
    return(test_it(p, LC_SCALER_2551));
}
#ifdef PROCPROPS
static procprop Scaler2551_2_stop_prop={0, 0, "int var, int pattern",
    "stoppt scaler 2551"};

procprop* prop_proc_Scaler2551_2_stop()
{
    return(&Scaler2551_2_stop_prop);
}
#endif
char name_proc_Scaler2551_2_stop[]="Scaler2551_stop";
int ver_proc_Scaler2551_2_stop=1;

/*****************************************************************************/

plerrcode proc_Scaler4434_2_update(ems_u32* p)
{
    int n, k;
    ems_u32 val, stat;
    struct int64* arr;
    unsigned int mask;

    for (mask=pattern, n=0, arr=(struct int64*)var_list[variable].var.ptr;
            mask;
            mask>>=1, n++) {
        ml_entry* m=ModulEnt(n+1);
        if (mask&1) {
            struct camac_dev* dev=m->address.camac.dev;
            camadr_t addr;

            addr=dev->CAMACaddr(CAMACslot_e(m), 0, 16);
            dev->CAMACwrite(dev, &addr, 0x1fe0);
            addr=dev->CAMACaddr(CAMACslot_e(m), 0, 10);
            dev->CAMACcntl(dev, &addr, &stat);
            addr=dev->CAMACaddr(CAMACslot_e(m), 0, 2);
            for (k=0; k<NUMCHAN_4434; k++) {
                dev->CAMACread(dev, &addr, &val);
                val&=0xffffff;
#ifdef OPTIMIERT
                addlong(arr+k, val);
#else
                arr[k].lower+=val;
                if (arr[k].lower<val) arr[k].upper++;
#endif
            }
            arr+=NUMCHAN_4434;
        } else {
            switch (m->modultype) {
            case LC_SCALER_4434: arr+=NUMCHAN_4434; break;
            case LC_SCALER_2551: arr+=NUMCHAN_2551; break;
            }
        }
    }
    return plOK;
}

plerrcode test_proc_Scaler4434_2_update(ems_u32* p)
{
    return(test_it(p, LC_SCALER_4434));
}
#ifdef PROCPROPS
static procprop Scaler4434_2_update_prop={0, 0, "int var, int pattern",
    "update of scaler 4434"};

procprop* prop_proc_Scaler4434_2_update()
{
    return(&Scaler4434_2_update_prop);
}
#endif
char name_proc_Scaler4434_2_update[]="Scaler4434_update";
int ver_proc_Scaler4434_2_update=1;

/*****************************************************************************/

plerrcode proc_Scaler2551_2_update(ems_u32* p)
{
    int n, k;
    ems_u32 val;
    struct int64* arr;
    unsigned int mask;

    for (mask=pattern, n=0, arr=(struct int64*)var_list[variable].var.ptr;
            mask;
            mask>>=1, n++) {
        ml_entry* m=ModulEnt(n+1);
        if (mask&1) {
            struct camac_dev* dev=m->address.camac.dev;
            for (k=0; k<NUMCHAN_2551; k++) {
                camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), k, 2);
                dev->CAMACread(dev, &addr, &val);
                val&=0xffffff;
#ifdef OPTIMIERT
                addlong(arr+k, val);
#else
                arr[k].lower+=val;
                if (arr[k].lower<val) arr[k].upper++;
#endif
            }
            arr+=NUMCHAN_2551;
        } else {
            switch (m->modultype) {
            case LC_SCALER_4434: arr+=NUMCHAN_4434; break;
            case LC_SCALER_2551: arr+=NUMCHAN_2551; break;
            }
        }
    }
    return plOK;
}

plerrcode test_proc_Scaler2551_2_update(ems_u32* p)
{
    return(test_it(p, LC_SCALER_2551));
}
#ifdef PROCPROPS
static procprop Scaler2551_2_update_prop={0, 0, "int var, int pattern",
    "update of scaler 2551"};

procprop* prop_proc_Scaler2551_2_update()
{
    return(&Scaler2551_2_update_prop);
}
#endif
char name_proc_Scaler2551_2_update[]="Scaler2551_update";
int ver_proc_Scaler2551_2_update=1;

/*****************************************************************************/

plerrcode proc_Scaler4434_2_lam(ems_u32* p)
{
    int n, k;
    ems_u32 val, stat;
    struct int64* arr;
    unsigned int mask;

    for (mask=pattern, n=0, arr=(struct int64*)var_list[variable].var.ptr;
            mask;
            mask>>=1, n++) {
        ml_entry* m=ModulEnt(n+1);
        if (mask&1) {
            struct camac_dev* dev=m->address.camac.dev;
            int slot=CAMACslot_e(m);
            camadr_t addr=dev->CAMACaddr(slot, 0, 10);

            dev->CAMACcntl(dev, &addr, &stat);
            addr=dev->CAMACaddr(slot, 0, 2);
            for (k=0; k<NUMCHAN_4434; k++) {
                dev->CAMACread(dev, &addr, &val);
                val&=0xffffff;
#ifdef OPTIMIERT
                addlong(arr+k, val);
#else
                arr[k].lower+=val;
                if (arr[k].lower<val) arr[k].upper++;
#endif
            }
            arr+=NUMCHAN_4434;
        } else {
            switch (m->modultype) {
            case LC_SCALER_4434: arr+=NUMCHAN_4434; break;
            case LC_SCALER_2551: arr+=NUMCHAN_2551; break;
            }
        }
    }
    return plOK;
}

plerrcode test_proc_Scaler4434_2_lam(ems_u32* p)
{
    return(test_it(p, LC_SCALER_4434));
}
#ifdef PROCPROPS
static procprop Scaler4434_2_lam_prop={0, 0, "int var, int pattern",
    "scaler 4434 lam"};

procprop* prop_proc_Scaler4434_2_lam()
{
    return(&Scaler4434_2_lam_prop);
}
#endif
char name_proc_Scaler4434_2_lam[]="Scaler4434_lam";
int ver_proc_Scaler4434_2_lam=1;

/*****************************************************************************/

plerrcode proc_Scaler2551_2_lam(ems_u32* p)
{
    int n, k;
    ems_u32 val;
    struct int64* arr;
    unsigned int mask;

    for (mask=pattern, n=0, arr=(struct int64*)var_list[variable].var.ptr;
            mask;
            mask>>=1, n++) {
        ml_entry* m=ModulEnt(n+1);
        if (mask&1) {
            struct camac_dev* dev=m->address.camac.dev;
            int slot=CAMACslot_e(m);

            for (k=0; k<NUMCHAN_2551; k++) {
                camadr_t addr=dev->CAMACaddr(slot, k, 2);
                dev->CAMACread(dev, &addr, &val);
                val&=0xffffff;
#ifdef OPTIMIERT
                addlong(arr+k, val);
#else
                arr[k].lower+=val;
                if (arr[k].lower<val) arr[k].upper++;
#endif
            }
            arr+=NUMCHAN_2551;
        } else {
            switch (m->modultype) {
            case LC_SCALER_4434: arr+=NUMCHAN_4434; break;
            case LC_SCALER_2551: arr+=NUMCHAN_2551; break;
            }
        }
    }
    return plOK;
}

plerrcode test_proc_Scaler2551_2_lam(ems_u32* p)
{
    return(test_it(p, LC_SCALER_2551));
}
#ifdef PROCPROPS
static procprop Scaler2551_2_lam_prop={0, 0, "int var, int pattern",
    "scaler 2551 lam"};

procprop* prop_proc_Scaler2551_2_lam()
{
    return(&Scaler2551_2_lam_prop);
}
#endif
char name_proc_Scaler2551_2_lam[]="Scaler2551_lam";
int ver_proc_Scaler2551_2_lam=1;

/*****************************************************************************/
/*
 * Scaler4434_update_read
 * Alle Kanaele werden ausgelesen, aber nur die in der Maske uebertragen
 * [0] : Anzahl der Argumente (3)
 * [1] : Variablenindex
 * [2] : Stationsnummer
 * [3] : Maske fuer zu uebertragende Kanaele
 */
plerrcode proc_Scaler4434_update_read(ems_u32* p)
{
    struct int64* arr;
    struct camac_dev* dev;
    camadr_t addr;
    int k, n, slot;
    unsigned int mask;
    ems_u32 val, stat;
    ml_entry* m;
    unsigned int numchan;
    ems_u32 *help;

    arr=(struct int64*)var_list[p[1]].var.ptr;
    for (n=1; n<p[2]; n++) {
        switch (ModulEnt(n)->modultype) {
        case LC_SCALER_4434: arr+=NUMCHAN_4434; break;
        case LC_SCALER_2551: arr+=NUMCHAN_2551; break;
        }
    }

    m=ModulEnt(p[2]);
    slot=CAMACslot_e(m);
    dev=m->address.camac.dev;

    addr=dev->CAMACaddr(slot, 0, 16);
    dev->CAMACwrite(dev, &addr, 0x1fe0);
    addr=dev->CAMACaddr(slot, 0, 10);
    dev->CAMACcntl(dev, &addr, &stat);
    addr=dev->CAMACaddr(slot, 0, 2);
    for (k=0; k<NUMCHAN_4434; k++) {
        dev->CAMACread(dev, &addr, &val);
        val&=0xffffff;
#ifdef OPTIMIERT
        addlong(arr+k, val);
#else
        arr[k].lower+=val;
        if (arr[k].lower<val) arr[k].upper++;
#endif
    }
    numchan=0;
    help=outptr++;
    for (k=0, mask=p[3]; mask>0; k++, mask>>=1) {
        if (mask&1) {
            numchan++;
            *outptr++=arr[k].upper;
            *outptr++=arr[k].lower;
        }
    }
    *help=numchan;

    return plOK;
}

plerrcode test_proc_Scaler4434_update_read(ems_u32* p)
{
    unsigned int vsize;
    plerrcode res;

    if (p[0]!=3) return(plErr_ArgNum);
    if ((res=var_attrib(p[1], &vsize))!=plOK) return(res);
    if (!memberlist)
        return plErr_NoISModulList;
    if (!valid_module(p[2], modul_camac, 0)) return(plErr_ArgRange);
    if (ModulEnt(p[2])->modultype!=LC_SCALER_4434) return(plErr_BadModTyp);
    /* unsinnig: NUMCHAN_4434=32
    if (p[3]&(0xffffffff<<NUMCHAN_4434)) return plErr_ArgRange;
    */
    return plOK;
}

#ifdef PROCPROPS
static procprop Scaler4434_update_read_prop={
    1,
    -1,
    "int var, int n, int pattern",
    "update scaler 4434 and read some channels"
};

procprop* prop_proc_Scaler4434_update_read()
{
    return(&Scaler4434_update_read_prop);
}
#endif

char name_proc_Scaler4434_update_read[]="Scaler4434_update_read";
int ver_proc_Scaler4434_update_read=1;

/*****************************************************************************/
/*
 * Scaler4434_update_read_n
 * [0] : Anzahl der Argumente (3)
 * [1] : Variablenindex
 * [2] : Stationsnummer
 * [3] : Anzahl der auszulesenden Kanaele
 */
plerrcode proc_Scaler4434_update_read_n(ems_u32* p)
{
    struct int64* arr;
    struct camac_dev* dev;
    camadr_t addr;
    unsigned int n, k;
    int slot;
    ems_u32 val, stat;
    ml_entry* m;
    n=p[3];

    arr=(struct int64*)var_list[p[1]].var.ptr;

    m=ModulEnt(p[2]);
    dev=m->address.camac.dev;
    slot=CAMACslot_e(m);

    addr=dev->CAMACaddr(slot, 0, 16);
    dev->CAMACwrite(dev, &addr, 0xe0+((n-1)<<8));
    addr=dev->CAMACaddr(slot, 0, 10);
    dev->CAMACcntl(dev, &addr, &stat);
    addr=dev->CAMACaddr(slot, 0, 2);
    for (k=0; k<n; k++) {
        dev->CAMACread(dev, &addr, &val);
        val&=0xffffff;
#ifdef OPTIMIERT
        addlong(arr+k, val);
#else
        arr[k].lower+=val;
        if (arr[k].lower<val) arr[k].upper++;
#endif
    }
    *outptr++=n;
    for (k=0; k<n; k++) {
        *outptr++=arr[k].upper;
        *outptr++=arr[k].lower;
    }

    return plOK;
}

plerrcode test_proc_Scaler4434_update_read_n(ems_u32* p)
{
    unsigned int vsize;
    plerrcode res;

    if (p[0]!=3) return(plErr_ArgNum);
    if ((res=var_attrib(p[1], &vsize))!=plOK) return(res);
    if (!memberlist)
        return plErr_NoISModulList;
    if (!valid_module(p[2], modul_camac, 0)) return(plErr_ArgRange);
    if (ModulEnt(p[2])->modultype!=LC_SCALER_4434) return(plErr_BadModTyp);
    if ((p[3]<1) || (p[3]>32)) return(plErr_ArgRange);
    return plOK;
}

#ifdef PROCPROPS
static procprop Scaler4434_update_read_n_prop={
    1,
    -1,
    "int var, int n, int pattern",
    "update scaler 4434 and read some channels"
};

procprop* prop_proc_Scaler4434_update_read_n()
{
    return(&Scaler4434_update_read_n_prop);
}
#endif

char name_proc_Scaler4434_update_read_n[]="Scaler4434_update_read_n";
int ver_proc_Scaler4434_update_read_n=1;

/*****************************************************************************/
/*
 * Scaler2251_update_read
 * Alle Kanaele werden ausgelesen, aber nur die in der Maske uebertragen
 * [0] : Anzahl der Argumente (3)
 * [1] : Variablenindex
 * [2] : Stationsnummer
 * [3] : Maske fuer zu uebertragende Kanaele
 */
plerrcode proc_Scaler2551_update_read(ems_u32* p)
{
    int n, k;
    struct camac_dev* dev;
    ml_entry* m;
    ems_u32 val, *help;
    struct int64* arr;
    int numchan, slot;
    unsigned int mask;

    arr=(struct int64*)var_list[p[1]].var.ptr;
    for (n=1; n<p[2]; n++) {
        switch (ModulEnt(n)->modultype) {
        case LC_SCALER_4434: arr+=NUMCHAN_4434; break;
        case LC_SCALER_2551: arr+=NUMCHAN_2551; break;
        }
    }

    m=ModulEnt(p[2]);
    slot=CAMACslot_e(m);
    dev=m->address.camac.dev;

    for (k=0; k<NUMCHAN_2551; k++) {
        camadr_t addr=dev->CAMACaddr(slot, k, 2);
        dev->CAMACread(dev, &addr, &val);
        val&=0xffffff;
#ifdef OPTIMIERT
        addlong(arr+k, val);
#else
        arr[k].lower+=val;
        if (arr[k].lower<val) arr[k].upper++;
#endif
    }
    numchan=0;
    help=outptr++;
    for (k=0, mask=p[3]; mask>0; k++, mask>>=1) {
        if (mask&1) {
            numchan++;
            *outptr++=arr[k].upper;
            *outptr++=arr[k].lower;
        }
    }
    *help=numchan;

    return(plOK);
}

plerrcode test_proc_Scaler2551_update_read(ems_u32* p)
{
    unsigned int vsize;
    plerrcode res;

    if (p[0]!=3) return plErr_ArgNum;
    if ((res=var_attrib(p[1], &vsize))!=plOK) return res;
    if (!memberlist)
        return plErr_NoISModulList;
    if (!valid_module(p[2], modul_camac, 0)) return plErr_ArgRange;
    if (ModulEnt(p[2])->modultype!=LC_SCALER_2551) return plErr_BadModTyp;
    if (p[3]&(0xffffffff<<NUMCHAN_2551)) return plErr_ArgRange;
    return plOK;
}

#ifdef PROCPROPS
static procprop Scaler2551_update_read_prop={
    1,
    -1,
    "int var, int pattern",
    "update of scaler 2551 and read some channels"
};

procprop* prop_proc_Scaler2551_update_read()
{
    return &Scaler2551_update_read_prop;
}
#endif

char name_proc_Scaler2551_update_read[]="Scaler2551_update_read";
int ver_proc_Scaler2551_update_read=1;

/*****************************************************************************/
/*
 * Nur die Kanaele in der Maske werden ausgelesen und uebertragen
 * Scaler4434_read
 * [0] : Anzahl der Argumente (3)
 * [1] : Variablenindex
 * [2] : Stationsnummer
 * [3] : Maske fuer auszulesende Kanaele
 */
plerrcode proc_Scaler4434_read(ems_u32* p)
{
    struct int64* arr;
    struct camac_dev* dev;
    camadr_t addr;
    unsigned int mask, xmask, i, k, n;
    ems_u32 val;
    ml_entry* m;
    unsigned int flanken, cluster, numchan;
    int chan0, chan1, slot;
    ems_u32 *help;

    arr=(struct int64*)var_list[p[1]].var.ptr;
    for (n=1; n<p[2]; n++) {
        switch (ModulEnt(n)->modultype) {
        case LC_SCALER_4434: arr+=NUMCHAN_4434; break;
        case LC_SCALER_2551: arr+=NUMCHAN_2551; break;
        }
    }
    m=ModulEnt(p[2]);
    slot=CAMACslot_e(m);
    dev=m->address.camac.dev;

    mask=p[3];
    xmask=mask^(mask<<1);

    /*printf("mask=%08x; xmask=%08x\n", mask, xmask);*/
    chan0=-1;
    chan1=-1;
    for (i=0, flanken=0; i<32; i++) {
#if 0
        if (i<4) {
            printf("m=%d; x=%d; flanken=%d; chan0=%d; chan1=%d\n", (mask&1)==1,
                    (xmask&1)==1, flanken, chan0, chan1);
        }
#endif
        flanken+=xmask&1;
        if (mask&1)
            chan1=i;
        else
            if (chan1<0) chan0=i;
        mask>>=1;
        xmask>>=1;
    }
/*printf("flanken=%d; chan0=%d; chan1=%d\n", (mask&1)==1, (xmask&1)==1,
    flanken, chan0, chan1);*/
    chan0++;
    cluster=(flanken+1)/2;

/*printf("4434_read: %d flanken, %d cluster\n", flanken, cluster);*/

    switch (cluster) {
    case 1:
        /* Controllregister schreiben */
        numchan=chan1-chan0+1;
        /*
        printf("           chan0=%d, chan1=%d; %d kanaele\n", chan0, chan1, numchan);
        printf("CAMACwrite(%x)\n", 0xe0|(numchan-1)<<8|chan0);
        */
        addr=dev->CAMACaddr(slot, 0, 16);
        dev->CAMACwrite(dev, &addr, 0xe0|(numchan-1)<<8|chan0);
        *outptr++=numchan;
        addr=dev->CAMACaddr(slot, 0, 2);
        for (k=chan0; k<=chan1; k++) {
            dev->CAMACread(dev, &addr, &val);
            val&=0xffffff;
#ifdef OPTIMIERT
            addlong(arr+k, val);
#else
            arr[k].lower+=val;
            if (arr[k].lower<val) arr[k].upper++;
#endif
            *outptr++=arr[k].upper;
            *outptr++=arr[k].lower;
        }
        break;
    case 0: break; /* 0 Kanaele */
    default:
        numchan=0;
        help=outptr++;
        addr=dev->CAMACaddr(slot, 0, 16);
        dev->CAMACwrite(dev, &addr, 0x1fe0);
        addr=dev->CAMACaddr(slot, 0, 2);
        for (k=0; k<NUMCHAN_4434; k++) {
            dev->CAMACread(dev, &addr, &val);
            val&=0xffffff;
#ifdef OPTIMIERT
            addlong(arr+k, val);
#else
            arr[k].lower+=val;
            if (arr[k].lower<val) arr[k].upper++;
#endif
        }
        for (k=0, mask=p[3]; mask>0; k++, mask>>=1) {
            if (mask&1) {
                numchan++;
                *outptr++=arr[k].upper;
                *outptr++=arr[k].lower;
            }
        }
        *help=numchan;
        break;
    }
    return plOK;
}

plerrcode test_proc_Scaler4434_read(ems_u32* p)
{
    unsigned int vsize;
    plerrcode res;

    if (p[0]!=3) return(plErr_ArgNum);
    if ((res=var_attrib(p[1], &vsize))!=plOK) return(res);
    if (!memberlist)
        return plErr_NoISModulList;
    if (!valid_module(p[2], modul_camac, 0)) return(plErr_ArgRange);
    if (ModulEnt(p[2])->modultype!=LC_SCALER_4434) return(plErr_BadModTyp);
    return plOK;
}
#ifdef PROCPROPS
static procprop Scaler4434_read_prop={1, 32, "int var, int n, int pattern",
    "read of some channels of scaler 4434"};

procprop* prop_proc_Scaler4434_read()
{
    return(&Scaler4434_read_prop);
}
#endif
char name_proc_Scaler4434_read[]="Scaler4434_read";
int ver_proc_Scaler4434_read=1;

/*****************************************************************************/
/*****************************************************************************/
