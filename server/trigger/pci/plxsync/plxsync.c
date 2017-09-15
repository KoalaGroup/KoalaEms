/*
 * trigger/pci/plxsync/plxsync.c
 * created 2007-Jun-23 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: plxsync.c,v 1.10 2011/04/06 20:30:36 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <objecttypes.h>
#include <errorcodes.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
#include "../../../main/signals.h"
#include "../../../main/scheduler.h"
#include "../../trigger.h"
#include "../../trigprocs.h"
#include "../../../objects/pi/readout.h"
#include "../../../commu/commu.h"
#include "../../../lowlevel/devices.h"
#include <xdrstring.h>

#include "../../../lowlevel/sync/syncdev.h"
#include "../../../lowlevel/sync/plxsync/plxsync.h"
#include <dev/pci/plxsync_var.h>
#include <dev/pci/plxsync_map.h>

enum trigg_mode {trig_none=-1, trigg_poll=0, trigg_signal=1, trigg_select=2};

struct private {
    char* device;
    int p;
    enum trigg_mode mode;
    int flags;
    int signal;
    int got_sig;
};

extern ems_u32* outptr;

#define ofs(what, elem) ((size_t)&(((what *)0)->elem))

RCS_REGISTER(cvsid, "trigger/pci/plxsync")

#define RINGMASK 0x7
#define RINGLEN (RINGMASK+1)
#define RINGLEN2 (RINGLEN/2)
static ems_u32 ev_ring[RINGLEN];
static ems_u32 ev_list[RINGLEN];
static int evridx, evlidx;
/*****************************************************************************/
static void
init_ev_ring(void)
{
    int i;
    for (i=0; i<RINGLEN ; i++)
        ev_ring[i]=-1;
    evridx=0;
    evlidx=-1;
}
/*****************************************************************************/
static void
save_ev_ring(void)
{
    int i, idx;
    for (i=0, idx=evridx+RINGLEN2; i<RINGLEN2; i++, idx++) {
        ev_list[i]=ev_ring[idx&RINGMASK];
    }
    evlidx=RINGLEN2;
}
/*****************************************************************************/
static void
push_evnr(ems_u32 evnr)
{
    ev_ring[evridx]=evnr;
    evridx=(evridx+1)&RINGMASK;
    if (evlidx<0)
        return;

    ev_list[evlidx++]=evnr;
    if (evlidx>=RINGLEN) {
        printf("eventnumbers:\n");
        for (evlidx=0; evlidx<RINGLEN; evlidx++) {
            printf("  [%2d] %10u %08x", evlidx, ev_list[evlidx],
                    ev_list[evlidx]);
            if (evlidx==RINGLEN2-1) {
                ems_u32 ist=ev_list[evlidx];
                ems_u32 soll=ev_list[evlidx-1]+1;
                ems_u32 diff_a, diff_b;
                if (ist/2==soll)
                    printf("  <<1");
                if (ist==soll/2)
                    printf("  >>1");
                diff_a=ist-soll;
                diff_b=soll-ist;
                //printf(" (%08x %08x) ", diff_a, diff_b);
                if (diff_a && !(diff_a&(diff_a-1)))
                    printf("  bit %d illegal (%08x)", ffs(diff_a), diff_a);
                if (diff_b && !(diff_b&(diff_b-1))) {
                    printf("  bit %d missing (%08x)", ffs(diff_b), diff_b);
                }
            }
            printf("\n");
        }
        evlidx=-1;
    }
}
/*****************************************************************************/
static void sighndlr(union SigHdlArg arg, int sig)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)arg.ptr;
    struct private* priv=(struct private*)tinfo->private;
    priv->got_sig=1;
}
/*****************************************************************************/
static plerrcode
done_trig_plxsync(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    u_int32_t ui32;

    T(done_trig_plxsync)

    ui32=0;
    if (ioctl(priv->p, PLXSYNC_ALLOW_IRQS, &ui32)<0) {
        printf("trig_plxsync: ioctl(%s, ALLOW_IRQS): %s\n", priv->device,
                strerror(errno));
    }
    if (priv->signal>0)
        remove_signalhandler(priv->signal);
    close(priv->p);
    free(priv->device);
    free(priv);
    return plOK;
}
/*****************************************************************************/
static int
get_trig_plxsync(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    struct plxsync_ctrl_reg reg;
    struct plxsync_exirqinfo info;
    u_int32_t trigger=1;
    int res;

#if 0
    printf("get_trig_plxsync: called\n");
#endif

    switch (priv->mode) {
    case trigg_poll:
        reg.offset=ofs(struct plxsync_reg, sr);
        if (ioctl(priv->p, PLXSYNC_CTRL_READ, &reg)<0) {
            printf("trig_plxsync: CTRL_READ: %s\n", strerror(errno));
            send_unsol_var(Unsol_RuntimeError, 4, rtErr_Trig, trinfo->eventcnt,
                    4, errno);
            fatal_readout_error();
            return 0;
        }
        if (reg.val&(sr_err|irq_err|irq_tout)) {
            printf("trig_plxsync: unexpected status: 0x%2x\n", reg.val);
            send_unsol_var(Unsol_RuntimeError, 4, rtErr_Trig, trinfo->eventcnt,
                    5, errno);
            //fatal_readout_error();
            //return 0;
        }
        if (!(reg.val&sr_busy))
            return 0;

        reg.offset=ofs(struct plxsync_reg, evc);
        if (ioctl(priv->p, PLXSYNC_CTRL_READ, &reg)<0) {
            printf("trig_plxsync: CTRL_READ: %s\n", strerror(errno));
            send_unsol_var(Unsol_RuntimeError, 4, rtErr_Trig, trinfo->eventcnt,
                    4, errno);
            fatal_readout_error();
            return 0;
        }
        trinfo->eventcnt=reg.val;
        if (priv->flags&1) {
            reg.offset=ofs(struct plxsync_reg, tpat);
            if (ioctl(priv->p, PLXSYNC_CTRL_READ, &reg)<0) {
                printf("trig_plxsync: CTRL_READ: %s\n", strerror(errno));
                send_unsol_var(Unsol_RuntimeError, 4, rtErr_Trig, trinfo->eventcnt,
                        4, errno);
                fatal_readout_error();
                return 0;
            }
            trigger=reg.val;
        }
        break;

    case trigg_signal:
        if (!priv->got_sig)
            return 0;

        res=read(priv->p, &info, sizeof(struct plxsync_exirqinfo));
        if ((res<0)&&(errno==EAGAIN))
            return 0;
        if (res<0) {
            printf("trig_plxsync: read: %s\n", strerror(errno));
            send_unsol_var(Unsol_RuntimeError, 4, rtErr_Trig, trinfo->eventcnt,
                    4, errno);
            fatal_readout_error();
            return 0;
        }
        if ((info.state&(sr_err|irq_err|irq_tout))||
            ((info.state&(sr_busy|irq_busy))!=(sr_busy|irq_busy))) {
            printf("trig_plxsync: unexpected status: 0x%02x\n", info.state);
            send_unsol_var(Unsol_RuntimeError, 4, rtErr_Trig, trinfo->eventcnt,
                    5, errno);
            fatal_readout_error();
            return 0;
        }
        trinfo->eventcnt=info.evc;
        trinfo->time.tv_sec=info.irq_sec;
        trinfo->time.tv_nsec=info.irq_nsec;
        trinfo->time_valid=1;
        if (priv->flags&1)
            trigger=info.tpat;
        break;

    case trigg_select:
        res=read(priv->p, &info, sizeof(struct plxsync_exirqinfo));
        if ((res<0)&&(errno==EAGAIN)) {
            printf("trig_plxsync: EAGAIN\n");
            return 0;
        }
        if (res<0) {
            printf("trig_plxsync: read: %s\n", strerror(errno));
            send_unsol_var(Unsol_RuntimeError, 4, rtErr_Trig, trinfo->eventcnt,
                    4, errno);
            fatal_readout_error();
            return 0;
        }
#if 0
        printf("trig_plxsync: state=0x%02x evc=%d\n", info.state, info.evc);
#endif
        push_evnr(info.evc);
        if ((info.state&(sr_err|irq_err|irq_tout))||
            ((info.state&(sr_busy|irq_busy))!=(sr_busy|irq_busy))) {
            char ss[256];
            sprintf(ss, "trig_plxsync: unexpected status: 0x%02x in event %u "
                    "after %u", info.state, info.evc, trinfo->eventcnt);
            printf("%s\n", ss);
            send_unsol_text("RuntimeError (probably nonfatal, check eventrate!)",
                            ss, 0);
            save_ev_ring();
#if 0
            send_unsol_var(Unsol_RuntimeError, 4, rtErr_Trig, trinfo->eventcnt,
                    10, info.state);
            fatal_readout_error();
            return 0;
#endif
        }
        trinfo->eventcnt=info.evc;
        trinfo->time.tv_sec=info.irq_sec;
        trinfo->time.tv_nsec=info.irq_nsec;
        trinfo->time_valid=1;
        if (priv->flags&1)
            trigger=info.tpat;
        break;
    }

    return trigger;
}
/*****************************************************************************/
#if 0
static void
reset_trig_plxsync(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    struct plxsync_ctrl_reg reg;

    reg.offset=ofs(struct plxsync_reg, sr);
    reg.val=sr_busy;
    if (ioctl(priv->p, PLXSYNC_CTRL_WRITE, &reg)<0) {
        printf("trig_plxsync: CTRL_WRITE: %s\n", strerror(errno));
        send_unsol_var(Unsol_RuntimeError, 4, rtErr_Trig, trinfo->eventcnt,
                4, errno);
        fatal_readout_error();
    }
}
#else
static void
reset_trig_plxsync(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    u_int32_t evt=trinfo->eventcnt;

#if 0
    printf("reset_trig_plxsync: called\n");
#endif
    if (ioctl(priv->p, PLXSYNC_ACK, &evt)<0) {
        printf("trig_plxsync: PLXSYNC_ACK: %s\n", strerror(errno));
        send_unsol_var(Unsol_RuntimeError, 4, rtErr_Trig, trinfo->eventcnt,
                4, errno);
        fatal_readout_error();
    }
}
#endif
/*****************************************************************************/
/*
 * pc number of arguments
 * p  remaining arguments from init_trig_plxsync
 * p[0] (integer) 0: poll; 1: signal; 2: select
 * [p[1] flags
 *       0x1: use trigger pattern
 * ]
 */
