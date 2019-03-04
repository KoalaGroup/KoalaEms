#ifndef _koa_raw_data_hxx_
#define _koa_raw_data_hxx_

#include "global.hxx"
#include "koala_data.hxx"
#include "TFile.h"
#include "TTree.h"

namespace DecodeUtil{
  class KoaRaw
  {
  public:
    KoaRaw() : fFile(nullptr) {}
    KoaRaw(TFile* file) : fFile(file) {}
    virtual ~KoaRaw();

    virtual void Setup(TFile* file) { fFile=file; }
    virtual void Init();
    virtual void Fill(koala_event* event);
    virtual void Done();

  private:
    void Decode(koala_event* event);

    virtual void InitImp() = 0;
    virtual void FillImp() = 0;
    virtual void DoneImp() = 0;

  public:
    TFile   *fFile;

    UChar_t *fModuleId;
    Char_t  *fResolution;
    Short_t   *fNrWords;
    Long64_t *fTimestamp;
    Int_t (*fData)[34];
  };

  //--------------------------------------------------------------//
  class KoaRawSimple : public KoaRaw
  {
  public:
    KoaRawSimple() {};
    KoaRawSimple(TFile* file) : KoaRaw(file) {};
    virtual ~KoaRawSimple();

  private:
    virtual void InitImp();
    virtual void FillImp();
    virtual void DoneImp();

  private:
    TTree   **fTree;
  };
}
#endif
