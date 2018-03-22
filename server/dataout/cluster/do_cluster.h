/*
 * dataout/cluster/do_cluster.h
 * created      04.04.97
 * 
 * $ZEL: do_cluster.h,v 1.17 2016/05/10 19:58:46 wuestner Exp $
 */

#ifndef _do_cluster_h_
#define _do_cluster_h_

#include <sconf.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <emsctypes.h>
#include <errorcodes.h>
#include <objecttypes.h>
#include "cluster.h"
#include "../../main/scheduler.h"

struct do_cluster_data {
    struct Cluster* active_cluster;
    int idx;
    int bytes;
};

#if 0
struct do_filebuf_data {
    int master; /* -1: I am the master; >=0: I am slave */
    char* bufdir; /* directory containing the buffers */
};
#endif

struct do_procs {
    errcode (*start) (int);
    errcode (*reset) (int);
    void (*freeze) (int);
    void (*advertise) (int, struct Cluster*);
    void (*patherror) (int, int);
    void (*cleanup) (int);
    errcode (*wind) (int, int);
    errcode (*status) (int);
    plerrcode (*filename) (int, const char**);
};

/* typedef enum {endien_native, endien_big, endien_little} endien_type; */
struct Dataout_cl {     /* wird initialisiert in: */
    struct do_procs procs;  /* init */
    struct seltaskdescr* seltask;  /* start (or init for tape) */
    int errorcode;          /* bei error */
    int suspended;          /* start */
    int vedinfo_sent;       /* start */
    DoRunStatus do_status;  /* init */
    DoSwitch doswitch;      /* init */
    /* endien_type endien; *//* little, big, native */
    struct Cluster* advertised_cluster;
    union {
        struct do_cluster_data c_data;
#if 0
        /*do_stream_data s_data;*/
        struct do_filebuf_data f_data;
#endif
    } d;
    void *s;
};

struct Dataout_statist {
    ems_u64 clusters;
    ems_u64 words;
    ems_u64 events;
    ems_u64 suspensions;
};

extern struct Dataout_cl dataout_cl[];      /* defined in cl_interface.c */
extern struct Dataout_statist do_statist[]; /* defined in cl_interface.c */

extern char *daq_lock_link;                 /* defined in cluster.c */
extern char *tsm_lock_link;                 /* defined in cluster.c */

struct Cluster* create_ved_cluster(void);
void clusters_link(struct Cluster* cluster);
errcode do_cluster_sock_init(int do_idx);
errcode do_cluster_tape_init(int do_idx);
errcode do_cluster_file_init(int do_idx);
errcode do_cluster_file_check_reopen(int do_idx);
errcode do_cluster_autosock_init(int do_idx);
errcode do_dummy_init(int do_idx);
void do_cluster_write(int path, enum select_types selected, union callbackdata data);
errcode do_NotImp_err(void);
errcode do_NotImp_err_ii(int, int);
void do_NotImp_void_ii(int, int);
void do_NotImp_void_i(int);
void do_NotImp_void(void);
void do_cluster_advertise(int do_idx, struct Cluster* cl);
void freeze_dataouts(void);
int printuse_clusters(FILE* outfilepath);
int cluster_using_shm(void);
errcode clusters_restart(void);
void clusters_conv_to_network(struct Cluster* cluster);
void send_ved_cluster(int do_idx);

void calculate_checksum(struct Cluster* cluster);

void set_max_ev_per_cluster(ems_i32, ems_i32*);
void set_max_time_per_cluster(ems_i32, ems_i32*); /* time in ms */

#endif

/*****************************************************************************/
/*****************************************************************************/
