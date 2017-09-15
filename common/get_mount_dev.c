/*
 * common/get_mount_dev.c
 * created 2011-Apr-11 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: get_mount_dev.c,v 2.3 2011/04/15 17:19:22 wuestner Exp $";

#define _GNU_SOURCE
#include "config.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <alloca.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <rcs_ids.h>
#include "get_mount_dev.h"

RCS_REGISTER(cvsid, "common")

/*****************************************************************************/
/*
 * canonicalise removes unneeded '/./' and '/../' from a pathname
 *
 * because strtok is used this procedure is not reentrant!
 */
static int
canonicalise(char *dst, char *src, size_t size /* size of dst */, FILE *log)
{
    int len=0; /* used space in dst */
    char *p0;

    dst[0]=0; /* just for safety */

    /* save a leading '/' because strtok would skip it */
    if (src[0]=='/') {
        strcpy(dst, "/");
        src++;
        len++;
    } else {
        dst[0]='\0';
    }

    p0=strtok(src, "/"); /* first strtok never returns NULL */
    if (!p0) /* src is empty */
        return 0;
    do {
        int add_it=0;

        if (!strcmp(p0, ".")) {
            /* ignore it */
        } else if (!strcmp(p0, "..")) {
            char *p1;
            /* find last '/' in dst */
            p1=strrchr(dst, '/');
            if (!dst[0]) /* dst is empty */
                add_it=1;
            else if (!p1) /* dst does not contain '/' */
                dst[0]=0; /* remove last element (==whole string) */
            else if (!strcmp(p1+1, "..")) /* last element is ".." */
                add_it=1;
            else
                *p1=0; /* remove last element */
        } else {
            add_it=1;
        }
        if (add_it) {
            size_t l;
            if (len+(l=strlen(p0)+1)>=size) {
                fprintf(log, "get_mount_dev: pathname too long\n");
                return -1;
            }
            if (len)
                strcat(dst, "/");
            strcat(dst, p0);
            len+=l;
        }
        p0=strtok(0, "/");
    } while (p0);

    return 0;
}

/*
 * expand_link combines the name of a symbolic link and its content
 */
static int
expand_link(char *name, size_t size, FILE *log)
{
    ssize_t sres;
    int res=-1;
    char *b, *p;

    b=(char*)(malloc(2*size+1));
    if (!b) {
        fprintf(log, "get_mount_dev:malloc: %s\n", strerror(errno));
        return -1;
    }

    sres=readlink(name, b, size);
    if (sres<0) {
        fprintf(log, "get_mount_dev:readlink(%s): %s\n",
                name, strerror(errno));
        goto error;
    }
    b[sres]='\0';

    /* if b is absolute we use it as it is */
    if (b[0]=='/') {
        goto OK;
    }

    /* b is relative to name (not to our CWD!) */
    /* we have to combine name and b, we know that b is large enough for both */

    /* remove the last part of name */
    p=strrchr(name, '/');
    if (!p) { /* no directory part, b is relative to our CWD */
        goto OK;
    }
    p[1]='\0'; /* name now ends with a '/' */

    /* make space for name at begin of b and copy name into it */
    memmove(b+(p-name+1), b, strlen(b)+1);
    memcpy(b, name, p-name+1);

OK:
    res=canonicalise(name, b, size, log);

error:
    free(b);
    return res;
}

/*
 * 'name' can be a file or a symbolic link
 * if name is a file it must exist
 * if name is a link it must exist, but the destination of the link does not
 *   need to exist.
 * returns:
 *    0: success, *dev is valid
 *   -1: error
 *    1: 'name' does not exist
 */
int
get_mount_dev(const char *name, dev_t *dev, FILE *log)
{
    char a[PATH_MAX+1];
    struct stat buf;
    int res, lcount=20;

    if (strlen(name)>PATH_MAX) {
        fprintf(log, "get_mount_dev: filename %s is too long\n", name);
        return -1;
    }
    strcpy(a, name);

    res=lstat(a, &buf);
    if (res<0) {
        int error=errno;
        fprintf(log, "get_mount_dev(%s): %s\n", a, strerror(errno));
        return error==ENOENT?1:-1;
    }

    do {
        if (res==0) { /* either link or file */
            if (S_ISLNK(buf.st_mode)) { /* it is a link */
                if (!--lcount) {
                    fprintf(log, "get_mount_dev(%s): %s\n",
                            name, strerror(ELOOP));
                    return -1;
                }
                if (expand_link(a, PATH_MAX, log)<0)
                    break;
            } else { /* it exists and is not a symbolic link */
                *dev=buf.st_dev;
                return 0;
            }
        } else if (errno==ENOENT) { /* it does not exist */
            char *p;
            p=strrchr(a, '/');
            if (p && p!=a) {
                if (p==a)
                    p[1]=0;
                else
                    p[0]=0;
            } else {
                fprintf(log, "get_mount_dev(%s): %s\n", a, strerror(errno));
                break;
            }
        } else { /* fatal error */
            fprintf(log, "get_mount_dev:lstat(%s): %s\n", a, strerror(errno));
            break;
        }

        res=lstat(a, &buf);
    } while (1);

    return -1;
}
