/*
 * clientcommlib.c
 * created before: 16.08.94
 * 11.09.1998 PW: debugcl changed to policies
 */

#define _BSD_SOURCE
#define _OSF_SOURCE
#define _clientcommlib_c_
#include "config.h"
#include "clientcommlib.h"
#include <conststrings.h>
#include <swap.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#else
#include <string.h>
#endif
#include <errno.h>
#include <signal.h>
#if !defined(NEED_GETHOSTBYNAMEDEF)
#include <netdb.h>
#endif
/*#include <sys/uio.h>*/
#include <sys/socket.h>
#if !defined(NEED_NTOHLDEF) || (!defined(__cplusplus) && !defined(c_plusplus))
#include <netinet/in.h>
#endif
#include <arpa/inet.h>
#include <sys/un.h>
#include <netinet/tcp.h>
#include "compat.h"

typedef struct conf_ptr {
    ems_i32* ptr;
    struct conf_ptr* next;
    struct conf_ptr* prev;
} conf_ptr;

static conf_ptr* confptrs=0;

/*#define SENDLOG*/
/*#define SENDLOG1*/

/*****************************************************************************/

EMSerrcode EMS_errno;
static int sock= -1;
static int far;
static testenv tests;

static int lasttransid=0;
unsigned int client_id;

typedef struct Tsmsg {
    struct Tsmsg *next;
    msgheader header;
    ems_i32 *body;
} Tsmsg;

static Tsmsg *smsg=NULL;
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif
static void (*lasthand)(int);
#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
en_policies policies=pol_none;
static int mactive=0;
static int ignore_int=0;

static cbackproc ccommback=0;
void* ccommbackd=0;

/*****************************************************************************/

void addconfptr(ems_i32* ptr)
{
conf_ptr *ptr1;

if (ptr==0) return;
if (confptrs==0)
  {
  confptrs=(conf_ptr*)malloc(sizeof(conf_ptr));
  confptrs->ptr=ptr;
  confptrs->next=0;
  confptrs->prev=0;
  }
else
  {
  ptr1=confptrs;
  while (ptr1->next!=0)
    {
    if (ptr1->ptr==ptr) printf("ems_lib(%d): duplicate conf_ptr 0x%p\n",
      getpid(), ptr);
    ptr1=ptr1->next;
    }
  if (ptr1->ptr==ptr) printf("ems_lib(%d): duplicate conf_ptr 0x%p\n",
      getpid(), ptr);
  ptr1->next=(conf_ptr*)malloc(sizeof(conf_ptr));
  ptr1->next->next=0;
  ptr1->next->prev=ptr1;
  ptr1->next->ptr=ptr;
  }
ptr1=confptrs;
DC(
  while (ptr1!=0)
    {
    printf("ems_lib(%d): ptr 0x%p\n", getpid(), ptr1->ptr);
    ptr1=ptr1->next;
    }
  )
}

/*****************************************************************************/

int delconfptr(ems_i32* ptr)
{
conf_ptr *ptr1;

if (ptr==0) return 0;
if (confptrs==0)
  {
  printf("ems_lib(%d): no conf_ptrs allocated, 0x%p can not exist\n",
      getpid(), ptr);
  return(-1);
  }
else
  {
  ptr1=confptrs;
  while ((ptr1!=0) && (ptr1->ptr!=ptr)) ptr1=ptr1->next;
  if (ptr1==0)
    {
    printf("ems_lib(%d): conf_ptr 0x%p does not exist.\n", getpid(), ptr);
    return(-1);
    }
  else
    {
    if (ptr1==confptrs)
      {
      confptrs=ptr1->next;
      if (confptrs) confptrs->prev=0;
      free(ptr1);
      }
    else
      {
      if (ptr1->next) ptr1->next->prev=ptr1->prev;
      ptr1->prev->next=ptr1->next;
      free(ptr1);
      }
    return(0);
    }
  }
}

/*****************************************************************************/

void free_confirmation(ems_i32 *conf)
{/* free_confirmation */
    DC(printf("ems_lib(%d): free_confirmation(0x%08X)\n", getpid(), conf);)
    if (conf==0) return;
    if (delconfptr(conf)!=0) {
        if (tests.dumpcore)
            *(int*)0=0x12345678;
        else
            return;
    }
    free(conf);
}/* free_confirmation */

