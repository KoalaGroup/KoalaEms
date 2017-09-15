#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <clusterformat.h>

#define SWAP_INT(x) (((x>>24)&0x000000ff)|((x >>8)&0x0000ff00)|\
                     ((x<< 8)&0x00ff0000)|((x<<24)&0xff000000))

char* inname;
char sss[1024];

typedef struct
  {
  size_t event_nr;
  size_t event_idx;
  size_t sev_idx;
  size_t sev_id;
  int VED_ID;
  } cl_ev_info;

typedef struct
  {
  int nix;
  } cl_ved_info;

typedef struct
  {
  int nix;
  } cl_text_info;

typedef struct
  {
  int nix;
  } cl_no_more_info;

typedef union
  {
  cl_ev_info      ev_info;
  cl_ved_info     ved_info;
  cl_text_info    text_info ;
  cl_no_more_info no_more_info;
  } all_info;

typedef struct
  {
  unsigned int* buf;
  unsigned int* xbuf;
  size_t xsize;
  size_t file_offs;
  size_t rec_size;
  size_t rec_num;
  size_t ev_num;
  int incluster;
  clustertypes typ;
  all_info info;
  int num_vedinfo;
  int num_headers;
  int num_eventclusters;
  int num_trailers;
  int no_more_data;
  } fileinfo;

/* XDR Strings */

#define xdrstrlen(p) ((*(p) +3)/4+1)
#define xdrstrclen(p) (*(p))

#ifdef WORDS_BIGENDIAN
# define swap_falls_noetig(x,y)
#else
# define swap_falls_noetig(x,y) swap(x,y)
#endif /* WORDS_BIGENDIAN */

