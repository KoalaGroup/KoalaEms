/*
 * dump_events.c
 *
 * created: 2010-NOV-22 PW
 * ZEL$
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

static u_int32_t *data=0;
static int dnum=0;

static ssize_t
do_sev(int ifile, FILE *ofile, u_int32_t ev_size)
{
    u_int32_t head[2], sev_size;
    ssize_t res;
    int i;

    if (ev_size<2) {
        fprintf(stderr, "read sev: ev_size=%u\n", ev_size);
        return -1;
    }

    if ((res=read(ifile, head, 8))!=8) {
        fprintf(stderr, "read sev head: res=%lld, errno=%s\n",
            (long long int)res, strerror(errno));
        return -1;
    }
    fprintf(ofile, "SEV %u size %u\n",
        head[0], head[1]);

    sev_size=head[1];
    if (sev_size>ev_size-2) {
        fprintf(stderr, "read sev: ev_size=%u sev_size==%u\n",
            ev_size, sev_size);
        return -1;
    }

    if (dnum<sev_size) {
        if (!(data=(u_int32_t*)realloc(data, sev_size*4))) {
            fprintf(stderr, "read sev: alloc %u words: %s\n",
                sev_size, strerror(errno));
            return -1;
        }
        dnum=sev_size;
    }
    if ((res=read(ifile, data, sev_size*4))!=sev_size*4) {
        fprintf(stderr, "read sev: res=%lld instead of %lld, errno=%s\n",
            (long long int)res, (unsigned long long)(sev_size*4),
            strerror(errno));
        return -1;
    }

    for (i=0; i<sev_size; i++) {
        fprintf(ofile, "[%3i] 0x%08x\n", i, data[i]);
    }
    return 0;
}

static int
do_event(int ifile, FILE *ofile)
{
    u_int32_t head[4], size;
    ssize_t res;
    int sev;

    if ((res=read(ifile, head, 16))!=16) {
        fprintf(stderr, "read ev head: res=%lld, errno=%s\n",
            (long long int)res, strerror(errno));
        return -1;
    }
    fprintf(ofile, "EVENT %u size %u trigno %u subevents %u\n",
        head[1], head[0], head[2], head[3]);
    if (head[0]<3) {
        fprintf(stderr, "read ev: head[0]=%u\n", head[0]);
        return -1;
    }
    size=head[0]-3;
    for (sev=0; sev<head[3]; sev++) {
        res=do_sev(ifile, ofile, size);
        if (res<0)
            return res;
        else
            size-=res;
    }
    return 0;
}

int
main(int argc, char *argv[])
{
    FILE *ofile=stdout;
    int ifile=0;
    int ok;

    do {
        ok=do_event(ifile, ofile)>=0;
    } while (ok);
    return 0;
}
