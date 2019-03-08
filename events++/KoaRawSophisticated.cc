
#include "KoaRawSophisticated.hxx"

static Float_t g_invalidtime=-10;
static Float_t time_resolution[9]={-1,1./256,2./256,4./256,8./256,16./256,32./256,64./256,128./256};

namespace DecodeUtil
{
  //---------------------------------------------------------------------------//
  KoaRawSophisticated::~KoaRawSophisticated()
  {
    if(fKoaRawEvent) delete fKoaRawEvent;
    if(fEmsEvent) delete fEmsEvent;
    if(fTreeKoaRaw) delete fTreeKoaRaw;
    if(fTreeEms) delete fTreeEms;
  }

  void KoaRawSophisticated::InitImp()
  {
    fKoaRawEvent = new KoaRawEvent();
    fTreeKoaRaw=new TTree("KoalaEvent","Koala Mapped Raw Data");
    fTreeKoaRaw->Branch("KoalaEvent",&fKoaRawEvent);
    
    fEmsEvent = new KoaEmsEvent();
    fTreeEms=new TTree("EmsEvent","Ems Event Data");
    fTreeKoaRaw->Branch("EmsEvent",&fEmsEvent);
  }

  void KoaRawSophisticated::FillKoalaImp()
  {
    // Amplitudes
    // Si1 : Physical Position 48->17 ===> ADC1 1->32
    for(int i=0;i<32;i++){
      fKoaRawEvent->fSi1Amplitude[47-i]=fData[0][i];
    }
    // Si1 : Physical Position 16->1 ===> ADC2 1->16
    for(int i=0;i<16;i++){
      fKoaRawEvent->fSi1Amplitude[15-i]=fData[1][i];
    }
    // Si2 : Physical Position 1->32 ===> ADC3 1->32
    for(int i=0;i<32;i++){
      fKoaRawEvent->fSi2Amplitude[i]=fData[2][i];
    }
    // Si2 : Physical Position 33->64 ===> ADC4 1->32
    for(int i=0;i<32;i++){
      fKoaRawEvent->fSi2Amplitude[32+i]=fData[3][i];
    }
    // Ge1 : Physical Position 1->32 ===> ADC5 32->1
    for(int i=0;i<32;i++){
      fKoaRawEvent->fGe1Amplitude[31-i]=fData[4][i];
    }
    // Ge2 : Physical Position 1->32 ===> ADC6 1->32
    for(int i=0;i<32;i++){
      fKoaRawEvent->fGe2Amplitude[i]=fData[5][i];
    }
    // Recoil Detector Rear Side:
    // Si#1 Rear: ADC2 17
    // Si#2 Rear: ADC2 18
    // Ge#1 Rear: ADC2 19
    // Ge#2 Rear: ADC2 20
    for(int i=0;i<4;i++){
      fKoaRawEvent->fRecRearAmplitude[i]=fData[1][16+i];
    }
    // Fwd: 1->8 ===> QDC 1->8
    for(int i=0;i<8;i++){
      fKoaRawEvent->fFwdAmplitude[i]=fData[6][i];
    }

    /////////////////////
    // Time: ns
    for(int i=0;i<48;i++){
      fKoaRawEvent->fSi1Time[i]=g_invalidtime;
    }
    for(int i=0;i<64;i++){
      fKoaRawEvent->fSi2Time[i]=g_invalidtime;
    }
    for(int i=0;i<32;i++){
      fKoaRawEvent->fGe1Time[i]=g_invalidtime;
      fKoaRawEvent->fGe2Time[i]=g_invalidtime;
    }
    // Si1 : Physical Position 48->17 ===> TDC2 1->32
    for(int i=0;i<32;i++){
      fKoaRawEvent->fSi1Time[47-i]=fData[8][i]*time_resolution[fResolution[8]-1];
    }
    // Si2 : Physical Position 1->16 ===> TDC1 17->32
    for(int i=0;i<16;i++){
      fKoaRawEvent->fSi2Time[i]=fData[7][16+i]*time_resolution[fResolution[7]-1];
    }
    // Si2 : Physical Position 17->22 ===> TDC1 11->16
    for(int i=0;i<6;i++){
      fKoaRawEvent->fSi2Time[16+i]=fData[7][10+i]*time_resolution[fResolution[7]-1];
    }
    // Si1 Rear Time : TDC1 9
    // Si2 Rear Time : TDC1 10
    for(int i=0;i<2;i++){
      fKoaRawEvent->fRecRearTime[i]=fData[7][8+i]*time_resolution[fResolution[7]-1];
    }
    // Fwd: 1-8 ===> TDC1 1->8
    for(int i=0;i<8;i++){
      fKoaRawEvent->fFwdTime[i]=fData[7][i];
    }
    //
    fKoaRawEvent->fTriggerTime[0]=(fData[7][32]*time_resolution[fResolution[7]-1]);
    fKoaRawEvent->fTriggerTime[1]=(fData[8][32]*time_resolution[fResolution[8]-1]);

    //
    fTreeKoaRaw->Fill();
  }

  void KoaRawSophisticated::FillEmsImp()
  {
    // Timestamp
    fEmsEvent->fSecond=fEmsTimeSecond;
    fEmsEvent->fUSecond=fEmsTimeUSecond;

    // Recoil Detector 
    for(int i=0;i<4;i++){
      fEmsEvent->fScalerRec[i]=fScaler[i];
    }
    // Fwd Detector
    for(int i=0;i<4;i++){
      fEmsEvent->fScalerFwd[i]=fScaler[i+8];
    }
    // Common OR
    fEmsEvent->fScalerCommonOr=fScaler[4];
    // Ge1 and Ge2 Overlapping area (5 strips)
    for(int i=0;i<2;i++){
      fEmsEvent->fScalerGeOverlap[i]=fScaler[6+i];
    }
    // Si1 Si2 Rear Side
    for(int i=0;i<2;i++){
      fEmsEvent->fScalerSiRear[i]=fScaler[24+i];
    }
    // Fwd 8 Channels
    for(int i=0;i<8;i++){
      fEmsEvent->fScalerFwdCh[i]=fScaler[16+i];
    }

    //
    fTreeEms->Fill();
  }

  void KoaRawSophisticated::DoneImp()
  {
    fTreeKoaRaw->Write();
    fEmsEvent->Write();
  }
}
