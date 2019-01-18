#ifndef _mxdc32_data_hxx_
#define _mxdc32_data_hxx_
#include "global.hxx"
#include <sys/time.h>
#include <cstddef>

/* TODO: add into a dedicated namespace with the name, maybe, 'DecodeUtils' */

// data from EMS event
// each event has a timestamp and the scaler data
// the mesytec data are stored in another structure, they are not directly related to EMS events
struct ems_event {
  // From data frame, event index of this ems event
  uint32_t event_nr; 
  bool evnr_valid;
  // From Timestamp subevent(or IS)
  struct timeval tv;
  bool tv_valid;
  // From Scalor subevent(or IS)
  uint32_t scaler[32];
  bool scaler_valid;
  // Beam counter?
  float bct[8];
  bool bct_valid;
};

// single (sub) event of a single module
struct mxdc32_event {
  mxdc32_event(): next(nullptr) {};
  void invalidate();
  struct mxdc32_event *next; // used for linked-list
  uint32_t data[34]; // channel data words including trigger channel, the index is ch_id
  uint32_t header; // header word
  uint32_t footer; // EOE word
  uint64_t ext_stamp; // extended timestamp (extracted)
  bool ext_stamp_valid; // whether extended timestamp is found in the data frame
  int64_t timestamp; // timestamp of this event (extracted and with ext_stamp)
  uint64_t evnr; // koala event number of this event in this moudle, couted by parse_mxdc32
  int len; // number of following words in this event
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
  class mxdc32_depot* fMxdc32Depot;
};

class  mxdc32_depot {
public:
  static mxdc32_depot* Instance();
  mxdc32_event* get(void);
  void put(mxdc32_event*);

protected:
  mxdc32_depot(void): first(0) {};
  friend class Mxdc32DepotDestroyer;
  virtual ~mxdc32_depot();

private:
  mxdc32_event *first;

  static mxdc32_depot* _fInstance;
  static Mxdc32DepotDestroyer _fDestroyer;
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
  };
  struct mxdc32_event* prepare_event(int);
  void store_event(void);
  struct mxdc32_event* drop_event(void);

private:
  mxdc32_depot *fDepot;
  //uint32_t marking_type;
  int64_t last_time;
  //int64_t last_diff;
  //int64_t offset;
  struct mxdc32_event *prepared;
  struct mxdc32_event *first;
  struct mxdc32_event *last;
  struct mxdc32_statist statist;
};

// koala_event contains the module data corresponding to the same event, i.e. synchronized data.
// It also contains the ems event infomation corresponding to this KOALA event
struct koala_event {
  koala_event();
  ~koala_event();
  // koala_event *next;
  struct ems_event event_data; // copy of the actual ems event
  // including scaler data
  struct mxdc32_event *events[nr_mesymodules];
  uint32_t mesypattern; // whether corresponding module has data in this event
};

#endif