/*****************************************************************************/
/*
swap konvertiert ein Integerarray der Laenge len mit Adresse adr von Network
in Host-Byteorder (oder andersrum)
*/
void swap(int* adr, int len)
{
while (len--)
  {
  *adr=ntohl(*adr);
  adr++;
  }
}
/*****************************************************************************/
/*
extractstring konvertiert einen String vom XDR-Format in c-Format.
p ist Pointer auf den Quellstring in der XDR-Datenstruktur, s ist die
Zieladresse , der Funktionswert zeigt auf die naechste Adresse nach dem
String in der XDR-Datenstruktur
*/
int* extractstring(char* s, const int* p)
{
int size, len;

size= *p++; /* Stringlaenge im Normalformat = xdrstrclen */
len=(size+3)>>2; /* Anzahl der Worte = xdrstrlen-1 */
swap_falls_noetig((int*)p, len); /* Host-byte-ordering herstellen*/
memcpy(s, p, size);
s[size]=0;                 /* String abschliessen */
swap_falls_noetig((int*)p, len); /* zurueckdrehen, Original ist wiederhergestellt */
return((int*)p+len);             /* Pointer auf naechstes XDR-Element */
}
/*****************************************************************************/
int* xdrstrcdup(char** s, const int* ptr)
{
int *help;
*s=(char*)malloc(xdrstrclen(ptr)+1);
if (*s) help=extractstring(*s, ptr);
return help;
}
/*****************************************************************************/
void print_buf(fileinfo* inf, int num)
{
int i;
if (num>inf->xsize) num=inf->xsize;
fprintf(stderr, "buffer at 0x%08x:\n", (char*)(inf->xbuf)-(char*)(inf->buf));
for (i=0; i<num; i++) fprintf(stderr, "%08x ", inf->xbuf[i]);
fprintf(stderr, "\n\n");
}
/*****************************************************************************/
void dump(unsigned int* buf, size_t num)
{
    const int w=8;
    int i, j, n;
    int lines;

    lines=num/w;

    for (i=0, n=0; i<lines; i++, n+=w) {
        printf("%5d ", n);
        for (j=0; j<w; j++)  printf(" %08x", buf[n+j]);
        printf("\n");
        printf("      ");
        for (j=0; j<w; j++) {
            if (buf[n+j]<1000)
                printf(" %8d", buf[n+j]);
            else
                printf(" ........");
        }
        printf("\n");
    }
    if (n<num) {
        printf("%5d ", n);
        for (j=n; j<num; j++)  printf(" %08x", buf[j]);
        printf("\n");
        printf("      ");
        for (j=n; j<num; j++) {
            if (buf[j]<1000)
                printf(" %8d", buf[j]);
            else
                printf(" ........");
        }
        printf("\n");
    }
}
/*****************************************************************************/
int fehler(fileinfo* inf, char* message)
{
fprintf(stderr, "ERROR:\n", message);
fprintf(stderr, "  %s\n", message);

fprintf(stderr, "    in cluster %ld at byte %ld in file (word %ld inside cluster)\n",
    inf->rec_num,
    inf->file_offs,
    inf->xbuf-inf->buf);
if (inf->incluster)
  {
  switch (inf->typ)
    {
    case clusterty_events:
      fprintf(stderr, "    in event %ld (%ld in this cluster); sev %ld\n",
          inf->info.ev_info.event_nr,
          inf->info.ev_info.event_idx,
          inf->info.ev_info.sev_idx);
      break;
    case clusterty_ved_info:
      break;
    case clusterty_text:
      break;
    case clusterty_no_more_data:
      break;
    default:
      fprintf(stderr, "    unknown clustertype %d\n",inf->typ );
      break;
    }
  }
fprintf(stderr, "\n");
return -1;
}
/*****************************************************************************/
void print_statist(fileinfo* inf, int rec, int events)
{
if (((inf->rec_num%rec)==0) || ((inf->ev_num%events)==0))
  {
  fprintf(stderr, "%ld clusters; %ld events\n", inf->rec_num, inf->ev_num);
  }
}
/*****************************************************************************/
int check_options(fileinfo* inf)
{
    unsigned int optsize, used_optsize;
    int flags;
    unsigned int* saved_xbuf;

    if (inf->xsize<1) {
        sprintf(sss, "Cluster contains no options");
        return fehler(inf, sss);
    }
    optsize=inf->xbuf[0];
    inf->xbuf++; inf->xsize--;
    saved_xbuf=inf->xbuf;

    if (!optsize) return 0;

    if (inf->xsize<1) {
        sprintf(sss, "Cluster contains no option-flags");
        return fehler(inf, sss);
    }

    flags=inf->xbuf[0];
    inf->xbuf++; inf->xsize--; used_optsize=1;

    if (flags&1) { /* timestamp */
        if (inf->xsize<2) {
            sprintf(sss, "Cluster too short for timestamp (%d)", inf->xsize);
            return fehler(inf, sss);
        }
#if 0
        {
        struct timeval tv;
        char s[1024];
        struct tm* time;

        tv.tv_sec=(inf->xbuf)[0];
        tv.tv_usec=(inf->xbuf)[1];
        time=localtime(&tv.tv_sec);
        strftime(s, 1024, "%c", time);
        fprintf(stderr, "  Timestamp: %d:%06d (%s)\n", tv.tv_sec, tv.tv_usec, s);
        }
#endif
        inf->xbuf+=2; inf->xsize-=2; used_optsize+=2;
    }
    if (flags&2) { /* checksum */
        if (inf->xsize<1) {
            sprintf(sss, "Cluster too short for checksum (%d)", inf->xsize);
            return fehler(inf, sss);
        }
        inf->xbuf+=1; inf->xsize-=1; used_optsize+=1;
    }
    if (optsize!=used_optsize) {
        sprintf(sss, "optsize=%d; used optsize=%d\n", optsize, used_optsize);
        fehler(inf,  sss);
        if (optsize<used_optsize) return -1;
    }
    inf->xbuf=saved_xbuf+optsize;
    return 0;
}
/*****************************************************************************/
int check_cluster_no_more_data(fileinfo* inf)
{
    fprintf(stderr, "NO_MORE_DATA at 0x%08x size=%d\n", inf->file_offs,
            inf->xsize+2);
    if (inf->no_more_data) {
        sprintf(sss, "there are %d no_more_data clusters before",
                inf->no_more_data);
        fehler(inf,  sss);
    }
    inf->no_more_data++;
    if (check_options(inf)) return -1;
    if (inf->xsize) {
        sprintf(sss, "no_more_data cluster is not empty: diff=%d", inf->xsize);
        fehler(inf,  sss);
    }
    return 0;
}
/*****************************************************************************/
int check_line(fileinfo* inf)
{
    char* line;
    unsigned int* help;

    if (inf->xsize<xdrstrlen(inf->xbuf)) {
        sprintf(sss, "Cluster too short for line: size=%d, need %d",
                inf->xsize, xdrstrlen(inf->xbuf));
        return fehler(inf,  sss);
    }
    help=(unsigned int*)xdrstrcdup(&line, (const int*)inf->xbuf);
    if (line) {
        fprintf(stderr, "--> %s\n", line);
        free(line);
    } else {
        sprintf(sss, "malloc(%d) for line: %s", xdrstrclen(inf->xbuf)+1,
            strerror(errno));
        return fehler(inf,  sss);
    }
    inf->xsize-=help-inf->xbuf; inf->xbuf=help;
    return 0;
}
/*****************************************************************************/
int check_cluster_file(fileinfo* inf)
{
    return 0;
}
/*****************************************************************************/
int check_cluster_text(fileinfo* inf)
{
    int nlines, line;

    fprintf(stderr, "TEXT at 0x%08x size=%d\n", inf->file_offs, inf->xsize+2);
    if (inf->no_more_data) {
        fehler(inf, "text after no_more_data");
    }
    if (inf->num_eventclusters||inf->no_more_data)
        inf->num_trailers++;
    else
        inf->num_headers++;

    if (check_options(inf)) return -1;
    if (inf->xsize<3) {
        sprintf(sss, "Cluster text too short after options (%d)", inf->xsize);
        return fehler(inf, sss);
    }
    nlines=inf->xbuf[2];
    fprintf(stderr, "  %d lines:\n", nlines);
    inf->xbuf+=3; inf->xsize-=3;
    for (line=0; line<nlines; line++) {
        if (check_line(inf)) return -1;
    }
    if (inf->xsize!=0) {
        sprintf(sss, "wrong size of ved_text: diff is %d", inf->xsize);
        fehler(inf, sss);
        if (inf->xsize<0) return -1;
    }
    return 0;
}
/*****************************************************************************/
int check_ved(fileinfo* inf)
{
    unsigned int size;
    int i;

    size_t file_offs;
    file_offs=(char*)(inf->xbuf)-(char*)(inf->buf);
/*fprintf(stderr, "check_ved at 0x%08x xsize=%d\n", file_offs, inf->xsize);*/

    if (inf->xsize<2) {
        sprintf(sss, "ved_info too short (%d)", inf->xsize);
        return fehler(inf, sss);
    }

    fprintf(stderr, "    VED %d (0x%x)\n", inf->xbuf[0], inf->xbuf[0]);
    size=inf->xbuf[1];
    inf->xbuf+=2; inf->xsize-=2;

    if (inf->xsize<size) {
        sprintf(sss, "in ved_info %d ISs but size=%d", size, inf->xsize);
        return fehler(inf, sss);
    }

    fprintf(stderr, "    %d IS: ", size);
    for (i=0; i<size; i++) fprintf(stderr, "%d ", inf->xbuf[i]);
    fprintf(stderr, "\n");

    inf->xbuf+=size; inf->xsize-=size;
    return 0;
}
/*****************************************************************************/
int check_cluster_ved_info(fileinfo* inf)
{
    int nveds, ved;

    fprintf(stderr, "VED-INFO at 0x%08x size=%d\n", inf->file_offs,
            inf->xsize+2);

    if (inf->no_more_data||inf->num_eventclusters||inf->num_trailers) {
        return fehler(inf, "ved_info is not the first cluster");
    }
    inf->num_vedinfo++;

    if (check_options(inf)) return -1;
    if (inf->xsize<2) {
        sprintf(sss, "Cluster ved_info too short after options (%d)",
                inf->xsize);
        return fehler(inf, sss);
    }
    fprintf(stderr, "  version: 0x%x\n", inf->xbuf[0]);
    nveds=inf->xbuf[1];
    fprintf(stderr, "  %d VEDs\n", nveds);
    inf->xbuf+=2; inf->xsize-=2;
    for (ved=0; ved<nveds; ved++) {
        if (check_ved(inf)) return -1;
    }
    if (inf->xsize!=0) {
        sprintf(sss, "wrong size of ved_info: diff is %d", inf->xsize);
        fehler(inf, sss);
        if (inf->xsize<0) return -1;
    }
    return 0;
}
/*****************************************************************************/
/*
PCITrigData:

 0    mbx;
 1    state;
 2    evc;
 3    tmc;
 4    tdt;
 5    ldt;
 6    reje;
 7    tgap[0];
 8    tgap[1];
 9    tgap[2];
#ifdef PROFILE
10    flagnum==4;
11    flags[0];               intr=0
12    flags[1];               syncpoll=0/1/2
13    flags[2];               syncread=1
14    flags[flagnum-1];       syncread=0; ++ in syncpoll
15    timenum==7;
16    times[0];               start intr
17    times[1];               intr vor selwakeup
18    times[2];               end   intr
19    times[3];               start syncpoll
20    times[4];               end   syncpoll
21    times[5];               start syncread
22    times[timenum-1];       end   syncread
23    u_flagnum==0;
__    u_flags[u_flagnum];
24    u_timenum==6;
25    u_times[0];             start get_trig
26    u_times[1];             nach read pci_trigdata
27    u_times[2];             end   get_trig
28    u_times[3];             start reset_trig
29    u_times[4];             end   reset_trig
30    u_times[u_timenum-1];   ???
#endif
31    3;
32    0; // wrote_cluster
#ifdef SCHED_TRIGGER
33    suspensions;
34    after_suspension;
#endif

*/

