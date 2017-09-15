#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#define swap_int(a) ( ((a) << 24) | \
                      (((a) << 8) & 0x00ff0000) | \
                      (((a) >> 8) & 0x0000ff00) | \
        ((unsigned int)(a) >>24) )

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
switch (typ)
  {
  case clusterty_events:
    if ((num_events%100)==0) printf("events(%d); %d words\n", num_events, size);
    num_events++;
    break;
  case clusterty_ved_info:
    printf("ved_info; %d words\n", size);
    num_events=0;
    break;
  case clusterty_text:
    printf("text; %d words\n", size);
    num_events=0;
    break;
  case clusterty_wendy_setup:
    printf("wendy_setup; %d words\n", size);
    num_events=0;
   break;
  case clusterty_no_more_data:
    printf("no_more_data; %d words\n", size);
    num_events=0;
    break;
  default:
    printf("clustertyp 0x%08x; %d words\n", typ, size);
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
  res=read(p, (char*)buf, 32);
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
