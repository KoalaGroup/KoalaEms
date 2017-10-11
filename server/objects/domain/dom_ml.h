/*
 * objects/domain/dom_ml.h
 * $ZEL: dom_ml.h,v 1.14 2015/04/06 21:35:02 wuestner Exp $
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

/*
 * property_data is accessible via [GS]etModuleParams and can be used to
 * set some properties for readout procedures or similar
 * private_data is not accessible from outside, it can be used internally
 * for some module status (event counters, word counters ...)
 */

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
            ems_u32 addr;
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
        struct { /* for modules like sis3316 with ethernet connection */
            struct ip_dev* dev;
            char *address; /* host:port or [numerical_v6_host]:port */
            char *protocol; /* udp or tcp */
        } ip;
        struct {
            int length;
            int* data;
        } generic; /* future? */
    } address;
    ems_u32* property_data;
    void* private_data;
    int property_length; /* size of property_data in words */
    int private_length;  /* size of private_data in bytes (!) */
} ml_entry;

typedef struct {
    int modnum;
    ml_entry entry[1];
} Modlist;

extern Modlist *modullist;

#define ModulEnt(n) \
    (memberlist?&modullist->entry[memberlist[n]]:&modullist->entry[n])
#define ModulEnt_m(n) \
    (&modullist->entry[memberlist[n]])

errcode dom_ml_init(void);
errcode dom_ml_done(void);
int valid_module(unsigned int idx, Modulclass class);

plerrcode dump_modent(ml_entry *entry, int verbose);
plerrcode dump_modulelist(void);
plerrcode dump_memberlist(void);

#endif

/*****************************************************************************/
/*****************************************************************************/
