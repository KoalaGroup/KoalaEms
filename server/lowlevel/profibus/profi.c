static const char* cvsid __attribute__((unused))=
    "$ZEL: profi.c,v 1.3 2011/04/06 20:30:27 wuestner Exp $";

#include <stdio.h>
#include <errorcodes.h>
#include <rcs_ids.h>

/* Achtung, "OK" wird neu definiert! -- gluecklicherweise geht es gut */

#include "mem_pep.h"
#include "pbL2con.h"
#include "pbL2type.h"

#include "profi.h"

RCS_REGISTER(cvsid, "lowlevel/profibus")

#define DEF_STATION 1
#define DEF_SPEED 5
#define DEF_HSA 10
#define SEND_BUF_LEN 255
#define RECV_BUF_LEN 255
#define VENDOR "EMS-PROFILIB"
#define CONTROLLER "PEP VM30"
#define HW_REL "1"
#define SW_REL "1"

static int profi_open=0;
static T_FDL_SERVICE_DESCR *sdb;
static T_BUSPAR_BLOCK *par;
static T_FDL_SR_BLOCK *descr;
static char *sendbuf;
static char *recvbuf;

static int currenthsa;

struct rescdesc{
  int sap;
  char *buf;
  struct rescdesc *next;
};

static struct rescdesc *rescs=(struct rescdesc*)0;

extern T_FDL_SERVICE_DESCR *fdl_con_ind();

static int alloc_resources(){
  sdb=(T_FDL_SERVICE_DESCR*)malloc(sizeof(T_FDL_SERVICE_DESCR));
  if(!sdb)return(1);
  par=(T_BUSPAR_BLOCK*)malloc(sizeof(T_BUSPAR_BLOCK));
  if(!par){
    free(sdb);
    return(1);
  }
  descr=(T_FDL_SR_BLOCK*)malloc(sizeof(T_FDL_SR_BLOCK));
  if(!descr){
    free(sdb);
    free(par);
    return(1);
  }
  sendbuf=(char*)xsrqcmem(SEND_BUF_LEN,TPRAM);
  if(!sendbuf){
    free(sdb);
    free(par);
    free(descr);
    return(1);
  }
  recvbuf=(char*)xsrqcmem(RECV_BUF_LEN,TPRAM);
  if(!recvbuf){
    free(sdb);
    free(par);
    free(descr);
    free(sendbuf);
    return(1);
  }
  rescs=(struct rescdesc*)0;
  return(0);
}

static void free_resources(){
  struct rescdesc *ptr;
  if(sdb)free(sdb);
  if(par)free(par);
  if(descr)free(descr);
  if(sendbuf)free(sendbuf);
  if(recvbuf)free(recvbuf);
  ptr=rescs;
  while(ptr){
    if(ptr->buf)free(ptr->buf);
    ptr=ptr->next;
  }
}

static errcode setbusparam(station,bauds,hsa)
int station,bauds,hsa;
{
  T_FDL_SERVICE_DESCR *rsdb;

  par->loc_add.station=station;
  par->loc_add.segment=NO_SEGMENT;
  switch(bauds){
    case 5: /* fuer Kommunikation mit WIENER */
      par->baud_rate=K_BAUD_500;
      par->tsl=4000;
      par->min_tsdr=300; /* geaendert */
      par->max_tsdr=1000;
      par->tqui=0;
      par->tset=150; /* geaendert */
      par->ttr=50000;
      par->g=2;
      break;
    case 4:
      par->baud_rate=K_BAUD_500;
      par->tsl=4000;
      par->min_tsdr=100;
      par->max_tsdr=2000;
      par->tqui=0;
      par->tset=50;
      par->ttr=50000;
      par->g=2;
      break;
    case 3:
      par->baud_rate=K_BAUD_187_5;
      par->tsl=2000;
      par->min_tsdr=40;
      par->max_tsdr=1000;
      par->tqui=0;
      par->tset=20;
      par->ttr=25000;
      par->g=2;
      break;
    case 2:
      par->baud_rate=K_BAUD_93_75;
      par->tsl=1000;
      par->min_tsdr=25;
      par->max_tsdr=500;
      par->tqui=0;
      par->tset=40;
      par->ttr=13000;
      par->g=2;
      break;
    case 1:
      par->baud_rate=K_BAUD_19_2;
      par->tsl=400;
      par->min_tsdr=10;
      par->max_tsdr=200;
      par->tqui=0;
      par->tset=4;
      par->ttr=9000;
      par->g=2;
      break;
    case 0:
      par->baud_rate=K_BAUD_9_6;
      par->tsl=400;
      par->min_tsdr=20;
      par->max_tsdr=100;
      par->tqui=0;
      par->tset=2;
      par->ttr=5000;
      par->g=2;
      break;
    default:
      return(Err_ArgRange);
  }
  par->hsa=hsa;
  par->medium_red=NO_REDUNDANCY;
  par->in_ring_desired=TRUE;
  par->max_retry_limit=2;
  par->ident=(USIGN8*)malloc(4+strlen(VENDOR)+strlen(CONTROLLER)+
			     strlen(HW_REL)+strlen(SW_REL)+1);
  if(!(par->ident))return(Err_NoMem);
  par->ident[0]=strlen(VENDOR);
  par->ident[1]=strlen(CONTROLLER);
  par->ident[2]=strlen(HW_REL);
  par->ident[3]=strlen(SW_REL);
  strcpy(par->ident+4,VENDOR);
  strcat(par->ident+4,CONTROLLER);
  strcat(par->ident+4,HW_REL);
  strcat(par->ident+4,SW_REL);
  par->ind_buf_len=0;

  sdb->sap=MSAP_0;
  sdb->service=FMA2_SET_BUSPARAMETER;
  sdb->primitive=REQ;
  sdb->descr_ptr=(USIGN8*)par;

  fdl_req(sdb);
  rsdb=fdl_con_ind();
  if((!rsdb)||((rsdb->status!=OK)&&(rsdb->status!=LR)))
    return(Err_System);
  if(rsdb->status==LR){
    T_BUSPAR_BLOCK bpars;
    printf("Profibus aktiv - Parameter nicht geaendert\n");
    sdb->sap=MSAP_0;
    sdb->service=FMA2_READ_BUSPARAMETER;
    sdb->primitive=REQ;
    sdb->descr_ptr=(USIGN8*)&bpars;

    fdl_req(sdb);
    rsdb=fdl_con_ind();
    if((!rsdb)||(rsdb->status!=OK)){
      printf("kann aktuelle Profibus-Einstellungen nicht lesen\n");
      return(Err_System);
    }
    currenthsa=bpars.hsa;
  }else currenthsa=hsa;
  return(OK);
}

