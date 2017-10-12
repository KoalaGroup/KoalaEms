/*
 * dataout/cluster/do_stream_sock.c
 * 
 * created      27.10.97
 * 17.01.1999 PW: predefined_costumers used in advertise...
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: do_stream_sock.c,v 1.6 2007/11/03 16:00:23 wuestner Exp $";

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sconf.h>
#include <config.h>
#include <debug.h>
#include "do_cluster.h"
#include "../../main/scheduler.h"
#include "../../objects/domain/dom_dataout.h"
#include "macros.h"

#ifdef DMALLOC
#include dmalloc.h
#endif

#define swap_int(a) ( ((a) << 24) | \
		      (((a) << 8) & 0x00ff0000) | \
		      (((a) >> 8) & 0x0000ff00) | \
	((unsigned int)(a) >>24) )

/*****************************************************************************/
#if (MAX_DATAOUT>1)
static void do_cluster_write_error(int do_idx)
#else
static void do_cluster_write_error()
#endif
{
DoRunStatus old_status=dataout_clX(do_idx).do_status;
T(dataout/cluster/do_cluster.c:do_cluster_write_error)
printf("do_cluster_write_error\n");
dataout_clX(do_idx).do_status=Do_error;
sched_delete_select_task(dataout_clX(do_idx).seltask);
dataout_clX(do_idx).seltask=0;
dataout_clX(do_idx).procs.patherror(IDXargk(do_idx) -1);
if (old_status==Do_flushing)
#if (MAX_DATAOUT>1)
  check_do_done(do_idx);
#else
  check_do_done(0);
#endif
}
/*****************************************************************************/
static int new_evnr(int do_idx, do_stream_data* dat)
{
/* neue Eventnummer bestimmen */
int wieviel=dataout[do_idx].wieviel;
printf("new_evnr()\n");
if (dat->evnr<0) /* noch kein altes Event gehabt */
  {
  Cluster* cl=clusters;
  printf("suche ersten Cluster\n");
  while (cl && (cl->costumers[do_idx]!=1))
    {
    printf("cl=%p\n", cl);
    cl=cl->next;
    }
  if (cl==0)
    {
    printf("habe aber keinen\n");
    return -1;
    }
  if (wieviel==0)
    dat->evnr=cl->firstevent;
  else if (wieviel==-1)
    {
    printf("do_stream_sock_write: wieviel=%d\n", wieviel);
    panic();
    }
  else if (wieviel<0)
    dat->evnr=cl->firstevent+wieviel-cl->firstevent%wieviel;
  else /* if (wieviel>0) */
    {
    printf("do_stream_sock_write: wieviel=%d\n", wieviel);
    panic();
    }
  }
else
  {
  printf("alte ev_nr=%d\n", dat->evnr);
  if (wieviel==0)
    dat->evnr++;
  else if (wieviel==-1)
    {
    printf("do_stream_sock_write: wieviel=%d\n", wieviel);
    panic();
    }
  else if (wieviel<0)
    dat->evnr+=wieviel;
  else /* if (wieviel>0) */
    {
    printf("do_stream_sock_write: wieviel=%d\n", wieviel);
    panic();
    }
  }
printf("new_evnr=%d\n", dat->evnr);
return 0;
}
/*****************************************************************************/
static int start_subevents_suchen(int do_idx, do_stream_data* dat)
{
int geloescht, loop=0;
printf("start_subevents_suchen()\n");
do
  {
  Cluster* cl=clusters;
  int n, evmin;
  printf("start_subevents_suchen: loop %d\n", ++loop);
  while (cl) /* alte Cluster ausmerzen */
    {
    Cluster* ncl=cl->next;
    if ((cl->costumers[do_idx]==1) && (dat->evnr>=cl->firstevent+cl->numevents))
      {
      printf("loesche cluster; ved=%d; first ev=%d\n", cl->ved_id, cl->firstevent);
      cl->costumers[do_idx]=0;
      if (!--cl->numcostumers) clusters_recycle(cl);
      }
    cl=ncl;
    }
  cl=clusters;
  for (n=0; n<vednums.num_veds; n++)
    {
    if (!dat->sevs[n].cluster)
      {
      int weiter, ved_id, evn;
      printf("suche fuer slot %d\n", n);
      do
        {
        int o;
        weiter=0;
        ved_id=cl->ved_id;
        for (o=0; (o<vednums.num_veds) && !weiter; o++)
          if (dat->sevs[n].cluster && (dat->sevs[n].ved_id==ved_id)) weiter=1;
        if (weiter) cl=cl->next;
        }
      while (weiter && cl);
      if (!cl)
        {
        printf("nix gefunden\n");
        return -1; /* kein brauchbares Cluster gefunden */
        }
      {
      int i;
      printf("fand ved %d, ev %d; cl=%p; data=%p\n",
          cl->ved_id, cl->firstevent, cl, cl->data);
      for (i=0; i<20; i++) printf("%d  ", cl->data[i]);
      printf("\n");
      }
      dat->sevs[n].cluster=cl;
      dat->sevs[n].valid=1;
      cl->costumers[do_idx]=2;
      dat->sevs[n].sevptr=cl->data+8+cl->optsize;
      {
      int i;
      printf("optsize=%d\n", cl->optsize);
      for (i=0; i<20; i++) printf("%d  ", dat->sevs[n].sevptr[i]);
      printf("\n");
      }
      if (cl->swapped)
        dat->sevs[n].sevsize=swap_int(dat->sevs[n].sevptr[0])+1;
      else
        dat->sevs[n].sevsize=dat->sevs[n].sevptr[0]+1;
      for (evn=cl->firstevent; evn<dat->evnr; evn++)
        {
        dat->sevs[n].sevptr+=dat->sevs[n].sevsize;
        if (cl->swapped)
          dat->sevs[n].sevsize=swap_int(dat->sevs[n].sevptr[0])+1;
        else
          dat->sevs[n].sevsize=dat->sevs[n].sevptr[0]+1;
        }
      }
    else
      {
      Cluster*c=dat->sevs[n].cluster;
      printf("slot %d: besetzt durch ved %d; ev %d\n", n, c->ved_id, c->firstevent);
      }
    }
  /* hier sollte ein Cluster fuer jedes VED verfuegbar sein */
  /* kann aber sein, dass wir nicht alle braucen koennen */
  evmin=0;
  for (n=0; n<vednums.num_veds; n++)
    {
    if (dat->sevs[n].cluster)
      if (dat->sevs[n].cluster->firstevent>evmin)
        evmin=dat->sevs[n].cluster->firstevent;
    }
  /* evmin ist die kleinste "machbare" Eventnummer */
  printf("kleinstes moegliches event: %d\n", evmin);
  geloescht=0;
  if (evmin>dat->evnr)
    {
    int wieviel=dataout[do_idx].wieviel;
    printf("start_subevents_suchen: evmin=%d, evnr=%d\n", evmin, dat->evnr);
    if (wieviel==0)
      dat->evnr=evmin;
    else if (wieviel==-1)
      {
      printf("start_subevents_suchen: wieviel=%d\n", wieviel);
      panic();
      }
    else if (wieviel<0)
      dat->evnr=evmin+wieviel-evmin%wieviel;
    else /* if (wieviel>0) */
      {
      printf("start_subevents_suchen: wieviel=%d\n", wieviel);
      panic();
      }
    printf("  evnr --> %d\n", dat->evnr);
    for (n=0; n<vednums.num_veds; n++)
      {
      if (dat->sevs[n].cluster)
        {
        Cluster* cl=dat->sevs[n].cluster;
        if (dat->evnr>=cl->firstevent+cl->numevents)
          {
          printf("loesche cluster fuer n=%d\n", n);
          geloescht++;
          cl->costumers[do_idx]=0;
          if (!--cl->numcostumers) clusters_recycle(cl);
          dat->sevs[n].cluster=0;
          dat->sevs[n].valid=0;
          }
        }
      }
    }
  }
while (geloescht>0);
return 0;
}
/*****************************************************************************/
static int subevents_suchen(int do_idx, do_stream_data* dat)
{
int n;
printf("subevents_suchen()\n");
for (n=0; n<vednums.num_veds; n++)
  {
  /* nur wenn Eintrag noch nicht gueltig */
  if (!dat->sevs[n].valid)
    {
    /* alten Eintrag pruefen */
    Cluster* cl;
    printf("teste slot %d\n", n);
    if ((cl=dat->sevs[n].cluster))
      {
      if (dat->evnr>=cl->firstevent+cl->numevents)
        { /* Event kann nicht in diesem Cluster sein */
        printf("loesche cluster; ved=%d; first ev=%d\n", cl->ved_id, cl->firstevent);
        cl->costumers[do_idx]=0;
        if (!--cl->numcostumers) clusters_recycle(cl);
        dat->sevs[n].cluster=0;
        }
      else
        { /* Cluster ist noch brauchbar */
        printf("noch brauchbar: ved=%d; first ev=%d\n", cl->ved_id, cl->firstevent);
        dat->sevs[n].valid=1;
        dat->sevs[n].sevptr+=dat->sevs[n].sevsize;
        if (cl->swapped)
          dat->sevs[n].sevsize=swap_int(dat->sevs[n].sevptr[0])+1;
        else
          dat->sevs[n].sevsize=dat->sevs[n].sevptr[0]+1;
        }
      }
    /* kein gueltiger Eintrag (mehr) enthalten? */
    if (!dat->sevs[n].cluster)
      {
      int weiter;
      Cluster* cl=clusters;
      printf("brauche neuen cluster fuer slot %d\n", n);
      do
        {
        Cluster* ncl=cl->next;
        if (cl->costumers[do_idx]==1)
          {
          if (dat->evnr>=cl->firstevent+cl->numevents) /* altes Event */
            {
            printf("fand alten cluster: ved=%d; first ev=%d\n", cl->ved_id, cl->firstevent);
            cl->costumers[do_idx]=0;
            if (!--cl->numcostumers) clusters_recycle(cl);
            weiter=1;
            }
          else if (dat->evnr<cl->firstevent) /* Event zu neu */
            weiter=1;
          else
            { /* Cluster gefunden */
            int evn;
            printf("fand ved %d, ev %d\n", cl->ved_id, cl->firstevent);
            dat->sevs[n].cluster=cl;
            dat->sevs[n].valid=1;
            cl->costumers[do_idx]=2;
            dat->sevs[n].sevptr=cl->data+8+cl->optsize;
            if (cl->swapped)
              dat->sevs[n].sevsize=swap_int(dat->sevs[n].sevptr[0])+1;
            else
              dat->sevs[n].sevsize=dat->sevs[n].sevptr[0]+1;
            for (evn=cl->firstevent; evn<dat->evnr; evn++)
              {
              dat->sevs[n].sevptr+=dat->sevs[n].sevsize;
              if (cl->swapped)
                dat->sevs[n].sevsize=swap_int(dat->sevs[n].sevptr[0])+1;
              else
                dat->sevs[n].sevsize=dat->sevs[n].sevptr[0]+1;
              }
            weiter=0;
            }
          }
        else
          weiter=1;
        cl=ncl;
        }
      while (weiter && (cl!=0));
      if (weiter)
        {
        printf("fand nix\n");
        return -1; /* kein Cluster gefunden */
        }
      }
    }
  }
printf("habe alles\n");
return 0;
}
/*****************************************************************************/
static int make_event(int do_idx, do_stream_data* dat)
{
int evsize, sevnum, trigno, must_swap, *dst, n;
printf("make_event()\n");
/* eventlaenge bestimmen */
evsize=4; /* Header */
sevnum=0;
trigno=dat->sevs[0].sevptr[2];
printf("dat->evnr=%d\n", dat->evnr);
for (n=0; n<vednums.num_veds; n++)
  {
  int swapped=dat->sevs[n].cluster->swapped;
  int evnr;
  printf("cluster is%s swapped\n", swapped?"":"not ");
  if (swapped)
    {
    evnr=swap_int(dat->sevs[n].sevptr[1]);
    evsize+=swap_int(dat->sevs[n].sevptr[0])-3;
    sevnum+=swap_int(dat->sevs[n].sevptr[3]);
    }
  else
    {
    evnr=dat->sevs[n].sevptr[1];
    evsize+=dat->sevs[n].sevptr[0]-3;
    sevnum+=dat->sevs[n].sevptr[3];
    }
  printf("n=%d; cl=%p; data=%p, ptr=%p\n",
      n, dat->sevs[n].cluster, dat->sevs[n].cluster->data, dat->sevs[n].sevptr);
  if (evnr!=dat->evnr)
    {
    printf("slot %d: evnr=%d; dat->evnr=%d\n", n, evnr, dat->evnr);
    panic();
    }
  }
printf("%d subevents; size=%d\n", sevnum, evsize);
if (dat->eventsize<evsize)
  {
  free(dat->event);
  dat->event=(int*)malloc(evsize*4);
  if (dat->event==0)
    {
    printf("do_stream_sock.c::make_event(): malloc(%d*4): %s\n", evsize, strerror(errno));
    dat->eventsize=0;
    return -1;
    }
  dat->eventsize=evsize;
  }
dst=dat->event;
if (dataout_clX(do_idx).endien==endien_native)
  must_swap=dat->sevs[0].cluster->swapped;
else
#ifdef WORDS_BIGENDIAN
  must_swap=dataout_clX(do_idx).endien==endien_little;
#else
  must_swap=dataout_clX(do_idx).endien==endien_big;
#endif
if (must_swap)
  {
  *dst++=swap_int(evsize-1);
  *dst++=swap_int(dat->evnr);
  *dst++=swap_int(trigno);
  *dst++=swap_int(sevnum);
  }
else
  {
  *dst++=evsize-1;
  *dst++=dat->evnr;
  *dst++=trigno;
  *dst++=sevnum;
  }
for (n=0; n<vednums.num_veds; n++)
  {
  int* src=dat->sevs[n].sevptr+4;
  if (must_swap==dat->sevs[n].cluster->swapped)
    { /* schon richtig 'rum */
    int i;
    printf("n=%d; sevsize=%d\n", n, dat->sevs[n].sevsize);
    for (i=dat->sevs[n].sevsize-4; i; i--) *dst++=*src++;
    }
  else
    { /* alles verdreht */
    int i;
    printf("swapping; n=%d; sevsize=%d\n", n, dat->sevs[n].sevsize);
    for (i=dat->sevs[n].sevsize-4; i; i--) {*dst++=swap_int(*src); src++;}
    }
  }
dat->bytes=evsize*4;
return 0;
}
/*****************************************************************************/
static void do_stream_sock_write(int path, select_types selected,
    callbackdata data)
{
#if MAX_DATAOUT>1
int do_idx=data.i;
#else
int do_idx=0;
#endif
int res;
do_stream_data* dat=&dataout_clX(do_idx).d.s_data;

T(dataout/cluster/do_stream_sock.c::do_stream_sock_write)

if (dat->bytes==0)
  {
  int n;
  if (!dat->evnr_ok)
    {
    if (new_evnr(do_idx, dat)!=0)
      {
      printf("write: new_evnr schlug fehl\n");
      sched_select_task_suspend(dataout_clX(do_idx).seltask);
      dataout_clX(do_idx).suspended=1;
      return;
      }
    dat->evnr_ok=1;
    printf("write: ev_nr ok\n");
    for (n=0; n<vednums.num_veds; n++) dat->sevs[n].valid=0;
    }
  if (dat->start)
    {
    if (start_subevents_suchen(do_idx, dat)<0)
      {
      printf("write: start_subevents_suchen schlug fehl\n");
      sched_select_task_suspend(dataout_clX(do_idx).seltask);
      dataout_clX(do_idx).suspended=1;
      return;
      }
    else
      dat->start=0;
    }
  else
    {
    if (subevents_suchen(do_idx, dat)<0)
      {
      printf("write: subevents_suchen schlug fehl\n");
      sched_select_task_suspend(dataout_clX(do_idx).seltask);
      dataout_clX(do_idx).suspended=1;
      return;
      }
    }
  /* wir haben alle Subevents, bauen wir nun ein Event */
  if (make_event(do_idx, dat)!=0)
    {
    fatal_readout_error();
    do_cluster_write_error(IDXarg(do_idx));
    return;
    }
  }
printf("call write(path=%d, dat=%p, bytes=%d)\n", path,
    (char*)(dat->event)+dat->idx, dat->bytes);
res=write(path, (char*)(dat->event)+dat->idx, dat->bytes);
if (res<=0) /* Fehler? */
  {
  if ((errno!=EINTR) && (errno!=EWOULDBLOCK))
    {
    dataout_clX(do_idx).errorcode=errno;
    printf("call fatal_readout_error(), res=%d; errno=%s; bytes=%d\n",
        res, strerror(errno), dat->bytes);
    fatal_readout_error();
    do_cluster_write_error(IDXarg(do_idx));
    }
  return;
  }
dat->idx+=res;
dat->bytes-=res;
if (dat->bytes==0) dat->evnr_ok=0;
}
/*****************************************************************************/
static void do_stream_sock_connect(int path, select_types selected,
    callbackdata data)
{
#if MAX_DATAOUT>1
int do_idx=data.i;
#endif
do_special_sock* special=&dataout_clX(do_idx).s.sock_data;
T(dataout/cluster/do_stream_sock.c:do_stream_sock_connect)
printf("do_stream_sock_connect\n");
sched_delete_select_task(special->connecttask);
special->connecttask=0;

if (selected&select_except)
  {
  printf("Error in do_stream_sock_connect(path=%d; do_idx=%d)\n", path, data.i);
  dataout_clX(do_idx).errorcode=-1;
  dataout_clX(do_idx).do_status=Do_error;
  }
else
  {
  int val, ilen;
  ilen=sizeof(int);
  if (getsockopt(path, SOL_SOCKET, SO_ERROR, (char*)&val, &ilen)<0)
    {
    dataout_clX(do_idx).errorcode=errno;
    dataout_clX(do_idx).do_status=Do_error;
    printf("Error in do_stream_sock_connect(do_idx=%d)::getsockopt: %s\n", data.i,
        strerror(errno));
    }
  else
    {
    if (val!=0)
      {
      dataout_clX(do_idx).errorcode=val;
      dataout_clX(do_idx).do_status=Do_error;
      printf("Bad status in do_stream_sock_connect(path=%d; do_idx=%d): %s\n",
          path, data.i, strerror(val));
      }
    else /* ok */
      {
      char name[100];
      printf("dataout %d connected\n", data.i);
      special->connected=1;
      snprintf(name, 100, "do_%02d_stream_sock_write", do_idx);
      dataout_clX(do_idx).seltask=sched_insert_select_task(do_stream_sock_write,
          data, name, path, select_write|select_except);
      if (dataout_clX(do_idx).seltask==0)
        {
        dataout_clX(do_idx).errorcode=-1;
        dataout_clX(do_idx).do_status=Do_error;
        printf("do_stream_sock_connect: can't create task.\n");
        }
      else
       if (vednums.num_veds<0)
         {
         sched_select_task_suspend(dataout_clX(do_idx).seltask);
         }
      }
    }
  }
}
/*****************************************************************************/
static void do_stream_sock_accept(int path, select_types selected, callbackdata data)
{
#if MAX_DATAOUT>1
int do_idx=data.i;
#endif
do_special_sock* special=&dataout_clX(do_idx).s.sock_data;
int newpath;

T(cluster/do_stream_sock.c:do_sock_accept)
printf("do_stream_sock_accept\n");
if (selected&select_except)
  {
  dataout_clX(do_idx).errorcode=-1;
  close(path);
  special->s_path=-1;
  sched_delete_select_task(special->connecttask);
  special->connecttask=0;
  dataout_clX(do_idx).do_status=Do_error;
  printf("Error in do_sock_accept.\n");
  }
else
  {
  struct sockaddr addr;
  int size;
  newpath=accept(path, &addr, &size);
  if (newpath<0)
    {
    printf("do_sock_accept(do_idx=%d): error: %s\n", data.i, strerror(errno));
    dataout_clX(do_idx).errorcode=-1;
    }
  else /* accept ok */
    {
    if (special->connected)
      {
      printf("do_sock_accept(do_idx=%d): already used.\n");
      close(newpath);
      }
    else
      {
      printf("dataout %d accepted\n", data.i);
      if (fcntl(newpath, F_SETFD, FD_CLOEXEC)<0)
        {
        printf("do_stream_sock.c: fcntl(newpath, FD_CLOEXEC): %s\n", strerror(errno));
        }
      special->n_path=newpath;
      if (fcntl(special->n_path, F_SETFL, O_NDELAY)<0)
        {
        dataout_clX(do_idx).errorcode=-1;
        dataout_clX(do_idx).do_status=Do_error;
        printf("do_sock_accept(do_idx=%d): can't set to O_NDELAY: %s\n", data.i,
            strerror(errno));
        }
      else /* fcntl ok */
        {
        if (dataout_clX(do_idx).do_status=Do_running)
          {
          char name[100];
          snprintf(name, 100, "do_%02d_stream_sock_write", do_idx);
          dataout_clX(do_idx).seltask=
              sched_insert_select_task(do_stream_sock_write,
                data, name, special->n_path,
                select_write|select_except);
          if (dataout_clX(do_idx).seltask==0)
            {
            dataout_clX(do_idx).errorcode=-1;
            dataout_clX(do_idx).do_status=Do_error;
            printf("do_sock_accept(do_idx=%d): can't create task.\n", data.i);
            }
          else /* task ok */
            {
            /* OK */
            special->connected=1;
            if (vednums.num_veds<0)
              {
              sched_select_task_suspend(dataout_clX(do_idx).seltask);
              }
            }
          }
        else /* !Do_running */
          special->connected=1;
        }
      }
    }
  }
}
/*****************************************************************************/
static void do_stream_sock_veds(int do_idx, int n)
{
do_stream_data* dat=&dataout_clX(do_idx).d.s_data;
T(dataout/cluster/do_stream_sock.c::do_stream_sock_veds)
printf("do_stream_sock_veds(idx=%d, veds=%d)\n", do_idx, n);
/* dat->sevs muss mit 0 initialisiert sein */
dat->sevs=(struct do_stream_sev*)calloc(n, sizeof(struct do_stream_sev));
if (dataout_clX(do_idx).seltask)
  {
  sched_select_task_reactivate(dataout_clX(do_idx).seltask);
  }
}
/*****************************************************************************/
static errcode do_stream_sock_start(int do_idx)
{
do_special_sock* special=&dataout_clX(do_idx).s.sock_data;
do_stream_data* dat=&dataout_clX(do_idx).d.s_data;
callbackdata data;
int n;
data.i=do_idx;

T(cluster/do_stream_sock.c:do_stream_sock_start)
printf("do_stream_sock_start\n");
if (!special->server)
  {
  struct sockaddr_in sa;
  int r;
  
  special->n_path=socket(AF_INET, SOCK_STREAM, 0);
  if (special->n_path<0)
    {
    *outptr++=errno;
    dataout_clX(do_idx).do_status=Do_error;
    printf("do_stream_sock_start(do_idx=%d): can't open socket: %s\n", do_idx,
        strerror(errno));
    return Err_System;
    }
  if (fcntl(special->n_path, F_SETFD, FD_CLOEXEC)<0)
    {
    printf("do_stream_sock.c: fcntl(special->n_path, FD_CLOEXEC): %s\n", strerror(errno));
    }
  if (fcntl(special->n_path, F_SETFL, O_NDELAY)<0)
    {
    *outptr++=errno;
    dataout_clX(do_idx).errorcode=errno;
    dataout_clX(do_idx).do_status=Do_error;
    printf("do_stream_sock_start(do_idx=%d): can't set to O_NDELAY: %s\n",
        do_idx, strerror(errno));
    return Err_System;
    }

  sa.sin_family=AF_INET;
  sa.sin_port=htons(dataout[do_idx].addr.inetsock.port);
  sa.sin_addr.s_addr=htonl(dataout[do_idx].addr.inetsock.host);
  r=connect(special->n_path, (struct sockaddr*)&sa, sizeof(struct sockaddr_in));
  if (r==0)
    special->connected=1;
  else if (errno==EINPROGRESS)
    {
    char name[100];
    snprintf(name, 100, "do_%02d_stream_sock_connect", do_idx);
    special->connecttask=sched_insert_select_task(do_stream_sock_connect, data,
        name, special->n_path, select_write|select_except);
    if (special->connecttask==0)
      {
      dataout_clX(do_idx).errorcode=-1;
      dataout_clX(do_idx).do_status=Do_error;
      printf("do_stream_sock_start(do_idx=%d): can't create task.\n", do_idx);
      return Err_System;
      }
    }
  else
    {
    *outptr++=errno;
    dataout_clX(do_idx).errorcode=errno;
    dataout_clX(do_idx).do_status=Do_error;
    printf("do_stream_sock_start(do_idx=%d): can't connect socket: %s\n",
        do_idx, strerror(errno));
    return Err_System;
    }
  }

if (special->connected) /* egal, ob client oder server */
  {
  char name[100];
  snprintf(name, 100, "do_%02d_stream_sock_write", do_idx);
  dataout_clX(do_idx).seltask=sched_insert_select_task(do_stream_sock_write, data,
      name, special->n_path, select_write|select_except);
  if (dataout_clX(do_idx).seltask==0)
    {
    dataout_clX(do_idx).errorcode=-1;
    dataout_clX(do_idx).do_status=Do_error;
    printf("do_sock_start(do_idx=%d): can't create task.\n", do_idx);
    return Err_System;
    }
  /* wir haben aber noch keine Daten... */
  sched_select_task_suspend(dataout_clX(do_idx).seltask);
  }
else
  dataout_clX(do_idx).seltask=0;

dataout_clX(do_idx).suspended=1;

do_statistX(do_idx).clusters=0;
do_statistX(do_idx).words=0;
do_statistX(do_idx).suspensions=0;

dat->evnr_ok=0;
dat->evnr=-1;
dat->bytes=0;
dat->start=1;

printf("do_stream_sock_start: vednums.num_veds=%d\n", vednums.num_veds);
if (readout_active && (vednums.num_veds>-1))
    do_stream_sock_veds(do_idx, vednums.num_veds);

dataout_clX(do_idx).do_status=Do_running;

return OK;
}
/*****************************************************************************/
#if MAX_DATAOUT>1
static errcode do_stream_sock_disable(int do_idx)
{
T(dataout/cluster/do_stream_sock.c:do_stream_sock_disable)
printf("do_stream_sock_disable\n");
dataout_cl[do_idx].doswitch=Do_disabled;
return OK;
}
#endif
/*****************************************************************************/
#if MAX_DATAOUT>1
static errcode do_stream_sock_enable(int do_idx)
{
T(dataout/cluster/do_stream_sock.c:do_stream_sock_enable)
printf("do_stream_sock_enable\n");
dataout_cl[do_idx].doswitch=Do_enabled;
return OK;
}
#endif
/*****************************************************************************/
static void do_stream_sock_freeze(int do_idx)
{
/*
called from fatal_readout_error()
stops all activities of this dataout, but leaves all other status unchanged
*/
T(cluster/do_stream_tape:do_stream_sock_freeze)
printf("do_stream_sock_freeze\n");
D(D_TRIG, printf("do_stream_sock_freeze(do_idx=%d)\n", do_idx);)
sched_delete_select_task(dataout_clX(do_idx).seltask);
dataout_clX(do_idx).seltask=0;
}
/*****************************************************************************/
#if MAX_DATAOUT>1
static void do_stream_sock_patherror(int do_idx, int error)
#else
static void do_stream_sock_patherror(int error)
#endif
{
do_special_sock* special=&dataout_clX(do_idx).s.sock_data;
T(dataout/cluster/do_stream_sock.c:do_stream_sock_patherror)
printf("do_stream_sock_patherror\n");
if (special->n_path>=0)
  {
  close(special->n_path);
  special->n_path=-1;
  }
special->connected=0;
}
/*****************************************************************************/
static errcode do_stream_sock_reset(int do_idx)
{
do_special_sock* special=&dataout_clX(do_idx).s.sock_data;
do_stream_data* dat=&dataout_clX(do_idx).d.s_data;
T(cluster/do_stream_sock.c::do_stream_sock_reset)
printf("do_stream_sock_reset\n");
dataout_clX(do_idx).errorcode=0;
if (dataout_clX(do_idx).do_status!=Do_done)
    dataout_clX(do_idx).do_status=Do_neverstarted;
sched_delete_select_task(dataout_clX(do_idx).seltask);
dataout_clX(do_idx).seltask=0;

if (special->n_path>=0)
  {
  close(special->n_path);
  special->n_path=-1;
  }
special->connected=0;
free(dat->sevs); dat->sevs=0;
free(dat->event); dat->event=0; dat->eventsize=0;
return OK;
}
/*****************************************************************************/
static void do_stream_sock_cleanup(int do_idx)
{
do_special_sock* special=&dataout_clX(do_idx).s.sock_data;
do_stream_data* dat=&dataout_clX(do_idx).d.s_data;
T(dataout/cluster/do_stream_sock.c:do_stream_sock_cleanup)
printf("do_stream_sock_cleanup\n");
if (special->s_path>=0) close(special->s_path);
if (special->n_path>=0) close(special->n_path);
if (special->server && special->connecttask)
    sched_delete_select_task(special->connecttask);
if (dataout_clX(do_idx).seltask)
  {
  printf("Fehler in do_stream_sock_cleanup: seltask!=0\n");
  sched_delete_select_task(dataout_clX(do_idx).seltask);
  }
free(dat->sevs);
free(dat->event);
dataout_clX(do_idx).configured=0;
}
/*****************************************************************************/
#if MAX_DATAOUT>1
void do_stream_sock_advertise(int do_idx, Cluster* cl)
{
int accepted=0, wieviel=dataout[do_idx].wieviel;
T(dataout/cluster/do_cluster.c:do_cluster_advertise)

if ((cl->type!=clusterty_events) || (cl->numevents==0)) return;
if (cl->predefined_costumers && !cl->costumers[do_idx]) return;
printf("advertise ved %d; ev %d to dataout %d ?: ", cl->ved_id, cl->firstevent,
    do_idx);
fflush(stdout);
getchar();

if (wieviel==0)
  {
  accepted=1;
  }
else if (wieviel==-1)
  {
  accepted=0;
  }
else if (wieviel<0)
  {
  accepted=(cl->firstevent-1)%wieviel+cl->numevents>=wieviel;
  }
else /* if (wieviel>0) */
  {
  accepted=0;
  }

if (!accepted) return;
cl->costumers[do_idx]=1;
cl->numcostumers++;

if (dataout_cl[do_idx].suspended && dataout_cl[do_idx].seltask)
  {
  printf("wecke task\n");
  sched_select_task_reactivate(dataout_cl[do_idx].seltask);
  dataout_cl[do_idx].suspended=0;
  }
if (!dataout_cl[do_idx].advertised_cluster)
    dataout_cl[do_idx].advertised_cluster=cl;
}
#else
void do_stream_sock_advertise(Cluster* cl)
{
T(dataout/cluster/do_cluster.c:do_cluster_advertise)

/*
 * if ((cl->type!=clusterty_events) || (cl->numevents==0)) return;
 * printf("advertise ved %d; ev %d ?: ", cl->ved_id, cl->firstevent);
 * fflush(stdout);
 * getchar();
 */

cl->costumers[0]=1;
cl->numcostumers++;
if (dataout_cl.suspended && dataout_cl.seltask)
  {
  printf("wecke task\n");
  sched_select_task_reactivate(dataout_cl.seltask);
  dataout_cl.suspended=0;
  }
if (!dataout_cl.advertised_cluster) dataout_cl.advertised_cluster=cl;
}
#endif
/*****************************************************************************/
static do_procs sock_procs={
  do_stream_sock_start,
  do_stream_sock_reset,
  do_stream_sock_freeze,
#if MAX_DATAOUT>1
  do_stream_sock_disable,
  do_stream_sock_enable,
#endif
  do_stream_sock_advertise,
  do_stream_sock_patherror,
  do_stream_sock_cleanup,
  /*wind*/ do_NotImp_err,
  /* status */ 0,
  };
