/*
 * commu_db.hxx
 * 
 * $ZEL: commu_db.hxx,v 2.8 2014/07/14 15:12:19 wuestner Exp $
 * 
 * created before 08.02.94
 * 
 */

#ifndef _commu_db_hxx_
#define _commu_db_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <ved_addr.hxx>

/*****************************************************************************/
class C_db
  {
  public:
    C_db(){}
    virtual ~C_db(){}
    //virtual char* listname()=0;
    virtual const string& listname()=0;
    virtual void new_db(const string&);
    virtual C_VED_addr* getVED(const string&)=0;
    virtual void setVED(const string&, const C_VED_addr&);
    virtual C_strlist* getVEDlist()=0;
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
