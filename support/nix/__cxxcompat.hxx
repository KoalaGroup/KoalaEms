/*
 * cxxcompat.hxx
 * $ZEL: cxxcompat.hxx,v 2.1 2003/09/11 12:05:28 wuestner Exp $
 * 
 * created: 01.Apr.2003 PW
 */

#ifndef _cxxcompat_hxx_
#define _cxxcompat_hxx_

#if defined (__STD_STRICT_ANSI) || defined (__STRICT_ANSI__)

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>

#define STRING string
#define SSTREAM stringstream
#define OSSTREAM ostringstream
#define ISSTREAM istringstream
#define NORETURN(x)

#else /* ! __STRICT_ANSI__ */

#error NO ANSI
#include <iostream.h>
#include <fstream.h>
#include <strstream.h>
#include <iomanip.h>
#include <String.h>

#define NO_ANSI_CXX

class STRING: public String {
    public:
        static const int npos;
        STRING():String() {}
        STRING(const char* c):String(c) {}
        STRING(const String& s):String(s) {}
        const char* c_str() const {return (const char*)(*this);}
        STRING substr(int start, int n) const {
            return (*this)(start, n==STRING::npos?this->length()-start:n);
        }
        int compare(const STRING& s) const {
            if ((*this)<s) return -1;
            if ((*this)>s) return 1;
            return 0;
        }
        int compare(const char* s) const {
            if ((*this)<s) return -1;
            if ((*this)>s) return 1;
            return 0;
        }
};

class SSTREAM: public strstream {};

class OSSTREAM: public ostrstream {
    public:
        STRING str() {
            STRING s;
            strstreambuf* sb;
            sb=this->rdbuf();
            s=sb->str();
            sb->freeze(0);
            return s;
        }
};

// class ISSTREAM: public istrstream {
//     public:
//         ISSTREAM(const char* c):istrstream(c) {}
// };

#define NORETURN(x) return(x)

#endif

#ifdef HAVE_NAMESPACES
using namespace std;
#endif

#endif
