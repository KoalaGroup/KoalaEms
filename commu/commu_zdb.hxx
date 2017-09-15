/*
 * commu_zdb.hxx
 * 
 * $ZEL: commu_zdb.hxx,v 2.8 2004/11/18 19:31:37 wuestner Exp $
 * 
 * created 17.07.95
 * 26.03.1998 PW: adapded for <string>
 * 14.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#ifndef _commu_zdb_hxx_
#define _commu_zdb_hxx_

#include "config.h"
#include "cxxcompat.hxx"
#include "commu_db.hxx"

/*****************************************************************************/
class C_zdb: public C_db
  {
  private:
    STRING liste;
  public:
    C_zdb(const STRING&);
    virtual ~C_zdb();
    virtual const STRING& listname() {return liste;}
    virtual C_VED_addr* getVED(const STRING&);
    virtual C_strlist* getVEDlist();
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
