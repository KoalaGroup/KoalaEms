#include "KoaAssembler.hxx"
#include "KoaLoguru.hxx"
#include "global.hxx"

namespace DecodeUtil
{
  int
  KoaAssembler::Assemble()
  {
    CHECK_NOTNULL_F(fKoalaPrivate,"Set Koala Event Private List first!");
    CHECK_NOTNULL_F(fEmsPrivate,"Set Ems Event Private List first!");
    CHECK_NOTNULL_F(fMxdc32Private,"Set Mxdc32 Event Private List first!");
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
