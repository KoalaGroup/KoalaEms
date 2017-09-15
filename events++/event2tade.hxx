/*
 * ems/events++/cluster2tade.hxx
 * 
 * created 2004-11-30 PW
 */

#ifndef _cluster2tade_hxx_
#define _cluster2tade_hxx_

#include "config.h"
#include "cxxcompat.hxx"
#include "ems_objects.hxx"

/*
unbekannte Zuordnung:
t02
buffercheck  50
LC_ADC_1881M 32
PH_10CX      33
LC_TDC_1877  34
LC_ADC_1875A 35
TrigData     1002

t03
buffercheck  51
LC_ADC_1881M 20
PH_10CX      21
LC_TDC_1877  27
LC_ADC_1875A 28
TrigData     1003

t04
buffercheck  52
LC_ADC_1881M 22
PH_10CX      23
LC_TDC_1877  24
LC_ADC_1875A 30
TrigData     1004

t06
buffercheck  53
LC_ADC_1881M 26
PH_10CX      25
LC_TDC_1877  29
LC_ADC_1875A 31
TrigData     1006

t05
Timestamp            1
sis3600read          2
sis3800ShadowUpdate  3
v792cblt            36

Moddef
Scaler:
t01
LC_SCALER_2551 slot


Detdef
0 0 t01 0 0          timestamp
1 0-11 t01 13 0-11 name; LC_SCALER_2551
2 0-11 t01 14 0-11 name; LC_SCALER_2551
3 0-31 t01 17 0-31 name; LC_SCALER_4434
4 0-31 t01 18 0-31 name; LC_SCALER_4434
5 0-31 t01 19 0-31 name; LC_SCALER_4434
6 0-31 t05 0x2000 0-31 name; SIS_3800
7 0-31 t05 0x3000 0-31 name; SIS_3800
8 0-31 t05 0x4000 0-31 name; SIS_3800
9 0-31 t05 0x5000 0-31 name; SIS_3800
10 0-31 t05 0x6000 0-31 name; SIS_3800
11 0-31 t05 0x7000 0-31 name; SIS_3800


*/

namespace cl2tade {

struct isdescriber {
    ems_u32 is_id;
    string crate;
    int(*decoder)(isdescriber&, ems_object&, sev_object&);
};

int parse_detdef(file_object&, bool ignore=false);
int parse_moddef(file_object&);
int decode_to_tade(deque<sev_object>& event, ems_object& ems,
        FILE*, FILE*, FILE*, FILE*, int* n=0);
int decode_to_tade_nosplit(deque<sev_object>& event, ems_object& ems,
        FILE*, FILE*, FILE*, FILE*, FILE*, int* n=0);
void clear(void);

} // end of namespace cl2tade

#endif
