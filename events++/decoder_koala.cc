#include "decoder_koala.hxx"
#include "parser_koala.hxx"
#include "KoaLoguru.hxx"
#include "decode_util.hxx"
#include <iostream>
#include <cstdio>

using namespace std;

namespace DecodeUtil
{
  Decoder::Decoder(): fEmsPrivate(nullptr),
                      fMxdc32Private(nullptr),
                      fKoalaPrivate(nullptr),
                      fAssembler(nullptr),
                      fAnalyzer(nullptr)
  {
    
  }

  Decoder::~Decoder()
  {
    // delete assembler
    if(fAssembler)
      delete fAssembler;
    
    // delete analyzer
    if(fAnalyzer)
      delete fAnalyzer;

    // delete parsers
    for(auto it = fSubEvtParser.begin(); it != fSubEvtParser.end(); it++){
      delete it->second;
    }
  }

  void
  Decoder::SetPrivateList(private_list* list)
  {
    fEmsPrivate=&list->fEmsPrivate;
    fMxdc32Private=list->fMxdc32Private;
    fKoalaPrivate=&list->fKoalaPrivate;
  }

  void
  Decoder::SetParsers(EmsIsInfo* ISes)
  {
    //[TODO] need a Parser Factory?
    int i=0;
    
    while(ISes[i].type != parserty_invalid){
      LOG_S(INFO) << ISes[i].name << ISes[i].is_id;
      //
      switch(ISes[i].type){
      case parserty_time:
        fSubEvtParser.insert(std::pair<uint32_t, ParserVirtual*>(ISes[i].is_id, new ParserTime(fEmsPrivate, ISes[i].name, ISes[i].is_id)));
        break;
      case parserty_scalor:
        fSubEvtParser.insert(std::pair<uint32_t, ParserVirtual*>(ISes[i].is_id, new ParserScalor(fEmsPrivate, ISes[i].name, ISes[i].is_id)));
        break;
      case parserty_mxdc32:
        fSubEvtParser.insert(std::pair<uint32_t, ParserVirtual*>(ISes[i].is_id, new ParserMxdc32(fMxdc32Private, ISes[i].name, ISes[i].is_id)));
        break;
      default:
        LOG_S(WARNING)<< "Unknown Sub Event Parser";
      }
      //
      i++;
    }

    return;
  }

  void
  Decoder::SetAssembler(KoaAssembler* assembler)
  {
    CHECK_NOTNULL_F(assembler);
    //
    fAssembler = assembler;
    fAssembler->SetMxdc32Private(fMxdc32Private);
    fAssembler->SetKoalaPrivate(fKoalaPrivate);
    fAssembler->SetEmsPrivate(fEmsPrivate);
  }

  void
  Decoder::SetAnalyzer(KoaAnalyzer* analyzer)
  {
    CHECK_NOTNULL_F(analyzer);
    //
    fAnalyzer = analyzer;
    fAnalyzer->SetKoalaPrivate(fKoalaPrivate);
    fAnalyzer->SetEmsPrivate(fEmsPrivate);
  }

  int
  Decoder::DecodeCluster(const uint32_t *buf, int size)
  {
    int idx=0;
    clustertypes clustertype;

    idx+=2; // size and endiantest are already known
    if (check_size("cluster", idx, size, 1)<0)
        return -1;
    clustertype=static_cast<clustertypes>(buf[idx++]);
    switch (clustertype) {
    case clusterty_events:
        fStatistics.evclusters++;
        LOG_S(INFO)<<"cluster: events No."<< fStatistics.evclusters;
        if (parse_events(buf+idx, size-idx)<0)
            return -1;
        break;
    case clusterty_ved_info:
        // currently ignored
        LOG_S(INFO)<<"cluster: ved_info";
        break;
    case clusterty_text:
        // currently ignored
        LOG_S(INFO)<<"cluster: text";
        break;
    case clusterty_file:
        // currently ignored
        LOG_S(INFO)<<"cluster: file";
        break;
    case clusterty_no_more_data:
        LOG_S(INFO)<<"cluster: no more data";
        break;
    case clusterty_wendy_setup:  // no break
    case clusterty_async_data:   // no break
    case clusterty_async_data2:  // no break
    default:
        LOG_S(ERROR)<<"unknown or unhandled clustertype"<<clustertype<<endl;
        return -1;
    }

    return clustertype==clusterty_no_more_data?0:1;
  }

