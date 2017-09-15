/*
 * objects/is/is.h
 * created before 03.08.94
 */
/*
 * $ZEL: is.h,v 1.14 2009/06/05 15:06:01 wuestner Exp $
 */

#ifndef _is_h_
#define _is_h_
#include <errorcodes.h>
#include <sconf.h>
#include "../objectcommon.h"
#include "is_hints.h"

#if defined (ISVARS)
typedef struct {
    int* var;
    int size;
} ISV;
#define is_var isvar->var
#endif /* ISVARS */

struct readoutlist {
    ems_u32 *list;
    unsigned int length;
    int priority;
    int refcount;
};

struct IS {
    int id;
    int enabled;
    int *members;

#ifdef ISVARS
    ISV isvar;
#endif /* ISVARS */

#ifdef READOUT_CC
    //ems_u32 *readoutlist[MAX_TRIGGER];
    //int prioritaet[MAX_TRIGGER];
    //unsigned int listenlaenge[MAX_TRIGGER];
    struct readoutlist *readoutlist[MAX_TRIGGER];
#endif /* READOUT_CC */
    int importance;
    enum decoding_hints decoding_hints;
};

extern struct IS* is_list[];
errcode is_init(void);
errcode is_done(void);
errcode is_getnamelist(ems_u32* p, unsigned int num);
errcode CreateIS(ems_u32* p, unsigned int num);
errcode DeleteIS(ems_u32* p, unsigned int num);
errcode DownloadISModulList(ems_u32* p, unsigned int num);
errcode UploadISModulList(ems_u32* p, unsigned int num);
errcode DeleteISModulList(ems_u32* p, unsigned int num);
#ifdef READOUT_CC
errcode DownloadReadoutList(ems_u32* p, unsigned int num);
errcode UploadReadoutList(ems_u32* p, unsigned int num);
errcode DeleteReadoutList(ems_u32* p, unsigned int num);
errcode EnableIS(ems_u32* p, unsigned int num);
errcode DisableIS(ems_u32* p, unsigned int num);
errcode GetISStatus(ems_u32* p, unsigned int num);
#endif
errcode GetISParams(ems_u32* p, unsigned int num);
#ifdef ISVARS
plerrcode allocisvar(int size);
plerrcode reallocisvar(int size);
plerrcode freeisvar(void);
int isvarsize(void);
#endif
objectcommon *lookup_is(ems_u32* id, unsigned int idlen, unsigned int* remlen);
/*
int *dir_is(int* ptr);
*/
#endif

/*****************************************************************************/
/*****************************************************************************/
