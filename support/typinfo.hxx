/*
 * typinfo.hxx
 * 
 * created 09.06.98 PW
 */

#ifndef _typinfo_hxx_
#define _typinfo_hxx_
#include "config.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
#ifdef HAVE_NAMESPACES
namespace typen {
#else
#define typen
#endif

enum type_ids {i8_type, ui8_type, i16_type, ui16_type, i32_type, ui32_type,
  i64_type, ui64_type, i128_type, ui128_type, f32_type, f64_type, f128_type};

template<class T> const char* type_name(T);
template<class T> type_ids type_id(T);
template<class T> int is_float(T);

template<> const char* type_name<char>(char);
template<> type_ids type_id<char>(char);
template<> int is_float<char>(char);

template<> const char* type_name<unsigned char>(unsigned char);
template<> type_ids type_id<unsigned char>(unsigned char);
template<> int is_float<unsigned char>(unsigned char);

template<> const char* type_name<short>(short);
template<> type_ids type_id<short>(short);
template<> int is_float<short>(short);

template<> const char* type_name<unsigned short>(unsigned short);
template<> type_ids type_id<unsigned short>(unsigned short);
template<> int is_float<unsigned short>(unsigned short);

template<> const char* type_name<int>(int);
template<> type_ids type_id<int>(int);
template<> int is_float<int>(int);

template<> const char* type_name<unsigned int>(unsigned int);
template<> type_ids type_id<unsigned int>(unsigned int);
template<> int is_float<unsigned int>(unsigned int);

template<> const char* type_name<long>(long);
template<> type_ids type_id<long>(long);
template<> int is_float<long>(long);

template<> const char* type_name<unsigned long>(unsigned long);
template<> type_ids type_id<unsigned long>(unsigned long);
template<> int is_float<unsigned long>(unsigned long);

template<> const char* type_name<float>(float);
template<> type_ids type_id<float>(float);
template<> int is_float<float>(float);

template<> const char* type_name<double>(double);
template<> type_ids type_id<double>(double);
template<> int is_float<>(double);

#ifdef HAVE_NAMESPACES
}
#endif

#endif
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
