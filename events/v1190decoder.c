/*
 * v1190decoder.c
 * created 2012-02-17 PW
 * $ZEL: v1190decoder.c,v 1.1 2012/03/12 14:14:42 wuestner Exp $
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

static double scale=195.3125e-6; /* TDC increment in us */

static struct timeval tv_event; /* the timestamp of the current event */
static struct timeval tv_start; /* the timestamp of the first event */
static long long last_hit;      /* the last TDC value of the current event */
static long long start_hit;     /* the last TDC value of the first event */

static long long last[128];     /* the last TDC value for each channel */
static long long corr=0;        /* the accumulated overflows */
static int overflows=0;
static int event, word;

struct stackent {
    int ev;
    int word;
    int chan;
    int val;
    long long cval;
    long long diff;
    const char *txt;
};
struct stackent stack[10];

static void init(void)
{
    int i;
    for (i=0; i<128; i++)
        last[i]=-1;
    tv_start.tv_sec=0;
    tv_start.tv_usec=0;
    start_hit=0;
    memset(stack, 0, sizeof(stack));
}

static void
stack_it(int ev, int word, int chan, int val, long long cval, long long diff)
{
    int i;
    for (i=9; i>0; i--)
        stack[i]=stack[i-1];
    stack[0].ev=ev;
    stack[0].word=word;
    stack[0].chan=chan;
    stack[0].val=val;
    stack[0].cval=cval;
    stack[0].diff=diff;
    stack[0].txt=0;
}

static void
mark_it(const char *txt)
{
    stack[0].txt=txt;
}

static void
dump_stack(int x)
{
    int i;
    for (i=x?9:0; i>=0; i--) {
        printf("[%4d|%4d] chan=%3d val=%6d %10lld diff %7lld %s\n",
                stack[i].ev,
                stack[i].word,
                stack[i].chan,
                stack[i].val,
                stack[i].cval,
                stack[i].diff,
                stack[i].txt?stack[i].txt:"");
    }
    if (x)
        printf("\n");
}

static int
do_afterevent(void)
{
/*
 * last_hit should correspond to the timestamp of the event
 * 
 * first time we are called we just store the timestamp and the value
 * of the last hit of the event
 * 
 * for the following events we can calculate the differences and
 * compare them
 */

    struct timeval tv_diff;
    double tdc_usecs;
    double tv_usecs;
    double diff, quot;

return 0;

    if (tv_start.tv_sec==0) { /* first event */
        tv_start=tv_event;
        start_hit=last_hit;
        return 0;
    }

    tdc_usecs=last_hit*scale;
    timersub(&tv_event, &tv_start, &tv_diff);
    tv_usecs=tv_diff.tv_sec*1000000+tv_diff.tv_usec;

    diff=tv_usecs-tdc_usecs;
    quot=diff/tv_usecs;

    //printf("event %3d last %lld usecs %f ovl %d\n",
    //        event, last_hit, tdc_usecs, overflows);
    printf("event %3d tdc-tv: %f %f %f\n", event, tv_usecs, diff, quot);

    return 0;
}

