/*
 * commu_log.hxx
 * 
 * $ZEL: commu_log.hxx,v 2.11 2004/11/18 19:31:23 wuestner Exp $
 * 
 * created before 03.09.93
 * 26.03.1998 PW: adapded for <string>
 * 14.06.1998 PW: adapted for STD_STRICT_ANSI
 */

#ifndef _commu_log_hxx_
#define _commu_log_hxx_

#include "config.h"
#include "cxxcompat.hxx"
#include <commu_logg.hxx>
#include <commu_message.hxx>
#include <sys/types.h>
#include <un.h>
#include <netinet/in.h>
#include <errors.hxx>

/*****************************************************************************/

class C_log;

class C_log_manipi
  {
  private:
    typedef C_log& (*log_manipi) (C_log&, int);
    log_manipi f;
    int arg;
  public:
    C_log_manipi(log_manipi, int);
    friend C_log& operator<<(C_log&, const C_log_manipi&);
  };

/*****************************************************************************/

struct loggstruct
  {
  //C_cosylogger    *col;
  //C_syslogger     *syl;
  //C_speciallogger *spl;
  //C_filelogger    *fil;
  loggstruct();
  ~loggstruct();
  int addlog(C_logger*);
  void dellog(int);
  C_logger** logs;
  int numlogs;
  };

/*****************************************************************************/

class C_log
  {
  private:
    typedef C_log& (*log_manip) (C_log&);
    int base;
    //void putit(const char* s) {sbuffer+=s;}
    void putit(const STRING& s) {sbuffer+=s;}
  protected:
    STRING sbuffer;
    loggstruct& log;
    int printtime;
    int level;
  public:
    C_log(loggstruct&);
    virtual ~C_log();
    virtual void flushit();
    virtual void resetit();
    virtual void time(int);
    virtual void setbase(int);
    virtual void setlevel(int);
    C_log& operator<<(int);
    C_log& operator<<(unsigned int);
    C_log& operator<<(const void*);
    C_log& operator<<(char);
    C_log& operator<<(const char*);
    C_log& operator<<(const STRING);
    C_log& operator<<(const fd_set*);
    C_log& operator<<(struct sockaddr_un);
    C_log& operator<<(struct sockaddr_in);
    C_log& operator<<(log_manip);
    C_log& operator<<(const C_error&);
  };

/*****************************************************************************/

/*
Errorlog:
  Ausgaben erfolgen immer
  Ziel:
    Cosylog: E_LOG //oder <0
    syslog
    spezielle Clients
*/
class C_elog: public C_log
  {
  public:
    C_elog(loggstruct&);
    virtual ~C_elog();
    virtual void flushit();
  };

/*****************************************************************************/

/*
Notice-Log:
  Ausgaben erfolgen immer
  Ziel:
    Cosylog: S_LOG
    syslog
    spezielle Clients
*/
class C_nlog: public C_log
  {
  public:
    C_nlog(loggstruct&);
    virtual ~C_nlog();
    virtual void flushit();
  };

/*****************************************************************************/

/*
Debug-Log:
  Ausgaben nur wenn DEBUG definiert ist und auf Wunsch
  Ziel:
    Cosylog: T_LOG
    spezielle Clients
*/
class C_dlog: public C_log
  {
  public:
    C_dlog(loggstruct&);
    virtual ~C_dlog();
    virtual void flushit();
  };

/*****************************************************************************/

C_log& flush(C_log&);
C_log& reset(C_log&);
C_log& hex(C_log&);
C_log& dec(C_log&);
C_log& time(C_log&);
C_log& error(C_log&);
C_log_manipi level(const int);
C_log_manipi error(const int);

#endif

/*****************************************************************************/
/*****************************************************************************/
