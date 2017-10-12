/*
 * trigger/pci/sharedzelsync/sh_zelsynctrigger.c
 * created 2011-03-17 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL$";

#include <sconf.h>
#include <debug.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <strings.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <errorcodes.h>
#include <unsoltypes.h>
#include "../../../main/signals.h"
#include "../../../main/scheduler.h"
#include "../../trigger.h"
#include "../../trigprocs.h"
#ifdef OBJ_VAR
#include "../../../objects/var/variables.h"
#endif
#include "../../../objects/pi/readout.h"
#include "../../../commu/commu.h"
#include <xdrstring.h>
#include <dev/pci/zelsync.h>
#include "sh_zelsynctrigger.h"

#ifdef DMALLOC
#include dmalloc.h
#endif

#define MAINSTAT SYNC_GET_MASTER|SYNC_GET_GO|SYNC_GET_T4LATCH|SYNC_GET_TAW_ENA|\
    SYNC_GET_AUX_OUT|\
    SYNC_GET_EN_INT

typedef enum {
    sync_poll=0,
    sync_signal,
    sync_select
} sync_mode;

typedef enum {
    flag_raux0=8,
    flag_t4=4,
    flag_taw=2,
    flag_tint=1
} syncflags;

struct private {
    sync_mode mode;
    int master;
    struct syncintctrl intctrl;
    u_int32_t conversiontime;
    int flags;
    u_int32_t softtrigg_; /* ID for soft trigger */
    u_int32_t set_aux_out; /* set aux_out to this value */
    char *pathname;
    int path;
    volatile int sigtrig;
#ifdef SYNCSTATIST
    struct syncstatistics lsyncstat; /* local teadtime */
    struct syncstatistics tsyncstat; /* total teadtime */
    struct syncstatistics gsyncstat; /* trigger gaps */
    ems_u64 zelsyncrejected;
#endif
    struct trigstatus pci_trigdata, pci_oldtrigdata;
};

extern ems_u32* outptr;

/*****************************************************************************/
static char* getbits(unsigned int v)
{
    static char s[42];
    char st[32];
    int i, j, k, l, m;

    for (i=0; i<41; i++) s[i]=' ';
    s[41]=0;
    for (i=0; i<32; i++) st[i]='0';

    for (i=31; v; i--, v>>=1) if (v&1) st[i]='1';

    l=m=0;
    for (k=0; k<4; k++) {
        for (j=0; j<2; j++) {
            for (i=0; i<4; i++) s[l++]=st[m++];
            l++;
        }
        l++;
    }
    return s;
}
/*****************************************************************************/
static const char* statnames[]={
    "Master",
    "GO",
    "T4LATCH",
    "TAW_ENA",
    "AUX_OUT(0)",
    "AUX_OUT(1)",
    "AUX_OUT(2)",
    "AUX0_RES_TRIG",
    "Int_Ena_AUX(0)",
    "Int_Ena_AUX(1)",
    "Int_Ena_AUX(2)",
    "Int_Ena_EOC",
    "Int_Ena_SI",
    "Int_Ena_TRIG",
    "Int_Ena_GAP",
    "GSI(obsolete)",
    "Trigger(0)",
    "Trigger(1)",
    "Trigger(2)",
    "Trigger(3)",
    "GO_RING",
    "VETO",
    "SI",
    "INH",
    "AUX_IN(0)", 
    "AUX_IN(1)", 
    "AUX_IN(2)", 
    "EOC",
    "SI_RING",
    "(unused)",
    "GAP",
    "TDT_RING",
};
#if 0
static ems_u32 stat_aux_out    =0x0000070;
static ems_u32 stat_int_ena_aux=0x0000700;
static ems_u32 stat_trigger    =0x00f0000;
static ems_u32 stat_aux_in     =0x7000000;
static ems_u32 stat_single_bits=~(stat_aux_out|stat_int_ena_aux|
        stat_trigger|stat_aux_in);
