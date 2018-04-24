/*
 * ems/events++/parse_koala.hxx
 * 
 * created 2015-Nov-25 PeWue
 * $ZEL: parse_koala.hxx,v 1.3 2016/02/04 12:40:08 wuestner Exp $
 */

// This file defines the data structure related to one single KOALA event
#ifndef _parse_koala_hxx_
#define _parse_koala_hxx_
#include "TH1F.h"
enum mesytypes {mesytec_madc32=0x21f10032UL, mesytec_mqdc32=0x22f10032UL,
        mesytec_mtdc32=0x23f10032UL};

struct mesymodules {
    uint32_t ID;//module id
    enum mesytypes mesytype;
};

// Hardware configuration
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

// data from EMS event
// each event has a timestamp and the scaler data
// the mesytec data are stored in another structure, they are not directly related to EMS events
struct event_data {
    // From data frame, event index of this ems event
    uint32_t event_nr; 
    bool evnr_valid;
    // From Timestamp subevent(or IS)
    struct timeval tv;
    bool tv_valid;
    // From Scalor subevent(or IS)
    uint32_t scaler[32];
    bool scaler_valid;
    // Beam counter?
    float bct[8];
    bool bct_valid;
};

// single (sub) event of a single module
struct mxdc32_event {
    void invalidate();
    struct mxdc32_event *next;
    uint32_t data[34]; // channel data including trigger channel
    uint32_t header; // header word
    uint32_t footer; // EOE word
    uint64_t ext_stamp; // extended timestamp word
    // uint32_t trigger;
    bool ext_stamp_valid; // whether extended timestamp is found in the data frame
    // int64_t ext_time;
    int64_t timestamp; // timestamp of this event
    uint64_t evnr; // koala event number of this event in this moudle, couted by parse_mxdc32
    int len; // number of following words in this event
};

// koala_event contains the module data corresponding to the same event, i.e. synchronized data.
// It also contains the ems event infomation corresponding to this KOALA event
struct koala_event {
    koala_event();
    ~koala_event();
    // koala_event *next;
    struct event_data event_data; // copy of the actual ems event
                                  // including scaler data
    struct mxdc32_event *events[nr_mesymodules];
    uint32_t mesypattern; // whether corresponding module has data in this event
};

// The following functions are to be invoked in parse_koala.cc
int use_koala_event(koala_event *koala);
int use_koala_event(koala_event *koala, TH1F* h);
void use_koala_init(void);
void use_koala_done(void);

#endif
