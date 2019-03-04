#include "KoaOnlineAnalyzer.hxx"
#include "KoaLoguru.hxx"

#define ADC_MAXRANGE 0x1FFF
#define QDC_MAXRANGE 0xFFF
#define TDC_MAXRANGE 0xFFFF
#define UNDER_THRESHOLD -5
#define ADC_OVERFLOW 0x2000
#define QDC_OVERFLOW 0x1000

static Float_t g_invalidtime=-10;

namespace DecodeUtil
{
  //
  KoaOnlineAnalyzer::KoaOnlineAnalyzer(const char* dir, Long_t address, Int_t size, Int_t entries) : fMapAddress(address), fMapSize(size), fMaxEvents(entries)
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
  void
  KoaOnlineAnalyzer::SetMapAddress(Long_t address)
  {
    fMapAddress=address;
  }
  void
  KoaOnlineAnalyzer::SetMapSize(Int_t size)
  {
    fMapSize = size;
  }
  //
  int
  KoaOnlineAnalyzer::Init()
  {
    // create memory mapped file
		TMapFile::SetMapAddress(fMapAddress);
    fMapFile = TMapFile::Create(fFileName.Data(),"RECREATE",fMapSize,"KOALA_Online_NEW");

    // create histograms
    InitHistograms();

    fMapFile->Update();
    fMapFile->ls();
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

    // InitTrees();
    // initialize detector mapping
    InitMapping();
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
      DetectorMapped();
      koala_cur->recycle();

      // fill histograms
      // FillTrees();
      FillHistograms();

      // update mapfile
      fCounter++;
      if(fCounter%100==0){
        fMapFile->Update();
      }
      if(fCounter%fMaxEvents==0)
        Reset();
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
  }
  //
  void
  KoaOnlineAnalyzer::Reset()
  {
    // fCounter=0;
    ResetHistograms();
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
  //
  void
  KoaOnlineAnalyzer::InitMapping()
  {
    // Timestamps
    for(int i=0;i<48;i++){
      fPSi1_Timestamp[i]=reinterpret_cast<Int_t*>(&g_invalidtime);
    }
    for(int i=0;i<64;i++){
      fPSi2_Timestamp[i]=reinterpret_cast<Int_t*>(&g_invalidtime);
    }
    for(int i=0;i<32;i++){
      fPGe1_Timestamp[i]=reinterpret_cast<Int_t*>(&g_invalidtime);
      fPGe2_Timestamp[i]=reinterpret_cast<Int_t*>(&g_invalidtime);
    }
    // Si1 : Physical Position 48->17 ===> TDC2 1->32
    for(int i=0;i<32;i++){
      fPSi1_Timestamp[47-i]=&fData[8][i];
    }
    // Si2 : Physical Position 1->16 ===> TDC1 17->32
    for(int i=0;i<16;i++){
      fPSi2_Timestamp[i]=&fData[7][16+i];
    }
    // Si2 : Physical Position 17->24 ===> TDC1 9->16
    for(int i=0;i<8;i++){
      fPSi2_Timestamp[16+i]=&fData[7][8+i];
    }
    // Fwd: 1-8 ===> TDC1 1->8
    for(int i=0;i<8;i++){
      fPFwd_Timestamp[i]=&fData[7][i];
    }

    // Amplitudes
    // Si1 : Physical Position 48->17 ===> ADC1 1->32
    for(int i=0;i<32;i++){
      fPSi1_Amplitude[47-i]=&fData[0][i];
    }
    // Si1 : Physical Position 16->1 ===> ADC2 1->16
    for(int i=0;i<16;i++){
      fPSi1_Amplitude[15-i]=&fData[1][i];
    }
    // Recoil Detector Rear Side:
    // Si#1 Rear: ADC2 17
    // Si#2 Rear: ADC2 18
    // Ge#1 Rear: ADC2 19
    // Ge#2 Rear: ADC2 20
    for(int i=0;i<4;i++){
      fPRecRear_Amplitude[i]=&fData[1][16+i];
    }
    // Si2 : Physical Position 1->32 ===> ADC3 1->32
    for(int i=0;i<32;i++){
      fPSi2_Amplitude[i]=&fData[2][i];
    }
    // Si2 : Physical Position 33->64 ===> ADC4 1->32
    for(int i=0;i<32;i++){
      fPSi2_Amplitude[32+i]=&fData[3][i];
    }
    // Ge1 : Physical Position 1->32 ===> ADC5 32->1
    for(int i=0;i<32;i++){
      fPGe1_Amplitude[31-i]=&fData[4][i];
    }
    // Ge2 : Physical Position 1->32 ===> ADC6 1->32
    for(int i=0;i<32;i++){
      fPGe2_Amplitude[i]=&fData[5][i];
    }
    // Fwd: 1->8 ===> QDC 1->8
    for(int i=0;i<8;i++){
      fPFwd_Amplitude[i]=&fData[6][i];
    }
  }
  //
  void
  KoaOnlineAnalyzer::DetectorMapped()
  {
    const Float_t time_resolution[9]={-1,1./256,2./256,4./256,8./256,16./256,32./256,64./256,128./256};

    // Timestamps
    for(int i=0;i<48;i++){
      fSi1_Timestamp[i]=g_invalidtime;
    }
    for(int i=0;i<64;i++){
      fSi2_Timestamp[i]=g_invalidtime;
    }
    for(int i=0;i<32;i++){
      fGe1_Timestamp[i]=g_invalidtime;
      fGe2_Timestamp[i]=g_invalidtime;
    }
    // Si1 : Physical Position 48->17 ===> TDC2 1->32
    for(int i=0;i<32;i++){
      fSi1_Timestamp[47-i]=fData[8][i]*time_resolution[fResolution[8]-1];
    }
    // Si2 : Physical Position 1->16 ===> TDC1 17->32
    for(int i=0;i<16;i++){
      fSi2_Timestamp[i]=fData[7][16+i]*time_resolution[fResolution[7]-1];
    }
    // Si2 : Physical Position 17->24 ===> TDC1 9->16
    for(int i=0;i<8;i++){
      fSi2_Timestamp[16+i]=fData[7][8+i]*time_resolution[fResolution[7]-1];
    }
    // Fwd: 1-8 ===> TDC1 1->8
    for(int i=0;i<8;i++){
      fFwd_Timestamp[i]=fData[7][i]*time_resolution[fResolution[7]-1];
    }
    fTrig_Timestamp[0]=(fData[7][32]*time_resolution[fResolution[7]-1]);
    fTrig_Timestamp[1]=(fData[8][32]*time_resolution[fResolution[8]-1]);

    // Amplitudes
    // Si1 : Physical Position 48->17 ===> ADC1 1->32
    for(int i=0;i<32;i++){
      fSi1_Amplitude[47-i]=fData[0][i];
    }
    // Si1 : Physical Position 16->1 ===> ADC2 1->16
    for(int i=0;i<16;i++){
      fSi1_Amplitude[15-i]=fData[1][i];
    }
    // Recoil Detector Rear Side:
    // Si#1 Rear: ADC2 17
    // Si#2 Rear: ADC2 18
    // Ge#1 Rear: ADC2 19
    // Ge#2 Rear: ADC2 20
    for(int i=0;i<4;i++){
      fRecRear_Amplitude[i]=fData[1][16+i];
    }
    // Si2 : Physical Position 1->32 ===> ADC3 1->32
    for(int i=0;i<32;i++){
      fSi2_Amplitude[i]=fData[2][i];
    }
    // Si2 : Physical Position 33->64 ===> ADC4 1->32
    for(int i=0;i<32;i++){
      fSi2_Amplitude[32+i]=fData[3][i];
    }
    // Ge1 : Physical Position 1->32 ===> ADC5 32->1
    for(int i=0;i<32;i++){
      fGe1_Amplitude[31-i]=fData[4][i];
    }
    // Ge2 : Physical Position 1->32 ===> ADC6 1->32
    for(int i=0;i<32;i++){
      fGe2_Amplitude[i]=fData[5][i];
    }
    // Fwd: 1->8 ===> QDC 1->8
    for(int i=0;i<8;i++){
      fFwd_Amplitude[i]=fData[6][i];
    }
  }
  //
  void
  KoaOnlineAnalyzer::InitTrees()
  {
    fSi1Tree = new TTree("Si1","Si1");
    fSi1Tree->Branch("Amplitude",fSi1_Amplitude,"Amplitude[48]/I");
    fSi1Tree->Branch("Time",fSi1_Timestamp,"Time[48]/F");
    fSi1Tree->SetCircular(100);

    fSi2Tree = new TTree("Si2","Si2");
    fSi2Tree->Branch("Amplitude",fSi2_Amplitude,"Amplitude[64]/I");
    fSi2Tree->Branch("Time",fSi2_Timestamp,"Time[64]/F");
    fSi2Tree->SetCircular(100);

    fGe1Tree = new TTree("Ge1","Ge1");
    fGe1Tree->Branch("Amplitude",fGe1_Amplitude,"Amplitude[32]/I");
    fGe1Tree->Branch("Time",fGe1_Timestamp,"Time[32]/F");
    fGe1Tree->SetCircular(100);

    fGe2Tree = new TTree("Ge2","Ge2");
    fGe2Tree->Branch("Amplitude",fGe2_Amplitude,"Amplitude[32]/I");
    fGe2Tree->Branch("Time",fGe2_Timestamp,"Time[32]/F");
    fGe2Tree->SetCircular(100);

    fFwdTree = new TTree("Fwd","Fwd");
    fFwdTree->Branch("Amplitude",fFwd_Amplitude,"Amplitude[8]/I");
    fFwdTree->Branch("Time",fFwd_Timestamp,"Time[8]/F");
    fFwdTree->SetCircular(100);
  }
  //
  void
  KoaOnlineAnalyzer::FillTrees()
  {
    fSi1Tree->Fill();
    fSi2Tree->Fill();
    fGe1Tree->Fill();
    fGe2Tree->Fill();
    fFwdTree->Fill();
  }
  //
  void
  KoaOnlineAnalyzer::InitHistograms()
  {
    // Forward Detector
    for(int i=0;i<8;i++){
      hFwdAmp[i] = new TH1F(Form("hFwdAmp_%d",i+1),Form("Fwd Scintillator Amplitude: %d",i+1),QDC_MAXRANGE+11,-9.5, QDC_MAXRANGE+1.5);
      hFwdTime[i] = new TH1F(Form("hFwdTime_%d",i+1),Form("Fwd Scintillator Timestamp: %d",i+1),2050,-1.5,2048.5);
    }

    // Trigger timestamp
    for(int i=0;i<2;i++){
      hTrigTime[i] = new TH1F(Form("hTrigTime_%d",i+1),Form("TDC%d: Trigger Time",i+1),2050,-1.5,2048.5);
    }

    // Recoil Detector Timestamp
    // for(int i=0;i<32;i++){
    //   hSi1Time[i] = new TH1F(Form("hSi1Time_%d",i+17),Form("Si#1_Strip#%d : Timestamp (ns)",i+17),2050,-1.5,2048.5);
    // }
    // for(int i=0;i<24;i++){
    //   hSi2Time[i] = new TH1F(Form("hSi2Time_%d",i+1),Form("Si#2_Strip#%d : Timestamp (ns)",i+1),2050,-1.5,2048.5);
    // }
    hRecTime = new TH1F("hRecTime","Recoil Detector Time (ns): Accumulated",2050,-1.5,2048);

    // Recoil Hits
    hSi1Hits = new TH2F("hSi1Hits","Si#1 FrontSide: Hits Spectrum",48,0.5,48.5,ADC_MAXRANGE+2,-1.5,ADC_MAXRANGE+0.5);
    hSi2Hits = new TH2F("hSi2Hits","Si#2 FrontSide: Hits Spectrum",64,0.5,64.5,ADC_MAXRANGE+2,-1.5,ADC_MAXRANGE+0.5);
    hGe1Hits = new TH2F("hGe1Hits","Ge#1 FrontSide: Hits Spectrum",32,0.5,32.5,ADC_MAXRANGE+2,-1.5,ADC_MAXRANGE+0.5);
    hGe2Hits = new TH2F("hGe2Hits","Ge#2 FrontSide: Hits Spectrum",32,0.5,32.5,ADC_MAXRANGE+2,-1.5,ADC_MAXRANGE+0.5);
    
    // Recoil Rear Amplitude
    hSi1RearAmp = new TH1F("hSi1RearAmp","Si#1 RearSide: Amplitude",ADC_MAXRANGE+2,-1.5,ADC_MAXRANGE+0.5);
    hSi2RearAmp = new TH1F("hSi2RearAmp","Si#2 RearSide: Amplitude",ADC_MAXRANGE+2,-1.5,ADC_MAXRANGE+0.5);
    hGe1RearAmp = new TH1F("hGe1RearAmp","Ge#1 RearSide: Amplitude",ADC_MAXRANGE+2,-1.5,ADC_MAXRANGE+0.5);
    hGe2RearAmp = new TH1F("hGe2RearAmp","Ge#2 RearSide: Amplitude",ADC_MAXRANGE+2,-1.5,ADC_MAXRANGE+0.5);
  }
  //
  void
  KoaOnlineAnalyzer::FillHistograms()
  {
    // Forward Detector
    for(int i=0;i<8;i++){
      hFwdAmp[i]->Fill(fFwd_Amplitude[i]);
      if((*fPFwd_Timestamp[i])!=UNDER_THRESHOLD)
        hFwdTime[i]->Fill(fFwd_Timestamp[i]);
    }

    // Trigger Timestamp
    for(int i=0;i<2;i++){
      hTrigTime[i]->Fill(fTrig_Timestamp[i]);
    }

    // Recoil Timestamp
    for(int i=0;i<32;i++){
      if((*fPSi1_Timestamp[16+i])!=UNDER_THRESHOLD){
        // hSi1Time[i]->Fill(fSi1_Timestamp[16+i]);
        hRecTime->Fill(fSi1_Timestamp[16+i]);
      }
    }
    for(int i=0;i<24;i++){
      if((*fPSi2_Timestamp[i])!=UNDER_THRESHOLD){
        // hSi2Time[i]->Fill(fSi2_Timestamp[i]);
        hRecTime->Fill(fSi2_Timestamp[i]);
      }
    }

    // Recoil Hits
    for(int i=0;i<48;i++){
      hSi1Hits->Fill(i+1,fSi1_Amplitude[i]);
    }
    for(int i=0;i<64;i++){
      hSi2Hits->Fill(i+1,fSi2_Amplitude[i]);
    }
    for(int i=0;i<32;i++){
      hGe1Hits->Fill(i+1,fGe1_Amplitude[i]);
      hGe2Hits->Fill(i+1,fGe2_Amplitude[i]);
    }

    // Recoil Rear Amplitude
    hSi1RearAmp->Fill(fRecRear_Amplitude[0]);
    hSi2RearAmp->Fill(fRecRear_Amplitude[1]);
    hGe1RearAmp->Fill(fRecRear_Amplitude[2]);
    hGe2RearAmp->Fill(fRecRear_Amplitude[3]);
  }
  //
  void
  KoaOnlineAnalyzer::ResetHistograms()
  {
    // Forward Detector
    for(int i=0;i<8;i++){
      hFwdTime[i]->Reset();
      hFwdAmp[i]->Reset();
    }
    // Trigger timestamp
    for(int i=0;i<2;i++){
      hTrigTime[i]->Reset();
    }
    // Recoil Timestamp
    // for(int i=0;i<32;i++){
    //   hSi1Time[i]->Reset();
    // }
    // for(int i=0;i<24;i++){
    //   hSi2Time[i]->Reset();
    // }
    hRecTime->Reset();
    // Recoil Hits
    hSi1Hits->Reset();
    hSi2Hits->Reset();
    hGe1Hits->Reset();
    hGe2Hits->Reset();
    // Recoil Rear Amplitude
    hSi1RearAmp->Reset();
    hSi2RearAmp->Reset();
    hGe1RearAmp->Reset();
    hGe2RearAmp->Reset();
  }
  //
  void
  KoaOnlineAnalyzer::SetMaxEvents(Int_t entries)
  {
    fMaxEvents = entries;
  }
}
