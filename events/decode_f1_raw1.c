/*
 * decode_f1_raw.c
 * 
 * created 28.Jun.2005 PW
 * $ZEL: decode_f1_raw1.c,v 1.1 2006/02/17 15:15:19 wuestner Exp $
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>

#include <emsctypes.h>

#define LETRA
#define EXT

static int
do_word(int p)
{
    ems_u32 w, addr, channel;
    int edge, res;
    long long time_h, time_hh, time_l, time, diff;
    static long long lasttime=0;
    static w1;

    res=read(p, &w, 4);
    if (res!=4) {
        perror("read");
        return -1;
    }

#ifdef EXT
    if ((w&0x0fff0000)!=0x0fff0000) {
        w1=w;
        return 0;
    } else {
        time_hh=w&0xffff;
    }
#else
    w1=w;
    time_hh=0;
#endif

    addr=(w1>>28)&0xf;
    channel=(w1>>22)&0x3f;
    time_h=(w1>>16)&0x3f;
    if ((w1&0xffff)==0xffff)
        return 0;

#ifdef LETRA
    time_l=w1&0xfffe;
    edge=w1&1;
#else
    time_l=w1&0xffff;
    edge=0;
#endif
    time_h+=time_hh<<6;

    printf("ch %d edge %d\n", channel, edge);
    if ((channel==0) && (edge==1)) {
        /*
        printf("w1=%08x w0=%08x\n", w1, w);
        printf("time_hh=%llu time_h=%llu time_l=%llu\n", time_hh, time_h, time_l);
        */
        time=time_h*53200+time_l;
        diff=time-lasttime;
#ifdef EXT
        if (diff<0)
            diff+=223136972800;
#else
        if (diff<0)
            diff+=3404800;
#endif
        printf("%llu\n", diff);
        lasttime=time;
    }
    return 0;
}

int
main(int argc, char* argv[])
{
    int p=0; /* data path */
    int res;

    do {
        res=do_word(p);
    } while (!res);

    return 0;
}
