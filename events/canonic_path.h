/*
 * events/canonic_path.h
 * $ZEL: canonic_path.h,v 1.2 2010/09/04 21:23:42 wuestner Exp $
 * 
 * created 17.Apr.2004 PW
 */

#ifndef _canonic_path_h_
#define _canonic_path_h_

int canonic_path(const char* pathname, char** dirname, char** filename, int split);

#endif
