#ifndef _mxdc32_data_hxx_
#define _mxdc32_data_hxx_
#include "global.hxx"
#include <sys/time.h>
#include <cstddef>

/* TODO: add into a dedicated namespace with the name, maybe, 'DecodeUtils' */


// single (sub) event of a single module
class mxdc32_event {
public:
  mxdc32_event(): next(nullptr) {}
  void invalidate();
  void recycle();

  mxdc32_event *next; // used for linked-list
  uint32_t data[34]; // channel data words including trigger channel, the index is ch_id
  uint32_t header; // header word
  uint32_t footer; // EOE word
  uint64_t ext_stamp; // extended timestamp (extracted)
  bool ext_stamp_valid; // whether extended timestamp is found in the data frame
  int64_t timestamp; // timestamp of this event (extracted and with ext_stamp)
  uint64_t evnr; // koala event number of this event in this moudle, couted by parse_mxdc32
  int len; // number of following words in this event

private:
  friend class mxdc32_depot;
  ~mxdc32_event() {}
};

// depot for empty event structures (to avoid 'delete' and 'new')
class mxdc32_depot;

class Mxdc32DepotDestroyer
{
public:
  Mxdc32DepotDestroyer(mxdc32_depot* depot=0) { fMxdc32Depot=depot; }
  ~Mxdc32DepotDestroyer();

  void SetMxdc32Depot(mxdc32_depot* depot) { fMxdc32Depot=depot; }

private:
  mxdc32_depot* fMxdc32Depot;
};

class  mxdc32_depot {
public:
  static mxdc32_depot* Instance();
  mxdc32_event* get(void);
  void put(mxdc32_event*);

protected:
  mxdc32_depot(void): first(0) {}
  friend class Mxdc32DepotDestroyer;
  virtual ~mxdc32_depot();

private:
  mxdc32_event *first;

  static mxdc32_depot* fInstance;
  static Mxdc32DepotDestroyer fDestroyer;
};

// statistics for a single module
struct mxdc32_statist {
  //struct timeval updated;
  //uint64_t scanned;  /* how often scan_events was called */
  uint64_t events;   /* number of event headers found */
  uint64_t words;    /* number of data words */
  //uint32_t triggers; /* trigger counter in footer */
  //uint32_t last_stamp[2];
  //bool last_stamp_valid[2];
};

// data for a single module including a linked list of events
class mxdc32_private {
public:
  mxdc32_private():last_time(0), prepared(0), first(0), last(0)
  {
    fDepot = mxdc32_depot::Instance();
  }
  ~mxdc32_private();

  mxdc32_event* prepare_event();
  mxdc32_event* get_prepared();
  void store_event(void);
  mxdc32_event* drop_event(void);
  bool is_empty();

  uint64_t get_statist_events()
  {
    return statist.events;
  }

  uint64_t get_statist_words()
  {
    return statist.words;
  }

private:
  mxdc32_depot *fDepot;
  //uint32_t marking_type;
  int64_t last_time;
  //int64_t last_diff;
  //int64_t offset;
  mxdc32_event *prepared;
  mxdc32_event *first;
  mxdc32_event *last;

public:
  mxdc32_statist statist;
};

#endif
