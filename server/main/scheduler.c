/*
 * main/scheduler.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: scheduler.c,v 1.10 2011/04/06 20:30:28 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#ifndef OSK
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#if !defined(__Lynx__) & !defined(__linux__)
#include <sys/select.h>
#endif
#endif /* OSK */
#include <rcs_ids.h>
#include "signals.h"
#include "timers.h"

#include "scheduler.h"
#include "server.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#ifndef __Lynx__
#define CHAR_P(x) ((char*)x)
#else
#define CHAR_P(x) x
#endif

volatile int breakflag;

static struct taskdescr *current;
static int numtasks;
#ifndef OSK
static int numselecttasks;
static int numselectpathes;
static struct taskdescr* select_descr;
static fd_set select_readfds;
static fd_set select_writefds;
static fd_set select_exceptfds;
static fd_set select_allfds;
static struct seltaskdescr* select_tasks;
static int select_tasks_changed;
#ifdef SELECT_STATIST
struct t_sched_select_statist sched_select_statist;
#endif  
#endif /* OSK */

RCS_REGISTER(cvsid, "main")

static volatile struct plistentry *breakaction, **ba_last;

static struct taskdescr idletask,*blocking;
/*****************************************************************************/
static void
handle_break(void)
{
    sigresc m;

    T(handle_break)
    disable_breaks(&m);
    while (breakaction) {
        struct plistentry *help;
        help=breakaction->next; /* proc can free breakaction */
        enable_breaks(&m);
        (breakaction->proc)(breakaction->arg);
        disable_breaks(&m);
        breakaction=help;
    }
    ba_last= &breakaction;
    breakflag=0;

    enable_breaks(&m);
}
/*****************************************************************************/
static void
insert_breakaction(struct plistentry* pl)
{
    sigresc m;

    T(insert_breakaction)
    disable_breaks(&m);
    *ba_last=pl; /* Reihenfolge ist wichtig: sonst Endlosschleife,
                    wenn gleicher entry mehrmals eingefuegt wird */
    pl->next=(struct plistentry*)0;
    ba_last=(volatile struct plistentry**)(&(pl->next));

    enable_breaks(&m);
}
/*****************************************************************************/
void
sched_mainloop()
{
    T(sched_mainloop)
    for(;;) {
        if (current->p_prio1) {
            if (!current->p_p2) {
                int n;
                void(*tp)(union callbackdata);
                union callbackdata ta;
                current->p_p2=current->p_prio2;
                tp=current->proc;
                ta=current->arg;
                /* p_prio1 kann auf 0 gesetzt werden (zum suspendieren) */
                for (n=current->p_prio1; (current->p_prio1) && (n>0); n--) {
	            if (breakflag) {
	                handle_break();
	                break;
	            }
	            (*tp)(ta);
                }
            } else {
                current->p_p2--;
            }
        }  
        current=current->p_next;
    }
}
/*****************************************************************************/
static void waiter(void){
T(waiter)
  /* select? */
/*  printf("idle\n"); */
  pause();
}

/*****************************************************************************/
int init_sched(){
  sigresc m;
T(init_sched)
  disable_breaks(&m);

  breakaction=0;
  ba_last= &breakaction;
  breakflag=0;

  idletask.typ=sched_idle;
  idletask.proc=(void(*)(union callbackdata))waiter;
  idletask.arg.p=(void*)0;
  idletask.p_prio1=1;
  idletask.p_prio2=idletask.p_p2=0;
  idletask.p_next= &idletask;
  idletask.p_prev= &idletask;
  idletask.name="idle";
  current= &idletask;

  blocking=0;
  numtasks=0;
#ifndef OSK
  numselecttasks=0;
  select_tasks=0;
  FD_ZERO(&select_readfds);
  FD_ZERO(&select_writefds);
  FD_ZERO(&select_exceptfds);
  FD_ZERO(&select_allfds);
#ifdef SELECT_STATIST
gettimeofday(&sched_select_statist.tv, 0);
sched_select_statist.idle=0.;
#endif  
#endif /* OSK */
  enable_breaks(&m);
  return(0);
}

