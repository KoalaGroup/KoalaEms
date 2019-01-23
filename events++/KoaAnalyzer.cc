#include "KoaAnalyzer.hxx"
#include "KoaLoguru.hxx"
#include "TStyle.h"

namespace DecodeUtil
{
  int
  KoaAnalyzer::Analyze()
  {
    CHECK_NOTNULL_F(fKoalaPrivate,"Set Koala Event Private List first!");
    CHECK_NOTNULL_F(fEmsPrivate,"Set Ems Event Private List first!");
    //
    RecycleEmsEvents();
    //
    RecycleKoalaEvents();

    return 0;
  }

  //
  void
  KoaAnalyzer::RecycleEmsEvents()
  {
    ems_event* ems_cur=nullptr;
    while(ems_cur=fEmsPrivate->drop_event()){
      ems_cur->recycle();
    }
  }
  
  // //
  void
  KoaAnalyzer::RecycleKoalaEvents()
  {
    koala_event* koala_cur=nullptr;
    while(koala_cur=fKoalaPrivate->drop_event()){
      koala_cur->recycle();
    }
  }

  //
  KoaSimpleAnalyzer::KoaSimpleAnalyzer(const char* outputfile="koala_raw.root", bool use_simplestructure=true,  int max_diff=0)
  {
    SetOutputFile(outputfile);
    SetSimpleStructure(use_simplestructure);
    SetMaxDiff(max_diff);
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
    strcat(pdffile,".pdf");
    
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
  void
  KoaSimpleAnalyzer::Init()
  {
    rfile=new TFile(rootfile,"recreate");
    //
    book_hist();

    koala_raw->Setup(rfile);
    koala_raw->Init();
  }

  //
  void
  KoaSimpleAnalyzer::book_hist()
  {
    gStyle->SetOptStat(111111);
    gStyle->SetPaperSize(TStyle::kA4);
    for(int mod=0;mod<nr_mesymodules;mod++){
      h_timediff[mod] = new TH1F(Form("h_timediff_%d",mod),Form("Timestamp diff of ADC%d (offset=%d)",mod+1,mod*100),600,-50,550);
      h_timediff[mod]->SetLineColor(kBlack+mod);
    }
    //
    for(int i=0;i<nr_mesymodules;i++){
      hwords[i]=new TH1F(Form("hwords_%s",mesymodules[i].label),Form("hwords_%s",mesymodules[i].label),40,-2.5,37.5);
    }
    //
    
  }

  //
  int KoaSimpleAnalyzer::Analyze()
  {
    
  }
}
