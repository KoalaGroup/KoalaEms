#include "KoaTimestampAssembler.hxx"
#include "koala_data.hxx"

#define TS_RANGE 0x40000000

namespace DecodeUtil
{
  //
  int
  KoaTimestampAssembler::Assemble()
  {
    // if not empty, pop out the next event
    while(ToBeAssembled()){
      // Is there remaining modules to be checked from previous cluster
      if(!fToBeCheckedList.empty()){
        if(!ProcessUnsyncModules())
          break;
      }
      // check all modules sync or not
      if(IsSync()){
        StoreEvent();
      }
      else if(!ProcessUnsyncModules()){
        // if not, iterate every unsync module to find the next sync event
        break;
      }
    }

    return 0;
  }
  //
  bool
  KoaTimestampAssembler::IsSameTS(int64_t first, int64_t second)
  {
    int64_t delta_t = first - second;
    if(delta_t > TS_RANGE/2) delta_t-=TS_RANGE;
    if(delta_t < -TS_RANGE/2) delta_t+=TS_RANGE;

    if(delta_t>3 || delta_t<-3)
      return false;
    else
      return true;
  }
  //
  bool
  KoaTimestampAssembler::ToBeAssembled()
  {
    for (int mod=0; mod<nr_mesymodules; mod++) {
      if (fMxdc32Private[mod].is_empty()) return false;
    }
    return true;
  }
  //
  bool
  KoaTimestampAssembler::IsSync()
  {
    CHECK_F(fToBeCheckedList.empty(),"There are remaining modules to be processed from previous cluster");
    //
    fEventBuffer[fRefModuleIndex]=fMxdc32Private[fRefModuleIndex].drop_event();
    fCurrentTS[fRefModuleIndex]=fEventBuffer[fRefModuleIndex]->timestamp;
    for(int i=0;i<nr_mesymodules;i++){
      if(i!=fRefModuleIndex){
        fEventBuffer[i]=fMxdc32Private[i].drop_event();
        fCurrentTS[i]=fEventBuffer[i]->timestamp;
        if(!IsSameTS(fCurrentTS[i],fCurrentTS[fRefModuleIndex])){
          fToBeCheckedList.push_back(i);
          fEventBuffer[i]->recycle();
          fUnsyncStat[i]++;
        }
      }
    }
    return fToBeCheckedList.empty();
  }
  //
  void
  KoaTimestampAssembler::StoreEvent()
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
  KoaTimestampAssembler::ProcessUnsyncModules()
  {
    int index;
    bool found_new;
    // if not empty, pop out next event in the unsync module and check sync or not
    while(!fToBeCheckedList.empty()){
      index=fToBeCheckedList.back();
      found_new=false;
      //
      while(!fMxdc32Private[index].is_empty()){
        fEventBuffer[index]=fMxdc32Private[index].drop_event();
        fCurrentTS[index]=fEventBuffer[index]->timestamp;
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
      return false;
    }
    return true;
}
void
KoaTimestampAssembler::Print()
{
  printf("############################################\n");
  printf("Unsync events:\n");
  for(int i=0;i<nr_mesymodules;i++){
    printf("%ld\t",fUnsyncStat[i]);
  }
  printf("\n");
}
}
