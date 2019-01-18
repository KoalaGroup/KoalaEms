#include "mxdc32_data.hxx"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <limits>       // std::numeric_limits
#include <unistd.h>
#include <fcntl.h>

using namespace std;

//---------------------------------------------------------------------------//
void
mxdc32_event::invalidate()
{
  bzero(this, sizeof(*this));
}

//---------------------------------------------------------------------------//
/*
 * prepare_event prepares a new, empty event structure
 */
struct mxdc32_event*
mxdc32_private::prepare_event(int ID)
{
    // if there is an unused event (previous data where invalid), return this
    // otherwise fetch one from the depot
    if (prepared==0)
        prepared=fDepot->get();
    if (prepared==0) { // no more memory?
        cout<<"cannot create storage for the next event"<<endl;
        return 0;
    }
    prepared->invalidate();
    return prepared;
}
//---------------------------------------------------------------------------//
/*
 * store_event stores the previously prepared event at the end of
 * the liked event list
 */
void
mxdc32_private::store_event(void)
{
    // just a sanity check (can never be true :-)
    if (prepared==0) {
        cout<<"store_event: no 'prepared' event pending"<<endl;
        exit(1);
    }
//cout<<"private::store seq "<<prepared->seq<<endl;
    if (last) {
        last->next=prepared;
        last=prepared;
    } else {
        last=prepared;
        first=prepared;
    }
    prepared=0;
}
//---------------------------------------------------------------------------//
/*
 * store_event releases the first (oldest) event from the event queue
 */
struct mxdc32_event*
mxdc32_private::drop_event(void)
{
    struct mxdc32_event *help;

    /* idiotic, but save */
    if (!first)
        return 0;

    if (last==first)
        last=0;
    help=first;
    first=first->next;

    return help;
}

//---------------------------------------------------------------------------//
koala_event::koala_event(void) :
  // next(0),
  mesypattern(0)
{
  /* events[] will be filled in collect_koala_event */
}
//---------------------------------------------------------------------------//
koala_event::~koala_event(void)
{
  mxdc32_depot* depot=mxdc32_depot::Instance();
  for (int mod=0; mod<nr_mesymodules; mod++) {
    if (events[mod])
      depot->put(events[mod]);
  }
}
//---------------------------------------------------------------------------//
mxdc32_depot* mxdc32_depot::_fInstance=0;
Mxdc32DepotDestroyer mxdc32_depot::_fDestroyer;

Mxdc32DepotDestroyer::~Mxdc32DepotDestroyer()
{
  if(fMxdc32Depot)
    delete fMxdc32Depot;
}

mxdc32_depot* mxdc32_depot::Instance()
{
  if(!_fInstance){
    _fInstance = new mxdc32_depot();
    _fDestroyer.SetMxdc32Depot(_fInstance);
  }
  return _fInstance;
}

//---------------------------------------------------------------------------//
mxdc32_depot::~mxdc32_depot()
{
  mxdc32_event* current=first;
  while(current){
    current=first->next;
    delete first;
  }
}
//---------------------------------------------------------------------------//
/*
 * mxdc32_depot stores empty mxdc32_event structures for recycling.
 * mxdc32_depot::get hands out such a structure or allocates a new one
 * the new structure is not cleared or initialised, this is the task
 * of mxdc32_private::prepare_event
 */
struct mxdc32_event*
mxdc32_depot::get(void)
{
    struct mxdc32_event *event;

    if (first) {
        event=first;
        first=event->next;
    } else {
        event=new struct mxdc32_event;
    }
    if (event==0) {
        cout<<"mxdc32_depot:get: cannot allocate new event structure"<<endl;
    }
    return event;
}
//---------------------------------------------------------------------------//
/*
 * mxdc32_depot stores empty mxdc32_event structures for recycling.
 * put stores unused mesytec events inside mxdc32_depot for later use
 */
void
mxdc32_depot::put(struct mxdc32_event *event)
{
    if (!event) {
        cout<<"depot::put: event=0"<<endl;
        return;
    }
    //cout<<"depot::put: store "<<event->seq<<endl;
    event->next=first;
    first=event;
}
