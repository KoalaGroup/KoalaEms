/*
 * clientcomm.c
 * $ZEL: clientcomm.c,v 2.29 2017/10/21 19:10:02 wuestner Exp $
 *
 * created before: 16.08.94
 * 14.05.1998 PW: EMSE_Unknown changed to EMSE_CommuTimeout
 * 11.09.1998 PW: debugcl changed to policies
 * 22.03.1999 PW: GetDataoutStatus_1 added
 * 16.Apr.2004 PW: FlushDataout added
 */

#define _clientcomm_c_
#define _DEFAULT_SOURCE
#include "config.h"
#include "clientcommlib.h"
#include "clientcomm.h"
#include <sys/types.h>
#include <sys/time.h>
#if !defined(NEED_GETHOSTBYNAMEDEF)
#include <netdb.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <sys/errno.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#else
#include <string.h>
#endif
#include <pwd.h>
#include <unistd.h>
#include <ems_err.h>
#include <xdrstring.h>
#include <sys/socket.h>
#if !defined(NEED_NTOHLDEF) || (!defined(__cplusplus) && !defined(c_plusplus))
#include <netinet/in.h>
#endif
#include "compat.h"

/* $ZEL: clientcomm.c,v 2.29 2017/10/21 19:10:02 wuestner Exp $ */

static int testpath= -1;
static struct timeval deftimeout_={20, 0};
static struct timeval *deftimeout= &deftimeout_;
static testenv tests;
#ifdef NEED_SYS_ERRLIST
extern int sys_nerr;
extern char *sys_errlist[];
#endif
extern char *emserrlstr[];
extern char client_name[];

static char experiment[1024]="";

int def_unsol=0;

/*****************************************************************************/

int test_path(int path)
{
int old;

DTR(printf("ems_lib(%s): test_path(%d)\n", client_name, path);)
old=testpath;
testpath=path;
return(old);
}

/*****************************************************************************/

void settimeout(int sec)
{
DT(printf("ems_lib(%s): settimeout(%d)\n", client_name, sec);)
if (sec<=0)
  deftimeout=NULL;
else
  {
  deftimeout_.tv_sec=sec;
  deftimeout= &deftimeout_;
  }
}

/*****************************************************************************/

static void subtime(struct timeval *t, const struct timeval *sub)
{
t->tv_sec-=sub->tv_sec;
t->tv_usec-=sub->tv_usec;
if (t->tv_usec<0)
  {
  t->tv_sec-=1;
  t->tv_usec+=1000000;
  }
}

/*****************************************************************************/

static void restzeit(const struct timeval *start, const struct timeval *zeit,
  struct timeval *rest)
{
struct timeval jetzt;
struct timezone tz;

gettimeofday(&jetzt, &tz);
subtime(&jetzt, start);
*rest=*zeit;
subtime(rest, &jetzt);
}

/*****************************************************************************/

static int negzeit(const struct timeval *t)
{
if (t->tv_sec<0) return(1);
if (t->tv_sec>0) return(0);
if (t->tv_usec>0) return(0);
return(1);
}

/*****************************************************************************/

int GetConfirmation(ems_u32* ved, union Reqtype* requesttype,
    ems_u32* transaction, ems_u32* size, ems_i32** confirmation,
    const struct timeval *timeout)
{
    ems_i32 *body;
    int testtime, res;
    struct timeval start, rest;
    msgheader header;

    DC(printf("ems_lib(%s): GetConfirmation\n", client_name);)
    DT(
        if (timeout)
            printf("ems_lib(%s): GetConfirmation; timeout=(%d, %d)\n",
                    client_name, timeout->tv_sec, timeout->tv_usec);
        else
            printf("ems_lib(%s): GetConfirmation; timeout=NULL\n", client_name);
    )

    while (getmessage(&header, &body)==0) {
        DC(printf("ems_lib(%s): Got Confirmation %d\n", client_name,
                header.transid);)
        if ((header.flags & Flag_Intmsg)==0)
                goto ok;
        if (policies&pol_nodebug) {/* misuse: pol_nodebug means 'debug client'*/
            header.type.reqtype=(Request)(header.type.reqtype|0x80000000U);
            goto ok;
        }
        DC(printf("ems_lib(%s): delete internal confirmation\n", client_name);)
        free_confirmation(body);
    }

    testtime=(timeout!=0) && ((timeout->tv_sec!=0) || (timeout->tv_usec!=0));
    if (testtime) gettimeofday(&start, 0);
    while (1) {
        if (testtime) {
            restzeit(&start, timeout, &rest);
            if (negzeit(&rest)) {
                DT(printf("ems_lib(%s): GetConfirmation timed out\n",
                        client_name);)
                return(0);
            }
            res=readmessage(testpath, &rest);
        } else {
            res=readmessage(testpath, timeout);
        }
        if (res<1)
                return(res);
        getmessage(&header, &body);
        DC(printf("ems_lib(%s): Got Confirmation %d\n", client_name,
                header.transid);)
        if ((header.flags & Flag_Intmsg)==0)
                goto ok;
        if (policies&pol_nodebug) {
            header.type.reqtype=(Request)(header.type.reqtype|0x80000000U);
            goto ok;
        }
        DC(printf("ems_lib(%s): delete internal confirmation\n", client_name);)
        free_confirmation(body);
    }

ok:
    if (ved) *ved=header.ved;
    if (requesttype) *requesttype=header.type;
    *transaction=header.transid;
    *size=header.size;
    *confirmation=body;
    DC(printf("ems_lib(%s): return confirmation %d; 0x%08X\n", client_name,
        *transaction, *confirmation);)
    return(1);
}/* GetConfirmation */

