/*
 * ems/events++/strawtest_data.hxx
 * 
 * created 2014-Nov-03 PW
 */

#ifndef _strawtest_data_hxx_
#define _strawtest_data_hxx_

#include "config.h"
#include <cstdint>
//#include <unistd.h>
#include "vtainer.hxx"

struct sampledata {
    bool overflow;
    int32_t value;
};

struct noisedata {
    bool valid;
    bool working;
    int max;
    int tau;
    void clear(void) {valid=false; max=0; tau=0; working=false;}
};

struct scalerdata {
    bool valid;
    bool half;
    int32_t value;
    void clear(void) {half=false; valid=false;}
};

struct timeampdata {
    bool valid;
    bool half;
    int bits; // bit 15, 14, 13
    int32_t time;
    int32_t ampl;
    void clear(void) {half=false; valid=false;}
};

struct iwampldata {
    bool valid;
    bool half;
    int32_t start;
    int32_t end;
    void clear(void) {half=false; valid=false;}
};

struct pulsedata {
    bool end_amplA_valid;
    bool end_amplB_valid;
    int32_t end_amplA;
    int32_t end_amplB;
    void clear(void) {
        end_amplA_valid=end_amplB_valid=false;
    }
};

struct miscdata {
    bool overruns_valid;
    int overruns;
    bool gradient_valid;
    int gradient;
    bool cross_over_level_valid;
    int cross_over_level;
    bool cross_under_level_valid;
    int cross_under_level;
    void clear(void) {
        overruns_valid=false;
        gradient_valid=false;
        cross_over_level_valid=false;
        cross_under_level=false;
    }
};

struct channel_data {
    bool fired;
    vtainer<u_int32_t> rawdata;
    vtainer<sampledata> samples;
    scalerdata scaler;
    noisedata noise;
    timeampdata iwstart;
    iwampldata iwampl;
    timeampdata iwmax;
    timeampdata iwcenter;
    timeampdata swstart;
    pulsedata pulse;
    miscdata misc;

    void clear(void) {
        fired=false;
        rawdata.clear();
        samples.clear();
        scaler.clear();
        noise.clear();
        iwstart.clear();
        pulse.clear();
        misc.clear();
    }
};

#endif
