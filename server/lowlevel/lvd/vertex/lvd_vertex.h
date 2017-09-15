/*
 * lowlevel/lvd/vertex/lvd_vertex.h
 * created 2005-Feb-25 PW
 * $ZEL: lvd_vertex.h,v 1.1 2005/05/27 19:09:12 wuestner Exp $
 */

#ifndef _lvd_vertex_h_
#define _lvd_vertex_h_

#include <sconf.h>
#include <errorcodes.h>

int lvd_vertex_low_printuse(FILE* outfilepath);
errcode lvd_vertex_low_init(char* arg);
errcode lvd_vertex_low_done(void);

#endif
