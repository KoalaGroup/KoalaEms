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
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <math.h>

#define LETRA
#define EXT

int matrix[16][65536];

static int
xread(int p, ems_u32* d, int l)
{
    int res, ll=l*4;

    res=read(p, d, ll);
    if (res!=ll) {
        if (res<0) {
            perror("read");
        } else {
            fprintf(stderr, "short read\n");
        }
        return -1;
    }
    return 0;
}

static int
do_word(ems_u32 w)
{
    int mod, chan, val;

    mod=(w>>28)&0xf;
    chan=(w>>22)&0x3f;
    val=(w&0xfff);
    /*printf("%d %2d, 0x%03x\n", mod, chan, val);*/
    matrix[chan][val]++;
    return 0;
}

static int
do_sqdc_data(ems_u32* d, int l)
{
    int i;

    if (!(d[0]&0x80000000)) {
        fprintf(stderr, "0x%08x not an event header\n", d[0]);
        return -1;
    }

    /*printf("head=%x event=%d l=%d\n", d[0], d[2], l);*/
    if (l>515) {
        fprintf(stderr, "event %d: len=%d\n", d[2], l);
        l=515;
    }

    for (i=3; i<l; i++)
        do_word(d[i]);

    return 0;
}

static int
do_event(int p)
{
    ems_u32 size;
    ems_u32 *ev;

    if (xread(p, &size, 1)<0) return -1;
    if (!(ev=malloc((size+1)*4))) {
        perror("malloc");
        return -1;
    }
    if (xread(p, ev+1, size)<0) return -1;
    ev[0]=size;

    if (do_sqdc_data(ev+7, size-6)<0) return -1;

    free(ev);
    return 0;
}

int
main(int argc, char* argv[])
{
    int p=0; /* data path */
    int res, i, j;
    long long int num, sum, qsum;
    double average, sigma;

    bzero(matrix, sizeof(matrix));

    do {
        res=do_event(p);
    } while (!res);

    for (i=0; i<16; i++) {
        printf("channel %d\n", i);
        num=0; sum=0; qsum=0;
        for (j=0; j<65536; j++) {
            if (matrix[i][j]) {
                int v=matrix[i][j];
                printf("%d %d\n", j, v);
                num+=v;
                sum+=(long long int)v*(long long int)j;
                qsum+=(long long int)v*(long long int)j*(long long int)j;
            }
        }
        /*printf("num=%lld sum=%lld qsum=%lld\n", num, sum, qsum);*/
        average=(double)sum/(double)num;
        sigma=sqrt((double)qsum/(double)num-average*average);
        printf("num: %lld average=%f sigma=%f\n\n", num, average, sigma);
    }

    return 0;
}
