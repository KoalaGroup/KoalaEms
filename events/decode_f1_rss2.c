/*
 * decode_f1_raw.c
 * 
 * created 28.Jun.2005 PW
 * $ZEL: decode_f1_rss2.c,v 1.1 2006/02/17 15:15:21 wuestner Exp $
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

#define UNIT 2.5e-6 /* 2.6 um */
#define TBIN 100e-6  /* 100 us */
#define RANGEFRACT 50

double Amplitude, Speed;
double estimated_time;
int estimated_tics;

int PNUM;
int TNUM;
#define CHAN 4
#define CACHELEN 1024

int path=0; /* data path */

struct timestamp {
    long long time;
    int channel;
    int edge;
};

#define ch(chan, edge) (((chan)<<1)+((edge)<<0))

struct statval {
    double sum;
    double qsum;
    double min;
    double max;
    double mean;
    double sigma;
    int num;
};

struct pstatist {
    struct statval abs;
    struct statval diff;
    struct statval speed;
};

struct tstatist {
    struct statval x;
};

struct statval amplitude;
struct statval period;

struct pstatist* pstatist;  /* [CHAN][NUM] */
struct tstatist* tstatist;  /* [CHAN][TNUM] */
struct pstatist* _pstatist; /* [CHAN][NUM]  */
struct tstatist* _tstatist; /* [CHAN][TNUM] */

#define STATIST(p, chan, idx) p[idx*CHAN+chan]

static void
clear_statval(struct statval* s)
{
    s->sum=0;
    s->qsum=0;
    s->min=MAXDOUBLE;
    s->max=-MAXDOUBLE;
    s->mean=0;
    s->sigma=0;
    s->num=0;
}

static int
init_statist()
{
    int idx, chan;

    PNUM=estimated_tics+estimated_tics/10; /* +10% */
    fprintf(stderr, "PNUM=%d\n", PNUM);
    pstatist=malloc(sizeof(struct pstatist)*CHAN*PNUM);
    if (pstatist==0) {
        perror("malloc");
        return -1;
    }

    TNUM=(int)((estimated_time/TBIN)*1.1); /* +10% */
    fprintf(stderr, "TNUM=%d\n", TNUM);
    tstatist=malloc(sizeof(struct tstatist)*CHAN*TNUM);
    if (tstatist==0) {
        perror("malloc");
        return -1;
    }

    _pstatist=malloc(sizeof(struct pstatist)*CHAN*PNUM);
    if (_pstatist==0) {
        perror("malloc");
        return -1;
    }

    _tstatist=malloc(sizeof(struct tstatist)*CHAN*TNUM);
    if (_tstatist==0) {
        perror("malloc");
        return -1;
    }

    clear_statval(&amplitude);
    clear_statval(&period);

    for (idx=0; idx<PNUM; idx++) {
        for (chan=0; chan<CHAN; chan++) {
            struct pstatist* s=&STATIST(pstatist, chan, idx);
            struct pstatist* _s=&STATIST(_pstatist, chan, idx);
            clear_statval(&s->abs);
            clear_statval(&s->diff);
            clear_statval(&s->speed);
            clear_statval(&_s->abs);
            clear_statval(&_s->diff);
            clear_statval(&_s->speed);
        }
    }
    for (idx=0; idx<TNUM; idx++) {
        for (chan=0; chan<CHAN; chan++) {
            struct tstatist* s=&STATIST(tstatist, chan, idx);
            struct tstatist* _s=&STATIST(_tstatist, chan, idx);
            clear_statval(&s->x);
            clear_statval(&_s->x);
        }
    }

    return 0;
}

static int
clear_temp_statist()
{
    int idx, chan;

    for (idx=0; idx<PNUM; idx++) {
        for (chan=0; chan<CHAN; chan++) {
            struct pstatist* _s=&STATIST(_pstatist, chan, idx);
            clear_statval(&_s->abs);
            clear_statval(&_s->diff);
            clear_statval(&_s->speed);
        }
    }
    for (idx=0; idx<TNUM; idx++) {
        for (chan=0; chan<CHAN; chan++) {
            struct tstatist* _s=&STATIST(_tstatist, chan, idx);
            clear_statval(&_s->x);
        }
    }
    return 0;
}

