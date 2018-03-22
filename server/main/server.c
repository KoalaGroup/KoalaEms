/*
 * server.c
 * created before 13.10.94
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: server.c,v 1.41 2017/10/20 23:21:31 wuestner Exp $";

#define _main_c_
#include <sconf.h>
#include <debug.h>

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../objects/notifystatus.h"
#include "../commu/commu.h"
#ifdef PI_READOUT
#include "../objects/pi/readout.h"
#include "../dataout/dataout.h"
#endif /* PI_READOUT */
#include "conststrings.h"
#include "requests.h"
#include "server.h"
#include "nowstr.h"
#include "signals.h"
#ifdef OBJ_DO
#include "../dataout/dataout.h"
#endif
#ifdef PI_LAM
#include "../objects/pi/lam/lam.h"
#endif
#include "../objects/ved/ved.h"
#include "../lowlevel/lowlevel_init.h"
#ifdef LOWLEVELBUFFER
#include "../lowlevel/lowbuf/lowbuf.h"
#endif
#ifdef TRIGGER
#include "../trigger/trigger.h"
#endif

#ifdef DMALLOC
#include <dmalloc.h>
#endif

#ifdef __linux__
#define LINUX_LARGEFILE O_LARGEFILE
#else
#define LINUX_LARGEFILE 0
#endif

/* in server/conf/sconf.c */
extern char configuration[], buildname[], builddate[];

void moncontrol(int);

ems_u32* outptr;

RCS_REGISTER(cvsid, "main")

/*****************************************************************************/
const char*
nowstr(void)
{
    static char ts[50];
    struct timeval now;
    struct tm* tm;
    time_t tt;

    gettimeofday(&now, 0);
    tt=now.tv_sec;
    tm=localtime(&tt);
    strftime(ts, 50, NOWSTR, tm);
    return ts;
}
/*****************************************************************************/

static char *helpline[]= {
  "usage:\nserver"
#ifdef DEBUG
  " [-D ] {-d <debugcode>} [-v] [-t]"
#endif /* DEBUG */
  " [-p <port>]"
  " [-l <lowlevel parameter>]"
  " [-o <output-parameter>]"
  " [-w <logfile>]|"
  " [-q]"
  "\n"
};
/*****************************************************************************/
static void
printuse(FILE* outfilepath)
{
    unsigned int i;
    int res;

    for (i=0; i<sizeof(helpline)/sizeof(helpline[0]); i++)
        fprintf(outfilepath, "%s", helpline[i]);

    /* port-Parameter*/
    fprintf(outfilepath, "accepted parameters for <port>:\n");
    res=printuse_port(outfilepath);
    if (res==0)
        fprintf(outfilepath, "none\n");

#ifdef LOWLEVEL
    /* lowlevel-Parameter */
    fprintf(outfilepath, "accepted parameters for <lowlevel-Parameter>:\n");
    res=printuse_lowlevel(outfilepath);
    if (res==0)
        fprintf(outfilepath, "none\n");
#endif

#ifdef OBJ_DO
    /* Output-Parameter */
    fprintf(outfilepath, "accepted parameters for <Output-Parameter>:\n");
    res=printuse_output(outfilepath);
    if (res==0)
        fprintf(outfilepath, "none\n");
#endif
}
/*****************************************************************************
static void
drucks(callbackdata arg)
{
    printf("%s\n",(char*)arg.p);
}
*****************************************************************************/
#ifdef DEBUG
static struct dtentry
  {
  char *name;
  int mask;
  } debugtable[]=
    {
      {"TRACE",D_TRACE},
      {"COMM",D_COMM},
      {"TR",D_TR},
      {"DUMP",D_DUMP},
      {"MEM",D_MEM},
      {"EV",D_EV},
      {"PL",D_PL},
      {"USER",D_USER},
      {"REQ",D_REQ},
      {"TRIG",D_TRIG},
      {"TIME",D_TIME},
      {"FORK",D_FORK},
      {"LOW",D_LOW},
      {0,0}
    };
#endif

static char *lowarg=(char*)0;
static char *outarg=(char*)0;

