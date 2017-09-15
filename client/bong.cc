/*
 * bong.cc
 */

#include "config.h"
#include "cxxcompat.hxx"
#include "bong.hxx"
#include <math.h>
#include <sys/time.h>

#include "versions.hxx"

VERSION("2007-04-18", __FILE__, __DATE__, __TIME__,
"$ZEL: bong.cc,v 1.4 2007/04/18 19:57:52 wuestner Exp $")
#define XVERSION

static int verbose=0;
int bang::map=0;
float bang::g=-0.01;
float bang::f=-0.0005;

/*****************************************************************************/

class rr
  {
  public:
    rr();
    static float r1();
    static float r11();
    static float rf(float);
  };
rr::rr()
{
timeval tv;
gettimeofday(&tv, 0);
srandom(tv.tv_usec);
}
float rr::r1()
{
int r=random();
float fr=static_cast<double>(r)/static_cast<double>(4294967295U);
return fr;
}
float rr::r11()
{
int r=random();
float fr=static_cast<double>(r)/static_cast<double>(4294967295U);
return fr*2-1;
}
float rr::rf(float f)
{
int r=random();
float fr=static_cast<double>(r)/static_cast<double>(4294967295U);
return 1+(fr*2-1)*f;
}

bang::bang(bang* prev, float y, float v, float m, float mst)
:prev(prev), next(0)
{
if (prev)
  {
  prev->next=this;
  id=prev->id+1;
  }
else
  id=0;
status.y=y;
status.v=v;
status.mt=m;
status.mst=mst;
status.pos=static_cast<int>(y);
if (status.pos==24) status.pos=23;
}

int bang::crash_fix(char* name, float y, float* time)
{
if (verbose)
  {
  cerr<<"["<<id<<"] crash_fix for "<<name<<": target.y="<<y<<endl;
  cerr<<"           y="<<status.y<<"; v="<<status.v<<endl;
  }
if (g!=0.)
  {
  float p2=status.v/(g*status.mst); // p2=p/2;
  float q=2*(status.y-y)/(g*status.mst);
  float discr=p2*p2-q;
  if (verbose) cerr<<"p/2="<<p2<<"; q="<<q<<"; disc="<<discr<<endl;
  if (discr<0)
    return 0;
  else
    {
    float t1=-p2+sqrtf(discr);
    float t2=-p2-sqrtf(discr);
    if (verbose) cerr<<"  t1="<<t1<<"; t2="<<t2<<endl;
    if ((t1<0) && (t2>0))
      {
      *time=t2;
      }
    else if ((t2<0) && (t1>0))
      {
      *time=t1;
      }
    else if ((t2>0) && (t1>0))
      {
      *time=t1>t2?t2:t1;
      }
    else
      return 0;
    }
  }
else
  {
  if (status.v==0) return 0;
  float t=(y-status.y)/status.v;
  if (t<=0) return 0;
  *time=t;
  }
if (verbose) cerr<<"  return "<<*time<<endl;
return 1;
}

int bang::crash_moving(state* st, float* time)
{
if (status.v!=st->v) // wird zusammenstossen
  {
  float t=(status.y-st->y)/(st->v-status.v);
  if (t>0)
    {
    *time=t;
    return 1;
    }
  }
return 0;
}

void bang::move(float time, bang** hit)
{
if (verbose)
  {
  cerr<<"["<<id<<"] move: time="<<time;
  if (*hit==this) cerr<<"; crash";
  cerr<<endl;
  }
if (*hit==this)
  {
  switch (crashtype)
    {
    case crash_ceiling:
      //cerr<<"crash_ceiling"<<endl;
      break;
    case crash_floor:
      //cerr<<"crash_floor"<<endl;
      break;
    case crash_upper:
      //cerr<<"crash_upper"<<endl;
      if (next->crashtype!=crash_lower)
        {
        cerr<<"["<<id<<"]next is not crash_lower"<<endl;
        }
      else
        *hit=next;
      break;
    case crash_lower:
      //cerr<<"crash_lower"<<endl;
      break;
    case crash_upspalt:
      //cerr<<"crash_upspalt"<<endl;
      break;
    case crash_downspalt:
      //cerr<<"crash_downspalt"<<endl;
      break;
    default:
      //cerr<<"crash_sonstwie: "<<crashtype<<endl;
      break;
    }
  status.y=crashstate.y;
  status.v=crashstate.v;
  status.pos=crashstate.pos;
  crashed=crashtype;
  }
else
  {
  //cerr<<"["<<id<<"]no crash"<<endl;
  status.y=status.y+time*status.v+time*time*(g*status.mst)/2.;
  status.v=status.v+time*(g*status.mst);
  status.pos=static_cast<int>(status.y);
  if (status.pos==24) status.pos=23;
  crashed=crash_none;
  }
if ((status.pos<0) || (status.pos>23))
  {
  cerr<<"["<<id<<"]bang: pos="<<status.pos<<endl;
  }
else
  map|=1<<status.pos;
}