/*****************************************************************************/

void init_environment(testenv* t)
{
if (getenv("EMS_LIB_CONTR"))
  {
  printf("EMS_LIB_CONTR is set\n");
  tests.listcontrol=1;
  }
if (getenv("EMS_LIB_REQS"))
  {
  printf("EMS_LIB_REQS is set\n");
  tests.listrequests=1;
  }
if (getenv("EMS_LIB_CONFS"))
  {
  printf("EMS_LIB_CONFS is set\n");
  tests.listconfs=1;
  }
if (getenv("EMS_LIB_TIMES"))
  {
  printf("EMS_LIB_TIMES is set\n");
  tests.listtimes=1;
  }
if (getenv("EMS_LIB_TRACE"))
  {
  printf("EMS_LIB_TRACE is set\n");
  tests.listtrace=1;
  }
if (getenv("EMS_LIB_CORE"))
  {
  printf("EMS_LIB_CORE is set\n");
  tests.dumpcore=1;
  }
if (getenv("EMS_LIB_ALL"))
  {
  printf("EMS_LIB_ALL is set\n");
  tests.listcontrol=1;
  tests.listrequests=1;
  tests.listconfs=1;
  tests.listtimes=1;
  tests.listtrace=1;
  }
*t=tests;
}

/*****************************************************************************/

int getconfpath()
{
DCT(printf("ems_lib(%d): getconfpath=%d\n", getpid(), sock);)
return(sock);
}

/*****************************************************************************/

int int_ignore(int ignore)
{
int old;

old=ignore_int;
ignore_int=ignore;
return(old);
}

/*****************************************************************************/

int nexttransid(void)
{
if (++lasttransid==-1) lasttransid=1;
return(lasttransid);
}

/*****************************************************************************/

void printqueue()
{
Tsmsg *pntr;

if (smsg==NULL) {printf("  no messages in queue\n"); return;}

printf("messages in queue: ");
pntr=smsg;
while (pntr!=NULL)
  {
  /*printf("%d%s", pntr->header.transid, pntr->next==NULL?"\n":", ");*/
  printf("xid=%d; type=%d; flags=0x%X\n", pntr->header.transid,
      pntr->header.type.reqtype,pntr->header.flags);
  pntr=pntr->next;
  }
}

/*****************************************************************************/

static int sendblock(char *block, int size)
{/* sendblock */
int index=0, res=0;

DTR(printf("ems_lib(%d): sendblock\n", getpid());)

while((index<size) && (res!=-1))
  {
  res=write(sock, &block[index], size-index);
  if (res>=0)
    index+=res;
  else if (errno==EINTR)
    res=0;
  }
EMS_errno=(EMSerrcode)errno;
if (res<0)
  {
  Clientcommlib_abort(EMS_errno);
  return -1;
  }
else
  return 0;
}/* sendblock */

/*****************************************************************************/
int sendmessage(msgheader *header, ems_u32 *body)
{
    static int active=0;
    int res;
    int size;

#if 0
    {
    int i;
    if (header->flags&Flag_Intmsg)
        printf("sendmessage(%s, ved=%d)\n",
                Int_str(header->type.intmsgtype), header->ved);
    else
        printf("sendmessage(%s, ved=%d)\n",
                Req_str(header->type.reqtype), header->ved);
    for (i=0; i<header->size; i++) {
        printf("0x%x ", body[i]);
    }
    printf("\n");
    }
#endif
    if (active) {
        DTR(printf("ems_lib(%d): sendmessage already active\n", getpid());)
        EMS_errno=EMSE_NotReentrant;
        return(-1);
    }
    if (sock<1) {
        EMS_errno=EMSE_NotInitialised;
        return(-1);
    } else {
        active=1;
        size=header->size;
        if (far) swap_falls_noetig((ems_u32*)header, headersize);
        if ((res=sendblock((char*)header, sizeof(msgheader))==0)) {
            if (size>0) {
                if (far) swap_falls_noetig(body, size);
                res=sendblock((char*)body, size*4);
            }
        }
        if (body!=NULL) free(body);
        active=0;
        DTR(printf("ems_lib(%d): leave sendmessage, res=%d\n", getpid(), res);)
        return(res);
    }
}
/*****************************************************************************/

