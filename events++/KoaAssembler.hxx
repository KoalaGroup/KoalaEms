#ifndef _koa_assembler_hxx_
#define _koa_assembler_hxx_

#include "ems_data.hxx"
#include "koala_data.hxx"
#include "mxdc32_data.hxx"
#include "KoaLoguru.hxx"

namespace DecodeUtil
{
  
  class KoaAssembler
  {
  public:
    KoaAssembler() : fKoalaPrivate(nullptr), fMxdc32Private(nullptr), fEmsPrivate(nullptr) {}
    virtual ~KoaAssembler() {}
    virtual int Init() {
      CHECK_NOTNULL_F(fKoalaPrivate,"Set Koala Event Private List first!");
      CHECK_NOTNULL_F(fEmsPrivate,"Set Ems Event Private List first!");
      CHECK_NOTNULL_F(fMxdc32Private,"Set Mxdc32 Event Private List first!");
    }
    virtual int Assemble();
    virtual int Done() {}
    virtual void Print() {}

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

  protected:
    static bool IsSameTS(int64_t fisrt, int64_t second);
    bool ToBeAssembled();
     
  protected:
    koala_private*  fKoalaPrivate;
    mxdc32_private* fMxdc32Private;
    ems_private*    fEmsPrivate; // TODO: EmsPrivate not used in this class
  };
}

#endif