/*****************************************************************************/

int GetConfirmation_h(ems_i32** confirmation, msgheader* header,
    const struct timeval *timeout)
{
    int res;

    DC(printf("ems_lib(%s): GetConfirmation_h(...)\n", client_name);)
    if (getmessage(header, confirmation)!=0) {
        res=readmessage(testpath, timeout);
        if (res<1) return(res);
        getmessage(header, confirmation);
    }
    return(1);
}

/*****************************************************************************/

int GetSpecialConfirmation(ems_u32* ved, union Reqtype* requesttype,
    ems_i32* transaction, ems_u32* size, ems_i32** confirmation,
    const struct timeval *timeout)
{
    ems_i32 *body;
    int testtime, res;
    struct timeval start, rest;
    struct timezone tz;
    msgheader header;

    DC(printf("ems_lib(%s): GetSpecialConfirmation(xid=%d)\n", client_name,
            *transaction);)
    DT(
        if (timeout)
            printf("ems_lib(%s): GetSpecialConfirmation; timeout=(%d, %d)\n",
                    client_name, timeout->tv_sec, timeout->tv_usec);
        else
            printf("ems_lib(%s): GetSpecialConfirmation; timeout=NULL\n",
                    client_name);
    )

    /*if (getspecialmessage(*transaction, &header,  &body)==0) goto ok;*/
    if (getspecialmessage(transaction, 0, 0, &header,  &body)==0)
            goto ok;

    testtime=(timeout!=0) && ((timeout->tv_sec!=0) || (timeout->tv_usec!=0));
    if (testtime) gettimeofday(&start, &tz);
    while (1) {
        if (testtime) {
            restzeit(&start, timeout, &rest);
            if (negzeit(&rest)) {
                DT(printf("ems_lib(%s): GetSpecialConfirmation timed out\n",
                        client_name);)
                return(0);
            }
            res=readmessage(testpath, &rest);
        } else {
            res=readmessage(testpath, timeout);
        }
        if (res<1) return(res);
        if (getspecialmessage(transaction, 0, 0, &header, &body)==0) goto ok;
    }

ok:
    if (header.flags & Flag_Intmsg)
            header.type.reqtype=(Request)(header.type.reqtype|0x80000000U);
    if (ved) *ved=header.ved;
    if (requesttype) *requesttype=header.type;
    *size=header.size;
    *confirmation=body;
    DC(printf("ems_lib(%s): return confirmation %d; 0x%08X\n", client_name,
            *transaction, *confirmation);)
    return(1);
}/* GetSpecialConfirmation */

/*****************************************************************************/

int GetConfir(ems_i32* xid, ems_u32* ved, union Reqtype* req,
    ems_i32** confirmation, msgheader* header, const struct timeval *timeout)
{
    ems_i32 *body;
    int testtime, res;
    struct timeval start, rest;

    DC(printf("ems_lib(%s): GetConfir(...)\n", client_name);)
    if (getspecialmessage(xid, ved, req, header, &body)==0)
            goto ok;

    testtime=(timeout!=0) && ((timeout->tv_sec!=0) || (timeout->tv_usec!=0));
    if (testtime) gettimeofday(&start, 0);
    while (1) {
        if (testtime) {
            restzeit(&start, timeout, &rest);
            if (negzeit(&rest)) {
                DT(printf("ems_lib(%s): GetSpecialConfirmation timed out\n",
                        client_name);)
                return(0);
            }
            res=readmessage(testpath, &rest);
        } else {
            res=readmessage(testpath, timeout);
        }
        if (res<1)
                return(res);
        if (getspecialmessage(xid, ved, req, header, &body)==0)
                goto ok;
    }

ok:
    if (header->flags & Flag_Intmsg) {
        /*printf("GetConfir: Intmsg\n");*/
    }
    *confirmation=body;
    return(1);
}

/*****************************************************************************/

static int GetExtraConfirmation(msgheader* header, ems_i32** confirmation,
    struct timeval *timeout, int neu)
{
int res;
int testtime;
struct timeval start, rest;
struct timezone tz;

DC(printf("ems_lib(%s): GetExtraConfirmation(...)\n", client_name);)
DT(
  if (timeout)
    printf("ems_lib(%s): GetExtraConfirmation; timeout=(%d, %d)\n",
        client_name, timeout->tv_sec, timeout->tv_usec);
  else
    printf("ems_lib(%s): GetExtraConfirmation; timeout=NULL\n", client_name);
  )

if (!neu && getmessage(header, confirmation)==0) goto ok;

testtime=(timeout!=0) && ((timeout->tv_sec!=0) || (timeout->tv_usec!=0));
if (testtime) gettimeofday(&start, &tz);
while (1)
  {
  if (testtime)
    {
    restzeit(&start, timeout, &rest);
    if (negzeit(&rest))
      {
      DT(printf("ems_lib(%s): GetExtraConfirmation timed out\n", client_name);)
      return(0);
      }
    res=readmessage(testpath, &rest);
    }
  else
    res=readmessage(testpath, timeout);
  if (res<1) return(res);
  if (getmessage(header, confirmation)==0) goto ok;
  }

ok:
DC(printf("ems_lib(%s): return confirmation %d; 0x%08X\n", client_name,
    *confirmation);)
return(1);
}

/*****************************************************************************/

static int
makereq(ems_u32 ved, int bodysize, msgheader *header, ems_u32 **body)
{
    if (bodysize>0) {
        if ((*body=(ems_u32*)calloc(bodysize, 4))==0) {
            EMS_errno=(EMSerrcode)ENOMEM;
            return(-1);
        }
    } else if (body!=0) {
        *body=0;
    }
    header->size=bodysize;
    header->client=client_id;
    header->ved=ved;
    header->transid=nexttransid();
    return(0);
}

