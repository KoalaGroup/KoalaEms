/*
 * decode_f1_raw.c
 * 
 * created 28.Jun.2005 PW
 * $ZEL: decode_f1_rss.c,v 1.1 2006/02/17 15:15:20 wuestner Exp $
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <math.h>
#include <values.h>

#include <emsctypes.h>

#define LETRA
#define EXT

#define NUM 16384
#define CHAN 8

#define ch(chan, edge, dir) ((((chan)>>3)<<2)+((edge)<<1)+(dir))
/*
   ch 0: chan 4; falling edge; dir 0
   ch 1: chan 4; falling edge; dir 1
   ch 2: chan 4; raising edge; dir 0
   ch 3: chan 4; raising edge; dir 1
   ch 4: chan 8; falling edge; dir 0
   ch 5: chan 8; falling edge; dir 1
   ch 6: chan 8; raising edge; dir 0
   ch 7: chan 8; raising edge; dir 1
*/

struct statist {
    double sum;
    double qsum;
    double min;
    double max;
    double mean;
    double sigma;
    int num;
};

struct statist statist[CHAN][NUM];

static void
init_statist()
{
    int i, j;
    for (i=0; i<NUM; i++) {
        for (j=0; j<4; j++) {
            struct statist* s=&statist[j][i];
            s->sum=0;
            s->qsum=0;
            s->min=MAXDOUBLE;
            s->max=-MAXDOUBLE;
            s->mean=0;
            s->sigma=0;
            s->num=0;
        }
    }
}

static void
calculate_statist()
{
    int i, j;

    for (i=0; i<NUM; i++) {
        for (j=0; j<4; j++) {
            struct statist* s=&statist[j][i];
            if (s->num>0) {
                s->mean=s->sum/s->num;
                s->sigma=sqrt(s->qsum/s->num-s->mean*s->mean);
            }
        }
    }
}

static void
dump_statist(int chan)
{
    int i;

    for (i=0; i<NUM; i+=2) {
        struct statist* s=&statist[chan][i];
        /*printf("%3d %d %f %f\n", i, s->num, s->mean, s->sigma);*/
        if (s->num==0) break;
        printf("%3d %e %e %e\n", i, s->mean, s->min, s->max);
    }
}

static void
add_value(int chan, int idx, long long val)
{
    struct statist* s=&statist[chan][idx];
    const double dist=5*1e-6; /* 10 um */
    double t;
    double speed;

    t=(val*120.3)*1e-12;
    speed=dist/t;

    s->sum+=speed;
    s->qsum+=speed*speed;
    if (speed<s->min) s->min=speed;
    if (speed>s->max) s->max=speed;
    s->num++;
}

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
do_word(ems_u32 w, int id)
{
    ems_u32 addr, channel;
    int edge;
    long long time_h, time_hh, time_l, time, diff;
    static long long lasttime[9]={0,0,0,0,0,0,0,0,0};
    static int idx[CHAN]={-1,-1,-1,-1,-1,-1,-1,-1};
    static int dir=-1;
    static ems_u32 w1;

    /*printf("w%d=%08x\n", idx&1, w);*/
#ifdef EXT
    if (id&1) {
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
    /*
    printf("w1=%08x w0=%08x\n", w1, w);
    printf("time_hh=%llu time_h=%llu time_l=%llu\n", time_hh, time_h, time_l);
    printf("ch %d edge %d %llu\n", channel, edge, diff);
    */

#ifdef EXT
    if (diff<0)
        diff+=223136972800;
#else
    if (diff<0)
        diff+=3404800;
#endif

    if (channel==0) {
        int i;

        if (!edge || (diff<10000)) /* stoerung */
            return 0;
        if (dir<0) dir=1;
        dir=1-dir;
        for (i=0; i<CHAN; i++) idx[i]=0;
    } else /*(channel 4 or 8)*/ {
        if (dir>=0) {
            int chan;
            chan=ch(channel, edge, dir);
            /*printf("chan %d %llu\n", chan, diff);*/
            add_value(chan, idx[chan], diff);
            idx[chan]++;
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
        if (evcount>5000000) return -1;
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

    init_statist();

    do {
        res=do_event(p);
    } while (!res);

    calculate_statist();
    dump_statist(ch(4, 1, 0));

    return 0;
}
