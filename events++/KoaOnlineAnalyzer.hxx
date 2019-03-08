#ifndef _koa_online_analyzer_hxx_
#define _koa_online_analyzer_hxx_

#include "KoaAnalyzer.hxx"
#include "global.hxx"
#include "TString.h"
#include "TTree.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TMapFile.h"
#include "TGraph.h"
#include <ctime>

namespace DecodeUtil{
  class KoaOnlineAnalyzer: public KoaAnalyzer
  {
  public:
    KoaOnlineAnalyzer(const char* mapdir="/var/tmp",Long_t address=0x7fd3b6ad9000, Int_t size=1e9, Int_t reset_events=100000, Int_t circular_event=5e6);
    virtual ~KoaOnlineAnalyzer();

    void SetDirectory(const char* dir);
    void SetMapAddress(Long_t address);
    void SetMapSize(Int_t size);
    void SetMaxEvents(Int_t entries);
    void SetCircularSize(Int_t circular_event);
    void SetScalerUpdateInterval(Int_t second);
    void SetScalerResetInterval(Int_t second);

    virtual int Init();
    virtual int Analyze();
    virtual int Done();

    void Reset();

  private:
    void Decode(koala_event* koala);
    void InitMapping();

    void InitTrees();
    void FillTrees();

    void DetectorMapped();

    void InitHistograms();
    void FillHistograms();
    void ResetHistograms();
    void DeleteHistograms();

    void InitScalerGraphs();
    void FillScalerGraphs();
    void ResetScalerGraphs();
    void DeleteScalerGraphs();

  private:
    TMapFile *fMapFile;
    Long_t    fMapAddress;
    Int_t     fMapSize;
    TString   fMapFileName;
    Long_t    fKoalaCounter;
    Long_t    fEmsCounter;
    Int_t     fMaxEvents;

    TFile    *fRootFile;
    TString   fRootFileName;
    Int_t     fCircularSize;
    TTree    *fTree;

    // electronics mapped
    UChar_t *fModuleId;
    Char_t  *fResolution;
    Short_t   *fNrWords;
    Long64_t *fTimestamp;
    Int_t   (*fData)[34];
    // detector mapped
    Int_t    *fPRecRear_Amplitude[4];
    Int_t    *fPRecRear_Timestamp[2];
    Int_t     *fPSi1_Amplitude[48];
    Int_t   *fPSi1_Timestamp[48];
    Int_t     *fPSi2_Amplitude[64];
    Int_t   *fPSi2_Timestamp[64];
    Int_t     *fPGe1_Amplitude[32];
    Int_t   *fPGe1_Timestamp[32];
    Int_t     *fPGe2_Amplitude[32];
    Int_t   *fPGe2_Timestamp[32];
    Int_t     *fPFwd_Amplitude[8];
    Int_t   *fPFwd_Timestamp[8];

    Int_t     fRecRear_Amplitude[4];
    Float_t   fRecRear_Timestamp[2];
    Int_t     fSi1_Amplitude[48];
    Float_t   fSi1_Timestamp[48];// unit: ns
    Int_t     fSi2_Amplitude[64];
    Float_t   fSi2_Timestamp[64];// unit: ns
    Int_t     fGe1_Amplitude[32];
    Float_t   fGe1_Timestamp[32];// unit: ns
    Int_t     fGe2_Amplitude[32];
    Float_t   fGe2_Timestamp[32];// unit: ns
    Int_t     fFwd_Amplitude[8];
    Float_t   fFwd_Timestamp[8];// unit: ns
    Float_t   fTrig_Timestamp[2];
    
    // scaler
    UInt_t  *fScaler;
    UInt_t  *fScalerDiff;
    Double_t *fHitRate;
    time_t  fEmsTimeSecond;
    Long_t  fEmsTimeUSecond;
    Int_t   fScalerUpdateInterval;// unit: s
    Int_t   fScalerResetInterval;// unit:s

    UInt_t  fEventNr;
    Double_t fEventRate;
    Double_t fDaqEfficiency;

    UInt_t  *fPScalerRec[4];// 0-->Si#1, 2-->Si#2, 3->Ge#1, 4-->Ge#2
    UInt_t  *fPScalerFwd[4];// 0-->1&2, 1-->3&4, 2-->5&6, 3-->7&8
    UInt_t  *fPScalerCommonOr;
    UInt_t  *fPScalerGeOverlap[2];
    UInt_t  *fPScalerSiRear[2];

    Double_t *fPHitRateRec[4];// 0-->Si#1, 2-->Si#2, 3->Ge#1, 4-->Ge#2
    Double_t  *fPHitRateFwd[4];// 0-->1&2, 1-->3&4, 2-->5&6, 3-->7&8
    Double_t  *fPHitRateCommonOr;
    Double_t  *fPHitRateGeOverlap[2];
    Double_t  *fPHitRateSiRear[2];

    // TGraph  *gScalerRec[4];
    // TGraph  *gScalerFwd[4];
    // TGraph  *gScalerCommonOr;
    // TGraph  *gScalerGeOverlap[2];
    // TGraph  *gScalerSiRear[2];

    TGraph  *gHitRateRec[4];
    TGraph  *gHitRateFwd[4];
    TGraph  *gHitRateCommonOr;
    TGraph  *gHitRateGeOverlap[2];
    TGraph  *gHitRateSiRear[2];

    // TGraph  *gEventNr;
    TGraph  *gEventRate;
    TGraph  *gDaqEfficiency;

    TH1F*     hTrigTime[2];
    TH1F*     hFwdAmp[8];
    TH1F*     hFwdTime[8];
    // TH1F*     hSi1Time[32];
    // TH1F*     hSi2Time[24];
    TH1F*     hRecTime;
    TH1F*     hRecRearTime[2];

    TH2F*     h2RecHits[4];
    TH2F*     h2RecHitsCut[4];
    TH1F*     hRecRearAmp[4];
    TH1F*     hRecRearAmpCut[4];
    //
    TH2F*     h2FwdTimeVSAmp[2];
    TH2F*     h2FwdTimeVSRecAmp[2];
    TH2F*     h2FwdAmpVSRecAmp[2];
    TH1F*     hRecRearTimeDiff[2];
    // TH2F*     h2Si1RearAmpVsTime;
  };
}
#endif
