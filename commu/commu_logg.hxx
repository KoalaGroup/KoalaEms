/*
 * commu_logg.hxx
 * 
 * $ZEL: commu_logg.hxx,v 2.14 2014/07/14 15:12:19 wuestner Exp $
 * 
 * created before 03.09.93
 * 
 */

#ifndef _commu_logg_hxx_
#define _commu_logg_hxx_

#include "config.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <commu_message.hxx>
#include <commu_list_t.hxx>

using namespace std;

/*****************************************************************************/

class C_logger
  {
  protected:
    char *name;
    char* timestring();
    virtual void put(int, const char*)=0;
    virtual void put(int, const string&)=0;
  public:
//    typedef int Error;
    C_logger(const char*);
    virtual ~C_logger()=0;
    virtual void lstring_t(int, const char*);// Stringausgabe mit Prior und Zeit
    virtual void lstring_t(int, const string&);// Stringausgabe mit Prior und Zeit
    virtual void lstring(int, const char*);  // Stringausgabe mit Priority
    virtual void lstring(int, const string&);  // Stringausgabe mit Priority
    virtual void lstring_t(const char *) {}
  };

/*****************************************************************************/

class C_cosylogger: public C_logger
  {
  private:
    int cosy;
    ofstream cons;
    virtual void put(int, const char*);
    virtual void put(int, const string&);
  public:
    C_cosylogger(const char*, int, const char*);
    virtual ~C_cosylogger();
    int add_window(const char *);
    int retry();
  };

/*****************************************************************************/

class C_syslogger: public C_logger
  {
  private:
    virtual void put(int, const char*);
    virtual void put(int, const string&);
  public:
    C_syslogger(const char*);
    virtual ~C_syslogger();
    virtual void lstring_t(const char *);       // Stringausgabe mit Zeit, Typ 0
    virtual void lstring_t(int, const char *);  // Stringausgabe mit Typ und Zeit
    virtual void lstring_t(int, const string&); // Stringausgabe mit Prior und Zeit
  };

/*****************************************************************************/

class C_speciallogger: public C_logger
  {
  private:
    virtual void put(int, const char*);
    virtual void put(int, const string&);
    C_ints servers;
  public:
    C_speciallogger(const char*);
    virtual ~C_speciallogger();
    void addserver(int);
    void removeserver(int);
    C_speciallogger& operator<<(const C_message&);
  };

/*****************************************************************************/

class C_filelogger: public C_logger
  {
  private:
    virtual void put(int, const char*);
    virtual void put(int, const string&);
    ofstream out;
    int dest;
  public:
    C_filelogger(const char*, const char*);
    virtual ~C_filelogger();
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