errcode profibus_low_init(arg)
char *arg;
{
  char *profipath;
  int station,bauds,hsa;
  errcode res;

  if((!arg)||(cgetstr(arg,"profidev",&profipath)<0))
    profipath="/profi_2";
  if(fdl_open(profipath)==-1){
    printf("kann %s nicht oeffnen\n",profipath);
    profi_open=0;
    return(Err_System);
  }else profi_open=1;

  if(alloc_resources()){
    printf("kann Profibus-Resourcen nicht allozieren\n");
    return(Err_NoMem);
  }
  if((!arg)||(cgetnum(arg,"profistation",&station)<0))
    station=DEF_STATION;
  if((!arg)||(cgetnum(arg,"profispeed",&bauds)<0))
    bauds=DEF_SPEED;
  if((!arg)||(cgetnum(arg,"profihsa",&hsa)<0))
    hsa=DEF_HSA;
  if(res=setbusparam(station,bauds,hsa)){
    printf("kann Profibus nicht konfigurieren\n");
    return(res);
  }
  return(OK);
}

void deactivate_all_saps();

errcode profibus_low_done(){
  if(profi_open){
    deactivate_all_saps();
    fdl_close();
    profi_open=0;
    free_resources();
  }
  return(OK);
}

int activate_client_sap(nr,rstation)
int nr,rstation;
{
  struct rescdesc *resc;
  T_FDL_SAP_DESCR *sap;
  T_FDL_SAP_BLOCK *block;
  int slen,rlen;
  T_FDL_SERVICE_DESCR *rsdb;

  resc=(struct rescdesc*)malloc(sizeof(struct rescdesc));
  sap=(T_FDL_SAP_DESCR*)malloc(sizeof(T_FDL_SAP_DESCR));
  resc->sap=nr;
  resc->buf=(char*)sap;
  resc->next=rescs;
  rescs=resc;
  resc=(struct rescdesc*)malloc(sizeof(struct rescdesc));
  block=(T_FDL_SAP_BLOCK*)malloc(sizeof(T_FDL_SAP_BLOCK));
  resc->sap=nr;
  resc->buf=(char*)block;
  resc->next=rescs;
  rescs=resc;
  slen=SEND_BUF_LEN-(FDL_OFFSET+FDL_TRAILER);
  rlen=RECV_BUF_LEN-(FDL_OFFSET+FDL_TRAILER);

  block->max_len_sda_req_low=slen;
  block->max_len_sda_req_high=slen;
  block->max_len_sdn_req_low=slen;
  block->max_len_sdn_req_high=slen;
  block->max_len_srd_req_low=slen;
  block->max_len_srd_req_high=slen;
  block->max_len_sda_ind_low=rlen;
  block->max_len_sda_ind_high=rlen;
  block->max_len_sdn_ind_low=rlen;
  block->max_len_sdn_ind_high=rlen;
  block->max_len_srd_con_low=rlen;
  block->max_len_srd_con_high=rlen;

  sap->sap_nr=nr;
  sap->rem_add.station=rstation;
  sap->rem_add.segment=NO_SEGMENT;
  sap->sda=INITIATOR;
  sap->sdn=INITIATOR;
  sap->srd=INITIATOR;
  sap->csrd=SERVICE_NOT_ACTIVATED;
  sap->sap_block_ptr=(USIGN8*)block;

  sdb->sap=MSAP_2;
  sdb->service=FMA2_ACTIVATE_SAP;
  sdb->primitive=REQ;
  sdb->descr_ptr=(USIGN8*)sap;

  fdl_req(sdb);
  rsdb=fdl_con_ind();
  if(!rsdb)return(-1);
  if(rsdb->status!=OK)return(rsdb->status);
  return(0);
}

