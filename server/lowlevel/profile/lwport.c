#include <stdio.h>

#include <errorcodes.h>
#include <sconf.h>

#ifdef NOVOLATILE
#define VOLATILE
#else
#define VOLATILE volatile
#endif

static VOLATILE int *portadr;

void write_profile_port(x)
int x;
{
  *portadr=x;
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
