/*
 * dataout/stream/socket/sockstream.c
 * created 08.12.96 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sockstream.c,v 1.9 2011/04/06 20:30:22 wuestner Exp $";

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <objecttypes.h>
#include <swap.h>
#include <rcs_ids.h>

#include "../../../objects/domain/dom_dataout.h"
#include "../../../objects/pi/readout.h"
#include "../../dataout.h"

int *next_databuf;
int buffer_free;

extern ems_u32 *outptr;

typedef enum {unused, queued, collecting, sending} bufusage;
typedef struct
  {
  bufusage flag;
  int size;
  int sequence;
  } bufferinfo;

static int do_idx;
static int buffer_num, buffer_size, buffer_mark;
static int** bufarr=0;
static bufferinfo* bufinfo=0;
static int collectseq, sendseq;
static int collectbufidx, sendbufidx;
static int collectidx; /* Anzahl der Integer */
static int sendidx;    /* Anzahl der Byte */
static int *sendbuf, *collectbuf;
static int unusedbufnum, queuedbufnum;
static int sendsize;
static int last_databuf, last_dataidx;
static int fd;

RCS_REGISTER(cvsid, "dataout/stream/socket")

/*****************************************************************************/

static void pool_dump()
{
int i;

printf("eventcnt=%d; collectseq=%d; sendseq=%d\n", eventcnt, collectseq,
  sendseq);
printf("  buffer_num: %d, unusedbufnum: %d, queuedbufnum: %d\n",
  buffer_num, unusedbufnum, queuedbufnum);
for (i=0; i<buffer_num; i++)
  {
  printf("  %d: buf=%p; flag=%d; seq=%d; size=%d\n", i, bufarr[i],
      bufinfo[i].flag, bufinfo[i].sequence, bufinfo[i].size);
  }
}

/*****************************************************************************/

static void pool_check(const char* n, int x)
{
int num_unused=0, num_queued=0, num_collecting=0, num_sending=0;
int i, unknown, fehler=0, aux=0;
for (i=0; i<buffer_num; i++)
  {
  switch (bufinfo[i].flag)
    {
    case unused:
      num_unused++;
      break;
    case queued:
      num_queued++;
      if (bufinfo[i].sequence<sendseq) {fehler=1; aux=i;}
      if (bufinfo[i].sequence>collectseq) {fehler=2; aux=i;}
      break;
    case collecting:
      if (bufinfo[i].sequence!=collectseq) {fehler=3; aux=i;}
      num_collecting++;
      break;
    case sending:
      if (bufinfo[i].sequence!=sendseq) {fehler=4; aux=i;}
      num_sending++;
      break;
    default:
      unknown++;
      fehler=5;
      aux=i;
      break;
    }
  }

if (unusedbufnum!=num_unused) fehler=6;
if (queuedbufnum!=num_queued) fehler=7;

if (fehler)
  {
  printf("sockstream.c: pool_check(%s): consistency error %d.%d (idx=%d)!!\n",
      n, fehler, aux, x);
  printf("  unused: %d, queued: %d, collecting: %d, sending: %d\n",
    num_unused, num_queued, num_collecting, num_sending);
  if (readout_active==Invoc_active) fatal_readout_error();
  pool_dump();
  }
}

/*****************************************************************************/

static void pool_init()
{
int i;

T(pool_init)
for (i=0; i<buffer_num; i++) bufinfo[i].flag=unused;
unusedbufnum=buffer_num;
queuedbufnum=0;
sendbuf=0;
collectbuf=0;
pool_check("pool_init", -1);
}

/*****************************************************************************/

static void pool_get_collectbuf()
{
int idx=0;

if (unusedbufnum==0) return;
while ((idx<buffer_num) && (bufinfo[idx].flag!=unused)) idx++;
if (idx==buffer_num)
  {
  printf("Fehler in pool_get_collectbuf: no buffer free\n");
  pool_dump();
  if (readout_active==Invoc_active) fatal_readout_error();
  }
else
  {
  unusedbufnum--;
  bufinfo[idx].flag=collecting;
  bufinfo[idx].sequence=++collectseq;
  collectbufidx=idx;
  collectbuf=bufarr[idx];
  collectidx=0;
  pool_check("pool_get_collectbuf", idx);
  }
}

/*****************************************************************************/

void cleanup()
{
if (bufarr)
  {
  int i;
  for (i=0; i<buffer_num; i++) {if (bufarr[i]) free(bufarr[i]);}
  free(bufarr); bufarr=0;
  }
if (bufinfo)
  {
  free(bufinfo);
  bufinfo=0;
  }
}

/*****************************************************************************/

int printuse_output(FILE* outfilepath)
{
return 0;
}

/*****************************************************************************/

errcode dataout_init(char* arg)
{
T(dataout_init)
#ifdef EVENT_BUFBEG
###############################################################
#ERROR in sockstream.c: EVENT_BUFBEG darf nicht definiert sein#.
###############################################################
#endif

bufarr=0;
bufinfo=0;
do_idx=-1;
fd=-1;
buffer_free=0;
return OK;
}

