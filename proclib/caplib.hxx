/*
 * caplib.hxx       
 * 
 * created: 16.08.94 PW
 * 25.03.1998 PW: adapded for <string>
 * 12.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#ifndef _caplib_hxx_
#define _caplib_hxx_

#include "config.h"
#include "cxxcompat.hxx"
#define DEF_SEPARATOR '.'

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
    STRING get(int proc) const;
    int size() const;
    char version_separator() const;
    char version_separator(char);
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
