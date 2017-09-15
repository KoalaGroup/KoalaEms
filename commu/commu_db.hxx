/*
 * commu_db.hxx
 * 
 * $ZEL: commu_db.hxx,v 2.7 2004/11/18 19:31:07 wuestner Exp $
 * 
 * created before 08.02.94
 * 26.03.1998 PW: adapded for <string>
 * 14.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#ifndef _commu_db_hxx_
#define _commu_db_hxx_

#include "config.h"
#include "cxxcompat.hxx"
#include <ved_addr.hxx>

/*****************************************************************************/
class C_db
  {
  public:
    C_db(){}
    virtual ~C_db(){}
    //virtual char* listname()=0;
    virtual const STRING& listname()=0;
    virtual void new_db(const STRING&);
    virtual C_VED_addr* getVED(const STRING&)=0;
    virtual void setVED(const STRING&, const C_VED_addr&);
    virtual C_strlist* getVEDlist()=0;
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