/*****************************************************************************/
void done_sched(){}

/*****************************************************************************/
#ifndef OSK
struct seltaskdescr* get_first_select_task()
{
return select_tasks;
}

struct seltaskdescr* get_next_select_task(struct seltaskdescr *h)
{
return h->next;
}

void sched_print_select_tasks()
{
struct seltaskdescr *h;
T(sched_print_select_tasks)
printf("Select-Tasks:\n");

h=select_tasks;
while (h!=0)
  {
  printf(" %s (%d, %c%c%c(%c%c%c)): %p/%d\n",
            h->name, h->fds,
            (h->types&select_read?'r':'_'), 
            (h->types&select_write?'w':'_'), 
            (h->types&select_except?'e':'_'),
            (FD_ISSET(h->fds, &select_readfds)?'r':'_'),
            (FD_ISSET(h->fds, &select_writefds)?'w':'_'),
            (FD_ISSET(h->fds, &select_exceptfds)?'e':'_'),
            h->arg.p, h->arg.i);
  h=h->next;
  }
}
#endif /* OSK */
/*****************************************************************************/
void sched_print_tasks()
{
  sigresc m;
  struct taskdescr *h;
T(sched_print_tasks)
  printf("Tasks:\n");

  disable_breaks(&m);

  h=current;
  do{
    printf(" %s (%s): %d/%d/%d\n",h->name,
	   (h->typ==sched_blocking?"block":"poll"),
	   h->p_prio1,h->p_prio2,h->p_p2);
    h=h->p_next;
  }while(h!=current);
#ifndef OSK
sched_print_select_tasks();
#endif /* OSK */

  enable_breaks(&m);
}

/*****************************************************************************/
static void
do_insert(union callbackdata data)
{
struct taskdescr* t=(struct taskdescr*)data.p;
  sigresc m;
T(do_insert)
  disable_breaks(&m);

  if(blocking){
    if(numtasks!=1){
      printf("Fehler: !=1 blocking");
      exit(0);
    }
    blocking->proc=blocking->p_pproc;
    blocking=0;
  }

  if(!numtasks){
    if(current!=&idletask){
      printf("Fehler: current!=idle");
      exit(0);
    }
    current=t;

    if(t->typ==sched_blocking){
      t->proc=t->p_blproc;
      blocking=t;
    }
  }else{
    /* insert after current */
    t->p_next=current->p_next;
    t->p_prev=current;
  }

  current->p_next=t;
  t->p_next->p_prev=t;

  t->p_insert_pending=0;
  numtasks++;

  enable_breaks(&m);
/*sched_print_tasks();*/
}

/*****************************************************************************/
struct taskdescr*
sched_insert_poll_task(void(*proc)(union callbackdata),
        union callbackdata arg, int p1, int p2, char* name)
{
    struct taskdescr *mytask;
    struct plistentry *pl;

    T(sched_insert_poll_task)
    mytask=(struct taskdescr*)malloc(sizeof(struct taskdescr));
    if (!mytask)
        return 0;
    mytask->typ=sched_polling;
    mytask->proc=proc;
    mytask->arg=arg;
    mytask->name=name;
    mytask->p_prio1=p1;
    mytask->p_prio2=p2;
    mytask->p_p2=0; /* sofort */
    mytask->p_insert_pending=1;
    mytask->p_remove_pending=0;

    pl=&mytask->pl;
    pl->proc=do_insert;
    pl->arg.p=mytask;

    insert_breakaction(pl);
    breakflag=1;

