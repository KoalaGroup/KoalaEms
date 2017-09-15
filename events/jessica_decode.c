/*
 * jessica_decode.c
 * 
 * created 26.Oct.2003 
 * $ZEL: jessica_decode.c,v 1.1 2003/10/27 11:47:41 wuestner Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

typedef unsigned int ems_u32;

#define SWAP_32(n) (((n & 0xff000000) >> 24) | \
    ((n & 0x00ff0000) >>  8) | \
    ((n & 0x0000ff00) <<  8) | \
    ((n & 0x000000ff) << 24))

struct event_buffer {
    ems_u32 size;
    ems_u32 evno;
    ems_u32 trigno;
    ems_u32 subevents;
    ems_u32* data;
    size_t max_subevents;
    ems_u32** subeventptr;
};

struct event_buffer eb;
ems_u32* cldata;
int cldatasize;

/******************************************************************************/
static void
printusage(char* argv0)
{
    printf("usage: %s [-h]\n",
        basename(argv0));
    printf("  -h: this help\n");
}
/******************************************************************************/
static int
readargs(int argc, char* argv[])
{
    int c, err;

    err=0;
    while (!err && ((c=getopt(argc, argv, "h")) != -1)) {
        switch (c) {
        case 'h': printusage(argv[0]); return 1;
        default: err=1;
        }
    }
    if (err) {
        printusage(argv[0]);
        return -1;
    }
    return 0;
}
/******************************************************************************/
static int
decode_run_nr(ems_u32* ptr, int size)
{
    if (size!=1) {
        printf("\nrun nr: size=%d\n", size);
        return -1;
    }
    printf("\nrun nr=%d\n", ptr[0]);
    return 0;
}
/******************************************************************************/
static int
decode_timestamp(ems_u32* ptr, int size)
{
    struct tm* tm;
    time_t t;
    char s[1024];

    if (size!=2) {
        printf("\ntimestamp: size=%d\n", size);
        return -1;
    }
    /* printf("\ntimestamp: %08x %08x\n", ptr[0], ptr[1]); */

    t=ptr[0];
    tm=localtime(&t);
    strftime(s, 1024, "%Y-%m-%d %H:%M:%S", tm);
    printf("\ntimestamp: %s.%06d\n", s, ptr[1]);
    return 0;
}
/******************************************************************************/
static int
decode_caen_v265(ems_u32* ptr, int size)
{
    int i, j;

    if (size!=16) {
        printf("\ncaen v265: size=%d\n", size);
        return -1;
    }

    printf("\nV265 Charge Integrating ADC:\n");
    for (j=0; j<2; j++) {
        printf("%d: ", j?15:12);
        for (i=0; i<16; i++) {
            ems_u32 v=ptr[i];
            int r15=!!(v&(1<<12));
            if (r15==j) {
                int chan=(v>>13)&7;
                int val=v&0xfff;
                if (!r15) val<<=3;
                printf("[%d %04x] ", chan, val);
            }
        }
        printf("\n");
    }
    return 0;
}
/******************************************************************************/
static int
decode_caen_v556(ems_u32* ptr, int size)
{
    int nr_mod, i;

    printf("\nV556 Peak Sensing ADC\n");

    nr_mod=*ptr++;
    for (i=0; i<nr_mod; i++) {
        ems_u32 head;
        int nr_data, j;

        head=*ptr++;
        printf("module %d, header 0x%04x\n", i, head);
        nr_data=(head>>12)&7;
        for (j=0; j<nr_data; j++) {
            ems_u32 v;
            v=*ptr++;
            printf("  %08x chan %d data %4d\n",
                v,
                (v>>12)&7,
                v&0xfff);
        }
    }

    return 0;
}
/******************************************************************************/
static int
decode_sis3400(ems_u32* ptr, int size)
{
    ems_u32 v, count;
    int i;

    /* printf("\nsis3400: size=%d\n", size); */
    v=*ptr++; size--;
    if (v!=1) {
        printf("\nsis3400: %d modules\n", v);
        return -1;
    }
    count=*ptr++; size--;
    if (count!=size) {
        printf("\nsis3400: size=%d count=%d\n", size, count);
        return -1;
    }
    printf("\nSIS3400 Time Stamper\n");
    for (i=0; i<count; i++) {
        v=*ptr++; size--;
        printf("  %08x id %d chan %2d stamp %7d %c\n",
            v,
            (v>>26)&0x1f,
            (v>>20)&0x3f,
            v&0xfffff,
            (v&0x80000000)?'t':' ');
    }    
    printf("\n");
    return 0;
}
/******************************************************************************/
static int
proceed_event(struct event_buffer* eb)
{
    ems_u32* sptr=eb->data;
    int i;

    printf("event %d trig %d sub %d size %d\n",
            eb->evno, eb->trigno, eb->subevents, eb->size);
/*
    for (i=0; i<eb->size; i++) printf("%3d 0x%08x\n", i, eb->data[i]);
*/
    for (i=0; i<eb->subevents; i++) {
        int size;
        eb->subeventptr[i]=sptr;
        size=sptr[1];
        sptr+=size+2;
    }

    for (i=0; i<eb->subevents; i++) {
        ems_u32* ptr=eb->subeventptr[i];
/*
        printf("subevent %d size=%d\n", ptr[0], ptr[1]);
*/
        switch (ptr[0]) {
        case 0: decode_run_nr(ptr+2, ptr[1]); break;
        case 1: decode_timestamp(ptr+2, ptr[1]); break;
        case 2: decode_caen_v265(ptr+2, ptr[1]); break;
        case 3: decode_caen_v556(ptr+2, ptr[1]); break;
        case 4: decode_sis3400(ptr+2, ptr[1]); break;
        default: printf("unknown subevent %d; size=%d\n", ptr[0], ptr[1]);
        }
    }
    return 0;
}
/******************************************************************************/
static int
adjust_event_buffer(struct event_buffer* eb)
{
    if (eb->max_subevents<eb->subevents) {
        if (eb->subeventptr) free(eb->subeventptr);
        eb->subeventptr=malloc(eb->subevents*sizeof(ems_u32*));
        if (!eb->subeventptr) {
            printf("malloc %d ptrs for subeventptr: %s\n", eb->subevents, strerror(errno));
            return -1;
        }
        eb->max_subevents=eb->subevents;
    }

    return 0;
}
/******************************************************************************/
static int
xread(int p, ems_u32* buf, int len)
{
    int restlen, da;

    restlen=len*sizeof(ems_u32);
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
/******************************************************************************/
static int
read_cluster(int p)
{
    ems_u32 head[2], v;
    ems_u32* data;
    int wenden, size, evnum, i;

    if (xread(p, head, 2)) return -1;
    switch (head[1]) {
    case 0x12345678: wenden=0; break;
    case 0x78563412: wenden=1; break;
    default:
        printf("unkown endien 0x%x\n", head[1]);
        return -1;
    }
    size=(wenden?SWAP_32(head[0]):head[0])-1;
    if (size>cldatasize) {
        if (cldata) free(cldata);
        cldata=malloc(size*sizeof(ems_u32));
        if (!cldata) {
            perror("malloc");
            return -1;
        }
    }
    if (xread(p, cldata, size)) return -1;
    if (wenden) {
        for (i=0; i<size; i++) cldata[i]=SWAP_32(cldata[i]);
    }
    printf("size=%d type=%d\n", size, cldata[0]);
    switch (cldata[0]) { /* clustertype */
    case 0: /* events */
        break;
    case 1: /* ved_info */
    case 2: /* text */
    case 3: /* wendy_setup */
    case 4: /* file */
        return 0;
    case 5: /* no_more_data */
        printf("no more data\n");
        return 1;
    }

    data=cldata+1;
    v=*data++;
    data+=v;   /* skip optional fields */
    data++;    /* skip flags */
    data++;    /* skip VED_ID */
    data++;    /* skip fragment_id */

    evnum=*data++;
    for (i=0; i<evnum; i++) {
        int evsize=*data++;
        eb.data=data;

        eb.size=evsize-3;
        eb.evno=*eb.data++;
        eb.trigno=*eb.data++;
        eb.subevents=*eb.data++;
        if (adjust_event_buffer(&eb)<0) return -1;

        if (proceed_event(&eb)<0) return -1;

        data+=evsize;
    }

    return 0;
}
/******************************************************************************/
main(int argc, char *argv[])
{
    char *p;

    if (readargs(argc, argv)) return 1;

    cldata=0;
    cldatasize=0;
    memset(&eb, 0, sizeof(struct event_buffer));

    while (!read_cluster(0)) {}

    return 0;
}
/******************************************************************************/
/******************************************************************************/
