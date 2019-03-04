#ifndef _koa_online_analyzer_hxx_
#define _koa_online_analyzer_hxx_

#include "KoaAnalyzer.hxx"
#include "global.hxx"
#include "TString.h"
#include "TTree.h"
#include "TH1F.h"
#include "TMapFile.h"

namespace DecodeUtil{
  class KoaOnlineAnalyzer: public KoaAnalyzer
  {
  public:
    KoaOnlineAnalyzer(const char* dir="/var/tmp",Long_t address=0x7fd3b6ad9000, Int_t size=1e8);
    virtual ~KoaOnlineAnalyzer();

    void SetDirectory(const char* dir);

    virtual int Init();
    virtual int Analyze();
    virtual int Done();

    void Reset();

  private:
    // void book_hist();
    // void delete_hist();
    void Decode(koala_event* koala);
    void InitMapping();
    void DetectorMapped();
    void InitTrees();
    void FillTrees();

  private:
    TMapFile *fMapFile;
    Long_t    fMapAddress;
    Int_t     fMapSize;
    TString   fFileName;
    Long_t    fCounter;

    // electronics mapped
    UChar_t *fModuleId;
    Char_t  *fResolution;
    Short_t   *fNrWords;
    Long64_t *fTimestamp;
    Int_t   (*fData)[34];
    // detector mapped
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
    
    // objects saved into map file
    TTree    *fSi1Tree;
    TTree    *fSi2Tree;
    TTree    *fGe1Tree;
    TTree    *fGe2Tree;
    TTree    *fFwdTree;

    TH1F     *hadc;
    TH1F     *hqdc;
    TH1F     *htdc;
  };
}
#endif
