/*
 * trigger/pci/zelsync/zelsynctrigger.c
 * created 02.10.96 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: zelsynctrigger.c,v 1.29 2017/10/21 22:37:36 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <errorcodes.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
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
#include "../../../lowlevel/sync/pci_zel/synchrolib.h"
#include "../../../lowlevel/sync/pci_zel/sync_pci_zel.h"
#include <dev/pci/zelsync.h>
#include "zelsynctrigger.h"

#ifdef DMALLOC
#include dmalloc.h
#endif

RCS_REGISTER(cvsid, "trigger/pci/zelsync")

#define MAINSTAT SYNC_GET_MASTER|SYNC_GET_GO|SYNC_GET_T4LATCH|SYNC_GET_TAW_ENA|\
    SYNC_GET_AUX_OUT|\
    SYNC_GET_EN_INT

struct synctestcontrol {
    int softtrigger; /* 0: kein Softtrigger*/
                     /* 1...: zu erzeugender Trigger; Hardware nicht benutzt */
    int set_aux_out; /* 0: aux_out nicht benutzt */
                     /* 1..7: aux_out wird auf diesen Wert gesetzt */
    int warp_id;     /* !=0: incr trigger_id every (warp_id) events */
};

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
    unsigned int conversiontime;
    unsigned int fcatime;
    int flags;
    int softtrigg;
    int testcontrolvar;
    struct synctestcontrol* testcontrol;
    char* device;
    int sigtrig;
    struct synctestcontrol* testdata;
    struct syncmod_info* info;
    int id; /* device id */
    volatile ems_u32 *base;
#ifdef SYNCSTATIST
    struct syncstatistics lsyncstat; /* local teadtime */
    struct syncstatistics tsyncstat; /* total teadtime */
    struct syncstatistics gsyncstat; /* trigger gaps */
    ems_u64 zelsyncrejected;
#endif
    struct trigstatus pci_trigdata, pci_oldtrigdata;
#ifdef PCISYNC_PROFILE /* defined in sys/dev/pci/pcisyncvar.h */
    struct trigprofdata pcisync_profdata;
    struct trigprofdata pcisync_profdata_new;
