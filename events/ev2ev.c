/*
 * events/ev2ev.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL$";

#ifdef __linux__
#define LINUX_LARGEFILE O_LARGEFILE
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#else
#define LINUX_LARGEFILE 0
#endif

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ev2ev.h"

const char *infile, *outfile;
int verbose=0;

#define SWAP_32(n) (((n & 0xff000000) >> 24) | \
    ((n & 0x00ff0000) >>  8) | \
    ((n & 0x0000ff00) <<  8) | \
    ((n & 0x000000ff) << 24))

/*****************************************************************************/
static void
usage(const char *argv0)
{
    fprintf(stderr, "usage: %s [-h] [-v] <infile> <outfile>\n", argv0);
}
/*****************************************************************************/
static int
read_args(int argc, char *argv[])
{
    extern char *optarg;
    extern int optind, opterr, optopt;

    int err=0, c;

    while (!err && ((c = getopt(argc, argv, "hv")) != -1)) {
        switch (c) {
        case 'v':
            verbose=1;
            break;
        case 'h':
            usage(argv[0]);
            err=1;
            break;
        default:
            err=1;
        }
        if (err)
            return -1;
    }

    if (argc>optind) {
        infile=strcmp(argv[optind], "-")?argv[optind]:0;
        optind++;
    } else {
        infile=0;
    }
    if (argc>optind) {
        outfile=strcmp(argv[optind], "-")?argv[optind]:0;
        optind++;
    } else {
        outfile=0;
    }
    if (argc>optind) {
        fprintf(stderr, "too many arguments given\n");
        return -1;
    }

    return 0;
}
/******************************************************************************/
static int
xread(int p, u_int32_t* buf, int len)
{
    int restlen, da;

    restlen=len*4;
    while (restlen) {
        da=read(p, buf, restlen);
        if (da>0) {
            buf+=da;
            restlen-=da;
        } else if (errno==EINTR) {
            continue;
        } else {
            printf("xread: %s\n", strerror(errno));
            return -1;
        }
    }
    return 0;
}
/*****************************************************************************/
int xwrite(int p, u_int32_t* buf, int len)
{
    int da, restlen;
    char *bufptr;

    restlen=len*sizeof(int);
    bufptr=(char*)buf;
    while (restlen) {
        da=write(p, bufptr, restlen);
        if (da>0) {
            bufptr+=da;
            restlen-=da;
        } else {
            if (errno!=EINTR) {
            printf("xwrite: %s\n", strerror(errno));
                return -1;
            }
        }
    }
    return 0;
}
/*****************************************************************************/
static void
free_event(struct event *event)
{
    free(event->subevents);
    free(event->data);
    free(event);
}
/*****************************************************************************/
static struct event*
read_event(int p)
{
    u_int32_t head[4], *ptr;
    struct event *event;
    int size, swap, i;

    if (xread(p, head, 2))
        return 0;
    if (head[4]&0xffff0000) { /* we never have more than 2**16 subevents */
        swap=1;
    } else if (head[4]==0) { /* empty event */
        swap=head[0]&0xffff0000;
    }
    if (swap) {
        head[0]=SWAP_32(head[0]);
        head[1]=SWAP_32(head[1]);
        head[2]=SWAP_32(head[2]);
        head[3]=SWAP_32(head[3]);
    }
    size=head[0]+1;
    if (!(event=malloc(sizeof(struct event)))) {
        perror("malloc");
        return 0;
    }
    event->nr_subevents=head[3];
    event->eventnumber=head[1];
    if (!(event->data=malloc(size*4))) {
        perror("malloc");
        free(event);
        return 0;
    }
    if (!(event->subevents=malloc(event->nr_subevents
            *sizeof(struct subevent)))) {
        perror("malloc");
        free(event->data);
        free(event);
        return 0;
    }
    event->data[0]=head[0];
    event->data[1]=head[1];
    event->data[2]=head[2];
    event->data[3]=head[3];
    if (xread(p, event->data+4, size-4)) {
        free(event->subevents);
        free(event->data);
        free(event);
        return 0;
    }
    for (i=4; i<size; i++)
        event->data[i]=SWAP_32(event->data[i]);

    if (!event->nr_subevents)
        return event;

    ptr=event->data+4;
    for (i=0; i<event->nr_subevents; i++) {
        event->subevents[i].ID=*ptr++;
        event->subevents[i].length=*ptr++;
        event->subevents[i].data=ptr;
        ptr+=event->subevents[i].length;
    }
    
    return event;
}
/*****************************************************************************/
static int
write_event(int p, struct event *event)
{
    u_int32_t size=0, nr_subevents=0;
    int i;

    /* count nonempty subevents and total words */
    for (i=0; i<event->nr_subevents; i++) {
        if (event->subevents[i].length) {
            nr_subevents++;
            size+=event->subevents[i].length;
        }
    }

    /* write event header */
    if (xwrite(p, &size, 1))
        return -1;
    if (xwrite(p, &event->data[1], 1))
        return -1;
    if (xwrite(p, &event->data[2], 1))
        return -1;
    if (xwrite(p, &nr_subevents, 1))
        return -1;

    /* write subevents */
    for (i=0; i<event->nr_subevents; i++) {
        struct subevent *subevent=event->subevents+i;
        if (subevent->length) {
            if (xwrite(p, &subevent->ID, 1))
                return -1;
            if (xwrite(p, &subevent->length, 1))
                return -1;
            if (xwrite(p, subevent->data, subevent->length))
                return -1;
        }
    }
    return 0;    
}
/*****************************************************************************/
static int
event_empty(struct event *event)
{
    int i;
    for (i=0; i<event->nr_subevents; i++) {
        if (event->subevents[i].length)
            return 0;
    }
    return 1;
}
/*****************************************************************************/

int
main(int argc, char *argv[])
{
    int inpath, outpath, res=0;
    struct event *event;

    if (read_args(argc, argv)<0)
        return 1;

    if (infile) {
        inpath=open(infile, O_RDONLY, 0);
        if (inpath<0) {
            fprintf(stderr, "open \"%s\": %s\n", infile, strerror(errno));
            return 2;
        }
    } else {
        inpath=0;
    }
    if (outfile) {
        outpath=open(outfile, O_WRONLY|O_EXCL|O_CREAT|LINUX_LARGEFILE, 0644);
/* or: O_WRONLY|O_CREAT|O_TRUNC|LINUX_LARGEFILE */
        if (outpath<0) {
            fprintf(stderr, "create \"%s\": %s\n", outfile, strerror(errno));
            return 2;
        }
    } else {
        outpath=1;
    }

    while ((event=read_event(inpath))) {

        res=manipulate_event(event);
        if (res) {
            free_event(event);
            goto error;
        }

        if (!event_empty(event)) {
            if (write_event(outpath, event))
                goto error;
        }
        free_event(event);
    }

error:
    close(inpath);
    close(outpath);

    return 0;
}

/*****************************************************************************/
/*****************************************************************************/

