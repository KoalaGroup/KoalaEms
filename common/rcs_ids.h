/*
 * rcs_ids.h
 * created 2011-03-31 PW
 * 
 * $ZEL: rcs_ids.h,v 2.2 2011/04/07 14:07:08 wuestner Exp $
 */

#ifndef _rcs_ids_h_
#define _rcs_ids_h_

#include <stdio.h>

struct rcs_id {
    const char *id;
    const char *path;
};

void rcs_store(const char*, const char*);
void rcs_dump(FILE*);
int rcs_get(const struct rcs_id**);


#define RCS_REGISTER(id, path)                                     \
static void rcs_register(void) __attribute__((constructor(2000))); \
static void rcs_register(void)                                     \
{                                                                  \
    rcs_store(id, path);                                           \
}

#endif
