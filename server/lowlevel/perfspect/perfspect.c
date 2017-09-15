/*
 * lowlevel/perfspect/perfspect.c
 * created 29.Juni.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: perfspect.c,v 1.8 2011/04/06 20:30:27 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifndef HAVE_CGETCAP
#include <getcap.h>
#endif
#include <emsctypes.h>
#include <rcs_ids.h>

#include "../../main/server.h"
#include "perfspect.h"
#include <xdrstring.h>

RCS_REGISTER(cvsid, "lowlevel/perfspect")

static char* devicename="perf";
static char* configname="perfp";

#define PERFSPECT_CRASH

#ifdef PERFSPECT_CRASH
#define PERFSPECT_FATAL() *(int*)0=0
#else
#define PERFSPECT_FATAL() do {} while (0)
#endif

#define SPECTSIZE_ 1000
/*#define SDEBUG*/
static int spectsize;
static int scale;

static struct perf_dev* perfdev=0;

/* spectrum of one point of time in the readout sequence */
struct perf_spect {
    char* name;   /* short descr. of the point (e.g. "FRD after wait") */
    int hits;
    int overflows;
    int* histp;
    ems_u32 last;
};

/* the spectra of one EMS procedure in one procedure list */
struct perf_procinfo {
    int id;
    int num_extra_spects;
    struct perf_spect start;
    struct perf_spect end;
    struct perf_spect* extra_spects;
};

struct perf_isinfo {
    int numproc;
    struct perf_procinfo* procinfos;
};

static struct perf_isinfo perf[MAX_TRIGGER][MAX_IS];
static struct perf_isinfo* perfspect_is_ptr;
static struct perf_procinfo* perfspect_proc_ptr;
static enum perfspect_state perfspect_state;
static int perfspect_time_idx;
static int perfspect_trigg;
static int perfspect_is;
static int perfspect_proc;

const char* perfstatenames[]={
    "invalid",
    "setup",
    "ready",
    "inactive",
    "active",
};
/*
perf
perf[trig]
perf[trig][is]
perf[trig][is].numproc
perf[trig][is].procinfos*
perf[trig][is].procinfos[proc]
perf[trig][is].procinfos[proc].id
perf[trig][is].procinfos[proc].num_extra_spects
perf[trig][is].procinfos[proc].start;
perf[trig][is].procinfos[proc].end;
perf[trig][is].procinfos[proc].extra_spects
*/

static const int stacksize=1024;
static int stack_changer=0;
static ems_u32 stack_[2][1024];
static ems_u32* stack;
static ems_u32* stack_old;
static int stackidx;
static int stackidx_old;
static int stackmark;

