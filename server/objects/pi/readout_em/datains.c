static const char* cvsid __attribute__((unused))=
    "$ZEL: datains.c,v 1.13 2011/04/06 20:30:29 wuestner Exp $";

#include <errno.h>
#include <unistd.h>
#ifdef OSK
/*#include <module.h>*/
#include "../../../lowlevel/vicvsb/vic.h"
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
/*#include <sys/ipc.h>
#include <sys/shm.h>*/
#include "../../../lowlevel/vicvsb/vicvsbreg.h"
#endif

#include <errorcodes.h>
#include <sconf.h>
#include <rcs_ids.h>
#include "../../domain/dom_datain.h"

#include "../../../lowlevel/oscompat/oscompat.h"

#include "rtinfo.h"
#include "datains.h"

RCS_REGISTER(cvsid, "objects/pi/readout_em")

extern ems_u32* outptr;

struct inpinfo roinfo[MAX_DATAIN];
int bufferanz;

typedef union{
/*#ifdef OSK
    mod_exec *mod;
#else
    int *shmaddr;
#endif
*/
modresc mresc;
    int path;
}resourceinfo;

static resourceinfo resc[MAX_DATAIN];

errcode datain_pre_init(int idx, int qlen, ems_u32* q)
{return OK;}

errcode datain_init(i)
int i;
{
    datain_info *di;
    struct inpinfo *inb;
    di= &datain[i];
    inb= &roinfo[bufferanz];
    if(di->bufftyp==InOut_Ringbuffer){
	inb->use_driver=0;
	inb->poll_driver=0;
	switch(di->addrtyp){
	    case Addr_Raw:
		inb->bufstart=di->addr.addr;
		break;
	    case Addr_Modul:{
	      inb->bufstart=link_datamod(di->addr.modulname,&(resc[i].mresc));
	      if(!inb->bufstart){
		*outptr++=errno;
		return(Err_System);
	      }
		break;
		}
	    case Addr_Driver_syscall:
		inb->poll_driver=1;
	    case Addr_Driver_mixed:
		inb->use_driver=1;
	    case Addr_Driver_mapped:{
		int path;
		if((path=resc[i].path=inb->path=
#ifdef OSK
		    open(di->addr.driver.path,3)
#else
		    open(di->addr.driver.path,O_RDWR,0)
#endif
		    )==-1){
		    *outptr++=errno;
		    return(Err_System);
		}
		inb->space=di->addr.driver.space;
#ifdef OSK
		if(inb->poll_driver){
		  _setstat(path,ss_setSpace,di->addr.driver.space);
		}else{
		    struct vic_opts x;
		    _gs_opt(path,&x);
		    if(x.NormalTyp!=di->addr.driver.space){
			close(path);
			*outptr++=E_BTYP;
			return(Err_System);
		    }
		}
#else
		{
		    int space;
                    if (fcntl(path, F_SETFD, FD_CLOEXEC)<0)
                      {
                      printf("datains.c: fcntl(path, FD_CLOEXEC): %s\n", strerror(errno));
                      }
		    if((ioctl(path,GETVICSPACE,&space)<0)||
		       (space!=di->addr.driver.space)){
			close(path);
			*outptr++=EFAULT;
			return(Err_System);
		    }
		}
#endif
		if(di->addr.driver.option&1){
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
		if(di->addr.driver.option&2){
#ifdef OSK
		  int s;
		  if((s=_getstat(path,ss_CSR,0))==-1){
		    *outptr++=errno;
		    close(path);
		    return(Err_System);
		  }
		  _setstat(path,ss_CSR,0,s|0xe);
#else
		  struct viccsr r;
		  r.reg=0;
		  if(ioctl(path,READCSR,&r)<0){
		    *outptr=errno;
		    close(path);
		    return(Err_System);
		  }
		  r.val|=0xe; /* transparent, long */
		  ioctl(path,WRITECSR,&r);
#endif
		}
#ifdef OSK
		if(di->addr.driver.option&4){ /* VME D64 */
		  if(_setstat(path,ss_setSpace,0xc)==-1){
		    *outptr++=errno;
		    close(path);
		    return(Err_System);
		  }
		}
#endif
		if(inb->poll_driver)
		    inb->base=0;
		else{
#ifdef OSK
		    if((inb->base=_getstat(path,ss_getbase))==-1){
			*outptr++=errno;
			close(path);
			return(Err_System);
		    }
#else
		    /* unsupported, memory mappen? */
		    {
			close(path);
			*outptr++=EOPNOTSUPP;
			return(Err_System);
		    }
#endif
		}
		inb->bufstart=(int*)((char*)(inb->base)+di->addr.driver.offset);
		break;
		}
	    default: return(Err_Program);
	}
	inb->letztes_event=0;
	inb->ip=inb->bufstart;
	bufferanz++;
    }
    return(OK);
}

errcode datain_done(i)
int i;
{
    datain_info *di;
    di= &datain[i];
    if(di->bufftyp==InOut_Ringbuffer){
	switch(di->addrtyp)
	{
	    case Addr_Raw:
		break;
	    case Addr_Modul:{
/*#ifdef OSK
		munlink(resc[i].mod);
#else
		shmdt((char*)resc[i].shmaddr);
#endif
*/
	      unlink_datamod(&(resc[i].mresc));
		break;
		}
	    case Addr_Driver_syscall:
	    case Addr_Driver_mixed:
	    case Addr_Driver_mapped:
		close(resc[i].path);
		break;
	    default: return(Err_Program);
	}
    }
    return(OK);
}
/*****************************************************************************/
errcode remove_datain(int idx)
{
printf("objects/pi/readout_em/datains.c:remove_datain(%d) called (dummy).\n",
    idx);
return OK;
}
/*****************************************************************************/
/*****************************************************************************/
