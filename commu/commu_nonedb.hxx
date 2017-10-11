/*
 * commu_nonedb.hxx
 * 
 * $ZEL: commu_nonedb.hxx,v 2.4 2014/07/14 15:12:19 wuestner Exp $
 * 
 * created 14.08.95 PW
 */

#ifndef _commu_nonedb_hxx_
#define _commu_nonedb_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <commu_db.hxx>

using namespace std;

class C_nonedb: public C_db
  {
  public:
    C_nonedb() {}
    virtual ~C_nonedb() {}
  private:
    void all();
  public:
    //virtual char* listname();
    virtual const string& listname();
    virtual void new_db(const String&) {all();}
    virtual C_VED_addr* getVED(const String&) {all(); return 0;}
    virtual void setVED(const String&, const C_VED_addr&) {all();}
    virtual C_strlist* getVEDlist() {all(); return 0;}
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