#endif
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
int print_zelsync_status(struct triggerinfo* trinfo, ems_u32** ptr, int text)
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
fatal(__attribute__((unused)) struct triggerinfo* trinfo, int code,
        unsigned int num, ...)
{
    ems_u32 msg[16];
    va_list ap;
    unsigned int i;

    msg[0]=rtErr_Trig;
    msg[1]=global_evc.ev_count;
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
int get_pci_trigdata(struct triggerinfo* trinfo,
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
int get_syncstatist(struct triggerinfo* trinfo,
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
void clear_syncstatist(struct triggerinfo* trinfo, enum syncstatist_types typ)
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
plerrcode set_syncstatist(struct triggerinfo* trinfo,
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
    res=set_syncstatist(trinfo, syncstatist_ldt|syncstatist_tdt|syncstatist_gap,
                3000, 0);
    return res;
}
/*****************************************************************************/
struct syncstatistics* get_syncstatist_ptr(struct triggerinfo* trinfo,
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
#ifdef OBJ_VAR
static plerrcode inittest(int var, struct synctestcontrol** control)
{
    plerrcode res;
    unsigned int vsize;

    T(inittest)
    res=var_attrib(var, &vsize);
    if (res==plOK) {
        if (vsize!=sizeof(struct synctestcontrol)/sizeof(int)) {
            printf("variable %d for testcontrol: size=%d; %lld expected\n",
                var, vsize,
                (unsigned long long)(sizeof(struct synctestcontrol)/
                        sizeof(int)));
            *outptr++=var;
            return plErr_IllVarSize;
        }
    } else if(res==plErr_NoVar) {
        struct synctestcontrol* c;
        if ((res=var_create(var, sizeof(struct synctestcontrol)/sizeof(int)))
                !=plOK) {
            printf("cannot create variable %d for testcontrol\n", var);
            *outptr++=var;
            return res;
        }
        c=(struct synctestcontrol*)var_list[var].var.ptr;
        c->softtrigger=0;
        c->set_aux_out=0;
    }
    *control=(struct synctestcontrol*)var_list[var].var.ptr;
    return plOK;
}
#endif
/*****************************************************************************/
static void sighndlr(union SigHdlArg arg, __attribute__((unused)) int sig)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)arg.ptr;
    struct private* priv=(struct private*)tinfo->private;
    priv->sigtrig=1;
}
/*****************************************************************************/
static plerrcode
done_trig_zelsync(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    T(done_trig_zelsync)

    if (!priv->softtrigg) {
        if (priv->info->intctrl.signal) {
            remove_signalhandler(priv->info->intctrl.signal);
        }

        priv->base[SYNC_SSCR]=SYNC_RES_GO;
        /* syncmod_reset(priv->id); */
        syncmod_detach(priv->id);
        free(priv->device);
    }
    free(tinfo->private);
    tinfo->private=0;
    return plOK;
}
/*****************************************************************************/
static int
get_trig_zelsync(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    unsigned int trigger;

    if (priv->softtrigg) {
        trinfo->count++;
        return priv->softtrigg;
    }

#ifdef SYNC_PROFILE
trigprofdata.s0=base[SYNC_TMC];
trigprofdata.t[0]+=trigprofdata.s0;
#endif

    switch (priv->mode) {
    case sync_signal: {
        unsigned int got;
        int res;

        if (!priv->sigtrig) {
            if ((priv->testdata && priv->testdata->set_aux_out) &&
                    ((priv->base[SYNC_CSR]&SYNC_GET_TDT_RING)==0)) {
                /* setzen */
                priv->base[SYNC_SSCR]=(priv->testdata->set_aux_out&7)<<4;
                /* gleich wieder zurueck */
                priv->base[SYNC_SSCR]=(priv->testdata->set_aux_out&7)<<20;
            }
            return(0);
        }
        priv->sigtrig=0;
        got=0;
        while (got<sizeof(struct trigstatus)) {
            res=read(priv->info->path, ((char*)&priv->pci_trigdata)+got,
                    sizeof(struct trigstatus)-got);
            if (res<0) {
                printf("read trigstatus: %s\n", strerror(errno));
                fatal(trinfo, 1, 1, errno);
                return(0);
            }
            got+=res;
        }
        if ((priv->pci_trigdata.mbx&SYNC_MBX_EOC)==0) {
            printf("got signal; but mbx is %08x (%s)\n",priv->pci_trigdata.mbx,
                    getbits(priv->pci_trigdata.mbx));
            fatal(trinfo, 2, 2, priv->pci_trigdata.mbx,
                    priv->pci_trigdata.state);
            return(0);
        }
        if ((priv->pci_trigdata.state&SYNC_GET_EOC)==0) {
            printf("got signal; but stat is %08x (%s)\n",
                    priv->pci_trigdata.state,
                    getbits(priv->pci_trigdata.state));
            printf("got: %08x %08x %08x\n", priv->pci_trigdata.mbx,
                    priv->pci_trigdata.state,
                    priv->pci_trigdata.evc);
            fatal(trinfo, 3, 2, priv->pci_trigdata.mbx,
                    priv->pci_trigdata.state);
            return(0);
        }
    }
    break;
    case sync_poll: {
        int i;
        if (((priv->pci_trigdata.state=priv->base[SYNC_CSR])&SYNC_GET_EOC)==0) {
            /* noch kein Trigger gekommen */
            if ((priv->testdata && priv->testdata->set_aux_out) &&
                    ((priv->pci_trigdata.state&SYNC_GET_TDT_RING)==0)) {
                /* wir machen uns selbst einen Trigger */
                unsigned int v, w;
                v=priv->base[SYNC_SSCR];
                if (((v&SYNC_GET_AUX_OUT)>>4)!=0) {
                    printf("Fehler1: CR=0x%08x\n", v);
                }
                w=(priv->testdata->set_aux_out&7)<<4;
                priv->base[SYNC_SSCR]=w;  /* setzen */
                v=priv->base[SYNC_SSCR];
                if (((v&SYNC_GET_AUX_OUT)>>4)!=(priv->testdata->set_aux_out&7)) {
                    printf("Fehler2: CR=0x%08x\n", v);
                }
                w=(priv->testdata->set_aux_out&7)<<20;
                priv->base[SYNC_SSCR]=w; /* gleich wieder zurueck */
                v=priv->base[SYNC_SSCR];
                if (((v&SYNC_GET_AUX_OUT)>>4)!=0) {
                    printf("Fehler3: CR=0x%08x\n", v);
                }
            }
            return(0);
        }
        priv->pci_trigdata.mbx=0;
        priv->pci_trigdata.state=priv->base[SYNC_CSR];
        priv->pci_trigdata.evc=priv->base[SYNC_EVC];
        priv->pci_trigdata.tmc=priv->base[SYNC_TMC];
        priv->pci_trigdata.tdt=priv->base[SYNC_TDTR];
        priv->pci_trigdata.reje=priv->base[SYNC_REJE];
        i=0;
        while ((i<3)
                && ((priv->pci_trigdata.tgap[i++]=
                                    priv->base[SYNC_TGAP])&SYNC_GAP_MORE));
    }
    break;
    case sync_select: {
        int got;
        int SYNC_MBX_flag;

        got=read(priv->info->path, (char*)&priv->pci_trigdata,
                sizeof(struct trigstatus));
        /*printf("nach read: "); decode_zelsync_status(base[SYNC_CSR]);*/
        if (got!=sizeof(struct trigstatus)) {
            printf("read trigstatus: got=%d: %s\n", got, strerror(errno));
            fatal(trinfo, 5, 1, errno);
            return(0);
        }
        priv->pci_oldtrigdata.tdt=priv->pci_trigdata.tdt;
#ifdef PCISYNC_PROFILE
        pcisync_profdata=pcisync_profdata_new;
        pcisync_profdata_new=priv->pci_trigdata.data;
#endif
        SYNC_MBX_flag=(priv->flags&flag_tint)?SYNC_MBX_TRIGG:SYNC_MBX_EOC;
        if ((priv->pci_trigdata.mbx&SYNC_MBX_flag)==0) {
            statusprint(priv->pci_trigdata.state, "mailbox error (ignored)");
            if (priv->base[SYNC_CSR]&SYNC_GET_INH) {
                priv->base[SYNC_CSR]=SYNC_RES_DEADTIME|SYNC_RES_TRIGINT;
                statusprint(priv->base[SYNC_CSR], "reset local inhibit");
            }
            /* fatal_readout_error(); */
            return(0);
        }
        if (((priv->pci_trigdata.state&SYNC_GET_EOC)==0) 
                || ((priv->pci_trigdata.state&SYNC_GET_INH)==0)) {
            if (priv->flags&flag_tint) {
                int cc=0;
                printf("no EOC\n");
                while ((priv->base[SYNC_CSR]&SYNC_GET_EOC)==0) cc++;
                printf("cc=%d\n", cc);
            } else {
                statusprint(priv->pci_trigdata.state, "no EOC or no INH");
                fatal(trinfo, 9, 2, priv->pci_trigdata.mbx,
                        priv->pci_trigdata.state);
                return(0);
            }
        }
    }
    break;
    }

    if (trinfo->count!=priv->pci_trigdata.evc) {
        printf("zelsynctrigger: eventcnt: %d ==> %d\n",
                trinfo->count, priv->pci_trigdata.evc);
        statusprint(priv->pci_trigdata.state, 0);
        fatal(trinfo, 6, 3, priv->pci_trigdata.mbx, priv->pci_trigdata.state,
                priv->pci_trigdata.evc);
        /* eventcnt=priv->pci_trigdata.evc; */
        return(0);
    }
    if (priv->pci_trigdata.state&(SYNC_GET_SI_RING|SYNC_GET_SI)) {
        statusprint(priv->pci_trigdata.state, "Subenvent invalid");
        fatal(trinfo, 7, 2, priv->pci_trigdata.mbx, priv->pci_trigdata.state);
        return(0);
    }

    trigger=(priv->pci_trigdata.state&SYNC_GET_TRIG)>>16;
    if (trigger==0) {
        statusprint(priv->pci_trigdata.state, "got no trigger");
        fatal(trinfo, 8, 2, priv->pci_trigdata.mbx, priv->pci_trigdata.state);
        return(0);
    }
    trinfo->count++;

    if (priv->testdata && priv->testdata->warp_id) {
        if (trinfo->count%priv->testdata->warp_id==0) {
            trigger+=16;
            printf("trigger set to %d (ev=%d)\n", trigger, trinfo->count);
        }
    }
    return trigger;
}

/*****************************************************************************/
static void
reset_trig_zelsync(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;

    if (priv->softtrigg)
        return;

/*
 * if (priv->master)
 *   priv->pci_trigdata.ldt=base[SYNC_TMC];
 * else
 *   {
 *   if (base[SYNC_CSR]&SYNC_GET_TDT_RING)
 *     {
 *     priv->pci_trigdata.ldt=base[SYNC_TMC];
 *     if ((base[SYNC_CSR]&SYNC_GET_TDT_RING)==0) * inzwischen abgelaufen *
 *       {
 *       priv->pci_trigdata.ldt=base[SYNC_TMC]+base[SYNC_TDTR];
 *       }
 *     }
 *   else
 *     {
 *     priv->pci_trigdata.ldt=base[SYNC_TMC]+base[SYNC_TDTR];
 *     }
 *   }
 */

    priv->pci_trigdata.ldt=priv->base[SYNC_TMC];

    /* trigger wieder freigeben */
    priv->base[SYNC_CSR]=SYNC_RES_DEADTIME|SYNC_RES_TRIGINT;
    /*priv->base[SYNC_SSCR]=SYNC_RES_AUX_OUT0;*/
    priv->pci_oldtrigdata=priv->pci_trigdata;

#ifdef SYNCSTATIST
    {
        /* !!! tdt belongs to the previous event */
        unsigned int tdt=priv->pci_trigdata.tdt;
        unsigned int ldt=priv->pci_trigdata.ldt;

        priv->zelsyncrejected+=priv->pci_trigdata.reje;
        if (trinfo->count>2) {
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
                    printf("ev. %d: tdt=0\n", trinfo->count);
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
                    unsigned int gap=priv->pci_trigdata.tgap[i]&0xffff;
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
 * p[2] (boolean) master
 * p[3] (integer) ctime
 * [p[4] (integer) 0: poll; 1: signal; 2: select
 * [p[5] (bitmask) RAUX0|T4LATCH|TAW|TINT
 *       (0x8) RAUX0  : AUX_IN0 resets trigger
 *       (0x4) T4LATCH: Trigger 4 is latched
 *       (0x2) TAW    : use trigger acceptance window
 *       (0x1) TINT   : use trigger interrupt, not EOC interrupt
 * [p[6] (integer) fast clear acceptance time
 * [p[7] (integer) control_var fuer tests]]]]
 */

plerrcode init_trig_zelsync(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv;
    ems_u32* pp;
    unsigned int stat;
    int rest;
    plerrcode pres=plOK;

    priv=calloc(1, sizeof(struct private));
    if (!priv) {
        printf("init_trig_zelsync: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    priv->id=-1;
    tinfo->private=priv;
    tinfo->magic=init_trig_zelsync;

    if (p[0]<1) {
        printf("init_trig_zelsync: no parameters\n");
        *outptr++=1;
        pres=plErr_ArgNum;
        goto error;
    }
    if (xdrstrlen(p+1)<1) {
        printf("init_trig_zelsync: device name ist %d characters long.\n",
                xdrstrlen(p+1));
        *outptr++=2;
        pres=plErr_ArgNum;
    }
    rest=p[0]-xdrstrlen(p+1);

    if (rest<2) {
        printf("init_trig_zelsync: device name needs %d words, "
                "but I have only %d\n",
                xdrstrlen(p+1), p[0]);
        *outptr++=3;
        pres=plErr_ArgNum;
        goto error;
    }
    pp=xdrstrcdup(&priv->device, p+1);
    if (priv->device==0) {
        pres=plErr_NoMem;
        goto error;
    }
    priv->master=pp[0];

    priv->conversiontime=pp[1];
    if (priv->conversiontime&0xff000000) {
        printf("init_trig_zelsync: Conversion time 0x%08x is too large.\n",
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
        printf("init_trig_zelsync: mode %d is not known\n", priv->mode);
        *outptr++=2;
        pres=plErr_ArgRange;
        goto error;
    }

    if (rest>3)
        priv->flags=pp[3];
    else
        priv->flags=0;
    if (rest>4)
        priv->fcatime=pp[4];
    else
        priv->fcatime=0;

    if (rest>5) {
#ifndef OBJ_VAR
        pres=plErr_IllVar;
        goto error;
#else
        priv->testcontrolvar=pp[5];
        if ((pres=inittest(priv->testcontrolvar, &priv->testcontrol))!=plOK)
            goto error;

        priv->testdata=priv->testcontrol;
        priv->softtrigg=priv->testdata->softtrigger;
#endif
    } else {
        priv->testdata=0;
        priv->softtrigg=0;
    }
    trinfo->count=0;

#ifdef SYNCSTATIST
    if ((pres=initstatistic(trinfo))!=plOK)
        return pres;
#endif

    if (priv->softtrigg) {
        tinfo->proctype=tproc_poll;
        tinfo->get_trigger=get_trig_zelsync;
        tinfo->reset_trigger=reset_trig_zelsync;
        tinfo->done_trigger=done_trig_zelsync;
        return plOK;
    }

    if ((pres=syncmod_attach(priv->device, &priv->id))!=plOK) {
        printf("init_trig_zelsync: attach syncmodule: %s\n", strerror(errno));
        *outptr++=errno;
        goto error;
    }

    priv->info=syncmod_getinfo(priv->id);
    priv->base=priv->info->base;
/*
 * {
 * int flags, oflags;
 * int got;
 * if (fcntl(priv->info->path, F_GETFL, &oflags)<0)
 *   {
 *   printf("init_trig_zelsync: can't get flags: %s\n", strerror(errno));
 *   goto mist;
 *   }
 * printf("init_trig_zelsync: original flags=0x%08x\n", flags);
 * if (fcntl(priv->info->path, F_SETFL, O_NDELAY)<0)
 *   {
 *   printf("init_trig_zelsync: can't set O_NDELAY: %s\n", strerror(errno));
 *   goto mist;
 *   }
 * if (fcntl(priv->info->path, F_GETFL, &flags)<0)
 *   {
 *   printf("init_trig_zelsync: can't get flags: %s\n", strerror(errno));
 *   goto mist;
 *   }
 * printf("flags after NDELAY=0x%08x\n", flags);
 * got=read(priv->info->path, (char*)&priv->pci_trigdata, sizeof(struct trigstatus));
 * printf("dummy trigstatus: got=%d: %s\n", got, got<0?strerror(errno):"ok");
 * if (got>0) print_zelsync_status(0, 1);
 * if (fcntl(priv->info->path, F_SETFL, oflags)<0)
 *   {
 *   printf("init_trig_zelsync: can't set flags: %s\n", strerror(errno));
 *   goto mist;
 *   }
 * if (fcntl(priv->info->path, F_GETFL, &flags)<0)
 *   {
 *   printf("init_trig_zelsync: can't get flags: %s\n", strerror(errno));
 *   goto mist;
 *   }
 * printf("init_trig_zelsync: flags=0x%08x\n", flags);
 * mist:;
 * }
 */

    bzero(&priv->pci_oldtrigdata, sizeof(struct trigstatus));

    syncmod_init(priv->id, priv->master);
    stat=priv->base[SYNC_CSR];

    if (((stat&0xffff)|SYNC_GET_MASTER)!=SYNC_GET_MASTER) {
        statusprint(stat, "after reset (a)");
        *outptr++=1;
        pres=plErr_HWTestFailed;
        goto error;
    }

    if (priv->master) {
        if ((stat&0xffff)==0) {
            printf("init_trig_zelsync: cannot become syncmaster.\n");
            pres=plErr_HWTestFailed;
            goto error;
        }
        if ((stat&0xffff)!=SYNC_GET_MASTER) {
            statusprint(stat, "after reset (b)");
            *outptr++=1;
            pres=plErr_HWTestFailed;
            goto error;
        }
    } else {
        if ((stat&0xffff)!=0) {
            statusprint(stat, "after reset (c)");
            *outptr++=1;
            pres=plErr_HWTestFailed;
            goto error;
        }
    }

    priv->base[SYNC_FCAT]=priv->fcatime;
    stat=priv->base[SYNC_FCAT];
    if (stat!=priv->fcatime) {
        printf("init_trig_zelsync: can't set fast clear acceptance time: 0x%08x ==> 0x%08x\n",
            priv->fcatime, stat);
        *outptr++=3;
        pres=plErr_HWTestFailed;
        goto error;
    }

    priv->base[SYNC_CVTR]=priv->conversiontime;
    stat=priv->base[SYNC_CVTR];
    if (stat!=priv->conversiontime) {
        printf("init_trig_zelsync: can't set conversiontime: 0x%08x ==> 0x%08x\n",
            priv->conversiontime, stat);
        *outptr++=3;
        pres=plErr_HWTestFailed;
        goto error;
    }

    if (priv->flags&flag_t4) {
        priv->base[SYNC_SSCR]=SYNC_SET_T4LATCH;
        stat=priv->base[SYNC_CSR];
        if ((stat&SYNC_SET_T4LATCH)==0) {
            statusprint(stat, "after enable t4 latch");
            *outptr++=6;
            pres=plErr_HWTestFailed;
            goto error;
        }
        printf("init_trig_zelsync: t4 latch enabled\n");
    }

    if (priv->flags&flag_taw) {
        priv->base[SYNC_SSCR]=SYNC_SET_TAW_ENA;
        stat=priv->base[SYNC_CSR];
        if ((stat&SYNC_SET_TAW_ENA)==0) {
            statusprint(stat, "after enable trigger acceptance window");
            *outptr++=6;
            pres=plErr_HWTestFailed;
            goto error;
        }
        printf("init_trig_zelsync: trigger acceptance window enabled\n");
    }

    {
        int i;
        for (i=0; (priv->base[SYNC_CSR]&SYNC_GET_GAP) && (i<4); i++)
            stat=priv->base[SYNC_TGAP];
    }

    {
        /* read and discard old data */
        struct trigstatus trigstatus;
        struct timeval tv;
        fd_set readfds;
        int res;

        do {
            tv.tv_sec=tv.tv_usec=0;
            FD_ZERO(&readfds);
            FD_SET(priv->info->path, &readfds);
            res=select(priv->info->path+1, &readfds, 0, 0, &tv);
            switch (res) {
            case -1:
                perror("zelsynctrigger.c:init: select");
                break;
            case 0:
                break;
            default:
                res=read(priv->info->path, &trigstatus, sizeof(struct trigstatus));
                printf("zelsynctrigger.c:init: preread: res=%d\n", res);
            }
        } while (res>0);
    }

    stat=priv->base[SYNC_CSR];
    if (stat&SYNC_GET_GO_RING) {
        statusprint(stat, "GO from ring: wrong initialisation sequence "
                "or no master present.");
        *outptr++=4;
        pres=plErr_HWTestFailed;
        goto error;
    }
    if (stat&SYNC_GET_TRIG) {
        printf("spurious trigger from ring.\n");
        printf("status 0x%08x: %s\n", stat, getbits(stat));
        *outptr++=5;
        pres=plErr_HWTestFailed;
        goto error;
    }

    if (priv->mode==sync_signal) {
        int sig;
        union SigHdlArg sigarg;
        struct syncintctrl intctrl;

        priv->sigtrig=0;

        sigarg.ptr=tinfo;
        sig=install_signalhandler(sighndlr, sigarg, "trigger");
        if (sig==-1) {
            printf("can't install signalhandler for syncmod.\n");
            *outptr++=4;
            pres=plErr_Other;
            goto error;
        } else {
            printf("signalhandler installed; signal=%d\n", sig);
        }
        intctrl.signal=sig;
        if (priv->flags&flag_tint) {
            intctrl.signalmask=SYNC_MBX_TRIGG;
            intctrl.selectmask=SYNC_MBX_TRIGG;
            intctrl.readmask=SYNC_MBX_TRIGG;
        } else {
            intctrl.signalmask=SYNC_MBX_EOC;
            intctrl.selectmask=SYNC_MBX_EOC;
            intctrl.readmask=SYNC_MBX_EOC;
        }
        if (syncmod_intctrl(priv->info, &intctrl)<0) {
            printf("can't set intctrl for syncmod: %s\n", strerror(errno));
            *outptr++=errno;
            remove_signalhandler(sig);
            *outptr++=5;
            pres=plErr_System;
            goto error;
        }
    }

    if ((priv->mode==sync_signal) || (priv->mode==sync_select)) {
        if (priv->flags&flag_tint) {
            priv->base[SYNC_SSCR]=SYNC_EN_INT_TRIGG;
            stat=priv->base[SYNC_CSR];
            if ((stat&SYNC_EN_INT_TRIGG)==0) {
                statusprint(stat, "after enable trigg_int");
                *outptr++=6;
                pres=plErr_HWTestFailed;
                goto error;
            }
        } else {
            priv->base[SYNC_SSCR]=SYNC_EN_INT_EOC;
            stat=priv->base[SYNC_CSR];
            if ((stat&SYNC_EN_INT_EOC)==0) {
                statusprint(stat, "after enable eoc_int");
                *outptr++=6;
                pres=plErr_HWTestFailed;
                goto error;
            }
        }
    }

    if (priv->master) {
        register int c=0, stat;
        priv->base[SYNC_CSR]=SYNC_RES_DEADTIME|SYNC_RES_TRIGINT;
        priv->base[SYNC_SSCR]=SYNC_SET_GO;
        while ((((stat=priv->base[SYNC_CSR])&SYNC_GET_GO_RING)==0) && (++c<10000));
        if ((stat&SYNC_GET_GO)==0) {
            statusprint(stat, "after go");
            *outptr++=7;
            pres=plErr_HWTestFailed;
            goto error;
        }
        if ((stat&SYNC_GET_GO_RING)==0) {
            printf("no GO from ring: ring open?\n");
            *outptr++=8;
            pres=plErr_HWTestFailed;
            goto error;
        }
    } else {
        priv->base[SYNC_SSCR]=SYNC_SET_GO;
        stat=priv->base[SYNC_CSR];
        if ((stat&SYNC_GET_GO)==0) {
            statusprint(stat, "after go");
            *outptr++=7;
            pres=plErr_HWTestFailed;
            goto error;
        }
        if ((stat&SYNC_GET_GO_RING)!=0) {
            statusprint(stat, "GO from ring: wrong initialisation sequence");
            *outptr++=9;
            pres=plErr_HWTestFailed;
            goto error;
        }
    }

    if (priv->base[SYNC_CSR]&SYNC_GET_INH)
        priv->base[SYNC_CSR]=SYNC_RES_DEADTIME|SYNC_RES_TRIGINT;

    switch (priv->mode) {
    case sync_poll:
        tinfo->proctype=tproc_poll;
        break;
    case sync_signal:
        tinfo->proctype=tproc_signal;
        break;
    case sync_select:
        tinfo->proctype=tproc_select;
        tinfo->i.tp_select.path=priv->info->path;
        tinfo->i.tp_select.seltype=select_read;
        break;
    }
    tinfo->get_trigger=get_trig_zelsync;
    tinfo->reset_trigger=reset_trig_zelsync;
    tinfo->done_trigger=done_trig_zelsync;

    statusprint(priv->base[SYNC_CSR], "Status at end of init");
    return plOK;

error:
    if (priv->id>=0)
        syncmod_detach(priv->id);
    free(priv->device);
    free(priv);
    return pres;
}
/*****************************************************************************/

char name_trig_zelsync[]="zel";
int ver_trig_zelsync=1;

/*****************************************************************************/
/*****************************************************************************/