struct hist {
    unsigned long num;
    unsigned long ovl;
    unsigned long sum;
    unsigned long qsum;
    unsigned int h[2000];
};

struct h {
    struct hist ia;
    struct hist ie;
    struct hist ua;
    struct hist rad;
    struct hist red;
    struct hist re;
    struct hist ue;
} H;
/*****************************************************************************/
static void
init_hist()
{
    bzero(&H, sizeof(H));
}
/*****************************************************************************/
static void
dump_hist()
{
    int i;

    printf("# ia: %d %d %d\n", H.ia.num, H.ia.sum/H.ia.num, H.ia.ovl);
    printf("# ie: %d %d %d\n", H.ie.num, H.ie.sum/H.ie.num, H.ie.ovl);
    printf("# ua: %d %d %d\n", H.ua.num, H.ua.sum/H.ua.num, H.ua.ovl);
    printf("# ue: %d %d %d\n", H.ue.num, H.ue.sum/H.ue.num, H.ue.ovl);
    for (i=0; i<2000; i++) {
        printf("%5.1f %6d %6d %6d %6d %6d %6d %6d\n", i/10.,
                H.ia.h[i],
                H.ie.h[i],
                H.ua.h[i],
                H.rad.h[i],
                H.red.h[i],
                H.re.h[i],
                H.ue.h[i]
                );
    }
}
/*****************************************************************************/
static void
incr(struct hist* h, unsigned int d)
{
    h->num++;
    h->sum+=d;
    if (d<2000)
        h->h[d]++;
    else
        h->ovl++;
}
/*****************************************************************************/