int deactivate_sap(nr)
int nr;
{
  T_FDL_DEACT_SAP Sap;
  T_FDL_DEACT_SAP *sap;
  T_FDL_SERVICE_DESCR *rsdb;
  struct rescdesc **ptr;

  sap= &Sap;

  sap->ssap=nr;
  sdb->sap=MSAP_2;
  sdb->service=FMA2_DEACTIVATE_SAP;
  sdb->primitive=REQ;
  sdb->descr_ptr=(USIGN8*)sap;

  fdl_req(sdb);
  rsdb=fdl_con_ind();
  if(!rsdb)return(-1);
  if(rsdb->status!=OK)return(rsdb->status);

  ptr= &rescs;
  while(*ptr){
    if((*ptr)->sap==nr){
      struct rescdesc *help;
      if((*ptr)->buf)free((*ptr)->buf);
      help=(*ptr)->next;
      free(*ptr);
      *ptr=help;
    }else ptr= &((*ptr)->next);
  }
  return(0);
}

void deactivate_all_saps(){
  while(rescs)deactivate_sap(rescs->sap);
}

int do_sda(sap,dest,remsap,data,len)
int sap,dest,remsap;
char *data;
int len;
{
  T_FDL_SERVICE_DESCR *rsdb;

  memcpy(sendbuf+FDL_OFFSET,data,len);

  descr->remote_sap=remsap;
  descr->rem_add.station=dest;
  descr->rem_add.segment=NO_SEGMENT;
  descr->serv_class=LOW;
  descr->send_data.buffer_ptr=(USIGN8*)sendbuf;
  descr->send_data.length=len;

  sdb->sap=sap;
  sdb->service=SDA;
  sdb->primitive=REQ;
  sdb->descr_ptr=(USIGN8*)descr;

  fdl_req(sdb);
  rsdb=fdl_con_ind();
  if(!rsdb)return(-1);
  if(rsdb->status!=OK)return(rsdb->status);
  return(0);
}

int do_srd(sap,dest,remsap,sdata,slen,rdata,rlen)
int sap,dest,remsap;
char *sdata;
int slen;
char *rdata;
int *rlen;
{
  T_FDL_SERVICE_DESCR *rsdb;

  memcpy(sendbuf+FDL_OFFSET,sdata,slen);

  descr->remote_sap=remsap;
  descr->rem_add.station=dest;
  descr->rem_add.segment=NO_SEGMENT;
  descr->serv_class=LOW;
  descr->send_data.buffer_ptr=(USIGN8*)sendbuf;
  descr->send_data.length=slen;
  descr->resource.buffer_ptr=(USIGN8*)recvbuf;
  descr->resource.length=RECV_BUF_LEN;

  sdb->sap=sap;
  sdb->service=SRD;
  sdb->primitive=REQ;
  sdb->descr_ptr=(USIGN8*)descr;

  fdl_req(sdb);
  rsdb=fdl_con_ind();
  if(!rsdb)return(-1);
  if(rsdb->status!=DL)return(rsdb->status);
  if(*rlen<descr->receive_data.length)return(-2);
  *rlen=descr->receive_data.length;
  memcpy(rdata,descr->receive_data.buffer_ptr,*rlen);
  return(0);
}

int do_sdn(sap,dest,remsap,data,len)
int sap,dest,remsap;
char *data;
int len;
{
  T_FDL_SERVICE_DESCR *rsdb;

  memcpy(sendbuf+FDL_OFFSET,data,len);

  descr->remote_sap=remsap;
  descr->rem_add.station=dest;
  descr->rem_add.segment=NO_SEGMENT;
  descr->serv_class=LOW;
  descr->send_data.buffer_ptr=(USIGN8*)sendbuf;
  descr->send_data.length=len;

  sdb->sap=sap;
  sdb->service=SDN;
  sdb->primitive=REQ;
  sdb->descr_ptr=(USIGN8*)descr;

  fdl_req(sdb);
  rsdb=fdl_con_ind();
  if(!rsdb)return(-1);
  if(rsdb->status!=OK)return(rsdb->status);
  return(0);
}