void send_info(const char *info)
{/* send_info */
#ifdef SENDLOG
int res;
int *body;

if (debugcl) return;
if (sock==-1)
  goto fehler;
else
  {
  header.size=1+strxdrlen(info);
  header.client=client_id;
  header.ved= EMS_commu;
  header.type=Intmsg_SendInfo;
  header.flags=Flag_Intmsg | Flag_NoLog;
  header.transid=0;/* soll geaendert werden */
  body=(int*)malloc(4*header.size);
  if (body==0) goto fehler;
  body[0]=Infomsg_textinfo;
  outstring(body+1, info);
  if (sendmessage(&header, body)==-1) goto fehler;
  }

return;
fehler:
  {
  FILE *f;
  f=fopen("/dev/console", "a");
  if (f)
    {
    fprintf(f, "clientcommlib: %s\n", info);
    fclose(f);
    }
  }
#else
return;
#endif
}/* send_info */

/*****************************************************************************/

void send_info1(const char *info)
{/* send_info1 */
#ifdef SENDLOG1
int res;
int *body;

if (debugcl) return;
if (sock==-1)
  goto fehler;
else
  {
  header.size=1+strxdrlen(info);
  header.client=client_id;
  header.ved= EMS_commu;
  header.type=Intmsg_SendInfo;
  header.flags=Flag_Intmsg | Flag_NoLog;
  header.transid=0;/* soll geaendert werden */
  body=(int*)malloc(4*header.size);
  if (body==0) goto fehler;
  body[0]=Infomsg_textinfo;
  outstring(body+1, info);
  if (sendmessage(&header, body)==-1) goto fehler;
  }

return;
fehler:
  {
  FILE *f;
  f=fopen("/dev/console", "a");
  if (f)
    {
    fprintf(f, "clientcommlib: %s\n", info);
    fclose(f);
    }
  }
#else
return;
#endif
}/* send_info1 */

/*****************************************************************************/

static int receiveblock(char *block, int size, int restart, int ignore_int)
{/* receiveblock */
static int index;
int res;

DTR(printf("ems_lib(%d): receiveblock\n", getpid());)

if (!restart) index=0;
while (index<size)
  {
  res=read(sock, &block[index], size-index);
  if (res>0)
    {
    index+=res;
    }
  else if (res<0)
    {
    if ((errno!=EINTR) || (!ignore_int && (index==0)))
      {
      EMS_errno=(EMSerrcode)errno;
      Clientcommlib_abort(EMS_errno);
      return(-1);
      }
    }
  else /* res==0 */
    {
    EMS_errno=(EMSerrcode)EPIPE;
    Clientcommlib_abort(EMS_errno);
    return(-1);
    }
  }
return(index);
}/* receiveblock */

/*****************************************************************************/

static int
receivemessage(msgheader **header, ems_i32 **body)
{/* receivemessage */
    static int restart=0, header_ok;
    static msgheader kopf;
    static int size;
    ems_i32 *block;
    int res;
    /*struct timezone tz;*/

    DTR(printf("ems_lib(%d): receivemessage\n", getpid());)

    if (sock<1) {
        EMS_errno=EMSE_NotInitialised;
        return(-1);
    }

    if (!restart) header_ok=0;
    if (!header_ok) {
        /* receive header */
        res=receiveblock((char*)&kopf, sizeof(msgheader), restart, ignore_int);
        if (res!=sizeof(msgheader)) {
            if (EMS_errno==EINTR) restart=1;
            return(-1);
        }
        restart=0;
        header_ok=1;
        if (far) swap_falls_noetig((ems_u32*)&kopf, headersize);
    /*  gettimeofday((struct timeval*)&kopf.utv6, &tz);*/
        size=kopf.size;
        if (size>0) {
            block=(ems_i32*)calloc(size, 4);
            if (block==0) {EMS_errno=(EMSerrcode)errno; return(-1);}
        }
    }
    *header=&kopf;

    if (size>0) {
        /* receive body */
        res=receiveblock((char*)block, size*4, restart, 1);
        if (res!=size*4) {
            if (EMS_errno==EINTR)
                restart=1;
            else
                free(block);
            return(-1);
        }
        restart=0;
        if (far) swap_falls_noetig((ems_u32*)block, size);
        *body=block;
    } else {
      *body=0;
    }

    return(0);
}/* receivemessage */

