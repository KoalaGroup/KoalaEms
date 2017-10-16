/*
 * ems/events++/cluster_data_timed.hxx
 * 
 * created 2006-Nov-14 PW
 *
 * $ZEL: cluster_data_timed.hxx,v 1.1 2007/02/25 19:52:33 wuestner Exp $
 */

#ifndef _ems_objects_hxx_
#define _ems_objects_hxx_

#include "config.h"
#include "cxxcompat.hxx"
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
        int IS_ID;
        int importance;
    };
    struct ved_info {
        int VED_ID;
        int nr_is;
        is_info *is_infos;
    };

    bool valid;
    timeval tv;
    int version;
    int nr_veds;
    ved_info *ved_infos;

    void dump(int level) const;
};

struct ems_subevent {
    ems_subevent():next(0), data(0) {}
    ~ems_subevent() {delete[] data;}

    ems_subevent *next;
    ems_u32 sev_id;
    ems_u32 size;
    ems_u32 *data;
};

struct ved_subevents {
    ved_subevents():next(0), nr_subevents(0), subevents(0), last_subevent(0) {};
    ~ved_subevents();
    struct ved_subevents *next;
    ems_u32 ev_no;
    ems_u32 trigger;
    ems_u32 triggertime;
    double tdiff;        // distance to previous event
    ems_u32 nr_subevents;
    ems_subevent *subevents;
    ems_subevent *last_subevent;
};

struct ems_event {
    ems_event():nr_subevents(0), vsubevents(0), subevents(0) {}
    ~ems_event();

    ems_u32 event_nr;
    ems_u32 trigger;
    ems_u32 nr_subevents;
    ved_subevents* vsubevents;
    ems_subevent* subevents;
};

class ems_data {
  public:
    ems_data(void);
    ~ems_data();

    int nr_texts;
    ems_text** texts;
    int nr_files;
    ems_file** files;
    ems_ved_info ved_info;

    int parse_cluster(const ems_cluster*);

    int get_event(ems_event**);
    bool events_available(void);

    struct resolutions {
        unsigned int isId;
        double resolution;
    };
    resolutions *resols;
    int num_resols;

  protected:

    struct ved_slot {
        ved_slot():resolution(0.), trigger_is(-1), vsubevents(0),
                last_vsubevents(0), num_vsubevents(0) {};
        ~ved_slot();
        ems_data* parent;
        double resolution;
        unsigned int trigger_is;
        ved_subevents *vsubevents;
        ved_subevents *last_vsubevents;
        int num_vsubevents;
        int insert_subevent(ved_subevents*);
        ved_subevents *extract_subevent(void);
        double triggerdiff(ems_u32, ems_u32);
        int find_trigger_is(ved_subevents*);
    };

    int find_ved_idx(int);
    int parse_timestamp(const ems_cluster*, timeval*);
    int parse_cluster_file(const ems_cluster*, ems_file**);
    int parse_cluster_text(const ems_cluster*, ems_text*);
    int parse_cluster_ved_info(const ems_cluster*, ems_ved_info*);
    int parse_cluster_no_more_data(const ems_cluster*);
    int parse_cluster_events(const ems_cluster*);
    int resync(void);
    int nr_IS;
    int *IS_IDs;
    int nr_VED;
    int *VED_IDs;
    ved_slot *ved_slots;
};

#endif
