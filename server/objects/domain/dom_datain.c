/*
 * objects/domain/dom_datain.c
 * created before 02.02.94
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: dom_datain.c,v 1.23 2015/04/06 21:35:00 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#ifdef OSK
#include <types.h>
#include <in.h>
#endif
#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>

#include "dom_datain.h"
#include "domdiobj.h"
#include "../pi/readout.h"
#include "../pi/datain.h"

#ifdef DMALLOC
#include dmalloc.h
#endif

RCS_REGISTER(cvsid, "objects/domain")

/*****************************************************************************/

extern ems_u32* outptr;

datain_info datain[MAX_DATAIN];

static void free_datain_res(int i)
{
  switch(datain[i].addrtyp){
    case Addr_Modul:
      if(datain[i].addr.modulname)
        free(datain[i].addr.modulname);
      break;
    case Addr_LocalSocket:
      if(datain[i].addr.localsockname)
        free(datain[i].addr.localsockname);
      break;
    case Addr_V6Socket:
        free(datain[i].addr.inetV6sock.addr);
        free(datain[i].addr.inetV6sock.node);
        free(datain[i].addr.inetV6sock.service);
        break;
    case Addr_Driver_syscall:
    case Addr_Driver_mixed:
    case Addr_Driver_mapped:
      if(datain[i].addr.driver.path)
        free(datain[i].addr.driver.path);
      break;
  }
  if (datain[i].bufinfo) free(datain[i].bufinfo);
  if (datain[i].addrinfo) free(datain[i].addrinfo);
  datain[i].bufinfo=datain[i].addrinfo=0;
}

/*****************************************************************************/

errcode dom_datain_init()
{
int i;

T(dom_datain_init)
for (i=0; i<MAX_DATAIN; i++)
  {
  datain[i].bufftyp= -1;
  datain[i].bufinfo=(int*)0;
  datain[i].addrinfo=(int*)0;
  }
return(OK);
}

/*****************************************************************************/

errcode dom_datain_done()
{
    int i;
    T(dom_datain_done)

    for (i=0;i<MAX_DATAIN;i++){
        if(datain[i].bufftyp!=-1){
            remove_datain(i);
            free_datain_res(i);
            datain[i].bufftyp= -1;
        }
    }
    return(OK);
}

/*****************************************************************************/
/*
 * downloaddatain (bisher)
 * p[0] : Domain-ID (index)
 * p[1] : Buffer-Type
 * p[2] : Address-Type
 * p[3] : raw-address | modulname | driverpath
 * [p[...] : space,offset,option]
 *
 * downloaddatain (neu)
 * p[0] : Domain-ID (index)
 * p[1] : Buffer-Type
 * p[2] : -1
 * p[3] : argnum
 * p[4...] : Buffer args
 * p[4+argnum] : Address-Type
 * p[5+argnum] : argnum
 * p[6+argnum ...] : Address args
 */

