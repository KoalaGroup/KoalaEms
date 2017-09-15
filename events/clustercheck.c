#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <clusterformat.h>

#define TSIZE 655360

#define SWAP_INT(x) (((x>>24)&0x000000ff)|((x >>8)&0x0000ff00)|\
                     ((x<< 8)&0x00ff0000)|((x<<24)&0xff000000))

int use_tape, quiet, vlevel;
char* inname;
char sss[1024];

typedef struct
  {
  int n; /*no_more*/   /*vlevel>1*/
  int v; /*vedinfo*/   /*vlevel>2*/
  int t; /*text*/      /*vlevel>3*/
  int f; /*file*/      /*vlevel>?*/
  int e; /*events*/    /*vlevel>4*/
  int s; /*subevents*/ /*vlevel>5*/
  } vcodes;
vcodes vc;

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
  enum clustertypes typ;
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

#define UL(x) ((unsigned long long int)(x))

/*****************************************************************************/
/*
swap konvertiert ein Integerarray der Laenge len mit Adresse adr von Network
in Host-Byteorder (oder andersrum)
*/
void swap(int* adr, int len)
{
    while (len--) {
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
    if (*s)
        help=extractstring(*s, ptr);
    return help;
}
/*****************************************************************************/
void print_buf(fileinfo* inf, int num)
{
    int i;
    if (num>inf->xsize)
        num=inf->xsize;
    fprintf(stderr, "buffer at 0x%08llx:\n",
            UL((char*)(inf->xbuf)-(char*)(inf->buf)));
    for (i=0; i<num; i++)
        fprintf(stderr, "%08x ", inf->xbuf[i]);
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
    fprintf(stderr, "ERROR:\n  %s\n", message);

    fprintf(stderr, "    in cluster %ld at byte %ld in file "
            "(word %ld inside cluster)\n",
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
int check_options(fileinfo* inf, int v)
{
unsigned int optsize, used_optsize;
int flags;
unsigned int* saved_xbuf;

if (inf->xsize<1)
  {
  sprintf(sss, "Cluster contains no options");
  return fehler(inf, sss);
  }
optsize=inf->xbuf[0];
inf->xbuf++; inf->xsize--;
saved_xbuf=inf->xbuf;

if (!optsize) return 0;

if (inf->xsize<1)
  {
  sprintf(sss, "Cluster contains no option-flags");
  return fehler(inf, sss);
  }

flags=inf->xbuf[0];
inf->xbuf++; inf->xsize--; used_optsize=1;

if (flags&1) /* timestamp */
  {
  struct timeval tv;
  char s[1024];
  struct tm* time;

        if (inf->xsize<2) {
            sprintf(sss, "Cluster too short for timestamp (%lld)",
                    UL(inf->xsize));
            return fehler(inf, sss);
        }
  tv.tv_sec=(inf->xbuf)[0];
  tv.tv_usec=(inf->xbuf)[1];
  time=localtime(&tv.tv_sec);
  strftime(s, 1024, "%c", time);
  if (v)
      fprintf(stderr, "  Timestamp: %lld:%06lld (%s)\n",
            UL(tv.tv_sec), UL(tv.tv_usec), s);
  inf->xbuf+=2; inf->xsize-=2; used_optsize+=2;
  }
if (flags&2) /* checksum */
  {
  if (inf->xsize<1)
    {
    sprintf(sss, "Cluster too short for checksum (%lld)", UL(inf->xsize));
    return fehler(inf, sss);
    }
  if (v) fprintf(stderr, "  Checksum: %08x\n", inf->xbuf[0]);
  inf->xbuf+=1; inf->xsize-=1; used_optsize+=1;
  }
if (optsize!=used_optsize)
  {
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
    if (vc.n)
        fprintf(stderr, "NO_MORE_DATA at 0x%08llx size=%lld\n",
                UL(inf->file_offs), UL(inf->xsize+2));
if (inf->no_more_data)
  {
  sprintf(sss, "there are %d no_more_data clusters before", inf->no_more_data);
  fehler(inf,  sss);
  }
inf->no_more_data++;
if (check_options(inf, vc.n)) return -1;
    if (inf->xsize) {
        sprintf(sss, "no_more_data cluster is not empty: diff=%lld",
                UL(inf->xsize));
        fehler(inf,  sss);
    }
return 0;
}
/*****************************************************************************/
int check_line(fileinfo* inf, int v)
{
    char* line;
    unsigned int* help;

    if (inf->xsize<xdrstrlen(inf->xbuf)) {
        sprintf(sss, "Cluster too short for line: size=%lld, need %d",
                UL(inf->xsize), xdrstrlen(inf->xbuf));
        return fehler(inf,  sss);
    }
help=(unsigned int*)xdrstrcdup(&line, (const int*)inf->xbuf);
if (line)
  {
  if (v) fprintf(stderr, "--> %s\n", line);
  free(line);
  }
else
  {
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

    if (vc.t)
        fprintf(stderr, "TEXT at 0x%08llx size=%lld\n",
                UL(inf->file_offs), UL(inf->xsize+2));
if (inf->no_more_data)
  {
  fehler(inf, "text after no_more_data");
  }
if (inf->num_eventclusters||inf->no_more_data)
  inf->num_trailers++;
else
  inf->num_headers++;

if (check_options(inf, vc.t)) return -1;
    if (inf->xsize<3) {
        sprintf(sss, "Cluster text too short after options (%lld)",
                UL(inf->xsize));
        return fehler(inf, sss);
    }
if (vc.t)
  {
  fprintf(stderr, "  flags: %d\n", inf->xbuf[0]);
  fprintf(stderr, "  fragment: %d\n", inf->xbuf[1]);
  }
nlines=inf->xbuf[2];
if (vc.t) fprintf(stderr, "  %d lines:\n", nlines);
inf->xbuf+=3; inf->xsize-=3;
for (line=0; line<nlines; line++)
  {
  if (check_line(inf, vc.t)) return -1;
  }
    if (inf->xsize!=0) {
        sprintf(sss, "wrong size of ved_text: diff is %lld", UL(inf->xsize));
        fehler(inf, sss);
        if (inf->xsize<0)
            return -1;
    }
return 0;
}
/*****************************************************************************/
int check_ved(fileinfo* inf, int v)
{
unsigned int size;
int i;

size_t file_offs;
file_offs=(char*)(inf->xbuf)-(char*)(inf->buf);
/*fprintf(stderr, "check_ved at 0x%08x xsize=%d\n", file_offs, inf->xsize);*/

    if (inf->xsize<2) {
        sprintf(sss, "ved_info too short (%lld)", UL(inf->xsize));
        return fehler(inf, sss);
    }

if (v) fprintf(stderr, "    VED %d (0x%x)\n", inf->xbuf[0], inf->xbuf[0]);
size=inf->xbuf[1];
inf->xbuf+=2; inf->xsize-=2;

    if (inf->xsize<size) {
        sprintf(sss, "in ved_info %d ISs but size=%lld", size, UL(inf->xsize));
        return fehler(inf, sss);
    }
if (v)
  {
  fprintf(stderr, "    %d IS: ", size);
  for (i=0; i<size; i++) fprintf(stderr, "%d ", inf->xbuf[i]);
  fprintf(stderr, "\n");
  }
inf->xbuf+=size; inf->xsize-=size;
return 0;
}
/*****************************************************************************/
int check_cluster_ved_info(fileinfo* inf)
{
int nveds, ved;

    if (vc.v)
        fprintf(stderr, "VED-INFO at 0x%08llx size=%lld\n",
                UL(inf->file_offs), UL(inf->xsize+2));

if (inf->no_more_data||inf->num_eventclusters||inf->num_trailers)
  {
  return fehler(inf, "ved_info is not the first cluster");
  }
inf->num_vedinfo++;

if (check_options(inf, vc.v)) return -1;
    if (inf->xsize<2) {
        sprintf(sss, "Cluster ved_info too short after options (%lld)",
            UL(inf->xsize));
        return fehler(inf, sss);
    }
if (vc.v) fprintf(stderr, "  version: 0x%x\n", inf->xbuf[0]);
nveds=inf->xbuf[1];
if (vc.v) fprintf(stderr, "  %d VEDs\n", nveds);
inf->xbuf+=2; inf->xsize-=2;
for (ved=0; ved<nveds; ved++)
  {
  if (check_ved(inf, vc.v)) return -1;
  }
    if (inf->xsize!=0) {
        sprintf(sss, "wrong size of ved_info: diff is %lld", UL(inf->xsize));
        fehler(inf, sss);
        if (inf->xsize<0)
            return -1;
    }
return 0;
}
/*****************************************************************************/
int check_sev(fileinfo* inf, int v)
{
unsigned int size;

    if (inf->xsize<2) {
        sprintf(sss, "Subevent too short (%lld)", UL(inf->xsize));
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
if (v) fprintf(stderr, "      subevent id %d\n", inf->xbuf[0]);
size=inf->xbuf[1];
if (v) fprintf(stderr, "      subevent size %d\n", size);

if (size>16384) {
    printf("check_sev: size=%u\n", size);
    return -1;
}

inf->xbuf+=size+2; inf->xsize-=size+2;
return 0;
}
/*****************************************************************************/
int check_event(fileinfo* inf, int v, int vs)
{
unsigned int size;
unsigned int nsev, sev;
unsigned int* saved_xbuf;

if (inf->xsize<4)
  {
  sprintf(sss, "Event too short (%lu)", inf->xsize);
  return fehler(inf,  sss);
  }
if (inf->xsize>65536)
  {
  sprintf(sss, "Event too large (%lu)", inf->xsize);
  return fehler(inf,  sss);
  }
size=(inf->xbuf)[0];
saved_xbuf=inf->xbuf;

inf->info.ev_info.event_nr=(inf->xbuf)[1];
if (v)
  {
  fprintf(stderr, "    event nr. %d\n", (inf->xbuf)[1]);
  fprintf(stderr, "    trigg nr. %d\n", (inf->xbuf)[2]);
  }
nsev=(inf->xbuf)[3];
if (v) fprintf(stderr, "    %d subevents\n", nsev);
inf->xbuf+=4; inf->xsize-=4;

for (sev=0; sev<nsev; sev++) {
    size_t saved_sev_xsize;

    saved_sev_xsize=inf->xsize;
    if (check_sev(inf, vs)) {
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
        sprintf(sss, "eventsize=%d but used size=%lld", size,
                UL(inf->xbuf-saved_xbuf));
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

if (vc.e)
  fprintf(stderr, "Events at 0x%08x, size=%d\n", inf->file_offs, inf->xsize+2);
if (!inf->num_vedinfo)
  {
  return fehler(inf, "no vedinfo before events");
  }
if (inf->no_more_data)
  {
  return fehler(inf, "events after no_more_data");
  }
inf->num_eventclusters++;

if (inf->xsize>65536)
  {
  sprintf(sss, "Event cluster too large (%lu)", inf->xsize);
  return fehler(inf, sss);
  }
if (check_options(inf, vc.e)) return -1;
if (inf->xsize<4)
  {
  sprintf(sss, "Event cluster too short after options (%lu)", inf->xsize);
  return fehler(inf, sss);
  }
if (inf->xbuf[0])
  {
  fprintf(stderr, "  flags: 0x%x\n", inf->xbuf[0]);
  }
if (vc.e)
  {
  fprintf(stderr, "  VED_ID: %d\n", inf->xbuf[1]);
  fprintf(stderr, "  fragment_id: %d\n", inf->xbuf[2]);
  }
nev=inf->xbuf[3];
if (vc.e) fprintf(stderr, "  Number of Events: %u\n", nev);
inf->xbuf+=4; inf->xsize-=4;

for (ev=0; ev<nev; ev++) {
    if (check_event(inf, vc.e, vc.e&&vc.s)) {
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
fprintf(stderr, "check_cluster at 0x%08x xsize=%d\n", file_offs, inf->xsize);

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

inf->rec_num++;

inf->buf=0;
buf=(unsigned int*)malloc(TSIZE);
if (!buf)
  {
  sprintf(sss, "malloc %d: %s", TSIZE, strerror(errno));
  return fehler(inf, sss);
  }

if (use_tape)
  {
  int res;
  res=read(p, (char*)buf, TSIZE);
  if (res<0)
    {
    sprintf(sss, "read: %s\n", strerror(errno));
    free(buf);
    return fehler(inf, sss);
    }
  else if (res==0)
    {
    fehler(inf, "read filemark");
    free(buf);
    return 0;
    }
  if (res%4)
    {
    sprintf(sss, "read %d bytes, not a multiple of 4", res);
    free(buf);
    return fehler(inf, sss);
    }
  size=res/4-1;
  inf->rec_size=res;
  }
else /* !use_tape */
  {
  int res;
  res=read(p, (char*)buf, 2*sizeof(int));
  if (res!=2*sizeof(int))
    {
    sprintf(sss, "read header: %s", strerror(errno));
    free(buf);
    return fehler(inf, sss);
    }
  switch (buf[1])
    {
    case 0x12345678:
      size=buf[0];
      break;
    case 0x78563412:
      size=SWAP_INT(buf[0]);
      break;
    default:
      sprintf(sss, "unknown endien :0x%08x", buf[2]);
      free(buf);
      return fehler(inf, sss);
    }
  if ((size+1)*sizeof(int)>TSIZE)
    {
    sprintf(sss, "cluster too big: %d words", size+1);
    free(buf);
    return fehler(inf, sss);
    }
  res=read(p, (char*)(buf+2), (size-1)*sizeof(int));
  if (res!=(size-1)*sizeof(int))
    {
    sprintf(sss, "read data: %s", strerror(errno));
    free(buf);
    return fehler(inf, sss);
    }
  inf->rec_size=(size+1)*sizeof(int);
  } /* !use_tape */

switch (buf[1])
  {
  case 0x12345678:
    break;
  case 0x78563412:
    {
    int i;
    for (i=0; i<size+1; i++) buf[i]=SWAP_INT(buf[i]);
    }
    break;
  default:
    sprintf(sss, "unknown endien :0x%08x", buf[1]);
    free(buf);
    return fehler(inf, sss);
  }
if (size!=buf[0])
  {
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
printf("last cluster=%lu\n", inf.rec_num);
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

use_tape=0; quiet=0, vlevel=0;
printusage(argv[0]);
err=0;
while (!err && ((c=getopt(argc, argv, "hqtv:")) != -1))
  {
  switch (c)
    {
    case 'h': printusage(argv[0]); return 1;
    case 'q': quiet=1; break;
    case 't': use_tape=1; break;
    case 'v':
      {
      char* end;
      vlevel=strtol(optarg, &end, 0);
      if (*end!=0)
        {
        fprintf(stderr, "cannot convert \"%s\" to integer.\n", optarg);
        printusage(argv[0]); return 1;
        }
      }
      break;
    default: err=1;
    }
  }
if (err) {printusage(argv[0]); return 1;}

printf("vlevel=%d\n", vlevel);
vc.n=vlevel>1;
vc.v=vlevel>2;
vc.t=vlevel>3;
vc.e=vlevel>4;
vc.s=vlevel>5;

if (optind<argc)
  inname=argv[optind];
else
  inname="-";

if (strcmp(inname, "-"))
  p=open(inname, O_RDONLY, 0);
else
  p=0;

if (p==-1)
  {
  fprintf(stderr, "open(%s: %s)\n", inname, strerror(errno));
  return 1;
  }

check_clusters(p);

if (p>0) close(p);
return 0;
}
/*****************************************************************************/
/*****************************************************************************/
