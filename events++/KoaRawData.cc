#include "KoaRawData.hxx"
#define UNDER_THRESHOLD -5
#define ADC_OVERFLOW 0x2000
#define QDC_OVERFLOW 0x1000

namespace DecodeUtil
{
  KoaRaw::~KoaRaw()
  {
    if(fModuleId)   delete [] fModuleId;
    if(fResolution) delete [] fResolution;
    if(fNrWords)    delete [] fNrWords;
    if(fTimestamp)  delete [] fTimestamp;
    if(fData)       delete [] fData;
  }

  void KoaRaw::Init()
  {
    if(fFile==nullptr){
      printf("Error: setup TFile first (KoaRaw)\n");
      return;
    }

    fFile->cd();

    fModuleId = new UChar_t[nr_mesymodules];
    fResolution = new Char_t[nr_mesymodules];
    fNrWords  =  new Short_t[nr_mesymodules];
    fTimestamp = new Long64_t[nr_mesymodules];
    fData      = new Int_t[nr_mesymodules][34];

    InitImp();
  }

  void KoaRaw::Fill(koala_event* koala)
  {
    Decode(koala);
    FillImp();
  }

  void KoaRaw::Done()
  {
    fFile->cd();
    DoneImp();
  }


  void KoaRaw::Decode(koala_event* koala)
  {
    mxdc32_event *event;
    for(int mod=0;mod<nr_mesymodules;mod++){
      event = koala->modules[mod];
      //
      fTimestamp[mod] = event->timestamp;
      fModuleId[mod]= (event->header>>16)&0xff;
      fNrWords[mod] = event->header&0xfff;
      switch(mesymodules[mod].mesytype)
        {
        case mesytec_madc32:
          {
            fResolution[mod] = (event->header>>12)&0x7;
            for(int ch=0;ch<32;ch++){
              if(event->data[ch]){
                if((event->data[ch]>>14)&0x1){
                  fData[mod][ch] = ADC_OVERFLOW;
                }
                else{
                  switch(fResolution[mod])
                    {
                    case 0:
                      fData[mod][ch] = (event->data[ch])&0x7ff;
                      break;
                    case 1:
                    case 2:
                      fData[mod][ch] = (event->data[ch])&0xfff;
                      break;
                    case 3:
                    case 4:
                      fData[mod][ch] = (event->data[ch])&0x1fff;
                      break;
                    default:
                      printf("Unknown resolution (KoaRawSimple::Decode)\n");
                    }
                }
              }
              else{
                fData[mod][ch] = UNDER_THRESHOLD;
              }
            }
            break;
          }
        case mesytec_mqdc32:
          {
            for(int ch=0;ch<32;ch++){
              if(event->data[ch]){
                if((event->data[ch]>>15)&0x1){
                  fData[mod][ch] = QDC_OVERFLOW;
                }
                else{
                  fData[mod][ch] = (event->data[ch])&0xfff;
                }
              }
              else{
                fData[mod][ch] = UNDER_THRESHOLD;
              }
            }
            break;
          }
        case mesytec_mtdc32:
          {
            fResolution[mod] = (event->header>>12)&0xf;
            for(int ch=0;ch<34;ch++){
              if(event->data[ch]){
                fData[mod][ch] = (event->data[ch])&0xffff;
              }
              else{
                fData[mod][ch] = UNDER_THRESHOLD;
              }
            }
            break;
          }
        default:
          break;
        }
    }
  }

  //---------------------------------------------------------------------------//
  KoaRawSimple::~KoaRawSimple()
  {
    if(fTree){
      for(int mod=0;mod<nr_mesymodules;mod++){
        if(fTree[mod]) delete fTree[mod];
      }
      delete [] fTree;
    }
  }

  void KoaRawSimple::InitImp()
  {
    fTree = new TTree*[nr_mesymodules];

    for(int mod=0;mod<nr_mesymodules;mod++){
      fTree[mod]=new TTree(mesymodules[mod].label,mesymodules[mod].label);

      fTree[mod]->Branch("ModuleId",fModuleId+mod,"ModuleId/b");
      fTree[mod]->Branch("NrWords",fNrWords+mod,"NrWords/S");
      fTree[mod]->Branch("Timestamp",fTimestamp+mod,"Timestamp/L");
      fTree[mod]->Branch("Data",fData+mod,"Data[34]/I");
      if(mesymodules[mod].mesytype != mesytec_mqdc32){
        fTree[mod]->Branch("Resolution",fResolution+mod,"Resolution/B");
      }
    }
  }

  void KoaRawSimple::FillImp()
  {
    for(int mod=0;mod<nr_mesymodules;mod++){
      fTree[mod]->Fill();
    }
  }

  void KoaRawSimple::DoneImp()
  {
    for(int mod=0;mod<nr_mesymodules;mod++){
      fTree[mod]->Write();
    }
  }
}
