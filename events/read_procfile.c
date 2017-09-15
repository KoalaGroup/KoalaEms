#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <ctype.h>

#include "read_procfile.h"

static int
parse_is(struct is* is, char* s, int line)
{
    ems_u32 id=-1;
    int res, n=-1;

    res=sscanf(s, "%d%n", &id, &n);
    if (res!=1) {
        fprintf(stderr, "(parse_is) cannot parse >%s< (line %d)\n", s, line);
        return -1;
    }
    is->id=id;
    return 0;
}

static int
parse_trigger(struct is* is, char* s, int line)
{
    struct proclist* current_proclist;
    ems_u32 trigger[MAXTRIGGER];
    ems_u32 t;
    int num=0, res, n, i;

    do {
        res=sscanf(s, "%d %n", &t, &n);
        if (res==1) {
            if (t>=MAXTRIGGER) {
                fprintf(stderr, "illegal trigger %d in line %d\n", t, line);
                return -1;
            }
            if (num<16) {
                trigger[num++]=t;
            } else {
                 fprintf(stderr, "too many triggers in line %d\n", line);
                 return -1;
            }
            s+=n;
        }
    } while (res==1);
    if (!num) {
        fprintf(stderr, "no trigger given in trigger line %d\n", line);
        return -1;
    }
    current_proclist=&is->lists[trigger[0]];
    is->current_proclist=current_proclist;
    current_proclist->num_procs=0;
    current_proclist->procs=0;

    for (i=0; i<num; i++) {
        is->proclists[trigger[i]]=current_proclist;
    }

    return 0;
}

static int
parse_proc(struct proclist* proclist, char* s, int line)
{
    char sp[1024];
    struct proc *proc;
    int res, args ,n , i;

    proc=malloc(sizeof(struct proc));
    if (!(proclist->num_procs&0xf)) {
        int i;
        struct proc** help=malloc((proclist->num_procs+16)*sizeof(struct proc*));
        for (i=0; i<proclist->num_procs; i++) help[i]=proclist->procs[i];
        free(proclist->procs);
        proclist->procs=help;
    }
    proclist->procs[proclist->num_procs++]=proc;

    res=sscanf(s, "%s%n", &sp, &n);
    if (res!=1) {
        fprintf(stderr, "(parse_proc A) cannot parse >%s< (line %d)\n", s, line);
        return -1;
    }
    s+=n;
    proc->name=strdup(sp);

    res=sscanf(s, "%d %n", &args, &n);
    if (res!=1) {
        fprintf(stderr, "(parse_proc B) cannot parse >%s< (line %d) (argcount missing)\n",
                s, line);
        return -1;
    }
    s+=n;
    proc->args=malloc(args*sizeof(ems_u32));
    proc->num_args=args;
    for (i=0; i<args; i++) {
        ems_u32 arg;
        res=sscanf(s, "%d %n", &arg, &n);
        if (res!=1) {
            fprintf(stderr, "(parse_proc C) cannot parse >%s< "
                    "(line %d argnum %d arg %d res=%d n=%d)\n",
                    s, line, args, i, res, n);
            return -1;
        }
        s+=n;
        proc->args[i]=arg;
    }
    proc->num_cachevals=0;
    proc->cachevals=0;

    return 0;
}

int read_procfile(const char* name, struct iss* iss)
{
    FILE* f;
    char ss[1024], *s;
    char sk[1024];
    struct is* is;
    int res, line=0;

    iss->iss=0;
    iss->num_is=0;
    is=0;

    f=fopen(name, "r");
    if (!f) {
        fprintf(stderr, "open %s: %s\n", name, strerror(errno));
        return -1;
    }

    while ((s=fgets(ss, 1024, f))) {
        int n;

       line++;
        /* skip trailing white space */
        while isspace(*s) s++;
        /* skip empty lines and comments */
        if (!*s || (*s=='#')) continue;

        res=sscanf(s, "%s %n", &sk, &n);
        if (res!=1) {
            fprintf(stderr, "(key) cannot parse >%s< (line %d)\n", s, line);
            goto error;
        }
        s+=n;

        if (!strcasecmp(sk, "is:")) {
            int i;
            is=malloc(sizeof(struct is));
            if (!(iss->num_is&0xf)) {
                int i;
                struct is** help=malloc((iss->num_is+16)*sizeof(struct is*));
                for (i=0; i<iss->num_is; i++) help[i]=iss->iss[i];
                free(iss->iss);
                iss->iss=help;
            }
            iss->iss[iss->num_is++]=is;
            for (i=0; i<16; i++) {
                is->proclists[i]=0;
                is->lists[i].num_procs=0;
                is->lists[i].procs=0;
            }
            is->current_proclist=0;
            if (parse_is(is, s, line)<0) goto error;
        } else if (!strcasecmp(sk, "trigger:")) {
            if (!is) {
                fprintf(stderr, "no IS given before trigger (line %d)\n",
                        line);
                goto error;
            }
            if (parse_trigger(is, s, line)<0) goto error;
        } else if (!strcasecmp(sk, "proc:")) {
            if (!is) {
                fprintf(stderr, "no IS given before proc (line %d)\n", line);
                goto error;
            }
            if (!is->current_proclist) {
                fprintf(stderr, "no trigger given before proc (line %d)\n",
                        line);
                goto error;
            }
            if (parse_proc(is->current_proclist, s, line)<0) goto error;
        } else {
            fprintf(stderr, "unknown key >%s< (line %d)\n", sk, line);
            goto error;
        }
    }

    fclose(f);
    return 0;

error:
    fclose(f);
    return -1;
}

void dump_procfile(struct iss* iss)
{
    int i_is, i_trig, i_proc, i;

    printf("dump_procfile:\n");
    printf("num_is=%d\n", iss->num_is);
    for (i_is=0; i_is<iss->num_is; i_is++) {
        struct is* is=iss->iss[i_is];
        printf("IS %d:\n", is->id);
        for (i_trig=0; i_trig<16; i_trig++) {
            if (is->proclists[i_trig]) {
                struct proclist* proclist=is->proclists[i_trig];
                printf("trigger=%d\n", i_trig);
                for (i_proc=0; i_proc<proclist->num_procs; i_proc++) {
                    struct proc* proc=proclist->procs[i_proc];
                    printf("proc %s", proc->name);
                    for (i=0; i<proc->num_args; i++) {
                        printf(" %x", proc->args[i]);
                    }
                    printf("\n");
                }
            }
        }
    }
}
