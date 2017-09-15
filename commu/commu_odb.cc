/*
 * commu_odb.cc
 * 
 * created 17.07.95 PW
 */

//#include <sys/param.h>
//#include <iostream.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h>
//#include <string.h>
//#include <errno.h>
//#include <prototypes.h>
//#include <commu_def.hxx>
//#include <readargs.hxx>
//#include <errors.hxx>
//#include <sys/socket.h>

//#include <cosydb.h>    spaeter wieder?
//#include <oo.h>        spaeter wieder?

#include <commu_odb.hxx>
#include "versions.hxx"

VERSION("Aug 11 1995", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_odb.cc,v 2.4 2004/11/26 15:14:25 wuestner Exp $")
#define XVERSION

/*****************************************************************************/

C_odb::C_odb(const char* _fdbname, const char* _dbname)
:fdbname(_fdbname), dbname(_dbname)
{
ooTrans transaction;
ooHandle(ooFDObj) fdb;
ooHandle(ooDBObj) db;
ooHandle(ooContObj) cont;

if (ooInit()!=oocSuccess) goto fehler;

if (transaction.start(oocMROW)!=oocSuccess) goto fehler;

if (fdb.open(fdbname, oocRead)!=oocSuccess) goto fehler;

if (db.open(fdb, dbname, oocRead)!=oocSuccess) goto fehler;

if (cont.open(db, "VEDroutes", oocRead)!=oocSuccess) goto fehler;

if (transaction.commit()!=oocSuccess) goto fehler;

return;

fehler:
delete this;
throw new C_program_error("C_odb::C_odb(): error in database");
}

/*****************************************************************************/

C_odb::~C_odb()
{}

/*****************************************************************************/

String C_odb::listname()
{
ostrstream ss;
ss << fdbname << ends;
return(ss.str());
}

/*****************************************************************************/

C_VED_addr* C_odb::getVED(const char *ved_name)
{
ooTrans transaction;
ooHandle(ooFDObj) fdb;
ooHandle(ooDBObj) db;
ooHandle(ooContObj) cont;
ooHandle(VEDroute_oo) ved;

if (transaction.start(oocMROW)!=oocSuccess) goto fehler;

if (fdb.open(fdbname, oocRead)!=oocSuccess) goto fehler;

if (db.open(fdb, dbname, oocRead)!=oocSuccess) goto fehler;

if (cont.open(db, "VEDroutes", oocRead)!=oocSuccess) goto fehler;

if (ved.lookupObj(cont, ved_name, oocRead)!=oocSuccess) goto fehler;

if (ved->path[0]==0)
  {
  C_VED_addr_inet* addr;
  addr=new C_VED_addr_inet(ved->host, ved->port);
  if (transaction.commit()!=oocSuccess) {delete addr; goto fehler;}
  return(addr);
  }
else
  {
  C_VED_addr_inet_path* addr;
  C_strlist* list;
  list=new C_strlist(1);
  list->set(0, ved->path);
  addr=new C_VED_addr_inet_path(ved->host, ved->port, list);
  if (transaction.commit()!=oocSuccess) {delete addr; goto fehler;}
  return(addr);
  }

fehler:
transaction.abort();
throw new C_program_error("C_odb::getVED(): VED not found");

return(0); // wird nie erreicht
}

/*****************************************************************************/

C_strlist* C_odb::getVEDlist()
{
ooTrans transaction;
ooHandle(ooFDObj) fdb;
ooHandle(ooDBObj) db;
ooHandle(ooContObj) cont;
ooHandle(VEDroute_oo) ved;
ooItr(VEDroute_oo) ved_i;
int count, i;
C_strlist *strlist;

if (transaction.start(oocMROW)!=oocSuccess) goto fehler;

if (fdb.open(fdbname, oocRead)!=oocSuccess) goto fehler;

if (db.open(fdb, dbname, oocRead)!=oocSuccess) goto fehler;

if (cont.open(db, "VEDroutes", oocRead)!=oocSuccess) goto fehler;

if (cont.contains((ooItr(ooObj)&)ved_i))
  {
  count=0;
  while (ved_i.next()) count++;
  }
else
  count=0;

strlist=new C_strlist(count);
if (cont.contains((ooItr(ooObj)&)ved_i))
  {
  i=0;
  while (ved_i.next()) strlist->set(i++, ved_i.getObjName(cont));
  }

if (transaction.commit()!=oocSuccess)
  {
  delete strlist;
  goto fehler;
  }

return(strlist);

fehler:
transaction.abort();
throw new C_program_error("C_odb::C_odb(): error in database");

return(0); // wird nie erreicht
}

/*****************************************************************************/

int C_odb::addVED(char*, char*, int*, char*)
{
throw new C_program_error("C_odb::addVED() not jet implemented");
return(0);
}

/*****************************************************************************/

int C_odb::changeVED(char*, char*, int*, char*)
{
throw new C_program_error("C_odb::changeVED() not jet implemented");
return(0);
}

/*****************************************************************************/

int C_odb::delVED(char*)
{
throw new C_program_error("C_odb::delVED() not jet implemented");
return(0);
}

/*****************************************************************************/
/*****************************************************************************/
