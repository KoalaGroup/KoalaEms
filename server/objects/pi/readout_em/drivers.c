static const char* cvsid __attribute__((unused))=
    "$ZEL: drivers.c,v 1.2 2011/04/06 20:30:29 wuestner Exp $";

#include <errno.h>
#ifdef OSK
#include "../../../lowlevel/vicvsb/vic.h"
#else
#include <fcntl.h>
#include "../../../lowlevel/vicvsb/vicvsbreg.h"
#endif
#include <rcs_ids.h>

RCS_REGISTER(cvsid, "objects/pi/readout_em")

#ifdef nochnicht

#define SPACE_VAR -1
#define TYPE_VME 777
#define TYPE_VIC 888
#define TYPE_VIC_LANG 888

struct drinfo{
  int type,mapspace,rwspace;
};

int get_driver_info(path,info)
int path;
struct drinfo *info;
{
#ifdef OSK
  struct vic_opts x;
  _gs_opt(path,&x);
  info->type=0;
  info->mapspace=x.NormalTyp;
  info->rwspace=SPACE_VAR;
#else
  ioctl(path,GETVICSPACE,&space);
#endif
}

int set_driver_rwspace(path,type,space)
int path,rwspace;
{
#ifdef OSK
  if(type==TYPE_VIC){
    _setstat(path,ss_setSpace,space);
    return(0);
  }
#endif
  *outptr++=EOPNOTSUPP;
  return(-1);
}

#endif /* nochnicht */

int process_driver_option(path,type,opt)
int path,opt;
{
  if(opt&1){
#ifdef OSK
    int s;
    if((s=_getstat(path,ss_CSR,0))==-1){
      *outptr++=errno;
      close(path);
      return(Err_System);
    }
    _setstat(path,ss_CSR,0,s|0x1006);
#else
    struct viccsr r;
    r.reg=0;
    if(ioctl(path,READCSR,&r)<0){
      *outptr=errno;
      close(path);
      return(Err_System);
    }
    r.val|=0x1006; /* transparent, VIC-Mode */
    ioctl(path,WRITECSR,&r);
#endif
  }
}