/*****************************************************************************/

errcode dataout_done()
{
T(dataout_done)
if (fd>=0) close(fd);
fd= -1;
cleanup();
do_idx=-1;
return OK;
}

/*****************************************************************************/

static errcode try_to_open(int idx)
{
struct sockaddr_in sa;
int nochmal, res;

printf("try_to_open(do_idx=%d, host=%08X, port=%d)\n",
    idx, dataout[idx].addr.inetsock.host, dataout[idx].addr.inetsock.port);
if ((fd=socket(AF_INET,SOCK_STREAM,0))<0)
  {
  printf("socket(...): %s\n", strerror(errno));
  *outptr++=errno;
  return Err_System;
  }
if (fcntl(fd, F_SETFD, FD_CLOEXEC)<0)
  {
  printf("sockstream.c: fcntl(fd, FD_CLOEXEC): %s\n", strerror(errno));
  }
sa.sin_family=AF_INET;
sa.sin_addr.s_addr=htonl(dataout[idx].addr.inetsock.host);
sa.sin_port=htons(dataout[idx].addr.inetsock.port);
do
  {
  nochmal=0;
  if ((res=connect(fd, (struct sockaddr*)&sa, sizeof(struct sockaddr_in)))==-1)
    {
    printf("connect(...): %s\n", strerror(errno));
    if (errno==EINTR) nochmal=1;
    }
  }
while (nochmal);
if (res==-1)
  {
  *outptr++=errno;
  close(fd); fd=-1;
  return Err_System;
  }
if (fcntl(fd, F_SETFL, O_NDELAY)==-1)
  {
  printf("fcntl(O_NDELAY): %s\n", strerror(errno));
  *outptr++=errno;
  close(fd); fd=-1;
  return Err_System;
  }
return OK;
}


/*****************************************************************************/

errcode start_dataout(void)
{
errcode res;

T(start_dataout)
/*printf("start_dataout()\n");*/
if (do_idx<0)
  {
  printf("start_dataout: kein dataout geladen\n");
  return Err_NoDo;
  }
pool_init();
buffer_free=0;
collectseq=0;
sendseq=1;
last_databuf=-1;
if ((res=try_to_open(do_idx))!=OK) return res;
pool_init();
return OK;
}

/*****************************************************************************/

static void pool_queuesendbuf(int idx)
{
bufinfo[idx].flag=queued;
queuedbufnum++;
pool_check("queuesendbuf", idx);
}

/*****************************************************************************/

static void pool_dequeuesendbuf()
{
int idx;

if (queuedbufnum==0)
  {
  pool_check("dequeuesendbuf", -1);
  return;
  }
idx=0;
while ((idx<buffer_num) && ((bufinfo[idx].flag!=queued)
    || (bufinfo[idx].sequence!=sendseq))) idx++;
if (idx==buffer_num)
  {
  printf("Fehler in getnewsendbuf; search for sequence %d\n", sendseq);
  if (readout_active==Invoc_active) fatal_readout_error();
  pool_dump();
  return;
  }
queuedbufnum--;
bufinfo[idx].flag=sending;
sendsize=bufinfo[idx].size*sizeof(int);
sendbufidx=idx;
sendidx=0;
sendbuf=bufarr[idx];
pool_check("dequeuesendbuf", idx);
swap_falls_noetig(sendbuf, bufinfo[idx].size);
if (last_databuf==idx) last_databuf=-1;
}

/*****************************************************************************/

static void pool_freebuf(int idx)
{
bufinfo[idx].flag=unused;
unusedbufnum++;
pool_check("freebuf", idx);
}

/*****************************************************************************/

static int try_to_write(int final)
{
int res;

if (sendbuf==0)
  {
  if (final) printf("try_to_write: dequeuesendbuf\n");
  pool_dequeuesendbuf();
  }
if (sendbuf==0)
  {
  if (final) printf("try_to_write: no sendbuf\n");
  return 0;
  }
/*
 * if (final)
 *     printf("try_to_write: sendbuf=%p, sendbufidx=%d\n", sendbuf, sendbufidx);
 */

res=write(fd, ((char*)sendbuf)+sendidx, sendsize-sendidx);
if (res<0)
  {
  switch (errno)
    {
    case EINTR:
      printf("try_to_write: %s; seq=%d; queued=%d\n", strerror(errno),
          bufinfo[sendbufidx].sequence, queuedbufnum);
    case EAGAIN: break;
    default:
      printf("try_to_write(): %s\n", strerror(errno));
      if (readout_active==Invoc_active) fatal_readout_error();
      break;
    }
  return 0;
  }
else
  {
  if (res==0)
    {
    printf("try_to_write: res=0; sendbuf=%p; sendidx=%d; sendsize=%d\n",
      sendbuf, sendidx, sendsize);
    }
  sendidx+=res;
  if (sendidx>=sendsize)
    {
    if (final) printf("try_to_write: free %d\n", sendbufidx);
    pool_freebuf(sendbufidx);
    sendbuf=0;
    sendseq++;
    }
  return 1;
  }
}

