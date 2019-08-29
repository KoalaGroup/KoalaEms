#ifndef _koa_raw_data_hxx_
#define _koa_raw_data_hxx_
#include "global.hxx"
#include "TTree.h"
#include "KoaRaw.hxx"

namespace DecodeUtil{
  class KoaRawSimple : public KoaRaw
  {
  public:
    KoaRawSimple() {};
    KoaRawSimple(TFile* file) : KoaRaw(file) {};
    virtual ~KoaRawSimple();

  private:
    virtual void InitImp();
    virtual void FillKoalaImp();
    virtual void FillEmsImp();
    virtual void DoneImp();

  private:
    TTree   **fTreeKoala;
    TTree   *fTreeEms;
  };
}

#endif
