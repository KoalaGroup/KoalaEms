/******************************************************************************
*                                                                             *
* swatch.hxx                                                                  *
*                                                                             *
* OSF1/ULTRIX                                                                 *
*                                                                             *
* created: 17.08.94                                                           *
* last changed: 18.08.94                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#ifndef _swatch_hxx_
#define _swatch_hxx_

#include <sys/time.h>

/*****************************************************************************/

class swatch
  {
  public:
    swatch();
    swatch(const swatch&);
    ~swatch();
  private:
    int running;
    struct timeval real_0;
    struct timeval real_1;
    struct timeval system_0;
    struct timeval system_1;
    struct timeval user_0;
    struct timeval user_1;
  public:
    void start();
    void stop();
    void reset();
    int status() const {return(running);}
    double system();
    double user();
    double real();
//    static double resolution();
  };

#endif

/*****************************************************************************/
/*****************************************************************************/
