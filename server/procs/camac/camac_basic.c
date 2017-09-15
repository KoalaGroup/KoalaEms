/*
 * procs/camac/camac_basic.c
 * this file created from older parts: 2007-Sep-14 PW
 * parts created:
 * primitiv.c: before 1994-03-07
 * quick.c:    before 1993-09-39
 * esone.c:    before 1993-11-23
 * quickc.c:          2005-04-22 PW
 * response.c: before 1995-02-01
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: camac_basic.c,v 1.10 2011/04/06 20:30:29 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include <../../commu/commu.h>
#include "../../lowlevel/oscompat/oscompat.h"
#include "../../lowlevel/camac/camac.h"
#include "../../lowlevel/devices.h"
#include "../procs.h"
#include "../procprops.h"
#include "../../objects/domain/dom_ml.h"

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

RCS_REGISTER(cvsid, "procs/camac")

/*****************************************************************************/
/*
 * p[0]: 1
 * p[1]: crate
 */
plerrcode proc_CCCC(ems_u32* p)
{
    struct camac_dev* dev;
    dev=get_camac_device(p[1]);
    dev->CCCC(dev);
    return plOK;
}

plerrcode test_proc_CCCC(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_camac, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}
#ifdef PROCPROPS
static procprop CCCC_prop={0, 0, 0, 0};

procprop* prop_proc_CCCC()
{
    return(&CCCC_prop);
}
#endif

char name_proc_CCCC[]="CCCC";
int ver_proc_CCCC=1;
/*****************************************************************************/
/*
 * p[0]: 2
 * p[1]: crate
 * p[2]: 0: inhibit off 1: inhibit on
 */
plerrcode proc_CCCI(ems_u32* p)
{
    struct camac_dev* dev;
    dev=get_camac_device(p[1]);
    dev->CCCI(dev, p[2]);
    return plOK;
}

plerrcode test_proc_CCCI(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_camac, p[1], 0))!=plOK)
        return pres;
    if (p[2]>1)
        return plErr_ArgRange;
    wirbrauchen=0;
    return plOK;
}
#ifdef PROCPROPS
static procprop CCCI_prop={0, 0, 0, 0};

procprop* prop_proc_CCCI()
{
    return(&CCCI_prop);
}
#endif

char name_proc_CCCI[]="CCCI";
int ver_proc_CCCI=1;
/*****************************************************************************/
/*
 * p[0]: 1
 * p[1]: crate
 */
plerrcode proc_CCCZ(ems_u32* p)
{
    struct camac_dev* dev;
    dev=get_camac_device(p[1]);
    dev->CCCZ(dev);
    return plOK;
}

plerrcode test_proc_CCCZ(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_camac, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}
#ifdef PROCPROPS
static procprop CCCZ_prop={0, 0, 0, 0};

procprop* prop_proc_CCCZ()
{
    return(&CCCZ_prop);
}
#endif

char name_proc_CCCZ[]="CCCZ";
int ver_proc_CCCZ=1;
/*****************************************************************************/
/*
 * p[0]: Nr. of args (3 od. 4)
 * p[1]: n (slot or module index)
 * p[2]: a
 * p[3]: f
 * [p[4]: data]
 */
plerrcode proc_CFSA(ems_u32* p)
{
    unsigned int n=*++p, a=*++p, f=*++p;
    ml_entry* m=ModulEnt(n);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), a, f);
    ems_u32 stat;
    int res;

    switch (f>>3) {
        case 0: res=dev->CAMACread(dev, &addr, outptr++); break;
        case 2: res=dev->CAMACwrite(dev, &addr, *++p); break;
        default: res=dev->CAMACcntl(dev, &addr, &stat);
    }
    return res?plErr_System:plOK;
}

plerrcode test_proc_CFSA(ems_u32* p)
{
    unsigned int anz= *p++,n= *p++,a= *p++,f= *p;
    if ((anz!=3)&&(anz!=4))
        return plErr_ArgNum;
    if (a>15||f>31)
        return plErr_BadHWAddr;
    if (anz!=((f>>3)==2?4:3))
        return plErr_ArgNum;
    if (!valid_module(n, modul_camac, 1))
        return plErr_ArgRange;
    wirbrauchen=((f>>3)?0:1);
    return plOK;
}
#ifdef PROCPROPS
static procprop CFSA_prop={1, -1, 0, 0};

