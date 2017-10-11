/*
 * commu_zdb.cc
 * 
 * created 17.07.95
 * 
 */

#include "config.h"

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <sys/param.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <errors.hxx>

#include "commu_zdb.hxx"
#include "compat.h"
#include "versions.hxx"

VERSION("2014-07-11", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_zdb.cc,v 2.11 2014/07/14 15:12:20 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/

C_zdb::C_zdb(const string& commbezlist)
:liste(commbezlist)
{
FILE *list;

list=fopen(liste.c_str(), "r");
if (list==(FILE*)0)
  {
  ostringstream s;
  s<<"fopen "<<liste<<ends;
  throw new C_unix_error(errno, s);
  }
fclose(list);
}

/*****************************************************************************/

C_zdb::~C_zdb()
{}

/*****************************************************************************/

C_VED_addr* C_zdb::getVED(const string& ved_name_)
{
string ved_name(ved_name_);
FILE *list;
char name[1024], s[1024];
char path[1024], host[1024];
int port;
int found;
int smart=0;

found=0;
if ((list=fopen(liste.c_str(), "r"))==NULL)
  throw new C_unix_error(errno, "open commlist");

while ((fgets(s, 256, list)!=NULL) && !found)
  {
  if (s[0]=='+')
    smart=1;
  else if ((s[0]!='\n') && (s[0]!='#'))
    {
    int res=sscanf(s, " %s %s %d %s", name, host, &port, path);
//    if (res==-1) perror("sscanf");
    if (res<4) path[0]=0;
    if (res<3) port=0;
    if (ved_name==name) found=1;
    }
  }
fclose(list);
if (found)
  {
  if (path[0]==0)
    {
    if (port==0)
      {
      C_VED_addr_unix* addr;
      addr=new C_VED_addr_unix(host);
      return(addr);
      }
    else
      {
      C_VED_addr_inet* addr;
      addr=new C_VED_addr_inet(host, port);
      return(addr);
      }
    }
  else
    {
    C_VED_addr_inet_path* addr;
    C_strlist* list;
    list=new C_strlist(1);
    list->set(0, path);
    addr=new C_VED_addr_inet_path(host, port, list);
    return(addr);
    }
  }
else if (smart)
  {
  string hname; string portnum;
  int l=ved_name.length();
#ifdef NO_ANSI_CXX
  int colon=ved_name.index(":");
#else
  int colon=ved_name.find(':');
#endif
  if (colon<0)
    {
    hname=ved_name;
    port=2048;
    }
  else
    {
#ifdef NO_ANSI_CXX
    hname=ved_name(0, colon);
    portnum=ved_name(colon+1, l-colon);
#else
    hname=ved_name.substr(0, colon);
    portnum=ved_name.substr(colon+1, l-colon);
#endif
    port=17;
    sscanf(portnum.c_str(), "%d", &port);
    }
  C_VED_addr_inet* addr=new C_VED_addr_inet(hname, port);
  return addr;
  }
else
  {
  return (C_VED_addr*)0;
  }
}

/*****************************************************************************/

C_strlist* C_zdb::getVEDlist()
{
FILE *list;
#if 0
int c, i, res;
#else
int c, i;
#endif

C_strlist *strlist;
char line[1024], name[1024];

if ((list=fopen(liste.c_str(), "r"))==NULL)
  {
  throw new C_unix_error(errno, "open commlist");
  }

c=0;
while (fgets(line, 256, list)!=NULL) if ((line[0]!='\n') && (line[0]!='#')) c++;
rewind(list);

strlist=new C_strlist(c);

i=0;
while (fgets(line, 256, list)!=NULL)
  {
  if ((line[0]!='\n') && (line[0]!='#'))
    {
#if 0
    res=sscanf(line, " %s", name);
#endif
    strlist->set(i++, name);
    }
  }
fclose(list);
return(strlist);
}

/*****************************************************************************/
/*****************************************************************************/
