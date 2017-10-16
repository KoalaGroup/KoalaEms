/******************************************************************************
*                                                                             *
* w2ped.cc                                                                    *
*                                                                             *
* OSF1                                                                        *
*                                                                             *
* created 06.04.95                                                            *
* last changed 06.04.95                                                       *
*                                                                             *
******************************************************************************/

#include "config.h"
#include "cxxcompat.hxx"
#include <versions.hxx>

VERSION("Aug 14 1998", __FILE__, __DATE__, __TIME__,
"$ZEL: w2ped.cc,v 1.6 2005/02/11 15:45:19 wuestner Exp $")
#define XVERSION

int lidx=0;

/*****************************************************************************/

void start()
{
cout.setf(ios::hex, ios::basefield);
cout.setf(ios::showbase);
}

/*****************************************************************************/

void stop()
{
if ((lidx%3)!=0) cout << endl;
}

/*****************************************************************************/

void write_it(int a, int b)
{
cout << setw(10) << a << " "
    << setw(10) << b;

if ((++lidx%3)==0)
  cout << endl;
else
  cout << " ";
}

/*****************************************************************************/

main()
{
char line[1024];
STRING s, s1;

start();
while (cin.getline(line, 1024, '\n'))
  {
  int idx;
  s=line;
#ifdef NO_ANSI_CXX
  if ((idx=s.index("Chann:"))<0) continue;
#else
  if ((idx=s.find("Chann:"))==string::npos) continue;
#endif
  ISSTREAM str(/*s.c_str()*/line);
  int ch, ped, imped, idev;
  float mped, dev;
  str >> s1 >> ch >> s1 >> s1 >> ped >> s1 >> mped >> s1 >> s1 >> dev;
  imped=(int)(mped*1000);
  idev=(int)(dev*1000);
//  cout << "...... " << imped << " " << idev << endl;
  write_it(imped, idev);
  }
stop();
return(0);
}

/*****************************************************************************/
/*****************************************************************************/
