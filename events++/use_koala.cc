/*
 * ems/events++/use_koala.cc
 * 
 * created 2015-Nov-25 PeWue
 * $ZEL: use_koala.cc,v 1.5 2016/02/04 12:40:08 wuestner Exp $
 */

/*
 * This program is supposed to be incomplete.
 * The procedure use_koala_event is called by parse_koala for each collected
 * koala event. use_koala_event has to delete the koala_event after use.
 * This can be done before leaving use_koala_event, or one (or more)
 * koala events can temporarily stored and deleted in subsequent execution
 * of use_koala_event.
 * use_koala_init is called before the first call of use_koala_event. It can
 * be used to initialise static data.
 * use_koala_doneis called after the last call of use_koala_event. It can be
 * used to print statistics, close files or other usefull things.
 */

#define _DEFAULT_SOURCE

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cerrno>
#include <cstdint>
#include <cstring>
//#include <cmath>
#include <limits>       // std::numeric_limits
#include <unistd.h>
#include "parse_koala.hxx"
#include "TH1F.h"
#include "TCanvas.h"
#include "TFile.h"

using namespace std;

// statistics
struct koala_statist {
    uint32_t koala_events;
    uint32_t complete_events;
    uint32_t without_qdc;
    uint32_t only_qdc;
    uint32_t good_adc;
};
static struct koala_statist koala_statist;

