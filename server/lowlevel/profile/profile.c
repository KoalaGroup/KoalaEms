#include <stdio.h>

#include <errorcodes.h>

static int starttab[]={1,2,4,8,16,32};
static int _starttab[]={-2,-3,-5,-9,-17,-33};
static int donetab[]={0,0,0,0,0,0};
static int eins[]={1,1,1,1,1,1};
static int _eins[]={-2,-2,-2,-2,-2,-2};
static int zwei[]={2,2,2,2,2,2};
static int _zwei[]={-3,-3,-3,-3,-3,-3};
static int drei[]={4,4,4,4,4,4};
static int _drei[]={-5,-5,-5,-5,-5,-5};
static int vier[]={8,8,8,8,8,8};
static int _vier[]={-9,-9,-9,-9,-9,-9};
static int fuenf[]={16,16,16,16,16,16};
static int _fuenf[]={-17,-17,-17,-17,-17,-17};
static int sechs[]={32,32,32,32,32,32};
static int _sechs[]={-33,-33,-33,-33,-33,-33};
static int _fuenfsechs[]={-49,-49,-49,-49,-49,-49};
static int sieben[]={64,64,64,64,64,64};
static int _sieben[]={-65,-65,-65,-65,-65,-65};
static int acht[]={128,128,128,128,128,128};
static int _acht[]={-129,-129,-129,-129,-129,-129};

static int *nulltab[]={
  0,
  0,
  0,
  0,
  0,
  0
};
static int *rostab[]={
  starttab,
  0,
  0,
  0,
  0,
  0
};
static int *roetab[]={
  donetab,
  0,
  0,
  0,
  0,
  0
};
static int *outstab[]={
  0,
  0,
  0,
  sieben,
  starttab,
  0,
};
static int *outetab[]={
  0,
  0,
  0,
  _sieben,
  _starttab,
  0,
};
static int *procstab[]={
  0,
  0,
  starttab,
  0,
  0,
  0
};
static int *procetab[]={
  0,
  0,
  _starttab,
  0,
  0,
  0
};
static int *kombistab[]={
  eins,
  zwei,
  drei,
  vier,
  fuenf,
  sechs
};
static int *kombietab[]={
  _eins,
  _zwei,
  _drei,
  _vier,
  _fuenfsechs,
  0
};

int **profilestab,**profileetab;
int profileoutput;

errcode profile_low_init(arg)
char *arg;
{
  long tab;
  errcode res;
  tab=0;
  if(arg)cgetnum(arg,"proftab",&tab);
  switch(tab){
    case 0:
      profilestab=nulltab;
      profileetab=nulltab;
      break;
    case 1:
      profilestab=rostab;
      profileetab=roetab;
      break;
    case 2:
      profilestab=outstab;
      profileetab=outetab;
      break;
    case 3:
      profilestab=procstab;
      profileetab=procetab;
      break;
    case 4:
      profilestab=kombistab;
      profileetab=kombietab;
      break;
    default:
      printf("falsche Profile-Tabelle %d\n",tab);
      return(Err_ArgRange);
  }
  if(res=init_profile_port(arg))return(res);
  profileoutput=0;
  write_profile_port(0);
  return(OK);
}

errcode profile_low_done()
{
  return(done_profile_port());
}
