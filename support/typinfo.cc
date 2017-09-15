/*
 * typinfo.cc
 * 
 * created 09.06.98 PW
 */

#include "config.h"
#include "typinfo.hxx"
#include "versions.hxx"

#ifndef XVERSION
VERSION("Jun 11 1998", __FILE__, __DATE__, __TIME__)
static char* rcsid="$ZEL: typinfo.cc,v 2.2 2004/11/26 14:45:41 wuestner Exp $";
#define XVERSION
#endif
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/

#ifdef HAVE_NAMESPACES
namespace typen {
#endif

template<> const char* type_name<char>(char) {return "int_8";}
template<> type_ids type_id<char>(char) {return i8_type;}
template<> int is_float<char>(char) {return 0;}

template<> const char* type_name<unsigned char>(unsigned char) {return "uint_8";}
template<> type_ids type_id<unsigned char>(unsigned char) {return ui8_type;}
template<> int is_float<unsigned char>(unsigned char) {return 0;}

template<> const char* type_name<short>(short) {return "int_16";}
template<> type_ids type_id<short>(short) {return i16_type;}
template<> int is_float<short>(short) {return 0;}

template<> const char* type_name<unsigned short>(unsigned short) {return "uint_16";}
template<> type_ids type_id<unsigned short>(unsigned short) {return ui16_type;}
template<> int is_float<unsigned short>(unsigned short) {return 0;}

template<> const char* type_name<int>(int) {return "int_32";}
template<> type_ids type_id<int>(int) {return i32_type;}
template<> int is_float<int>(int) {return 0;}

template<> const char* type_name<unsigned int>(unsigned int) {return "uint_32";}
template<> type_ids type_id<unsigned int>(unsigned int) {return ui32_type;}
template<> int is_float<unsigned int>(unsigned int) {return 0;}

#ifdef __alpha
template<> const char* type_name<long>(long) {return "int_64";}
template<> type_ids type_id<long>(long) {return i64_type;}
template<> int is_float<long>(long) {return 0;}

template<> const char* type_name<unsigned long>(unsigned long) {return "uint_64";}
template<> type_ids type_id<unsigned long>(unsigned long) {return ui64_type;}
template<> int is_float<unsigned long>(unsigned long) {return 0;}
#else
template<> const char* type_name<long>(long) {return "int_32";}
template<> type_ids type_id<long>(long) {return i32_type;}
template<> int is_float<long>(long) {return 0;}

template<> const char* type_name<unsigned long>(unsigned long) {return "uint_32";}
template<> type_ids type_id<unsigned long>(unsigned long) {return ui32_type;}
template<> int is_float<unsigned long>(unsigned long) {return 0;}
#endif

template<> const char* type_name<float>(float) {return "float_32";}
template<> type_ids type_id<float>(float) {return f32_type;}
template<> int is_float<float>(float) {return 1;}

template<> const char* type_name<double>(double) {return "float_64";}
template<> type_ids type_id<double>(double) {return f64_type;}
template<> int is_float<>(double) {return 1;}

#ifdef HAVE_NAMESPACES
}
#endif
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~/
