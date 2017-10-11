/*
 * proclib/caplib.hxx
 * 
 * created: 16.08.94 PW
 * 
 * $ZEL: caplib.hxx,v 2.7 2014/07/14 15:11:53 wuestner Exp $
 */

#ifndef _caplib_hxx_
#define _caplib_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#define DEF_SEPARATOR '.'

using namespace std;

/*****************************************************************************/

class C_capability_list
  {
  public:
    C_capability_list(int size);
    C_capability_list(const C_capability_list&);
    ~C_capability_list();

  private:
    struct capentry
      {
      char *name;
      int index;
      int version;
      };
    capentry* caplist;
    int maxsize;
    int entries;
    char separator;

  public:
    void add(char* name, int index, int version);
    int get(const char* name) const;
    int get(const char* name, int version) const;
    string get(int proc) const;
    int size() const;
    char version_separator() const;
    char version_separator(char);
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
