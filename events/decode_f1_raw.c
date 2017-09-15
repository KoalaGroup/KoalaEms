/*
 * decode_f1_raw.c
 * 
 * created 28.Jun.2005 PW
 * $ZEL: decode_f1_raw.c,v 1.1 2006/02/17 15:15:19 wuestner Exp $
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
read_event(int p, ems_u32* ev, int max)
{
    static ems_u32 cache[1024];
    static const int maxlen=1024;
    static int idx=0, len=0;
    int again, pos=0;

    do {
        again=0;
        while ((idx<len) && (pos==0 || !(cache[idx]&0x80000000))) {
            /* XXX max is ignored here */
            ev[pos++]=cache[idx++];
        }
        if (idx==len) {
            again=1;
            len=read(p, cache, maxlen*4);
            if (len<0) {
                fprintf(stderr, "read: %s\n", strerror(errno));
                return -1;
            }
            len/=4;
            idx=0;
        }
    } while (again);
    return pos;
}

static int
do_word(ems_u32 w, int idx)
{
    ems_u32 addr, channel;
    int edge;
    long long time_h, time_hh, time_l, time, diff, diff0;
    static long long lasttime[9]={0,0,0,0,0,0,0,0,0};
    static long long last0time=0;
    static ems_u32 w1;

    /*printf("w%d=%08x\n", idx&1, w);*/
#ifdef EXT
    if (idx&1) {
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
#ifdef LETRA
    time_l=w1&0xfffe;
    edge=w1&1;
#else
    time_l=w1&0xffff;
    edge=0;
#endif
    time_h+=time_hh<<6;
    time=time_h*53200+time_l;

    diff=time-lasttime[channel];
    lasttime[channel]=time;

#ifdef EXT
        if (diff<0)
            diff+=223136972800;
#else
        if (diff<0)
            diff+=3404800;
#endif

    if (channel==0) {
        if (edge,1) {
            if (diff>10000, 1) {
                diff0=time-last0time;
                if (diff0<0)
                    diff0+=223136972800;
                last0time=time;
                printf("ch %d edge %d %lld\n", channel, edge, diff0);
            }
        }
    }

    return 0;
}

static int
do_event(int p)
{
    static int nextevent=-1;
    ems_u32 ev[65536];
    ems_u32 len, addr, id, evcount;
    int res, i;

    res=read_event(p, ev, 65536);
    if (res<0)
        return -1;

    if (!(ev[0]&0x80000000)) {
        fprintf(stderr, "not an event header\n");
        return -1;
    }
    if (res<3) {
        fprintf(stderr, "event too short (%d words)\n", res);
        return -1;
    }

    len=(ev[0]&0xffff)/4;
    addr=(ev[1]>>16)&0xffff;
    id=ev[1]&0xffff;
    evcount=ev[2];
    if (evcount!=nextevent) {
         fprintf(stderr, "event %d but %d expected\n", evcount, nextevent);
    }
    nextevent=evcount+1;

    if ((evcount%1000000)==0) {
    fprintf(stderr, "len=%d addr=%d id=%d evcount=%d\n",
        len, addr, id, evcount);
    }

    if (len!=res) {
        fprintf(stderr, "event size mismatch: len=%d, header=%d\n", res, len);
        len=res;
    }

    for (i=3; i<len; i++) {
        do_word(ev[i], i);
    }

    return 0;        
}

int
main(int argc, char* argv[])
{
    int p=0; /* data path */
    int res;

    do {
        res=do_event(p);
    } while (!res);

    return 0;
}