procprop* prop_proc_CFSA()
{
    return(&CFSA_prop);
}
#endif

char name_proc_CFSA[]="CFSA";
int ver_proc_CFSA=1;
/*****************************************************************************/
/*
 * p[0]: Nr. of args
 * p[1]: n (modul)
 * p[2]: a
 * p[3]: f
 * p[4]: len
 * [p[5...]: data]
 */
plerrcode proc_CFUBC(ems_u32* p)
{
    int n= *++p,a= *++p,f= *++p,len= *++p;
    ml_entry* m=ModulEnt(n);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr;
    ems_u32 stat;
    int res=0;

    addr=dev->CAMACaddr(CAMACslot_e(m), a, f);
    switch(f>>3){
    case 0:
        res=dev->CAMACblread(dev, &addr, len, outptr);
        if (res>=0)
            outptr+=res;
        break;
    case 2:
        res=dev->CAMACblwrite(dev, &addr, len, ++p);
        break;
    default:
        while (!res && len--)
            res=dev->CAMACcntl(dev, &addr, &stat);
    }
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_CFUBC(ems_u32* p)
{
    unsigned int anz=*p++, n=*p++, a=*p++, f=*p++, len=*p;
    if (anz<4)
        return plErr_ArgNum;
    if (a>15||f>31)
        return plErr_BadHWAddr;
    if (anz!=((f>>3)==2?4+len:4))
        return plErr_ArgNum;
    if (!valid_module(n, modul_camac, 1))
        return plErr_ArgRange;
    wirbrauchen=((f>>3)?0:len);
    return plOK;
}

#ifdef PROCPROPS
static procprop CFUBC_prop={1, -1, 0, 0};

procprop* prop_proc_CFUBC()
{
    return(&CFUBC_prop);
}
#endif

char name_proc_CFUBC[]="CFUBC";
int ver_proc_CFUBC=1;
/*****************************************************************************/
/*
 * p[0]: Nr. of args (3 od. 4)
 * p[1]: n (modul)
 * p[2]: a
 * p[3]: f
 * p[4]: len
 * [p[5...]: data]
 */
plerrcode proc_CSUBC(ems_u32* p)
{
    int n= *++p,a= *++p,f= *++p,len= *++p;
    ml_entry* m=ModulEnt(n);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr;
    ems_u32 stat;
    int res=0;

    addr=dev->CAMACaddr(CAMACslot_e(m), a, f);
    switch(f>>3){
        case(0): {
            res=dev->CAMACblread16(dev, &addr, len, outptr);
            if (res>=0) outptr+=res;
            }
            break;
        case(2): res=dev->CAMACblwrite(dev, &addr, len, ++p);break;
        default: while (!res && len--) res=dev->CAMACcntl(dev, &addr, &stat);
    }
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_CSUBC(ems_u32* p)
{
    return test_proc_CFUBC(p);  /* wirbrauchen zu grosszuegig */
}

#ifdef PROCPROPS
static procprop CSUBC_prop={1, -1, 0, 0};

procprop* prop_proc_CSUBC()
{
    return(&CSUBC_prop);
}
#endif

char name_proc_CSUBC[]="CSUBC";
int ver_proc_CSUBC=1;
/*****************************************************************************/
/*
 * p[0]: 1
 * p[1]: module
 */
plerrcode proc_nAFslot(ems_u32* p)
{
    ml_entry* m=ModulEnt(p[1]);

    *outptr++=CAMACslot_e(m);
    return plOK;
}

plerrcode test_proc_nAFslot(ems_u32* p)
{
    if (p[0]!=1)
        return(plErr_ArgNum);
    if (!valid_module(p[1], modul_camac, 1))
        return plErr_ArgRange;
    wirbrauchen=1;
    return plOK;
}

char name_proc_nAFslot[]="nAFslot";
int ver_proc_nAFslot=1;
/*****************************************************************************/
/*
 * p[0]: 4
 * p[1]: crate
 * p[2]: N
 * p[3]: A
 * p[4]: F
 */
plerrcode proc_CNAFread(ems_u32* p)
{
    struct camac_dev* dev=get_camac_device(p[1]);
    camadr_t addr=dev->CAMACaddr(p[2], p[3], p[4]);
    int res;

    res=dev->CAMACread(dev, &addr, outptr++);
    return res?plErr_System:plOK;
}

plerrcode test_proc_CNAFread(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_camac, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=1;
    return plOK;
}

char name_proc_CNAFread[]="CNAFread";
int ver_proc_CNAFread=1;
/*****************************************************************************/
/*
 * p[0]: 4
 * p[1]: crate
 * p[2]: N
 * p[3]: A
 * p[4]: F
 * p[5]: no of reads
 */
plerrcode proc_CNAFread_untilQ(ems_u32* p)
{
    struct camac_dev* dev=get_camac_device(p[1]);
    camadr_t addr=dev->CAMACaddr(p[2], p[3], p[4]);
    ems_u32 val;
    int i=p[5];

    do {
        if (dev->CAMACread(dev, &addr, &val)<0)
            return plErr_System;
    } while (!dev->CAMACgotQ(val) && --i);
    *outptr++=val;
    return plOK;
}

plerrcode test_proc_CNAFread_untilQ(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=5)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_camac, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=1;
    return plOK;
}

char name_proc_CNAFread_untilQ[]="CNAFread_untilQ";
int ver_proc_CNAFread_untilQ=1;
/*****************************************************************************/
/*
 * p[0]: 3
 * p[1]: module
 * p[2]: A
 * p[3]: F
 */
plerrcode proc_nAFread(ems_u32* p)
{
    ml_entry* m=ModulEnt(p[1]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), p[2], p[3]);
    int res;

    res=dev->CAMACread(dev, &addr, outptr++);
    return res?plErr_System:plOK;
}

