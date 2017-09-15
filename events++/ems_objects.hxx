/*
 * ems/events++/ems_objects.hxx
 * 
 * created 2004-12-05 PW
 */

#ifndef _ems_objects_hxx_
#define _ems_objects_hxx_

#include "config.h"
#include "cxxcompat.hxx"

#include <vector>
#include <queue>

class file_object {
  public:
    //file_object(char* name):name(name), fragment(-1) {}
    file_object(string name):name(name), fragment(-1) {}
    //~tfile_object();

    string name;
    string content;
    int fragment;

    void dump();
    static file_object& find_file_object(char* name);
};

class text_object {
  public:
    text_object(void);
    ~text_object();

    string key;
    //int num;
    vector<string> lines;

    void add_line(char* s);
    void dump(void);
};

class sev_object {
  public:
    ems_u32 event_nr;
    ems_u32 trigger;
    ems_u32 sev_id;
    vector<ems_u32> data;
};

class ems_object {
  public:
    ems_object(): next_evno(0), ignore_missing(false), ignored_missings(0),
        _runnr_valid(false), _runnr(-1) {}

    deque<sev_object>* ved_slots;
    int num_ved;
    vector<ems_u32> sev_ids;
    vector<ems_u32> ved_ids;
    ems_u32 next_evno;
    vector<text_object> texts;
    vector<file_object> files;

    bool ignore_missing;
    int ignored_missings;

    int get_vedindex(ems_u32 id);
    bool runnr_valid() const {return _runnr_valid;}
    int runnr() const {return _runnr;}
    file_object& find_file_object(string name);
    //text_object& find_text_object(string name);

    int parse_file(ems_u32* buf, int num);
    int parse_text(ems_u32* buf, int num);
    int parse_ved_info(ems_u32* buf, int num);
    int replace_file_object(string name, string filename);

    void save_file(string name, bool force);
    //void save_text(string name, bool forc);

  protected:
    bool _runnr_valid;
    int  _runnr;
    void parse_superheader(text_object&);
};

#endif
