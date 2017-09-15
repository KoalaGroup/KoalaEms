/******************************************************************************
*                                                                             *
* pipecomm.c                                                                  *
*                                                                             *
* OS9                                                                         *
*                                                                             *
* 09.03.94                                                                    *
*                                                                             *
* MD                                                                          *
*                                                                             *
******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: pipecomm.c,v 1.8 2011/04/06 20:30:21 wuestner Exp $: pipecomm.c,v 1.7 2009/08/10 21:02:20 wuestner Exp ";

#include <sconf.h>
#include <debug.h>
#include <msg.h>
#include <requesttypes.h>
#include <intmsgtypes.h>
#include <errorcodes.h>
#include <rcs_ids.h>

#ifdef DMALLOC
#include dmalloc.h
#endif

static int pin,pout;
extern msgheader header;
extern int connected;

RCS_REGISTER(cvsid, "commu/pipe")

/*****************************************************************************/

errcode _init_comm(port)
char *port;
{
char pinname[265];
char poutname[265];

if (port)
  {
  strcpy(pinname, port);
  strcpy(poutname, port);
  }
else
  {
  strcpy(pinname, DEFPORT);
  strcpy(poutname, DEFPORT);
  }
strcat(pinname, "in");
strcat(poutname, "out");
DV(D_COMM, printf("inpipe: %s, outpipe: %s\n", pinname, poutname);)
if ((pin=creat(pinname, 3))==-1)
  return(Err_System);
if ((pout=creat(poutname, 3))==-1)
  return(Err_System);
return(OK);
}

errcode _done_comm(){
  return(OK);
}

/*****************************************************************************/

Request get_request(reqbuf, siz)
int **reqbuf, *siz;
{
int len, restlen, da, size;
char *bufptr;
int *msg;

T(get_request)
do
  {
  restlen=sizeof(msgheader);
  bufptr=(char*)&header;
  while (restlen)
    {
    da=read(pin, bufptr, restlen);
    if (da>0)    /*falls Error, wird nicht gezaehlt*/
      {
      DV(D_COMM, printf("%d Headerbytes angekommen\n",da);)
      bufptr+=da;
      restlen-=da;
      }
    }
  size=*siz=header.size;
  DV(D_COMM, printf("Request %d, Flags %d: es folgen %d Bytes!\n",
      header.type,header.flags, size*sizeof(int));)
  if (size>0)
    {
    msg= *reqbuf=(int*)calloc(size, sizeof(int));
    restlen=size*sizeof(int);
    bufptr=(char*)msg;
    while (restlen)
      {
      da=read(pin, bufptr, restlen);
      if (da>0)
        {
        DV(D_COMM, printf("%d Bytes angekommen\n", da);)
        bufptr+=da;
        restlen-=da;
        }
      }
    }
  else msg= *reqbuf=(int*)0;
  if (header.flags&Flag_Intmsg)
    {
    D(D_COMM, printf("Intmsg, sofort zurueck\n");)
    header.flags|=Flag_Confirmation;
    header.size=0;
    write(pout, &header, sizeof(msgheader));
    if (msg) free(msg);
    if (header.type.intmsgtype==Intmsg_Open)
      connected=1;
    else if (header.type.intmsgtype==Intmsg_Close)
      connected=0;
    }
  }
while(header.flags&Flag_Intmsg);
return(header.type.reqtype);
}

/*****************************************************************************/

Request get_request_noblock(reqbuf, siz)
int **reqbuf, *siz;
{
int len, restlen, da, size;
char *bufptr;
int *msg;

T(get_request_noblock)
if (_gs_rdy(pin)<1) return(Req_Nothing);
restlen=sizeof(msgheader);
bufptr=(char*)&header;
while (restlen)
  {
  da=read(pin, bufptr, restlen);
  if (da>0)    /*falls Error, wird nicht gezaehlt*/
    {
    DV(D_COMM, printf("%d Headerbytes angekommen\n",da);)
    bufptr+=da;
    restlen-=da;
    }
  }
size=*siz=header.size;
DV(D_COMM, printf("Es folgen %d Bytes!\n", size*sizeof(int));)
if (size>0)
  {
  msg= *reqbuf=(int*)calloc(size, sizeof(int));
  restlen=size*sizeof(int);
  bufptr=(char*)msg;
  while(restlen)
    {
    da=read(pin, bufptr, restlen);
    if (da>0)
      {
      DV(D_COMM, printf("%d Bytes angekommen\n", da);)
      bufptr+=da;
      restlen-=da;
      }
    }
  }
else
  msg= *reqbuf=(int*)0;
if (header.flags&Flag_Intmsg)
  {
  header.flags|=Flag_Confirmation;
  header.size=0;
  write(pout, &header, sizeof(msgheader));
  if (msg) free(msg);
  if (header.type.intmsgtype==Intmsg_Open)
    connected=1;
  else if (header.type.intmsgtype==Intmsg_Close)
    connected=0;
  return(Req_Nothing);
  }
return(header.type.reqtype);
}

/*****************************************************************************/

void send_confirmation(header, confbuf, size)
msgheader *header;
int *confbuf;
int size;
{
int res;

T(send_confirmation)
header->size=size;
res=write(pout, (char*)header, sizeof(msgheader));
DV(D_COMM, printf("sent msgheader: %d bytes of %d.\n", res, sizeof(msgheader));)
res=write(pout, (char*)confbuf, size*sizeof(int));
DV(D_COMM, printf("sent body: %d bytes of %d.\n", res, size*sizeof(int));)
}

/*****************************************************************************/

void free_msg(msg)
int *msg;
{
T(free_msg)
if (msg) free((char*)msg);
}

/*****************************************************************************/
/*****************************************************************************/