plerrcode test_proc_nAFread(ems_u32* p)
{
    if (p[0]!=3)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac, 1))
        return plErr_ArgRange;
    wirbrauchen=1;
    return plOK;
}

char name_proc_nAFread[]="nAFread";
int ver_proc_nAFread=1;
/*****************************************************************************/
/*
 * p[0]: 3
 * p[1]: module
 * p[2]: A
 * p[3]: F
 * p[4]: QX
 * [p[5]: warning only]
 */
plerrcode proc_nAFreadQX(ems_u32* p)
{
    ml_entry* m=ModulEnt(p[1]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), p[2], p[3]);
    plerrcode pres=plOK;

    dev->CAMACread(dev, &addr, outptr++);
    if ((*outptr&p[4])!=p[4]) {
        complain("nAFreadQX(%d, %d, %d): invalid QX: 0x%08x",
            CAMACslot_e(m),
            p[2], p[3], *outptr);
        if (p[0]>4)
            pres=plErr_HW;
        else
            tsleep(50);
    }
    outptr++;
    return pres;
}

plerrcode test_proc_nAFreadQX(ems_u32* p)
{
    if (p[0]!=3)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac, 1))
        return plErr_ArgRange;
    wirbrauchen=1;
    return plOK;
}

char name_proc_nAFreadQX[]="nAFreadQX";
int ver_proc_nAFreadQX=1;
/*****************************************************************************/
/*
p[0]: num
p[1...3]: n, A, F
p[4]: no of reads
*/
plerrcode proc_nAFread_untilQ(ems_u32* p)
{
    int res, i=p[4];
    ml_entry* m=ModulEnt(p[1]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), p[2], p[3]);
    ems_u32 val;

    do {
        res=dev->CAMACread(dev, &addr, &val);
    } while (!dev->CAMACgotQ(val) && --i);
    *outptr++=val;
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_nAFread_untilQ(ems_u32* p)
{
    if (p[0]!=4)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac, 1))
        return plErr_ArgRange;
    wirbrauchen=1;
    return plOK;
}

