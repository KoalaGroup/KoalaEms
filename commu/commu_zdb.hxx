/*
 * commu_zdb.hxx
 * 
 * $ZEL: commu_zdb.hxx,v 2.9 2014/07/14 15:12:20 wuestner Exp $
 * 
 * created 17.07.95
 * 
 */

#ifndef _commu_zdb_hxx_
#define _commu_zdb_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include "commu_db.hxx"

using namespace std;

/*****************************************************************************/
class C_zdb: public C_db
  {
  private:
    string liste;
  public:
    C_zdb(const string&);
    virtual ~C_zdb();
    virtual const string& listname() {return liste;}
    virtual C_VED_addr* getVED(const string&);
    virtual C_strlist* getVEDlist();
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