/*****************************************************************************/

static int
makerequest(ems_u32 ved, Request request, int bodysize,
                msgheader *header, ems_u32 **body)
{
    if (makereq(ved, bodysize, header, body)==-1)
        return(-1);
    header->type.reqtype=request;
    header->flags=0;
    return 0;
}

/*****************************************************************************/

static int
makeintrequest(ems_u32 ved, IntMsg request, int bodysize,
        msgheader *header, ems_u32 **body)
{
    if (makereq(ved, bodysize, header, body)==-1)
        return(-1);
    header->type.intmsgtype=request;
    header->flags=Flag_Intmsg;
    return 0;
}

/*****************************************************************************/

static ems_i32
standardrequest(ems_u32 ved, Request request)
{
ems_u32 xid;
msgheader header;

if (makerequest(ved, request, 0, &header, 0)==-1) return(-1);
xid=header.transid;
if (sendmessage(&header, 0)==-1) return(-1);
return(xid);
}

/*****************************************************************************/

static ems_i32
standardrequest1(ems_u32 ved, Request request, ems_u32 val)
{
ems_u32 *body, xid;
msgheader header;

if (makerequest(ved, request, 1, &header, &body)==-1) return(-1);
body[0]=val;
xid=header.transid;
if ((sendmessage(&header, body))==-1)
  return(-1);
else
  return(xid);
}

/*****************************************************************************/

static ems_i32
standardrequest2(ems_u32 ved, Request request,
        ems_u32 val1, ems_u32 val2)
{
ems_u32 *body, xid;
msgheader header;

if (makerequest(ved, request, 2, &header, &body)==-1) return(-1);
body[0]=val1;
body[1]=val2;
xid=header.transid;
if ((sendmessage(&header, body))==-1)
  return(-1);
else
  return(xid);
}

/*****************************************************************************/

static ems_i32 __attribute__((unused))
standardrequest3(ems_u32 ved, Request request,
        ems_u32 val1, ems_u32 val2, ems_u32 val3)
{
ems_u32 *body, xid;
msgheader header;

if (makerequest(ved, request, 3, &header, &body)==-1) return(-1);
body[0]=val1;
body[1]=val2;
body[2]=val3;
xid=header.transid;
if ((sendmessage(&header, body))==-1)
  return(-1);
else
  return(xid);
}

/*****************************************************************************/

ems_i32
Rawrequest(ems_u32 ved, Request request, ems_u32 size, ems_u32 *body)
{
ems_u32 xid;
msgheader header;

header.size=size;
header.client=client_id;
header.ved=ved;
header.transid=nexttransid();
header.type.reqtype=request;
header.flags=0;
xid=header.transid;
if ((sendmessage(&header, body))==-1)
  return(-1);
else
  return(xid);
}

/*****************************************************************************/

static int
awaitconf(msgheader *header, ems_u32 *size,
        ems_u32* reqbody, ems_i32 **confbody)
{
    ems_u32 ret_ved;
    ems_i32 xid;
    union Reqtype ret_req;
    int oldtest, res;

    xid=header->transid;
    if ((sendmessage(header, reqbody))==-1)
        return(-1);

    oldtest=test_path(-1);
    res=GetSpecialConfirmation(&ret_ved, &ret_req, &xid, size, confbody,
            deftimeout);
    test_path(oldtest);
    if (res==0) {
        EMS_errno=(EMSerrcode)ETIMEDOUT; 
        res= -1;
    }
    return res;
}

/*****************************************************************************/

static int comm_start(void)
{
    ems_i32 *confirmation;
    ems_u32 *body, *ptr;
    ems_u32 size, ret_ved;
    ems_i32 xid;
    union Reqtype ret_req;
    int res, oldtest;
    struct hostent* host;
    struct passwd* pwptr;
    int ipaddr;
    char hostname[256];
    char user[64];
    int uid, gid;
    msgheader header;

    gethostname(hostname, 255);
    if ((host=gethostbyname(hostname))==0)
        ipaddr=0;
    else
        ipaddr=ntohl(*(int*)(host->h_addr_list[0]));

    uid=getuid();
    gid=getgid();
    pwptr=getpwuid(uid);
    if (pwptr!=0)
        strcpy(user, pwptr->pw_name);
    else {
        char* USER;
        if ((USER=getenv("USER"))==0)
            strcpy(user, "");
        else
            strcpy(user, USER);
    }

    if (strcmp(experiment, "")==0) {
        char* s;
        if ((s=getenv("EXPERIMENT"))==0)
            strcpy(experiment, user);
        else
            strcpy(experiment, s);
    }

    size=1+strxdrlen(hostname)+3+strxdrlen(user)+2+strxdrlen(experiment)+3;

    if (makeintrequest(EMS_commu, Intmsg_ClientIdent, size, &header, &body)==-1)
        {Clientcommlib_abort(EMS_errno); return(-1);}

    xid=header.transid;
    ptr=body;
    *ptr++=0;
    ptr=outstring(ptr, hostname);
    *ptr++=ipaddr;
    *ptr++=getpid();
    *ptr++=getppid();
    ptr=outstring(ptr, user);
    *ptr++=uid;
    *ptr++=gid;
    ptr=outstring(ptr, experiment);
    *ptr++=policies;
    *ptr++=0;
    *ptr++=def_unsol;
    if (ptr-size!=body) printf("fehler in comm_start: size=%d, diff=%ld\n",
        size, (unsigned long)(ptr-body));

    if ((sendmessage(&header, body))==-1) {
        Clientcommlib_abort(EMS_errno);
        return(-1);
    }

    oldtest=test_path(-1);
    res=GetSpecialConfirmation(&ret_ved, &ret_req, &xid, &size,
        &confirmation, deftimeout);
    test_path(oldtest);
    if (res==-1) {
        Clientcommlib_abort(EMS_errno);
        return(-1);
    }
    if (res==0) { /* timeout */
        EMS_errno=(EMSerrcode)ETIMEDOUT;
        Clientcommlib_abort(EMS_errno);
        return(-1);
    }
    EMS_errno=(EMSerrcode)confirmation[0];
    if (EMS_errno==EMSE_OK) client_id=confirmation[1];
    if (ver_intmsgtab!=confirmation[2]) EMS_errno=EMSE_IntVersion;

    free_confirmation(confirmation);

    if (EMS_errno==EMSE_OK)
        return(0);
    else {
        Clientcommlib_abort(EMS_errno);
        return(-1);
    }
}/* comm_start */