static errcode downloaddatain(ems_u32* p, unsigned int len)
{
int index, res, i, l;
ems_u32 *q, *r;
int qlen, rlen;

T(downloaddatain)
D(D_REQ, {
  int i;
  printf("DownloadDomain Datain(");
  for (i=0; i<len; i++) printf("%d%s", p[i], i+1<len?", ":"");
  printf(")\n");})
if ((unsigned int)len<3) return Err_ArgNum;

if (p[2]==-1)
  {
  q=p+4;
  qlen=p[3];
  if (len<qlen+6) {*outptr++=1; return Err_ArgNum;}
  r=p+qlen+6;
  rlen=p[qlen+5];
  if (len<qlen+rlen+6) {*outptr++=2; return Err_ArgNum;}
  }
else
  {
  q=p+2;
  qlen=0;
  r=p+3;
  rlen=len-3;
  }
index=p[0];
if ((unsigned int) index>=MAX_DATAIN) return (Err_IllDomain);
if (readout_active) return (Err_PIActive);
if (datain[index].bufftyp!=-1) return (Err_DomainDef);
if (p[2]==-1)
  datain[index].addrtyp=p[qlen+4];
else
  datain[index].addrtyp=p[2];

switch(datain[index].addrtyp){
#ifdef OSK
  case Addr_Raw:
    if(rlen!=1)return(Err_ArgNum);
    datain[index].addr.addr=(int*)r[0];
    r++; rlen--;
    break;
#endif
  case Addr_Modul:
    if((rlen<1)||(rlen!=xdrstrlen(r)))return(Err_ArgNum);
    l=xdrstrlen(r);
    r=xdrstrcdup(&datain[index].addr.modulname, r);
    if(!(datain[index].addr.modulname))return(Err_NoMem);
    r+=l; rlen-=l;
    break;
  case Addr_Socket:
    if (rlen==1) /* passiv */
      {
      datain[index].addr.inetsock.port=r[0];
      datain[index].addr.inetsock.host=INADDR_ANY;
      r++; rlen--;
      }
    else if (rlen==2) /* active */
      {
      datain[index].addr.inetsock.host=r[0];
      datain[index].addr.inetsock.port=r[1];
      r+=2; rlen-=2;
      }
    else
      return(Err_ArgNum);
    break;
  case Addr_V6Socket: {
        struct addrinfo hints;
        int res;

        if (rlen<1) { /* we need the flags */
            *outptr++=6;
            return Err_ArgNum;
        }
        datain[index].addr.inetV6sock.flags=r[0];
        r++; rlen--;
        if (!(datain[index].addr.inetV6sock.flags&ip_passive)) {
            if ((rlen<1)||(rlen<xdrstrlen(r))) { /* we need a node string */
                *outptr++=7;
                return Err_ArgNum;
            }
            xdrstrcdup(&datain[index].addr.inetV6sock.node, r);
            rlen-=xdrstrlen(r);
            r+=xdrstrlen(r);
            if (!datain[index].addr.inetV6sock.node)
                return Err_NoMem;
        } else {
            datain[index].addr.inetV6sock.node=0;
        }
        if ((rlen<1)||(rlen<xdrstrlen(r))) { /* we need a service string */
            *outptr++=8;
            free(datain[index].addr.inetV6sock.node);
            return Err_ArgNum;
        }
        xdrstrcdup(&datain[index].addr.inetV6sock.service, r);
        if (!datain[index].addr.inetV6sock.service) {
            free(datain[index].addr.inetV6sock.node);
            return Err_NoMem;
        }

        memset(&hints, 0, sizeof(struct addrinfo));
        if (datain[index].addr.inetV6sock.flags&ip_passive)
            hints.ai_flags=AI_PASSIVE;
        if (datain[index].addr.inetV6sock.flags&ip_v6only)
            hints.ai_family=AF_INET6;
        else if (datain[index].addr.inetV6sock.flags&ip_v4only)
            hints.ai_family=AF_INET; /* to be corrected later */
        else
            hints.ai_family=AF_INET;
        hints.ai_socktype=SOCK_STREAM; /* ??? */
        hints.ai_protocol=IPPROTO_TCP; /* ??? */
        res=getaddrinfo(
                datain[index].addr.inetV6sock.node,
                datain[index].addr.inetV6sock.service,
                &hints,
                &datain[index].addr.inetV6sock.addr);
        if (res) {
            printf("dom_datain(%d):getaddrinfo: %s\n",
                    index, gai_strerror(res));
            free(datain[index].addr.inetV6sock.node);
            free(datain[index].addr.inetV6sock.service);
            return Err_ArgRange;
        }
    }
    break;
  case Addr_LocalSocket:
    if((rlen<1)||(rlen!=xdrstrlen(r)))return(Err_ArgNum);
    l=xdrstrlen(r);
    xdrstrcdup(&datain[index].addr.localsockname, r);
    if (!(datain[index].addr.localsockname)) return(Err_NoMem);
    r+=l; rlen-=l;
    break;
  case Addr_Driver_syscall:
  case Addr_Driver_mixed:
  case Addr_Driver_mapped:{
    ems_u32 *help;
    if((rlen<4)||(rlen!=3+xdrstrlen(r)))return(Err_ArgNum);
    l=xdrstrlen(r);
    help=xdrstrcdup(&datain[index].addr.driver.path, r);
    if(!(datain[index].addr.driver.path))return(Err_NoMem);
    datain[index].addr.driver.space= *help++;
    datain[index].addr.driver.offset= *help++;
    datain[index].addr.driver.option= *help;
    l+=3;
    r+=l; rlen-=l;
    break;
    }
  default:
    return(Err_AddrTypNotImpl);
  }
datain[index].bufftyp=p[1];
datain[index].bufinfosize=qlen;
datain[index].bufinfo=malloc(qlen*sizeof(int));
for (i=0; i<qlen; i++) datain[index].bufinfo[i]=q[i];
datain[index].addrinfosize=rlen;
#if 0
printf("downloaddatain: addrinfosize=%d\n", datain[index].addrinfosize);
#endif
datain[index].addrinfo=malloc(rlen*sizeof(int));
for (i=0; i<rlen; i++) datain[index].bufinfo[i]=r[i];
if ((res=datain_pre_init(index, qlen, q))!=OK)
  {
  free_datain_res(index);
  datain[index].bufftyp=-1;
  return res;
  }
else
  return OK;
}

