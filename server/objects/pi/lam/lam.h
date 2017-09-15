/*
 * objects/pi/lam/lam.h
 * created before 26.08.93
 * $ZEL: lam.h,v 1.8 2007/10/15 15:28:17 wuestner Exp $
 */

#ifndef _lam_h_
#define _lam_h_

#include <errorcodes.h>
#include <emsctypes.h>
#include "../../../trigger/trigger.h"

/*****************************************************************************/

struct trigprocinfo; /* defined in server/trigger/trigprocs.h */
struct LAM {
    struct LAM *next;
    struct LAM *prev;
    int idx;
    int id;
    int is;
    int output;
    //int active;
    int cnt;
    int trigproc;
    ems_u32* trigargs;
    int proclistlen;
    ems_u32 *proclist;
    struct triggerinfo trinfo;
};

extern struct LAM *lam_list;

struct LAM* get_lam(int idx);
errcode pi_lam_init(void);
errcode pi_lam_done(void);
errcode createlam(ems_u32* p, unsigned int num);
errcode deletelam(ems_u32* p, unsigned int num);
errcode startlam(ems_u32* p, unsigned int num);
errcode resetlam(ems_u32* p, unsigned int num);
errcode stoplam(ems_u32*, unsigned int);
errcode resumelam(ems_u32*, unsigned int);
errcode stoplams(void);
errcode getlamattr(ems_u32* p, unsigned int num);
errcode getlamparams(ems_u32* p, unsigned int num);

#endif

/*****************************************************************************/
/*****************************************************************************/
