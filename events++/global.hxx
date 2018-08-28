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
  // {1, mesytec_madc32, "ADC1"},
  // {2, mesytec_madc32, "ADC2"},
  // {3, mesytec_madc32, "ADC3"},
  // {4, mesytec_madc32, "ADC4"},
  //{5, mesytec_madc32, "ADC5"},
  // {6, mesytec_madc32, "ADC6"},
  {9, mesytec_madc32, "ADC"},
  {10, mesytec_mqdc32, "QDC"},
  {11, mesytec_mtdc32, "TDC"},
};
static const int nr_mesymodules=sizeof(mesymodules)/sizeof(struct mesymodules);

#endif