int check_sev(fileinfo* inf)
{
    unsigned int size, i;
    unsigned int* data;
    static int count=10;

    if (inf->xsize<2) {
        sprintf(sss, "Subevent too short (%d)", inf->xsize);
        return fehler(inf, sss);
    }
    if (inf->xsize>65536) {
        printf("FEHLER in check_sev\n");
        printf("xsize=%lu\n", inf->xsize);
        printf("file_offs=%lu\n", inf->file_offs);
        printf("rec_size=%lu\n", inf->rec_size);
        printf("rec_num=%lu\n", inf->rec_num);
        printf("ev_num=%lu\n", inf->ev_num);
        return -1;
    }
    inf->info.ev_info.sev_id=inf->xbuf[0];
#if 0
    if (count) {
        fprintf(stderr, "      subevent id %d\n", inf->xbuf[0]);
        count--;
    }
#endif
    size=inf->xbuf[1];
#if 0
    if (count) {
        fprintf(stderr, "      subevent size %d\n", size);
        count--;
    }
#endif

    if (size>16384) {
        printf("check_sev: size=%u\n", size);
        return -1;
    }

    data=inf->xbuf+2;
#if 0
    if (count) {
        for (i=0; i<size; i++)
            printf("%d ", data[i]);
        printf("\n");
    }
#endif
    if (data[2]>2) {
        /*printf("num=%d event=%d\n", H.ia.num, inf->info.ev_info.event_nr);*/
        incr(&H.ia, data[16]);
        incr(&H.ie, data[18]);
        incr(&H.ua, data[25]);
        incr(&H.rad, data[21]);
        incr(&H.red, data[22]);
        incr(&H.re, data[26]);
        incr(&H.ue, data[29]);
    }
    inf->xbuf+=size+2; inf->xsize-=size+2;
    return 0;
}
/*****************************************************************************/
int check_event(fileinfo* inf)
{
    unsigned int size;
    unsigned int nsev, sev;
    unsigned int* saved_xbuf;

    if (inf->xsize<4) {
        sprintf(sss, "Event too short (%lu)", inf->xsize);
        return fehler(inf,  sss);
    }
    if (inf->xsize>65536) {
        sprintf(sss, "Event too large (%lu)", inf->xsize);
        return fehler(inf,  sss);
    }
    size=(inf->xbuf)[0];
    saved_xbuf=inf->xbuf;

    inf->info.ev_info.event_nr=(inf->xbuf)[1];
#if 0
    fprintf(stderr, "    event nr. %d\n", (inf->xbuf)[1]);
    fprintf(stderr, "    trigg nr. %d\n", (inf->xbuf)[2]);
#endif

    nsev=(inf->xbuf)[3];
#if 0
    fprintf(stderr, "    %d subevents\n", nsev);
#endif
    inf->xbuf+=4; inf->xsize-=4;

    for (sev=0; sev<nsev; sev++) {
        size_t saved_sev_xsize;

        saved_sev_xsize=inf->xsize;
        if (check_sev(inf)) {
            printf("subevent %lu\n", inf->info.ev_info.sev_idx);
            dump(saved_xbuf, size+1);
            break;
        }
        if (inf->xsize>65536) {
            printf("check_event: sev=%d xsize=%lu saved=%lu\n",
                    sev, inf->xsize, saved_sev_xsize);
            printf("subevent %lu\n", inf->info.ev_info.sev_idx);
            dump(saved_xbuf, size+1);
            return -1;
            inf->info.ev_info.sev_idx++;
        }
    }
    saved_xbuf++;
    if (saved_xbuf+size!=inf->xbuf) {
        sprintf(sss, "eventsize=%d but used size=%d", size,
                inf->xbuf-saved_xbuf);
        fehler(inf, sss);
        dump(saved_xbuf-1, size+1);
        /*if (saved_xbuf+size<inf->xbuf) return -1;*/
    }
    inf->xbuf=saved_xbuf+size;
    return 0;
}
/*****************************************************************************/
int check_cluster_events(fileinfo* inf)
{
    unsigned int nev, ev;

    inf->info.ev_info.event_nr=-1;
    inf->info.ev_info.event_idx=0;
    inf->info.ev_info.sev_idx=0;
    inf->info.ev_info.sev_id=-1;

#if 0
  fprintf(stderr, "Events at 0x%08x, size=%d\n", inf->file_offs, inf->xsize+2);
#endif
    if (!inf->num_vedinfo) {
        return fehler(inf, "no vedinfo before events");
    }
    if (inf->no_more_data) {
        return fehler(inf, "events after no_more_data");
    }
    inf->num_eventclusters++;

    if (inf->xsize>65536) {
        sprintf(sss, "Event cluster too large (%lu)", inf->xsize);
        return fehler(inf, sss);
    }
    if (check_options(inf)) return -1;
    if (inf->xsize<4) {
        sprintf(sss, "Event cluster too short after options (%lu)", inf->xsize);
        return fehler(inf, sss);
    }
    if (inf->xbuf[0]) {
        fprintf(stderr, "  flags: 0x%x\n", inf->xbuf[0]);
    }
#if 0
    fprintf(stderr, "  VED_ID: %d\n", inf->xbuf[1]);
    fprintf(stderr, "  fragment_id: %d\n", inf->xbuf[2]);
#endif

    nev=inf->xbuf[3];
#if 0
    fprintf(stderr, "  Number of Events: %u\n", nev);
#endif
    inf->xbuf+=4; inf->xsize-=4;

    for (ev=0; ev<nev; ev++) {
        if (check_event(inf)) {
            printf("event %lu\n", inf->info.ev_info.event_idx);
            return -1;
        }
        inf->info.ev_info.event_idx++;
        inf->ev_num++;
    }
    return 0;
}
/*****************************************************************************/
int check_cluster(fileinfo* inf)
{
int res;

size_t file_offs;
file_offs=(char*)(inf->xbuf)-(char*)(inf->buf);
/*fprintf(stderr, "check_cluster at 0x%08x xsize=%d\n", file_offs, inf->xsize);*/

if (inf->xsize>65536)
  {
  sprintf(sss, "Cluster too large");
  return fehler(inf, sss);
  }
if (inf->xsize<1)
  {
  sprintf(sss, "Cluster contains no typefield");
  return fehler(inf, sss);
  }
inf->typ=inf->xbuf[0];
inf->incluster=1;
inf->xbuf++; inf->xsize--;

switch (inf->typ)
  {
  case clusterty_events:
    res=check_cluster_events(inf);
    break;
  case clusterty_ved_info:
    res=check_cluster_ved_info(inf);
    break;
  case clusterty_text:
    res=check_cluster_text(inf);
    break;
  case clusterty_file:
    res=check_cluster_file(inf);
    break;
  case clusterty_no_more_data:
    res=check_cluster_no_more_data(inf);
    break;
  default:
    fprintf(stderr, "Unknown clustertype %d\n", inf->typ);
    res=-1;
  }
return res;
}
/*****************************************************************************/
int read_cluster(int p, fileinfo* inf)
{
    unsigned int size;
    unsigned int* buf;
    unsigned int sbuf[2];
    int res;

    inf->rec_num++;

    inf->buf=0;

    res=read(p, (char*)sbuf, 2*sizeof(int));
    if (res!=2*sizeof(int)) {
        sprintf(sss, "read header: %s", strerror(errno));
        return fehler(inf, sss);
    }
    switch (sbuf[1]) {
    case 0x12345678:
        size=sbuf[0];
        break;
    case 0x78563412:
        size=SWAP_INT(sbuf[0]);
        break;
    default:
        sprintf(sss, "unknown endien :0x%08x", sbuf[1]);
        return fehler(inf, sss);
    }

    buf=(unsigned int*)malloc((size+1)*sizeof(int));
    if (!buf) {
        sprintf(sss, "malloc %d words: %s", size, strerror(errno));
        return fehler(inf, sss);
    }

    buf[0]=sbuf[0];
    buf[1]=sbuf[1];

    res=read(p, (char*)(buf+2), (size-1)*sizeof(int));
    if (res!=(size-1)*sizeof(int)) {
        sprintf(sss, "read data: %s", strerror(errno));
        free(buf);
        return fehler(inf, sss);
    }
    inf->rec_size=(size+1)*sizeof(int);

    switch (buf[1]) {
    case 0x12345678:
        break;
    case 0x78563412: {
        int i;
        for (i=0; i<size+1; i++) buf[i]=SWAP_INT(buf[i]);
        } break;
    default:
        sprintf(sss, "unknown endien :0x%08x", buf[1]);
        free(buf);
        return fehler(inf, sss);
    }
    if (size!=buf[0]) {
        sprintf(sss, "size=%d; but buf[0]=%d", size, buf[0]);
        free(buf);
        return fehler(inf, sss);
    }
    inf->buf=buf;
    return 0;
}
/*****************************************************************************/
void check_clusters(int p)
{
int weiter;
fileinfo inf;

inf.file_offs=0;
inf.rec_num=0;
inf.ev_num=0;
inf.num_vedinfo=0;
inf.num_headers=0;
inf.num_eventclusters=0;
inf.num_trailers=0;
inf.no_more_data=0;

do
  {
  inf.incluster=0;
  inf.rec_size=0;
  if (read_cluster(p, &inf))
    {
    if (inf.buf) free(inf.buf);
    printf("cluster=%lu\n", inf.rec_num);
    return;
    }
  if (inf.buf)
    {
    inf.xbuf=inf.buf+2;
    if (inf.buf[0]>16384) {
        printf("buf[0]=%d\n", inf.buf[0]);
        printf("cluster=%lu\n", inf.rec_num);
        return;
    }
    inf.xsize=inf.buf[0]-1;
    weiter=check_cluster(&inf)==0;
    free(inf.buf); 
    }
  inf.file_offs+=inf.rec_size;
  print_statist(&inf, 100, 1000);
  }
while (weiter);
fprintf(stderr, "last cluster=%lu\n", inf.rec_num);
}
/*****************************************************************************/
void printusage(char* argv0)
{
fprintf(stderr, "usage: %s [-h] [-t] [-v level] [infile]\n", basename(argv0));
fprintf(stderr, "  -h: this help\n");
/*fprintf(stderr, "  -q: quiet (no informational output)\n");*/
fprintf(stderr, "  -t: input is tape\n");
fprintf(stderr, "  -v: level of verboseness:\n");
fprintf(stderr, "      0: quiet\n");
fprintf(stderr, "      1: only no_more_data clusters\n");
fprintf(stderr, "      2: + VED info\n");
fprintf(stderr, "      3: + Text info\n");
fprintf(stderr, "      4: + Event info\n");
fprintf(stderr, "      5: + Subevent info\n");
}
/*****************************************************************************/
main(int argc, char* argv[])
{
    int c, err;
    int p;

    err=0;
    while (!err && ((c=getopt(argc, argv, "h")) != -1)) {
        switch (c) {
        case 'h': printusage(argv[0]); return 1;
        default: err=1;
        }
    }
    if (err) {printusage(argv[0]); return 1;}

    if (optind<argc)
        inname=argv[optind];
    else
        inname="-";

    if (strcmp(inname, "-"))
        p=open(inname, O_RDONLY, 0);
    else
        p=0;

    if (p==-1) {
        fprintf(stderr, "open(%s: %s)\n", inname, strerror(errno));
        return 1;
    }

    init_hist();

    check_clusters(p);

    if (p>0) close(p);

    dump_hist();

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