/*****************************************************************************/

int Clientcomm_init(const char* sockname)
{
init_environment(&tests);
if (Clientcommlib_init_u(sockname)==-1) return(-1);
return(comm_start());
}

/*****************************************************************************/

int Clientcomm_init_e(const char* hostname, int port)
{
#ifdef DECNET
int i;
#endif

init_environment(&tests);
if ((hostname==0) || (hostname[0]==0))
  {
  EMS_errno=EMSE_HostUnknown;
  return(-1);
  }
#ifdef DECNET
i=strlen(hostname)-1;
if (hostname[i]==':')
  {
  while ((i>=0) && (hostname[i]==':')) {hostname[i]=0; i--;}
  if (Clientcommlib_init_d(hostname, port)==-1) return(-1);
  }
else
#endif
  {if (Clientcommlib_init_i(hostname, port)==-1) return(-1);}
return(comm_start());
}

/*****************************************************************************/

int Clientcomm_done()
{
DC(printf("ems_lib(%s): Clientcomm_done()\n", client_name);)
return(Clientcommlib_done());
}

/*****************************************************************************/

void Clientcomm_abort(int reason)
{
DC(printf("ems_lib(%s): Clientcomm_abort(%d)\n", client_name, reason);)
Clientcommlib_abort(reason);
}

/*****************************************************************************/

int OpenVED(const char* ved_name)
{
    ems_i32* confirmation;
    ems_u32 *request;
    ems_u32 size;
    ems_u32 ret_ved;
    ems_i32 xid, ved;
    union Reqtype ret_req;
    int res, oldtest;
    msgheader header;

    DC(printf("ems_lib(%s): OpenVED(%s)\n", client_name, ved_name);)

    size=strxdrlen(ved_name);
    if (makeintrequest(EMS_commu, Intmsg_OpenVED, size, &header, &request)==-1)
        return(-1);

    xid=header.transid;
    outstring(&request[0], ved_name);
    if ((sendmessage(&header, request))==-1)
        return(-1);

    oldtest=test_path(-1);

    res=GetSpecialConfirmation(&ret_ved, &ret_req, &xid, &size,
            &confirmation, deftimeout);
    test_path(oldtest);

    if (res==0) { /* commu antwortet nicht */
        EMS_errno=EMSE_CommuTimeout;
        return -1;
    }
    if (res<0) /* Fehler in GetSpecialConfirmation */
        return(-1);

    if (size<1) {           /* Programmfehler */
        printf("Fehler in clientcomm.c; 1: size=%d\n", size);
        EMS_errno=EMSE_System;
        return -1;
    }

    if (confirmation[0]==EMSE_OK) {
        DC(printf("ems_lib(%s): open OK\n", client_name);)
        if (size<4) {
            printf("Fehler in clientcomm.c; 2: size=%d\n", size);
            EMS_errno=EMSE_System;
            return -1;
        }
        if (confirmation[1]!=0) printf("Programmfehler in OpenVED\n");
        ved=confirmation[2];
        if (confirmation[3]>=3) {
            /* Versionen sollten getestet werden   */
            /* confirmation[4]=ved_version     (4) */
            /* confirmation[5]=request_version (2) */
            /* confirmation[6]=unsol_version   (2) */
        } else {
            printf("OpenVED in commu unvollstaendig\n");
        }
        free_confirmation(confirmation);
        return ved;
    } else if (confirmation[0]==EMSE_InProgress) { /* warten wir noch ein wenig */
        struct timeval start, rest;
        struct timezone tz;
        int weiter=1;
        int ok=0;
        int neu=0;
        DC(printf("ems_lib(%s): open in progress\n", client_name);)
        if (size<3) {
            printf("Fehler in clientcomm.c; 3: size=%d\n", size);
            EMS_errno=EMSE_System;
            return -1;
        }
        ved=confirmation[2];
        free_confirmation(confirmation);

        gettimeofday(&start, &tz);
        restzeit(&start, deftimeout, &rest);
        oldtest=test_path(-1);
        while (weiter && !negzeit(&rest)) {
            res=GetExtraConfirmation(&header, &confirmation, &rest, neu);
            neu=1;
            if (res==-1) { /* Fehler */
                return -1;
            } else if (res==0) { /* Timeout */
                weiter=0;
            } else {
                if (((header.flags & Flag_Intmsg)!=0) &&
                        ((header.flags & Flag_Unsolicited)!=0) &&
                        (header.type.intmsgtype==Intmsg_OpenVED) &&
                        (confirmation[2]==ved)) {
                    ok=1;
                    weiter=0;
                } else {
                    /*printf("ungetmessage(xid=%d, conf=%p)\n",
                            header.transid, confirmation);*/
                    ungetmessage(&header, confirmation);
                    /*printqueue();*/
                }
            }
            if (weiter) restzeit(&start, deftimeout, &rest);
        }
        test_path(oldtest);
        if (ok) {
            DC(printf("ems_lib(%s): got answer\n", client_name);)
            size=header.size;
            if (size<1) {
                EMS_errno=EMSE_System;
                return -1;
            }
            if (confirmation[0]==EMSE_OK) {
                DC(printf("ems_lib(%s): answer OK.\n", client_name);)
                if (size<4) {
                    unsigned int i;
                    printf("Fehler in clientcomm.c; 4: size=%d; xid=%d\n",
                            size, header.transid);
                    for (i=0; i<size; i++)
                        printf("conf[%d]= %d\n", i, confirmation[i]);
                    EMS_errno=EMSE_System;
                    return -1;
                }
                if (confirmation[1]!=1) printf("Programmfehler in OpenVED\n");
                if (confirmation[3]>=3) {
                    /* Versionen sollten getestet werden   */
                    /* confirmation[4]=ved_version     (4) */
                    /* confirmation[5]=request_version (2) */
                    /* confirmation[6]=unsol_version   (2) */
                } else {
                    printf("OpenVED in commu unvollstaendig\n");
                }
                free_confirmation(confirmation);
                return ved;
            } else {
                DC(printf("ems_lib(%s): answer not OK.\n", client_name);)
                if (size<3) {
                    printf("Fehler in clientcomm.c; 5: size=%d\n", size);
                    EMS_errno=EMSE_System;
                    return -1;
                }
                if (confirmation[1]!=1) printf("Programmfehler in OpenVED\n");
                EMS_errno=(EMSerrcode)confirmation[0];
                free_confirmation(confirmation);
                return -1;
            }
        } else { /* timeout */
            int ires;
            DC(printf("ems_lib(%s): answer timed out\n", client_name);)
            if (makeintrequest(EMS_commu, Intmsg_CloseVED, 1, &header, &request)==-1)
                return -1;
            xid=header.transid;
            request[0]= ved;
            if (sendmessage(&header, request)==-1) {
                DC(printf("ems_lib(%s): can't send message\n", client_name);)
                return -1;
            }

            oldtest=test_path(-1);
            if ((ires=GetSpecialConfirmation(&ret_ved, &ret_req, &xid, &size,
                    &confirmation, deftimeout))>0) {
                DC(printf("ems_lib(%s): got conf.\n", client_name);)
                free_confirmation(confirmation);
            }
            test_path(oldtest);
            EMS_errno=(EMSerrcode)ETIMEDOUT;
            return -1;
        }
    } else {
        DC(printf("ems_lib(%s): got error.\n", client_name);)
        EMS_errno=(EMSerrcode)confirmation[0];
        free_confirmation(confirmation);
        return -1;
    }
}/* OpenVED */

