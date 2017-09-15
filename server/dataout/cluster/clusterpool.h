/*
 * dataout/cluster/clusterpool.h
 * static char *rcsid="$ZEL: clusterpool.h,v 1.2 2006/02/12 23:10:41 wuestner Exp $";
 */

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>

enum clpooltype {clpool_none, clpool_malloc, clpool_lowbuf, clpool_shm};

errcode clusterpool_alloc(enum clpooltype pooltype, size_t clsize, int clnum);
errcode clusterpool_release(void);
void    clusterpool_clear(void);
int     clusterpool_valid(void* cluster);
void*   alloc_cluster(size_t size, const char* text);
void    free_cluster(void* cluster);
enum clpooltype clusterpool_type(void);
const char*     clusterpool_name(void);
off_t           clusterpool_offset(void* cluster);
void            clusterpool_dump(void);