static plerrcode
init_trig_plxsync_(ems_u32* p, int pc, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    struct plxsync_ctrl_reg reg;
    struct plxsync_ident ident;
    plerrcode pres;
    u_int32_t ui32;

    if (pc<1 || pc>2) {
        *outptr++=3;
        return plErr_ArgNum;
    }
    if (pc>1)
        priv->flags=p[1];
    else
        priv->flags=0;

    priv->mode=(enum trigg_mode)p[0];

    trinfo->eventcnt=0;

    priv->signal=-1;

    if (ioctl(priv->p, PLXSYNC_IDENT, &ident)<0) {
        printf("trig_plxsync: ioctl(%s, IDENT): %s\n", priv->device,
                strerror(errno));
        *outptr++=errno;
        pres=plErr_System;
        goto error;         
    }
    if (priv->flags&1 && (ident.ident&0xf0000000)<0x10000000) {
        printf("trig_plxsync: trigger pattern not supported\n");
        pres=plErr_BadModTyp;
        goto error;
    }

    init_ev_ring();

    switch (priv->mode) {
    case trigg_poll:
        D(D_TRIG, printf("trig_plxsync: using poll mode\n");)
        break;
    case trigg_signal: {
        union SigHdlArg sigarg;

        D(D_TRIG, printf("trig_plxsync: using signal mode\n");)

        priv->got_sig=0;

        sigarg.val=0;
        sigarg.ptr=0;
#ifdef F_SETSIG
        priv->signal=install_signalhandler(sighndlr, sigarg, "trigger");
#else
        priv->signal=install_signalhandler_sig(sighndlr, SIGIO, sigarg,
                "trigger");
#endif
        if (priv->signal==-1) {
            printf("trig_plxsync: can't install signal handler.\n");
            pres=plErr_Other;
            goto error;
        } else {
            D(D_TRIG,
                printf("trig_plxsync: signal handler installed; signal=%d\n",
                    priv->signal);)
        }
        }
        break;
    case trigg_select:
        D(D_TRIG, printf("trig_plxsync: using select mode\n");)
        break;
    default:
        {
        printf("trig_plxsync: mode %d not known\n", priv->mode);
        *outptr++=2;
        pres=plErr_ArgRange;
        goto error;
        }
    }

    if (priv->signal>0) {
        if (fcntl(priv->p, F_SETOWN, getpid())<0) {
            printf("trig_plxsync: fcntl(%s, F_SETOWN): %s\n", priv->device,
                    strerror(errno));
            *outptr++=errno;
            pres=plErr_System;
            goto error;         
        }
#ifdef F_SETSIG
        if (fcntl(priv->p, F_SETSIG, priv->signal)<0) {
            printf("trig_plxsync: fcntl(%s, F_SETSIG): %s\n", priv->device,
                    strerror(errno));
            *outptr++=errno;
            pres=plErr_System;
            goto error;         
        }
#endif
    }

    if (ioctl(priv->p, PLXSYNC_RESET)<0) {
        printf("trig_plxsync: ioctl(%s, RESET): %s\n", priv->device,
                strerror(errno));
        *outptr++=errno;
        pres=plErr_System;
        goto error;         
    }
#if 1
    ui32=PLXSYNC_IRQS;
    if (ioctl(priv->p, PLXSYNC_ALLOW_IRQS, &ui32)<0) {
        printf("trig_plxsync: ioctl(%s, ALLOW_IRQS): %s\n", priv->device,
                strerror(errno));
        *outptr++=errno;
        pres=plErr_System;
        goto error;         
    }
#endif

    /* enable */
    reg.offset=ofs(struct plxsync_reg, cr);
    if (ioctl(priv->p, PLXSYNC_CTRL_READ, &reg)<0) {
        printf("trig_plxsync: CTRL_READ: %s\n", strerror(errno));
        send_unsol_var(Unsol_RuntimeError, 4, rtErr_Trig, trinfo->eventcnt,
                4, errno);
        fatal_readout_error();
    }
    reg.val|=cr_ena;
    if (priv->flags&1)
        reg.val|=cr_tpat;
    if (ioctl(priv->p, PLXSYNC_CTRL_WRITE, &reg)<0) {
        printf("trig_plxsync: CTRL_WRITE: %s\n", strerror(errno));
        send_unsol_var(Unsol_RuntimeError, 4, rtErr_Trig, trinfo->eventcnt,
                4, errno);
        fatal_readout_error();
    }

    switch (priv->mode) {
    case trigg_poll:
        tinfo->proctype=tproc_poll;
        break;
    case trigg_signal:
        tinfo->proctype=tproc_signal;
        break;
    case trigg_select:
        tinfo->proctype=tproc_select;
        tinfo->i.tp_select.path=priv->p;
        tinfo->i.tp_select.seltype=select_read;
        break;
    }
    tinfo->get_trigger=get_trig_plxsync;
    tinfo->reset_trigger=reset_trig_plxsync;
    tinfo->done_trigger=done_trig_plxsync;

    return plOK;

error:
    if (priv->signal>0)
        remove_signalhandler(priv->signal);
    return pres;
}
/*===========================================================================*/
/*
 * p[0] number of arguments
 * p[1]  -1: device given as device special file
 *      >=0: device of type 'modul_sync', already open
 * [p[2] (string ) device (if p[1]==-1)]
 * pp[0] (integer) 0: poll; 1: signal; 2: select
 * [pp[1] flags
 *       0x1: use trigger pattern
 * ]
 */
