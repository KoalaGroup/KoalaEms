#include "KoaSimpleAnalyzer.hxx"
#include "TStyle.h"
#include "TCanvas.h"
#include <iostream>
#include "THStack.h"

using namespace std;

namespace DecodeUtil{
  //
  KoaSimpleAnalyzer::KoaSimpleAnalyzer(const char* outputfile, bool use_simplestructure,  int max_diff) : rfile(nullptr), koala_raw(nullptr)
  {
    SetOutputFile(outputfile);
    SetSimpleStructure(use_simplestructure);
    SetMaxDiff(max_diff);
  }

  //
  KoaSimpleAnalyzer::~KoaSimpleAnalyzer()
  {
    if(koala_raw)
      delete koala_raw;
    if(rfile)
      delete rfile;

    //
    // delete_hist();
  }

  //
  void
  KoaSimpleAnalyzer::SetOutputFile(const char* outputfile)
  {
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
    strcat(pdffile,"_check.pdf");
    
  }

  //
  void
  KoaSimpleAnalyzer::SetSimpleStructure(bool flag)
  {
    if(flag)
      koala_raw=new KoaRawSimple();
    else{
      //[TODO]
    }
  }

  //
  void
  KoaSimpleAnalyzer::SetMaxDiff(int max_diff)
  {
    max_tsdiff=max_diff;
  }

  //
  int
  KoaSimpleAnalyzer::Init()
  {
    rfile=new TFile(rootfile,"recreate");
    //
    book_hist();

    koala_raw->Setup(rfile);
    koala_raw->Init();
  }

  //
  int
  KoaSimpleAnalyzer::Done()
  {
    draw_hist();
    save_hist();
    //
    koala_raw->Done();
  }

  //
  void
  KoaSimpleAnalyzer::book_hist()
  {
    gStyle->SetOptStat(111111);
    gStyle->SetPaperSize(TStyle::kA4);
    for(int mod=0;mod<nr_mesymodules;mod++){
      h_timediff[mod] = new TH1F(Form("h_timediff_%d",mod),Form("Timestamp diff of ADC%d (offset=%d)",mod+1,mod*100),nr_mesymodules*100,-50,nr_mesymodules*100-50);
      h_timediff[mod]->SetLineColor(kBlack+mod);
    }
    //
    for(int i=0;i<nr_mesymodules;i++){
      hwords[i]=new TH1F(Form("hwords_%s",mesymodules[i].label),Form("hwords_%s",mesymodules[i].label),40,-2.5,37.5);
    }
    //
  }

  //
  void
  KoaSimpleAnalyzer::delete_hist()
  {
    //
    for(int mod=0;mod<nr_mesymodules;mod++){
      if(h_timediff[mod]) delete h_timediff[mod];
    }
    //
    for(int mod=0;mod<nr_mesymodules;mod++){
      if(hwords[mod]) delete hwords[mod];
    }
    return;
  }

  //
  void
  KoaSimpleAnalyzer::fill_hist(koala_event* koala)
  {
    mxdc32_event *event;
    for(int i=0;i<nr_mesymodules;i++){
      event=koala->modules[i];
      hwords[i]->Fill(event->len);
    }
  }

  //
  void
  KoaSimpleAnalyzer::draw_hist()
  {
    TCanvas* can=new TCanvas("can","Raw Data Checking");
    gPad->SetLogy();
    can->Print(Form("%s[",pdffile));

    THStack* hstack=new THStack("htimediff","Timestampe Diff");
    for(int mod=0;mod<nr_mesymodules;mod++){
      //TODO this line crash when use_koala_event is not invoked anytime
      hstack->Add(h_timediff[mod]);
    }
    hstack->Draw();
    can->Print(pdffile);
    delete hstack;

    //
    for(int mod=0;mod<nr_mesymodules;mod++){
      hwords[mod]->Draw();
      can->Print(pdffile);
    }

    can->Print(Form("%s]",pdffile));
    delete can;

    return;
  }

  void
  KoaSimpleAnalyzer::save_hist()
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

