#include "KoaRawSimple.hxx"

namespace DecodeUtil{
  KoaRawSimple::~KoaRawSimple()
  {
    if(fTreeKoala){
      for(int mod=0;mod<nr_mesymodules;mod++){
        if(fTreeKoala[mod]) delete fTreeKoala[mod];
      }
      delete [] fTreeKoala;
    }
    if(fTreeEms) delete fTreeEms;
  }

  void KoaRawSimple::InitImp()
  {
    fTreeKoala = new TTree*[nr_mesymodules];

    for(int mod=0;mod<nr_mesymodules;mod++){
      fTreeKoala[mod]=new TTree(mesymodules[mod].label,mesymodules[mod].label);

      fTreeKoala[mod]->Branch("ModuleId",fModuleId+mod,"ModuleId/b");
      fTreeKoala[mod]->Branch("NrWords",fNrWords+mod,"NrWords/S");
      fTreeKoala[mod]->Branch("Timestamp",fTimestamp+mod,"Timestamp/L");
      fTreeKoala[mod]->Branch("Data",fData+mod,"Data[34]/I");
      if(mesymodules[mod].mesytype != mesytec_mqdc32){
        fTreeKoala[mod]->Branch("Resolution",fResolution+mod,"Resolution/B");
      }
    }
    //
    fTreeEms = new TTree("Ems","EMS Event Data");
    fTreeEms->Branch("Scaler",fScaler,"Scaler[32]/i");
    fTreeEms->Branch("EmsTimeSecond",&fEmsTimeSecond,"EmsTimeSecond/L");
    fTreeEms->Branch("EmsTimeUSecond",&fEmsTimeUSecond,"EmsTimeUSecond/L");
  }

  void KoaRawSimple::FillKoalaImp()
  {
    for(int mod=0;mod<nr_mesymodules;mod++){
      fTreeKoala[mod]->Fill();
    }
  }

  void KoaRawSimple::FillEmsImp()
  {
    fTreeEms->Fill();
  }

  void KoaRawSimple::DoneImp()
  {
    for(int mod=0;mod<nr_mesymodules;mod++){
      fTreeKoala[mod]->Write();
    }
    fTreeEms->Write();
  }
}