/*****************************************************************************/

int readmessage(int testpath, const struct timeval *timeout)
{/* readmessage */
ems_i32 *body;
msgheader *header;
Tsmsg *msg;
fd_set readfds;
int res, lastpath, ok;

DTR(printf("ems_lib(%d): readmessage\n", getpid());)
DT(
  if (timeout)
    {
    printf("ems_lib(%d): readmessage: timeout=(%d, %d)\n", getpid(),
        timeout->tv_sec, timeout->tv_usec);
    }
  else
    {
    int x;
    printf("ems_lib(%d): readmessage: timeout=NULL\n", getpid());
    x=*(int*)0;
    }
  )
if (mactive)
  {
  DTR(printf("ems_lib(%d): readmessage already active\n", getpid());)
  EMS_errno=EMSE_NotReentrant;
  return(-1);
  }
if (sock==-1) {EMS_errno=EMSE_NotInitialised; return(-1);}
mactive=1;

ok=0;
while (((testpath!=-1) || (timeout!=NULL)) && !ok)
  {
  FD_ZERO(&readfds);
  FD_SET(sock, &readfds); lastpath=sock;
  if (testpath>=0)
    {
    FD_SET(testpath, &readfds);
    if (testpath>lastpath) lastpath=testpath;
    }
  if (timeout)
    {
    struct timeval t=*timeout;
#ifndef __hpux__
    res=select(lastpath+1, &readfds, (fd_set*)0, (fd_set*)0, &t);
#else
    res=select(lastpath+1, (int*)&readfds, (fd_set*)0, (fd_set*)0, &t);
#endif
    }
  else
    {
#ifndef __hpux__
    res=select(lastpath+1, &readfds, (fd_set*)0, (fd_set*)0,
        (struct timeval*)0);
#else
    res=select(lastpath+1, (int*)&readfds, (fd_set*)0, (fd_set*)0,
        (struct timeval*)0);
#endif
    }
  if (res==0)
    {
    /*send_info("readmessage: timeout or interrupt");*/
    mactive=0;
    DTR(printf("ems_lib(%d): leave readmessage with timeout or interrupt\n",
        getpid());)
    return(0);
    }
  if (res==-1)
    {
    if ((errno!=EINTR) || !ignore_int || (timeout!=0))
      {
      EMS_errno=(EMSerrcode)errno;
      mactive=0;
      DTR(printf("ems_lib(%d): leave readmessage, errno=%d, ignore_int=%d, "
          "timeout=%d\n", getpid(), strerror(errno), ignore_int, timeout);)
      return(-1);
      }
    }
  else /* bleibt res>0 uebrig */
    {
    if ((testpath>=0) && (FD_ISSET(testpath, &readfds)))
      {
      /*send_info("readmessage: tested path has data");*/
      mactive=0;
      DTR(printf("ems_lib(%d): leave readmessage with data on tested path\n",
          getpid());)
      return(0);
      }
    ok=1;
    }
  }

if (receivemessage(&header, &body)==-1)
  {
  mactive=0;
  DTR(printf("ems_lib(%d): leave readmessage with error in receivemessage\n",
      getpid());)
  return(-1);
  }

if ((msg=(Tsmsg*)malloc(sizeof(Tsmsg)))==NULL)
  {
  EMS_errno=(EMSerrcode)errno;
  if (body) free(body);
  mactive=0;
  DTR(printf("ems_lib(%d): leave readmessage, no memory\n", getpid());)
  return(-1);
  }
msg->header=*header;
msg->body=body;
if (smsg==NULL)
  msg->next=NULL;
else
  msg->next=smsg;
smsg=msg;
mactive=0;
DTR(printf("ems_lib(%d): leave readmessage, ok\n", getpid());)
return(1);
}/* readmessage */

