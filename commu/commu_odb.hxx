/*
 * commu_odb.hxx
 * 
 * $ZEL: commu_odb.hxx,v 2.5 2014/07/14 15:12:19 wuestner Exp $
 * 
 * created 17.07.95 PW
 */

#ifndef _commu_odb_hxx_
#define _commu_odb_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <commu_db.hxx>

using namespace std;

class C_odb: public C_db
  {
  private:
    String fdbname;
    String dbname;
  public:
    C_odb(const String&, const String&);
    virtual ~C_odb();
    //virtual char* listname();
    virtual const STRING& listname();
    virtual C_VED_addr* getVED(const String&);
    virtual C_strlist* getVEDlist();
    virtual int changeVED(const String&, const C_VED_addr&);
    virtual int delVED(const String&);
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
