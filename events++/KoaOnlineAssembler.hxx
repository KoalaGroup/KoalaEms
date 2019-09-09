#ifndef _koa_online_assembler_hxx_
#define _koa_online_assembler_hxx_

#include "KoaAssembler.hxx"
#include "KoaLoguru.hxx"
#include <vector>

namespace DecodeUtil
{
  class KoaOnlineAssembler : public KoaAssembler
  {
  public:
    KoaOnlineAssembler(): fCheckRange(3)
    {
      SetRefModuleIndex(0);
      fCurrentTS = new int64_t[nr_mesymodules];
      fEventBuffer = new mxdc32_event*[nr_mesymodules];
      fUnsyncStat = new uint32_t[nr_mesymodules]();
    }

    virtual ~KoaOnlineAssembler()
    {
      if(fCurrentTS) delete fCurrentTS;
      if(fUnsyncStat) delete fUnsyncStat;
    }

    void SetRefModuleIndex(int index){
      fRefModuleIndex = index;
      CHECK_F(index < nr_mesymodules,"Module Index not of range! There are only %d modules in the DAQ.", nr_mesymodules);

      if(mesymodules[fRefModuleIndex].mesytype == mesytec_mtdc32)
        LOG_F(ERROR,"TDC is not allowed to be a reference module");
    }

  public:
    int  Assemble();
    void Print();

  private:
    bool  IsSync();
    void  StoreEvent();
    bool  ProcessUnsyncModules();

  private:
    int   fRefModuleIndex; // the reference module as starting point
                           // normally, it should be the first ADC
    int   fCheckRange; // the allowed max events loss when unsynchronization first occurs
    int64_t  *fCurrentTS;
    std::vector<int>  fToBeCheckedList;
    mxdc32_event **fEventBuffer;
    uint32_t *fUnsyncStat;
  };
}

#endif