char name_proc_nAFread_untilQ[]="nAFread_untilQ";
int ver_proc_nAFread_untilQ=1;
/*****************************************************************************/
/*
 * p[0]: 5
 * p[1]: crate
 * p[2]: N
 * p[3]: A
 * p[4]: F
 * p[5]: data
 */
plerrcode proc_CNAFwrite(ems_u32* p)
{
    struct camac_dev* dev=get_camac_device(p[1]);
    camadr_t addr=dev->CAMACaddr(p[2], p[3], p[4]);
    int res;

    res=dev->CAMACwrite(dev, &addr, p[5]);
    return res?plErr_System:plOK;
}

plerrcode test_proc_CNAFwrite(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=5)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_camac, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}
char name_proc_CNAFwrite[]="CNAFwrite";
int ver_proc_CNAFwrite=1;
/*****************************************************************************/
/*
 * p[0]: 4
 * p[1]: module
 * p[2]: A
 * p[3]: F
 * p[4]: data
 */
plerrcode proc_nAFwrite(ems_u32* p)
{
    ml_entry* m=ModulEnt(p[1]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), p[2], p[3]);
    int res;

    res=dev->CAMACwrite(dev, &addr, p[4]);
    return res?plErr_System:plOK;
}

plerrcode test_proc_nAFwrite(ems_u32* p)
{
    if (p[0]!=4)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac, 1))
        return plErr_ArgRange;
    wirbrauchen=0;
    return plOK;
}

char name_proc_nAFwrite[]="nAFwrite";
int ver_proc_nAFwrite=1;
/*****************************************************************************/
/*
p[0]: num of args
p[1...3]: n, A, F
p[4]: data
p[5]: max. tries
*/
plerrcode proc_nAFwrite_untilQ(ems_u32* p)
{
    int i=p[5];
    ml_entry* m=ModulEnt(p[1]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), p[2], p[3]);
    ems_u32 stat;
    int res;

    do {
        res=dev->CAMACwrite(dev, &addr, p[4]);
    } while ((dev->CAMACstatus(dev, &stat), dev->CAMACgotQ(stat)) && --i);
    *outptr++=dev->CAMACgotQ(stat);
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_nAFwrite_untilQ(ems_u32* p)
{
    if (p[0]!=5)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac, 1))
        return plErr_ArgRange;
    wirbrauchen=1;
    return plOK;
}

char name_proc_nAFwrite_untilQ[]="nAFwrite_untilQ";
int ver_proc_nAFwrite_untilQ=1;
/*****************************************************************************/
/*
p[0]: num of args
p[1...4]: C, N, A, F
p[5]: data
p[6]: max. tries
*/
plerrcode proc_CNAFwrite_untilQ(ems_u32* p)
{
    struct camac_dev* dev=get_camac_device(p[1]);
    camadr_t addr=dev->CAMACaddr(p[2], p[3], p[4]);
    ems_u32 stat;
    int res, i=p[6];

    do {
        res=dev->CAMACwrite(dev, &addr, p[5]);
    } while ((dev->CAMACstatus(dev, &stat), dev->CAMACgotQ(stat)) && --i);
    *outptr++=dev->CAMACgotQ(stat);
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_CNAFwrite_untilQ(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=6)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_camac, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=1;
    return plOK;
}

char name_proc_CNAFwrite_untilQ[]="CNAFwrite_untilQ";
int ver_proc_CNAFwrite_untilQ=1;
/*****************************************************************************/
/*
p[0]: num of args
p[1...4]: C, N, A, F
p[5]: data
*/
plerrcode proc_CNAFwrite_q(ems_u32* p)
{
    struct camac_dev* dev=get_camac_device(p[1]);
    camadr_t addr=dev->CAMACaddr(p[2], p[3], p[4]);
    ems_u32 stat;
    int res;

    res=dev->CAMACwrite(dev, &addr, p[5]);
    if (res)
        return plErr_System;
    res=dev->CAMACstatus(dev, &stat);
    *outptr++=dev->CAMACgotQX(stat);
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_CNAFwrite_q(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=5)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_camac, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=1;
    return plOK;
}

