/*
 * commu_cdb.hxx
 * 
 * $ZEL: commu_cdb.hxx,v 2.11 2004/11/18 19:31:04 wuestner Exp $
 *
 * created 17.07.95
 * 26.03.1998 PW: adapded for <string>
 * 14.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#ifndef _commu_cdb_hxx_
#define _commu_cdb_hxx_

#include "config.h"
#include "cxxcompat.hxx"
#include <commu_db.hxx>

/*****************************************************************************/
class C_cdb: public C_db
  {
  private:
    int proc_num;
    static const int proc_type;
    static const STRING proc_basename;
  public:
    C_cdb();
    virtual ~C_cdb();
    //virtual char* listname();
    virtual const STRING& listname();
    virtual C_VED_addr* getVED(const STRING&);
    virtual C_strlist* getVEDlist();
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