static int
do_word(u_int32_t d, int print)
{
    static int num=0, max=300;
    static int oldval=0, oldchan=-1;

    int edge, chan, val, cc=0;
    long long cval, diff;

    if (d & 0xf8000000) {
        switch ((d>>27)&0x1f) {
        case 0x4: {
                int group;
                int id=d&0x7fff;
                int tdc=(d>>24)&3;
                printf("ERROR: tdc %d code %04x:", tdc, id);
                for (group=0; group<4; group++) {
                    int dd=d>>(3*group);
                    if ((dd>>0)&1)
                        printf(" Hit lost group %d from FIFO", group);
                    if ((dd>>1)&1)
                        printf(" Hit lost group %d from L1", group);
                    if ((dd>>2)&1)
                        printf(" Hit error in group %d", group);
                }
                if ((d>>12)&1)
                    printf(" event size limit reached");
                if ((d>>13)&1)
                    printf(" event lost");
                if ((d>>14)&1)
                    printf(" fatal chip error");
                printf("\n");
                return 0;
            }
            break;
        default:
            printf("[%4d|%4d] unrecognized word: %08x\n", event, word, d);
            return -1;
        }
    }

    edge=(d>>26)&0x1;
    chan=(d>>19)&0x7f;
    val=d&0x7ffff;
    cval=corr+val;

    /*
     * because of a strange error (duplicated data word) we have to do
     * the following test
     */
    if (val==oldval && chan==oldchan) {
        printf("[%4d|%4d] duplicated word\n", event, word);
        return 0;
    }
    oldval=val;
    oldchan=chan;


    if (last[chan]>=0) {
        diff=cval-last[chan];
        if (diff<=0) {
            corr+=1<<19;
            overflows++;
            cval=corr+val;
            diff=cval-last[chan];
            cc=1;
        }

        stack_it(event, word, chan, val, cval, diff);

        print=0;
        if (num++<max)
            print=1;
#if 0
        if (chan==1 && (diff<254710 || diff>254980)) {
            mark_it("err1");
            print=2;
        }
        if (chan==2 && (diff<51195 || diff>51205)) {
            mark_it("err2");
            print=2;
        }
#endif

        if (print)
            dump_stack(print>1);
    } else {
        printf("[%4d|%4d] chan=%3d skipped\n", event, word, chan);
    }

    last[chan]=cval;
    last_hit=cval;

    return 0;
}

static int
do_subevent_timestamp(u_int32_t *buf)
{
    if (buf[1]!=2) {
        fprintf(stderr, "illegal word counter %d\n", buf[1]);
        return -1;
    }

    tv_event.tv_sec=buf[2];
    tv_event.tv_usec=buf[3];

    return 0;
}

static int
do_subevent_1190(u_int32_t *buf)
{
    int i, n;

    if (buf[1]!=buf[2]+1) {
        fprintf(stderr, "illegal word counter %d | %d\n", buf[1], buf[2]);
        return -1;
    }

    word=0;
    n=buf[2]+3;
    for (i=3; i<n; i++) {
        if (do_word(buf[i], i<10 || i>n-4)<0)
            return -1;
        word++;
    }

    return 0;
}

static int
do_subevent(u_int32_t *buf)
{
    switch (buf[0]) {
    case 50:
        return do_subevent_1190(buf);
    case 100:
        return do_subevent_timestamp(buf);
    default:
        fprintf(stderr, "illegal subevent %d\n", buf[0]);
        return -1;
    }
}

static int
do_event(void)
{
    static u_int32_t *buf=0;
    static int bufsize=0;

    u_int32_t head[4];
    int res, n, idx, sev;

    /* read header */
    if ((res=read(0, head, 16))!=16) {
        if (res<0) {
            fprintf(stderr, "read header: %s\n", strerror(errno));
        } else {
            if (res>0)
                fprintf(stderr, "read header: unexpected end of file\n");
            else
                fprintf(stderr, "no more data\n");
        }
        return -1;
    }

    /* allocate buffer if necessary */
    if (bufsize<head[0]) {
        free(buf);
        bufsize=head[0];
        buf=(u_int32_t*)malloc(bufsize*4);
        if (!buf) {
            fprintf(stderr, "allocate buffer for %d words: %s\n",
                    bufsize , strerror(errno));
            return -1;
        }
    }

    /* read data */
    n=(head[0]-3)*4;
    if ((res=read(0, buf, n))!=n) {
        if (res<0)
            fprintf(stderr, "read data: %s\n", strerror(errno));
        else
            fprintf(stderr, "unexpected end of file\n");
        return -1;
    }

    /* skip first event */
    if (head[1]<2)
        return 0;

    event=head[1];

    idx=0;
    for (sev=0; sev<head[3]; sev++) {
        if (idx>=head[0]-3) {
            fprintf(stderr, "data error\n");
            return -1;
        }
        if (do_subevent(buf+idx)<0)
            return -1;
        idx+=buf[idx+1]+2;
    }

    if (do_afterevent()<0)
        return -1;

    return 0;
}

int main(int argc, char *argv[])
{
    int res;

    init();

    do {
        res=do_event();
    } while (!res);

    return 0;
}