//---------------------------------------------------------------------------//
void
use_koala_init(void)
{
    bzero(&koala_statist, sizeof(struct koala_statist));
}
//---------------------------------------------------------------------------//
void
use_koala_done(void)
{
    cout<<"    koala_events   : "<<setw(8)<<koala_statist.koala_events<<endl;
    cout<<"    good ADC       : "<<setw(8)<<koala_statist.good_adc<<endl;
//    cout<<"    complete_events: "<<setw(8)<<koala_statist.complete_events<<endl;
//    cout<<"    without_qdc    : "<<setw(8)<<koala_statist.without_qdc<<endl;
//    cout<<"    only_dqc       : "<<setw(8)<<koala_statist.only_qdc<<endl;
    cout<<"    good/all       : "
            <<static_cast<double>(koala_statist.complete_events)/
                    static_cast<double>(koala_statist.koala_events)
            <<endl;
    cout<<"     adc/all       : "
            <<static_cast<double>(koala_statist.good_adc)/
                    static_cast<double>(koala_statist.koala_events)
            <<endl;
}
//---------------------------------------------------------------------------//
__attribute__((unused))
static void
print_times(koala_event *a, koala_event *b)
{
    printf("%02x <===> %02x\n", a->mesypattern, b->mesypattern);
    for (int i=0; i<nr_mesymodules; i++) {
        printf("[%d]", i);
        if (a->events[i])
            printf(" %10ld", a->events[i]->timestamp);
        else
            printf("           ");
        if (b->events[i])
            printf(" %10ld\n", b->events[i]->timestamp);
        else
            printf("           \n");
    }
}
//---------------------------------------------------------------------------//
#if 1
__attribute__((unused))
static void
analyse_koala(koala_event *koala)
{
    mxdc32_event *event;
    int mod, i;

    for (mod=0; mod<nr_mesymodules; mod++) {
        event=koala->events[mod];
        if (event) {
            for (i=0; i<32; i++) {
                printf("%c", event->data[i]?'*':'-');
            }
            printf("\n");
        } else {
            printf("00000000000000000000000000000000\n");
        }
    }
    printf("\n");
}
#else
__attribute__((unused))
static void
analyse_koala(koala_event *koala)
{
    mxdc32_event *event;
    int mod, i;

    //printf("pattern=%02x\n", koala->mesypattern);
    for (mod=0; mod<nr_mesymodules; mod++) {
        event=koala->events[mod];
        if (event) {
            for (i=0; i<32; i++) {
                printf("%c", event->data[i]?'*':'-');
            }
            printf("\n");
        } else {
            printf("@\n");
        }
    }
    printf("\n");

#if 0
    mxdc32_event *qevent=koala->events[6];
    mxdc32_event *tevent=koala->events[7];
    for (i=0; i<32; i++) {
        printf("%08x %08x %5d %4d\n",
                tevent->data[i], qevent->data[i],
                tevent->data[i]&0xffff, qevent->data[i]&0xfff);
    }
#endif
#if 0
    for (i=0; i<32; i++) {
        printf("%4d %4d %4d %4d %4d %4d %4d %5d\n",
                koala->events[0]->data[i]&0xfff,
                koala->events[1]->data[i]&0xfff,
                koala->events[2]->data[i]&0xfff,
                koala->events[3]->data[i]&0xfff,
                koala->events[4]->data[i]&0xfff,
                koala->events[5]->data[i]&0xfff,
                koala->events[6]->data[i]&0xfff,
                koala->events[7]->data[i]&0xffff);
    }
#endif
#if 0
    printf("%f %f %f %f %f %f %f %f\n",
            koala->event_data.bct[0],
            koala->event_data.bct[1],
            koala->event_data.bct[2],
            koala->event_data.bct[3],
            koala->event_data.bct[4],
            koala->event_data.bct[5],
            koala->event_data.bct[6],
            koala->event_data.bct[7]);
#endif
}
#endif
//---------------------------------------------------------------------------//
int use_koala_event(koala_event *koala, TH1F** h)
{
  koala_statist.koala_events++;
  if (koala->mesypattern==0x3f) {
    koala_statist.complete_events++;
  }
  if (koala->mesypattern==0xbf) {
    koala_statist.without_qdc++;
  }

  if (koala->mesypattern==0x40) {
    koala_statist.only_qdc++;
  }

  if (koala->mesypattern&0x3f) {
    koala_statist.good_adc++;
  }

  // fill the hist
  mxdc32_event *event;
  int64_t tmin=koala->events[nr_mesymodules-1]->timestamp;
  int64_t trange=0x40000000;
  int64_t t;
  int64_t delta_t;
  static bool overflow=false;
  static bool print=false;
  for (int mod=0; mod<nr_mesymodules; mod++) {
    event=koala->events[mod];
    t=event->timestamp;
    if((t-tmin)>trange/2) delta_t=t-tmin-trange;
    else if((t-tmin)<-trange/2) delta_t=t-tmin+trange;
    else delta_t=t-tmin;
    h[mod]->Fill(delta_t+mod*100);
    // if((t-tmin)>1000){
    //   overflow=true;
    //   print=true;
    // }
  }

  if(overflow){
    printf("overflow (%d): ",koala_statist.complete_events);
    for (int mod=0; mod<nr_mesymodules; mod++) {
      event=koala->events[mod];
      t=event->timestamp;
      printf("%d\t", t);
    }
    printf("\n");
    overflow=false;
  } else if(print){
    printf("after overflow (%d): ",koala_statist.complete_events);
    for (int mod=0; mod<nr_mesymodules; mod++) {
      event=koala->events[mod];
      t=event->timestamp;
      printf("%d\t", t);
    }
    printf("\n");
    print=false;
  }
  delete koala;
  return 0;
}
//---------------------------------------------------------------------------//
#if 0
koala_event *last_koala=0;

int
use_koala_event(koala_event *koala)
{
    koala_statist.koala_events++;
    if (koala->mesypattern==0xff)
        koala_statist.complete_events++;
    if (koala->mesypattern==0xbf)
        koala_statist.without_qdc++;
    if (koala->mesypattern==0x40)
        koala_statist.only_dqc++;

    if (last_koala) {
        if (((koala->mesypattern|last_koala->mesypattern)==0xff) &&
            ((koala->mesypattern&last_koala->mesypattern)==0)) {
                print_times(last_koala, koala);
        }
        delete last_koala;
    }
    last_koala=koala;

    return 0;
}
#else
int
use_koala_event(koala_event *koala)
{
    koala_statist.koala_events++;
    if (koala->mesypattern==0x3f) {
        koala_statist.complete_events++;
    }
    if (koala->mesypattern==0xbf) {
        koala_statist.without_qdc++;
    }

    if (koala->mesypattern==0x40) {
        koala_statist.only_qdc++;
    }

    if (koala->mesypattern&0x3f) {
        koala_statist.good_adc++;
    }

    analyse_koala(koala);

    delete koala;
    return 0;
}
#endif
//---------------------------------------------------------------------------//
