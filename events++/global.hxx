#ifndef _global_hxx_
#define _global_hxx_
#include <cstdint>
enum mesytypes {mesytec_madc32=0x21f10032UL, mesytec_mqdc32=0x22f10032UL,
                mesytec_mtdc32=0x23f10032UL};

struct mesymodules {
  uint32_t ID;//module id
  enum mesytypes mesytype;
  char label[8];
};

// Hardware configuration
// TODO: move the configuration to the pre-build stage (i.e. configure stage)
static const struct mesymodules mesymodules[]= {
  {1, mesytec_madc32, "ADC1"},
  {2, mesytec_madc32, "ADC2"},
  {3, mesytec_madc32, "ADC3"},
  {4, mesytec_madc32, "ADC4"},
  {5, mesytec_madc32, "ADC5"},
  {6, mesytec_madc32, "ADC6"},
  {10, mesytec_mqdc32, "QDC"},
  {11, mesytec_mtdc32, "TDC1"},
  {12, mesytec_mtdc32, "TDC2"},
};

static const int nr_mesymodules=sizeof(mesymodules)/sizeof(struct mesymodules);

enum parser_types {
  parserty_time,
  parserty_scalor,
  parserty_mxdc32,
  parserty_invalid
};

struct EmsIsInfo {
  char name[32];// user defined name for this IS
  parser_types type;// type of the corresponding parser
  uint32_t is_id;//should match this IS's id in the wad file
};

#endif