static void
calculate_statval(struct statval* s)
{
    if (s->num>0) {
        s->mean=s->sum/s->num;
        s->sigma=sqrt(s->qsum/s->num-s->mean*s->mean);
    }
    
}

static void
calculate_statist()
{
    int idx, chan;

    calculate_statval(&amplitude);
    calculate_statval(&period);
    for (idx=0; idx<PNUM; idx++) {
        for (chan=0; chan<CHAN; chan++) {
            struct pstatist* s=&STATIST(pstatist, chan, idx);
            calculate_statval(&s->abs);
            calculate_statval(&s->diff);
            calculate_statval(&s->speed);
        }
    }
    for (idx=0; idx<TNUM; idx++) {
        for (chan=0; chan<CHAN; chan++) {
            struct tstatist* s=&STATIST(tstatist, chan, idx);
            calculate_statval(&s->x);
        }
    }
}

static void
dump_statval(struct statval* s, const char* name)
{
    fprintf(stderr, "%s: mean=%e, sigma=%e, min=%e, max=%e\n", name, s->mean,
        s->sigma, s->min, s->max);
}
static void
dump_ps(int idx, struct pstatist* s)
{
    if (s->diff.num>0)
    printf("%e %e %e %e\n", idx*UNIT, s->diff.mean, s->diff.min, s->diff.max);
/*
    if (s->speed.num>0)
    printf("%e %e %e %e\n", idx*UNIT, s->speed.mean, s->speed.min, s->speed.max);
*/
}
/*
static void
dump_ts(int idx, struct tstatist* s)
{
    if  (s->x.num) {
        printf("%e %e %e %e %d\n", idx*TBIN, s->x.mean,
            s->x.min, s->x.max, s->x.num);
    }
}
*/
static void
dump_pstatist(int chan)
{
    int i;
    int num;

    num=PNUM-1;
    while ((STATIST(pstatist, chan, num).speed.num==0) && (num>0)) num--;
    for (i=0; i<=num; i++) {
        dump_ps(i, &STATIST(pstatist, chan, i));
    }

}
/*
static void
dump_tstatist(int chan)
{
    int i;
    int num;

    num=TNUM-1;
    while ((STATIST(tstatist, chan, num).x.num==0) && (num>0)) num--;
    for (i=0; i<=num; i++) {
        dump_ts(i, &STATIST(tstatist, chan, i));
    }
}
*/

static void
merge_statval(struct statval* dest, struct statval* source)
{
    dest->sum+=source->sum;
    dest->qsum+=source->qsum;
    if (source->min<dest->min) dest->min=source->min;
    if (source->max>dest->max) dest->max=source->max;
    dest->num+=source->num;
}

static void
merge_statist(void)
{
    int idx, chan;

    for (idx=0; idx<PNUM; idx++) {
        for (chan=0; chan<CHAN; chan++) {
            struct pstatist* s=&STATIST(pstatist, chan, idx);
            struct pstatist* _s=&STATIST(_pstatist, chan, idx);
            merge_statval(&s->abs, &_s->abs);
            merge_statval(&s->diff, &_s->diff);
            merge_statval(&s->speed, &_s->speed);
        }
    }
    for (idx=0; idx<TNUM; idx++) {
        for (chan=0; chan<CHAN; chan++) {
            struct tstatist* s=&STATIST(tstatist, chan, idx);
            struct tstatist* _s=&STATIST(_tstatist, chan, idx);
            merge_statval(&s->x, &_s->x);
        }
    }
}

static void
add_statval(struct statval* s, double val)
{
    s->sum+=val;
    s->qsum+=val*val;
    if (val<s->min) s->min=val;
    if (val>s->max) s->max=val;
    s->num++;
}

