/*
 * cxxcompat.hxx
 * $ZEL: cxxcompat.hxx,v 2.2 2004/11/26 14:45:31 wuestner Exp $
 * 
 * created: 01.Apr.2003 PW
 */

#ifndef _cxxcompat_hxx_
# define _cxxcompat_hxx_

# if defined (__STD_STRICT_ANSI) || defined (__STRICT_ANSI__)

#  include <iostream>
#  include <fstream>
#  include <sstream>
#  include <iomanip>
#  include <string>

// #define STRING string
// #define SSTREAM stringstream
// #define OSSTREAM ostringstream
// #define ISSTREAM istringstream
#  define STRING string
#  define STRINGSTREAM stringstream
#  define OSTRINGSTREAM ostringstream
#  define ISTRINGSTREAM istringstream

#  define NORETURN(x)

# else /* ! __STRICT_ANSI__ */

#  error NO ANSI

# endif

# ifdef HAVE_NAMESPACES
using namespace std;
# endif

#endif
