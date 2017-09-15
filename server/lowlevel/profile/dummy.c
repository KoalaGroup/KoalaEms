static const char* cvsid __attribute__((unused))=
    "$ZEL: dummy.c,v 1.2 2011/04/06 20:30:27 wuestner Exp $";

#include <stdio.h>
#include <errorcodes.h>
#include <rcs_ids.h>

RCS_REGISTER(cvsid, "lowlevel/profile")

errcode init_profile_port(){
  return(OK);
}

void write_profile_port(x)
int x;
{
  printf("Profile out %x\n",x);
}

errcode done_profile_port(){
  return(OK);
}
