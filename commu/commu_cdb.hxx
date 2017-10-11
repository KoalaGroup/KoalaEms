/*
 * commu_cdb.hxx
 * 
 * $ZEL: commu_cdb.hxx,v 2.12 2014/07/14 15:12:19 wuestner Exp $
 *
 * created 17.07.95
 * 
 */

#ifndef _commu_cdb_hxx_
#define _commu_cdb_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <commu_db.hxx>

using namespace std;

/*****************************************************************************/
class C_cdb: public C_db
  {
  private:
    int proc_num;
    static const int proc_type;
    static const string proc_basename;
  public:
    C_cdb();
    virtual ~C_cdb();
    //virtual char* listname();
    virtual const string& listname();
    virtual C_VED_addr* getVED(const string&);
    virtual C_strlist* getVEDlist();
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
