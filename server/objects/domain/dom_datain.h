/*
 * objects/domain/dom_datain.h
 * 
 * $ZEL: dom_datain.h,v 1.8 2017/10/27 21:06:48 wuestner Exp $
 * 
 * created 1993-04-27
 */

#ifndef _dom_datain_h_
#define _dom_datain_h_

#include <netdb.h>
#include <objecttypes.h>
#include <emsctypes.h>

typedef struct
{
  InOutTyp bufftyp;
  IOAddr addrtyp;
  union{
    int* addr;
    char *modulname;
    char *localsockname;
    struct{
      char *path;
      int space;
      int offset;
      int option;
    } driver;
    struct{
      u_int32_t host;
      int port;
    } inetsock;
    struct{
      struct addrinfo *addr;
      char *node;
      char *service;
      int flags;
    } inetV6sock;
  } addr;
  size_t bufinfosize;
  ems_u32* bufinfo;
  size_t addrinfosize;
  ems_u32* addrinfo;
} datain_info;

extern datain_info datain[];
errcode dom_datain_init(void);
errcode dom_datain_done(void);
errcode uploaddatain(ems_u32* p, unsigned int len);
errcode deletedatain(ems_u32* p, unsigned int len);

#endif

/*****************************************************************************/
/*****************************************************************************/

