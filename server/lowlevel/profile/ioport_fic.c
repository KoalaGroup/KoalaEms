#include <sconf.h>

#include "zilogs.h"

#ifdef FIC8234
#define CNTLADR 0x5980000c
#define PORTADR 0x59800008 /* Port A */
#define mccrbit 2

#define zset(adr,val) *(int*)(adr)=(val)
#define zget(adr) (*(int*)(adr))

static int port_init[]={
  pAms_,0x00, /* init as bit port */
  pAhs_,0x00, /* handshake specification */
  pAcs_,0x00, /* null */
  pAdp_,0x00, /* data path polarity, bit 0..7 not inverted */
  pAdd_,0x00, /* data direction, init as output */
  pAsc_,0x00, /* special IO control, standard TTL */
  pAdr_,0x00, /* write 0 in port A */
  -1
}
#endif

#ifdef FIC8232
#define CNTLADR 0x11000003
#define PORTADR 0x11000001 /* Port B */
#define mccrbit 7
#define zset(adr,val) *(char*)(adr)=(char)(val)
#define zget(adr) (*(char*)(adr))
static int port_init[]={
  pBms_,0x00, /* init as bit port */
  pBhs_,0x00, /* handshake specification */
  pBcs_,0x00, /* null */
  pBdp_,0x00, /* data path polarity, bit 0..7 not inverted */
  pBdd_,0x00, /* data direction, init as output */
  pBsc_,0x00, /* special IO control, standard TTL */
  pBdr_,0x00, /* write 0 in port B */
  -1
}
#endif

static void Zinit(addr,tab)
void *addr;
int *tab;
{
  int *tptr,help;
  tptr=tab;
  while((help= *tptr++)!=-1){
    zset(addr,help);
    tsleep(2);
    help= *tptr++;
    zset(addr,help);
    tsleep(2);
  }
}

int init_profile_port(){
  int help;
  Zinit(CNTLADR,port_init);
  zset(CNTLADR,mccr_);
  tsleep(2);
  help=zget(CNTLADR);
  tsleep(2);
  zset(CNTLADR,mccr_);
  zset(CNTLADR,help&(1<<mccrbit));
  return(0);
}

void write_profile_port(x)
int x;
{
  zset(PORTADR,x);
}

int done_profile_port(){
  return(0);
}
