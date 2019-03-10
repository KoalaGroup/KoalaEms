#ifndef _ems_data_hxx_
#define _ems_data_hxx_

#include <cstdint>
#include <sys/time.h>

// data from EMS event
// each event has a timestamp and the scaler data
// the mesytec data are stored in another structure, they are not directly related to EMS events

class ems_event {
public:
  ems_event(): next(nullptr) {}
  void invalidate();
  void recycle();

  ems_event* next;
  // From data frame, event index of this ems event
  // can be used to check the continuity of the data flow in online mode
  uint32_t event_nr; 
  bool evnr_valid;
  // From Timestamp subevent(or IS)
  // can be used to calculate the hit rates and event rates
  struct timeval tv;
  bool tv_valid;
  // From Scalor subevent(or IS)
  uint32_t scaler[32];
  bool scaler_valid;
  // Beam counter?
  // Not used currently
  float bct[8];
  bool bct_valid;

private:
  friend class ems_depot;
  ~ems_event() {}
};

class ems_depot;

class EmsDepotDestroyer
{
public:
  EmsDepotDestroyer(ems_depot* depot=0) { fEmsDepot=depot; }
  ~EmsDepotDestroyer();

  void SetEmsDepot(ems_depot* depot) { fEmsDepot=depot; }

private:
  ems_depot* fEmsDepot;
};

class  ems_depot {
public:
  static ems_depot* Instance();
  ems_event* get(void);
  void put(ems_event*);

protected:
  ems_depot(void): first(0) {}
  friend class EmsDepotDestroyer;
  virtual ~ems_depot();

private:
  ems_event *first;

  static ems_depot* fInstance;
  static EmsDepotDestroyer fDestroyer;
};

// statistics for a single module
struct ems_statist {
  //struct timeval updated;
  //uint64_t scanned;  /* how often scan_events was called */
  uint64_t events;   /* number of event headers found */
  uint64_t words;    /* number of data words */
  //uint32_t triggers; /* trigger counter in footer */
  //uint32_t last_stamp[2];
  //bool last_stamp_valid[2];
};

class ems_private {
public:
  ems_private(): prepared(0), first(0), last(0)
  {
    fDepot = ems_depot::Instance();
  }
  ~ems_private();

  ems_event* prepare_event();
  ems_event* get_prepared();
  void store_event(void);
  ems_event* drop_event(void);
  bool  is_empty();
  ems_event* head();

private:
  ems_depot *fDepot;

  ems_event *prepared;
  ems_event *first;
  ems_event *last;

public:
  ems_statist statist;
};
#endif
