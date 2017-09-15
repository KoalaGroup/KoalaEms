/*
 * clusterreader.hxx
 * 
 * $ZEL: clusterreader.hxx,v 1.11 2004/11/26 14:40:13 wuestner Exp $
 * 
 * created: 12.03.1998 PW
 * last changed: 31.07.1998 PW
 */

#ifndef _clusterreader_hxx_
#define _clusterreader_hxx_

#include "config.h"
#include "cxxcompat.hxx"
#include <sys/time.h>
#include <iopathes.hxx>
#include "cluster.hxx"
#include <debuglevel.hxx>
#include "make_file.hxx"

class Clusterreader;
///////////////////////////////////////////////////////////////////////////////
class C_eventstatist
  {
  public:
    C_eventstatist(Clusterreader*);
  protected:
    Clusterreader* reader;
    int after_seconds, after_clusters;
    struct timeval letzt;
    int last_clusters_read;
    int last_events_extracted;
    int last_bytes_extracted;
    int last_bytes_read;
  public:
    int clusters_read;
    int events_read;
    int events_extracted;
    int bytes_extracted;
    int bytes_read;
    void reset();
    void print(int);
    void setup(int, int);
  };
///////////////////////////////////////////////////////////////////////////////
class CLUSTERcontainer: public C_cluster
  {
  public:
    CLUSTERcontainer(int* cluster, int size, int& num)
      :C_cluster(cluster, size, num), next(0) {}
    virtual ~CLUSTERcontainer() {}
  public:
    CLUSTERcontainer* next;
    int* next_data; // zeigt auf den Anfang des naechsten Events
    unsigned int firstevent, lastevent;
  };
///////////////////////////////////////////////////////////////////////////////
class VEDslot
  {
  public:
    VEDslot();
    ~VEDslot();
  protected:
    Clusterreader* reader;
    CLUSTERcontainer *bottom, *top;
    unsigned int bottomevent_, topevent_, nextevent_;
    unsigned int last_seen_event_;
  public:
    void init(Clusterreader*);
    void add_cluster(CLUSTERcontainer*);
    unsigned int get_last_of_bottom() const;
    unsigned int topevent() const {return topevent_;}
    unsigned int bottomevent() const {return bottomevent_;}
    unsigned int last_seen_event() const {return last_seen_event_;}
    void purge_cluster();
    void purge_events(unsigned int);
    int test_event(unsigned int) const;
    int eventinfo(unsigned int, int&, int&, int&);
    int get_subevent(unsigned int, int*);
  };
///////////////////////////////////////////////////////////////////////////////
class Clusterreader: public nocopy {
  friend class VEDslot;
  friend class C_eventstatist;
  public:
    Clusterreader(const STRING& pathname, int literally);
    Clusterreader(int path);
    Clusterreader(const char* host, int port);
    ~Clusterreader();
    typedef enum {input_ok, input_exhausted, input_error} inputstatus;
  protected:
    C_iopath iopath_;
    int maxmem_;
    int recsize_;
    VEDslot* veds;
    int numveds;
    int num_cached_clusters;
    inputstatus input_status_;
    C_eventstatist statist_;
    int save_header_;
    CLUSTERcontainer* (Clusterreader::*reader)();
    CLUSTERcontainer* tapereader();
    CLUSTERcontainer* streamreader();
    unsigned int next_evnum;
    struct vedinfo {
          vedinfo():vednum(0), isnum(0), islist(0), indices(0) {}
          ~vedinfo() {delete[] islist; delete[] indices;}
          int vednum;
          int isnum;
          int* islist;
          int* indices;
          void reset() {delete[] islist; delete[] indices;
              vednum=0; isnum=0; islist=0; indices=0;}
          int vedid2vedidx(int id);
    };
    vedinfo ved_info;
    fileinfo f_info;

    void purge_cluster();
    void got_events(CLUSTERcontainer*);
    void got_ved_info(CLUSTERcontainer*);
    void got_text(CLUSTERcontainer*);
    void got_file(CLUSTERcontainer*);
    void got_no_more_data(CLUSTERcontainer*);
    int* get_event(unsigned int);
  public:
    C_iopath& iopath() {return iopath_;}
    C_eventstatist& statist() {return statist_;}
    void restart();
    void reset();
    int* get_next_event();
    int* get_best_event(int lowdelay);
    int maxmem(int x) {int v=maxmem_; maxmem_=x; return v;}
    int maxmem() const {return maxmem_;}
    int recsize(int x) {int v=recsize_; recsize_=x; return v;}
    int recsize() const {return recsize_;}
    void read_cluster();
    int input_status() const {return input_status_;}
    void list_clusters();
    int save_header(int);
};
#endif
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
