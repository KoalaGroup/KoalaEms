/*
 * objects/domain/dom_ml.h
 * $ZEL: dom_ml.h,v 1.13 2011/08/13 20:12:05 wuestner Exp $
 * created 15.01.93
 */

#ifndef _dom_ml_h_
#define _dom_ml_h_

#include <emsctypes.h>
#include <errorcodes.h>
#include <objecttypes.h>

#if 0
old version; for reference
typedef struct {
    int address;  /* oder Slotnummer*/
    int type;
} ml_entry;
#endif

typedef struct ml_entry {
    int modultype;
    Modulclass modulclass;
    union { /* selected by modulclass */
        struct {
            struct camac_dev* dev;
            unsigned int crate;
            ems_u32 slot;
        } camac;
        struct {
            struct vme_dev* dev;
            unsigned int crate;
            /*ems_u64*/ ems_u32 addr;
        } vme;
        struct {
            struct fastbus_dev* dev;
            unsigned int crate;
            ems_u32 pa;
        } fastbus;
        struct {
            struct lvd_dev* dev;
            unsigned int crate;
            ems_u32 mod;
        } lvd;
        struct {
            struct canbus_dev* dev;
            unsigned int bus;
            ems_u32 id;
        } can;
        struct {
            int length;
            int* data;
        } generic; /* future? */
#ifdef DEFAULT_MODULE_CLASS
        struct {
            int addr;
        } unspec; /* backward compatibility */
#endif
    } address;
    int property_length;
    ems_u32* property_data;
    ems_u32* private_data;
} ml_entry;

typedef struct {
    int modnum;
    /*int old_style;*/ /* 1: only modul_unspec is used */
    ml_entry entry[1];
} Modlist;

extern Modlist *modullist;

#define ModulEnt(n) \
    (memberlist?&modullist->entry[memberlist[n]]:&modullist->entry[n])
#define ModulEnt_m(n) \
    (&modullist->entry[memberlist[n]])

errcode dom_ml_init(void);
errcode dom_ml_done(void);
int valid_module(unsigned int idx, Modulclass class, int accept_unspec);

plerrcode dump_modent(ml_entry *entry, int verbose);
plerrcode dump_modulelist(void);
plerrcode dump_memberlist(void);

#endif

/*****************************************************************************/
/*****************************************************************************/