#endif
/*****************************************************************************/
static void pretty_print_stat(ems_u32 stat)
{
    int i, j;
    printf("status 0x%08x: %s\n", stat, getbits(stat));
    for (i=31; i>=0; i--) {
        if (stat&(1<<i)) {
            int indent=28+32-i;
            indent-=i/4;
            indent-=i/8;
            for (j=0; j<indent; j++)
                printf(" ");
            printf("%s\n", statnames[i]);
        }
    }
}
/*****************************************************************************/
static void statusprint(ems_u32 stat, const char* text)
{
    if (text)
        printf("zelsynctrigger: %s\n", text);
    pretty_print_stat(stat);
}
/*****************************************************************************/
int print_sharedzelsync_status(struct triggerinfo* trinfo, ems_u32** ptr,
        int text)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    struct trigstatus pci_trigdata;

    if (tinfo->magic!=init_trig_zelsync) {
        printf("print_zelsync_status: current trigger is not zelsync\n");
        return -1;
    }
    pci_trigdata=priv->pci_trigdata;

if (ptr)
  {
  *(*ptr)++=pci_trigdata.mbx;
  *(*ptr)++=pci_trigdata.state;
  *(*ptr)++=pci_trigdata.evc;
  *(*ptr)++=pci_trigdata.tmc;
  *(*ptr)++=pci_trigdata.tdt;
  *(*ptr)++=pci_trigdata.reje;
  *(*ptr)++=pci_trigdata.tgap[0];
  *(*ptr)++=pci_trigdata.tgap[1];
  *(*ptr)++=pci_trigdata.tgap[2];
  }
if (text)
  {
  printf("mailbox: (%s)\n", getbits(pci_trigdata.mbx));
  printf("status : (%s)\n", getbits(pci_trigdata.state));
  if (pci_trigdata.state&1) printf("Master ");
  if (pci_trigdata.state&2) printf("GO ");
  if (pci_trigdata.state&4) printf("T4LATCH ");
  if (pci_trigdata.state&8) printf("TAW_ENA ");
  if (pci_trigdata.state&&0x70) printf("AUX_OUT(%d) ",
        (pci_trigdata.state&0x70)>>4);
  if (pci_trigdata.state&0x80) printf("AUX0_RES_TRIG ");
  if (pci_trigdata.state&0x700) printf("Int_Ena_AUX(%d) ",
        (pci_trigdata.state&0x700)>>8);
  if (pci_trigdata.state&0x800) printf("Int_Ena_EOC ");
  if (pci_trigdata.state&0x1000) printf("Int_Ena_SI ");
  if (pci_trigdata.state&0x2000) printf("Int_Ena_TRIG ");
  if (pci_trigdata.state&0x4000) printf("Int_Ena_GAP ");
  if (pci_trigdata.state&0x8000) printf("GSI(obsolete) ");
  if (pci_trigdata.state&0xf0000) printf("Trigger(%d) ",
        (pci_trigdata.state&0xf0000)>>16);
  if (pci_trigdata.state&0x100000) printf("GO_RING ");
  if (pci_trigdata.state&0x200000) printf("VETO ");
  if (pci_trigdata.state&0x400000) printf("SI ");
  if (pci_trigdata.state&0x800000) printf("INH ");
  if (pci_trigdata.state&0x7000000) printf("AUX_IN(%d) ",
        (pci_trigdata.state&0x7000000)>>24);
  if (pci_trigdata.state&0x8000000) printf("EOC ");
  if (pci_trigdata.state&0x10000000) printf("SI_RING ");
  if (pci_trigdata.state&0x40000000) printf("GAP ");
  if (pci_trigdata.state&0x80000000) printf("TDT_RING ");
  printf("\n");
  
  printf("eventcount=%d; tmc=%d; tdt=%d; rej=%d\n"
         "gap1=%d; gap2=%d; gap3=%d\n",
    pci_trigdata.evc,
    pci_trigdata.tmc,
    pci_trigdata.tdt,
    pci_trigdata.reje, 
    pci_trigdata.tgap[0], pci_trigdata.tgap[1], pci_trigdata.tgap[2]);
  }
  return 0;
}
/*****************************************************************************/
static void
fatal(struct triggerinfo* trinfo, int code, unsigned int num, ...)
{
    ems_u32 msg[16];
    va_list ap;
    int i;

    msg[0]=rtErr_Trig;
    msg[1]=trinfo->eventcnt;
    msg[2]=code;
    va_start(ap, num);
    for (i=0; i<num; i++) {
        msg[i+3]=va_arg(ap, ems_u32);
    }
    va_end(ap);
    send_unsolicited(Unsol_RuntimeError, msg, num+3);
    fatal_readout_error();
}
/*****************************************************************************/
int get_pci_sharedtrigdata(struct triggerinfo* trinfo,
        struct trigstatus *new, struct trigstatus *old)
{
    struct trigprocinfo* tinfo;
    struct private* priv;