char name_proc_CNAFwrite_q[]="CNAFwrite_q";
int ver_proc_CNAFwrite_q=1;
/*****************************************************************************/
/*
p[0]: num of args
p[1...3]: n, A, F
p[4]: data
*/
plerrcode proc_nAFwrite_q(ems_u32* p)
{
    ml_entry* m=ModulEnt(p[1]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), p[2], p[3]);
    ems_u32 stat;
    int res;

    res=dev->CAMACwrite(dev, &addr, p[4]);
    if (res)
        return plErr_System;
    res=dev->CAMACstatus(dev, &stat);
    *outptr++=dev->CAMACgotQX(stat);
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_nAFwrite_q(ems_u32* p)
{
    if (p[0]!=4)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac, 1))
        return plErr_ArgRange;
    wirbrauchen=1;
    return plOK;
}

char name_proc_nAFwrite_q[]="nAFwrite_q";
int ver_proc_nAFwrite_q=1;
/*****************************************************************************/
/*
 * p[0]: 4
 * p[1]: crate
 * p[2]: N
 * p[3]: A
 * p[4]: F
 */
plerrcode proc_CNAFcntl(ems_u32* p)
{
    struct camac_dev* dev=get_camac_device(p[1]);
    camadr_t addr=dev->CAMACaddr(p[2], p[3], p[4]);
    ems_u32 stat;
    int res;

    res=dev->CAMACcntl(dev, &addr, &stat);
    return res?plErr_System:plOK;
}

plerrcode test_proc_CNAFcntl(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_camac, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}
char name_proc_CNAFcntl[]="CNAFcntl";
int ver_proc_CNAFcntl=1;
/*****************************************************************************/
/*
 * p[0]: 4
 * p[1]: crate
 * p[2]: N
 * p[3]: A
 * p[4]: F
 */
plerrcode proc_CNAFcntl_q(ems_u32* p)
{
    struct camac_dev* dev=get_camac_device(p[1]);
    camadr_t addr=dev->CAMACaddr(p[2], p[3], p[4]);
    ems_u32 stat;
    int res;

    res=dev->CAMACcntl(dev, &addr, &stat);
    if (res)
        return plErr_System;
    res=dev->CAMACstatus(dev, &stat);
    *outptr++=dev->CAMACgotQX(stat);
    return res?plErr_System:plOK;
}

plerrcode test_proc_CNAFcntl_q(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=4)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_camac, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}
char name_proc_CNAFcntl_q[]="CNAFcntl_q";
int ver_proc_CNAFcntl_q=1;
/*****************************************************************************/
/*
 * p[0]: 3
 * p[1]: module
 * p[2]: A
 * p[3]: F
 */
plerrcode proc_nAFcntl(ems_u32* p)
{
    ml_entry* m=ModulEnt(p[1]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), p[2], p[3]);
    ems_u32 stat;
    int res;

    res=dev->CAMACcntl(dev, &addr, &stat);
    return res?plErr_System:plOK;
}

plerrcode test_proc_nAFcntl(ems_u32* p)
{
    if (p[0]!=3)
        return(plErr_ArgNum);
    if (!valid_module(p[1], modul_camac, 1))
        return plErr_ArgRange;
    wirbrauchen=0;
    return plOK;
}

char name_proc_nAFcntl[]="nAFcntl";
int ver_proc_nAFcntl=1;
/*****************************************************************************/
/*
p[0]: num of args
p[1...3]: n, A, F
p[4]: num
*/
plerrcode proc_nAFcntl_untilQ(ems_u32* p)
{
    int i=p[4];
    ems_u32 status;
    ml_entry* m=ModulEnt(p[1]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), p[2], p[3]);

    do {
        dev->CAMACcntl(dev, &addr, &status);
    } while (!dev->CAMACgotQ(status) && --i);
    *outptr++=dev->CAMACgotQX(status);
    return(plOK);
}

plerrcode test_proc_nAFcntl_untilQ(ems_u32* p)
{
    if (p[0]!=4)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac, 1))
        return plErr_ArgRange;
    wirbrauchen=1;
    return plOK;
}