plerrcode init_trig_plxsync(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv;
    plerrcode pres=plOK;
    int used;

    if (p[0]<1) {
        printf("init_trig_plxsync: no arguments\n");
        *outptr++=1;
        return plErr_ArgNum;
    }

    priv=calloc(1, sizeof(struct private));
    if (!priv) {
        printf("init_trig_plxsync: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    priv->p=-1;
    priv->mode=trig_none;
    priv->device=0;
    tinfo->private=priv;

    if (((ems_i32*)p)[1]<0) {
        if ((p[0]<2) || (xdrstrlen(p+2)>p[0]-1)) {
            *outptr++=2;
            pres=plErr_ArgNum;
            goto error;
        }
        xdrstrcdup(&priv->device, p+2);
        if (priv->device==0) {
            pres=plErr_NoMem;
            goto error;
        }
        used=1+xdrstrlen(p+2);

        priv->p=open(priv->device, O_RDWR, 0);
        if (priv->p<0) {
            printf("trig_plxsync: open(%s) %s\n", priv->device, strerror(errno));
            *outptr++=errno;
            pres=plErr_System;
            goto error;
        }
        if (fcntl(priv->p, F_SETFD, FD_CLOEXEC)<0) {
            printf("trig_plxsync: fcntl(%s, FD_CLOEXEC): %s\n", priv->device,
                    strerror(errno));
        }
        if (fcntl(priv->p, F_SETFL, O_NDELAY)<0) {
            printf("trig_plxsync: fcntl(%s, F_SETFL): %s\n", priv->device,
                    strerror(errno));
        }
    } else {
        /*struct generic_dev *dev;*/
        struct sync_dev *dev;
        struct plxsync_info *info;

        if ((pres=find_odevice(modul_sync, p[1], (struct generic_dev**)&dev))!=plOK)
            goto error;

        if (dev->synctype!=sync_plx) {
            printf("synchronisation device %d is not 'sync_plx'\n", p[1]);
            pres=plErr_BadModTyp;
            goto error;
        }
        printf("init_trig_plxsync: using device %s\n", dev->pathname);
        info=(struct plxsync_info*)dev->info;
        priv->p=dup(info->p);
        priv->device=strdup(dev->pathname);
        used=1;
    }

    pres=init_trig_plxsync_(p+1+used, p[0]-used, trinfo);

error:
    if (pres!=plOK) {
        if (priv->p>=0)
            close(priv->p);
        if (priv->device)
            free(priv->device);
        free(priv);
    }
    return pres;
}
/*****************************************************************************/

char name_trig_plxsync[]="plxsync";
int ver_trig_plxsync=1;

/*****************************************************************************/
/*****************************************************************************/
