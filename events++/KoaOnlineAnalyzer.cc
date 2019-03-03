#include "KoaOnlineAnalyzer.hxx"
#include "KoaLoguru.hxx"

#define ADC_MAXRANGE 0x1FFF
#define QDC_MAXRANGE 0xFFF
#define TDC_MAXRANGE 0xFFFF
#define UNDER_THRESHOLD -5
#define ADC_OVERFLOW 0x2000
#define QDC_OVERFLOW 0x1000

namespace DecodeUtil
{
  //
  KoaOnlineAnalyzer::KoaOnlineAnalyzer(const char* dir, Long_t address, Int_t size) : fMapAddress(address), fMapSize(size)
  {
    SetDirectory(dir);
  }
  //
  KoaOnlineAnalyzer::~KoaOnlineAnalyzer()
  {
    
  }
  //
  void
  KoaOnlineAnalyzer::SetDirectory(const char* dir)
  {
    fFileName.Form("%s/koala_online.map",dir);
  }
  //
  int
  KoaOnlineAnalyzer::Init()
  {
    // create memory mapped file
		TMapFile::SetMapAddress(fMapAddress);
    fMapFile = TMapFile::Create(fFileName.Data(),"RECREATE",fMapSize,"KOALA_Online_NEW");

    // create histograms
    hadc = new TH1F("hadc","hadc",ADC_MAXRANGE+2,-1.5,ADC_MAXRANGE+0.5);
    hqdc = new TH1F("hqdc","hqdc",QDC_MAXRANGE+11,-9.5, QDC_MAXRANGE+1.5);
    htdc = new TH1F("htdc","htdc",TDC_MAXRANGE+2,-1.5,TDC_MAXRANGE+0.5);

    // create trees
    fModuleId = new UChar_t[nr_mesymodules];
    fResolution = new Char_t[nr_mesymodules];
    fNrWords  =  new Short_t[nr_mesymodules];
    fTimestamp = new Long64_t[nr_mesymodules];
    fData = new Int_t[nr_mesymodules][34];
    // for(int mod=0;mod<nr_mesymodules;mod++){
    //   fTree[mod]=new TTree(mesymodules[mod].label,mesymodules[mod].label);

    //   fTree[mod]->Branch("ModuleId",fModuleId+mod,"ModuleId/b");
    //   fTree[mod]->Branch("NrWords",fNrWords+mod,"NrWords/S");
    //   fTree[mod]->Branch("Timestamp",fTimestamp+mod,"Timestamp/L");
    //   fTree[mod]->Branch("Data",fData+mod,"Data[34]/I");
    //   if(mesymodules[mod].mesytype != mesytec_mqdc32){
    //     fTree[mod]->Branch("Resolution",fResolution+mod,"Resolution/B");
    //   }
    // }

  }
  //
  int
  KoaOnlineAnalyzer::Analyze()
  {
    // process koala event
    koala_event* koala_cur=nullptr;
    while(koala_cur=fKoalaPrivate->drop_event()){
      // extract channel data
      Decode(koala_cur);
      koala_cur->recycle();
      // fill histograms
      if(fCounter%30000==0)
        Reset();
      hadc->Fill(fData[0][0]);
      hqdc->Fill(fData[6][0]);
      htdc->Fill(fData[8][0]);
      // update mapfile
      if(fCounter%100==0){
        fMapFile->Update();
        if(!fCounter) fMapFile->ls();
      }
      fCounter++;
    }

    //
    RecycleEmsEvents();
    return 0;
  }
  //
  int
  KoaOnlineAnalyzer::Done()
  {
    if(fModuleId)   delete [] fModuleId;
    if(fResolution) delete [] fResolution;
    if(fNrWords)    delete [] fNrWords;
    if(fTimestamp)  delete [] fTimestamp;
    if(fData)       delete [] fData;
    //
    if(hadc) delete hadc;
    if(hqdc) delete hqdc;
    if(htdc) delete htdc;
  }
  //
  void
  KoaOnlineAnalyzer::Reset()
  {
    // fCounter=0;
    hadc->Reset();
    hqdc->Reset();
    htdc->Reset();
  }
  //
  void
  KoaOnlineAnalyzer::Decode(koala_event* koala)
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
                      LOG_F(WARNING,"Unknown resolution");
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
}
