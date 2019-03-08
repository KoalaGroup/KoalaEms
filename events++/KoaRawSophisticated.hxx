#ifndef _koa_raw_sophisticated_hxx_
#define _koa_raw_sophisticated_hxx_

#include "KoaRaw.hxx"
#include "TTree.h"
#include "KoaRawEvent.hxx"
#include "KoaEmsEvent.hxx"

namespace DecodeUtil{
  class KoaRawSophisticated : public KoaRaw
  {
  public:
    KoaRawSophisticated(): fTreeKoaRaw(nullptr),
                    fKoaRawEvent(nullptr),
                    fEmsEvent(nullptr)
    {
    }
    KoaRawSophisticated(TFile* file): KoaRaw(file),
                               fTreeKoaRaw(nullptr),
                               fKoaRawEvent(nullptr),
                               fEmsEvent(nullptr)
    {
    }
    virtual ~KoaRawSophisticated();

  private:
    virtual void InitImp();
    virtual void FillKoalaImp();
    virtual void FillEmsImp();
    virtual void DoneImp();

  private:
    TTree   *fTreeKoaRaw;
    TTree   *fTreeEms;
    KoaRawEvent *fKoaRawEvent;
    KoaEmsEvent *fEmsEvent;
  };
}
#endif