/*****************************************************************************/

int messageavailable()
{/* messageavailable */
struct timeval timeout;

DTR(printf("ems_lib(%d): messageavailable\n", getpid());)
if (smsg==NULL)
  {
  timeout.tv_sec=0;
  timeout.tv_usec=0;
  readmessage(0, &timeout);
  }
return(smsg!=NULL);
}/* messageavailable */

/*****************************************************************************/

int messagepending()
{
return(smsg!=NULL);
}

/*****************************************************************************/

int ungetmessage(msgheader* header, ems_i32 *body)
{
    Tsmsg *msg, *hmsg;

    if (mactive) {
        printf("ungetmessage reentered\n");
        EMS_errno=EMSE_NotReentrant;
        return(-1);
    }
    mactive=1;

    delconfptr(body);

    if ((msg=(Tsmsg*)malloc(sizeof(Tsmsg)))==NULL) {
        EMS_errno=(EMSerrcode)errno;
        if (body) free(body);
        mactive=0;
        return(-1);
    }
    msg->header=*header;
    msg->body=body;
    msg->next=NULL;
    if (smsg==NULL) {
        smsg=msg;
    } else {
        hmsg=smsg;
        while (hmsg->next!=0) hmsg=hmsg->next;
        hmsg->next=msg;
    }
    mactive=0;
    return(0);
}

/*****************************************************************************/

int getmessage(msgheader* header, ems_i32 **body)
{/* getmessage */
    Tsmsg *pntr;

    DTR(printf("ems_lib(%d): getmessage\n", getpid());)
    if (smsg==NULL) {
        DTR(printf("ems_lib(%d): leave getmessage, no message\n", getpid());)
        return(-1);
    }
    if (mactive) {
        DTR(printf("ems_lib(%d): leave getmessage, already active\n", getpid());)
        EMS_errno=EMSE_NotReentrant;
        return(-1);
    }
    mactive=1;
    *header=smsg->header;
    *body=smsg->body;
    pntr=smsg;
    smsg=smsg->next;
    free(pntr);
    addconfptr(*body);
    mactive=0;
    DTR(printf("ems_lib(%d): leave getmessage, ok\n", getpid());)
    return(0);
}/* getmessage */

/*****************************************************************************/

int getspecialmessage(ems_i32* xid, ems_u32* ved, union Reqtype* req,
    msgheader* header, ems_i32 **body)
{
Tsmsg *pntr1, *pntr2;

DTR(printf("ems_lib(%d): getspecialmessage\n", getpid());)
if (smsg==NULL)
  {
  DTR(printf("ems_lib(%d): leave getspecialmessage, no message\n", getpid());)
  return(-1);
  }

if (mactive)
  {
  DTR(printf("ems_lib(%s): leave getspecialmessage, already active\n",
      client_name);)
  EMS_errno=EMSE_NotReentrant;
  return(-1);
  }
mactive=1;

pntr1=smsg;
while ((pntr1!=0) &&
    ((xid && (*xid!=pntr1->header.transid)) ||
    (ved && (*ved!=pntr1->header.ved)) ||
    (req && (req->generic!=pntr1->header.type.generic)))) pntr1=pntr1->next;

if (pntr1==NULL)
  {
  mactive=0;
  DTR(printf("ems_lib(%d): leave getspecialmessage, no message\n", getpid());)
  return(-1);
  }
*header=pntr1->header;
*body=pntr1->body;

if (pntr1==smsg)
  smsg=smsg->next;
else
  {
  pntr2=smsg;
  while (pntr2->next!=pntr1) pntr2=pntr2->next;
  pntr2->next=pntr1->next;
  }
free(pntr1);
addconfptr(*body);
mactive=0;
DTR(printf("ems_lib(%d): leave getspecialmessage, ok\n", getpid());)
return(0);
}