/*
 * #ifndef OSK 
 * #ifdef __GNUC__
 * __warn_references(main, "info: this program uses main(), which is expected.");
 * #endif
 * #endif
 */
/*****************************************************************************/
static void
shutdown_server(void)
{
    T(shutdown_server)
printf("shutdown_server\n");
#ifdef DMALLOC
    dmalloc_verify(0);
#endif

#ifdef PI_LAM
    /* LAMs deaktivieren */
    stoplams();
#endif

#ifdef PI_READOUT
    readout_active=0;
#endif

    ved_done();

#ifdef PI_READOUT
    dataout_done();
#endif

#ifdef LOWLEVEL
    lowlevel_done();
#endif
    done_commandinterp();
    done_comm();
#ifdef LOWLEVELBUFFER
    lowbuffer_done();
#endif
    done_sched();
    done_timers();
    done_signalhandler();

#ifdef DMALLOC
printf("calling dmalloc_shutdown\n");
    dmalloc_verify(0);
    dmalloc_shutdown();
#endif
}
/*****************************************************************************/
static void
print_start(char *argv0)
{
    printf("== %s ==\n== SERVER (%s)!!!\n", nowstr(), argv0);
}
/*****************************************************************************/
int
main(int argc, char** argv)
{
    int c;
    int error=0;
    char *port=(char*)0;
    errcode res;
    char* OPTSTRING;

    moncontrol(0);

    setlinebuf(stdout);

    T(main)
    print_start(argv[0]);

    OPTSTRING="p:c:l:o:w:qh"
#ifdef DEBUG
    "Dd:vt"
#endif
;

    while ((c=getopt(argc,argv,OPTSTRING))!=EOF) {
        switch(c){
#ifdef DEBUG
        case 'D': debug=D_COMM | D_TR | D_PL | D_USER | D_REQ; break;
        case 'd':{
            struct dtentry *d;
            if (strncmp(optarg,"D_",2)==0)
                optarg+=2;
            d= &debugtable[0];
            while ((d->name)&&(strcasecmp(d->name,optarg)))
                d++;
            if (d->name) {
                debug|=d->mask;
            } else {
                printf("unrecognised debug code %s\n", optarg);
                error=1;
            }
            break;
            }
        case 'v': verbose=1; break;
        case 't': debug|=D_TRACE; break;
#endif
        case 'p':
            port=optarg;
            break;
        case 'l':
            lowarg=optarg;
            break;
        case 'o':
            outarg=optarg;
            break;
        case 'q':
            printf("VED %s\n", buildname);
            printf("configuration from %s\n", builddate);
            printf("configuration:\n%s\n", configuration);
            rcs_dump(stdout);
            exit(0);
            break;
        case 'w': {
            int newpath;
            newpath=open(optarg, O_WRONLY|O_CREAT|O_APPEND|LINUX_LARGEFILE,
                    0644);
            if (newpath==-1) {
                printf("open logfile \"%s\": %s\n", optarg, strerror(errno));
                exit(1);
            }
            if (newpath!=1)
                dup2(newpath, 1);
            if (newpath!=2)
                dup2(newpath, 2);
            if ((newpath!=1) && (newpath!=2))
                close(newpath);
            print_start(argv[0]);
            }
            break;
        case 'h':
        case '?': error=1; break;
        }
    }

    if (error) {
        printuse(stderr);
        exit(1);
    }
#ifdef DEBUG
    if (debug)
        printf("Debuglevel: 0x%08x\n",debug);
#endif

    if ((res=init_signalhandler())) {
        printf("Fatal: cannot install signal handler: %s\n", R_errstr(res));
        exit(1);
    }

    init_timers();
    init_sched();

#ifdef LOWLEVELBUFFER
    if ((res=lowbuffer_init(lowarg))!=OK) {
        printf("Fatal: cannot initialize lowlevel buffer: %s\n",
                R_errstr(res));
        exit(1);
    }
#endif

    if ((res=init_comm(port))!=OK) {
        printf("Fatal: cannot open communication path: %s\n", R_errstr(res));
        exit(1);
    }

    init_commandinterp();

#ifdef LOWLEVEL
    if ((res=lowlevel_init(lowarg))!=OK) {
        printf("Fatal: cannot initialize lowlevel functions: %s\n",
                R_errstr(res));
        exit(1);
    }
#endif

#ifdef TRIGGER
    if ((res=trigger_init())!=OK) {
        printf("Fatal: cannot initialize trigger functions: %s\n",
                R_errstr(res));
        exit(1);
    }
#endif

#ifdef PI_READOUT
    if((res=dataout_init(outarg))!=OK) {
        printf("Fatal: cannot initialize data path: %s\n", R_errstr(res));
        exit(1);
    }
#endif

    if((res=ved_init())!=OK) {
        printf("Fatal: cannot initialize VED: %s\n", R_errstr(res));
        exit(1);
    }
D(D_REQ,printf("Ready for requests\n");)

    install_shutdownhandler(shutdown_server);

/*
 * {
 * callbackdata arg;
 * arg.p="hurra, ich lebe noch";
 * sched_exec_periodic(drucks,arg,5000,"hallo");
 * }
 */

#if 0
    if (mlockall(MCL_CURRENT|MCL_FUTURE)<0) {
        /* not fatal but alarming */
        printf("mlockall: %s\n", strerror(errno));
    }
#endif

    sched_mainloop();
    return 0;
}
/*****************************************************************************/
errcode
ResetVED(__attribute__((unused)) int* p, int size)
{
    errcode res, rr;

    printf("== %s ResetVED ==\n", nowstr());

    if (size)
        return Err_ArgNum;

#ifdef PI_READOUT
    /* Readout muss vom User beendet werden */
    if (readout_active)
        return Err_PIActive;
#endif

#ifdef DMALLOC
    dmalloc_verify(0);
#endif

    notifystatus(status_action_reset, Object_ved, 0, 0);

#ifdef PI_LAM
/* LAMs deaktivieren */
    stoplams();
#endif /* PI_LAM */

    res=ved_done();
    D(D_REQ, if (res!=OK) printf("ResetVED: ved_done()=%d\n", res);)

#ifdef PI_READOUT
    rr=dataout_done();
    if (res==OK)
        res=rr;
    D(D_REQ, if (rr!=OK) printf("ResetVED: dataout_done()=%d\n", rr);)
#endif

#ifdef TRIGGER
    rr=trigger_done();
    if (res==OK)
        res=rr;
    D(D_REQ, if (rr!=OK) printf("ResetVED: trigger_done()=%d\n", rr);)
#endif

#ifdef LOWLEVEL
    rr=lowlevel_done();
    if (res==OK)
        res=rr;
    D(D_REQ, if (rr!=OK) printf("ResetVED: lowlevel_done()=%d\n", rr);)
#endif

#ifdef DMALLOC
    dmalloc_verify(0);
#endif

#ifdef LOWLEVEL
    if (!res) {
        res=lowlevel_init(lowarg);
        D(D_REQ, if (res!=OK) printf("ResetVED: lowlevel_init()=%d\n", res);)
    }
#endif

#ifdef TRIGGER
    if (res==OK) {
        res=trigger_init();
        D(D_REQ, if (res!=OK) printf("ResetVED: trigger_init()=%d\n", res);)
    }
#endif

#ifdef PI_READOUT
    if (res==OK) {
        res=dataout_init(outarg);
        D(D_REQ, if (res!=OK) printf("ResetVED: dataout_init()=%d\n", res);)
    }
#endif
    if (res==OK) {
        res=ved_init();
        D(D_REQ, if (res!=OK) printf("ResetVED: ved_init()=%d\n", res);)
    }

#ifdef DMALLOC
    dmalloc_verify(0);
#endif

    return res;
}
/*****************************************************************************/
#ifndef OSK
#include <signal.h>

void
panic(void)
{
    printf("### PANIC ###\n");
    sched_print_tasks();
    fflush(stdout);
    kill(getpid(), SIGSEGV);
}
#endif
/*****************************************************************************/
void
die(const char* text)
{
    printf("### ABORT: %s ###\n", text);
    fflush(stdout);
    exit(10);
}
/*****************************************************************************/
/*****************************************************************************/