/*****************************************************************************/
errcode uploaddatain(ems_u32* p, unsigned int len)
{
    ems_u32 *help=0;
    int i;
    unsigned int index;

    T(uploaddatain)

    if (len<1)
        return Err_ArgNum;

    index=p[0];
    if (index>=MAX_DATAIN)
        return(Err_ArgRange);
    if (datain[index].bufftyp==-1)
        return Err_NoDomain;

/*
 *     We assume that no old client still exists.
 *     Therefore we will use the new protocol (with exlicit argument counters)
 *     The implementation of the new protocol was badly broken before
 *     2014-09-07, uploaddatain could never work (but it is normally not
 *     needed).
 */

    /* marker for new protocol */
    *outptr++=-1;

    /* info about the used buffer */
    *outptr++=datain[index].bufftyp;
    *outptr++=datain[index].bufinfosize;
    for (i=0; i<datain[index].bufinfosize; i++)
        *outptr++=datain[index].bufinfo[i];

    /* info about the used address */
    *outptr++=datain[index].addrtyp;
    help=outptr++; /* placeholder for number of args */

    switch(datain[index].addrtyp) {
#ifdef OSK
    case Addr_Raw:
        *outptr++=(int)(datain[index].addr.addr);
        /* break missing here? */
#endif
    case Addr_Modul:
        outptr=outstring(outptr, datain[index].addr.modulname);
        break;
    case Addr_Socket:
        if (datain[index].addr.inetsock.host==INADDR_ANY) { /* passiv */
            *outptr++=datain[index].addr.inetsock.port;
        } else { /* active */
            *outptr++=datain[index].addr.inetsock.host;
            *outptr++=datain[index].addr.inetsock.port;
        }
        break;
    case Addr_V6Socket:
        *outptr++=datain[index].addr.inetV6sock.flags;
        if (!(datain[index].addr.inetV6sock.flags & ip_passive))
            outptr=outstring(outptr, datain[index].addr.inetV6sock.node);
        outptr=outstring(outptr, datain[index].addr.inetV6sock.service);
        break;
    case Addr_LocalSocket:
        outptr=outstring(outptr, datain[index].addr.localsockname);
        break;
    case Addr_Driver_syscall:
    case Addr_Driver_mixed:
    case Addr_Driver_mapped:
        outptr=outstring(outptr, datain[index].addr.driver.path);
        *outptr++=datain[index].addr.driver.space;
        *outptr++=datain[index].addr.driver.offset;
        *outptr++=datain[index].addr.driver.option;
        break;
    case Addr_Null:
        break;
    default:
        return(Err_Program);
    }

    /* additional address info (if any) */
    for (i=0; i<datain[index].addrinfosize; i++)
        *outptr++=datain[index].addrinfo[i];

    *help=outptr-help-1;

    return OK;
}
/*****************************************************************************/

errcode deletedatain(ems_u32* p, unsigned int len)
{
int index;
errcode res;
T(deletedatain)

if (len<1) return(Err_ArgNum);
index=p[0];
D(D_REQ, printf("deletedatain(%d)\n", p[0]);)
if ((unsigned int)index>=MAX_DATAIN) return(Err_ArgRange);
if (readout_active) return(Err_PIActive);
if (datain[index].bufftyp==-1) return(Err_NoDomain);
if ((res=remove_datain(index))) return(res);
free_datain_res(index);
datain[index].bufftyp= -1;
return(OK);
}

/*****************************************************************************/

static objectcommon *lookup_dom_di(ems_u32* id, unsigned int idlen,
        unsigned int* remlen)
{
  T(lookup_dom_di)
  if(idlen>0){
    if((unsigned int)id[0]>=MAX_DATAIN)return(0);
    *remlen=idlen;
    return((objectcommon*)&dom_di_obj);
  }else{
    *remlen=0;
    return((objectcommon*)&dom_di_obj);
  }
}

/*****************************************************************************/

static ems_u32 *dir_dom_di(ems_u32* ptr)
{
  int i;
  for(i=0; i<MAX_DATAIN; i++)
    if(datain[i].bufftyp!=-1)
      *ptr++=i;
  return(ptr);
}

/*****************************************************************************/

domobj dom_di_obj={
  {
    0,0,
    /*(lookupfunc)*/lookup_dom_di,
    dir_dom_di,
    0
  },
  downloaddatain,
  uploaddatain,
  deletedatain
};

/*****************************************************************************/
/*****************************************************************************/