static void
add_pvalue(int chan, int idx, double abssec, double diffsec, double speed)
{
    if (idx<PNUM) {
        struct pstatist* s=&STATIST(_pstatist, chan, idx);
        add_statval(&s->abs, abssec);
        add_statval(&s->diff, diffsec);
        add_statval(&s->speed, speed);
    } else {
        fprintf(stderr, "Overflow: idx=%d\n", idx);
        exit(0);
    }
}

static void
add_tvalue(int chan, double time, double x)
{
    unsigned int tidx;
    if (time<0)
        return;
    tidx=(int)(time/TBIN);
    if (tidx<TNUM) {
        struct tstatist* s=&STATIST(_tstatist, chan, tidx);
        add_statval(&s->x, x);
    } else {
        fprintf(stderr, "Overflow: time=%e tidx=%d\n", time, tidx);
        exit(0);
    }
}

static long long
tdiff(long long t1, long long t0)
{
    long long t;
    t=t1-t0;
#ifdef EXT
    /* wrap over if (t < -10s) */
    if (t<-83125519534)
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
    /* distance is 8 tics == 20um */
    return (8*UNIT)/(val*120.3e-12);
}

static int
get_word(ems_u32 *w)
{
    static ems_u32 cache[CACHELEN];
    static int idx=0, len=0;

    if (idx==len) {
        len=read(path, cache, CACHELEN*4);
        if (len<0) {
            fprintf(stderr, "read: %s\n", strerror(errno));
            return -1;
        }
        len/=4;
        idx=0;
        if (len==0) {
            fprintf(stderr, "no more data\n");
            return -1;
        }
    }

    *w=cache[idx++];
    return 0;
}
/*
static const char*
print_timestamp(struct timestamp* ts)
{
    static char s[32];
    snprintf(s, 32, "%d %d %010llx", ts->channel, ts->edge, ts->time);
    return s;
}
*/
static int
get_timestamp(struct timestamp* stamp)
{
    long long time_h, time_hh, time_l;
    ems_u32 w0, w1;

    if (get_word(&w0)<0)
        return -1;

    if (w0&0x80000000) { /* skip header */
        if (get_word(&w0)<0) /* skip card ID */
            return -1;
        if (get_word(&w0)<0) /* skip event number */
            return -1;
        if (get_word(&w0)<0)
            return -1;
        if (w0&0x80000000) {
            printf("empty event!\n");
            return -1;
        }
    }
#ifdef EXT
    if (get_word(&w1)<0)
        return -1;
    if ((w1&0x80000000) || ((w1&0x0fff0000)!=0x0fff0000)) {
        printf("invalid second word: %08x\n", w1);
        return -1;
    }
    time_hh=w1&0xffff;
#else
    time_hh=0;
#endif
#ifdef LETRA
    time_l=w0&0xfffe;
    stamp->edge=w0&1;
#else
    time_l=w0&0xffff;
    stamp->edge=0;
#endif
    time_h=(w0>>16)&0x3f;
    time_h+=time_hh<<6;
    stamp->time=time_h*53200+time_l;
    stamp->channel=(w0>>22)&0x3f;

    return 0;
}

struct timestamp stamps[16];

static int
prefetch_timestamps(void)
{
    int i, j;

    for (i=0; i<16; i++)
        stamps[i].time=0;
    for (i=0; i<15; i++) {
        struct timestamp ts;
        int p;

        if (get_timestamp(&ts)<0)
            return -1;
        p=15;
        while (stamps[p].time>ts.time)
            p--;
        for (j=1; j<=p; j++)
            stamps[j-1]=stamps[j];
        stamps[p]=ts;
    }

    return 0;
}

