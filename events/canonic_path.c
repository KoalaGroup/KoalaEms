/*
 * events/canonic_path.c
 * $ZEL: canonic_path.c,v 1.2 2010/09/04 21:23:12 wuestner Exp $
 * 
 * created 17.Apr.2004 PW
 */

#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "canonic_path.h"

enum element_type {
    e_invalid=0, /* unused entry */
    e_root,      /* leading / */
    e_empty,     /* '' */
    e_null,      /* . */
    e_back,      /* .. */
    e_simple,    /* any other dir */
    e_file       /* last element; no trailing / */
};

struct pathelement {
    char* element;
    int len;
    enum element_type etype;
};

static void
move_elements(struct pathelement* ed, struct pathelement* es)
{
    while (es->etype!=e_invalid) *ed++=*es++;
    *ed=*es;
}

int
canonic_path(const char* pathname, char** dirname, char** filename, int split)
{
    struct pathelement* elements;
    char *path, *newpath, *p0, *p1;
    int e, i, l;

    if (!pathname || !*pathname) {
        fprintf(stderr, "canonic_path: no or empty path given\n");
        return -1;
    }

    if (*pathname=='/') {
        path=strdup(pathname);
    } else {
        char *cwd=getcwd(0, 0);
        if (!cwd) {
            fprintf(stderr, "getcwd: %s\n", strerror(errno));
            return -1;
        }
        path=malloc(strlen(cwd)+strlen(pathname)+2);
        sprintf(path, "%s/%s", cwd, pathname);
        free(cwd);
    }

    l=strlen(path);

    e=0;
    for (i=0; i<l; i++)
        if (path[i]=='/')
            e++;

    elements=calloc(e+2, sizeof(struct pathelement));

    e=0;
    p0=path;
    if (*path=='/') {
        elements[e].etype=e_root;
        p0++;
        e++;
    }
    do {
        p1=strchr(p0, '/');
        if (p1) {
            elements[e].element=p0;
            elements[e].len=p1-p0-1;
            *p1=0;
            if (!strcmp(p0, ""))
                elements[e].etype=e_empty;
            else if (!strcmp(p0, "."))
                elements[e].etype=e_null;
            else if (!strcmp(p0, ".."))
                elements[e].etype=e_back;
            else
                elements[e].etype=e_simple;
            e++;
            p0=p1+1;
        } else {
            elements[e].element=p0;
            elements[e].len=strlen(p0);
            elements[e].etype=e_file;
        }
    } while (p1);
    e=0;
    while (elements[e].etype!=e_invalid) {
        switch (elements[e].etype) {
        case e_root:
        case e_simple:
        case e_file:
            e++;
            break;
        case e_empty:
        case e_null:
            move_elements(elements+e, elements+e+1);
            break;
        case e_back:
            if (!e) {
                free(path);
                return -1;
            }
            if (elements[e-1].etype==e_root) {
                move_elements(elements+e, elements+e+1);
            } else {
                move_elements(elements+e-1, elements+e+1);
                e--;
            }
            break;
        case e_invalid:
            break;
        }
    };

    if (filename)
        *filename=0;
    newpath=malloc(l+1);
    *newpath=0;
    for (e=0; elements[e].etype!=e_invalid; e++) {
        switch (elements[e].etype) {
        case e_root:
            strcat(newpath, "/");
            break;
        case e_simple:
            strcat(newpath, elements[e].element);
            strcat(newpath, "/");
            break;
        case e_file:
            if (!split)
                strcat(newpath, elements[e].element);
            if (filename)
                *filename=strdup(elements[e].element);
            break;
        case e_empty:
        case e_null:
        case e_back:
            fprintf(stderr, "program error in canonic_path\n");
        case e_invalid:
            break;
        }
    }
    free(path);
    *dirname=newpath;
    if (filename && !*filename)
        *filename=strdup("");

    return 0;
}
