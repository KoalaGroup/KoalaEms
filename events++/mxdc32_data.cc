#include "mxdc32_data.hxx"
#include "KoaLoguru.hxx"
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
  // WARNING: bzero is a deprecated function, use memset instead
  // bzero(this, sizeof(*this));
  memset(this,0,sizeof(*this));
}

//---------------------------------------------------------------------------//
void mxdc32_event::recycle()
{
  mxdc32_depot* fDepot=mxdc32_depot::Instance();
  fDepot->put(this);
}

//---------------------------------------------------------------------------//
mxdc32_private::~mxdc32_private()
{
  if(prepared) prepared->recycle();
  mxdc32_event* current=first;
  while(current){
    current = first->next;
    first->recycle();
    first=current;
  }
}
//---------------------------------------------------------------------------//
/*
 * prepare_event prepares a new, empty event structure
 */
mxdc32_event*
mxdc32_private::prepare_event()
{
    // if there is an unused event (previous data where invalid), return this
    // otherwise fetch one from the depot
    if (prepared==0)
        prepared=fDepot->get();

    CHECK_NOTNULL_F(prepared,"Can't allocate memory for the next event");
    prepared->invalidate();
    return prepared;
}

//---------------------------------------------------------------------------//
mxdc32_event*
mxdc32_private::get_prepared()
{
  LOG_SCOPE_FUNCTION(INFO);
  CHECK_NOTNULL_F(prepared,"prepare_event first");

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
mxdc32_event*
mxdc32_private::drop_event(void)
{
    mxdc32_event *help;

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
bool
mxdc32_private::is_empty()
{
  if(!first)
    return true;

  return false;
}

//---------------------------------------------------------------------------//
mxdc32_event* mxdc32_private::head()
{
  return first;
}

//---------------------------------------------------------------------------//
mxdc32_depot* mxdc32_depot::fInstance=nullptr;
Mxdc32DepotDestroyer mxdc32_depot::fDestroyer;

Mxdc32DepotDestroyer::~Mxdc32DepotDestroyer()
{
  if(fMxdc32Depot)
    delete fMxdc32Depot;
}

mxdc32_depot* mxdc32_depot::Instance()
{
  if(!fInstance){
    fInstance = new mxdc32_depot();
    fDestroyer.SetMxdc32Depot(fInstance);
  }
  return fInstance;
}

//---------------------------------------------------------------------------//
mxdc32_depot::~mxdc32_depot()
{
  mxdc32_event* current=first;
  while(current){
    current=first->next;
    delete first;
    first=current;
  }
}
//---------------------------------------------------------------------------//
/*
 * mxdc32_depot stores empty mxdc32_event structures for recycling.
 * mxdc32_depot::get hands out such a structure or allocates a new one
 * the new structure is not cleared or initialised, this is the task
 * of mxdc32_private::prepare_event
 */
mxdc32_event*
mxdc32_depot::get(void)
{
    mxdc32_event *event;

    if (first) {
        event=first;
        first=event->next;
    } else {
        event=new mxdc32_event;
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
mxdc32_depot::put(mxdc32_event *event)
{
    if (!event) {
        cout<<"mxdc32_depot::put: event=0"<<endl;
        return;
    }
    //cout<<"depot::put: store "<<event->seq<<endl;
    event->next=first;
    first=event;
}