/*****************************************************************************/

int CloseVED(ems_u32 ved)
{
ems_i32* confirmation;
ems_u32 *request;
ems_u32 size, ret_ved;
ems_i32 xid;
union Reqtype ret_req;
int res, oldtest;
msgheader header;

if (makeintrequest(EMS_commu, Intmsg_CloseVED, 1, &header, &request)==-1)
    return(-1);
xid=header.transid;
request[0]=ved;
if ((sendmessage(&header, request))==-1) return(-1);
oldtest=test_path(-1);
res=GetSpecialConfirmation(&ret_ved, &ret_req, &xid, &size,
    &confirmation, deftimeout);
test_path(oldtest);
if (res==-1) return(-1);

if (res==0)
  {
  EMS_errno=(EMSerrcode)ETIMEDOUT;
  return(-1);
  }
if (confirmation[0]==EMSE_OK)
  res=0;
else
  {
  EMS_errno=(EMSerrcode)confirmation[0];
  res= -1;
  }
free_confirmation(confirmation);
return(res);
}

/*****************************************************************************/

int GetHandle(const char* ved_name)
{
    ems_i32* confirmation;
    ems_u32 *request;
    ems_u32 ved, size, ret_ved;
    ems_i32 xid;
    union Reqtype ret_req;
    int res, oldtest;
    msgheader header;

    size=strxdrlen(ved_name);
    if (makeintrequest(EMS_commu, Intmsg_GetHandle, size,&header,&request)==-1)
        return(-1);

    xid=header.transid;
    outstring(request, ved_name);
    if ((sendmessage(&header, request))==-1)
        return -1;
    oldtest=test_path(-1);
    res=GetSpecialConfirmation(&ret_ved, &ret_req, &xid, &size,
        &confirmation, deftimeout);
    test_path(oldtest);
    if (res==0) EMS_errno=(EMSerrcode)ETIMEDOUT;
    if (res<1)
        return -1;

    if (confirmation[0]==EMSE_OK) {
        ved=confirmation[1];
    } else {
        EMS_errno=(EMSerrcode)confirmation[0];
        ved=(ems_u32)-1;
    }
    free_confirmation(confirmation);
    return ved;
}

/*****************************************************************************/

int GetName(ems_u32 ved, char* name)
{
    ems_i32* confbody;
    ems_u32 size, *reqbody;
    msgheader header;

    if (makeintrequest(EMS_commu, Intmsg_GetName, 1, &header, &reqbody)==-1)
        return(-1);
    reqbody[0]=ved;
    /* reqbody is freed from sendmessage in awaitconf */
    if (awaitconf(&header, &size, reqbody, &confbody)==-1)
        return(-1);
    if (size<1) {
        EMS_errno=EMSE_Proto;
        return(-1);
    }
    if (confbody[0]!=EMSE_OK) {
        EMS_errno=(EMSerrcode)confbody[0];
        free_confirmation(confbody);
        return(-1);
    }
    extractstring(name, (ems_u32*)(confbody+1));
    free_confirmation(confbody);
    return(0);
}

