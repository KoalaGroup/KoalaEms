#ifndef _koa_analyzer_hxx_
#define _koa_analyzer_hxx_

#include "ems_data.hxx"
#include "koala_data.hxx"
#include "KoaRawData.hxx"
#include "TFile.h"
#include "TH1F.h"
#include "global.hxx"

namespace DecodeUtil{
  class KoaAnalyzer
  {
  public:
    KoaAnalyzer(): fKoalaPrivate(nullptr),
                   fEmsPrivate(nullptr)
    {}
    virtual ~KoaAnalyzer() {}
    virtual int Analyze();

    void SetKoalaPrivate(koala_private* pri)
    {
      fKoalaPrivate = pri;
    }
    void SetEmsPrivate(ems_private* pri)
    {
      fEmsPrivate = pri;
    }

  private:
    void RecycleEmsEvents();
    void RecycleKoalaEvents();

  private:
    koala_private* fKoalaPrivate;
    ems_private*   fEmsPrivate;
  };

  //
  class KoaSimpleAnalyzer: public KoaAnalyzer
  {
  public:
    KoaSimpleAnalyzer(const char* outputfile="koala_raw.root", bool use_simplestructure=true,  int max_diff=0);

    void SetOutputFile(const char* outputfile);
    void SetSimpleStructure(bool);
    void SetMaxDiff(int);

    void Init();
    virtual int Analyze();

  private:
    void book_hist();

  private:
    char outputbase[1024];
    char rootfile[1024];
    char pdffile[1024];
    int  max_tsdiff;

    TFile* rfile;
    TH1F* h_timediff[nr_mesymodules];
    TH1F* hwords[nr_mesymodules];
    KoaRawSimple* koala_raw;
  };
}
#endif