    /* wakeup aus waiter? */
    return mytask;
}
/*****************************************************************************/
struct taskdescr*
sched_insert_block_task(void(*proc)(union callbackdata),
        void(*pollproc)(union callbackdata), union callbackdata arg,
        int p1, int p2, char* name)
{
    struct taskdescr *mytask;
    struct plistentry *pl;

    T(sched_insert_block_task)
    mytask=(struct taskdescr*)malloc(sizeof(struct taskdescr));
    if (!mytask)
        return 0;
    mytask->typ=sched_blocking;
    mytask->p_blproc=proc;
    mytask->p_pproc=pollproc;
    mytask->proc=pollproc;
    mytask->arg=arg;
    mytask->name=name;
    mytask->p_prio1=p1;
    mytask->p_prio2=p2;
    mytask->p_p2=0; /* sofort */
    mytask->p_insert_pending=1;
    mytask->p_remove_pending=0;

    pl=&mytask->pl;
    pl->proc=do_insert;
    pl->arg.p=mytask;

    insert_breakaction(pl);
    breakflag=1;

    /* wakeup aus waiter? */
    return mytask;
}
/*****************************************************************************/
#ifndef OSK
static void select_proc(struct timeval* to)
{
struct seltaskdescr* descr;
fd_set readfds, writefds, exceptfds;
int res;
#ifdef SELECT_STATIST
struct timeval tv0, tv1;
#endif

T(select_proc)
bcopy(CHAR_P(&select_readfds), CHAR_P(&readfds), sizeof(fd_set));
bcopy(CHAR_P(&select_writefds), CHAR_P(&writefds), sizeof(fd_set));
bcopy(CHAR_P(&select_exceptfds), CHAR_P(&exceptfds), sizeof(fd_set));

/* statt numselectpathes ist auch getdtablesize() moeglich */
#ifdef SELECT_STATIST
gettimeofday(&tv0, 0);
#endif
res=select(numselectpathes, &readfds, &writefds, &exceptfds, to);
#ifdef SELECT_STATIST
gettimeofday(&tv1, 0);
sched_select_statist.idle+=
    tv1.tv_sec-tv0.tv_sec+(tv1.tv_usec-tv0.tv_usec)/1000000.;
#endif
if (res>0)
  {
  select_tasks_changed=0;
  for (descr=select_tasks; descr!=0; descr=descr->next)
    {
    enum select_types types=0;
    if (FD_ISSET(descr->fds, &readfds)) types|=select_read;
    if (FD_ISSET(descr->fds, &writefds)) types|=select_write;
    if (FD_ISSET(descr->fds, &exceptfds)) types|=select_except;
    if (types)
      {
      (*descr->proc)(descr->fds, types, descr->arg);
      }
    if (select_tasks_changed) return; /* linked list 'select_tasks' has been
    changed; cannot be used any more in this loop */
    }
  }
}

static void
select_waitproc(union callbackdata arg)
{
    T(select_waitproc)
    select_proc((struct timeval*)0);
}