/*****************************************************************************/
/*
int getspecialmessage(int transid, msgheader* header, int **body)
{
Tsmsg *pntr1, *pntr2;

DTR(printf("ems_lib(%s): getspecialmessage\n", client_name);)
if (smsg==NULL)
  {
  DTR(printf("ems_lib(%s): leave getspecialmessage, no message\n", client_name);)
  return(-1);
  }

if (mactive)
  {
  DTR(printf("ems_lib(%s): leave getspecialmessage, already active\n",
      client_name);)
  EMS_errno=EMSE_NotReentrant;
  return(-1);
  }
mactive=1;

pntr1=smsg;
while ((pntr1!=NULL) && (pntr1->header.transid!=transid)) pntr1=pntr1->next;

if (pntr1==NULL)
  {
  mactive=0;
  DTR(printf("ems_lib(%s): leave getspecialmessage, no message\n", client_name);)
  return(-1);
  }
*header=pntr1->header;
*body=pntr1->body;

if (pntr1==smsg)
  smsg=smsg->next;
else
  {
  pntr2=smsg;
  while (pntr2->next!=pntr1) pntr2=pntr2->next;
  pntr2->next=pntr1->next;
  }
free(pntr1);
addconfptr(*body);
mactive=0;
DTR(printf("ems_lib(%s): leave getspecialmessage, ok\n", client_name);)
return(0);
}
*/
/*****************************************************************************/
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif
void sig_pipe(int sig)
{
/*printf("clientcommlib: SIGPIPE erhalten\n");*/
}
#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
/*****************************************************************************/

static int commlib_start(int far)
{
if (sock!=-1) {EMS_errno=EMSE_Initialised; return(-1);}
DCT(printf("ems_lib(pid=%d): commlib_start\n", getpid());)

lasthand=signal(SIGPIPE, sig_pipe);
return(0);
}

/*****************************************************************************/

int Clientcommlib_init_u(const char* sockname)
{/* Clientcommlib_init_u */
u_int optval=1;
struct sockaddr_un sa;

if (commlib_start(0)==-1) return(-1);
DCT(printf("ems_lib(%d): commlib_init_u(%s)\n", getpid(), sockname);)
if ((sock=socket(AF_UNIX, SOCK_STREAM, 0))==-1)
  {
  EMS_errno=(EMSerrcode)errno;
  return(-1);
  }

if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&optval,
    sizeof(optval))==-1)
  {
  EMS_errno=(EMSerrcode)errno;
  close(sock); sock= -1;
  return(-1);
  }

bzero((char*)&sa, sizeof(struct sockaddr_un));
sa.sun_family=AF_UNIX;
strcpy(sa.sun_path, sockname);
if (connect(sock, (struct sockaddr*)&sa, sizeof(struct sockaddr_un))==-1)
  {
  EMS_errno=(EMSerrcode)errno;
  close(sock); sock= -1;
  return(-1);
  }
far=0;
if (ccommback) ccommback(1, 0, sock, ccommbackd);
return(0);
}/* Clientcommlib_init_u */

/*****************************************************************************/

int Clientcommlib_init_i(const char* hostname, int port)
{/* Clientcommlib_init_i */
u_int optval;
struct sockaddr_in sa;
struct hostent *host;

if (commlib_start(1)==-1) return(-1);
DCT(printf("ems_lib(%d): commlib_init_i(%s, %d)\n", getpid(), hostname, port);)
if ((sock=socket(AF_INET, SOCK_STREAM, 0))==-1)
  {
  EMS_errno=(EMSerrcode)errno;
  return(-1);
  }

optval=1;
if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&optval,
    sizeof(optval))==-1)
  {
  EMS_errno=(EMSerrcode)errno;
  close(sock); sock= -1;
  return(-1);
  }

optval=1;
if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
    (char*)&optval, sizeof(optval))==-1)
  {
  EMS_errno=(EMSerrcode)errno;
  close(sock); sock= -1;
  return(-1);
  }

bzero((char*)&sa, sizeof(struct sockaddr_in));
sa.sin_family=AF_INET;
sa.sin_port=htons(port);
host=gethostbyname((char*)hostname);
if (host!=0)
  {
  sa.sin_addr.s_addr= *(u_int*)(host->h_addr_list[0]);
  }
else
  {
  sa.sin_addr.s_addr=inet_addr((char*)hostname);
  if (sa.sin_addr.s_addr==(in_addr_t)-1)
    {
    EMS_errno=EMSE_HostUnknown;
    close(sock); sock= -1;
    return(-1);
    }
  }

