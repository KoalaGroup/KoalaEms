/*
 * commu_cosy_logg.cc
 * 
 * created before 05.02.95 PW
 */

#include <syslog.h>
#include <commu_logg.hxx>

#ifdef COSYLOG
#include <CosyLogIFC.h>
#include "versions.hxx"

VERSION("Feb 05 1995", __FILE__, __DATE__, __TIME__,
"$ZEL: commu_cosy_logg.cc,v 2.4 2004/11/26 15:14:16 wuestner Exp $")
#define XVERSION

#define I_LOG 180 // Command Log-Infos
#define A_LOG 280 // Acknowledge Log-Infos
#define E_LOG 380 // Hard- and Software Error Log-Infos
#define S_LOG 480 // State Log-Infos
#define T_LOG 580 // Trace Points for program testing
#endif

/*****************************************************************************/

C_cosylogger::C_cosylogger(const char* progname, int cosylog,
    const char* ersatz)
:C_logger(progname), cons(ersatz, ios::app)
{
cosy=cosylog;
#ifndef COSYLOG
cosy=0;
#else
if (cosy)
  {
  int rc;
  if ((rc=coslogi_(NULL, NULL, "no_auto"))!=0)
      cons << name << ":initlog: error " << rc << '\n';
  cosy=(rc==0);
  }
#endif
}

/*****************************************************************************/

C_cosylogger::~C_cosylogger()
{
#ifdef COSYLOG
if (cosy)
  {
  int rc;
  rc=coslogc_();
  if (rc!=0) cerr << name << ":donelog: error " << rc << '\n';
  }
#endif
}

/*****************************************************************************/

int C_cosylogger::retry()
{
#ifndef COSYLOG
return(-4);
#else
if (cosy) return(0);

int rc;
rc=coslogi_(NULL, NULL, "no_auto");
cosy=(rc==0);
return(rc);
#endif
}

/*****************************************************************************/

int C_cosylogger::add_window(const char *name)
{
#ifdef COSYLOG
if (cosy)
  {
  return(coslogax_(name));
  }
return(-4);
#else
return(-4);
#endif
}

/*****************************************************************************/

void C_cosylogger::put(int prior, const char *s)
{
#ifdef COSYLOG
if (cosy)
  {
  int i, type, rc, ipar[MAX_PARAMETERS];
  const char *spar[MAX_STRINGS];

  for (i=0; i<MAX_PARAMETERS; i++) ipar[i]=0;
  for (i=1; i<MAX_STRINGS; i++) spar[i]="";
  spar[0]=s;
  switch (prior)
    {
    case LOG_ALERT  :
#ifndef __osf__
    case LOG_SALERT :
#endif
    case LOG_EMERG  :
    case LOG_ERR    :
    case LOG_CRIT   :
    case LOG_WARNING: type=E_LOG; break;
    case LOG_NOTICE : type=S_LOG; break;
    case LOG_INFO   : type=I_LOG; break;
    case LOG_DEBUG  : type=T_LOG; break;
    default         : type=I_LOG; break;
    }
  rc=coslog_(type, ipar, spar);
  if (rc!=0)
    {
    cosy=0;
    cons << "C_log::put: error " << rc << endl;
    }
  }
if (!cosy)
#endif
cons << s << endl;
}

/*****************************************************************************/
/*****************************************************************************/
