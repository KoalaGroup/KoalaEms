/*
 * lowlevel/lowlevel_init.c.m4
 * created before 09.11.93
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lowlevel_init.c.m4,v 1.8 2011/04/06 20:30:22 wuestner Exp $";

#include <stdio.h>
#include <errorcodes.h>
#include "lowlevel_init.h"
#include "devices.h"
#include "rcs_ids.h"

define(`lowcase_', `translit(`$*', `ABCDEFGHIJKLMNOPQRSTUVWXYZ/',
                                 `abcdefghijklmnopqrstuvwxyz_')')

define(`lowlib', ``#include' "$1/lowcase_($1).h"')
include(lowlibs)

RCS_REGISTER(cvsid, "lowlevel")

/*****************************************************************************/

typedef errcode(*initfunc)(char*);
typedef errcode(*donefunc)(void);

define(`lowlib', `  lowcase_($1)_low_init,')
initfunc initfuncs[]={
include(lowlibs)
    0,
};

define(`lowlib', `  lowcase_($1)_low_done,')
donefunc donefuncs[]={
include(lowlibs)
    0,
};

/*****************************************************************************/

errcode lowlevel_init(char* param)
{
        errcode err;
        int i;

        devices_init();

#ifdef LOWLEVEL_RAWMEM
        /* rawmem has to be initialised first */
        if ((err=rawmem_low_init(param)))
            return err;
#endif
        for (i=0; initfuncs[i]; i++) {
#ifdef LOWLEVEL_RAWMEM
                if (initfuncs[i]==rawmem_low_init)
                    continue;
#endif
                if ((err=initfuncs[i](param)))
                    return err;
        }

        return OK;
}

/*****************************************************************************/

errcode lowlevel_done(void)
{
        errcode err, res=OK;
        int i;

        devices_done();

        for (i=0; donefuncs[i]; i++) {
#ifdef LOWLEVEL_RAWMEM
                if (donefuncs[i]==rawmem_low_done)
                    continue;
#endif
                if ((err=donefuncs[i]()))
                    return err;
        }
#ifdef LOWLEVEL_RAWMEM
        /* rawmem has to be destroyed last */
        if ((err=rawmem_low_done()))
            return err;
#endif

        return res;
}

/*****************************************************************************/

int printuse_lowlevel(FILE* outfilepath)
{
int res=0;
define(`lowlib', `res+=lowcase_($1)_low_printuse(outfilepath);')
include(lowlibs)
return res;
}

/*****************************************************************************/
/*****************************************************************************/