/*****************************************************************************/
void
perfspect_reset(void)
{
    int trig, is, proc, ex;

    printf("perfspect_reset\n");
    for (trig=0; trig<MAX_TRIGGER; trig++) {
        for (is=0; is<MAX_IS; is++) {
            if (perf[trig][is].numproc) {
                for (proc=0; proc<perf[trig][is].numproc; proc++) {
                    for (ex=0; ex<perf[trig][is].procinfos[proc].num_extra_spects; ex++) {
                        free(perf[trig][is].procinfos[proc].extra_spects[ex].histp);
                    }
                    free(perf[trig][is].procinfos[proc].extra_spects);
                    free(perf[trig][is].procinfos[proc].start.histp);
                    free(perf[trig][is].procinfos[proc].end.histp);
                }
                free(perf[trig][is].procinfos);
                perf[trig][is].procinfos=0;
                perf[trig][is].numproc=0;
            }
        }
    }
    stack_changer=0;
    stack=stack_[stack_changer];
    stack_old=stack_[1-stack_changer];
    printf("stack_   =%p\n", stack_);
    printf("stack    =%p\n", stack);
    printf("stack_old=%p\n", stack_old);
    stackidx_old=0;
    stackidx=0;
    perfspect_state=perfspect_state_invalid;
}
/*****************************************************************************/
static void
clear_spect(struct perf_spect* spect)
{
    memset(spect->histp, 0, spectsize*sizeof(int));
    spect->hits=0;
    spect->overflows=0;
}
/*****************************************************************************/
static plerrcode
realloc_spect(struct perf_spect* spect)
{
    spect->histp=realloc(spect->histp, spectsize*sizeof(int));
    if (!spect->histp)
        return plErr_NoMem;
    clear_spect(spect);
    return plOK;
}
/*****************************************************************************/
static void
perfspect_clear_arrays(void)
{
    int trig, is, proc, ex;

    printf("perfspect_clear_arrays\n");
    for (trig=0; trig<MAX_TRIGGER; trig++) {
        for (is=0; is<MAX_IS; is++) {
            if (perf[trig][is].numproc) {
                for (proc=0; proc<perf[trig][is].numproc; proc++) {
                    for (ex=0; ex<perf[trig][is].procinfos[proc].num_extra_spects; ex++) {
                        clear_spect(&perf[trig][is].procinfos[proc].extra_spects[ex]);
                    }
                    clear_spect(&perf[trig][is].procinfos[proc].start);
                    clear_spect(&perf[trig][is].procinfos[proc].end);
                }
            }
        }
    }
}
/*****************************************************************************/
plerrcode
perfspect_realloc_arrays(void)
{
    int trig, is, proc, ex;
    plerrcode pres=0; /*==plOK*/

    printf("perfspect_realloc_arrays\n");
    for (trig=0; trig<MAX_TRIGGER; trig++) {
        for (is=0; is<MAX_IS; is++) {
            if (perf[trig][is].numproc) {
                for (proc=0; proc<perf[trig][is].numproc; proc++) {
                    for (ex=0; ex<perf[trig][is].procinfos[proc].num_extra_spects; ex++) {
                        pres|=realloc_spect(&perf[trig][is].procinfos[proc].extra_spects[ex]);
                    }
                    pres|=realloc_spect(&perf[trig][is].procinfos[proc].start);
                    pres|=realloc_spect(&perf[trig][is].procinfos[proc].end);
                }
            }
        }
    }

    if (!pres)
        return plOK;

    perfspect_set_state(perfspect_state_invalid);
    return plErr_NoMem;
}
/*****************************************************************************/
void
perfspect_set_state(enum perfspect_state state)
{
#ifdef SDEBUG
    printf("perfspect_set_state %s --> %s\n",
        perfstatenames[perfspect_state], perfstatenames[state]);
#endif
    perfspect_state=state;
}
/*****************************************************************************/
static void
swap_stack(void)
{
    stack_old=stack_[stack_changer];
    stack_changer=1-stack_changer; /* 1->0 or 0->1 */
    stack=stack_[stack_changer];
    stackidx_old=stackidx;
}
/*****************************************************************************/
void
perfspect_eventstart()
{
    if (perfdev==0) return;
    swap_stack();
    stackidx=0;
    stackmark=0;
    perfdev->perf_hw_eventstart(perfdev);
}
/*****************************************************************************/
static void
perfspect_push(ems_u32 val)
{
/*if (count++<100) printf("push %d\n", val);*/
    if (stackidx>=stacksize) return;
    stack[stackidx++]=val;
}
/*****************************************************************************/
static void
perfspect_mark_stack(void)
{
/*if (count++<100) printf("mark %d\n", stackidx);*/
    stackmark=stackidx;
    perfspect_push(-1);
}
/*****************************************************************************/
static void
perfspect_count_stack(void)
{
    stack[stackmark]=stackidx-stackmark-1;
/*if (count++<100) printf("count [%d]=%d\n", stackmark, stackidx-stackmark-1);*/
}
/*****************************************************************************/
static void
perfspect_push_trig_is(unsigned int trig, unsigned int is)
{
/*if (count++<100) printf("push is %d\n", is);*/
    perfspect_push(perfcode_trig_is); /* trigger ID is already in event header */
    perfspect_push(trig);
    perfspect_push(is);
}
/*****************************************************************************/
static void
perfspect_push_proc(int proc_idx)
{
/*if (count++<100) printf("push proc %d\n", proc_idx);*/
    perfspect_push(perfcode_proc);
    perfspect_push(proc_idx);
    perfspect_mark_stack();
}
/*****************************************************************************/
void
perfspect_set_trigg_is(unsigned int trig, unsigned int is)
{
#ifdef SDEBUG
    printf("perfspect_set_trigg_is trig %d is %d\n", trig, is);
#endif
    perfspect_trigg=trig;
    perfspect_is=is;
    perfspect_is_ptr=&perf[trig][is];
    perfspect_push_trig_is(trig, is);
}
/*****************************************************************************/
void
perfspect_alloc_isdata(int numproc)
{
    if (perfspect_state!=perfspect_state_setup) return;

    if (!perfspect_is_ptr) {
        printf("perfspect_allocate_is: perfspect_is_ptr==0\n");
        PERFSPECT_FATAL();
    }
    if (perfspect_is_ptr->numproc) {
        printf("perfspect_allocate_is: IS structure already allocated\n");
        PERFSPECT_FATAL();
        return;
    }
    perfspect_is_ptr->procinfos=
        (struct perf_procinfo*)calloc(numproc, sizeof(struct perf_procinfo));
    if (!perfspect_is_ptr->procinfos) {
        printf("perfspect_allocate_is: %d procs: %s\n",
            numproc, strerror(errno));
        PERFSPECT_FATAL();
        return;
    }
    perfspect_is_ptr->numproc=numproc;
    return;
}
/*****************************************************************************/
static char* startname="begin";
static char* endname="end";
/*
    the strings in 'names' have to be in static storage
    they are NOT copied!
*/
void
perfspect_alloc_procdata(int proc_idx, int proc_id, const char* name,
        int num, char** names)
{
    int i;

    if (perfspect_state!=perfspect_state_setup) return;

    printf("perfspect: %s (idx %d id %d is %d trigg %d): %d spectra\n",
        name, proc_idx, proc_id, perfspect_is, perfspect_trigg, num);
    if (!perfspect_is_ptr) {
        printf("perfspect_alloc_procdata: perfspect_is_ptr==0\n");
        PERFSPECT_FATAL();
    }
    if (perfspect_is_ptr!=&perf[perfspect_trigg][perfspect_is]) {
        printf("perfspect_alloc_procdata perfspect_is_ptr mismatch\n");
        PERFSPECT_FATAL();
    }
    if (perfspect_is_ptr->numproc<=proc_idx) {
        printf("perfspect_allocate_is: numproc=%d idx=%d\n",
                perfspect_is_ptr->numproc, proc_idx);
        PERFSPECT_FATAL();
        return;
    }
    perfspect_proc_ptr=&perfspect_is_ptr->procinfos[proc_idx];
    perfspect_proc_ptr->id=proc_id;
    perfspect_proc_ptr->extra_spects=calloc(num, sizeof(struct perf_spect));
    if (!perfspect_proc_ptr->extra_spects) {
        printf("perfspect_allocate_procdata(num=%d): %s\n",
            num, strerror(errno));
        PERFSPECT_FATAL();
        return;
    }
    perfspect_proc_ptr->num_extra_spects=num;
    perfspect_proc_ptr->start.name=startname;
    perfspect_proc_ptr->end.name=endname;
    for (i=0; i<num; i++) {
        perfspect_proc_ptr->extra_spects[i].name=names[i];
    }

    return;
}
/*****************************************************************************/
static void
perfspect_put(struct perf_spect* spect, ems_u32 time)
{
    perfspect_push(time);
    if (scale) time>>=scale;
    if (time>=spectsize) {
        spect->overflows++;
    } else {
        spect->histp[time]++;
    }
    spect->hits++;
}
/*****************************************************************************/
void perfspect_start_proc(int proc_idx)
{
#ifdef SDEBUG
    printf("perfspect_start_proc idx %d\n", proc_idx);
#endif
    if (perfspect_state!=perfspect_state_active) return;
    if (!perfspect_is_ptr) {
        printf("perfspect_start_proc: perfspect_is_ptr==0\n");
        PERFSPECT_FATAL();
    }
    if (perfspect_is_ptr->numproc<=proc_idx) {
        printf("perfspect_start_proc: numproc=%d idx=%d\n",
                perfspect_is_ptr->numproc, proc_idx);
        PERFSPECT_FATAL();
        return;
    }
    perfspect_push_proc(proc_idx);
    perfspect_proc=proc_idx;
    perfspect_proc_ptr=&perfspect_is_ptr->procinfos[proc_idx];
    if (perfdev)
        perfspect_put(&perfspect_proc_ptr->start, perfdev->perf_time(perfdev));
    perfspect_time_idx=0;
}
/*****************************************************************************/
void perfspect_end_proc(void)
{
#ifdef SDEBUG
    printf("perfspect_end_proc state %d \n", perfspect_state);
#endif
    if (perfspect_state!=perfspect_state_active) return;
    if (!perfspect_proc_ptr) {
        printf("perfspect_end_proc: perfspect_proc_ptr==0\n");
        PERFSPECT_FATAL();
        return;
    }
    if (perfdev)
        perfspect_put(&perfspect_proc_ptr->end, perfdev->perf_time(perfdev));
    perfspect_count_stack();
    perfspect_proc_ptr=0;
    perfspect_proc=-1;
}
/*****************************************************************************/
void
perfspect_record_time(char* caller)
{
#ifdef SDEBUG
    printf("perfspect_record_time(%s)\n", caller);
#endif
    if (perfspect_state!=perfspect_state_active) return;
    /*printf("perfspect_record_time: proc idx %d is %d trigg %d\n",
        perfspect_proc, perfspect_is, perfspect_trigg);*/
    
    if (!perfspect_proc_ptr) {
        printf("perfspect_record_time(%s): perfspect_proc_ptr==0\n", caller);
        PERFSPECT_FATAL();
        return;
    }
    if (perfspect_proc_ptr->num_extra_spects<=perfspect_time_idx) {
        printf("perfspect_record_time(%s): num_extra_spects=%d idx=%d\n",
                caller,
                perfspect_proc_ptr->num_extra_spects, perfspect_time_idx);
        PERFSPECT_FATAL();
        return;
    }
    if (perfdev) {
        unsigned int time=perfdev->perf_time(perfdev);
        perfspect_put(&perfspect_proc_ptr->extra_spects[perfspect_time_idx],
            time);
    }
    perfspect_time_idx++;
}
/*****************************************************************************/
plerrcode perfspect_get_spect(unsigned int trig, unsigned int is,
    unsigned int proc, int ident, ems_u32** out)
{
    struct perf_isinfo* is_ptr;
    struct perf_procinfo* proc_ptr;
    struct perf_spect* spect;
    int* hist;
    int i, j, num;

    if (trig>=MAX_TRIGGER) {
        **out=1; (*out)++;
        return plErr_ArgRange;
    }
    if (is>=MAX_IS) {
        **out=2; (*out)++;
        return plErr_ArgRange;
    }
    is_ptr=&perf[trig][is];
    if (!is_ptr) {
        printf("perfspect_get_spect: is_ptr==0\n");
        return plErr_System;
    }
    if (proc>=is_ptr->numproc) {
        **out=3; (*out)++;
        return plErr_ArgRange;
    }
    proc_ptr=is_ptr->procinfos+proc;
    if (!proc_ptr) {
        printf("perfspect_get_spect: proc_ptr==0\n");
        return plErr_System;
    }
    switch (ident) {
    case -1:
        spect=&proc_ptr->start;
        break;
    case -2:
        spect=&proc_ptr->end;
        break;
    default:
        if ((unsigned int)ident>=proc_ptr->num_extra_spects) {
            **out=4; (*out)++;
            return plErr_ArgRange;
        }
        spect=proc_ptr->extra_spects+ident;
    }
    if (!spect) {
        printf("perfspect_get_spect: spect==0\n");
        return plErr_System;
    }

    hist=spect->histp;
    if (!hist) {
        printf("perfspect_get_spect: hist==0\n");
        return plErr_System;
    }

    **out=5;                (*out)++;
    **out=spectsize;        (*out)++;
    **out=scale;            (*out)++;
    **out=spect->hits;      (*out)++;
    **out=spect->overflows; (*out)++;
    i=0;
    while ((i<spectsize) && !hist[i]) i++;
    **out=i;                (*out)++;

    j=spectsize-1;
    while ((j>i) && !hist[j]) j--;
    j++;
    **out=j-i;              (*out)++;
    num=7+j-i;
    for (; i<j; i++) {**out=hist[i]; (*out)++;}

    return plOK;
}
/*****************************************************************************/
/*
 * p[0]: number of arguments
if given:
 * nix                 --> list of used triggers
 * p[1]: trigger id    --> list of ISs using this trigger
 * p[2]: is idx        --> list of procedure IDs
 * p[3]: procedure-idx --> number of extraspects
 * p[4]: spectrum      --> name of spectrum
 */
