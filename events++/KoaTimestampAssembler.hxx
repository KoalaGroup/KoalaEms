#ifndef _koa_timestamp_assembler_hxx_
#define _koa_timestamp_assembler_hxx_

#include "KoaAssembler.hxx"
#include "KoaLoguru.hxx"
#include <vector>

namespace DecodeUtil
{
  class KoaTimestampAssembler : public KoaAssembler
  {
  public:
    KoaTimestampAssembler(): fRefModuleIndex(0),
                             fQdcMaxDiff(3),
                             fCheckRange(3)
    {
      fCurrentTS = new int64_t[nr_mesymodules];
      fEventBuffer = new mxdc32_event*[nr_mesymodules];
    }

    virtual ~KoaTimestampAssembler()
    {
      if(fCurrentTS) delete fCurrentTS;
    }

    void SetRefModuleIndex(int index){
      CHECK_F(fRefModuleIndex=index < nr_mesymodules,"Module Index not of range! There are only %d modules in the DAQ.", nr_mesymodules);

      if(mesymodules[fRefModuleIndex].mesytype == mesytec_mtdc32)
        LOG_F(ERROR,"TDC is not allowed to be a reference module");
    }
    void SetQdcMaxDiff(int diff){
      fQdcMaxDiff=diff;
    }

  public:
    int  Assemble();

  private:
    bool  IsSameTS(int64_t first, int64_t second);
    bool  ToBeAssembled(); // check whether all the modules have events to be processed
    bool  IsSync();
    void  StoreEvent();
    bool  ProcessUnsyncModules();

  private:
    int   fRefModuleIndex; // the reference module as starting point
                           // normally, it should be the first ADC
    int   fQdcMaxDiff; // the width of QDC gate in 62.5ns
    int   fCheckRange; // the allowed max events loss when unsynchronization first occurs
    int64_t  *fCurrentTS;
    std::vector<int>  fToBeCheckedList;
    mxdc32_event **fEventBuffer;

  };
}

#endif
