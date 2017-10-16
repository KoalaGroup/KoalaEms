/*
 * cxxcompat.cc
 * 
 * created 09.Apr.2003 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <versions.hxx>

VERSION("Apr 09 2003", __FILE__, __DATE__, __TIME__)
static char *rcsid="$ZEL: cxxcompat.cc,v 1.1 2003/09/30 09:17:39 wuestner Exp $";
#define XVERSION

#ifdef NO_ANSI_CXX

const int STRING::npos=-1;

#endif
