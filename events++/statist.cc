/******************************************************************************
*                                                                             *
* statist.cc                                                                  *
*                                                                             *
* OSF1/ULTRIX                                                                 *
*                                                                             *
* created: 19.08.96                                                           *
* last changed: 20.08.96                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
******************************************************************************/

#include "statist.hxx"

/*****************************************************************************/

void statist_int::add(int i)
{
sum+=i;
qsum+=i*i;
if (num)
  {
  if (i<min)
    {
    min=i; nmin=1;
    }
  else if (i==min)
    nmin++;
  if (i>max)
    {
    max=i; nmax=1;
    }
  else if (i==max)
    nmax++;
  }
else
  {
  min=i; max=i;
  nmin=1; nmax=1;
  }
num++;
mark();
}

/*****************************************************************************/

void statist_int::add(statist_int& st)
{
sum+=st.sum;
qsum+=st.qsum;
if (num==0)
  {
  min=st.min;
  max=st.max;
  nmin=st.nmin;
  nmax=st.nmax;
  }
else if (st.num>0)
  {
  if (min==st.min)
    nmin+=st.nmin;
  else if (min>st.min)
    {
    min=st.min;
    nmin=st.nmin;  
    }
  if (max==st.max)
    nmax+=st.nmax;
  else if (max<st.max)
    {
    max=st.max;
    nmax=st.nmax;  
    }
  }
num+=st.num;
}

/*****************************************************************************/

void statist_int::print(ostream& st)
{
if (num==0)
  {
  st<<"no data"<<endl;
  return;
  }

double average;
average=sum/num;
if (min==max)
  {
  st<<"num: "<<num<<" constant size: "<<min<<endl;
  }
else
  {
  st<<"num: "<<num<<" average: "<<average
     <<" max: "<<max<<" (" <<nmax<<" times)"
     <<" min: "<<min<<" (" <<nmin<<" times)"<<endl;
  }
}

/*****************************************************************************/

statist::statist(void)
:num(0), sum(0), qsum(0), mdist(0)
{}

/*****************************************************************************/

void statist::mark(void)
{
if (mdist && (sum>=nextmark))
  {
  cerr<<'.'<<flush;
  do nextmark+=mdist; while (nextmark<=sum);
  }
}

/*****************************************************************************/

int statist::markdist(int n)
{
int m=mdist;
mdist=n;
nextmark=mdist*(int)(sum/mdist+1);
return m;
}

/*****************************************************************************/

void statist_float::print(ostream& st)
{
st << "statist_float::print ist ein dummy"<<endl;
}

/*****************************************************************************/
/*****************************************************************************/
