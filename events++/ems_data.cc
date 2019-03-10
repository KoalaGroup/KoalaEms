#include "ems_data.hxx"
#include "KoaLoguru.hxx"
#include <iostream>
#include <cerrno>
#include <cstring>
#include <cstdlib>

using namespace std;

//---------------------------------------------------------------------------//
void
ems_event::invalidate()
{
  // WARNING: bzero is a deprecated function, use memset instead
  // bzero(this, sizeof(*this));
  memset(this,0,sizeof(*this));
}

//---------------------------------------------------------------------------//
void ems_event::recycle()
{
  ems_depot* fDepot=ems_depot::Instance();
  fDepot->put(this);
}

//---------------------------------------------------------------------------//
ems_depot* ems_depot::fInstance=nullptr;
EmsDepotDestroyer ems_depot::fDestroyer;

EmsDepotDestroyer::~EmsDepotDestroyer()
{
  if(fEmsDepot)
    delete fEmsDepot;
}

ems_depot* ems_depot::Instance()
{
  if(!fInstance){
    fInstance = new ems_depot();
    fDestroyer.SetEmsDepot(fInstance);
  }
  return fInstance;
}

//---------------------------------------------------------------------------//
ems_depot::~ems_depot()
{
  LOG_SCOPE_FUNCTION(1);
  ems_event* current=first;
  int i=0;
  while(current){
    i++;
    current=first->next;
    CHECK_NOTNULL_F(first);
    LOG_SCOPE_F(1,"ite: %d (first=%x)",i,first);
    delete first;
    first=current;
  }
}
//---------------------------------------------------------------------------//
ems_event*
ems_depot::get(void)
{
    ems_event *event;

    if (first) {
        event=first;
        first=event->next;
    } else {
        event=new ems_event;
        LOG_F(1,"new=%x",event);
    }
    if (event==0) {
        cout<<"ems_depot:get: cannot allocate new event structure"<<endl;
    }
    return event;
}
//---------------------------------------------------------------------------//
void
ems_depot::put(ems_event *event)
{
    if (!event) {
        cout<<"ems_depot::put: event=0"<<endl;
        return;
    }
    event->next=first;
    first=event;
}

//---------------------------------------------------------------------------//
ems_private::~ems_private()
{
  if(prepared) prepared->recycle();
  ems_event* current=first;
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
ems_event*
ems_private::prepare_event()
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
ems_event*
ems_private::get_prepared()
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
ems_private::store_event(void)
{
    // just a sanity check (can never be true :-)
    if (prepared==0) {
        cout<<"ems_private::store_event: no 'prepared' event pending"<<endl;
        exit(1);
    }
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
ems_event*
ems_private::drop_event(void)
{
    ems_event *help;

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
ems_private::is_empty()
{
  if(!first)
    return true;

  return false;
}

//---------------------------------------------------------------------------//
ems_event*
ems_private::head()
{
  return first;
}