/*****************************************************************************/

int Nichts(ems_u32 ved)
{
    return(standardrequest(ved, Req_Nothing));
}

/*****************************************************************************/

int Initiate(ems_u32 ved, ems_u32 ved_id)
{
return(standardrequest1(ved, Req_Initiate, ved_id));
}

/*****************************************************************************/

int Identify(ems_u32 ved, int mod)
{
return(standardrequest1(ved, Req_Identify, mod));
}

/*****************************************************************************/

int Conclude(ems_u32 ved)
{
return(standardrequest(ved, Req_Conclude));
}

/*****************************************************************************/

int ResetVED(ems_u32 ved)
{
return(standardrequest(ved, Req_ResetVED));
}

/*****************************************************************************/

int GetVEDStatus(ems_u32 ved)
{
return(standardrequest(ved, Req_GetVEDStatus));
}

/*****************************************************************************/
ems_i32
GetNameList(ems_u32 ved, int count, int *req)
{
    ems_u32 *body, xid;
    msgheader header;
    int i;

    if (makerequest(ved, Req_GetNameList, count, &header, &body)==-1)
        return(-1);

    for (i=0; i<count; i++) body[i]=req[i];
    xid=header.transid;
    if ((sendmessage(&header, body))==-1)
        return(-1);
    else
        return(xid);
}
/*****************************************************************************/
int GetCapabilityList(ems_u32 ved, Capabtyp type)
{
    return(standardrequest1(ved, Req_GetCapabilityList, type));
}
/*****************************************************************************/
ems_i32
GetProcProperties(ems_u32 ved, ems_u32 level, ems_u32 num,
        const ems_u32* list)
{
    ems_u32 *body, xid;
    msgheader header;
    unsigned int i;

    if (makerequest(ved, Req_GetProcProperties, num+2, &header, &body)==-1)
        return(-1);
    body[0]=level;
    body[1]=num;
    for (i=0; i<num; i++)
        body[i+2]=list[i];
    xid=header.transid;
    if ((sendmessage(&header, body))==-1)
        return(-1);
    else
        return(xid);
}
/*****************************************************************************/

ems_i32
DownloadDomain(ems_u32 ved, Domain type, ems_u32 domain_ID, int size,
    const ems_u32* domain)
{
ems_u32 *body, xid;
msgheader header;
int i;

if (makerequest(ved, Req_DownloadDomain, size+2, &header, &body)==-1)
    return(-1);
body[0]=type;
body[1]=domain_ID;
for (i=0; i<size; i++) body[i+2]=domain[i];
xid=header.transid;
if ((sendmessage(&header, body))==-1)
  return(-1);
else
  return(xid);
}

/*****************************************************************************/

int UploadDomain(ems_u32 ved, Domain type, ems_u32 domain_ID)
{
return(standardrequest2(ved, Req_UploadDomain, type, domain_ID));
}

/*****************************************************************************/

int DeleteDomain(ems_u32 ved, Domain type, ems_u32 domain_ID)
{
return(standardrequest2(ved, Req_DeleteDomain, type, domain_ID));
}

/*****************************************************************************/

ems_i32
CreateProgramInvocation(ems_u32 ved, Invocation type, ems_u32 invocation_ID,
    int numparams, const ems_u32 *params)
{
ems_u32 *body, xid;
msgheader header;
int i;

if (makerequest(ved, Req_CreateProgramInvocation, numparams+2, &header,
    &body) ==-1) return(-1);
body[0]=type;
body[1]=invocation_ID;
for (i=0; i<numparams; i++) body[i+2]=params[i];
xid=header.transid;
if ((sendmessage(&header, body))==-1)
  return(-1);
else
  return(xid);
}

/*****************************************************************************/

int DeleteProgramInvocation(ems_u32 ved, Invocation type, ems_u32 invocation_ID)
{
return(standardrequest2(ved, Req_DeleteProgramInvocation, type, invocation_ID));
}

/*****************************************************************************/

ems_i32
StartProgramInvocation(ems_u32 ved, Invocation type, ems_u32 invocation_ID)
{
ems_u32 size, xid;
ems_u32 *body;
msgheader header;

size=2+strxdrlen(experiment);
if (makerequest(ved, Req_StartProgramInvocation, size, &header, &body) ==-1)
  return(-1);
body[0]=type;
body[1]=invocation_ID;
outstring(body+2, experiment);
xid=header.transid;
if ((sendmessage(&header, body))==-1)
  return(-1);
else
  return(xid);
}

/*****************************************************************************/

ems_i32
ResetProgramInvocation(ems_u32 ved, Invocation type, ems_u32 invocation_ID)
{
ems_u32 size, xid;
ems_u32 *body;
msgheader header;

size=2+strxdrlen(experiment);
if (makerequest(ved, Req_ResetProgramInvocation, size, &header, &body) ==-1)
    return(-1);
body[0]=type;
body[1]=invocation_ID;
outstring(body+2, experiment);
xid=header.transid;
if ((sendmessage(&header, body))==-1)
  return(-1);
else
  return(xid);
}

/*****************************************************************************/

ems_i32
ResumeProgramInvocation(ems_u32 ved, Invocation type, ems_u32 invocation_ID)
{
ems_u32 size, xid;
ems_u32 *body;
msgheader header;

size=2+strxdrlen(experiment);
if (makerequest(ved, Req_ResumeProgramInvocation, size, &header, &body) ==-1)
    return(-1);
body[0]=type;
body[1]=invocation_ID;
outstring(body+2, experiment);
xid=header.transid;
if ((sendmessage(&header, body))==-1)
  return(-1);
else
  return(xid);
}

