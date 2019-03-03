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
    KoaOnlineAnalyzer(const char* dir="/var/tmp",Long_t address=0x7fd3b6ad9000, Int_t size=1e6);
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

  private:
    TMapFile *fMapFile;
    Long_t    fMapAddress;
    Int_t     fMapSize;
    TString   fFileName;
    Long_t    fCounter;

    UChar_t *fModuleId;
    Char_t  *fResolution;
    Short_t   *fNrWords;
    Long64_t *fTimestamp;
    Int_t   (*fData)[34];
    // TTree   **fTree;
    TH1F     *hadc;
    TH1F     *hqdc;
    TH1F     *htdc;
  };
}
#endif