if (connect(sock, (struct sockaddr*)&sa, sizeof(struct sockaddr_in))==-1)
  {
  EMS_errno=(EMSerrcode)errno;
  close(sock); sock= -1;
  return(-1);
  }
far=1;
if (ccommback) ccommback(1, 0, sock, ccommbackd);
return(0);
}/* Clientcommlib_init_i */

/*****************************************************************************/
#ifdef DECNET
int Clientcommlib_init_d(const char* hostname, int port)
{/* Clientcommlib_init_d */
u_int optval=1;
struct sockaddr_dn sa;
struct dn_naddr *add;
struct nodeent *node;
int i;

if (commlib_start(TRUE)==-1) return(-1);
if ((sock=socket(AF_DECnet, SOCK_STREAM, 0))==-1)
  {
  /*perror("Clientcommlib_init_d: cannot create socket");*/
  EMS_errno=errno;
  return(-1);
  }

if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&optval,
    sizeof(optval))==-1)
  {
  /*perror("Clientcommlib_init_d: cannot set socketoptions");*/
  EMS_errno=errno;
  close(sock); sock= -1;
  return(-1);
  }

bzero(&sa, sizeof(struct sockaddr_dn));
sa.sdn_family=AF_DECnet;
sa.sdn_objnum=150;
node=getnodebyname(hostname);
if (node!=0)
  {
  bcopy(node->n_addr, sa.sdn_nodeaddr, node->n_length);
  sa.sdn_nodeaddrl=node->n_length;
  }
else
  {
  add=dnet_addr(hostname);
  if (add==NULL)
    {
    EMS_errno=EMSE_HostUnknown;
    close(sock); sock= -1;
    return(-1);
    }
  sa.sdn_add= *add;
  }
if (connect(sock, &sa, sizeof(struct sockaddr_dn)) < 0)
  {
  /*perror("Clientcommlib_init_d:connect");*/
  EMS_errno=errno;
  close(sock); sock= -1;
  return(-1);
  }
far=TRUE;
if (ccommback) ccommback(1, 0, sock);
return(0);
}/* Clientcommlib_init_d */
#endif
/*****************************************************************************/

int Clientcommlib_done(void)
{/* Clientcommlib_done */
    ems_i32 *body;
    msgheader *headerptr;
    msgheader header;
    int oldsock;

    DCT(printf("ems_lib(%d): commlib_done()\n", getpid());)
    if (sock==-1) {
        EMS_errno=EMSE_NotInitialised;
        return-1;
    }
    if (mactive) {
        EMS_errno=EMSE_NotReentrant;
        return(-1);
    }
    mactive=1;
    header.size=0;
    header.client=client_id;
    header.ved= EMS_commu;
    header.type.intmsgtype=Intmsg_Close;
    header.flags=Flag_Intmsg;
    header.transid=0;/* soll geaendert werden */
    if (sendmessage(&header, NULL)!=-1) {
        do {
            if (receivemessage(&headerptr, &body)==-1) break;
            if (body!=NULL) free(body);
        }
        while (((headerptr->flags&Flag_Intmsg)==0) ||
              (headerptr->type.intmsgtype!=Intmsg_Close));
    }
    oldsock=sock;
    close(sock);
    sock= -1;
    signal(SIGPIPE, lasthand);
    mactive=0;
    if (ccommback) ccommback(0, 0, oldsock, ccommbackd);
    return(0);
}/* Clientcommlib_done */

/*****************************************************************************/

void Clientcommlib_abort(int reason)
{
int oldsock=sock;
DCT(printf("ems_lib(%d): commlib_abort(%d)\n", getpid(), reason);)
if (sock>=0) close(sock);
sock= -1;
if (ccommback) ccommback(-1, reason, oldsock, ccommbackd);
}

/*****************************************************************************/

cbackproc install_ccommback(cbackproc proc, void* data)
{
cbackproc old;
old=ccommback;
ccommback=proc;
ccommbackd=data;
return old;
}

/*****************************************************************************/
/*****************************************************************************/
