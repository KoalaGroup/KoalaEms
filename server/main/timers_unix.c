/*
 * main/timers_unix.c
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: timers_unix.c,v 1.9 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <rcs_ids.h>
#include "signals.h"
#include "timers.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

RCS_REGISTER(cvsid, "main")

static struct timerentry *tqueue, *rmqueue, **lastrm;

static void
insert_queue(struct timerentry* e) /* nicht unterbrechen! */
{
    struct timerentry **q;

    q= &tqueue;
    while ((*q)&&(timercmp(&(*q)->t,&e->t,<)))
        q= &(*q)->next;
    e->next= *q;
    *q=e;
}

static void
check_queue(struct timeval* t) /* nicht unterbrechen! */
{
    while ((tqueue) && (!timercmp(&tqueue->t,t,>))) {
        struct timerentry *help;
        help=tqueue;
        tqueue=tqueue->next;
        (help->proc)(help->arg);
        if (help->periodic) {
            help->t.tv_sec+=help->periodic/TIMER_RES;
            help->t.tv_usec+=(help->periodic%TIMER_RES)*(1000000/TIMER_RES);
            if (help->t.tv_usec>1000000) {
	        help->t.tv_usec-=1000000;
	        help->t.tv_sec++;
            }
            insert_queue(help);
        } else {
            help->next= *lastrm;
            *lastrm=help;
        }
    }
}

static void
clean_queue(void) /* nicht von sighandler aufrufen, nicht unterbrechen! */
{
  while(rmqueue){
    struct timerentry *help;
    help=rmqueue;
    rmqueue=rmqueue->next;
    free(help->name);
    free(help);
  }
  lastrm= &rmqueue;
}

static void
set_new_itimer(struct timeval* t)
{
  struct itimerval val;

  timerclear(&val.it_interval);
  val.it_value.tv_sec=tqueue->t.tv_sec-t->tv_sec;
  val.it_value.tv_usec=tqueue->t.tv_usec-t->tv_usec;
  if(val.it_value.tv_usec<0){
    val.it_value.tv_usec+=1000000;
    val.it_value.tv_sec--;
  }

  setitimer(ITIMER_REAL,&val,0);
}

/*
#include <dpmi.h>

volatile int tics = 0;

timer_handler()
{
  tics++;
}

int main()
{
  _go32_dpmi_seginfo old_handler, new_handler;

  printf("grabbing timer interrupt\n");
  _go32_dpmi_get_protected_mode_interrupt_vector(8, &old_handler);

  new_handler.pm_offset = (int)tic_handler;
  new_handler.pm_selector = _go32_my_cs();
  _go32_dpmi_chain_protected_mode_interrupt_vector(8, &new_handler);

  getkey();

  printf("releasing timer interrupt\n");
  _go32_dpmi_set_protected_mode_interrupt_vector(8, &old_handler);

  return 0;
}
*/
/*
 int ints_were_enabled;

 ints_were_enabled = disable();
 . . . do some stuff . . .
 if (ints_were_enabled)
 enable();
*/

static int
install_timer(int timeout, int periodic, void(*proc)(union callbackdata),
        union callbackdata arg, char* name, struct timerentry** resc)
{
    struct timeval t;
    struct timezone z;
    struct timerentry* this;
    sigresc m;

    disable_breaks(&m);

    gettimeofday(&t,&z);
    check_queue(&t);
    clean_queue();

    if (!(this=(struct timerentry*)malloc(sizeof(struct timerentry))))
        return -1;
    this->t.tv_sec=t.tv_sec+timeout/TIMER_RES;
    this->t.tv_usec=t.tv_usec+(timeout%TIMER_RES)*(1000000/TIMER_RES);
    if (this->t.tv_usec>1000000) {
        this->t.tv_usec-=1000000;
        this->t.tv_sec++;
    }
    this->proc=proc;
    this->arg=arg;
    this->periodic=periodic;
    this->name=strdup(name);
    *resc=this;
    insert_queue(this);

    set_new_itimer(&t);

    enable_breaks(&m);
    return 0;
}

int
install_timeout(int timeout, void(*proc)(union callbackdata),
        union callbackdata arg, char* name, struct timerentry** resc)
{
    return install_timer(timeout, 0, proc, arg, name, resc);
}

int
install_periodic(int zyklus, void(*proc)(union callbackdata),
        union callbackdata arg, char* name, timeoutresc* resc)
{
    return install_timer(zyklus, zyklus, proc, arg, name, resc);
}

int
remove_timer(timeoutresc* resc)
{
    struct timerentry **q;
    struct timeval t;
    struct timezone z;
    sigresc m;
    int res;

    disable_breaks(&m);

    gettimeofday(&t, &z);
    check_queue(&t);
    clean_queue();

    q= &tqueue;
    while ((*q)&&(*q!=*resc))
        q= &(*q)->next;
    if (*q) {
        struct timerentry *help;
        help= *q;
        *q=(*q)->next;
        free(help->name);
        free(help);
        res=0;
    } else {
        res=1;
    }

    if (tqueue)
        set_new_itimer(&t);

    enable_breaks(&m);
    return(res);
}

static void
timer_handler(__attribute__((unused)) int sig)
{
  struct timeval t;
  struct timezone z;

  gettimeofday(&t,&z);
  check_queue(&t);

  if(tqueue)set_new_itimer(&t);
}

int
init_timers(void)
{
  tqueue=rmqueue=0;
  lastrm= &rmqueue;
  install_timerhandler(timer_handler);
  return(0);
}

void
done_timers(void)
{
  clean_queue();
  /* more cleanup... */
}

void
print_timers(void)
{
  sigresc m;
  struct timeval t;
  struct timezone z;
  struct timerentry *h;

  disable_breaks(&m);

  gettimeofday(&t,&z);

  printf("Timers:\n");

  h=tqueue;
  while(h){
    double rem;
    rem=(double)(h->t.tv_sec-t.tv_sec)+
      (double)(h->t.tv_usec-t.tv_usec)/1000000.0;
    printf(" %s: %.3f/%d\n",h->name,rem,h->periodic);
    h=h->next;
  }

  enable_breaks(&m);
}