    tinfo=(struct trigprocinfo*)trinfo->tinfo;
    if (!tinfo) {
        printf("get_pci_trigdata: trigger is not initialized\n");
        return -1;
    }
    priv=(struct private*)tinfo->private;

    if (tinfo->magic!=init_trig_zelsync) {
        printf("get_pci_trigdata: current trigger is not zelsync\n");
        return -1;
    }
    if (new)
        *new=priv->pci_trigdata;
    if (old)
        *old=priv->pci_oldtrigdata;
    return 0;
}
/*****************************************************************************/
int get_sharedsyncstatist(struct triggerinfo* trinfo,
        ems_u64* zelsyncrejected)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;

    if (tinfo->magic!=init_trig_zelsync) {
        printf("get_syncstatist: current trigger is not zelsync\n");
        return -1;
    }
    if (zelsyncrejected)
        *zelsyncrejected=priv->zelsyncrejected;
    return 0;
}
/*****************************************************************************/
#ifdef SYNCSTATIST
static void
clearsyncstatist(struct syncstatistics* statistics)
{
    statistics->entries=0;
    statistics->ovl=0;
    statistics->max=0;
    statistics->min=0xffffffffL;
    statistics->sum=0;
    statistics->qsum=0;
    bzero(statistics->hist, statistics->histsize*sizeof(int));
}
void clear_sharedsyncstatist(struct triggerinfo* trinfo,
        enum syncstatist_types typ)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;

    if (typ&syncstatist_ldt)
        clearsyncstatist(&priv->lsyncstat);
    if (typ&syncstatist_tdt && priv->master)
        clearsyncstatist(&priv->tsyncstat);
    if (typ&syncstatist_gap && priv->master)
        clearsyncstatist(&priv->gsyncstat);
}
/*****************************************************************************/
static plerrcode setsyncstatist(struct syncstatistics* statistics,
    int size, int scale)
{
    statistics->histsize=size;
    statistics->histscale=scale;
    statistics->hist=realloc(statistics->hist, size*sizeof(int));
    if (statistics->hist==0) {
        statistics->histsize=0;
        return plErr_NoMem;
    }
    clearsyncstatist(statistics);
    return plOK;
}
plerrcode set_sharedsyncstatist(struct triggerinfo* trinfo,
        enum syncstatist_types typ, int size, int scale)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    plerrcode res;

    if (typ&syncstatist_ldt) {
        if ((res=setsyncstatist(&priv->lsyncstat, size, scale))!=plOK)
            return res;
    }
    if (typ&syncstatist_tdt && priv->master) {
        if ((res=setsyncstatist(&priv->tsyncstat, size, scale))!=plOK)
            return res;
    }
    if (typ&syncstatist_gap && priv->master) {
        if ((res=setsyncstatist(&priv->gsyncstat, size, scale))!=plOK)
            return res;
    }
    return plOK;
}
/*****************************************************************************/
static plerrcode initstatistic(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    plerrcode res;

    priv->zelsyncrejected=0;
    res=set_sharedsyncstatist(trinfo,
            syncstatist_ldt|syncstatist_tdt|syncstatist_gap,
            3000, 0);
    return res;
}
/*****************************************************************************/
struct syncstatistics* get_sharedsyncstatist_ptr(struct triggerinfo* trinfo,
        enum syncstatist_types typ)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;

    switch (typ) {
        case syncstatist_ldt: return &priv->lsyncstat;
        case syncstatist_tdt: return &priv->tsyncstat;
        case syncstatist_gap: return &priv->gsyncstat;
    }
    return 0;
}
#endif
/*****************************************************************************/
static void sighndlr(union SigHdlArg arg, int sig)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)arg.ptr;
    struct private* priv=(struct private*)tinfo->private;
    priv->sigtrig=1;
}
/*****************************************************************************/
static plerrcode
done_trig_sharedzelsync(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    T(done_trig_zelsync)

    if (priv->intctrl.signal) {
        remove_signalhandler(priv->intctrl.signal);
    }
    close(priv->path);
    free(priv->pathname);
    free(tinfo->private);
    tinfo->private=0;
    return plOK;
}
/*****************************************************************************/
static int
softtrigger(struct triggerinfo* trinfo, struct private* priv)
{
    int res;

    res=ioctl(priv->path, PCISYNC_SOFTTRIGG, &priv->softtrigg_);
    if (res<0) {
        complain("shared_zelsync:get:soft: %s", strerror(errno));
        fatal(trinfo, 1, 1, errno);
        return -1;
    }
    return 0;
}
/*****************************************************************************/
static int
auxtrigger(struct triggerinfo* trinfo, struct private* priv)
{
    u_int32_t pulse[2];
    int res;

    pulse[0]=priv->set_aux_out; /* only bits 0..2 used */
    pulse[1]=1; /* 1 us */
    res=ioctl(priv->path, PCISYNC_AUXPULSE, pulse);
    if (res<0) {
        complain("shared_zelsync:get:aux: %s", strerror(errno));
        fatal(trinfo, 1, 1, errno);
        return -1;
    }
    return 0;
}
/*****************************************************************************/
/*
 * selftrigger is called from get_trig_signal and get_trig_poll if
 * no trigger arrived yet. It tries to 'order' a trigger from the sync module
 * (via aux_out) or from the driver.
 * Only the master is supposed to do this.
 * returns:
 *   -1: if an error occured
 *    0: in any other case (even if no soft- or auxtrigger is defined)
 */
