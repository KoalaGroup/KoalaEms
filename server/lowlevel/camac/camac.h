/*
 * $ZEL: camac.h,v 1.37 2008/08/14 20:56:17 wuestner Exp $
 * lowlevel/camac/camac.h
 * 
 * (re)created: 02.10.2002
 */

#ifndef _camac_h_
#define _camac_h_

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <sys/types.h>
#include <errorcodes.h>
#include <emsctypes.h>
#include "../devices.h"

struct camac_dev;

/*
each header file has to define:
    typedef ... xxx_camadr_t;
    struct camac_xxx_info ...;
*/

#ifdef CAMAC_CC16
#include "cc16/cc16camaddr.h"
#endif

#ifdef CAMAC_PCICAMAC /* AMCC interface */
#include "pcicamac/pcicamaddr.h"
#endif

#ifdef CAMAC_JORWAY73A
#include "jorway73a/jorway73acamaddr.h"
#endif

#ifdef CAMAC_LNXDSP
#include "lnxdsp/lnxdspcamaddr.h"
#endif

#ifdef CAMAC_VCC2117
#include "vcc2117/vcc2117camaddr.h"
#endif

#ifdef CAMAC_SIS5100
#include "sis5100/sis5100camaddr.h"
#endif

enum camactypes {camac_none, camac_cc16, camac_pcicamac, camac_jorway73a,
    camac_lnxdsp, camac_vcc2117, camac_sis5100};

typedef union {
#ifdef CAMAC_CC16
    struct cc16_camadr_t cc16;
#endif
#ifdef CAMAC_PCICAMAC
    struct pcicamac_camadr_t pcicamac;
#endif
#ifdef CAMAC_JORWAY73A
    struct jorway73a_camadr_t jorway73a;
#endif
#ifdef CAMAC_LNXDSP
    struct lnxdsp_camadr_t lnxdsp;
#endif
#ifdef CAMAC_VCC2117
    struct vcc2117_camadr_t vcc2117;
#endif
#ifdef CAMAC_SIS5100
    struct sis5100_camadr_t sis5100;
#endif
} camadr_t;

struct camac_raw_procs {
    int (*sis1100_remote_write)(struct camac_dev*, int size,
            int am, ems_u32 addr, ems_u32 data, ems_u32* error);
    int (*sis1100_remote_read)(struct camac_dev*, int size,
            int am, ems_u32 addr, ems_u32* data, ems_u32* error);
    int (*sis1100_local_write)(struct camac_dev*, int offset,
            ems_u32 val, ems_u32* error);
    int (*sis1100_local_read)(struct camac_dev*, int offset,
            ems_u32* val, ems_u32* error);
};

#ifdef ZELFERA
struct FERA_procs {
    int  (*FERAenable)  (struct camac_dev*, int, int, int);
    int  (*FERAreadout) (struct camac_dev*, ems_u32**);
    void (*FERAgate)    (struct camac_dev*);
    int  (*FERAstatus)  (struct camac_dev*, ems_u32**);
    void (*FERAdisable) (struct camac_dev* dev);
    int  (*FERA_select_fd) (struct camac_dev* dev);
    int  (*FERAdmasize) (struct camac_dev* dev, int size);
};
#endif /* ZELFERA */

struct camac_dev {
/* this first element matches struct generic_dev */
    struct dev_generic generic;

    camadr_t  (*CAMACaddr)  (int, int, int);
    camadr_t* (*CAMACaddrP) (int, int, int);
    ems_u32 (*CAMACval) (ems_u32);
    ems_u32 (*CAMACgotQ) (ems_u32);
    ems_u32 (*CAMACgotX) (ems_u32);
    ems_u32 (*CAMACgotQX) (ems_u32);
    void (*CAMACinc) (camadr_t*);
    int (*CCCC) (struct camac_dev*);
    int (*CCCZ) (struct camac_dev*);
    int (*CCCI) (struct camac_dev*, int);
    int (*CAMACstatus) (struct camac_dev*, ems_u32*);
    int (*CAMACread) (struct camac_dev*, camadr_t*, ems_u32*);
    int (*CAMACwrite) (struct camac_dev*, camadr_t*, ems_u32);
    int (*CAMACcntl) (struct camac_dev*, camadr_t*, ems_u32*);
    int (*CAMACblread) (struct camac_dev*, camadr_t*, int, ems_u32*);
    int (*CAMACblread16) (struct camac_dev*, camadr_t*, int, ems_u32*);
    int (*CAMACblwrite) (struct camac_dev*, camadr_t*, int, ems_u32*);
    int (*CAMACblreadQstop) (struct camac_dev*, camadr_t*, int, ems_u32*);
    int (*CAMACblreadAddrScan) (struct camac_dev*, camadr_t*, int, ems_u32*);
    int (*CAMAClams) (struct camac_dev*, ems_u32*);
    int (*CAMACenable_lams) (struct camac_dev* dev, int idx);
    int (*CAMACdisable_lams) (struct camac_dev* dev, int idx);
    int (*camacdelay) (struct camac_dev* dev, int delay, int* olddelay);

#ifdef DELAYED_READ
    /* this function should be hardware (controller) dependend */
    //int (*delay_read)(struct camac_dev*, off_t src, void* dst, size_t size);
#endif

#ifdef ZELFERA
    struct FERA_procs* (*get_FERA_procs)(struct camac_dev*);
#endif
    struct camac_raw_procs*  (*get_raw_procs)(struct camac_dev*);

/*
 * DUMMY must be the last 'function'; it is used to check whether all other
 * function pointers are initialised.
 */
    plerrcode (*camcontrol) (struct camac_dev*, int space, int rw, ems_u32 offs,
            ems_u32* data);
    int (*DUMMY)(void);

    int camac_online;
    int camdelay;
    enum camactypes camactype;
    char* pathname;
    void* info;
};

/*#define CAMACslot_e(n) (ModulEnt(n)->address.camac.slot)*/
#define CAMACslot_e(e) ((e)->address.camac.slot)
#define CAMACaddrM(M, A, F) \
    (M)->address.camac.dev->CAMACaddr((M)->address.camac.slot, (A), (F))
#define CAMACaddrMP(M, A, F) \
    (M)->address.camac.dev->CAMACaddrP((M)->address.camac.slot, (A), (F))

int camac_low_printuse(FILE* outfilepath);
errcode camac_low_init(char* arg);
errcode camac_low_done(void);

#endif

/*****************************************************************************/
/*****************************************************************************/
