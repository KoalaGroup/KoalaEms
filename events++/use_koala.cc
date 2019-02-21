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
#include <cstdlib>
#include <limits>       // std::numeric_limits
#include <unistd.h>
#include "global.hxx"
#include "parse_koala.hxx"
#include "KoaRawData.hxx"
#include "TFile.h"
#include "TH1F.h"
#include "TTree.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TString.h"
#include "THStack.h"

using namespace std;

// statistics
struct koala_statist {
    uint32_t koala_events;
    uint32_t complete_events;
    uint32_t without_qdc;
    uint32_t only_qdc;
    uint32_t good_adc;
};

struct timestamp_statist{
  uint32_t equal_ev[nr_mesymodules];
  uint32_t plusone_ev[nr_mesymodules];
  uint32_t minusone_ev[nr_mesymodules];
  uint32_t plustwo_ev[nr_mesymodules];
  uint32_t minustwo_ev[nr_mesymodules];
  uint32_t unsync_ev[nr_mesymodules];
};

static struct koala_statist koala_statist;
static struct timestamp_statist timestamp_statist;
static char outputbase[1024];
static char rootfile[1024];
static char pdffile[1024];
static int  max_tsdiff;

static TFile* rfile;
static TH1F* h_timediff[nr_mesymodules];
static TH1F* hwords[nr_mesymodules];
static KoaRaw *koala_raw;

//---------------------------------------------------------------------------//
void book_hist()
{
  gStyle->SetOptStat(111111);
  gStyle->SetPaperSize(TStyle::kA4);
  for(int mod=0;mod<nr_mesymodules;mod++){
    h_timediff[mod] = new TH1F(Form("h_timediff_%d",mod),Form("Timestamp diff of ADC%d (offset=%d)",mod+1,mod*1000),8150,-100,8050);
    h_timediff[mod]->SetLineColor(kBlack+mod);
    h_timediff[mod]->SetLineWidth(3);
    if(mod== 2)
      h_timediff[mod]->SetLineColor(kBlue);
    if(mod== 3)
      h_timediff[mod]->SetLineColor(kGreen);
    if(mod== 4)
      h_timediff[mod]->SetLineColor(kRed+1);
    if(mod== 5)
      h_timediff[mod]->SetLineColor(kYellow);
  }
  //
  for(int i=0;i<nr_mesymodules;i++){
    hwords[i]=new TH1F(Form("hwords_%s",mesymodules[i].label),Form("hwords_%s",mesymodules[i].label),40,-2.5,37.5);
  }
  //
  return;
}

void fill_hist(koala_event* koala){
  mxdc32_event *event;
  for(int i=0;i<nr_mesymodules;i++){
    event=koala->events[i];
    hwords[i]->Fill(event->len);
  }
}

void delete_hist()
{
  for(int mod=0;mod<nr_mesymodules;mod++){
    if(!h_timediff[mod]) delete h_timediff[mod];
  }
  //
  for(int mod=0;mod<nr_mesymodules;mod++){
    if(!hwords[mod]) delete hwords[mod];
  }
  return;
}

void save_hist()
{
  rfile->cd();
  for(int mod=0;mod<nr_mesymodules;mod++){
    h_timediff[mod]->Write();
  }
  //
  for(int mod=0;mod<nr_mesymodules;mod++){
    hwords[mod]->Write();
  }
  return;
}

void draw_hist()
{
  TCanvas* can=new TCanvas("can","Timestamp Diff");
  gPad->SetLogy();
  THStack* hstack=new THStack("htimediff","Timestampe Diff");
  for(int mod=0;mod<nr_mesymodules;mod++){
    //TODO this line crash when use_koala_event is not invoked anytime
    hstack->Add(h_timediff[mod]);
  }

  hstack->Draw();
  can->Print(pdffile);
  delete can;
  delete hstack;
  //
  for(int mod=0;mod<nr_mesymodules;mod++){
    can = new TCanvas(Form("can_%s",mesymodules[mod].label),Form("can_%s",mesymodules[mod].label));
    gPad->SetLogy();
    hwords[mod]->Draw();
    can->Print(Form("hwords_%s.pdf",mesymodules[mod].label));
    delete can;
  }
  return;
}

