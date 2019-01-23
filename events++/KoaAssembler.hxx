#ifndef _koa_assembler_hxx_
#define _koa_assembler_hxx_

#include "ems_data.hxx"
#include "koala_data.hxx"
#include "mxdc32_data.hxx"

namespace DecodeUtil
{
  
  class KoaAssembler
  {
  public:
    virtual ~KoaAssembler() {}
    virtual int Assemble();

    void SetMxdc32Private(mxdc32_private* pri)
    {
      fMxdc32Private = pri;
    }

    void SetKoalaPrivate(koala_private* pri)
    {
      fKoalaPrivate = pri;
    }
    void SetEmsPrivate(ems_private* pri)
    {
      fEmsPrivate = pri;
    }

  private:
    koala_private*  fKoalaPrivate;
    mxdc32_private* fMxdc32Private;
    ems_private*    fEmsPrivate;
  };
}

#endif
