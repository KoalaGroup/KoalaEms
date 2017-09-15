/*
 * lowlevel/devices.h
 * 
 * created 01.Oct.2002 PW
 *
 * $ZEL: devices.h,v 1.20 2008/06/28 21:47:53 wuestner Exp $
 */

#ifndef _devices_h_
#define _devices_h_

#include <objecttypes.h>
#include <errorcodes.h>

/*
 * generic_dev is some kind of "base class" of the special "classes"
 * fastbus_dev, vme_dev, camac_dev (and hopefully others (later)) defined
 * in subdirectories of lowlevel.
 * struct dev_generic contains all members which are common for all these
 * "classes"
 * this construct will make casts from  special "classes" (which have
 * struct dev_generic as their first element) to struct generic_dev easy
 */
struct generic_dev; /* see some lines below */
struct dev_generic {
    Modulclass class;
    int alldev_idx;  /* index in devices */
    int dev_idx;     /* index in xdevices[class] */
/* XXX should we store an index in struct generic_dev** devices here to
   make find_device_index() unnecessary? */
   int online;
#ifdef DELAYED_READ
    int delayed_read_enabled;
#endif
    errcode (*done)(struct generic_dev*);
    /*
        after 'done' _only_ procedure pointers are allowed here;
        all these pointers from 'done' to 'DUMMY' in the real device-structs
        are tested whether they are initialised
    */
#ifdef DELAYED_READ
    void (*reset_delayed_read)(struct generic_dev*);
    int (*read_delayed)(struct generic_dev*);
    int (*enable_delayed_read)(struct generic_dev*, int);
#endif
    plerrcode (*init_jtag_dev)(struct generic_dev*, void* jdev);
};
struct generic_dev {
    struct dev_generic generic;
};

/*
 * devices is the list (of length numdevs) of
 * all registered devices of all types
 */
extern struct generic_dev** devices;
extern int numdevs;

/*
 * xdevices is an array of arrays of indices for the devices of all types
 * the size of each subarray is numxdevs[Modulclass]
 */
extern int* xdevices[modul_invalid]; /* index in devices[] */
extern int numxdevs[modul_invalid]; /* number of devices */

void devices_init(void);
errcode devices_done(void);

int register_device(Modulclass class, struct generic_dev* dev, const char*);
void destroy_device(Modulclass class, int crate);

/* slow, with checks */
/* find a device of type 'class' */
errcode find_device(Modulclass class, unsigned int crate, struct generic_dev**);
/* find a device of type 'class' and checks whether it is online */
plerrcode find_odevice(Modulclass class, unsigned int crate, struct generic_dev**);

/* find a device of specific type and checks whether it is online */
#define find_camac_odevice(crate, dev)                                   \
({                                                                       \
    struct generic_dev *device;                                          \
    plerrcode pres;                                                      \
    if ((pres=find_odevice(modul_camac, crate, &device))==plOK && dev)   \
        *dev=(struct camac_dev*)device;                                  \
    pres;                                                                \
})
#define find_lvd_odevice(crate, dev)                                     \
({                                                                       \
    struct generic_dev *device;                                          \
    plerrcode pres;                                                      \
    if ((pres=find_odevice(modul_lvd, crate, &device))==plOK && dev)     \
        *dev=(struct lvd_dev*)device;                                    \
    pres;                                                                \
})
#define find_fastbus_odevice(crate, dev)                                 \
({                                                                       \
    struct generic_dev *device;                                          \
    plerrcode pres;                                                      \
    if ((pres=find_odevice(modul_fastbus, crate, &device))==plOK && dev) \
        *dev=(struct fastbus_dev*)device;                                \
    pres;                                                                \
})
#define find_vme_odevice(crate, dev)                                     \
({                                                                       \
    struct generic_dev *device;                                          \
    plerrcode pres;                                                      \
    if ((pres=find_odevice(modul_vme, crate, &device))==plOK && dev)     \
        *dev=(struct vme_dev*)device;                                    \
    pres;                                                                \
})
#define find_perf_odevice(crate, dev)                                    \
({                                                                       \
    struct generic_dev *device;                                          \
    plerrcode pres;                                                      \
    if ((pres=find_odevice(modul_perf, crate, &device))==plOK && dev)    \
        *dev=(struct perf_dev*)device;                                   \
    pres;                                                                \
})
#define find_can_odevice(crate, dev)                                     \
({                                                                       \
    struct generic_dev *device;                                          \
    plerrcode pres;                                                      \
    if ((pres=find_odevice(modul_can, crate, &device))==plOK && dev)     \
        *dev=(struct can_dev*)device;                                    \
    pres;                                                                \
})
#define find_caenet_odevice(crate, dev)                                  \
({                                                                       \
    struct generic_dev *device;                                          \
    plerrcode pres;                                                      \
    if ((pres=find_odevice(modul_caenet, crate, &device))==plOK && dev)  \
        *dev=(struct caenet_dev*)device;                                 \
    pres;                                                                \
})
#define find_sync_odevice(crate, dev)                                    \
({                                                                       \
    struct generic_dev *device;                                          \
    plerrcode pres;                                                      \
    if ((pres=find_odevice(modul_sync, crate, &device))==plOK && dev)    \
        *dev=(struct sync_dev*)device;                                   \
    pres;                                                                \
})
#define find_pcihl_odevice(crate, dev)                                   \
({                                                                       \
    struct generic_dev *device;                                          \
    plerrcode pres;                                                      \
    if ((pres=find_odevice(modul_pcihl, crate, &device))==plOK && dev)   \
        *dev=(struct hlral*)device;                                      \
    pres;                                                                \
})

/* fast, without any range checks */
#define get_gendevice(class, crate) \
    (devices[xdevices[class][crate]])
#define get_camac_device(crate) \
    (struct camac_dev*)get_gendevice(modul_camac, (crate))
#define get_fastbus_device(crate) \
    (struct fastbus_dev*)get_gendevice(modul_fastbus, (crate))
#define get_vme_device(crate) \
    (struct vme_dev*)get_gendevice(modul_vme, (crate))
#define get_lvd_device(crate) \
    (struct lvd_dev*)get_gendevice(modul_lvd, (crate))
#define get_perf_device(crate) \
    (struct perf_dev*)get_gendevice(modul_perf, (crate))
#define get_can_device(crate) \
    (struct can_dev*)get_gendevice(modul_can, (crate))
#define get_caenet_device(crate) \
    (struct caenet_dev*)get_gendevice(modul_caenet, (crate))
#define get_sync_device(crate) \
    (struct sync_dev*)get_gendevice(modul_sync, (crate))
#define get_pcihl_device(crate) \
    (struct hlral*)get_gendevice(modul_pcihl, (crate))

#define num_devices(class) \
    (numxdevs[class])

void init_device_dummies(struct dev_generic*);

#ifdef DELAYED_READ
void reset_delayed_read(void);
int read_delayed(void);
#else
#define read_delayed() ({do {} while(0); 0;})
#endif

enum Modulclass find_devclass(const char* name);


#endif
