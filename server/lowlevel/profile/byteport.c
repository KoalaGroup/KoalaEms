static const char* cvsid __attribute__((unused))=
    "$ZEL: byteport.c,v 1.3 2011/04/06 20:30:27 wuestner Exp $";

#include <stdio.h>

#include <errorcodes.h>
#include <sconf.h>
#include <rcs_ids.h>

#ifdef NOVOLATILE
#define VOLATILE
#else
#define VOLATILE volatile
#endif

RCS_REGISTER(cvsid, "lowlevel/profile")

static VOLATILE char *portadr;

void write_profile_port(x)
int x;
{
  *portadr=(char)x;
}

errcode init_profile_port(arg)
char *arg;
{
  if((arg)&&(!cgetnum(arg,"profport",&portadr))){
    printf("profile: benutze Port %x\n",portadr);
    return(OK);
  }else{
    printf("init_profile_port: keine Adresse (profport)\n");
    return(Err_ArgRange);
  }
}

errcode done_profile_port(){
  return(OK);
}