static int
get_timestamp_sorted(struct timestamp* w)
{
/*
Normally stamps[1..15] are valid and sorted. stamps[0] is zero;
We have to fetch a new timestamp and to find the correct position p for it
in stamps.
Then we move the entries 1..p downwards, insert the new timestamp at position
p and can return stamps[0].
*/
    struct timestamp ts;
    int p, i;

    if (get_timestamp(&ts)<0)
        return -1;
    p=15;
    while (stamps[p].time>ts.time)
        p--;

    for (i=1; i<=p; i++)
        stamps[i-1]=stamps[i];
    stamps[p]=ts;

    *w=stamps[0];
    stamps[0].time=0;
    return 0;
}

struct status {
    long long t0;
    long long tzero;
    long long zerodiff;
    int level[2];
    int dir;
    int loop;
    int start_valid;

    int cidx[CHAN];
    int idx0;
    int ipos;
};

static struct status ss;

static void
init_status(void)
{
    ss.level[0]=-1;
    ss.level[1]=-1;
    ss.dir=-1;
    ss.loop=0;
    ss.start_valid=0;
    ss.tzero=0;
    ss.zerodiff=0;
}

static void 
check_dir(struct timestamp* ts)
{
    int xdir, channel, event, i;
    int loop_valid;

    channel=ts->channel>>3; /* channel 8 --> 1; channel 4 --> 0 */
    ss.level[channel]=ts->edge;
    if (ss.level[1-channel]<0)  /* level of other channel not yet known */
        return;

    event=ss.level[1-channel]^ts->edge;
    xdir=event^channel;

    if (xdir==ss.dir)           /* direction not changed */
        return;

    ss.dir=xdir;
/*
    fprintf(stderr, "loop %d idx %d time %e dir=%d\n", ss.loop, ss.idx0,
            l2sec(tdiff(ts->time, ss.t0.time)), ss.dir);
*/
    if (ss.dir==0) {
        loop_valid=(abs(estimated_tics-ss.idx0)<estimated_tics/RANGEFRACT);
        if (!loop_valid)
            fprintf(stderr, "invalid loop %d: idx0(%d) not in B-range (%d-%d)\n",
                ss.loop, ss.idx0,
                estimated_tics-estimated_tics/RANGEFRACT,
                estimated_tics+estimated_tics/RANGEFRACT);
        if (loop_valid && ss.start_valid) {
            merge_statist();
        }
        clear_temp_statist();
        if (ss.loop>3) {
            if (ss.tzero>0) {
                if (ss.zerodiff==0) {
                    ss.zerodiff=tdiff(ts->time, ss.tzero);
                    fprintf(stderr, "zerodiff=%lld %e\n",
                        ss.zerodiff, l2sec(ss.zerodiff));
                    ss.t0=ts->time;
                } else {
                    long long int t0=ss.tzero+ss.zerodiff;
                    if (loop_valid && ss.start_valid) {
                        add_statval(&amplitude, ss.idx0);
                        add_statval(&period, l2sec(tdiff(t0, ss.t0)));
                    }
                    ss.t0=t0;
                }
                ss.start_valid=1;
            } else {
                fprintf(stderr, "ss.tzero<=0\n");
                ss.start_valid=0;
            }
        }
        for (i=0; i<CHAN; i++)
            ss.cidx[i]=0;
        ss.idx0=0;
        ss.ipos=0;
        ss.loop++;
        ss.tzero=0;
    } else { /* dir=1 */
        loop_valid=(abs(estimated_tics/2-ss.idx0)<estimated_tics/RANGEFRACT);
        if (!loop_valid) {
            fprintf(stderr, "invalid loop %d: idx0(%d) not in A-range (%d-%d)\n",
                ss.loop, ss.idx0,
                estimated_tics/2-estimated_tics/RANGEFRACT,
                estimated_tics/2+estimated_tics/RANGEFRACT);
            ss.start_valid=0;
        }
    }
    if (!ss.start_valid) {
        fprintf(stderr, "start_valid=0 loop %d dir %d\n", ss.loop, ss.dir);
    }
    /*if (channel) ss.start_valid=0;*/
}