/*****************************************************************************/
errcode do_stream_sock_init(int do_idx)
{
do_special_sock* special=&dataout_clX(do_idx).s.sock_data;
do_stream_data* dat=&dataout_clX(do_idx).d.s_data;
callbackdata data;
data.i=do_idx;
T(dataout/cluster/do_stream_sock.c:do_stream_sock_init)
printf("do_stream_sock_init\n");

dataout_clX(do_idx).procs=sock_procs;
dataout_clX(do_idx).do_status=Do_neverstarted;
dataout_clX(do_idx).doswitch=
    readout_active==Invoc_notactive?Do_enabled:Do_disabled;

special->server=dataout[do_idx].addr.inetsock.host==INADDR_ANY;
special->s_path=-1;
special->n_path=-1;
special->connected=0;
dat->sevs=0;
dat->event=0;
dat->eventsize=0;

if (special->server)
  {
  struct sockaddr_in sa;
  char name[100];

  special->s_path=socket(AF_INET, SOCK_STREAM, 0);
  if (special->s_path<0)
    {
    *outptr++=errno;
    printf("do_stream_sock_init: can't open socket: %s\n", strerror(errno));
    return Err_System;
    }
  if (fcntl(special->s_path, F_SETFD, FD_CLOEXEC)<0)
    {
    printf("do_stream_sock.c: fcntl(special->s_path, FD_CLOEXEC): %s\n", strerror(errno));
    }
  sa.sin_family=AF_INET;
  sa.sin_port=htons(dataout[do_idx].addr.inetsock.port);
  sa.sin_addr.s_addr=INADDR_ANY;
  if(bind(special->s_path, (struct sockaddr*)&sa, sizeof(struct sockaddr_in))<0)
    {
    *outptr++=errno;
    printf("do_stream_sock_init: can't bind socket: %s\n", strerror(errno));
    close(special->s_path);
    return Err_System;
    }
  if (fcntl(special->s_path, F_SETFL, O_NDELAY)<0)
    {
    *outptr++=errno;
    printf("do_stream_sock_init: can't set to O_NDELAY: %s\n",
        strerror(errno));
    close(special->s_path);
    return Err_System;
    }
  if (listen(special->s_path, 1)<0)
    {
    *outptr++=errno;
    printf("do_stream_sock_init: can't listen: %s\n", strerror(errno));
    close(special->s_path);
    return Err_System;
    }

  snprintf(name, 100, "do_%02d_stream_sock_accept", do_idx);
  special->connecttask=sched_insert_select_task(do_stream_sock_accept, data,
      name, special->s_path, select_read|select_except);
  if (special->connecttask==0)
    {
    printf("do_stream_sock_init: can't create task.\n");
    close(special->s_path);
    return Err_System;
    }
  }
dataout_clX(do_idx).configured=1;
return OK;
}
/*****************************************************************************/
/*****************************************************************************/
