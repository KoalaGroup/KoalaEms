#include "decode_util.hxx"
#include "KoaLoguru.hxx"
#include <algorithm>

namespace DecodeUtil
{
  //---------------------------------------------------------------------------//
  int
  mxdc32_get_idx_by_id(uint32_t id)
  {
    int mod;

    for (mod=0; mod<nr_mesymodules; mod++) {
      if (mesymodules[mod].ID==id) {
        return mod;
      }
    }

    LOG_S(ERROR)<<"unrecognized module (ID="<<id<<")";
    return -1;
  }

  //---------------------------------------------------------------------------//
  int
  check_size(const char* txt, int idx, int size, int needed)
  {
    if (size>=idx+needed)
      return 0;

    LOG_S(ERROR)<<txt<<" at idx "<<idx<<" too short: "<<needed<<" words needed, " <<size-idx<<" available";
    return -1;
  }

  //---------------------------------------------------------------------------//
  void
  dump_data(const char* txt, const uint32_t *buf, int size, int idx)
  {
    printf("data dump: %s\n", txt);

    for (int i=std::max(idx-3, 0); i<size; i++) {
      printf("%c[%5x] %08x\n", i==idx?'>':' ', i, buf[i]);
    }
    printf("\n");
  }

  //---------------------------------------------------------------------------//
}