  void
  KoaSimpleAnalyzer::check_timestamp(koala_event* koala)
  {
    
    // timestamp check
    mxdc32_event *event;
    int64_t tmin=koala->modules[nr_mesymodules-1]->timestamp;
    if(mesymodules[nr_mesymodules-1].mesytype == mesytec_mqdc32){
      tmin -= max_tsdiff;
    }
    int64_t trange=0x40000000;
    int64_t delta_t;
    static bool unsync=false;
    static bool print=false;
    for (int mod=0; mod<nr_mesymodules; mod++) {
      event=koala->modules[mod];
      delta_t=event->timestamp-tmin;
      if(delta_t>trange/2) delta_t-=trange;
      else if(delta_t<-trange/2) delta_t+=trange;

      if(mesymodules[mod].mesytype == mesytec_mqdc32){
        delta_t -= max_tsdiff;
      }
      //fill hist
      h_timediff[mod]->Fill(delta_t+mod*100);

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
      printf("unsync event (%l): ",fKoalaPrivate->get_statist_events());
      for (int mod=0; mod<nr_mesymodules; mod++) {
        event=koala->modules[mod];
        printf("%l\t",event->timestamp);
      }
      printf("\n");
      unsync=false;
    } else if(print){
      printf("after unsync event (%l): ",fKoalaPrivate->get_statist_events());
      for (int mod=0; mod<nr_mesymodules; mod++) {
        event=koala->modules[mod];
        printf("%l\t",event->timestamp);
      }
      printf("\n");
      print=false;
    }
  }

  //
  int KoaSimpleAnalyzer::Analyze()
  {
    koala_event* koala_cur=nullptr;
    while(koala_cur=fKoalaPrivate->drop_event()){
      check_timestamp(koala_cur);
      fill_hist(koala_cur);
      koala_raw->Fill(koala_cur);
      koala_cur->recycle();
    }

    //
    RecycleEmsEvents();
  }

  //
  void KoaSimpleAnalyzer::Print()
  {
    uint64_t total_events=fKoalaPrivate->get_statist_events();

    //
    cout<<endl;
    cout<<"**********************************************"<<endl;
    cout<<"    koala_events   : "<<setw(8)<<total_events<<endl;

    cout<<"**********************************************"<<endl;
    cout<<"    sync_events   : ";
    for(int mod=0;mod<nr_mesymodules;mod++){
      cout<<setw(8)<<timestamp_statist.equal_ev[mod]<<"("
          <<static_cast<double>(timestamp_statist.equal_ev[mod])/
        static_cast<double>(total_events)
          <<")\t";
    }
    cout<<endl;
    cout<<"    plusone_events   : ";
    for(int mod=0;mod<nr_mesymodules;mod++){
      cout<<setw(8)<<timestamp_statist.plusone_ev[mod]<<"("
          <<static_cast<double>(timestamp_statist.plusone_ev[mod])/
        static_cast<double>(total_events)
          <<")\t";
    }
    cout<<endl;
    cout<<"    minusone_events   : ";
    for(int mod=0;mod<nr_mesymodules;mod++){
      cout<<setw(8)<<timestamp_statist.minusone_ev[mod]<<"("
          <<static_cast<double>(timestamp_statist.minusone_ev[mod])/
        static_cast<double>(total_events)
          <<")\t";
    }
    cout<<endl;
    cout<<"    plustwo_events   : ";
    for(int mod=0;mod<nr_mesymodules;mod++){
      cout<<setw(8)<<timestamp_statist.plustwo_ev[mod]<<"("
          <<static_cast<double>(timestamp_statist.plustwo_ev[mod])/
        static_cast<double>(total_events)
          <<")\t";
    }
    cout<<endl;
    cout<<"    minustwo_events   : ";
    for(int mod=0;mod<nr_mesymodules;mod++){
      cout<<setw(8)<<timestamp_statist.minustwo_ev[mod]<<"("
          <<static_cast<double>(timestamp_statist.minustwo_ev[mod])/
        static_cast<double>(total_events)
          <<")\t";
    }
    cout<<endl;
    cout<<"    unsync_events   : ";
    for(int mod=0;mod<nr_mesymodules;mod++){
      cout<<setw(8)<<timestamp_statist.unsync_ev[mod]<<"("
          <<static_cast<double>(timestamp_statist.unsync_ev[mod])/
        static_cast<double>(total_events)
          <<")\t";
    }
    cout<<endl;
    cout<<"**********************************************"<<endl;
    
  }
}
