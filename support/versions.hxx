/*
 * versions.hxx
 * 
 * $ZEL: versions.hxx,v 1.10 2010/10/13 14:17:12 wuestner Exp $
 * 
 * created ??.??.???? PW
 * 05.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#ifndef _versions_hxx_
#define _versions_hxx_

#include "config.h"
//#include "cxxcompat.hxx"
#include <iostream>
#include <iomanip>
#include <stdlib.h>

#if 0
#define VERSION(last, file, date, time) \
  class Version \
    { \
    public: \
      Version(const char* l, const char* f, const char* d, const char* t) \
        { \
        if (getenv("EMS_VERSIONS")!=0) \
            std::cerr<<f<<":"<<std::endl<<"last changed: "<<l<<"; " \
                <<"compiled: "<<d<<", "<<t<<std::endl; \
        } \
    }; \
   \
  static Version FileVersion(last, file, date, time);
#endif

#define VERSION(last, file, date, time, id) \
namespace { \
  class Version { \
    public: \
      Version(const char* l, const char* f, const char* d, const char* t, const char* i) \
      :ll(last), ff(file), dd(date), tt(time), ii(id) \
      { \
        if (getenv("EMS_VERSIONS")!=0) \
            std::cerr<<f<<":"<<std::endl<<"last changed: "<<l<<"; " \
                <<"compiled: "<<d<<", "<<t<<std::endl; \
        if (getenv("RCS_VERSIONS")!=0) \
            std::cerr<<i<<std::endl; \
      } \
      const char *ll; \
      const char *ff; \
      const char *dd; \
      const char *tt; \
      const char *ii; \
    }; \
   \
  Version FileVersion(last, file, date, time, id); \
}
#endif
