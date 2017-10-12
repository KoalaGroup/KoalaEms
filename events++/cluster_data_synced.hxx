/*
 * ems/events++/cluster_data.hxx
 * 
 * created 2006-Apr-28 PW
 *
 * $ZEL: cluster_data.hxx,v 1.5 2007/02/12 10:09:46 wuestner Exp $
 */

#ifndef _ems_objects_hxx_
#define _ems_objects_hxx_

#include "config.h"
#include <clusterformat.h>

class ems_cluster {
  public:
    ems_cluster(ems_u32* buf):type((enum clustertypes)buf[2]),
            size(buf[0]), data(buf) {}
    ~ems_cluster() {delete[] data;}

    enum clustertypes type;
    int size;
    ems_u32 *data;
};

class ems_file {
  public:
    ems_file();
    ~ems_file();

    timeval tv;
    string name;
    unsigned int fragment;
    bool complete;
    time_t ctime;
    time_t mtime;
    mode_t mode;
    string content;

    void dump(int level) const;
};

class ems_text {
  public:
    ems_text(void);
    ~ems_text();

    timeval tv;
    string key;
    int nr_lines;
    string* lines;

    void dump(int level) const;
};

class ems_ved_info {
  public:
    ems_ved_info(void);
    ~ems_ved_info();

    struct is_info {
        is_info(void):importance(0), decoding_hints(0) {}
        int IS_ID;
        int importance;
        int decoding_hints;
    };
    struct ved_info {
        ved_info(void):nr_is(0), is_infos(0), sync(-1) {}
        ~ved_info();
        string name;
        int VED_ID;
        int nr_is;
        is_info *is_infos;
        int sync;
    };

    bool valid;
    timeval tv;
    int version;
    int nr_veds;
    ved_info *ved_infos;

    int find_ved_idx(int VED_ID) const;
    int find_ved_idx(string name) const;
    int find_is_idx(int ved_idx, int IS_ID) const;
    int find_is(int IS_ID, int* ved_idx, int* is_idx) const;
    void dump(int level) const;
};

#if 0
struct ems_subevent {
    ems_subevent():data(0) {}
    ~ems_subevent() {delete[] data;}

    ems_subevent *next;
    ems_u32 sev_id;
    ems_u32 size;
    ems_u32 *data;

    void dump(int level) const;
};
#endif

class ved_configuration {
  public:
    ved_configuration(const string);
    ved_configuration(const ems_text*);
    ~ved_configuration();

    ved_configuration* next;
    const ems_text* text;
    string name;
    string vedname;
    int eventbuilder;
    int triggermaster;
    struct isid {
        int idx, ID;
    };
    int num_isids;
    isid *isids;

    void add_isid(int idx, int id);
    void dump(int level) const;
};

class ved_configurations {
  public:
    ved_configurations():confs(0) {};
    ~ved_configurations();

    ved_configuration* confs; // linked list

    void add_conf(ved_configuration* conf);
};

class sync_connections {
  public:
    sync_connections(const ems_file *file);
    ~sync_connections();

    const ems_file* file;
    struct conn {
        string crate_name;
        int addr;
        int port;
    };
    int num_conn;
    struct conn* conn;

    void add_conn(string crate_name, int addr, int port);
};

struct event_info {
    ems_u32 ev_no;
    double tdt;
    int rejected, accepted;
    int time;
    int trigger;
};

class hist {
  public:
    hist():num(0), sum(0), qsum(0) {}
    double num;
    double sum;
    double qsum;
    void add(double);
    void dump() const;
};

class ems_data {
  public:
    ems_data();
    ~ems_data();

    int nr_texts;
    ems_text** texts;
    int nr_files;
    ems_file** files;
    ems_ved_info ved_info;
    class sync_connections* sync_connections;
    class ved_configurations ved_configurations;

    struct is_data {
        is_data():next(0), data(0) {};
        ~is_data();
        struct is_data* next;
        ems_u32 ev_no;
        int size;
        ems_u32* data;
    };
    struct ved_slot;
    struct is_data_slot {
        is_data_slot():first(0), last(0),records(0) {};
        ~is_data_slot();
        struct is_data* first;
        struct is_data* last;
        void add(struct is_data*);
        void purge_before(ems_u32 ev_no);
        void purge_first();
        hist size;
        long int records;
    };

    struct ved_slot {
        ved_slot():is_data(0) {}
        ~ved_slot();
        struct is_data_slot* is_data;
        bool required;
        double new_ldt;
        hist ldt;
        hist size;
    };

    struct sync_ved {
        int ved_idx;
        int is_idx;
        is_data_slot* slot;
    };

    struct ved_slot* slotlist[16*8];

    struct ved_slot* ved_slots;
    struct sync_ved sync_ved;
    hist tdt;
    hist max;
    hist size;

    int parse_cluster(const ems_cluster*);
    int parse_timestamp(const ems_cluster*, timeval*);
    int parse_cluster_file(const ems_cluster*, ems_file**);
    int parse_cluster_text(const ems_cluster*, ems_text*);
    int parse_cluster_ved_info(const ems_cluster*, ems_ved_info*);
    int parse_cluster_no_more_data(const ems_cluster*);
#if 0
    //int parse_cluster_events(const ems_cluster*);
    //int parse_cluster_events_normal(const ems_cluster *cluster);
    //int parse_cluster_events_lvds(const ems_cluster *cluster);
    //int parse_lvds_events(ems_u32 *event, int ved_idx);
#endif
    int parse_subevent_lvd(ems_u32*, int, ems_u32, int);
    int parse_subevent_normal(ems_u32*, int, ems_u32, int);
    int parse_subevent(ems_u32* subevent, int ved_idx, int evno, int trigno);
    int parse_event(ems_u32* event, int ved_idx);
    int parse_cluster_events(const ems_cluster *cluster);
    struct is_data_slot* find_is_slot(int VED_ID, int IS_ID) const;
    void clear_ved_flags(void);
};

#endif
