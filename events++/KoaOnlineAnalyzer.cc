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
  KoaOnlineAnalyzer::KoaOnlineAnalyzer(const char* dir, Long_t address, Int_t size, Int_t entries, Int_t circular_event) : fMapAddress(address),
                                                                                                                           fMapSize(size),
                                                                                                                           fMaxEvents(entries),
                                                                                                                           fCircularSize(circular_event)
  {
    SetDirectory(dir);
    fEmsTimeSecond=time(0);
    fKoalaCounter=0;
    fEmsCounter=0;
    fScalerResetInterval=1200;
    fScalerUpdateInterval=5;
    fEventNr = 0;
  }
  //
  KoaOnlineAnalyzer::~KoaOnlineAnalyzer()
  {
    DeleteHistograms();
    DeleteScalerGraphs();
  }
  //
  void
  KoaOnlineAnalyzer::SetDirectory(const char* dir)
  {
    fMapFileName.Form("%s/koala_online.map",dir);
    fRootFileName.Form("%s/koala_online.root",dir);
  }
  //
  void
  KoaOnlineAnalyzer::SetMapAddress(Long_t address)
  {
    fMapAddress=address;
  }
  //
  void
  KoaOnlineAnalyzer::SetMapSize(Int_t size)
  {
    fMapSize = size;
  }
  //
  void
  KoaOnlineAnalyzer::SetCircularSize(Int_t circular_event)
  {
    fCircularSize = circular_event;
  }
  //
  void
  KoaOnlineAnalyzer::SetScalerUpdateInterval(Int_t second)
  {
    fScalerUpdateInterval = second;
  }
  //
  void
  KoaOnlineAnalyzer::SetScalerResetInterval(Int_t second)
  {
    fScalerResetInterval = second;
  }
  //
  int
  KoaOnlineAnalyzer::Init()
  {
    // Raw Data
    fModuleId = new UChar_t[nr_mesymodules];
    fResolution = new Char_t[nr_mesymodules];
    fNrWords  =  new Short_t[nr_mesymodules];
    fTimestamp = new Long64_t[nr_mesymodules];
    fData = new Int_t[nr_mesymodules][34];

    fScaler = new UInt_t[34];
    fScalerDiff = new UInt_t[34];
    fHitRate = new Double_t[34];

    // initialize detector mapping
    InitMapping();

    // create memory mapped file
		TMapFile::SetMapAddress(fMapAddress);
    fMapFile = TMapFile::Create(fMapFileName.Data(),"RECREATE",fMapSize,"KOALA_Online_NEW");

    // Histograms and Graphs in the map file
    InitHistograms();
    InitScalerGraphs();

    fMapFile->Update();
    fMapFile->ls();

    // Event structure to be saved into root file
    fRootFile = new TFile(fRootFileName,"recreate");
    InitTrees();
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
      fKoalaCounter++;
      if(fKoalaCounter%100==0){
        fMapFile->Update();
      }
    }

    //
    ems_event* ems_cur=nullptr;
    time_t cur_time_second;
    Long_t cur_time_usecond;
    double diff_time;
    UInt_t cur_events;
    while(ems_cur=fEmsPrivate->drop_event()){
      if(ems_cur->tv_valid && ems_cur->scaler_valid){
        cur_time_second=ems_cur->tv.tv_sec;
        diff_time=difftime(cur_time_second,fEmsTimeSecond);

        if(diff_time<fScalerUpdateInterval) break;

        if(diff_time>30){
          int nr=diff_time/5.0;
          Int_t npoints;
          for(int i=0;i<4;i++){
            for(int j=0;j<nr;j++){
              npoints=gHitRateRec[i]->GetN();
              gHitRateRec[i]->SetPoint(npoints,cur_time_second-(nr-j)*5,0);
            }
            for(int j=0;j<nr;j++){
              npoints=gHitRateFwd[i]->GetN();
              gHitRateFwd[i]->SetPoint(npoints,cur_time_second-(nr-j)*5,0);
            }
          }
          for(int j=0;j<nr;j++){
            npoints=gHitRateCommonOr->GetN();
            gHitRateCommonOr->SetPoint(npoints,cur_time_second-(nr-j)*5,0);
            npoints=gEventRate->GetN();
            gEventRate->SetPoint(npoints,cur_time_second-(nr-j)*5,0);
          }
          for(int i=0;i<2;i++){
            for(int j=0;j<nr;j++){
              npoints=gHitRateGeOverlap[i]->GetN();
              gHitRateGeOverlap[i]->SetPoint(npoints,cur_time_second-(nr-j)*5,0);
              npoints=gHitRateSiRear[i]->GetN();
              gHitRateSiRear[i]->SetPoint(npoints,cur_time_second-(nr-j)*5,0);
            }
          }
          
        }

        cur_time_usecond=ems_cur->tv.tv_usec;
        diff_time += (cur_time_usecond-fEmsTimeUSecond)*1e-6;
        cur_events=fKoalaPrivate->get_statist_events();
        for(int i=0;i<34;i++){
          fScalerDiff[i]=ems_cur->scaler[i]-fScaler[i];
          fScaler[i]=ems_cur->scaler[i];
          fHitRate[i]=fScalerDiff[i]/diff_time;
        }
        fEmsTimeSecond=cur_time_second;
        fEmsTimeUSecond=cur_time_usecond;
        fEventRate=(cur_events-fEventNr)/diff_time;
        fDaqEfficiency=(fScalerDiff[5])/(Double_t)fScalerDiff[4];
        fEventNr=cur_events;
        //
        if(fEmsCounter)
          FillScalerGraphs();
        //
        fEmsCounter++;
      }
      //
      ems_cur->recycle();
    }

    //
    Reset();
    return 0;
  }
  //
  int
  KoaOnlineAnalyzer::Done()
  {
    //
    if(fModuleId)   delete [] fModuleId;
    if(fResolution) delete [] fResolution;
    if(fNrWords)    delete [] fNrWords;
    if(fTimestamp)  delete [] fTimestamp;
    if(fData)       delete [] fData;
    //
    if(fScaler)     delete [] fScaler;
    if(fScalerDiff) delete [] fScalerDiff;
    if(fHitRate)    delete [] fHitRate;
    //
  }
  //
  void
  KoaOnlineAnalyzer::Reset()
  {
    ResetHistograms();
    ResetScalerGraphs();
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
    // Si1 : Physical Position 48->17 ===> TDC1 1->32
    for(int i=0;i<32;i++){
      fPSi1_Timestamp[47-i]=&fData[7][i];
    }
    // Si1 : Physical Position 16->1 ===> TDC2 1->16
    for(int i=0;i<16;i++){
      fPSi1_Timestamp[15-i]=&fData[8][i];
    }
    // Si2 : Physical Position 1->16 ===> TDC2 17->32
    for(int i=0;i<16;i++){
      fPSi2_Timestamp[i]=&fData[8][16+i];
    }
    // Si2 : Physical Position 17->32 ===> TDC3 1->16
    for(int i=0;i<16;i++){
      fPSi2_Timestamp[16+i]=&fData[9][i];
    }
    // Si2 : Physical Position 33->38 ===> TDC3 27->32
    for(int i=0;i<6;i++){
      fPSi2_Timestamp[32+i]=&fData[9][26+i];
    }
    // Si1 Rear Time : TDC3 25
    // Si2 Rear Time : TDC3 26
    for(int i=0;i<2;i++){
      fPRecRear_Timestamp[i]=&fData[9][24+i];
    }
    // Fwd: 1-8 ===> TDC3 17->24
    for(int i=0;i<8;i++){
      fPFwd_Timestamp[i]=&fData[9][16+i];
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

    //////////////////// Scaler ///////////////////
    // Recoil Detector 
    for(int i=0;i<4;i++){
      fPScalerRec[i]=fScaler+i;
      fPHitRateRec[i]=fHitRate+i;
    }
    // Fwd Detector
    for(int i=0;i<4;i++){
      fPScalerFwd[i]=fScaler+i+8;
      fPHitRateFwd[i]=fHitRate+i+8;
    }
    // Common OR
    fPScalerCommonOr=fScaler+4;
    fPHitRateCommonOr=fHitRate+4;
    // Evt gate
    fPScalerEvtGate=fScaler+5;
    fPHitRateEvtGate=fHitRate+5;
    // Ge1 and Ge2 Overlapping area (5 strips)
    for(int i=0;i<2;i++){
      fPScalerGeOverlap[i]=fScaler+6+i;
      fPHitRateGeOverlap[i]=fHitRate+6+i;
    }
    // Si1 Si2 Rear Side
    for(int i=0;i<2;i++){
      fPScalerSiRear[i]=fScaler+24+i;
      fPHitRateSiRear[i]=fHitRate+24+i;
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
    // Si1 : Physical Position 48->17 ===> TDC1 1->32
    for(int i=0;i<32;i++){
      fSi1_Timestamp[47-i]=fData[7][i]*time_resolution[fResolution[7]-1];
    }
    // Si1 : Physical Position 16->1 ===> TDC2 1->16
    for(int i=0;i<16;i++){
      fSi1_Timestamp[15-i]=fData[8][i]*time_resolution[fResolution[8]-1];
    }
    // Si2 : Physical Position 1->16 ===> TDC2 17->32
    for(int i=0;i<16;i++){
      fSi2_Timestamp[i]=fData[8][16+i]*time_resolution[fResolution[8]-1];
    }
    // Si2 : Physical Position 17->32 ===> TDC3 1->16
    for(int i=0;i<16;i++){
      fSi2_Timestamp[16+i]=fData[9][i]*time_resolution[fResolution[9]-1];
    }
    // Si2 : Physical Position 33->38 ===> TDC3 27->32
    for(int i=0;i<6;i++){
      fSi2_Timestamp[32+i]=fData[9][26+i]*time_resolution[fResolution[9]-1];
    }
    // Si1 Rear Time : TDC3 25
    // Si2 Rear Time : TDC3 26
    for(int i=0;i<2;i++){
      fRecRear_Timestamp[i]=fData[9][24+i]*time_resolution[fResolution[9]-1];
    }
    // Fwd: 1-8 ===> TDC3 1->8
    for(int i=0;i<8;i++){
      fFwd_Timestamp[i]=fData[9][16+i]*time_resolution[fResolution[9]-1];
    }
    fTrig_Timestamp[0]=(fData[7][32]*time_resolution[fResolution[7]-1]);
    fTrig_Timestamp[1]=(fData[8][32]*time_resolution[fResolution[8]-1]);
    fTrig_Timestamp[2]=(fData[9][32]*time_resolution[fResolution[9]-1]);

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
    // fSi1Tree = new TTree("Si1","Si1");
    // fSi1Tree->Branch("Amplitude",fSi1_Amplitude,"Amplitude[48]/I");
    // fSi1Tree->Branch("Time",fSi1_Timestamp,"Time[48]/F");
    // fSi1Tree->SetCircular(100);

    // fSi2Tree = new TTree("Si2","Si2");
    // fSi2Tree->Branch("Amplitude",fSi2_Amplitude,"Amplitude[64]/I");
    // fSi2Tree->Branch("Time",fSi2_Timestamp,"Time[64]/F");
    // fSi2Tree->SetCircular(100);

    // fGe1Tree = new TTree("Ge1","Ge1");
    // fGe1Tree->Branch("Amplitude",fGe1_Amplitude,"Amplitude[32]/I");
    // fGe1Tree->Branch("Time",fGe1_Timestamp,"Time[32]/F");
    // fGe1Tree->SetCircular(100);

    // fGe2Tree = new TTree("Ge2","Ge2");
    // fGe2Tree->Branch("Amplitude",fGe2_Amplitude,"Amplitude[32]/I");
    // fGe2Tree->Branch("Time",fGe2_Timestamp,"Time[32]/F");
    // fGe2Tree->SetCircular(100);

    // fFwdTree = new TTree("Fwd","Fwd");
    // fFwdTree->Branch("Amplitude",fFwd_Amplitude,"Amplitude[8]/I");
    // fFwdTree->Branch("Time",fFwd_Timestamp,"Time[8]/F");
    // fFwdTree->SetCircular(100);
  }
  //
  void
  KoaOnlineAnalyzer::FillTrees()
  {
    // fSi1Tree->Fill();
    // fSi2Tree->Fill();
    // fGe1Tree->Fill();
    // fGe2Tree->Fill();
    // fFwdTree->Fill();
  }
  //
  void
  KoaOnlineAnalyzer::InitHistograms()
  {
    TString RecName[4]={"Si#1","Si#2","Ge#1","Ge#2"};
    TString FwdName[4]={"Fwd#Out","Fwd#In","Fwd#Up","Fwd#Down"};
    Int_t   chnum[4]={48,64,32,32};
    // Forward Detector
    for(int i=0;i<8;i++){
      hFwdAmp[i] = new TH1F(Form("hFwdAmp_%d",i+1),Form("Fwd Scintillator Amplitude: %d",i+1),QDC_MAXRANGE+11,-9.5, QDC_MAXRANGE+1.5);
      hFwdTime[i] = new TH1F(Form("hFwdTime_%d",i+1),Form("Fwd Scintillator Timestamp: %d",i+1),2098,-1.5,2096.5);
    }

    // Trigger timestamp
    for(int i=0;i<2;i++){
      hTrigTime[i] = new TH1F(Form("hTrigTime_%d",i+1),Form("TDC%d: Trigger Time",i+1),2098,-1.5,2096.5);
    }

    // Recoil Detector Timestamp
    // for(int i=0;i<32;i++){
    //   hSi1Time[i] = new TH1F(Form("hSi1Time_%d",i+17),Form("Si#1_Strip#%d : Timestamp (ns)",i+17),2050,-1.5,2048.5);
    // }
    // for(int i=0;i<24;i++){
    //   hSi2Time[i] = new TH1F(Form("hSi2Time_%d",i+1),Form("Si#2_Strip#%d : Timestamp (ns)",i+1),2050,-1.5,2048.5);
    // }
    hRecTime = new TH1F("hRecTime","Recoil Detector Time (ns): Accumulated",2098,-1.5,2096.5);
    hRecRearTime[0] = new TH1F("hRecRearTime_1","Si#1 Rear Time (ns)",2098,-1.5,2096.5);
    hRecRearTime[1] = new TH1F("hRecRearTime_2","Si#2 Rear Time (ns)",2098,-1.5,2096.5);

    // Recoil Hits
    for(int i=0;i<4;i++){
      h2RecHits[i] = new TH2F(Form("h2RecHits_%d",i+1),Form("%s FrontSide: Hits Spectrum",RecName[i].Data()),chnum[i],0.5,chnum[i]+0.5,ADC_MAXRANGE+2,-1.5,ADC_MAXRANGE+0.5);
      h2RecHitsCut[i] = new TH2F(Form("h2RecHitsCut_%d",i+1),Form("%s FrontSide: Hits Spectrum (After Fwd Amplitude Cuts)",RecName[i].Data()),chnum[i],0.5,chnum[i]+0.5,ADC_MAXRANGE+2,-1.5,ADC_MAXRANGE+0.5);
      hRecRearAmp[i]=new TH1F(Form("hRecRearAmp_%d",i+1),Form("%s RearSide: Amplitude",RecName[i].Data()),ADC_MAXRANGE+2,-1.5,ADC_MAXRANGE+0.5);
      hRecRearAmpCut[i]=new TH1F(Form("hRecRearAmpCut_%d",i+1),Form("%s RearSide: Amplitude (After Fwd Amplitude Cuts)",RecName[i].Data()),ADC_MAXRANGE+2,-1.5,ADC_MAXRANGE+0.5);
    }
    

    // Si1 Correlation
    for(int i=0;i<2;i++){
    //   h2RecRearAmpVsTime[i]=new TH2F(Form("h2RecRearAmpVsTime_%d",i+1),Form("Si#%d Rear: Amplitude VS Time",i+1),ADC_MAXRANGE+2,-1.5,ADC_MAXRANGE+0.5,2098,-1.5,2096.5);
    // }
    // for(int i=0;i<2;i++){
    //   h2RecRearAmpVsTime[i+2]=new TH2F(Form("h2RecRearAmpVsTime_%d",i+3),Form("Ge#%d Rear: Amplitude VS Time",i+1),ADC_MAXRANGE+2,-1.5,ADC_MAXRANGE+0.5,2098,-1.5,2096.5);
      h2FwdTimeVSAmp[i]=new TH2F(Form("h2FwdTimeVSAmp_%d",i+1),Form("Fwd%d : Time VS Amplitude",i+1),2098,-1.5,2096.5,QDC_MAXRANGE+11,-9.5, QDC_MAXRANGE+1.5);
      h2FwdTimeVSRecAmp[i]=new TH2F(Form("h2FwdTimeVSRecAmp_%d",i+1),Form("Fwd1_Time VS Si%d_Amplitude",i+1),2098,-1.5,2096.5,ADC_MAXRANGE+11,-9.5, ADC_MAXRANGE+1.5);
      h2FwdAmpVSRecAmp[i]=new TH2F(Form("h2FwdAmpVSRecAmp_%d",i+1),Form("Fwd1_Amp VS Si%d_Amplitude",i+1),QDC_MAXRANGE+11,-9.5, QDC_MAXRANGE+1.5,ADC_MAXRANGE+11,-9.5, ADC_MAXRANGE+1.5);
      hRecRearTimeDiff[i]=new TH1F(Form("hRecRearTimeDiff_%d",i+1),Form("Si%dRear_Time - Fwd1_Time",i+1),2000,-1000,1000);
    }
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
    for(int i=0;i<22;i++){
      if((*fPSi2_Timestamp[i])!=UNDER_THRESHOLD){
        // hSi2Time[i]->Fill(fSi2_Timestamp[i]);
        hRecTime->Fill(fSi2_Timestamp[i]);
      }
    }
    for(int i=0;i<2;i++){
      if((*fPRecRear_Timestamp[i])!=UNDER_THRESHOLD){
        hRecRearTime[i]->Fill(fRecRear_Timestamp[i]);
      }
    }

    // Recoil Hits
    for(int i=0;i<48;i++){
      h2RecHits[0]->Fill(i+1,fSi1_Amplitude[i]);
      if(fFwd_Amplitude[0]>1000 && fFwd_Amplitude[1]>1000
         && fSi1_Timestamp[i]>0
         && -10<(fFwd_Timestamp[0]-fFwd_Timestamp[1])
         && (fFwd_Timestamp[0]-fFwd_Timestamp[1])<10 )
      {
        h2RecHitsCut[0]->Fill(i+1,fSi1_Amplitude[i]);
      }
    }
    for(int i=0;i<64;i++){
      h2RecHits[1]->Fill(i+1,fSi2_Amplitude[i]);
      if(fFwd_Amplitude[0]>1000 && fFwd_Amplitude[1]>1000
         && fSi2_Timestamp[i]>0
         && -10<(fFwd_Timestamp[0]-fFwd_Timestamp[1])
         && (fFwd_Timestamp[0]-fFwd_Timestamp[1])<10 )
      {
        h2RecHitsCut[1]->Fill(i+1,fSi2_Amplitude[i]);
      }
    }
    for(int i=0;i<32;i++){
      h2RecHits[2]->Fill(i+1,fGe1_Amplitude[i]);
      if(fFwd_Amplitude[0]>1000 && fFwd_Amplitude[1]>1000
         && fGe1_Timestamp[i]>0
         && -10<(fFwd_Timestamp[0]-fFwd_Timestamp[1])
         && (fFwd_Timestamp[0]-fFwd_Timestamp[1])<10 )
        h2RecHitsCut[2]->Fill(i+1,fGe1_Amplitude[i]);

      h2RecHits[3]->Fill(i+1,fGe2_Amplitude[i]);
      if(fFwd_Amplitude[0]>1000 && fFwd_Amplitude[1]>1000
         && fGe2_Timestamp[i]>0
         && -10<(fFwd_Timestamp[0]-fFwd_Timestamp[1])
         && (fFwd_Timestamp[0]-fFwd_Timestamp[1])<10 )
        h2RecHitsCut[3]->Fill(i+1,fGe2_Amplitude[i]);
    }

    // Recoil Rear Amplitude
    for(int i=0;i<4;i++){
      hRecRearAmp[i]->Fill(fRecRear_Amplitude[i]);
      if(fFwd_Amplitude[0]>1000)
        hRecRearAmpCut[i]->Fill(fRecRear_Amplitude[i]);
    }

    // Correlation
    for(int i=0;i<2;i++){
      if((*fPFwd_Timestamp[i])!=UNDER_THRESHOLD){
        h2FwdTimeVSAmp[i]->Fill(fFwd_Timestamp[i],fFwd_Amplitude[i]);
        if(*fPRecRear_Timestamp[i]!=UNDER_THRESHOLD){
          hRecRearTimeDiff[i]->Fill(fRecRear_Timestamp[i]-fFwd_Timestamp[0]);
        }
      }
    }
    for(int i=0;i<2;i++){
      h2FwdTimeVSRecAmp[i]->Fill(fFwd_Timestamp[0],fRecRear_Amplitude[i]);
      h2FwdAmpVSRecAmp[i]->Fill(fFwd_Amplitude[0],fRecRear_Amplitude[i]);
    }
  }
  //
  void
  KoaOnlineAnalyzer::ResetHistograms()
  {
    if(fKoalaCounter==fMaxEvents){
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
      // hRecTime->Reset();
      for(int i=0;i<2;i++){
        // hRecRearTime[i]->Reset();
      }
      // Recoil Hits
      // Recoil Rear Amplitude
      for(int i=0;i<4;i++){
        h2RecHits[i]->Reset();
        h2RecHitsCut[i]->Reset();
        hRecRearAmp[i]->Reset();
        hRecRearAmpCut[i]->Reset();
      }

      // Correlation
      for(int i=0;i<2;i++){
        h2FwdAmpVSRecAmp[i]->Reset();
        h2FwdTimeVSAmp[i]->Reset();
        h2FwdTimeVSRecAmp[i]->Reset();
        hRecRearTimeDiff[i]->Reset();
      }
      
      fKoalaCounter=0;
    }
  }
  //
  void
  KoaOnlineAnalyzer::DeleteHistograms()
  {
    //
    for(int i=0;i<2;i++){
      if(hTrigTime[i]) delete hTrigTime[i];
    }
    //
    for(int i=0;i<8;i++){
      if(hFwdAmp[i]) delete hFwdAmp[i];
      if(hFwdTime[i]) delete hFwdTime[i];
    }
    //
    if(hRecTime) delete hRecTime;
    for(int i=0;i<2;i++){
      if(hRecRearTime[i]) delete hRecRearTime[i];
    }
    //
    for(int i=0;i<4;i++){
      if(h2RecHits[i]) delete h2RecHits[i];
      if(h2RecHitsCut[i]) delete h2RecHitsCut[i];
      if(hRecRearAmp[i]) delete hRecRearAmp[i];
      if(hRecRearAmpCut[i]) delete hRecRearAmpCut[i];
    }
    //
    for(int i=0;i<2;i++){
      if(h2FwdTimeVSAmp[i]) delete h2FwdTimeVSAmp[i];
      if(h2FwdTimeVSRecAmp[i]) delete h2FwdTimeVSRecAmp[i];
      if(h2FwdAmpVSRecAmp[i]) delete h2FwdAmpVSRecAmp[i];
      if(hRecRearTimeDiff[i]) delete hRecRearTimeDiff[i];
    }
  }
  //
  void
  KoaOnlineAnalyzer::InitScalerGraphs()
  {
    TString RecName[4]={"Si#1","Si#2","Ge#1","Ge#2"};
    TString FwdName[4]={"Fwd#Out","Fwd#In","Fwd#Up","Fwd#Down"};
    //
    // for(int i=0;i<4;i++){
    //   gScalerRec[i] = new TGraph();
    //   gScalerRec[i]->SetNameTitle(Form("gScalerRec_%d",i+1),Form("%s : Scaler Counts",RecName[i].Data()));
    //   gScalerRec[i]->SetLineWidth(2);
    //   gScalerRec[i]->SetLineColor(1+i);
    //   gScalerRec[i]->SetMarkerColor(1+i);
    //   fMapFile->Add(gScalerRec[i]);
    // }
    // for(int i=0;i<4;i++){
    //   gScalerFwd[i] = new TGraph();
    //   gScalerFwd[i]->SetNameTitle(Form("gScalerFwd_%d",i+1),Form("%s : Scaler Counts",FwdName[i].Data()));
    //   gScalerFwd[i]->SetLineWidth(2);
    //   gScalerFwd[i]->SetLineColor(1+i);
    //   gScalerFwd[i]->SetMarkerColor(1+i);
    //   fMapFile->Add(gScalerFwd[i]);
    // }

    // gScalerCommonOr = new TGraph();
    // gScalerCommonOr->SetNameTitle("gScalerCommonOr","Trigger Scaler Counts");
    // gScalerCommonOr->SetLineWidth(2);
    // gScalerCommonOr->SetLineColor(6);
    // gScalerCommonOr->SetMarkerColor(6);
    // fMapFile->Add(gScalerCommonOr);
    // //
    // for(int i=0;i<2;i++){
    //   gScalerGeOverlap[i] = new TGraph();
    //   gScalerGeOverlap[i]->SetNameTitle(Form("gScalerGeOverlap_%d",i+1),Form("%s OverlapArea (5 strips): Scaler Counts",RecName[i+2].Data()));
    //   gScalerGeOverlap[i]->SetLineWidth(2);
    //   gScalerGeOverlap[i]->SetLineColor(1+i);
    //   gScalerGeOverlap[i]->SetMarkerColor(1+i);
    //   fMapFile->Add(gScalerGeOverlap[i]);
    // }
    // for(int i=0;i<2;i++){
    //   gScalerSiRear[i] = new TGraph();
    //   gScalerSiRear[i]->SetNameTitle(Form("gScalerSiRear_%d",i+1),Form("%s RearSide: Scaler Counts",RecName[i].Data()));
    //   gScalerSiRear[i]->SetLineWidth(2);
    //   gScalerSiRear[i]->SetLineColor(1+i);
    //   gScalerSiRear[i]->SetMarkerColor(1+i);
    //   fMapFile->Add(gScalerSiRear[i]);
    // }
    // //
    for(int i=0;i<4;i++){
      gHitRateRec[i] = new TGraph();
      gHitRateRec[i]->SetNameTitle(Form("gHitRateRec_%d",i+1),Form("%s : Hit Rate",RecName[i].Data()));
      gHitRateRec[i]->SetLineWidth(2);
      gHitRateRec[i]->SetLineColor(1+i);
      gHitRateRec[i]->SetMarkerColor(1+i);
      fMapFile->Add(gHitRateRec[i]);
    }
    for(int i=0;i<4;i++){
      gHitRateFwd[i] = new TGraph();
      gHitRateFwd[i]->SetNameTitle(Form("gHitRateFwd_%d",i+1),Form("%s : Hit Rate",FwdName[i].Data()));
      gHitRateFwd[i]->SetLineWidth(2);
      gHitRateFwd[i]->SetLineColor(1+i);
      gHitRateFwd[i]->SetMarkerColor(1+i);
      fMapFile->Add(gHitRateFwd[i]);
    }
    gHitRateCommonOr = new TGraph();
    gHitRateCommonOr->SetNameTitle("gHitRateCommonOr","Trigger Rate");
    gHitRateCommonOr->SetLineWidth(2);
    gHitRateCommonOr->SetLineColor(6);
    gHitRateCommonOr->SetMarkerColor(6);
    fMapFile->Add(gHitRateCommonOr);
    //
    for(int i=0;i<2;i++){
      gHitRateGeOverlap[i] = new TGraph();
      gHitRateGeOverlap[i]->SetNameTitle(Form("gHitRateGeOverlap_%d",i+1),Form("%s OverlapArea (5 strips): Hit Rate",RecName[i+2].Data()));
      gHitRateGeOverlap[i]->SetLineWidth(2);
      gHitRateGeOverlap[i]->SetLineColor(1+i);
      gHitRateGeOverlap[i]->SetMarkerColor(1+i);
      fMapFile->Add(gHitRateGeOverlap[i]);
    }
    for(int i=0;i<2;i++){
      gHitRateSiRear[i] = new TGraph();
      gHitRateSiRear[i]->SetNameTitle(Form("gHitRateSiRear_%d",i+1),Form("%s RearSide: Hit Rate",RecName[i].Data()));
      gHitRateSiRear[i]->SetLineWidth(2);
      gHitRateSiRear[i]->SetLineColor(1+i);
      gHitRateSiRear[i]->SetMarkerColor(1+i);
      fMapFile->Add(gHitRateSiRear[i]);
    }
    //
    // gEventNr->SetLineWidth(2);
    // gEventNr->SetLineColor(9);
    // gEventNr->SetMarkerColor(9);
    // fMapFile->Add(gEventNr);

    gEventRate = new TGraph();
    gEventRate->SetNameTitle("gEventRate","DAQ Event Rates");
    gEventRate->SetLineWidth(2);
    gEventRate->SetLineColor(9);
    gEventRate->SetMarkerColor(9);
    fMapFile->Add(gEventRate);

    gDaqEfficiency = new TGraph();
    gDaqEfficiency->SetNameTitle("gDaqEfficiency","DAQ Efficiency");
    gDaqEfficiency->SetLineWidth(2);
    gDaqEfficiency->SetLineColor(9);
    gDaqEfficiency->SetMarkerColor(9);
    fMapFile->Add(gDaqEfficiency);
  }
  //
  void
  KoaOnlineAnalyzer::FillScalerGraphs()
  {
    Int_t npoints;
    //
    for(int i=0;i<4;i++){
      // npoints=gScalerRec[i]->GetN();
      // gScalerRec[i]->SetPoint(npoints,fEmsTimeSecond,*fPScalerRec[i]);
      npoints=gHitRateRec[i]->GetN();
      gHitRateRec[i]->SetPoint(npoints,fEmsTimeSecond,*fPHitRateRec[i]);
    }
    //
    for(int i=0;i<4;i++){
      // npoints=gScalerFwd[i]->GetN();
      // gScalerFwd[i]->SetPoint(npoints,fEmsTimeSecond,*fPScalerFwd[i]);
      npoints=gHitRateFwd[i]->GetN();
      gHitRateFwd[i]->SetPoint(npoints,fEmsTimeSecond,*fPHitRateFwd[i]);
    }
    //
    // npoints=gScalerCommonOr->GetN();
    // gScalerCommonOr->SetPoint(npoints,fEmsTimeSecond,*fPScalerCommonOr);
    npoints=gHitRateCommonOr->GetN();
    gHitRateCommonOr->SetPoint(npoints,fEmsTimeSecond,*fPHitRateCommonOr);
    //
    for(int i=0;i<2;i++){
      // npoints=gScalerGeOverlap[i]->GetN();
      // gScalerGeOverlap[i]->SetPoint(npoints,fEmsTimeSecond,*fPScalerGeOverlap[i]);
      npoints=gHitRateGeOverlap[i]->GetN();
      gHitRateGeOverlap[i]->SetPoint(npoints,fEmsTimeSecond,*fPHitRateGeOverlap[i]);
    }
    //
    for(int i=0;i<2;i++){
      // npoints=gScalerSiRear[i]->GetN();
      // gScalerSiRear[i]->SetPoint(npoints,fEmsTimeSecond,*fPScalerSiRear[i]);
      npoints=gHitRateSiRear[i]->GetN();
      gHitRateSiRear[i]->SetPoint(npoints,fEmsTimeSecond,*fPHitRateSiRear[i]);
    }
    //
    // npoints=gEventNr->GetN();
    // gEventNr->SetPoint(npoints,fEmsTimeSecond,fEventNr);

    npoints=gEventRate->GetN();
    gEventRate->SetPoint(npoints,fEmsTimeSecond,*fPScalerEvtGate);

    npoints=gDaqEfficiency->GetN();
    gDaqEfficiency->SetPoint(npoints,fEmsTimeSecond,fDaqEfficiency);
  }
  //
  void
  KoaOnlineAnalyzer::ResetScalerGraphs()
  {
    Int_t npoints=gHitRateCommonOr->GetN();
    Int_t nr=fScalerResetInterval/fScalerUpdateInterval;
    if(npoints>nr){
      for(int i=0;i<4;i++){
        // gScalerRec[i]->Set(0);
        gHitRateRec[i]->Set(0);
      }
      for(int i=0;i<4;i++){
        // gScalerFwd[i]->Set(0);
        gHitRateFwd[i]->Set(0);
      }
      // gScalerCommonOr->Set(0);
      gHitRateCommonOr->Set(0);
      for(int i=0;i<2;i++){
        // gScalerGeOverlap[i]->Set(0);
        gHitRateGeOverlap[i]->Set(0);
      }
      for(int i=0;i<2;i++){
        // gScalerSiRear[i]->Set(0);
        gHitRateSiRear[i]->Set(0);
      }

      gEventRate->Set(0);
      // gEventNr->Set(0);
      gDaqEfficiency->Set(0);
    }
  }
  //
  void
  KoaOnlineAnalyzer::DeleteScalerGraphs()
  {
    for(int i=0;i<4;i++){
      // if(gScalerRec[i]) delete gScalerRec[i];
      if(gHitRateRec[i]) delete gHitRateRec[i];
    }
    for(int i=0;i<4;i++){
      // if(gScalerFwd[i]) delete gScalerFwd[i];
      if(gHitRateFwd[i]) delete gHitRateFwd[i];
    }
    // if(gScalerCommonOr) delete gScalerCommonOr;
    if(gHitRateCommonOr) delete gHitRateCommonOr;
    //
    for(int i=0;i<2;i++){
      // if(gScalerGeOverlap[i]) delete gScalerGeOverlap[i];
      if(gHitRateGeOverlap[i]) delete gHitRateGeOverlap[i];
    }
    for(int i=0;i<2;i++){
      // if(gScalerSiRear[i]) delete gScalerSiRear[i];
      if(gHitRateSiRear[i]) delete gHitRateSiRear[i];
    }

    // if(gEventNr) delete gEventNr;
    if(gEventRate) delete gEventRate;
    if(gDaqEfficiency) delete gDaqEfficiency;
  }
  //
  void
  KoaOnlineAnalyzer::SetMaxEvents(Int_t entries)
  {
    fMaxEvents = entries;
  }
}