static void 
check_zero(struct timestamp* ts)
{
    if ((ts->edge!=0) || (ss.dir!=1))
        return;

    /*if ((ss.idx0<62250) || (ss.idx0>62280)) {*//* 50mm_1.0m2e */
    /*if ((ss.idx0<61700) || (ss.idx0>61730)) {*/ /* 50mm_3.4m2e */
    if ((ss.idx0<61995) || (ss.idx0>62025)) { /* 50mm_3.4m2e */
        fprintf(stderr, "check_zero: invalid idx0=%d\n", ss.idx0);
    } else {
        ss.tzero=ts->time;
    }
}

static int
do_word(void)
{
    struct timestamp ts;
    static long long lasttime[CHAN][2];
    long long diff, abs;
    int channel, chan, nidx;
    double abssec, diffsec, speed, pos, ppos;

    if (get_timestamp_sorted(&ts)<0)
        return -1;

    if (ts.channel==0) {
        check_zero(&ts);
        return 0;
    }

    channel=ts.channel>>3; /* channel 8 --> 1; channel 4 --> 0 */

    ss.idx0++;
    check_dir(&ts);

    chan=ch(channel, ts.edge);
    nidx   =ss.cidx[chan]+1;
    pos    =ss.idx0*UNIT;
    ppos   =ss.ipos*UNIT;
    abs    =tdiff(ts.time, ss.t0);
    diff   =tdiff(ts.time, lasttime[chan][nidx&1]);
    abssec =l2sec(abs);
    diffsec=l2sec(diff);
    speed  =l2speed(diff);

    if (ss.start_valid && (nidx&1)) {
        add_pvalue(chan, ss.idx0, abssec, diffsec, speed);
        add_tvalue(chan, abssec, ppos);
    }

    ss.cidx[chan]=nidx;
    lasttime[chan][nidx&1]=ts.time;
    if (ss.dir)
        ss.ipos--;
    else
        ss.ipos++;

    return 0;
}

static void
calculate_sinus(void)
{
    double a, b, c, d;

    fprintf(stderr, "f(x)=a*cos(b*x+c)+d\n");

    a=-amplitude.mean*UNIT/4;
    b=2*M_PI/period.mean;
    c=0.;
    d=amplitude.mean*UNIT/4;

    fprintf(stderr, "a=%f b=%f c=%f d=%f\n", a, b, c, d);
}

int
main(int argc, char* argv[])
{
    extern char *optarg;
    extern int optind, opterr, optopt;
    int res, c, err;

    Amplitude=0;
    Speed=0;
    while (!err && ((c=getopt(argc, argv, "a:s:")) != -1)) {
        switch (c) {
        case 'a':
            Amplitude=strtod(optarg, 0);
            break;
        case 's':
            Speed=strtod(optarg, 0);
            break;
        default: err=1;
        }
    }
    if ((Amplitude==0) || (Speed==0)) err=1;
    if (err) {
        fprintf(stderr, "usage: %s -a amplitude/mm -s speed/(m/s)", argv[0]);
        return 1;
    }

    Amplitude/=1000.; /* use m instead of mm */

    estimated_tics=(Amplitude*4)/UNIT;
    estimated_time=(Amplitude*2*M_PI)/Speed;

    fprintf(stderr, "amplitude=%f speed=%f: tics=%d period=%f\n",
        Amplitude, Speed, estimated_tics, estimated_time);

    if (init_statist()<0)
        return 2;

    if (prefetch_timestamps()<0)
        return -1;

    init_status();
    do {
        res=do_word();
    } while (!res);

    calculate_statist();
    dump_statval(&amplitude, "amp");
    fprintf(stderr, "Amplitude is %f mm\n", amplitude.mean*UNIT/4*1000);
    dump_statval(&period, "per");
    fprintf(stderr, "Period is %f s\n", period.mean);
    fprintf(stderr, "maximum speed is %f m/s\n",
        amplitude.mean*UNIT/4*2*M_PI/period.mean);

    calculate_sinus();

    dump_pstatist(ch(0, 0));

    return 0;
}
