/*
 * objects/ved/ved.h
 * 
 * $ZEL: ved.h,v 1.7 2017/10/20 23:21:31 wuestner Exp $
 */

#ifndef _ved_h_
#define _ved_h_

#include <errorcodes.h>

#include <emsctypes.h>

/*int get_ved_id(void);*/
errcode ved_init(void);
errcode ved_done(void);
errcode Initiate(ems_u32*, int);
errcode Conclude(ems_u32*, int);
errcode GetVEDStatus(ems_u32*, int);
errcode Identify(ems_u32*, int);
errcode GetNameList(ems_u32*, unsigned int);
errcode GetCapabilityList(ems_u32*, unsigned int);
errcode GetProcProperties(ems_u32*, unsigned int);
/*
 * objectcommon *lookup_ved(int* id, int idlen, int* remlen)
 * int *dir_ved(int* ptr)
 */

#ifdef USE_SQL
struct sqldb {
    int valid;
    int do_id;
    char* host;
    char* user;
    char* passwd;
    char* db;
    int port;
};
#endif

struct ved_globals {
    int ved_id;
    int runnr;
#ifdef USE_SQL
    struct sqldb sqldb;
#endif
};

extern struct ved_globals ved_globals;

#endif
