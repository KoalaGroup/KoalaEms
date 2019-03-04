#ifndef _koa_decoder_hxx_
#define _koa_decoder_hxx_

#include "mxdc32_data.hxx"
#include "ems_data.hxx"
#include "koala_data.hxx"
#include "global.hxx"
#include "KoaAssembler.hxx"
#include "KoaAnalyzer.hxx"
#include "KoaParsers.hxx"
#include <map>

namespace DecodeUtil
{
  enum clustertypes {
    clusterty_events=0,
    clusterty_ved_info=1,
    clusterty_text=2,
    clusterty_wendy_setup=3,
    clusterty_file=4,
    clusterty_async_data=5,
    clusterty_async_data2=6, /* includes mqtt */
    clusterty_no_more_data=0x10000000,
  };

  // statistics
  struct global_statist {
    global_statist(): evclusters(0), events(0), subevents(0) {}
    uint32_t evclusters; // number of event cluster parsed
    uint32_t events;     // number of ems events parsed(==multievents for mxdc32)
    uint32_t subevents;  // number of ems subevents paresd totally(=events*ISNumber)
    // uint32_t koala_events;
    // uint32_t complete_events;
    // uint32_t without_qdc;
    // uint32_t only_dqc;
  };

  // private_list is just a collection of all used private lists used in decoding.
  // the object should be new & delete by the user himself before the decoding, and passed to Decoder
  struct private_list{
    ems_private  fEmsPrivate;
    mxdc32_private fMxdc32Private[nr_mesymodules];
    koala_private  fKoalaPrivate;
  };

  class Decoder
  {
  public:
    Decoder();
    ~Decoder();

    void SetPrivateList(private_list* list);
    void SetParsers(EmsIsInfo *ISes);
    void SetAssembler(KoaAssembler* assembler);
    void SetAnalyzer(KoaAnalyzer* analyzer);

    int  DecodeCluster(const uint32_t *buf, int size);
    int  Init();
    int  Done();
    int  Print();

  private:
    // util functions
    int parse_options(const uint32_t *buf, int size);
    int parse_events(const uint32_t *buf, int size);
    int parse_event(const uint32_t *buf, int size);
    int parse_subevent(const uint32_t *buf, int size, uint32_t sev_id);

  private:
    ems_private  *fEmsPrivate;
    mxdc32_private *fMxdc32Private;
    koala_private  *fKoalaPrivate;
    global_statist fStatistics;

    KoaAssembler* fAssembler;
    KoaAnalyzer*  fAnalyzer;
    std::map<uint32_t,ParserVirtual*>      fSubEvtParser;
  };
}

#endif
