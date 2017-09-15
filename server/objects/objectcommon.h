/*
 * objects/objectcommon.h
 * $ZEL: objectcommon.h,v 1.8 2006/02/23 18:37:51 wuestner Exp $
 */

#ifndef _objectcommon_h_
#define _objectcommon_h_

#include <sys/types.h>
#include <errorcodes.h>
#include <emsctypes.h>

#if 0
#define OP_READ 1
#define OP_WRITE 2
#define OP_CHSTATUS 4
#define OP_DEL 8
#define OP_CHOWN 16
#endif

typedef struct objectcommon {
    errcode (*init)(void);
    errcode (*done)(void);
    struct objectcommon* (*lookup)(ems_u32*, unsigned int, unsigned int*);
    ems_u32* (*dir)(ems_u32* ptr);
    int (*refcnt)(void);
#if 0
    int (*setprot)(ems_u32*, unsigned int, char*, int*);
    int (*getprot)(ems_u32*, unsigned int, char**, int**);
#endif
} objectcommon;

typedef struct objectcommon* (*lookupfunc)(void);

extern errcode notimpl(ems_u32*, unsigned int);
objectcommon *lookup_object(ems_u32*, unsigned int, unsigned int*);

#if 0
int checkperm(objectcommon*, ems_u32*, unsigned int, int);
void setid(char*);
#endif

#endif
