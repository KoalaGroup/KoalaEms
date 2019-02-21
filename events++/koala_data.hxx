#ifndef _koala_data_hxx_
#define _koala_data_hxx_

#include "global.hxx"
#include "mxdc32_data.hxx"
// koala_event contains the module data corresponding to the same event, i.e. synchronized data.
// [TODO] ??Shall it also contain the ems event infomation corresponding to this KOALA event?

class koala_event {
public:
  koala_event();
  // clean the buffer, after push-out from depot
  void invalidate();
  // recycle the buffer into depot
  void recycle();

  koala_event *next;
  // struct ems_event event_data; // copy of the actual ems event
  // including scaler data
  mxdc32_event *modules[nr_mesymodules];
  uint32_t mesypattern; // whether corresponding module has data in this event

private:
  // the buffer can only be newed & deleted in the depot
  friend class koala_depot;
  ~koala_event() {}
};

// depot for recycling unused koala_event structure
class koala_depot;

class KoalaDepotDestroyer
{
public:
  KoalaDepotDestroyer(koala_depot* depot=0){
    fKoalaDepot = depot;
  }
  ~KoalaDepotDestroyer();

  void SetKoalaDepot(koala_depot* depot){
    fKoalaDepot = depot;
  }

private:
  koala_depot *fKoalaDepot;
};

class koala_depot
{
public:
  static koala_depot* Instance();
  koala_event* get();
  void put(koala_event*);

protected:
  koala_depot(): first(nullptr) {}
  friend class KoalaDepotDestroyer;
  virtual ~koala_depot();

private:
  koala_event* first;

  static koala_depot* fInstance;
  static KoalaDepotDestroyer fDestroyer;
};

// statistics
struct koala_statist
{
  uint64_t events;
};

class koala_private
{
public:
  koala_private(): prepared(nullptr), first(nullptr), last(nullptr)
  {
    fDepot = koala_depot::Instance();
  }
  ~koala_private();

  koala_event* prepare_event();
  koala_event* get_prepared();
  void store_event();
  koala_event* drop_event();
  bool  is_empty();

private:
  koala_depot* fDepot;
  koala_event* prepared;
  koala_event* first;
  koala_event* last;

  koala_statist statist;
};

#endif