//---------------------------------------------------------------------------//
// check the timestamp and fill h_timediff
void check_timestamp(koala_event* koala)
{
  // timestamp check
  mxdc32_event *event;
  int64_t tmin=koala->events[nr_mesymodules-1]->timestamp;
  if(mesymodules[nr_mesymodules-1].mesytype == mesytec_mqdc32){
    tmin -= max_tsdiff;
  }
  int64_t trange=0x40000000;
  int64_t delta_t;
  static bool unsync=false;
  static bool print=false;
  for (int mod=0; mod<nr_mesymodules; mod++) {
    event=koala->events[mod];
    delta_t=event->timestamp-tmin;
    if(delta_t>trange/2) delta_t-=trange;
    else if(delta_t<-trange/2) delta_t+=trange;

    if(mesymodules[mod].mesytype == mesytec_mqdc32){
      delta_t -= max_tsdiff;
    }
    //fill hist
    h_timediff[mod]->Fill(delta_t+mod*1000);

    if(abs(delta_t) > 3){
      unsync=true;
      print=true;
      //
      timestamp_statist.unsync_ev[mod]++;
    }
    else if(delta_t==1) timestamp_statist.plusone_ev[mod]++;
    else if(delta_t==-1) timestamp_statist.minusone_ev[mod]++;
    else if(delta_t==-2) timestamp_statist.minustwo_ev[mod]++;
    else if(delta_t==2) timestamp_statist.plustwo_ev[mod]++;
    else timestamp_statist.equal_ev[mod]++;
  }

  // printing
  if(unsync){
    printf("unsync event (%d): ",koala_statist.complete_events);
    for (int mod=0; mod<nr_mesymodules; mod++) {
      event=koala->events[mod];
      printf("%d\t",event->timestamp);
    }
    printf("\n");
    unsync=false;
  } else if(print){
    printf("after unsync event (%d): ",koala_statist.complete_events);
    for (int mod=0; mod<nr_mesymodules; mod++) {
      event=koala->events[mod];
      printf("%d\t",event->timestamp);
    }
    printf("\n");
    print=false;
  }
}
//---------------------------------------------------------------------------//
// use_koala_setup will extract the filename base
void
use_koala_setup(const char* outputfile, bool use_simplestructure,  int max_diff)
{
  bzero(&koala_statist, sizeof(struct koala_statist));
  bzero(&timestamp_statist,sizeof(struct timestamp_statist));
  //
  const char* p;
  if((p = strrchr(outputfile,'.')) != NULL){
    strncpy(outputbase,outputfile,p-outputfile);
    outputbase[p-outputfile]='\0';
  }
  else{
    strcpy(outputbase,outputfile);
  }

  strcpy(rootfile,outputbase);
  strcpy(pdffile,outputbase);
  strcat(rootfile,".root");
  strcat(pdffile,".pdf");

  //
  if(use_simplestructure){
    koala_raw = new KoaRawSimple();
  }
  else{
    //TODO
    //koala_raw = new KoaRawComplex();
  }

  //
  max_tsdiff=max_diff;
}
//---------------------------------------------------------------------------//
void
use_koala_init(void)
{
    rfile=new TFile(rootfile,"recreate");
    //
    book_hist();

    koala_raw->Setup(rfile);
    koala_raw->Init();
}

//---------------------------------------------------------------------------//
void
use_koala_done(void)
{
    draw_hist();
    save_hist();
    // delete_hist();

    koala_raw->Done();

    delete koala_raw;
    // rfile must be deleted after koala_raw
    delete rfile;

    //
    cout<<endl;
    cout<<"**********************************************"<<endl;
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

    cout<<"**********************************************"<<endl;
    cout<<"    sync_events   : ";
    for(int mod=0;mod<nr_mesymodules;mod++){
      cout<<setw(8)<<timestamp_statist.equal_ev[mod]<<"("
          <<static_cast<double>(timestamp_statist.equal_ev[mod])/
        static_cast<double>(koala_statist.koala_events)
          <<")\t";
    }
    cout<<endl;
    cout<<"    plusone_events   : ";
    for(int mod=0;mod<nr_mesymodules;mod++){
      cout<<setw(8)<<timestamp_statist.plusone_ev[mod]<<"("
          <<static_cast<double>(timestamp_statist.plusone_ev[mod])/
        static_cast<double>(koala_statist.koala_events)
          <<")\t";
    }
    cout<<endl;
    cout<<"    minusone_events   : ";
    for(int mod=0;mod<nr_mesymodules;mod++){
      cout<<setw(8)<<timestamp_statist.minusone_ev[mod]<<"("
          <<static_cast<double>(timestamp_statist.minusone_ev[mod])/
        static_cast<double>(koala_statist.koala_events)
          <<")\t";
    }
    cout<<endl;
    cout<<"    plustwo_events   : ";
    for(int mod=0;mod<nr_mesymodules;mod++){
      cout<<setw(8)<<timestamp_statist.plustwo_ev[mod]<<"("
          <<static_cast<double>(timestamp_statist.plustwo_ev[mod])/
        static_cast<double>(koala_statist.koala_events)
          <<")\t";
    }
    cout<<endl;
    cout<<"    minustwo_events   : ";
    for(int mod=0;mod<nr_mesymodules;mod++){
      cout<<setw(8)<<timestamp_statist.minustwo_ev[mod]<<"("
          <<static_cast<double>(timestamp_statist.minustwo_ev[mod])/
        static_cast<double>(koala_statist.koala_events)
          <<")\t";
    }
    cout<<endl;
    cout<<"    unsync_events   : ";
    for(int mod=0;mod<nr_mesymodules;mod++){
      cout<<setw(8)<<timestamp_statist.unsync_ev[mod]<<"("
          <<static_cast<double>(timestamp_statist.unsync_ev[mod])/
        static_cast<double>(koala_statist.koala_events)
          <<")\t";
    }
    cout<<endl;
    cout<<"**********************************************"<<endl;
}
//---------------------------------------------------------------------------//
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
}
//---------------------------------------------------------------------------//
int use_koala_event(koala_event *koala)
{
  koala_statist.koala_events++;

  // check timestamp
  check_timestamp(koala);

  // fill hist
  fill_hist(koala);

  // data extraction and fill the tree
  koala_raw->Fill(koala);

  // delete koala_event
  delete koala;
  return 0;
}
//---------------------------------------------------------------------------//
#if 0
int
use_koala_event(koala_event *koala)
{
    koala_statist.koala_events++;
    analyse_koala(koala);

    delete koala;
    return 0;
}
#endif
//---------------------------------------------------------------------------//
