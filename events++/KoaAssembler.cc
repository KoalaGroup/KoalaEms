#include "KoaAssembler.hxx"
#include "global.hxx"

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
}