float bang::check()
{
crashtype=crash_none;
float t;
float spalt;

int verb=verbose;
//verbose=status.pos==19;

if (verbose)
  cerr<<"["<<id<<"]================= bang::check() ========================="<<endl;
if (crashed!=crash_ceiling)
  {
  // naechste Kollision mit einer oberen Subraumspalte oder der Decke
  spalt=ceilf(status.y);
  if (spalt==status.y) spalt+=1.;
  //cerr<<"y="<<status.y<<"; spalt="<<spalt<<endl;
  if (crash_fix("ceiling", spalt, &t))
    {
    if ((crashtype==crash_none) || (crash_time>t))
      {
      crash_time=t;
      crashstate.pos=static_cast<int>(spalt);
      crashstate.y=spalt;
      if (spalt<24)
        {
        crashtype=crash_upspalt;
        crashstate.v=status.v+crash_time*(g*status.mst);
        crashstate.pos=static_cast<int>(spalt);
        }
      else if (spalt==24)
        {
        crashtype=crash_ceiling;
        crashstate.v=-(status.v+crash_time*(g*status.mst))*rr::rf(.001);
        crashstate.pos=23;
        }
      else
        cerr<<"["<<id<<"]ist oben rausgeflogen"<<endl;
      }
    }
  }

if (crashed!=crash_floor)
  {
  // naechste Kollision mit einer unteren Subraumspalte oder dem Boden
  spalt=floorf(status.y);
  if (spalt==status.y) spalt-=1.;
  //cerr<<setprecision(20)<<"y="<<status.y<<"; spalt="<<spalt<<endl;
  if (crash_fix("floor", spalt, &t))
    {
    if ((crashtype==crash_none) || (crash_time>t))
      {
      crash_time=t;
      crashstate.pos=static_cast<int>(spalt)-1;
      crashstate.y=spalt;
      if (spalt>0)
        {
        crashtype=crash_downspalt;
        crashstate.v=status.v+crash_time*(g*status.mst);
        crashstate.pos=static_cast<int>(spalt)-1;
        }
      else if (spalt==0)
        {
        crashtype=crash_floor;
        crashstate.v=-(status.v+crash_time*(g*status.mst))*rr::rf(.001);
        crashstate.pos=0;
        }
      else
        cerr<<"["<<id<<"]ist unten rausgefallen"<<endl;
      }
    }
  }

// if (next && (crashed!=crash_upper)) // oben ist noch einer
//   {
//   if (crash_moving(&next->status, &t))
//     {
//     if ((crashtype==crash_none) || (crash_time>t))
//       {
//       crash_time=t;
//       crashstate.y=status.y+crash_time*status.v+crash_time*crash_time*(g*status.mst)/2.;
//       crashstate.v=status.v*rr::rf(.001);
//       crashstate.pos=(int)(crashstate.y);
//       crashtype=crash_upper;
//       }
//     }
//   }
// 
// if (prev && (crashed!=crash_lower)) // unten ist noch einer
//   {
//   if (crash_moving(&prev->status, &t))
//     {
//     if ((crashtype==crash_none) || (crash_time>t))
//       {
//       crash_time=t;
//       crashstate.y=status.y+crash_time*status.v+crash_time*crash_time*(g*status.mst)/2.;
//       crashstate.v=status.v*rr::rf(.001);
//       crashstate.pos=(int)(crashstate.y);
//       crashtype=crash_lower;
//       }
//     }
//   }

//cerr<<"status for next crash:"<<endl;
//cerr<<"  y="<<crashstate.y;
//cerr<<"  v="<<crashstate.v;
//cerr<<"  pos="<<crashstate.pos<<endl;
//cerr<<setw(2)<<status.pos<<" ("<<crashtype<<"): y="<<status.y<<" t="<<crash_time<<endl;
verbose=verb;
return crash_time;
}

float bang::gg(float ng)
{
float g_=g;
g=ng;
return g_;
}
