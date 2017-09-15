/*
 * trigger/vme/sis3100/sender.c
 * created: 02.Oct.2003 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sender3400.c,v 1.2 2011/04/06 20:30:36 wuestner Exp $";

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
    unsigned int v, w;
    struct timeval tv;

    if (argc!=4) {
        fprintf(stderr, "usage: %s time/ms device\n", argv[0]);
        return 1;
    }

    if (sscanf(argv[1], "%d", &zeit)!=1) {
        fprintf(stderr, "cannot convert '%s' to integer\n", argv[1]);
        return 1;
    }
    printf("zeit=%d\n", zeit);
    tv.tv_sec=zeit/1000; tv.tv_usec=(zeit%1000)*1000;

    if (sscanf(argv[2], "%d", &num)!=1) {
        fprintf(stderr, "cannot convert '%s' to integer\n", argv[2]);
        return 1;
    }
    printf("num=%d\n", num);

    p=open(argv[3], O_RDWR);
    if (p<0) {
        fprintf(stderr, "open %s: %s\n", argv[3], strerror(errno));
        return 2;
    }

    if (zeit) {
        for (; num; num--) {
            struct timeval tv1;
            u_int32_t v;
            int res;

            tv1=tv;
            res=select(0, 0, 0, 0, &tv1);
            if (res) {
                fprintf(stderr, "select: %s\n", strerror(errno));
                return 3;
            }
            v=0x08000600;
            res=ioctl(p, SIS1100_FRONT_IO, &v);
            if (res) {
                fprintf(stderr, "ioctl(FRONT_IO, 0x%08x): %s\n",
                        v, strerror(errno));
                return 3;
            }
            tv1=tv;
            select(0, 0, 0, 0, &tv1);
            v=0x06000800;
            res=ioctl(p, SIS1100_FRONT_IO, &v);
            if (res) {
                fprintf(stderr, "ioctl(FRONT_IO, 0x%08x): %s\n",
                        v, strerror(errno));
                return 3;
            }
            if (!(num&7)) {
                struct timespec ts;
                u_int32_t v;
                v=0x00000100;
                ioctl(p, SIS1100_FRONT_IO, &v);
                ts.tv_sec=0;
                ts.tv_nsec=100000;
                nanosleep(&ts, 0);
                v=0x01000000;
                ioctl(p, SIS1100_FRONT_IO, &v);
            }
        }
    } else {
        u_int32_t v;
/*
 *         u_int32_t v=0x08000600;
 *         for (;;) {
 *             ioctl(p, SIS1100_FRONT_IO, &v);
 *             v^=0x0e000e00;
 *         }
 */
        for (; num; num--) {
            if (!(num&7)) {
                u_int32_t v;
                v=0x00000100;
                ioctl(p, SIS1100_FRONT_IO, &v);
                v=0x01000000;
                ioctl(p, SIS1100_FRONT_IO, &v);
            }
            v=0x08000600;
            ioctl(p, SIS1100_FRONT_IO, &v);
            v=0x06000800;
            ioctl(p, SIS1100_FRONT_IO, &v);
        }
    }
    return 0;
}