/*****************************************************************************/

errcode stop_dataout(void)
{
int count=100;
T(stop_dataout)
/*printf("stop_dataout()\n");*/
if (collectbuf)
  {
  printf("stop_dataout: queuesendbuf; size=%d\n", collectidx);
  bufinfo[collectbufidx].size=collectidx;
  pool_queuesendbuf(collectbufidx);
  }
while ((sendbuf || queuedbufnum))
  {
  /*printf("stop: sendbuf=%p; queuedbufnum=%d\n", sendbuf, queuedbufnum);*/
  while (--count && try_to_write(1));
  pool_check("stop_dataout", -1);
  }  
close(fd); fd=-1;
return OK;
}

/*****************************************************************************/

void flush_databuf(int* p)
{
int len;

T(flush_databuf)
len=p-next_databuf;
collectbuf[collectidx]=len;
last_databuf=collectbufidx;
last_dataidx=collectidx;
collectidx+=len+1;
next_databuf=collectbuf+collectidx+1;
if (collectidx>buffer_mark)
  {
  bufinfo[collectbufidx].size=collectidx;
  pool_queuesendbuf(collectbufidx);
  collectbuf=0;
  while (try_to_write(0));
  pool_get_collectbuf();
  if (collectbuf)
    next_databuf=collectbuf+1;
  else
    buffer_free=0;
  }
}

/*****************************************************************************/

errcode get_last_databuf(void)
{
int *ptr, len;

T(get_last_databuf)
if ((last_databuf>=0) && (bufinfo[last_databuf].flag!=unused))
  {
  ptr=bufarr[last_databuf]+last_dataidx+1; len=ptr[-1];
  while (len--) *outptr++=*ptr++;
  return OK;
  }
else
  return Err_NoDomain;
}

/*****************************************************************************/

void schau_nach_speicher(void)
{
T(schau_nach_speicher)
try_to_write(0);
pool_get_collectbuf();
if (collectbuf)
  {
  next_databuf=collectbuf+1;
  buffer_free=1;
  }
}

/*****************************************************************************/
/*
 * q[0]: buffersize
 * q[1]: priority (==0); ignored
 * [q[2]: buffer_num
 *  q[3]: buffer_mark]
 */
errcode insert_dataout(int idx)
{
int i;

T(insert_dataout)
/*printf("insert_dataout(idx=%d)\n", idx);*/
if (idx!=0) return Err_IllDomain;
if (dataout[0].bufftyp!=InOut_Stream) return Err_BufTypNotImpl;
if (dataout[0].addrtyp!=Addr_Socket) return Err_AddrTypNotImpl;
if (dataout[0].inout_arg_num>2) return Err_ArgNum;
/*printf("buffsize=%d\n", dataout[0].buffsize);*/
if (dataout[0].inout_arg_num==0)
  {
  buffer_num=10;
  buffer_size=dataout[0].buffsize?dataout[0].buffsize:event_max*2;
  buffer_mark=buffer_size/2;
  }
else
  {
  /*
  for (i=0; i<dataout[0].inout_arg_num; i++)
      printf("args[%d]: %d\n", i, dataout[0].inout_args[i]);
  */
  buffer_num=dataout[0].inout_args[0];
  if (dataout[0].inout_arg_num>1) buffer_mark=dataout[0].inout_args[1];
  if (buffer_num<1)
    {
    *outptr++=1;
    return Err_ArgRange;
    }
  if (buffer_size<event_max)
    {
    *outptr++=2;
    return Err_ArgRange;
    }
  if ((buffer_mark<0) || (buffer_mark>buffer_size-event_max))
    {
    *outptr++=3;
    return Err_ArgRange;
    }
  }
/*
printf("  buffer_num=%d; buffer_size=%d; buffer_mark=%d\n",
    buffer_num, buffer_size, buffer_mark);
*/
if ((bufarr=(int**)calloc(buffer_num, sizeof(int*)))==0)
  {
  cleanup();
  *outptr++=1;
  return Err_NoMem;
  }
for (i=0; i<buffer_num; i++)
  {
  if ((bufarr[i]=(int*)malloc(buffer_size*sizeof(int)))==0)
    {
    cleanup();
    *outptr++=2;
    return Err_NoMem;
    }
  }
if ((bufinfo=(bufferinfo*)malloc(buffer_num*sizeof(bufferinfo)))==0)
  {
  cleanup();
  *outptr++=3;
  return Err_NoMem;
  }
do_idx=idx;
return OK;
}

/*****************************************************************************/

errcode remove_dataout(int idx)
{
T(remove_dataout)
/*printf("remove_dataout(%d)\n", idx);*/
if (idx!=0) return Err_IllDomain;
if (dataout[0].bufftyp!=InOut_Stream) return Err_BufTypNotImpl;
if (dataout[idx].addrtyp!=Addr_Socket) return Err_AddrTypNotImpl;
if (fd>=0) close(fd);
fd= -1;
cleanup();
do_idx=-1;
return OK;
}

/*****************************************************************************/
/*****************************************************************************/

