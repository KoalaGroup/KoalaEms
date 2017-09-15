static const char* cvsid __attribute__((unused))=
    "$ZEL: sender.c,v 1.4 2011/04/06 20:30:36 wuestner Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#ifdef __linux__
#include <sys/mman.h>
#include "/home/juelich/wuestner/dev/pci/zelsync.h"
#else
#include <sys/mman.h>
#include <machine/bus.h>
#include <rcs_ids.h>
#include "/usr/local/lkmx/sys/dev/pci/zelsync.h"
#include "/usr/local/lkmx/sys/dev/pci/pcisyncvar.h"
#endif

RCS_REGISTER(cvsid, "trigger/pci/zelsync")

char* path="/tmp/sync0";
int p, zeit;
volatile int *base;

/*****************************************************************************/
static int
syncmod_attach(void)
{
    int offs;

    if ((p=open(path, O_RDWR, 0))<0) {
        fprintf(stderr, "open '%s': %s", path, strerror(errno));
        return -1;
    }

#ifdef __linux__
    if ((base=(int*)mmap(0, 0x40, PROT_READ|PROT_WRITE, MAP_SHARED,
        p, (off_t)0))==(int*)-1) {
            perror("mmap");
            return -1;
    }
#else
    if (ioctl(p, PCISYNC_GETMAPOFFSET, &offs)<0) {
        printf("GETMAPOFFSET: %s\n", strerror(errno));
        close(p);
        return -1;
    }
    if ((base=(int*)mmap(0, 0x40, PROT_READ|PROT_WRITE, 0,
            p, (off_t)offs))==(int*)-1) {
        perror("mmap");
        return -1;
    }
#endif
/* printf("syncmod_map: base=%p\n", base); */
return 0;
}

int
main(int argc, char* argv[])
{
    unsigned int v, i;
    struct timeval tv;

    if (argc<2) {
        fprintf(stderr, "usage: %s time/ms [device]\n", argv[0]);
        exit(1);
    }
    if (argc>2) path=argv[2];
    if (sscanf(argv[1], "%d", &zeit)!=1) {
        fprintf(stderr, "cannot convert '%s' to integer\n", argv[1]);
        exit(1);
    }
    printf("zeit=%d\n", zeit);
    tv.tv_sec=zeit/1000; tv.tv_usec=(zeit%1000)*1000;

    if (syncmod_attach()!=0) exit(1);

    base[SYNC_SSCR]=SYNC_RES_AUX_OUT;
    v=base[SYNC_SSCR];
    if (v&SYNC_GET_AUX_OUT) {
        printf("status nach initial reset: 0x%08x\n", v);
        exit(2);
    }
    if (zeit) {
        for (i=0;;i++) {
            select(0, 0, 0, 0, &tv);
            if (tv.tv_sec>0) {printf("%d ", i); fflush(stdout);}
            base[SYNC_SSCR]=SYNC_SET_AUX_OUT1;
            v=base[SYNC_SSCR];
            if ((v&SYNC_GET_AUX_OUT)!=SYNC_GET_AUX_OUT1) {
                printf("status nach set: 0x%08x\n", v);
                exit(2);
            }
            base[SYNC_SSCR]=SYNC_RES_AUX_OUT1;
            v=base[SYNC_SSCR];
            if (v&SYNC_GET_AUX_OUT) {
                printf("status nach reset: 0x%08x\n", v);
                exit(2);
            }
        }
    } else {
        for (;;) {
            base[SYNC_SSCR]=SYNC_SET_AUX_OUT1;
            base[SYNC_SSCR]=SYNC_RES_AUX_OUT1;
        }
    }
}
