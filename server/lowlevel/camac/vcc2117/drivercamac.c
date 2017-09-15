#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>

#include "../../vicvsb/vicvsbreg.h"

#include "drivercamac.h"

static int camhandle= -1;

int CAMACread(addr)
int addr;
{
    int val;
    lseek(camhandle,addr,SEEK_SET);
    read(camhandle,&val,sizeof(int));
    return(val);
}

void CAMACwrite(addr,val)
int addr,val;
{
    lseek(camhandle,addr,SEEK_SET);
    write(camhandle,&val,sizeof(int));
}

void CAMACcntl(addr)
int addr;
{
    int val;
    lseek(camhandle,addr,SEEK_SET);
    write(camhandle,&val,sizeof(int));
}

errcode camac_low_init(arg)
char *arg;
{
    char *campath;
    struct viccsr r;
    int camstat;
    T(camac_low_init)
    if((!arg)||(cgetstr(arg,"camp",&campath)<0)){
	printf("kein CAMAC-Device gegeben\n");
	return(Err_ArgNum);
    }
    D(D_USER,printf("CAMAC ist %s\n",campath);)
    camhandle=open(campath,O_RDWR,0);
    free(campath);
    if(camhandle<0){
	perror("open CAMAC");
	return(Err_System);
    }
    if((ioctl(camhandle,GETVICSPACE,&camstat)<0)||(camstat)){
	printf("falscher Pfad/Treiber fuer CAMAC\n");
	close(camhandle);
	camhandle= -1;
	return(Err_System);
    }
    r.reg=0;
    if(ioctl(camhandle,READCSR,&r)<0){
	printf("Kann CAMAC nicht erreichen\n");
	close(camhandle);
	camhandle= -1;
	return(Err_System);
    }
    r.val|=0x1006; /* transparent, VIC-Mode */
    ioctl(camhandle,WRITECSR,&r);

    lseek(camhandle,Z2_PB,SEEK_SET);
    read(camhandle,&camstat,sizeof(int));
    camstat|=JUELICH_BIT;   /* deaktivieren */
    lseek(camhandle,Z2_PB,SEEK_SET);
    write(camhandle,&camstat,sizeof(int));

    CCCC();
    CCCZ();

    return(OK);
}

errcode camac_low_done()
{
    if(camhandle!=-1){
	close(camhandle);
	camhandle= -1;
    }
    return(OK);
}
