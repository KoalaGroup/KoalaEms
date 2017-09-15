#ifndef _read_procfile_h_
#define _read_procfile_h_

#include <emsctypes.h>

#define MAXTRIGGER 16

struct proc {
    char* name;
    int num_args;
    ems_u32* args;
    int num_cachevals;
    ems_u32* cachevals;
};

struct proclist {
    int num_procs;
    struct proc **procs;
};

struct is {
    ems_u32 id;
    struct proclist* proclists[16]; /* index is trigger; points to element of lists */
    struct proclist lists[16];      /* index does not have any meaning */
    struct proclist* current_proclist;
};

struct iss {
    struct is** iss;
    int num_is;
};

int read_procfile(const char* name, struct iss* iss);


#endif
