#ifndef _koa_analyzer_hxx_
#define _koa_analyzer_hxx_

#include "ems_data.hxx"
#include "koala_data.hxx"
#include "global.hxx"

namespace DecodeUtil{
  class KoaAnalyzer
  {
  public:
    KoaAnalyzer(): fKoalaPrivate(nullptr),
                   fEmsPrivate(nullptr)
    {}
    virtual ~KoaAnalyzer() {}
    virtual int Init() {}
    virtual int Analyze();
    virtual int Done() {}
    virtual void Print() {};

    void SetKoalaPrivate(koala_private* pri)
    {
      fKoalaPrivate = pri;
    }
    void SetEmsPrivate(ems_private* pri)
    {
      fEmsPrivate = pri;
    }

  protected:
    void RecycleEmsEvents();
    void RecycleKoalaEvents();

  protected:
    koala_private* fKoalaPrivate;
    ems_private*   fEmsPrivate;
  };
}
#endif
