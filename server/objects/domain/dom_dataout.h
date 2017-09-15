/*
 * objects/domain/dom_dataout.h
 * $ZEL: dom_dataout.h,v 1.8 2011/08/16 19:19:50 wuestner Exp $
 * 
 * created (before?) 12.05.93
 */

#ifndef _dom_dataout_h_
#define _dom_dataout_h_

#include <sconf.h>
#include <netdb.h>
#include <objecttypes.h>
#include <errorcodes.h>

struct dataout_info {
    InOutTyp bufftyp;
    int buffsize;
    int wieviel;
    IOAddr addrtyp;
    union {
        int* addr;
        char *modulname;
        char *filename;
        struct {
            char *path;
            int offset;
        } driver;
        struct {
            int host;
            int port;
        } inetsock;
        struct{
          struct addrinfo *addr;
          char *node;
          char *service;
          int flags;
        } inetV6sock;
    } addr;
    int inout_arg_num;
    int* inout_args;
    int address_arg_num;
    int* address_args;
    char* logfilename;
    char* loginfo;
    char* runlinkdir;
    int runlinkabsolute;
    int fadvise_flag;
    int fadvise_data;
};

#ifdef DATAOUT_MULTI
extern struct dataout_info dataout[];
#endif

errcode dom_dataout_init(void);
errcode dom_dataout_done(void);

#endif

/*****************************************************************************/
/*****************************************************************************/