static void
select_pollproc(union callbackdata arg)
{
    struct timeval to;
    T(select_pollproc)
    to.tv_sec=0; to.tv_usec=0;
    select_proc(&to);
}
#endif /* OSK */
/*****************************************************************************/
#ifndef OSK
struct seltaskdescr*
sched_insert_select_task(void(*proc)(int, enum select_types,
    union callbackdata), union callbackdata arg,
    char* name, int path, enum select_types types
#ifdef SELECT_STATIST
    , int active
#endif
    )
{
    struct seltaskdescr *descr, *help;

    T(sched_insert_select_task)
    if (FD_ISSET(path, &select_allfds)) {
        printf("sched_insert_select_task(): path %d in use\n", path);
        return 0;
    }

    if ((descr=malloc(sizeof(struct seltaskdescr)))==0) {
        printf("sched_insert_select_task(): malloc: %s\n", strerror(errno));
        return 0;
    }
    if ((descr->name=malloc(strlen(name)+1))==0) {
        printf("sched_insert_select_task(), a: malloc: %s\n", strerror(errno));
        return 0;
    }

    /*
    printf("sched_insert_select_task(%p, path=%d, name=%s)\n", descr, path,
        name);
    */
    descr->next=0;
    descr->prev=0;
    descr->proc=proc;
    descr->arg=arg;
    strcpy(descr->name, name);
    /*descr->name=name;*/
    descr->fds=path;
    descr->types=types;
    FD_SET(path, &select_allfds);
    if (types&select_read)
        FD_SET(path, &select_readfds);
    if (types&select_write)
        FD_SET(path, &select_writefds);
    if (types&select_except)
        FD_SET(path, &select_exceptfds);

    if (select_tasks==0) {
        select_tasks=descr;
    } else {
        descr->next=select_tasks;
        select_tasks->prev=descr;
        select_tasks=descr;
    }
    select_tasks_changed=1;
    numselectpathes=-1;
    for (help=select_tasks; help!=0; help=help->next) {
        if (help->fds>numselectpathes)
            numselectpathes=help->fds;
    }
    numselectpathes++;

    if (!numselecttasks) {
        union callbackdata data;
        data.p=(void*)0;
        if ((select_descr=sched_insert_block_task(select_waitproc,
                select_pollproc,
                data, 1, 0, "select_task"))==0) {
            printf("sched_insert_select_task(): sched_insert_block_task "
                    "schlug fehl\n");
            return 0;
        }
    }
    numselecttasks++;
    #ifdef SELECT_STATIST
    descr->statist.active=active;
    descr->statist.enabled=0;
    descr->statist.disabled=0;
    gettimeofday(&descr->statist.tv, 0);
    #endif
    return descr;
}
#endif /* OSK */
/*****************************************************************************/
#ifndef OSK
void sched_delete_select_task(struct seltaskdescr* descr)
{
int path;
struct seltaskdescr *help, *st;
T(sched_delete_select_task)

if (descr==0) return; /* emergency exit */
path=descr->fds;
    /*
    printf("sched_delete_select_task(%p, path=%d, name=%s)\n", descr, path,
            descr->name);
    */
st=select_tasks;
while (st && (st!=descr)) st=st->next;
if (!st)
  {
  printf("sched_delete_select_task: remove ghost?\n");
  panic();
  }
FD_CLR(path, &select_allfds);
FD_CLR(path, &select_readfds);
FD_CLR(path, &select_writefds);
FD_CLR(path, &select_exceptfds);

if (descr->prev!=0)
  descr->prev->next=descr->next;
else
  select_tasks=descr->next;
if (descr->next!=0) descr->next->prev=descr->prev;
free(descr->name);
free(descr);
select_tasks_changed=1;

numselectpathes=-1;
for (help=select_tasks; help!=0; help=help->next)
    if (help->fds>numselectpathes) numselectpathes=help->fds;
numselectpathes++;

numselecttasks--;
if (!numselecttasks) sched_remove_task(select_descr);
}
#endif /* OSK */
/*****************************************************************************/
#ifndef OSK
void sched_select_task_set(struct seltaskdescr* descr, enum select_types types
#ifdef SELECT_STATIST
    ,int active
#endif
    )
{
    T(sched_select_task_set)
    descr->types=types;
    FD_CLR(descr->fds, &select_readfds);
    FD_CLR(descr->fds, &select_writefds);
    FD_CLR(descr->fds, &select_exceptfds);
    if (types&select_read)
        FD_SET(descr->fds, &select_readfds);
    if (types&select_write)
        FD_SET(descr->fds, &select_writefds);
    if (types&select_except)
        FD_SET(descr->fds, &select_exceptfds);
#ifdef SELECT_STATIST
    if (active!=descr->statist.active) {
        struct timeval now;
        float diff;
        gettimeofday(&now, 0);
       /*
        * descr->statist.active?descr->statist.enabled:descr->statist.disabled+=
        * now.tv_sec-descr->statist.tv.tv_sec
        * +(now.tv_usec-descr->statist.tv.tv_usec)/1000000.;
        */
        diff=now.tv_sec-descr->statist.tv.tv_sec
            +(now.tv_usec-descr->statist.tv.tv_usec)/1000000.;
        if (descr->statist.active)
            descr->statist.enabled+=diff;
        else
            descr->statist.disabled+=diff;
        descr->statist.active=active;
        descr->statist.tv=now;
    }
#endif
}
#endif /* OSK */
/*****************************************************************************/
#ifndef OSK
enum select_types
sched_select_task_get(struct seltaskdescr* descr)
{
    T(sched_select_task_get)
    return descr->types;
}
#endif /* OSK */
/*****************************************************************************/
#ifndef OSK
#ifdef SELECT_STATIST
void sched_select_task_get_statist(struct seltaskdescr* descr, double* enabled,
    double* disabled, int clear)
{
    struct timeval now;
    T(sched_select_task_get_statist)
    gettimeofday(&now, 0);
    if (descr->statist.active)
        descr->statist.enabled+=
                now.tv_sec-descr->statist.tv.tv_sec
                +(now.tv_usec-descr->statist.tv.tv_usec)/1000000.;
    else
        descr->statist.disabled+=
                now.tv_sec-descr->statist.tv.tv_sec
                +(now.tv_usec-descr->statist.tv.tv_usec)/1000000.;

    descr->statist.tv=now;
    *enabled=descr->statist.enabled;
    *disabled=descr->statist.disabled;
    if (clear) {
        descr->statist.enabled=0.;
        descr->statist.disabled=0.;
    }
}

