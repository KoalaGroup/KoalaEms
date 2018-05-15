#ifndef _global_hxx_
#define _global_hxx_

enum mesytypes {mesytec_madc32=0x21f10032UL, mesytec_mqdc32=0x22f10032UL,
                mesytec_mtdc32=0x23f10032UL};

struct mesymodules {
  uint32_t ID;//module id
  enum mesytypes mesytype;
};

// Hardware configuration
// TODO: move the configuration to the pre-build stage (i.e. configure stage)
static const struct mesymodules mesymodules[]= {
  {1, mesytec_madc32},
  {2, mesytec_madc32},
  {3, mesytec_madc32},
  {4, mesytec_madc32},
  {5, mesytec_madc32},
  {6, mesytec_madc32},
  //{7, mesytec_mqdc32},
  //{8, mesytec_mtdc32},
};
static const int nr_mesymodules=sizeof(mesymodules)/sizeof(struct mesymodules);

#endif
