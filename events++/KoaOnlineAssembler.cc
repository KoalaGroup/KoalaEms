#include "KoaOnlineAssembler.hxx"
#include "koala_data.hxx"


namespace DecodeUtil
{
  //
  int
  KoaOnlineAssembler::Assemble()
  {
    // if not empty, pop out the next event
    while(ToBeAssembled()){
      // check all modules sync or not
      if(IsSync()){
        StoreEvent();
        LOG_F(INFO,"one more sync events: %d",fKoalaPrivate->statist.events);
      }
      else if(!ProcessUnsyncModules()){
        // if not, iterate every unsync module to find the next sync event
        LOG_F(INFO,"Unsync: end of this EMS Event");
        break;
      }
    }

    return 0;
  }
  //
  bool
  KoaOnlineAssembler::IsSync()
  {
    CHECK_F(fToBeCheckedList.empty(),"There are remaining modules to be processed from previous cluster");
    //
    fEventBuffer[fRefModuleIndex]=fMxdc32Private[fRefModuleIndex].drop_event();
    fCurrentTS[fRefModuleIndex]=fEventBuffer[fRefModuleIndex]->timestamp;
    if(mesymodules[fRefModuleIndex].mesytype == mesytec_mqdc32)
      fCurrentTS[fRefModuleIndex]-=fQdcTimeDiff;

    for(int i=0;i<nr_mesymodules;i++){
      if(i!=fRefModuleIndex){
        fEventBuffer[i]=fMxdc32Private[i].drop_event();
        fCurrentTS[i]=fEventBuffer[i]->timestamp;
        if(mesymodules[i].mesytype == mesytec_mqdc32)
          fCurrentTS[i]-=fQdcTimeDiff;
        if(!IsSameTS(fCurrentTS[i],fCurrentTS[fRefModuleIndex])){
          fToBeCheckedList.push_back(i);
          fEventBuffer[i]->recycle();
          fUnsyncStat[i]++;
        }
      }
      LOG_F(INFO,"module %d time: %ld",i,fCurrentTS[i]);
    }
    return fToBeCheckedList.empty();
  }
  //
  void
  KoaOnlineAssembler::StoreEvent()
  {
    koala_event *koala_cur=nullptr;
    koala_cur=fKoalaPrivate->prepare_event();
    for(int mod=0;mod<nr_mesymodules;mod++){
      koala_cur->mesypattern|=1<<mod;
      koala_cur->modules[mod]=fEventBuffer[mod];
    }
    fKoalaPrivate->store_event();
    fKoalaPrivate->statist.events++;
  }
  //
  bool
  KoaOnlineAssembler::ProcessUnsyncModules()
  {
    int index;
    bool found_new;
    bool offset=0;
    // if not empty, pop out next event in the unsync module and check sync or not
    while(!fToBeCheckedList.empty()){
      index=fToBeCheckedList.back();
      found_new=false;
      if(mesymodules[index].mesytype==mesytec_mqdc32)
        offset=fQdcTimeDiff;
      else
        offset=0;
      //
      while(!fMxdc32Private[index].is_empty()){
        fEventBuffer[index]=fMxdc32Private[index].drop_event();
        fCurrentTS[index]=fEventBuffer[index]->timestamp-offset;
        if(IsSameTS(fCurrentTS[fRefModuleIndex],fCurrentTS[index])){
          fToBeCheckedList.pop_back();
          found_new=true;
          break;
        }
        else{
          fEventBuffer[index]->recycle();
          fUnsyncStat[index]++;
        }
      }
      //
      if(!found_new)
        break;
    }
    //
    if(fToBeCheckedList.empty()){
      StoreEvent();
    }
    else{
      fToBeCheckedList.clear();
      return false;
    }
    return true;
  }
  //
void
KoaOnlineAssembler::Print()
{
  printf("############################################\n");
  printf("Unsync events:\n");
  for(int i=0;i<nr_mesymodules;i++){
    printf("%ld\t",fUnsyncStat[i]);
  }
  printf("\n");
}
}
