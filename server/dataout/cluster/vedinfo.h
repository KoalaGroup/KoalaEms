/*
 * dataout/cluster/vedinfo.h
 * created 13.Mar.2001 PW
 */
/*
 * static char *rcsid="$Id: vedinfo.h,v 1.4 2007/02/13 19:51:25 wuestner Exp $";
 */

#ifndef _vedinfo_h_
#define _vedinfo_h_

struct ISinfo {
    int is_id;
    int importance;
    int decoding_hints;
};

struct VEDinfo {
    int ved_id;
    int orig_id;
#if 0
    int importance; /* 0: default; 1...: lower *//* has been moved to ISinfo*/
#endif
    int num_is;
    struct ISinfo *is_info;
    int events; /* used in event builder only */
};

struct VEDinfos {
    int version;
    int num_veds;
    struct VEDinfo* info;
};

/* defined in dataout/cluster/cl_interface.c */
extern struct VEDinfos vedinfos;

#endif
