/*
 * common/xprintf.h
 * created 2013-01-17 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: xprintf.c,v 2.1 2013/01/17 22:38:02 wuestner Exp $";

#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "xprintf.h"

#include "rcs_ids.h"

RCS_REGISTER(cvsid, "common")

/*****************************************************************************/
struct xprintf {
    char *p;
    int size;
    int pos;
};
/*****************************************************************************/
void
xprintf_init(void **xpp)
{
    struct xprintf *xp=0;

    xp=calloc(1, sizeof(struct xprintf));
    if (!xp) {
        printf("xprintf_init: alloc %llu bytes: %s\n",
            (unsigned long long)sizeof(struct xprintf), strerror(errno));
    }

    *xpp=xp;
}
/*****************************************************************************/
void
xprintf_done(void **xpp)
{
    struct xprintf *xp=*(struct xprintf**)xpp;

    free(xp->p);
    free(xp);
    *xpp=0;
}
/*****************************************************************************/
const char*
xprintf_get(void *vxp)
{
    struct xprintf *xp=(struct xprintf*)vxp;
    return (xp->p);
}
/*****************************************************************************/
#if 0
int
xprintf_print(FILE *f, void *vxp)
{
    struct xprintf *xp=(struct xprintf*)vxp;
    return fprintf(f, "%s", xp->p);
}
#endif
/*****************************************************************************/
#define MINSIZE 128
int
xprintf(void *vxp, const char *format, ...)
{
    struct xprintf *xp=(struct xprintf*)vxp;
    va_list va;
    int again=1, need_more_space=0, n;

    if (xp) {
        do {
            /*  allocate an initial string of MINSIZE
                or make more space */
            if (!xp->p || xp->size<xp->pos+MINSIZE)
                need_more_space=1;


            if (need_more_space) {
                char *np;
                int ns;

                if (!xp->p)
                    ns=MINSIZE;
                else
                    ns=2*xp->size;
                np=realloc(xp->p, ns);
                if (!ns) {
                    printf("xprintf: alloc %llu bytes: %s\n",
                    (unsigned long long)ns, strerror(errno));
                    return -1;
                }
                xp->p=np;
                xp->size=ns;
            }

            va_start(va, format);
            n=vsnprintf(xp->p+xp->pos, xp->size-xp->pos, format, va);
            va_end(va);

            if (n>-1 && n<xp->size-xp->pos) { /* space was sufficient */
                xp->pos+=n;
                again=0;
            } else {
                need_more_space=1;
            }
        } while (again);
    } else {
        va_start(va, format);
        n=vprintf(format, va);
        va_end(va);
    }

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
