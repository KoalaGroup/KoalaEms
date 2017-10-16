/*
 * ems/events++/cluster_data_sync.hxx
 * 
 * created 2007-Feb-16 PW
 *
 * $ZEL: cluster_data_sync.hxx,v 1.1 2007/02/25 19:56:06 wuestner Exp $
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
        ved_info(void):nr_is(0), is_infos(0) {}
        int VED_ID;
        int nr_is;
        is_info *is_infos;
        int find_is_idx(int IS_ID) const;
    };

    bool valid;
    timeval tv;
    int version;
    int nr_veds;
    ved_info *ved_infos;

    int find_ved_idx(int VED_ID) const;
    void dump(int level) const;
};

struct ems_subevent {
    ems_subevent():data(0), trigdiff(-1.), trigdiff0(-1.) {}
    ~ems_subevent() {delete[] data;}

    ems_u32 ev_nr;
    ems_u32 trigger_id;
    ems_u32 sev_id;
    ems_u32 size;
    ems_u32 *data;

    double trigdiff;
    double trigdiff0;

    void dump(int level) const;
};

template<class Q> class FIFO {
  public:
    FIFO();
    ~FIFO();

    Q* operator [](int) const;
    Q* last(void) const {return _last;}
    int add(Q*);
    void remove(void);
    int depth(void) const;
    void dump(const char* text) const;

  protected:
    int ringmask;
    int first_used;
    int first_free;
    Q **ring;
    Q *_last;
};

class ems_fifo: public FIFO<ems_subevent> {
  public:
    ems_fifo():ignore(false), skip(false), skip_always(false),
        private_ptr(0) {}
    ems_u32 is_id;
    bool ignore, skip, skip_always;
    void* private_ptr;
};

class C_ems_data {
  public:
    C_ems_data(void);
    ~C_ems_data();

    int nr_texts;
    ems_text** texts;
    int nr_files;
    ems_file** files;
    ems_ved_info ved_info;

    int max_fifodepth;
    int min_fifodepth; // minimum depth before events_available is successfull

    int nr_is;
    ems_fifo *fifo; // fifo[nr_is]
    int find_is_index(ems_u32) const;
    int disable_empty_fifos(void);

    int parse_cluster(const ems_cluster*);
    int depth(void) const;
    bool events_available(void) const;

  protected:
    int parse_timestamp(const ems_cluster*, timeval*);
    int parse_cluster_file(const ems_cluster*, ems_file**);
    int parse_cluster_text(const ems_cluster*, ems_text*);
    int parse_cluster_ved_info(const ems_cluster*, ems_ved_info*);
    int parse_cluster_no_more_data(const ems_cluster*);
    int parse_subevent_lvd(ems_u32*, int, ems_u32, ems_u32);
    int parse_subevent_normal(ems_u32*, int, ems_u32, int);
    int parse_subevent(ems_u32* subevent, int ved_idx, int evno, int trigno);
    int parse_event(ems_u32* event, int ved_idx);
    int parse_cluster_events(const ems_cluster *cluster);
};

#endif
