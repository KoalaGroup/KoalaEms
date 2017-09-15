/*
 * decode_f1_raw.c
 * 
 * created 28.Jun.2005 PW
 * $ZEL: decode_f1_rss1.c,v 1.1 2006/02/17 15:15:21 wuestner Exp $
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
#include <math.h>
#include <values.h>

#include <emsctypes.h>

#define LETRA
#define EXT

#define NUM 16384
#define CHAN 16

#define ch(chan, edge, odd, dir) ((((chan)>>3)<<3)+((edge)<<2)+((odd)<<1)+(dir))
#define ch1(chan, edge, dir) ((((chan)>>3)<<3)+((edge)<<2)+(dir))
/*
   ch  0: chan 4; falling edge; even; dir 0
   ch  1: chan 4; falling edge;  odd; dir 0
   ch  2: chan 4; falling edge; even; dir 1
   ch  3: chan 4; falling edge;  odd; dir 1
   ch  4: chan 4; raising edge; even; dir 0
   ch  5: chan 4; raising edge;  odd; dir 0
   ch  6: chan 4; raising edge; even; dir 1
   ch  7: chan 4; raising edge;  odd; dir 1
   ch  8: chan 8; falling edge; even; dir 0
   ch  9: chan 8; falling edge;  odd; dir 0
   ch 10: chan 8; falling edge; even; dir 1
   ch 11: chan 8; falling edge;  odd; dir 1
   ch 12: chan 8; raising edge; even; dir 0
   ch 13: chan 8; raising edge;  odd; dir 0
   ch 14: chan 8; raising edge; even; dir 1
   ch 15: chan 8; raising edge;  odd; dir 1
*/

struct statist {
    double diff;
    double abs;
    double speed;
    double sum;
    double qsum;
    double min;
    double max;
    double mean;
    double sigma;
    int num;
};

struct statist statist[CHAN][NUM];
int numzeros=0;

static void
init_statist()
{
    int i, j;
    for (i=0; i<NUM; i++) {
        for (j=0; j<4; j++) {
            struct statist* s=&statist[j][i];
            s->diff=0;
            s->abs=0;
            s->speed=0;
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
dump_s(int idx, struct statist* s0, struct statist* s1)
{
    if ((s0->num>0) && (s0->num>01)) {
        printf("%6d %e %e %e %6d %e %e %e %d %d\n",
            idx, s0->abs, s0->diff, s0->speed,
            idx+1, s1->abs, s1->diff, s1->speed,
            s0->num, s1->num);
    }
}

static void
dump_statist(int chan)
{
    int i;
    int num;

    chan&=~1;
    num=NUM-1;
    while ((statist[chan][num].num==0) && (num>0)) num--;
    for (i=0; i+1<num; i++) {
        dump_s(i-num, &statist[chan][i], &statist[chan^2][i]);
    }

    chan|=1;
    num=NUM-1;
    while ((statist[chan][num].num==0) && (num>0)) num--;
    for (i=0; i+1<num; i++) {
        dump_s(i, &statist[chan][i], &statist[chan^2][i]);
    }
}

static void
add_value(int chan, int idx, double abssec, double diffsec, double speed)
{
    if (idx<NUM) {
        struct statist* s=&statist[chan][idx];
/*
        s->sum+=idx;
        s->qsum+=idx*idx;
        if (idx<s->min) s->min=idx;
        if (idx>s->max) s->max=idx;
*/
        s->abs=abssec;
        s->diff=diffsec;
        s->speed=speed;
        s->num++;
    }
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
            if (len!=maxlen*4) {
                if (len<0) {
                    fprintf(stderr, "read: %s\n", strerror(errno));
                    return -1;
                }
                fprintf(stderr, "read: no more data\n");
                return -1;
            }
            len/=4;
            idx=0;
        }
    } while (again);
    return pos;
}

static long long
tdiff(long long t1, long long t0)
{
    long long t;
    t=t1-t0;
#ifdef EXT
    if (t<0)
        t+=223136972800;
#else
    if (t<0)
        t+=3404800;
#endif
    return t;
}

static double
l2sec(long long val)
{
    return (val*120.3)*1e-12;
}

static double
l2speed(long long val)
{
    return 20e-6/((val*120.3)*1e-12);
}

static int
do_word(ems_u32 w, int id)
{
    static long long lasttime[CHAN];
    static long long last0=0, t0;
    static int idx[CHAN], idx1[CHAN], dir=-1;
    static ems_u32 w1;
    int addr, channel, edge;
    long long time_h, time_hh, time_l, time;

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

    if (channel==0) {
        long long diff;
        int i;

        diff=tdiff(time, last0);

        if (!edge || (diff<10000)) /* falsche flanke oder stoerung */
            return 0;

        last0=time;
        numzeros++;

        if (dir<0) {
            dir=1;
            for (i=0; i<CHAN; i++) lasttime[i]=time;
        }
        dir=1-dir;
        for (i=0; i<CHAN; i++) {idx[i]=0; idx1[i]=0;}
        t0=time;

    } else if (dir>=0) { /* channel 4 or 8 and valid direction */
        int chan      =ch(channel, edge, idx1[ch1(channel, edge, dir)]&1, dir);
        long long abs =tdiff(time, t0);
        long long diff=tdiff(time, lasttime[chan]);
        double abssec =l2sec(abs);
        double diffsec=l2sec(diff);
        double speed  =l2speed(diff);

        add_value(chan, idx[chan], abssec, diffsec, speed);
        /*
            if ((chan==ch(4,1,0)) && !(idx[chan]&1))
                printf("%4d %e %e\n", idx[chan],
                        l2sec(tdiff(time, t0)), l2speed(diff));
        */
        idx[chan]++;
        idx1[ch1(channel, edge, dir)]++;
        lasttime[chan]=time;
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
    fprintf(stderr, "len=%d addr=%d id=%d evcount=%d numzeros=%d\n",
        len, addr, id, evcount, numzeros);
        /*if (evcount>=5000000) return -1;*/
    }

    if (len!=res) {
        fprintf(stderr, "event size mismatch: len=%d, header=%d\n", res, len);
        len=res;
    }

    for (i=3; i<len; i++) {
        res=do_word(ev[i], i);
        if (res) return -1;
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
    fprintf(stderr, "numzeros=%d\n", numzeros);
    /*
    calculate_statist();
    */
    dump_statist(ch(4, 1, 0, 0));
    return 0;
}
