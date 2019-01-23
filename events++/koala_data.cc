#include "koala_data.hxx"
#include "mxdc32_data.hxx"
#include "KoaLoguru.hxx"
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cerrno>

using namespace std;

//---------------------------------------------------------------------------//
koala_event::koala_event(void) :
  next(nullptr),
  mesypattern(0)
{
  /* modules[] will be filled in collect_koala_event */
}

//---------------------------------------------------------------------------//
void
koala_event::invalidate()
{
  memset(this,0,sizeof(*this));
}

//---------------------------------------------------------------------------//
void koala_event::recycle()
{
  for (int mod=0; mod<nr_mesymodules; mod++) {
    if (modules[mod])
      modules[mod]->recycle();
  }
  
  koala_depot* fDepot=koala_depot::Instance();
  fDepot->put(this);
}

//---------------------------------------------------------------------------//
koala_depot* koala_depot::fInstance=nullptr;
KoalaDepotDestroyer koala_depot::fDestroyer;

KoalaDepotDestroyer::~KoalaDepotDestroyer()
{
  if(fKoalaDepot)
    delete fKoalaDepot;
}

koala_depot* koala_depot::Instance()
{
  if(!fInstance){
    fInstance = new koala_depot();
    fDestroyer.SetKoalaDepot(fInstance);
  }
  return fInstance;
}

//---------------------------------------------------------------------------//
koala_depot::~koala_depot()
{
  koala_event* current=first;
  while(current){
    current=first->next;
    delete first;
    first=current;
  }
}
//---------------------------------------------------------------------------//
koala_event*
koala_depot::get(void)
{
    koala_event *event;

    if (first) {
        event=first;
        first=event->next;
    } else {
        event=new koala_event;
    }
    if (event==0) {
        cout<<"koala_depot::get: cannot allocate new event structure"<<endl;
    }
    return event;
}
//---------------------------------------------------------------------------//
void
koala_depot::put(koala_event *event)
{
    if (!event) {
        cout<<"koala_depot::put: event=0"<<endl;
        return;
    }
    event->next=first;
    first=event;
}

//---------------------------------------------------------------------------//
koala_private::~koala_private()
{
  if(prepared) prepared->recycle();
  koala_event* current=first;
  while(current){
    current = first->next;
    first->recycle();
    first=current;
  }
}
//---------------------------------------------------------------------------//
koala_event*
koala_private::prepare_event()
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
koala_event*
koala_private::get_prepared()
{
  LOG_SCOPE_FUNCTION(INFO);
  CHECK_NOTNULL_F(prepared,"prepare_event first");

  return prepared;
}

//---------------------------------------------------------------------------//
void
koala_private::store_event(void)
{
    // just a sanity check (can never be true :-)
    if (prepared==0) {
        cout<<"koala_private::store_event: no 'prepared' event pending"<<endl;
        exit(1);
    }

    if (last) {
        last->next=prepared;
        last=prepared;
    } else {
        last=prepared;
        first=prepared;
    }

    statist.events++;
    prepared=0;
}

//---------------------------------------------------------------------------//
koala_event*
koala_private::drop_event(void)
{
    koala_event *help;

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
koala_private::is_empty()
{
  if(!first)
    return true;

  return false;
}