plerrcode
perfspect_get_info(ems_u32* p, ems_u32** outptr)
{
    ems_u32 *out=*outptr;
    int i, j;

    switch (p[0]) {
    case 0: /* list of used triggers */
        for (i=0; i<MAX_TRIGGER; i++) {
            int n=0;
            for (j=0; j<MAX_IS; j++) n+=perf[i][j].numproc;
            if (n) *out++=i;
        }
        break;
    case 1: /* list of ISs using this trigger */
        if (p[1]>=MAX_TRIGGER) return plErr_ArgRange;
        for (i=0; i<MAX_IS; i++) {
            if (perf[p[1]][i].numproc) *out++=i;
        }
        break;
    case 2: /* list of procedure IDs */
        if (p[1]>=MAX_TRIGGER) return plErr_ArgRange;
        if (p[2]>=MAX_IS) return plErr_ArgRange;
        for (i=0; i<perf[p[1]][p[2]].numproc; i++) {
            *out++=perf[p[1]][p[2]].procinfos[i].id;
        }
        break;
    case 3: /* number of extraspects */
        if (p[1]>=MAX_TRIGGER) return plErr_ArgRange;
        if (p[2]>=MAX_IS) return plErr_ArgRange;
        if (p[3]>=perf[p[1]][p[2]].numproc) return plErr_ArgRange;
        *out++=perf[p[1]][p[2]].procinfos[p[3]].num_extra_spects;
        break;
    case 4: /* name of spectrum */
        {
            ems_i32 spect=((ems_i32*)p)[4];
            char* name;

            if (p[1]>=MAX_TRIGGER) return plErr_ArgRange;
            if (p[2]>=MAX_IS) return plErr_ArgRange;
            if (p[3]>=perf[p[1]][p[2]].numproc) return plErr_ArgRange;
            if ((spect<-2) ||
                (spect>=perf[p[1]][p[2]].procinfos[p[3]].num_extra_spects))
                        return plErr_ArgRange;
            switch (spect) {
            case -2: name=perf[p[1]][p[2]].procinfos[p[3]].end.name; break;
            case -1: name=perf[p[1]][p[2]].procinfos[p[3]].start.name; break;
            default: name=perf[p[1]][p[2]].procinfos[p[3]].extra_spects[spect].name;
            }
            if (name)
                out=outstring(out, name);
            else
                out=outstring(out, "NONAME");
        }
    }
    *outptr=out;
    return plOK;
}
/*****************************************************************************/
plerrcode
perfspect_infotree(ems_u32** outptr)
{
    ems_u32 *hlp_trig, *out=*outptr;
    int trig, is, proc;

    *(hlp_trig=out++)=0;
    for (trig=0; trig<MAX_TRIGGER; trig++) {
        ems_u32 *hlp_is=0;
        int iscount=0;
        for (is=0; is<MAX_IS; is++) {
            struct perf_isinfo* is_ptr=&perf[trig][is];
            if (is_ptr->numproc) {
                if (!iscount) {
                    *out++=trig;
                    hlp_is=out++;
                }
                iscount++;
                *out++=is;
                *out++=is_ptr->numproc;
                for (proc=0; proc<is_ptr->numproc; proc++) {
                    *out++=proc;
                    *out++=is_ptr->procinfos[proc].id;
                    *out++=is_ptr->procinfos[proc].num_extra_spects;
                }
            }
        }
        if (iscount) {
            *hlp_is=iscount;
            (*hlp_trig)++;
        }
    }
    *outptr=out;
    return plOK;
}
/*****************************************************************************/
void
perfspect_get_setup(int* size, int* shift)
{
    *size=spectsize;
    *shift=scale;
}
/*****************************************************************************/
int
perfspect_readout_stack(ems_u32* outptr)
{
    memcpy(outptr, stack_old, stackidx_old*sizeof(ems_u32));
    return stackidx_old;
}
/*****************************************************************************/
plerrcode
perfspect_set_setup(int size, int shift)
{
    plerrcode pres;

    if (size>=0) spectsize=size;
    if (shift>=0) scale=shift;

    if (perfspect_state>=perfspect_state_ready) {
        if (size>=0) {
            pres=perfspect_realloc_arrays();
            if (pres)
                return pres;
        } else {
            perfspect_clear_arrays();
        }
    } else {
        printf("perfspect not ready\n");
        PERFSPECT_FATAL();
    }
    return plOK;
}
/*****************************************************************************/
int perfspect_low_printuse(FILE* outfilepath)
{
    fprintf(outfilepath, "  [:perfp=perfdev_path\n");
    return 1;
}
/*****************************************************************************/
struct perf_init {
    enum perftypes perftype;
    errcode (*init)(struct perf_dev* dev);
    const char* name;
};

