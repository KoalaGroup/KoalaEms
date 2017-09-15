/*
 * events++/tapedump.c
 * 
 * PW
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>

#define swap_int(a) ( ((a) << 24) | \
                      (((a) << 8) & 0x00ff0000) | \
                      (((a) >> 8) & 0x0000ff00) | \
        ((unsigned int)(a) >>24) )

#define xdrstrclen(p) (*(p))
#define swap_falls_noetig(x,y) swap(x,y)

int verbose=0;
int num_events=0;

typedef enum {clusterty_events=0, clusterty_ved_info, clusterty_text,
    clusterty_wendy_setup,
    clusterty_no_more_data=0x10000000} clustertypes;

void swap(int* adr, int len)
{
while (len--)
  {
  *adr=ntohl(*adr);
  adr++;
  }
}

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

int* xdrstrcdup(char** s,const int*  ptr)
{
int *help;
*s=(char*)malloc(xdrstrclen(ptr)+1);
if (*s) help=extractstring(*s, ptr);
return help;
}

void decode_options(int** buf, int* size)
{
int optsize;
int flags;
struct timeval tv;
int checksum;

optsize=*(*buf)++; (*size)--; 
if (!optsize) return;
flags=*(*buf)++; (*size)--; optsize--;
if (flags&1) /* timestamp */
  {
  struct tm* time;
  char s[256];
  tv.tv_sec=*(*buf)++; (*size)--; optsize--;
  tv.tv_usec=*(*buf)++; (*size)--; optsize--;
  time=localtime(&tv.tv_sec);
  strftime(s, 256, "%a %d. %b %Y %H:%M:%S", time);
  printf("  time: %s.%06d\n", s, tv.tv_usec);
  }
if (flags&2) /* checksum */
  {
  checksum=*(*buf)++; (*size)--; optsize--;
  printf("  checksum=0x%08x\n", checksum);
  }
if (flags&~3)
  {
  printf("  unknown optionflags: 0x%08x\n", flags);
  }
if (optsize!=0)
  {
  printf("  wrong optsize: %d\n", optsize);
  }
}

void decode_ved_info(int* buf, int size)
{
printf("--- ved_info\n");
decode_options(&buf, &size);
}

void decode_text(int* buf, int size)
{
int flags;
int fragment_id;
int num;

printf("--- text\n");
decode_options(&buf, &size);
flags=*buf++; size--;
fragment_id=*buf++; size--;
num=*buf++; size--;
printf("    flags=%d, frag=%d, num=%d\n", flags, fragment_id, num);
while (num--)
  {
  char* s;
  buf=xdrstrcdup(&s, buf);
  printf("  %s\n", s);
  free(s);
  }
}
void decode_wendy_setup(int* buf, int size)
{
printf("--- wendy_setup\n");
decode_options(&buf, &size);
}

void decode_no_more_data(int* buf, int size)
{
printf("--- no_more_data\n");
decode_options(&buf, &size);
}

void decode_events(int* buf, int size)
{
int flags;
int VED_ID;
int fragment_id;
int num;

printf("--- events\n");
decode_options(&buf, &size);
flags=*buf++; size--;
VED_ID=*buf++; size--;
fragment_id=*buf++; size--;
num=*buf++; size--;
printf("    flags=%d, VED=%d, frag=%d, num=%d\n",
    flags, VED_ID, fragment_id, num);
if (num)
  {
  int evsize;
  int evno;
  int trigno;
  int subno;
  evsize=*buf++; size--;
  evno=*buf++; size--;
  trigno=*buf++; size--;
  subno=*buf++; size--;
  printf("  event %d, size=%d, trigger=%d, %d subevents\n",
      evno, evsize, trigno, subno);
  }
}

void decode(int* buf, int rsize)
{
int size, endientest, typ;
if (rsize<3)
  {
  fprintf(stderr, "size=%d words\n", rsize);
  return;
  }
endientest=buf[1];
if (verbose) fprintf(stderr, "endientest=0x%08x\n", endientest);
switch (endientest)
  {
  int i;
  case 0x12345678: break;
  case 0x78563412:
    {
    for (i=0; i<rsize; i++)
      {
      buf[i]=swap_int(buf[i]);
      }
    }
    break;
  default:
    fprintf(stderr, "endientest=0x%08x\n", endientest);
    return;
  }
size=buf[0];
typ=buf[2];
if (verbose) printf("size=%d; type=%d\n", size, typ);
switch (typ)
  {
  case clusterty_events:
    if (verbose) printf("events\n");
    if (verbose || num_events==0) decode_events(buf+3, rsize-3);
    num_events++;
    break;
  case clusterty_ved_info:
    decode_ved_info(buf+3, rsize-3);
    num_events=0;
    break;
  case clusterty_text:
    decode_text(buf+3, rsize-3);
    num_events=0;
    break;
  case clusterty_wendy_setup:
    decode_wendy_setup(buf+3, rsize-3);
    num_events=0;
   break;
  case clusterty_no_more_data:
    decode_no_more_data(buf+3, rsize-3);
    num_events=0;
    break;
  default:
    printf("clustertyp 0x%08x\n", typ);
    num_events=0;
  }
}

main(int argc, char* argv[])
{
char* name;
int p;
int res;
int buf[16384];

if (argc<2)
  {
  fprintf(stderr, "usage: %s tape_device\n", argv[0]);
  return 0;
  }
p=open(argv[1], O_RDONLY, 0);
if (!p)
  {
  fprintf(stderr, "cannot open \"%s\": %s\n", argv[1], strerror(errno));
  return(2);
  }
do
  {
  res=read(p, (char*)buf, 65536);
  if (res<0)
    {
    printf("read: %s\n", strerror(errno));
    return 0;
    }
  if (res%4!=0)
    {
    fprintf(stderr, "size=%d bytes\n", res);
    return 3;
    }
  if (res==0)
    printf("Filemark\n");
  else if (res>0)
    {
    decode(buf, res/4);
    }
  }
while (res>=0);
}
