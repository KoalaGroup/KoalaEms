/*
 * lowlevel/lvd/lvdbus.h
 * created 10.Dec.2003 PW
 * $ZEL: lvdbus.h,v 1.49 2017/10/20 23:21:31 wuestner Exp $
 */

#ifndef _lvdbus_h_
#define _lvdbus_h_

#include <sconf.h>
#include <stdio.h>
#include <time.h>
#include <errorcodes.h>
#include <emsctypes.h>
#include "../devices.h"

#define MAXACARDS 0x10

#ifdef LVD_TRACE
#define DEFINE_LVDTRACE(name) int name
#define LVDTRACE_ACTIVE(name) (name)
#else
#define DEFINE_LVDTRACE(name)
#define LVDTRACE_ACTIVE(name) 0
#endif

struct lvd_dev;

struct lvdirq_callbackdata {
    ems_u32 mask;
    ems_i32 seq;
    struct timespec time;
};

typedef void (*lvdirq_callback)(struct lvd_dev*,
        const struct lvdirq_callbackdata*,
        void*);

enum lvdtypes {lvd_none, lvd_sis1100};
enum parsebits {
        lvd_parse_head=0x1,
        lvd_parse_sync=0x2,
        lvd_parse_ignore_errors_sync=0x4,
#if 0
        lvd_parse_f1=0x8,
        lvd_parse_gpx=0x10,
#endif
};

enum contr_type {
    contr_none,
    contr_f1,
    contr_gpx,
    contr_f1gpx
};

struct lvd_acard {
    struct lvd_dev *dev;
    unsigned int addr;
    ems_u32 id;      /* may be changed with force_module_id */
    ems_u32 orig_id; /* orig_id remains unchanged */
    ems_i32 serial;
    ems_u32 mtype;
    int initialized;
    int daqmode;
    void* cardinfo;
    void (*freeinfo)(struct lvd_dev*, struct lvd_acard*);
    int (*clear_card)(struct lvd_dev* dev, struct lvd_acard*);
    int (*start_card)(struct lvd_dev* dev, struct lvd_acard*);
    int (*stop_card)(struct lvd_dev* dev, struct lvd_acard*);
    int (*cardstat)(struct lvd_dev* dev, struct lvd_acard*, void*, int level);
};

struct lvd_ccard {
    ems_u32 id;
    ems_u32 serial;
    ems_u32 mtype;
    int daqmode;
    int trigger_mask;
    int trigger_edge;
    int rate_trigger_count;
    int rate_trigger_period;
    int extra_trig;
    int rate_trig;
    enum contr_type contr_type;
    double last_ev_time[2];
    ems_u32 f1_shadow[16];
};

struct datafilter;
typedef plerrcode (lvd_filterproc)(struct datafilter*, ems_u32 *data, int *len,
        int *maxlen);
struct datafilter {
    struct datafilter *next;
    void *definition;
    /* copied from definition for faster access */
    lvd_filterproc *filter;
    struct lvd_dev *dev;
    ems_u32 mask;
    ems_u32 *args;      /* arguments to be used by filter procedure */
    int num_args;
    ems_u32 *givenargs; /* original arguments given to init procedure */
    int num_givenargs; 
};

struct lvd_dev {
/* this first element matches struct generic_dev */
    struct dev_generic generic;

    int (*write)(struct lvd_dev*, ems_u32 addr, int size, ems_u32 data);
    int (*read)(struct lvd_dev*, ems_u32 addr, int size, ems_u32* data);
    plerrcode (*write_block)(struct lvd_dev*, int ci, int addr, int reg,
        int size, ems_u32* data, size_t* num);
    plerrcode (*read_block)(struct lvd_dev*, int ci, int addr, int reg,
        int size, ems_u32* data, size_t* num);
    int (*localreset)(struct lvd_dev*);
    int (*write_blind)(struct lvd_dev*, ems_u32 addr, int size, ems_u32 data);

    plerrcode (*prepare_single)(struct lvd_dev*);
    plerrcode (*start_single)(struct lvd_dev*, int selftrigger);
    plerrcode (*readout_single)(struct lvd_dev*, ems_u32* buffer, int* num, int);
    plerrcode (*stop_single)(struct lvd_dev*, int selftrigger);
    plerrcode (*cleanup_single)(struct lvd_dev*);

    plerrcode (*prepare_async)(struct lvd_dev*, int bufnum, size_t bufsize);
    plerrcode (*start_async)(struct lvd_dev*, int selftrigger);
    plerrcode (*readout_async)(struct lvd_dev*, ems_u32* buffer, int* num, int);
    plerrcode (*stop_async)(struct lvd_dev*, int selftrigger);
    plerrcode (*cleanup_async)(struct lvd_dev*);

    int (*statist)(struct lvd_dev*, ems_u32 flags, ems_u32 *ptr);

    int (*register_lvdirq) (struct lvd_dev*, ems_u32 mask, ems_u32 autoack_mask,
        lvdirq_callback callback, void *data, char *procname, int noclear);
    int (*unregister_lvdirq) (struct lvd_dev*, ems_u32 mask,
        lvdirq_callback callback);
    int (*acknowledge_lvdirq) (struct lvd_dev*, ems_u32 mask);

/*
 * DUMMY must be the last 'function'; it is used to check whether all other
 * function pointers are initialised.
 */
    int (*DUMMY)(void);

    DEFINE_LVDTRACE(trace);
    int silent_errors;
    int silent_regmanipulation;
    int statist_interval; /* dump statistics every X seconds */
    ems_u32 parseflags, parsecaps;
    void *sync_statist;

