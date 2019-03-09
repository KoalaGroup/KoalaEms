#include "KoaAssembler.hxx"
#include "global.hxx"

#define TS_RANGE 0x40000000

namespace DecodeUtil
{
  int
  KoaAssembler::Assemble()
  {
    //
    bool notfull=false;
    mxdc32_event *event=nullptr;

    for (int mod=0; mod<nr_mesymodules; mod++) {
      if (fMxdc32Private[mod].is_empty()) {
        notfull=true;
      }
    }

    koala_event *koala_cur=nullptr;
    while(!notfull){
      koala_cur=fKoalaPrivate->prepare_event();
      for (int mod=0; mod<nr_mesymodules; mod++) {
        koala_cur->mesypattern|=1<<mod;
        koala_cur->modules[mod]=fMxdc32Private[mod].drop_event();
      }
      fKoalaPrivate->store_event();
      fKoalaPrivate->statist.events++;
      //
      for (int mod=0; mod<nr_mesymodules; mod++) {
        if (fMxdc32Private[mod].is_empty()) {
          notfull=true;
        }
      }
    }

    return 0;
  }

  //
  bool
  KoaAssembler::IsSameTS(int64_t first, int64_t second)
  {
    int64_t delta_t = first - second;
    if(delta_t > TS_RANGE/2) delta_t-=TS_RANGE;
    if(delta_t < -TS_RANGE/2) delta_t+=TS_RANGE;

    if(delta_t>3 || delta_t<-3){
      LOG_F(INFO,"%ld != %d",first,second);
      return false;
    }
    else
      return true;
  }
  //
  bool
  KoaAssembler::ToBeAssembled()
  {
    for (int mod=0; mod<nr_mesymodules; mod++) {
      if (fMxdc32Private[mod].is_empty()) return false;
    }
    return true;
  }
  //
}