static int
selftrigger(struct triggerinfo* trinfo, struct private* priv)
{
    if (!priv->master)
        return 0;

    if (priv->softtrigg_)
        return softtrigger(trinfo, priv);
    else if (priv->set_aux_out)
        return auxtrigger(trinfo, priv);
    else
        return 0;
}
/*****************************************************************************/
static int
get_trig_signal(struct triggerinfo* trinfo, struct private* priv)
{
    ssize_t got;

    if (!priv->sigtrig)
        return selftrigger(trinfo, priv);

    priv->sigtrig=0;
    got=read(priv->path, ((char*)&priv->pci_trigdata),
            sizeof(struct trigstatus));
    if (got!=sizeof(struct trigstatus)) {
        complain("shared_zelsync:get:signal: got=%lld, %s",
                (long long)got, strerror(errno));
        fatal(trinfo, 1, 1, errno);
        return -1;
    }
    return 0;    
}
/*****************************************************************************/
static int
get_trig_poll(struct triggerinfo* trinfo, struct private* priv)
{
    ssize_t got;

    got=ioctl(priv->path, PCISYNC_DATA, &priv->pci_trigdata);
    if (got!=sizeof(struct trigstatus)) {
        if (got<0 && errno==EWOULDBLOCK) {
            return selftrigger(trinfo, priv);
        } else {
            complain("shared_zelsync:get:poll: got=%lld, %s",
                    (long long)got, strerror(errno));
            fatal(trinfo, 1, 1, errno);
            return -1;
        }
    }
    return 0;    
}
/*****************************************************************************/
static int
get_trig_select(struct triggerinfo* trinfo, struct private* priv)
{
    ssize_t got;

    got=read(priv->path, (char*)&priv->pci_trigdata, sizeof(struct trigstatus));
    if (got!=sizeof(struct trigstatus)) {
        complain("read trigstatus: got=%lld: %s",
                (long long)got, strerror(errno));
        fatal(trinfo, 5, 1, errno);
        return -1;
    }
    return 0;
}
/*****************************************************************************/
static int
get_trig_sharedzelsync(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    unsigned int trigger;
    int res=-1;

    switch (priv->mode) {
    case sync_signal:
        res=get_trig_signal(trinfo, priv);
        break;
    case sync_poll:
        res=get_trig_poll(trinfo, priv);
        break;
    case sync_select:
        res=get_trig_select(trinfo, priv);
        break;
    }
    if (res<0) { /* no trigger received */
        return selftrigger(trinfo, priv);
        return 0;
    }

    /* tdt belongs to the previous event */
    priv->pci_oldtrigdata.tdt=priv->pci_trigdata.tdt;

    if (trinfo->eventcnt!=priv->pci_trigdata.evc) {
        printf("zelsynctrigger: eventcnt: %d ==> %d\n",
                trinfo->eventcnt, priv->pci_trigdata.evc);
        statusprint(priv->pci_trigdata.state, 0);
        fatal(trinfo, 6, 3, priv->pci_trigdata.mbx, priv->pci_trigdata.state,
                priv->pci_trigdata.evc);
        /* eventcnt=priv->pci_trigdata.evc; */
        return 0;
    }

    if (priv->pci_trigdata.state&(SYNC_GET_SI_RING|SYNC_GET_SI)) {
        statusprint(priv->pci_trigdata.state, "Subenvent invalid");
        fatal(trinfo, 7, 2, priv->pci_trigdata.mbx, priv->pci_trigdata.state);
        return 0;
    }

    trigger=(priv->pci_trigdata.state&SYNC_GET_TRIG)>>16;
    if (trigger==0) {
        statusprint(priv->pci_trigdata.state, "got no trigger");
        fatal(trinfo, 8, 2, priv->pci_trigdata.mbx, priv->pci_trigdata.state);
        return 0;
    }
    trinfo->eventcnt++;

    return trigger;
}
/*****************************************************************************/
static void
reset_trig_sharedzelsync(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    u_int32_t dt;

    if (ioctl(priv->path, PCISYNC_READY, &dt)<0) {
        complain("clear trigger: %s", strerror(errno));
        fatal_readout_error();
        return;
    }

    priv->pci_trigdata.ldt=dt;
    priv->pci_oldtrigdata=priv->pci_trigdata;

#ifdef SYNCSTATIST
    {
        /* !!! tdt belongs to the previous event */
        int tdt=priv->pci_trigdata.tdt;
        int ldt=priv->pci_trigdata.ldt;

        priv->zelsyncrejected+=priv->pci_trigdata.reje;
        if (trinfo->eventcnt>2) {
            priv->lsyncstat.entries++;
            if (ldt>priv->lsyncstat.max)
                priv->lsyncstat.max=ldt;
            if (ldt<priv->lsyncstat.min)
                priv->lsyncstat.min=ldt;
            priv->lsyncstat.sum+=ldt;
            priv->lsyncstat.qsum+=ldt*ldt;

            if (priv->lsyncstat.histscale)
                ldt/=priv->lsyncstat.histscale;
            if (ldt<priv->lsyncstat.histsize)
                priv->lsyncstat.hist[ldt]++;
            else
                priv->lsyncstat.ovl++;

            if (priv->master) {
                int i, weiter;
                priv->tsyncstat.entries++;
                if (tdt==0)
                    printf("ev. %d: tdt=0\n", trinfo->eventcnt);
                if (tdt>priv->tsyncstat.max)
                    priv->tsyncstat.max=tdt;
                if (tdt<priv->tsyncstat.min)
                    priv->tsyncstat.min=tdt;
                priv->tsyncstat.sum+=tdt;
                priv->tsyncstat.qsum+=tdt*tdt;

                if (priv->tsyncstat.histscale)
                    tdt/=priv->tsyncstat.histscale;
                if (tdt<priv->tsyncstat.histsize)
                    priv->tsyncstat.hist[tdt]++;
                else
                   priv-> tsyncstat.ovl++;
                i=0;
                weiter=1;
                while (weiter && (i<3)
                        && ((priv->pci_trigdata.tgap[i]&SYNC_GAP_INVAL)==0)) {
                    int gap=priv->pci_trigdata.tgap[i]&0xffff;
                    priv->gsyncstat.entries++;
                    if (gap>priv->gsyncstat.max)
                        priv->gsyncstat.max=gap;
                    if (gap<priv->gsyncstat.min)
                        priv->gsyncstat.min=gap;
                    priv->gsyncstat.sum+=gap;
                    priv->gsyncstat.qsum+=gap*gap;

                    if (priv->gsyncstat.histscale)
                        gap/=priv->gsyncstat.histscale;
                    if (gap<priv->gsyncstat.histsize)
                        priv->gsyncstat.hist[gap]++;
                    else
                        priv->gsyncstat.ovl++;
                    weiter=priv->pci_trigdata.tgap[i]&SYNC_GAP_MORE;
                    i++;
                }
            }
        }
    }
#endif
}
/*****************************************************************************/
/*
 * p[0] number of arguments
 * p[1] (string ) device
 * pp[2] (integer) 0: slave 1: master 2: master+softtrigger
 * pp[3] (integer) ctime (24 bit)
 * [pp[4] (integer) 0: poll; 1: signal; 2: select (default: 2)
 * [pp[5] (bitmask) RAUX0|T4LATCH|TAW|TINT (default: 0)
 *       (0x8) RAUX0  : AUX_IN0 resets trigger
 *       (0x4) T4LATCH: Trigger 4 is latched
 *       (0x2) TAW    : use trigger acceptance window
 *       (0x1) TINT   : use trigger interrupt, not EOC interrupt
 * [pp[6] (integer) fast clear acceptance time (ignored)
 * [pp[7]
 *      selftrigger: (bitmask) 1..7: set aux_out to this value
 *                   (default: 7)
 *      softtrigger: (usigned) trigger-ID
 *                   (default: 1)
 * ]]]]
 */

