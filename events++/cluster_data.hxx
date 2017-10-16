/*
 * ems/events++/cluster_data.hxx
 * 
 * created 2006-Apr-28 PW
 *
 * $ZEL: cluster_data.hxx,v 1.6 2014/07/07 20:02:38 wuestner Exp $
 */

#ifndef _ems_objects_hxx_
#define _ems_objects_hxx_

#include "config.h"
#include <string>
#include <sys/stat.h>
#include <sys/time.h>
#include <clusterformat.h>

class ems_cluster {
  public:
    ems_cluster(ems_u32* buf):type(static_cast<enum clustertypes>(buf[2])),
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
    std::string name;
    unsigned int fragment;
    bool complete;
    time_t ctime;
    time_t mtime;
    mode_t mode;
    std::string content;

    void dump(int level) const;
};

class ems_async2;

class ems_mqtt {
  public:
    ems_mqtt(void) {}
    virtual ~ems_mqtt() {}

    std::string topic;

    virtual int parse(const ems_async2&, const ems_u32*) =0;
    //virtual void dump(int level) const =0;
};

class ems_mqtt_file: public ems_mqtt {
  public:
    ems_mqtt_file(void) {}
    virtual ~ems_mqtt_file() {}

    timeval mtime;
    int version;
    std::string filename;
    std::string content;

    virtual int parse(const ems_async2&, const ems_u32*);
    //virtual void dump(int level) const;
};

class ems_mqtt_opaque: public ems_mqtt {
  public:
    ems_mqtt_opaque(void) {}
    virtual ~ems_mqtt_opaque() {}

    timeval mtime;
    int version;
    std::string filename;
    std::string content;

    virtual int parse(const ems_async2&, const ems_u32*);
    //virtual void dump(int level) const;
};

class ems_async2 {
  public:
    ems_async2();
    ~ems_async2();

    const ems_cluster *cluster; // !!! not valid after parsing !!!
    timeval received;
    ems_u32 flags;
    ems_u32 version;
    std::string url;
    bool is_mqtt; // to be changed later, if other types exist
    union content {
        void *any;
        ems_mqtt *mqtt;
    } content;

    int parse(const ems_cluster *cluster);
    void dump(int level) const;
};

class ems_text {
  public:
    ems_text(void);
    ~ems_text();

    timeval tv;
    std::string key;
    int nr_lines;
    std::string* lines;

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
        ved_info(void):nr_is(0), skip(false), is_infos(0) {}
        int VED_ID;
        int nr_is;
        bool skip;
        is_info *is_infos;
    };

    bool valid;
    timeval tv;
    int version;
    int nr_veds;
    ved_info *ved_infos;

    int find_ved_idx(int VED_ID);
    int find_is_idx(int ved_idx, int IS_ID);
    void dump(int level) const;
};

struct ems_subevent {
    ems_subevent():data(0) {}
    ~ems_subevent() {delete[] data;}

    ems_subevent *next;
    ems_u32 sev_id;
    ems_u32 size;
    ems_u32 *data;

    void dump(int level) const;
};

struct ems_event {
    ems_event():nr_subevents(0), subevents(0) {}
    ~ems_event();

    ems_u32 event_nr;
    ems_u32 trigger;
    ems_u32 nr_subevents;
    ems_subevent* subevents;

    void dump(int level) const;
};

class ems_data {
  public:
    enum modus {sorted, unsorted};
    ems_data(enum modus=sorted);
    ~ems_data();

    int nr_texts;
    ems_text** texts;
    int nr_files;
    ems_file** files;
    ems_async2 *current_async2;
    ems_ved_info ved_info;
    int nr_skip_is;
    int *skip_is;
    ems_u32 allow_uncomplete;
    int purged_uncomplete;

    int parse_cluster(const ems_cluster*);

    int get_event(ems_event**);
    bool events_available(void);
    static int parse_timestamp(const ems_cluster*, timeval*);

  protected:
    struct event_info {
        event_info(int nr_VED, int _ev_no);
        ~event_info();

        event_info *next;
        bool complete;
        bool *veds;
        ems_u32 ev_no;
        ems_subevent* last_subevent;
        ems_event *event;
    };
    enum modus modus;
    void delist_fist_event(void);
    void purge_uncomplete(void);
    event_info* find_ev_info(ems_u32, int);
    event_info* get_ev_info(int ev_no, int trigno, int ved_idx);
    int parse_cluster_file(const ems_cluster*, ems_file**);
    int parse_cluster_text(const ems_cluster*, ems_text*);
    int parse_cluster_async2(const ems_cluster*, ems_async2**);
    int parse_cluster_ved_info(const ems_cluster*, ems_ved_info*);
    int parse_cluster_no_more_data(const ems_cluster*);
    //int parse_cluster_events(const ems_cluster*);
    //int parse_cluster_events_normal(const ems_cluster *cluster);
    //int parse_cluster_events_lvds(const ems_cluster *cluster);
    //int parse_lvds_events(ems_u32 *event, int ved_idx);
    int parse_subevent_lvd(ems_u32*, int, ems_u32, int);
    int parse_subevent_normal(ems_u32*, int, ems_u32, int);
    int parse_subevent(ems_u32* subevent, int ved_idx, int evno, int trigno);
    int parse_event(ems_u32* event, int ved_idx);
    int parse_cluster_events(const ems_cluster *cluster);

    ems_u32 smallest_ev_no;
    ems_u32 last_complete;
#if 0
    int nr_IS;
    int *IS_IDs;
    int nr_VED;
    int *VED_IDs;
#endif
    // linked list of event_info containing pointers to event data
    event_info* events;
    event_info* last_event;
    event_info** last_ved_event; // event_info*[nr_VED]
};

#endif
