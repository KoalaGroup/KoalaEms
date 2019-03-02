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

  //
  class KoaSimpleAnalyzer: public KoaAnalyzer
  {
  public:
    KoaSimpleAnalyzer(const char* outputfile="koala_raw.root", bool use_simplestructure=true,  int max_diff=0);
    virtual ~KoaSimpleAnalyzer();

    void SetOutputFile(const char* outputfile);
    void SetSimpleStructure(bool);
    void SetMaxDiff(int);

    virtual int Init();
    virtual int Analyze();
    virtual int Done();
    virtual void Print();

  private:
    void book_hist();
    void delete_hist();
    void fill_hist(koala_event* koala);
    void draw_hist();
    void save_hist();
    void check_timestamp(koala_event* koala);

    struct timestamp_statist{
      uint32_t equal_ev[nr_mesymodules];
      uint32_t plusone_ev[nr_mesymodules];
      uint32_t minusone_ev[nr_mesymodules];
      uint32_t plustwo_ev[nr_mesymodules];
      uint32_t minustwo_ev[nr_mesymodules];
      uint32_t unsync_ev[nr_mesymodules];
    };

  private:
    char outputbase[1024];
    char rootfile[1024];
    char pdffile[1024];
    int  max_tsdiff;

    TFile* rfile;
    TH1F* h_timediff[nr_mesymodules];
    TH1F* hwords[nr_mesymodules];
    KoaRawSimple* koala_raw;
    timestamp_statist timestamp_statist;
  };
}
#endif