    struct lvd_ccard ccard;
    enum contr_type contr_mode;
    int hsdiv;
    int tdc_range;
    float tdc_resolution;

    int num_acards;           /* sum of all acquisition cards */
    struct lvd_acard acards[MAXACARDS];
    ems_u32 a_pat;  /* pattern of acquisition cards */

    //int num_headerwords;
    //ems_u32 *headerwords;
    struct datafilter *datafilter;

    enum lvdtypes lvdtype;
    char* pathname;
    void* info;
};

void lvdbus_register_cardtype(ems_u32 module_type,
        int(*init)(struct lvd_dev*, struct lvd_acard*));
plerrcode lvdbus_reset(struct lvd_dev*);
plerrcode lvdbus_init(struct lvd_dev*, int num_ids, ems_u32* ids, int *code);
plerrcode lvdbus_done(struct lvd_dev* dev);
plerrcode lvd_init_controllers(struct lvd_dev*, int input, int hsdiv, int edge,
        int mode);

#ifdef LVD_TRACE
void lvd_settrace(struct lvd_dev*, int *old, int);
#else
#define lvd_settrace(dev, old, val) {do{} while(0);}
#endif

int lvd_silent_regmanipulation(struct lvd_dev*, int);
int lvd_statist_interval(struct lvd_dev* dev, int interval);
int lvd_statist(struct lvd_dev* dev, ems_u32 *args, ems_u32 *outp);

int lvd_enumerate(struct lvd_dev*, ems_u32* nums);
int lvd_a_ident(struct lvd_dev*, int addr, ems_u32* ident, ems_u32* serial);
int lvd_a_mtype(struct lvd_dev*, int addr, ems_u32* mtype);
int lvd_c_ident(struct lvd_dev*, ems_u32* ident, ems_u32* serial);
int lvd_c_mtype(struct lvd_dev*, ems_u32* mtype);
int lvd_find_acard(struct lvd_dev*, int addr);
plerrcode lvd_getdaqmode(struct lvd_dev*, ems_u32* mode);
plerrcode lvd_setdaqmode(struct lvd_dev*, int mode);
plerrcode lvdacq_getdaqmode(struct lvd_dev*, int addr, ems_u32* mode);
plerrcode lvdacq_setdaqmode(struct lvd_dev*, int addr, ems_u32 mtype, int mode);
plerrcode lvd_set_creg(struct lvd_dev*, int reg, int size, ems_u32 val);
plerrcode lvd_get_creg(struct lvd_dev*, int reg, int size, ems_u32* val);
plerrcode lvd_set_cregB(struct lvd_dev*, int reg, int size, ems_u32 val);
plerrcode lvd_get_cregB(struct lvd_dev*, int reg, int size, ems_u32* val);
plerrcode lvd_set_areg(struct lvd_dev*, int addr, int reg, int size, ems_u32 val);
plerrcode lvd_set_aregs(struct lvd_dev*, ems_u32 mtype, int reg, int size, ems_u32 val);
plerrcode lvd_get_areg(struct lvd_dev*, int addr, int reg, int size, ems_u32* val);
plerrcode lvd_set_aregB(struct lvd_dev*, int reg, int size, ems_u32 val);
plerrcode lvd_get_aregB(struct lvd_dev*, int reg, int size, ems_u32* val);
plerrcode lvd_start(struct lvd_dev*, int selftrigger);
plerrcode lvd_start_controller(struct lvd_dev* dev);
plerrcode lvd_stop_controller(struct lvd_dev* dev);
plerrcode lvd_stop(struct lvd_dev*, int selftrigger);
int lvd_clear_status(struct lvd_dev*);
int lvd_cratestat(struct lvd_dev*, void*, int level, int modulemask);
int lvd_contrstat(struct lvd_dev* dev, void*, int level);
int lvd_modstat(struct lvd_dev* dev, unsigned int addr, void*, int level);
int lvd_controllerstat(struct lvd_dev* dev, void*, int level);
int lvd_cardstat_acq(struct lvd_dev* dev, unsigned int addr, void*, int level, int all);
plerrcode lvd_get_eventcount(struct lvd_dev* dev, ems_u32*);
plerrcode lvd_force_module_id(struct lvd_dev* dev, ems_u32 addr, ems_u32 id,
    ems_u32 mask, int serial);

plerrcode lvd_i2c_read(struct lvd_dev* dev, int port, int region, int addr,
        int num, ems_u32 *outptr);
plerrcode lvd_i2c_write(struct lvd_dev* dev, int port, int region, int addr,
        int num, ems_u32 *data);
plerrcode lvd_i2c_dump(struct lvd_dev* dev, int port);

plerrcode lvd_init_jtag_dev(struct generic_dev* dev, void* jdev);

plerrcode
lvd_controller_f1gpx_set_rate_trigger_period(struct lvd_dev*, int);
plerrcode
lvd_controller_f1gpx_get_rate_trigger_period(struct lvd_dev*, ems_u32*);
plerrcode
lvd_controller_f1gpx_set_rate_trigger_count(struct lvd_dev*, int);
plerrcode
lvd_controller_f1gpx_get_rate_trigger_count(struct lvd_dev*, ems_u32*);
plerrcode
lvd_controller_f1gpx_extra_trig(struct lvd_dev*, int);
plerrcode
lvd_controller_f1gpx_rate_trig(struct lvd_dev*, int);


#endif