/*****************************************************************************/

ems_i32
StopProgramInvocation(ems_u32 ved, Invocation type, ems_u32 invocation_ID)
{
ems_u32 size, xid;
ems_u32 *body;
msgheader header;

size=2+strxdrlen(experiment);
if (makerequest(ved, Req_StopProgramInvocation, size, &header, &body) ==-1)
    return(-1);
body[0]=type;
body[1]=invocation_ID;
outstring(body+2, experiment);
xid=header.transid;
if ((sendmessage(&header, body))==-1)
  return(-1);
else
  return(xid);
}

/*****************************************************************************/

int GetProgramInvocationAttributes(ems_u32 ved, Invocation type, ems_u32 invocation_ID)
{
return(standardrequest2(ved, Req_GetProgramInvocationAttr, type,
    invocation_ID));
}

/*****************************************************************************/
int GetProgramInvocationParams(ems_u32 ved, Invocation type, ems_u32 invocation_ID)
{
return(standardrequest2(ved, Req_GetProgramInvocationParams, type,
    invocation_ID));
 }
/*****************************************************************************/
int CreateVariable(ems_u32 ved, ems_u32 variable_ID, int size)
{
    return(standardrequest2(ved, Req_CreateVariable, variable_ID, size));
}
/*****************************************************************************/
int DeleteVariable(ems_u32 ved, ems_u32 variable_ID)
{
    return(standardrequest1(ved, Req_DeleteVariable, variable_ID));
}
/*****************************************************************************/
int ReadVariable(ems_u32 ved, ems_u32 variable_ID)
{
    return(standardrequest1(ved, Req_ReadVariable, variable_ID));
}
/*****************************************************************************/
ems_i32
WriteVariable(ems_u32 ved, ems_u32 variable_ID, ems_u32 count,
        const ems_u32* val)
{
    ems_u32 *body, xid;
    msgheader header;
    unsigned int i;

    if (makerequest(ved, Req_WriteVariable, count+2, &header, &body)==-1)
        return -1;
    body[0]=variable_ID;
    body[1]=count;
    for (i=0; i<count; i++)
        body[i+2]=val[i];
    xid=header.transid;
    if ((sendmessage(&header, body))==-1)
        return(-1);
    else
        return(xid);
}
/*****************************************************************************/
int GetVariableAttributes(ems_u32 ved, ems_u32 variable_ID)
{
    return(standardrequest1(ved, Req_GetVariableAttributes, variable_ID));
}
/*****************************************************************************/
int CreateIS(ems_u32 ved, ems_u32 IS_ID, int ident)
{
    return(standardrequest2(ved, Req_CreateIS, IS_ID, ident));
}
/*****************************************************************************/
int DeleteIS(ems_u32 ved, ems_u32 IS_ID)
{
    return(standardrequest1(ved, Req_DeleteIS, IS_ID));
}
/*****************************************************************************/
ems_i32
DownloadISModulList(ems_u32 ved, ems_u32 IS_ID, ems_u32 nummod,
            const ems_u32* list)
{
    ems_u32 *body, xid;
    msgheader header;
    unsigned int i;

    if (makerequest(ved, Req_DownloadISModulList, nummod+2, &header, &body)==-1)
        return -1;
    body[0]=IS_ID;
    body[1]=nummod;
    for (i=0; i<nummod; i++)
        body[i+2]=list[i];
    xid=header.transid;
    if ((sendmessage(&header, body))==-1)
        return -1;
    else
        return xid;
}
/*****************************************************************************/
int UploadISModulList(ems_u32 ved, ems_u32 IS_ID)
{
    return(standardrequest1(ved, Req_UploadISModulList, IS_ID));
}
/*****************************************************************************/
int DeleteISModulList(ems_u32 ved, ems_u32 IS_ID)
{
    return(standardrequest1(ved, Req_DeleteISModulList, IS_ID));
}
/*****************************************************************************/
ems_i32
DownloadReadoutList(ems_u32 ved, ems_u32 IS_ID, ems_u32 priority,
        ems_u32 numtrig, const ems_u32* trigger, int listsize,
        const ems_u32* list)
{
    ems_u32 *body, *ptr, xid;
    msgheader header;
    unsigned int ui;
    int i;

    if (makerequest(ved, Req_DownloadReadoutList, numtrig+listsize+3,
            &header, &body)==-1)
        return(-1);
    ptr=body;
    *ptr++=IS_ID;
    *ptr++=numtrig;
    for (ui=0; ui<numtrig; ui++)
        *ptr++=trigger[ui];
    *ptr++=priority;
    for (i=0; i<listsize; i++)
        *ptr++=list[i];
    xid=header.transid;
    if ((sendmessage(&header, body))==-1)
        return -1;
    else
        return xid;
}
/*****************************************************************************/
int UploadReadoutList(ems_u32 ved, ems_u32 IS_ID, int trigger)
{
    return(standardrequest2(ved, Req_UploadReadoutList, IS_ID, trigger));
}
/*****************************************************************************/
int DeleteReadoutList(ems_u32 ved, ems_u32 IS_ID, int trigger)
{
    return(standardrequest2(ved, Req_DeleteReadoutList, IS_ID, trigger));
}
/*****************************************************************************/

int EnableIS(ems_u32 ved, ems_u32 IS_ID)
{
return(standardrequest1(ved, Req_EnableIS, IS_ID));
}

/*****************************************************************************/