char name_proc_nAFcntl_untilQ[]="nAFcntl_untilQ";
int ver_proc_nAFcntl_untilQ=1;
/*****************************************************************************/
/*
p[0]: num of args
p[1...4]: C, N, A, F
p[5]: num
*/
plerrcode proc_CNAFcntl_untilQ(ems_u32* p)
{
    struct camac_dev* dev=get_camac_device(p[1]);
    camadr_t addr=dev->CAMACaddr(p[2], p[3], p[4]);
    ems_u32 status;
    int i=p[5];

    do {
        dev->CAMACcntl(dev, &addr, &status);
    } while (!dev->CAMACgotQ(status) && --i);
    *outptr++=dev->CAMACgotQX(status);
    return(plOK);
}

plerrcode test_proc_CNAFcntl_untilQ(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=5)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_camac, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=1;
    return plOK;
}

char name_proc_CNAFcntl_untilQ[]="CNAFcntl_untilQ";
int ver_proc_CNAFcntl_untilQ=1;
/*****************************************************************************/
/*
p[0]: num of args
p[1...3]: n, A, F
*/
plerrcode proc_nAFcntl_q(ems_u32* p)
{
    ml_entry* m=ModulEnt(p[1]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), p[2], p[3]);
    ems_u32 stat;
    int res;

    res=dev->CAMACcntl(dev, &addr, &stat);
    *outptr++=dev->CAMACgotQX(stat);
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_nAFcntl_q(ems_u32* p)
{
    if (p[0]!=3)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac, 1))
        return plErr_ArgRange;
    wirbrauchen=1;
    return plOK;
}

char name_proc_nAFcntl_q[]="nAFcntl_q";
int ver_proc_nAFcntl_q=1;
/*****************************************************************************/
/*
p[0]: 1
p[1]: crate
*/
plerrcode proc_cqx(ems_u32* p)
{
    struct camac_dev* dev=get_camac_device(p[1]);
    ems_u32 stat;
    int res;

    res=dev->CAMACstatus(dev, &stat);
    *outptr++=dev->CAMACgotQX(stat);
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_cqx(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=1)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_camac, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=1;
    return plOK;
}

char name_proc_cqx[]="cQX";
int ver_proc_cqx=1;
/*****************************************************************************/
/*
p[0]: 1
p[1]: module
*/
plerrcode proc_qx(ems_u32* p)
{
    ml_entry* m=ModulEnt(p[1]);
    struct camac_dev* dev=m->address.camac.dev;
    ems_u32 stat;
    int res;

    res=dev->CAMACstatus(dev, &stat);
    *outptr++=dev->CAMACgotQX(stat);
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_qx(ems_u32* p)
{
    if (p[0]!=1)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_camac, 1))
        return plErr_ArgRange;
    wirbrauchen=1;
    return plOK;
}

char name_proc_qx[]="QX";
int ver_proc_qx=1;
/*****************************************************************************/
/*
 * p[0]: 5
 * p[1]: crate
 * p[2]: N
 * p[3]: A
 * p[4]: F
 * p[5]: num
 */
plerrcode proc_CNAFblread(ems_u32* p)
{
    struct camac_dev* dev=get_camac_device(p[1]);
    camadr_t addr=dev->CAMACaddr(p[2], p[3], p[4]);
    int res;

    res=dev->CAMACblread(dev, &addr, p[5], outptr);
    if (res>=0)
        outptr+=res;
printf("CNAFblread: res=%d\n", res);
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_CNAFblread(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=5)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_camac, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=p[5];
    return plOK;
}
char name_proc_CNAFblread[]="CNAFblread";
int ver_proc_CNAFblread=1;
/*****************************************************************************/
/*
 * p[0]: 5
 * p[1]: crate
 * p[2]: N
 * p[3]: A
 * p[4]: F
 * p[5...]: data
 */
