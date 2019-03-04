#include "KoaAnalyzer.hxx"
#include "KoaLoguru.hxx"
#include "TStyle.h"
#include "TCanvas.h"
#include "TH1.h"
#include "THStack.h"
#include <iostream>

using namespace std;

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
}