int DisableIS(ems_u32 ved, ems_u32 IS_ID)
{
return(standardrequest1(ved, Req_DisableIS, IS_ID));
}

/*****************************************************************************/

int GetISStatus(ems_u32 ved, ems_u32 IS_ID)
{
return(standardrequest1(ved, Req_GetISStatus, IS_ID));
}

/*****************************************************************************/

int ResetISStatus(ems_u32 ved, ems_u32 IS_ID)
{
return(standardrequest1(ved, Req_GetISStatus, IS_ID));
}

/*****************************************************************************/

ems_i32
DoCommand(ems_u32 ved, ems_u32 IS_ID, int listsize, const ems_u32* list)
{
ems_u32 *body, xid;
msgheader header;
int i;

if (makerequest(ved, Req_DoCommand, listsize+1, &header, &body)==-1)
      return(-1);
body[0]=IS_ID;
for (i=0; i<listsize; i++) body[i+1]=list[i];
xid=header.transid;
if ((sendmessage(&header, body))==-1)
  return(-1);
else
  return(xid);
}

/*****************************************************************************/

ems_i32
TestCommand(ems_u32 ved, ems_u32 IS_ID, int listsize, const ems_u32* list)
{
ems_u32 *body, xid;
msgheader header;
int i;

if (makerequest(ved, Req_TestCommand, listsize+1, &header, &body)==-1)
      return(-1);
body[0]=IS_ID;
for (i=0; i<listsize; i++) body[i+1]=list[i];
xid=header.transid;
if ((sendmessage(&header, body))==-1)
  return(-1);
else
  return(xid);
}

/*****************************************************************************/

int WindDataout(ems_u32 ved, ems_u32 DO_ID, ems_i32 offset)
{
return(standardrequest2(ved, Req_WindDataout, DO_ID, (ems_u32)offset));
}

/*****************************************************************************/

ems_i32
WriteDataout(ems_u32 ved, ems_u32 DO_ID, ems_u32 makeheader, ems_u32 size,
    const ems_u32* record)
{
ems_u32 *body, xid;
msgheader header;
unsigned int i;

if (makerequest(ved, Req_WriteDataout, size+3, &header, &body)==-1) return(-1);
body[0]=DO_ID;
body[1]=makeheader;
body[2]=size;
for (i=0; i<size; i++) body[i+3]=record[i];
xid=header.transid;
if ((sendmessage(&header, body))==-1)
  return(-1);
else
  return(xid);
}

/*****************************************************************************/

int GetDataoutStatus(ems_u32 ved, ems_u32 DO_ID)
{
    return(standardrequest1(ved, Req_GetDataoutStatus, DO_ID));
}

/*****************************************************************************/
int GetDataoutStatus_1(ems_u32 ved, ems_u32 DO_ID, int arg)
{
    return(standardrequest2(ved, Req_GetDataoutStatus, DO_ID, arg));
}
/*****************************************************************************/

int EnableDataout(ems_u32 ved, ems_u32 DO_ID)
{
    return(standardrequest1(ved, Req_EnableDataout, DO_ID));
}

/*****************************************************************************/

int DisableDataout(ems_u32 ved, ems_u32 DO_ID)
{
    return(standardrequest1(ved, Req_DisableDataout, DO_ID));
}

/*****************************************************************************/

int ConfirmationAvailable()
{
    return(messageavailable());
}

/*****************************************************************************/

int SpecialConfirmationAvailable(ems_u32 ved, int requesttype,
    int transaction)
{
    /*printf("SpecialConfirmationAvailable not yet implemented\n");*/
    EMS_errno=EMSE_NotImpl;
    return(-1);
}

/*****************************************************************************/

int SetExitCommu(ems_u32 val)
{
    ems_i32* confirmation;
    ems_u32 *body;
    msgheader header;
    ems_u32 size;
    ems_i32 xid;
    int res, oldtest;

    if (makeintrequest(EMS_commu, Intmsg_SetExitCommu, 1, &header, &body)==-1)
      return(-1);

    xid=header.transid;
    body[0]=val;
    if ((sendmessage(&header, body))==-1)
        return(-1);
    oldtest=test_path(-1);
    res=GetSpecialConfirmation(0, 0, &xid, &size, &confirmation, deftimeout);
    test_path(oldtest);
    if (res==-1) return(-1);

    if (confirmation[0]==EMSE_OK) {
        res=confirmation[1];
    } else {
        EMS_errno=(EMSerrcode)confirmation[0];
        res= -1;
    }
    free_confirmation(confirmation);
    return(res);
}

/*****************************************************************************/

int SetUnsolicited(ems_u32 ved, ems_u32 val)
{
ems_i32* confirmation;
ems_u32 *body;
msgheader header;
ems_u32 size;
ems_i32 xid;
int res, oldtest;

if (makeintrequest(EMS_commu, Intmsg_SetUnsolicited, 2, &header, &body)==-1)
  return(-1);

xid=header.transid;
body[0]=ved;
body[1]=val;
if ((sendmessage(&header, body))==-1) return(-1);
oldtest=test_path(-1);
res=GetSpecialConfirmation(0, 0, &xid, &size, &confirmation, deftimeout);
test_path(oldtest);
if (res<0) return -1;
if (res==0)
  {
  /* return(-1); */
  EMS_errno=(EMSerrcode)ETIMEDOUT;
  return -1;
  }
if (confirmation[0]==EMSE_OK)
  res=confirmation[1];
else
  {
  EMS_errno=(EMSerrcode)confirmation[0];
  res= -1;
  }
free_confirmation(confirmation);
return(res);
}

/*****************************************************************************/
/*****************************************************************************/