int profi_getlas(buf,len)
char *buf;
int *len;
{
  T_FDL_SERVICE_DESCR *rsdb;
  char buf2[127];

  /* wir glauben hsa=currenthsa, kann aber
   von anderen geaendert worden sein */

  if(*len<currenthsa+1)return(-2);

  sdb->sap=MSAP_0;
  sdb->service=FMA2_READ_LAS;
  sdb->primitive=REQ;
  sdb->descr_ptr=(USIGN8*)buf2;

  fdl_req(sdb);
  rsdb=fdl_con_ind();
  if(!rsdb)return(-1);
  if(rsdb->status!=OK)return(rsdb->status);
  memcpy(buf,buf2,currenthsa+1);
  *len=currenthsa+1;
  return(0);
}

int profi_getlivelist(buf,len)
char *buf;
int *len;
{
  T_FDL_SERVICE_DESCR *rsdb;
  char buf2[127];

  if(*len<currenthsa+1)return(-2);

  sdb->sap=MSAP_0;
  sdb->service=FMA2_LIVELIST;
  sdb->primitive=REQ;
  sdb->descr_ptr=(USIGN8*)buf2;

  fdl_req(sdb);
  rsdb=fdl_con_ind();
  if(!rsdb)return(-1);
  if(rsdb->status!=OK)return(rsdb->status);
  memcpy(buf,buf2,currenthsa+1);
  *len=currenthsa+1;
  return(0);
}

int profi_ident(dest,buf,len)
int dest;
char *buf;
int *len;
{
  T_FDL_SERVICE_DESCR *rsdb;

  descr->rem_add.station=dest;
  descr->rem_add.segment=NO_SEGMENT;
  descr->send_data.buffer_ptr=(USIGN8*)sendbuf;
  descr->send_data.length=0;
  descr->resource.buffer_ptr=(USIGN8*)recvbuf;
  descr->resource.length=RECV_BUF_LEN;

  sdb->sap=MSAP_0;
  sdb->service=FMA2_IDENT;
  sdb->primitive=REQ;
  sdb->descr_ptr=(USIGN8*)descr;

  fdl_req(sdb);
  rsdb=fdl_con_ind();
  if(!rsdb)return(-1);
  if((rsdb->status!=OK)&&(rsdb->status!=DL))return(rsdb->status);
  /* !!! DL widerspricht Beschreibung */
  if(*len<descr->receive_data.length)return(-2);
  *len=descr->receive_data.length;
  memcpy(buf,descr->receive_data.buffer_ptr,*len);
  return(0);
}

int profi_getstatus(buf)
struct profistatus *buf;
{
  T_BUSPAR_BLOCK bpars;
  T_FDL_STATISTIC_CTR stats;
  USIGN32 trr;
  T_FDL_SERVICE_DESCR *rsdb;

  sdb->sap=MSAP_0;
  sdb->service=FMA2_READ_BUSPARAMETER;
  sdb->primitive=REQ;
  sdb->descr_ptr=(USIGN8*)&bpars;

  fdl_req(sdb);
  rsdb=fdl_con_ind();
  if(!rsdb)return(-1);
  if(rsdb->status!=OK)return(rsdb->status);

  sdb->sap=MSAP_0;
  sdb->service=FMA2_READ_STATISTIC_CTR;
  sdb->primitive=REQ;
  sdb->descr_ptr=(USIGN8*)&stats;

  fdl_req(sdb);
  rsdb=fdl_con_ind();
  if(!rsdb)return(-1);
  if(rsdb->status!=OK)return(rsdb->status);

  sdb->sap=MSAP_0;
  sdb->service=FMA2_READ_TRR;
  sdb->primitive=REQ;
  sdb->descr_ptr=(USIGN8*)&trr;

  fdl_req(sdb);
  rsdb=fdl_con_ind();
  if(!rsdb)return(-1);
  if(rsdb->status!=OK)return(rsdb->status);

  buf->station=bpars.loc_add.station;
  buf->baud_rate=bpars.baud_rate;
  buf->tsl=bpars.tsl;
  buf->min_tsdr=bpars.min_tsdr;
  buf->max_tsdr=bpars.max_tsdr;
  buf->tqui=bpars.tqui;
  buf->tset=bpars.tset;
  buf->ttr=bpars.ttr;
  buf->g=bpars.g;
  buf->hsa=bpars.hsa;
  buf->max_retry_limit=bpars.max_retry_limit;
  buf->frame_send_count=stats.frame_send_count;
  buf->retry_count=stats.retry_count;
  buf->sd_count=stats.sd_count;
  buf->sd_error_count=stats.sd_error_count;
  buf->trr=trr;
  return(0);
}
