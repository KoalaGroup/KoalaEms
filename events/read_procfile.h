#ifndef _read_procfile_h_
#define _read_procfile_h_

#include <emsctypes.h>

#define MAXTRIGGER 16

struct proc {
    char* name;
    int (*proc)(ems_u32* data, int size, struct proc* proc);
    int num_args;
    ems_u32 *args;
    /*int num_cachevals;*/
    /*ems_u32 *cachevals;*/
};

struct proclist {
    int trigger; /* -1 for all (0 for list; to be defined later) */
    struct proc **procs;
};

struct is {
    ems_u32 id;
    int lvds;
    struct proclist **proclists; /* last element == NULL */
};

/*
    to be defined locally:
    struct is *iss[]={...}; (last element == NULL)
*/

#if 0
int read_procfile(const char* name, struct iss* iss);
#endif

#endif






#if 0

static struct proclist list_A1 = {
    -1,
    procs_2000,
};

static struct is is_2000 = {
    2000,
    {&list_A1, &list_A1, 0},
};

static struct is *iss[] = {
    &is_2000,
    0,
};

#endif
