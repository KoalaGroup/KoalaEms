#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

typedef unsigned int ems_u32;

struct event {
    ems_u32 len;
    ems_u32 evno;
    ems_u32 trigno;
    ems_u32 numofsubevs;
    ems_u32* data;
    int dsize;
};

static int
parity(ems_u32 d)
{
    d^=d>>16;
    d^=d>>8;
    d^=d>>4;
    d^=d>>2;
    d^=d>>1;
    return d&1;
}

static int
test_lc1881(const ems_u32* data, int size, int v)
{
    int header, i, res=0;
    int wc, buffer, address, val, channel, buf, ga;

    if (data[0]!=size-1) {
        if (v)
            printf("wrong size: %d (0x%x), sev size=%d\n",
                    data[0], data[0], size);
        return -1;
    }
    data++; size--;

    header=1;
    for (i=0; i<size; i++) {
        ems_u32 d=data[i];
        if (parity(d)) {
            if (v)
                printf("wrong parity: [%3d] %08x\n", i, d);
            res=-1;
        }
        if (v)
            printf("[%3d] %08x", i, d);

        if (header) {
            wc=d&0x7f;
            buffer=(d>>7)&0x3f;
            address=(d>>27)&0x1f;
            if (v)
                printf(" ga=%2d buffer=%2d wc=%d\n", address, buffer, wc);
            wc--;
            header=0;
        } else {
            val=d&0x3fff;
            channel=(d>>17)&0x7f;
            buf=(d>>24)&0x3;
            ga=(d>>27)&0x1f;
            if (v)
                printf(" ga=%2d buffer=%2d chan=%2d val=%d\n",
                        ga, buf, channel, val);
            if (ga!=address) {
                if (v)
                    printf("wrong address\n");
                res=-1;
            }
            if (buf!=(buffer&3)) {
                if (v)
                    printf("wrong buffer\n");
                res=-1;
            }
            wc--;
            if (wc==0)
                header=1;
        }
    }

    return res;
}

static int
read_event(FILE* f, struct event* event)
{
    int res;
    res=fread(event, 4, 4, f);
    if (res!=4) {
        printf("fread head: %s\n", strerror(errno));
        return -1;
    }
    if (event->dsize<event->len) {
        free(event->data);
        event->data=malloc(event->len*4);
        if (event->data==0) {
            printf("malloc %d words: %s\n", event->len, strerror(errno));
            return -1;
        }
        event->dsize=event->len;
    }
    res=fread(event->data, 4, event->len-3, f);
    if (res!=event->len-3) {
        printf("fread head: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

static int
test_subevent(ems_u32* data, int evnr, int v)
{
    int res=0;

    switch (data[0]) {
    case 23: {
            if (!v)
                res=test_lc1881(data+2, data[1], 0);
            if (res || v) {
                printf("### event %d\n", evnr);
                test_lc1881(data+2, data[1], 1);
            }
        }
        break;
    }
    return 0;
}

static int
test_event(struct event* event, int v)
{
    ems_u32* data=event->data;
    int sev;

    for (sev=0; sev<event->numofsubevs; sev++) {
        int res;

        res=test_subevent(data, event->evno, v);
        if (res<0)
            return -1;
        data+=data[1]+2;
    }
    return 0;
}

int
main(int argc, char* argv[])
{
    struct event event;
    event.data=0;
    event.dsize=0;

printf("argc=%d\n", argc);
    if (argc>1) {
        int evnr;
        evnr=atoi(argv[1]);
printf("evnr=%d\n", evnr);

        if (evnr<0) {
            do {
                if (read_event(stdin, &event)<0)
                    break;
                test_event(&event, 1);
                if (event.evno>=-evnr)
                    break;
            } while (1);
        } else {
            do {
                if (read_event(stdin, &event)<0)
                    break;
                if (evnr==event.evno) {
                    test_event(&event, 1);
                    break;
                }
            } while (1);
        }
    } else {
        int last_evnr=-1;
        int num=0;
        do {
            if (read_event(stdin, &event)<0)
                break;
            last_evnr=event.evno;
            test_event(&event, 0);
            num++;
        } while (1);
        printf("%d events read (%d)\n", num, last_evnr);
    }

    return 0;
}