  //---------------------------------------------------------------------------//
  int
  Decoder::parse_options(const uint32_t *buf, int size)
  {
    int idx=0;
    int32_t optsize;

    if (check_size("options", idx, size, 1)<0)
      return -1;

    optsize=buf[idx++];
    if (check_size("options", idx, size, optsize)<0)
      return -1;

    return optsize+1;
  }

  //---------------------------------------------------------------------------//
  int
  Decoder::parse_events(const uint32_t *buf, int size)
  {
    int num, nr_events, idx=0;

    if ((num=parse_options(buf, idx+size))<0)
      return -1;
    idx+=num;

    // we need flags, VED_ID, fragment_id and number_of_events
    if (check_size("events", idx, size, 4)<0)
      return -1;
    // skip flags, VED_ID, fragment_id
    idx+=3;

    nr_events=buf[idx++];
    for (int i=0; i<nr_events; i++) {
      int ev_size;
      // we need the event size
      if (check_size("events", idx, size, 1)<0)
        return -1;
      ev_size=buf[idx++];
      // and we need ev_size words
      if (check_size("events", idx, size, ev_size)<0)
        return -1;

      if (parse_event(buf+idx, ev_size)<0)
        return -1;
      idx+=ev_size;
    }

    if (idx!=size) {
      LOG_S(ERROR)<<"parse_events: size="<<size<<", used words="<<idx;
      return -1;
    }

    fStatistics.events+=nr_events;
    return 0;
    
  }

  //---------------------------------------------------------------------------//
  int
  Decoder::parse_event(const uint32_t *buf, int size)
  {
    int nr_sev, idx=0;
    int res;

    ems_event* event_data=fEmsPrivate->prepare_event();
    event_data->evnr_valid=false;
    event_data->tv_valid=false;
    event_data->scaler_valid=false;
    event_data->bct_valid=false;

    // we need event_number, trigger id and number of subevents
    if (check_size("event", idx, size, 3)<0)
        return -1;
    // store event_number
    event_data->event_nr=buf[idx++];
    event_data->evnr_valid=true;

    // skip trigger id
    idx++;
    nr_sev=buf[idx++];
    for (int i=0; i<nr_sev; i++) {
        int sev_size;
        uint32_t sev_id;

        // we need instrumentation systen ID (==subevent ID) and the size
        if (check_size("event", idx, size, 2)<0)
            return -1;
        sev_id=buf[idx++];
        sev_size=buf[idx++];
        // and we need sev_size words
        if (check_size("event", idx, size, sev_size)<0)
            return -1;
        if (parse_subevent(buf+idx, sev_size, sev_id)<0)
            return -1;
        idx+=sev_size;
    }
    
    if (idx!=size) {
        LOG_S(ERROR)<<"parse_events: size="<<size<<", used words="<<idx;
        return -1;
    }

    fStatistics.subevents+=nr_sev;

    fEmsPrivate->store_event();

    if(fAssembler->Assemble()<0)
      return -1;

    res = fAnalyzer->Analyze();

    return res<0?-1:0;
  }

  //---------------------------------------------------------------------------//
  int
  Decoder::parse_subevent(const uint32_t *buf, int size, uint32_t sev_id)
  {
    auto it = fSubEvtParser.find(sev_id);
    if(it == fSubEvtParser.end()){
      ABORT_F("Unknown IS id %d, no corresponding parser defined",sev_id);
    }

    ParserVirtual* parser=it->second;
    if(parser->Parse(buf,size)<0)
      return -1;

    return 0;
  }
}
