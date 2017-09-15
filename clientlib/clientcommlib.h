/*
 * clientcommlib.h
 * created before: 15.08.94
 * 12.11.2000 PW: init_environment changed
 */

#ifndef _clientcommlib_h_
#define _clientcommlib_h_

#include "config.h"
#include <sys/types.h>
#include <sys/time.h>
#include <cdefs.h>
#include <msg.h>
#include <ems_err.h>
#include "clientcomm.h"

/*****************************************************************************/

#include "ccommcallback.h"

#ifdef DEBUG
#define DCT(y) {if (tests.listcontrol)  {y}}
#define DR(y)  {if (tests.listrequests) {y}}
#define DC(y)  {if (tests.listconfs)    {y}}
#define DT(y)  {if (tests.listtimes)    {y}}
#define DTR(y) {if (tests.listtrace)    {y}}
#else
#define DCT(y)
#define DR(y)
#define DC(y)
#define DT(y)
#define DTR(y)
#endif
typedef struct
  {
  int listcontrol;  /* EMS_LIB_CONTR */
  int listrequests; /* EMS_LIB_REQS  */
  int listconfs;    /* EMS_LIB_CONFS */
  int listtimes;    /* EMS_LIB_TIMES */
  int listtrace;    /* EMS_LIB_TRACE */
  int dumpcore;     /* EMS_LIB_CORE  */
  } testenv;

int nexttransid(void);
int sendmessage(msgheader *header, ems_u32 *body);
void send_info(const char *info); /**/
int readmessage(int testpath, const struct timeval *timeout);
int getmessage(msgheader* header, ems_i32** body);
int getspecialmessage(ems_i32* xid, ems_u32* ved, union Reqtype* req,
    msgheader* header, ems_i32 **body);
int ungetmessage(msgheader* header, ems_i32 *body);
int Clientcommlib_init_u(const char* sockname);
int Clientcommlib_init_i(const char* hostname, int port);
int messageavailable();
int Clientcommlib_done(void);
int Clientcommlib_done(void);
void Clientcommlib_abort(int);
void init_environment(testenv*);
int messageavailable(void);
void printqueue(void);

#endif /* _clientcommlib_h_ */

/*****************************************************************************/
/*****************************************************************************/