plerrcode init_trig_sharedzelsync(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv;
    ems_u32* pp;
    int rest;
    plerrcode pres=plOK;

    priv=calloc(1, sizeof(struct private));
    if (!priv) {
        complain("init_trig_zelsync:calloc: %s", strerror(errno));
        return plErr_NoMem;
    }
    priv->path=-1;
    tinfo->private=priv;
    tinfo->magic=init_trig_zelsync;

    if (p[0]<1) {
        complain("init_trig_zelsync: no parameters");
        *outptr++=1;
        pres=plErr_ArgNum;
        goto error;
    }
    if (xdrstrlen(p+1)<1) {
        complain("init_trig_zelsync: device name ist %d characters long.",
                xdrstrlen(p+1));
        *outptr++=2;
        pres=plErr_ArgNum;
        goto error;
    }
    rest=p[0]-xdrstrlen(p+1);

    if (rest<2) {
        complain("init_trig_zelsync: device name needs %d words, "
                "but I have only %d",
                xdrstrlen(p+1), p[0]);
        *outptr++=3;
        pres=plErr_ArgNum;
        goto error;
    }

    pp=xdrstrcdup(&priv->pathname, p+1);
    if (priv->pathname==0) {
        complain("init_trig_zelsync:xdrstrcdup: %s", strerror(errno));
        pres=plErr_NoMem;
        goto error;
    }
    priv->master=pp[0];

    priv->conversiontime=pp[1];
    if (priv->conversiontime&0xff000000) {
        complain("init_trig_zelsync: Conversion time 0x%08x is too large.",
                priv->conversiontime);
        *outptr++=1;
        pres=plErr_ArgRange;
        goto error;
    }
    if (rest>2)
        priv->mode=(sync_mode)pp[2];
    else
        priv->mode=2;
    switch (priv->mode) {
    case sync_poll:
        D(D_TRIG, printf("init_trig_zelsync: using poll mode\n");)
        break;
    case sync_signal:
        D(D_TRIG, printf("init_trig_zelsync: using signal mode\n");)
        break;
    case sync_select:
        D(D_TRIG, printf("init_trig_zelsync: using select mode\n");)
        break;
    default:
        complain("init_trig_zelsync: mode %d is not known", priv->mode);
        *outptr++=2;
        pres=plErr_ArgRange;
        goto error;
    }

    if (rest>3)
        priv->flags=pp[3];
    else
        priv->flags=0;

    /* fcatime ignored */

    if (rest>5) {
        if (priv->master>0)
            priv->softtrigg_=pp[5];
        else
            priv->set_aux_out=pp[5]&7;
    }
    if (priv->master==2 && !priv->softtrigg_)
        priv->softtrigg_=1;
    if (priv->master==3 && !priv->set_aux_out)
        priv->set_aux_out=7;

    trinfo->eventcnt=0;

#ifdef SYNCSTATIST
    if ((pres=initstatistic(trinfo))!=plOK)
        return pres;
#endif

    if ((priv->path=open(priv->pathname, O_RDWR))<=0) {
        *outptr++=errno;
        complain("init_trig_zelsync: open \"%s\": %s",
                priv->pathname, strerror(errno));
        pres=plErr_System;
        goto error;
    }
    {
        long flags=0;
        if ((flags=fcntl(priv->path, F_GETFD))<0)
            complain("init_trig_zelsync: F_GETFD: %s", strerror(errno));
        flags|=FD_CLOEXEC;
        if (fcntl(priv->path, F_SETFD, &flags)<0)
            complain("init_trig_zelsync: FD_CLOEXEC: %s", strerror(errno));
    }
    {
        long flags=0;
        if ((flags=fcntl(priv->path, F_GETFL))<0)
            complain("init_trig_zelsync: F_GETFL: %s", strerror(errno));
        flags|=O_NONBLOCK;
        if (fcntl(priv->path, F_SETFL, &flags)<0) {
            *outptr++=errno;
            complain("init_trig_zelsync: O_NONBLOCK: %s", strerror(errno));
            pres=plErr_System;
            goto error;
        }
    }

    bzero(&priv->pci_oldtrigdata, sizeof(struct trigstatus));

    if (priv->master) {
        u_int32_t flags[2];
        if (ioctl(priv->path, PCISYNC_MASTER)<0) {
            *outptr++=errno;
            complain("init_trig_zelsync: PCISYNC_MASTER: %s", strerror(errno));
            pres=plErr_System;
            goto error;
        }
        flags[0]=0;
        flags[1]=0;
        if (priv->flags & flag_t4)
            flags[0]|=SYNC_SET_T4LATCH;
        if (priv->flags & flag_taw)
            flags[0]|=SYNC_SET_TAW_ENA;
        if (flags[0]) {
            if (ioctl(priv->path, PCISYNC_CSRBITS, flags)<0) {
                *outptr++=errno;
                complain("init_trig_zelsync: CSRBITS: %s", strerror(errno));
                pres=plErr_System;
                goto error;
            }
        }
    }

    if (ioctl(priv->path, PCISYNC_CONVTIME, &priv->conversiontime)<0) {
        *outptr++=errno;
        complain("init_trig_zelsync: CONVTIME: %s", strerror(errno));
        pres=plErr_System;
        goto error;
    }

    if (priv->mode==sync_signal) {
        union SigHdlArg sigarg;
        struct syncintctrl *intctrl=&priv->intctrl;

        priv->sigtrig=0;

        sigarg.ptr=tinfo;
        intctrl->signal=install_signalhandler(sighndlr, sigarg, "trigger");
        if (intctrl->signal<=0) {
            printf("can't install signalhandler for syncmod.\n");
            *outptr++=4;
            pres=plErr_Other;
            goto error;
        } else {
            printf("signalhandler installed; signal=%d\n", intctrl->signal);
        }
        if (priv->flags&flag_tint) {
            intctrl->signalmask=SYNC_MBX_TRIGG;
            intctrl->selectmask=SYNC_MBX_TRIGG;
            intctrl->readmask=SYNC_MBX_TRIGG;
        } else {
            intctrl->signalmask=SYNC_MBX_EOC;
            intctrl->selectmask=SYNC_MBX_EOC;
            intctrl->readmask=SYNC_MBX_EOC;
        }
        if (ioctl(priv->path, PCISYNC_INTCTRL, &intctrl)<0) {
            *outptr++=errno;
            complain("init_trig_zelsync: INTCTRL: %s", strerror(errno));
            *outptr++=5;
            pres=plErr_System;
            goto error;
        }
    }

    switch (priv->mode) {
    case sync_poll:
        tinfo->proctype=tproc_poll;
        break;
    case sync_signal:
        tinfo->proctype=tproc_signal;
        break;
    case sync_select:
        tinfo->proctype=tproc_select;
        tinfo->i.tp_select.path=priv->path;
        tinfo->i.tp_select.seltype=select_read;
        break;
    }
    tinfo->get_trigger=get_trig_sharedzelsync;
    tinfo->reset_trigger=reset_trig_sharedzelsync;
    tinfo->done_trigger=done_trig_sharedzelsync;

    if (ioctl(priv->path, PCISYNC_START)<0) {
        *outptr++=errno;
        complain("init_trig_zelsync: START: %s", strerror(errno));
        *outptr++=5;
        pres=plErr_System;
        goto error;
    }

    return plOK;

error:
    if (priv->intctrl.signal>0)
        remove_signalhandler(priv->intctrl.signal);
    if (priv->path>=0)
        close(priv->path);
    free(priv->pathname);
    free(priv);
    return pres;
}
/*****************************************************************************/

char name_trig_sharedzelsync[]="zel_shared";
int ver_trig_sharedzelsync=1;

/*****************************************************************************/
/*****************************************************************************/
