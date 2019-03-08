#ifndef _koa_raw_hxx_
#define _koa_raw_hxx_

#include "koala_data.hxx"
#include "ems_data.hxx"
#include "TFile.h"

namespace DecodeUtil{
  class KoaRaw
  {
  public:
    KoaRaw() : fFile(nullptr){}
    KoaRaw(TFile* file) : fFile(file){}
    virtual ~KoaRaw();

    virtual void Setup(TFile* file) { fFile=file; }
    virtual void Init();
    virtual void FillKoala(koala_event* event);
    virtual void FillEms(ems_event* event);
    virtual void Done();

  private:
    void DecodeKoala(koala_event* event);
    void DecodeEms(ems_event* event);

    virtual void InitImp() = 0;
    virtual void FillKoalaImp() = 0;
    virtual void FillEmsImp() = 0;
    virtual void DoneImp() = 0;

  public:
    TFile   *fFile;

    // Koala Event Data
    UChar_t *fModuleId;
    Char_t  *fResolution;
    Short_t   *fNrWords;
    Long64_t *fTimestamp;
    Int_t (*fData)[34];

    // EMS Event data
    // Scaler Data
    UInt_t  *fScaler;

    // Ems Timestamp
    Long_t  fEmsTimeSecond;
    Long_t  fEmsTimeUSecond;
    // UInt_t   fEventNr; [TODO]
  };
}
#endif
