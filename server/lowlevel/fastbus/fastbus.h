/*
 * $ZEL: fastbus.h,v 1.15 2007/04/19 22:17:29 wuestner Exp $
 * lowlevel/fastbus/fastbus.h
 * 
 * created: 25.04.97
 * 20.01.1999 PW: FWCa and FWDa added (in comment)
 * 22.May PW: rewritten for multiple crates
 */

#ifndef _fastbus_h_
#define _fastbus_h_
#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errorcodes.h>
#include <emsctypes.h>
#include "../devices.h"

struct fastbus_dev;

enum fastbustypes {fastbus_none, fastbus_chi, fastbus_sfi, fastbus_fvsbi,
    fastbus_sis3100_sfi};

struct fastbus_dev {
/* this first element matches struct generic_dev */
    struct dev_generic generic;

    int (*setarbitrationlevel)(struct fastbus_dev*, unsigned int level);
    int (*getarbitrationlevel)(struct fastbus_dev*, unsigned int* level);
    int (*FRC) (struct fastbus_dev*, ems_u32 pa, ems_u32 sa, ems_u32* data,
            ems_u32* ss);
    int (*FRCa) (struct fastbus_dev*, ems_u32 pa, ems_u32 sa, ems_u32* data,
            ems_u32* ss);
    int (*FRD) (struct fastbus_dev*, ems_u32 pa, ems_u32 sa, ems_u32* data,
            ems_u32* ss);
    int (*FRDa) (struct fastbus_dev*, ems_u32 pa, ems_u32 sa, ems_u32* data,
            ems_u32* ss);

    int (*multiFRC) (struct fastbus_dev*, unsigned int num, ems_u32* pa,
            ems_u32 sa, ems_u32* data, ems_u32* ss);
    int (*multiFRD) (struct fastbus_dev*, unsigned int num, ems_u32* pa,
            ems_u32 sa, ems_u32* data, ems_u32* ss);

    int (*FWC) (struct fastbus_dev*, ems_u32 pa, ems_u32 sa, ems_u32 data,
            ems_u32* ss);
    int (*FWCa)(struct fastbus_dev*, ems_u32 pa, ems_u32 sa, ems_u32 data,
            ems_u32* ss);
    int (*FWD) (struct fastbus_dev*, ems_u32 pa, ems_u32 sa, ems_u32 data,
            ems_u32* ss);
    int (*FWDa)(struct fastbus_dev*, ems_u32 pa, ems_u32 sa, ems_u32 data,
            ems_u32* ss);

    int (*FRCB)(struct fastbus_dev*, ems_u32 pa, ems_u32 sa, unsigned int count,
            ems_u32* dest, ems_u32* ss, const char* caller);
    int (*FRDB)(struct fastbus_dev*, ems_u32 pa, ems_u32 sa, unsigned int count,
            ems_u32* dest, ems_u32* ss, const char* caller);
    void(*FRDB_perfspect_needs)(struct fastbus_dev*, int*, char**);
    int (*FRDB_S)(struct fastbus_dev*, ems_u32 pa, unsigned int count,
            ems_u32* dest, ems_u32* ss, const char* caller);
    void(*FRDB_S_perfspect_needs)(struct fastbus_dev*, int*, char**);
#if 0
    int (*FWCB)(struct fastbus_dev*, ems_u32 pa, ems_u32 sa, unsigned int count,
            int* source, ems_u32* ss);
    int (*FWDB)(struct fastbus_dev*, ems_u32 pa, ems_u32 sa, unsigned int count,
            int* source, ems_u32* ss);
#endif
    int (*FRCM)(struct fastbus_dev*, ems_u32 ba, ems_u32* data, ems_u32* ss);
    int (*FRDM)(struct fastbus_dev*, ems_u32 ba, ems_u32* data, ems_u32* ss);
    int (*FWCM)(struct fastbus_dev*, ems_u32 ba, ems_u32 data, ems_u32* ss);
    int (*FWDM)(struct fastbus_dev*, ems_u32 ba, ems_u32 data, ems_u32* ss);

    int (*FCPC)(struct fastbus_dev*, ems_u32 pa, ems_u32* ss);
    int (*FCPD)(struct fastbus_dev*, ems_u32 pa, ems_u32* ss);
    int (*FCDISC)(struct fastbus_dev*);
    int (*FCRW)(struct fastbus_dev*, ems_u32* data, ems_u32* ss);
    int (*FCRWS)(struct fastbus_dev*, ems_u32 sa, ems_u32* data, ems_u32* ss);
    int (*FCWSA)(struct fastbus_dev*, ems_u32 sa, ems_u32* ss);
    int (*FCWWS)(struct fastbus_dev*, ems_u32 sa, ems_u32 data, ems_u32* ss);
    int (*FCWW)(struct fastbus_dev*, ems_u32 data, ems_u32* ss);
    int (*LEVELIN)(struct fastbus_dev*, int opt, ems_u32* dest);
    int (*LEVELOUT)(struct fastbus_dev*, int opt, ems_u32 set, ems_u32 res);
    int (*PULSEOUT)(struct fastbus_dev*, int opt, ems_u32 mask);
    int (*FRDB_multi_init)(struct fastbus_dev*, int addr, unsigned int num_pa,
            ems_u32* pa, ems_u32 sa, unsigned int max);
    int (*FRDB_multi_read)(struct fastbus_dev*, int addr, ems_u32* dest_data,
            int* count);
    int (*reset)(struct fastbus_dev*);
    int (*restart_sequencer)(struct fastbus_dev*);
    int (*status)(struct fastbus_dev*, ems_u32* status, int max);
    int (*dmalen)(struct fastbus_dev*, int subsys, int* rlen, int* wlen);
/*
 * DUMMY must be the last 'function'; it is used to check whether all other
 * function pointers are initialised.
 */
    int (*DUMMY)(void);

    enum fastbustypes fastbustype;
    char* pathname;
    void *info;
};

int fastbus_low_printuse(FILE* outfilepath);
errcode fastbus_low_init(char* arg);
errcode fastbus_low_done(void);

#endif

/*****************************************************************************/
/*****************************************************************************/
