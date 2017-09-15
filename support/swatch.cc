/******************************************************************************
*                                                                             *
* swatch.cc                                                                   *
*                                                                             *
* OSF1/ULTRIX                                                                 *
*                                                                             *
* created: 17.08.94                                                           *
* last changed: 18.08.94                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include <sys/time.h>
#include <sys/resource.h>
#include <swatch.hxx>
#include <compat.h>

/*****************************************************************************/

swatch::swatch()
:running(0)
{
reset();
}

/*****************************************************************************/

swatch::~swatch()
{}

/*****************************************************************************/

void swatch::start()
{
struct timezone tz;
struct rusage usage;

gettimeofday(&real_0, &tz);
getrusage(RUSAGE_SELF, &usage);
user_0=usage.ru_utime;
system_0=usage.ru_stime;
running=1;
}

/*****************************************************************************/

void swatch::stop()
{
struct timezone tz;
struct rusage usage;

gettimeofday(&real_1, &tz);
getrusage(RUSAGE_SELF, &usage);
user_1=usage.ru_utime;
system_1=usage.ru_stime;
running=0;
}

/*****************************************************************************/

void swatch::reset()
{
struct timezone tz;
struct rusage usage;

gettimeofday(&real_0, &tz);
getrusage(RUSAGE_SELF, &usage);
user_0=usage.ru_utime;
system_0=usage.ru_stime;
if (!running)
  {
  real_1=real_0;
  user_1=user_0;
  system_1=system_0;
  }
}

/*****************************************************************************/

double swatch::real()
{
struct timezone tz;
int sec, usec;

if (running) gettimeofday(&real_1, &tz);
sec=real_1.tv_sec-real_0.tv_sec;
usec=real_1.tv_usec-real_0.tv_usec;
return((double)sec+(double)usec/1000000.0);
}

/*****************************************************************************/

double swatch::system()
{
int sec, usec;

if (running)
  {
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  system_1=usage.ru_stime;
  }
sec=system_1.tv_sec-system_0.tv_sec;
usec=system_1.tv_usec-system_0.tv_usec;
return((double)sec+(double)usec/1000000.0);
}

/*****************************************************************************/

double swatch::user()
{
int sec, usec;

if (running)
  {
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  user_1=usage.ru_stime;
  }
sec=user_1.tv_sec-user_0.tv_sec;
usec=user_1.tv_usec-user_0.tv_usec;
return((double)sec+(double)usec/1000000.0);
}

/*****************************************************************************/
/*
double swatch::resolution()
{
struct timezone tz;
itimer.get_v();
itimer.get_r();
gettimeofday();
running=1;
}
*/

/*****************************************************************************/
/*****************************************************************************/