void sched_select_get_statist(double* runtime, double* idle, int clear)
{
    struct timeval now;
    T(sched_select_task_get_statist)
    gettimeofday(&now, 0);
    *idle=sched_select_statist.idle;
    *runtime=now.tv_sec-sched_select_statist.tv.tv_sec
                +(now.tv_usec-sched_select_statist.tv.tv_usec)/1000000.;
    if (clear) {
        sched_select_statist.tv=now;
        sched_select_statist.idle=0.;
    }
}
#endif
#endif /* OSK */
/*****************************************************************************/
#ifndef OSK
void sched_select_task_suspend(struct seltaskdescr* descr)
{
    T(sched_select_task_suspend)
    if (descr==0) return; /* emergency exit */
    FD_CLR(descr->fds, &select_readfds);
    FD_CLR(descr->fds, &select_writefds);
    FD_CLR(descr->fds, &select_exceptfds);
#ifdef SELECT_STATIST
    if (descr->statist.active) {
        struct timeval now;
        gettimeofday(&now, 0);
        descr->statist.enabled+=
                now.tv_sec-descr->statist.tv.tv_sec
                +(now.tv_usec-descr->statist.tv.tv_usec)/1000000.;
        descr->statist.active=0;
        descr->statist.tv=now;
    }
#endif
}
#endif /* OSK */
/*****************************************************************************/
#ifndef OSK
void sched_select_task_reactivate(struct seltaskdescr* descr)
{
    T(sched_select_task_reactivate)

    if (descr->types&select_read) FD_SET(descr->fds, &select_readfds);
    if (descr->types&select_write) FD_SET(descr->fds, &select_writefds);
    if (descr->types&select_except) FD_SET(descr->fds, &select_exceptfds);
#ifdef SELECT_STATIST
    if (!descr->statist.active) {
        struct timeval now;
        gettimeofday(&now, 0);
        descr->statist.disabled+=
                now.tv_sec-descr->statist.tv.tv_sec
                +(now.tv_usec-descr->statist.tv.tv_usec)/1000000.;
        descr->statist.active=1;
        descr->statist.tv=now;
    }
#endif
}
#endif /* OSK */
/*****************************************************************************/
static void
do_remove(union callbackdata data)
{
    struct taskdescr* t=(struct taskdescr*)data.p;
    sigresc m;
    T(do_remove)
    disable_breaks(&m);

    switch(t->typ) {
    case sched_polling:
    case sched_blocking:
        if (current==t)
            current=current->p_prev;

        t->p_next->p_prev=t->p_prev;
        t->p_prev->p_next=t->p_next;
        free(t);
        numtasks--;

        if (!numtasks) {
            if (current!=t) {
                printf("Fehler: current!=last");
                exit(0);
            }
            current= &idletask;
        }

        if (blocking) {
            if (numtasks) {
                printf("Fehler: !=1 war blocking");
                exit(0);
            }
            blocking=0;
        }

        if (numtasks==1) {
            if (current->typ==sched_blocking) {
                current->proc=current->p_blproc;
                blocking=current;
            }
        }
        break;
    case sched_timer:
        remove_timer(&t->t_timer);
        free(t);
        break;
    default:
        printf("remove wrong task\n");
        exit(0);
    }

    enable_breaks(&m);
}
/*****************************************************************************/
void sched_remove_task(struct taskdescr* descr)
{
T(sched_remove_task)
if (descr==0) return; /* emergency exit */
  descr->p_remove_pending=1;
  descr->pl.proc=do_remove;
  descr->pl.arg.p=descr;
  insert_breakaction(&descr->pl);
  breakflag=1;
  /* wakeup aus waiter? */
}