struct perf_init perf_init[]= {
#ifdef PERFSPECT_ZELSYNC
    {perf_pcisync, perfspect_init_pcisync, "pcisync"},
#endif
    {perf_none, 0, ""}
};

errcode perfspect_low_init(char* arg)
{
    char *devicepath, *help;
    int trig, is, i, j, n;
    errcode res;

    perfdev=0;

    spectsize=SPECTSIZE_;
    scale=0;

    if((!arg) || (cgetstr(arg, configname, &devicepath) < 0)){
        printf("no perf device given\n");
        return OK;
    }

    help=devicepath;
    res=0; n=0;
    while (help) {
        char *comma;
        comma=strchr(help, ',');
        if (comma) *comma='\0';
        if (!strcmp(help, "none")) {
            printf("No %s device for device %d\n", devicename, n);
            register_device(modul_perf, 0);
        } else {
            char *semi;
            struct perf_dev* dev;
            semi=strrchr(help, ';');
            if (!semi || semi==help) {
                printf("No perf device type given for %s\n", help);
                res=Err_ArgRange;
                break;
            }
            *semi++='\0';
            semi++;

            dev=calloc(1, sizeof(struct perf_dev));
            init_device_dummies(&dev->generic);
            /* find type of perf device */
            for (j=0; perf_init[j].perftype!=perf_none; j++) {
                if (strcmp(semi, perf_init[j].name)==0) {
                    dev->perftype=perf_init[j].perftype;
                    dev->pathname=strdup(help);
                    res=perf_init[j].init(dev);
                    break;
                }
            }
            /* perf device not found */
            if (perf_init[j].perftype==perf_none) {
                printf("perf device type %s not known.\n", semi);
                printf("valid types are:");
                for (j=0; perf_init[j].perftype!=perf_none; j++) {
                    printf(" '%s'", perf_init[j].name);
                }
                printf("\n");
                res=Err_ArgRange;
            }
            if (res==OK) {
                int nn, i;
                nn=(void**)&dev->DUMMY-(void**)&dev->generic.done;
                for (i=0; i<nn; i++) {
                    if (((void**)&dev->generic.done)[i]==0) {
                        printf("ERROR: PERF procedure idx=%d for %s "
                                "not set\n", i, help);
                        return Err_Program;
                    }
                }
                register_device(modul_perf, (struct generic_dev*)dev);
            } else {
                free(dev);
                break;
            }
        }
        n++;
        help=comma?comma+1:0;
    }
    free(devicepath);
    if (res) goto errout;

    {
        struct generic_dev* gendev;
        if (find_device(modul_perf, 0, &gendev)==OK)
            perfdev=(struct perf_dev*)gendev;
        else
            printf("perfspect: no usable device found\n");
    }

    for (trig=0; trig<MAX_TRIGGER; trig++) {
        for (is=0; is<MAX_IS; is++) {
            perf[trig][is].numproc=0;
            perf[trig][is].procinfos=0;
        }
    }
    return OK;

errout:
    for (i=0; i<num_devices(modul_perf); i++) {
        struct generic_dev* dev;
        if (find_device(modul_perf, i, &dev)==OK) {
            dev->generic.done(dev);
            destroy_device(modul_perf, i);
        }
    }

    return res;
}

errcode perfspect_low_done(void)
{
    /*perfspect_done();*/
    perfspect_reset();
    perfdev=0;
    return OK;
}

plerrcode
perfspect_set_dev(int idx)
{
    if (idx<0) {
        perfdev=0;
    } else {
        struct generic_dev* gendev;
        plerrcode pres;

        if ((pres=find_device(modul_perf, 0, &gendev))==OK)
            perfdev=(struct perf_dev*)gendev;
        else
            return pres;
    }
    return plOK;
}

struct perf_dev*
perfspect_get_dev(void)
{
    return perfdev;
}

/*****************************************************************************/
/*****************************************************************************/