plerrcode proc_CNAFblwrite(ems_u32* p)
{
    struct camac_dev* dev=get_camac_device(p[1]);
    camadr_t addr=dev->CAMACaddr(p[2], p[3], p[4]);
    int res;

    res=dev->CAMACblwrite(dev, &addr, p[0]-4, p+5);
    if (res>=0)
        outptr+=res;
printf("CNAFblwrite: res=%d\n", res);
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_CNAFblwrite(ems_u32* p)
{
    plerrcode pres;
    if (p[0]<5)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_camac, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}
char name_proc_CNAFblwrite[]="CNAFblwrite";
int ver_proc_CNAFblwrite=1;
/*****************************************************************************/
/*
 * p[0]: 3
 * p[1]: module
 * p[2]: A
 * p[3]: F
 * p[4]: num
 */
plerrcode proc_nAFblread(ems_u32* p)
{
    ml_entry* m=ModulEnt(p[1]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), p[2], p[3]);
    int res;

    res=dev->CAMACblread(dev, &addr, p[4], outptr);
    if (res>=0) outptr+=res;
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_nAFblread(ems_u32* p)
{
    if (p[0]!=4)
        return(plErr_ArgNum);
    if (!valid_module(p[1], modul_camac, 1))
        return plErr_ArgRange;
    wirbrauchen=p[4];
    return plOK;
}

char name_proc_nAFblread[]="nAFblread";
int ver_proc_nAFblread=1;
/*****************************************************************************/
/*
 * p[0]: 5
 * p[1]: crate
 * p[2]: N
 * p[3]: A
 * p[4]: F
 * p[5]: max
 */
plerrcode proc_CNAFreadQstop(ems_u32* p)
{
    struct camac_dev* dev=get_camac_device(p[1]);
    camadr_t addr=dev->CAMACaddr(p[2], p[3], p[4]);
    int res;

    res=dev->CAMACblreadQstop(dev, &addr, p[5], outptr);
    if (res>=0)
        outptr+=res;
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_CNAFreadQstop(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=5)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_camac, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=p[5]+1;
    return plOK;
}
char name_proc_CNAFreadQstop[]="CNAFreadQstop";
int ver_proc_CNAFreadQstop=1;
/*****************************************************************************/
/*
 * p[0]: 5
 * p[1]: crate
 * p[2]: N
 * p[3]: A
 * p[4]: F
 * p[5]: max
 */
plerrcode proc_CNAFreadQstop1(ems_u32* p)
{
    struct camac_dev* dev=get_camac_device(p[1]);
    camadr_t addr=dev->CAMACaddr(p[2], p[3], p[4]);
    int res;

    res=dev->CAMACblreadQstop(dev, &addr, p[5], outptr+1);
    *outptr++=res;
    if (res>=0)
        outptr+=res;
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_CNAFreadQstop1(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=5)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_camac, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=p[5]+1;
    return plOK;
}
char name_proc_CNAFreadQstop1[]="CNAFreadQstop1";
int ver_proc_CNAFreadQstop1=1;
/*****************************************************************************/
/*
 * p[0]: 3
 * p[1]: module
 * p[2]: A
 * p[3]: F
 * p[4]: max
 */
plerrcode proc_nAFreadQstop(ems_u32* p)
{
    ml_entry* m=ModulEnt(p[1]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), p[2], p[3]);
    int res;

    res=dev->CAMACblreadQstop(dev, &addr, p[4], outptr);
    if (res>=0) outptr+=res;
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_nAFreadQstop(ems_u32* p)
{
    if (p[0]!=4)
        return(plErr_ArgNum);
    if (!valid_module(p[1], modul_camac, 1))
        return plErr_ArgRange;
    wirbrauchen=p[4]+1;
    return plOK;
}

char name_proc_nAFreadQstop[]="nAFreadQstop";
int ver_proc_nAFreadQstop=1;
/*****************************************************************************/
/*
 * p[0]: 3
 * p[1]: module
 * p[2]: A
 * p[3]: F
 * p[4]: max
 */
plerrcode proc_nAFreadQstop1(ems_u32* p)
{
    ml_entry* m=ModulEnt(p[1]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), p[2], p[3]);
    int res;

    res=dev->CAMACblreadQstop(dev, &addr, p[4], outptr+1);
    *outptr++=res;
    if (res>=0) outptr+=res;
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_nAFreadQstop1(ems_u32* p)
{
    if (p[0]!=4)
        return(plErr_ArgNum);
    if (!valid_module(p[1], modul_camac, 1))
        return plErr_ArgRange;
    wirbrauchen=p[4]+1;
    return plOK;
}

char name_proc_nAFreadQstop1[]="nAFreadQstop1";
int ver_proc_nAFreadQstop1=1;
/*****************************************************************************/
/*
 * p[0]: 5
 * p[1]: crate
 * p[2]: N
 * p[3]: A
 * p[4]: F
 * p[5]: num
 */
plerrcode proc_CNAFreadAddrScan(ems_u32* p)
{
    struct camac_dev* dev=get_camac_device(p[1]);
    camadr_t addr=dev->CAMACaddr(p[2], p[3], p[4]);
    int res;

    res=dev->CAMACblreadAddrScan(dev, &addr, p[5], outptr);
    if (res>=0)
        outptr+=res;
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_CNAFreadAddrScan(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=5)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_camac, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=p[5];
    return plOK;
}
char name_proc_CNAFreadAddrScan[]="CNAFreadAddrScan";
int ver_proc_CNAFreadAddrScan=1;
/*****************************************************************************/
/*
 * p[0]: 3
 * p[1]: module
 * p[2]: A
 * p[3]: F
 * p[4]: num
 */
plerrcode proc_nAFreadAddrScan(ems_u32* p)
{
    ml_entry* m=ModulEnt(p[1]);
    struct camac_dev* dev=m->address.camac.dev;
    camadr_t addr=dev->CAMACaddr(CAMACslot_e(m), p[2], p[3]);
    int res;

    res=dev->CAMACblreadAddrScan(dev, &addr, p[4], outptr);
    if (res>=0) outptr+=res;
    return res<0?plErr_System:plOK;
}

plerrcode test_proc_nAFreadAddrScan(ems_u32* p)
{
    if (p[0]!=4)
        return(plErr_ArgNum);
    if (!valid_module(p[1], modul_camac, 1))
        return plErr_ArgRange;
    wirbrauchen=p[4];
    return plOK;
}

char name_proc_nAFreadAddrScan[]="nAFreadAddrScan";
int ver_proc_nAFreadAddrScan=1;
/*****************************************************************************/
/*
 * p[0]: 2
 * p[1]: crate
 * p[2]: camacdelay
 */
plerrcode proc_camacdelay(ems_u32* p)
{
    struct camac_dev* dev;
    int old;

    dev=get_camac_device(p[1]);
    dev->camacdelay(dev, p[2], &old);
    *outptr++=old;
    return plOK;
}

plerrcode test_proc_camacdelay(ems_u32* p)
{
    plerrcode pres;
    if (p[0]!=2)
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_camac, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=1;
    return plOK;
}

char name_proc_camacdelay[]="camacdelay";
int ver_proc_camacdelay=1;
/*****************************************************************************/
/*
 * p[0]: argcount==[4|5]
 * p[1]: crate
 * p[2]: space; // 0: AMCC/PLX 1: ADD_ON 2: CAMAC 3: CAMAC Block
 * p[3]: rw; // 1: read 2: write 3: write+read
 * p[4]: offs
 * [p[5]: data]
 */
plerrcode proc_camcontrol(ems_u32* p)
{
    struct camac_dev* dev;
    ems_u32 data;

    dev=get_camac_device(p[1]);
    if (p[3]&2)
        data=p[5];
    if (dev->camcontrol(dev, p[2], p[3], p[4], &data)<0)
        return plErr_System;
    if (p[3]&1)
        *outptr++=data;
    return plOK;
}

plerrcode test_proc_camcontrol(ems_u32* p)
{
    plerrcode pres;
    if (p[0]<4 || p[0]>5)
        return plErr_ArgNum;
    if ((p[3]&2) && (p[0]<5))
        return plErr_ArgNum;
    if ((pres=find_odevice(modul_camac, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=(p[3]&1)?1:0;
    return plOK;
}

char name_proc_camcontrol[]="camcontrol";
int ver_proc_camcontrol=1;
/*****************************************************************************/
/*****************************************************************************/
