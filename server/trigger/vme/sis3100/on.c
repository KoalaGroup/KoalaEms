/*
 * trigger/pci/sis1100/sender.c
 * created: 02.Oct.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: on.c,v 1.2 2011/04/06 20:30:36 wuestner Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <dev/pci/sis1100_var.h>
#include <rcs_ids.h>

RCS_REGISTER(cvsid, "trigger/vme/sis3100")

int p, zeit;
unsigned int num;

main(int argc, char* argv[])
{
    u_int32_t v=0x00000f00;
    int res;

    if (argc!=2) {
        fprintf(stderr, "usage: %s device\n", argv[0]);
        return 1;
    }

    p=open(argv[1], O_RDWR);
    if (p<0) {
        fprintf(stderr, "open %s: %s\n", argv[1], strerror(errno));
        return 2;
    }

    res=ioctl(p, SIS1100_FRONT_IO, &v);
    if (res) {
        fprintf(stderr, "ioctl(FRONT_IO, 0x%08x): %s\n",
            v, strerror(errno));
        return 3;
    }
    return 0;
}
