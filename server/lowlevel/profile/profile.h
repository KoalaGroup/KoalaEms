#ifndef _profile_h_
#define _profile_h_

#include <sconf.h>

#ifdef PROFILE
#define PROFILE_START(x) if(profilestab[x])write_profile_port(profileoutput|=profilestab[x][0])
#define PROFILE_START_2(x,y) if(profilestab[x])write_profile_port(profileoutput|=profilestab[x][y])
#define PROFILE_END(x) if(profileetab[x])write_profile_port(profileoutput&=profileetab[x][0])
#define PROFILE_END_2(x,y) if(profileetab[x])write_profile_port(profileoutput&=profileetab[x][y])
#else
#define PROFILE_START(x)
#define PROFILE_START_2(x,y)
#define PROFILE_END(x)
#define PROFILE_END_2(x,y)
#endif

enum{
  PROF_RO,
  PROF_RO_SUB,
  PROF_PROC,
  PROF_OUTALL,
  PROF_OUT,
  PROF_OUTWARP
};

extern int profileoutput;
extern int **profilestab,**profileetab;

#endif