/*****************************************************************************/
void sched_exec_once(union callbackdata data)
{
    T(sched_exec_once)
    insert_breakaction((struct plistentry*)data.p);
    breakflag=1;
    /* wakeup aus waiter? */
}

#if 0
static void do_exec_later(descr)
struct taskdescr *descr;
{
    insert_breakaction(descr->proc,0);
    breakflag=1;
    /* wakeup aus waiter? */
    free(descr);
}

struct taskdescr*
exec_later(void(*proc)(union callbackdata), int timeout, char* name)
{
    struct taskdescr *mytask;
    mytask=(struct taskdescr*)malloc(sizeof(struct taskdescr));
    if (!mytask)
        return 0;
    mytask->typ=sched_timer;
    mytask->proc=proc;
    mytask->name=name;
    install_timeout(timeout,do_exec_later,mytask,name,&mytask->t_timer);
    return mytask;
}
#endif
/*****************************************************************************/
struct taskdescr*
sched_exec_periodic(void(*proc)(union callbackdata),
        union callbackdata arg, int zyklus, char* name)
{
    struct taskdescr *mytask;
    union callbackdata data;

    T(sched_exec_periodic)
    mytask=(struct taskdescr*)malloc(sizeof(struct taskdescr));
    if (!mytask)
        return(0);
    mytask->typ=sched_timer;
    mytask->proc=mytask->pl.proc=proc;
    mytask->arg=mytask->pl.arg=arg;
    mytask->name=name;
    data.p=&mytask->pl;
    if (install_periodic(zyklus,sched_exec_once,data,name,&mytask->t_timer)<0) {
        free(mytask);
        return 0;
    }
    return mytask;
}
/*****************************************************************************/
int sched_adjust_prio(char* name, int p1, int p2)
{
    sigresc m;
    static struct taskdescr *h;

    T(sched_adjust_prio)
    disable_breaks(&m);

    h=current;
    do {
        if(!strcmp(name,h->name)) {
            h->p_prio1=p1;
            h->p_prio2=p2;
            h->p_p2=0;
            enable_breaks(&m);
            return 0;
        }
        h=h->p_next;
    } while (h!=current);

    enable_breaks(&m);
    return -1;
}
/*****************************************************************************/
int sched_adjust_prio_t(struct taskdescr* t, int p1, int p2)
{
  sigresc m;
T(sched_adjust_prio_t)
  disable_breaks(&m);

  t->p_prio1=p1;
  t->p_prio2=p2;
  t->p_p2=0;

  enable_breaks(&m);
  return(0);
}
/*****************************************************************************/
/*****************************************************************************/
