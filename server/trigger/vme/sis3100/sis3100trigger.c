/*
 * trigger/vme/sis3100/sis3100trigger.c
 * created 2006-Jul-09 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100trigger.c,v 1.8 2011/04/06 20:30:36 wuestner Exp $";

#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sconf.h>
#include <debug.h>
#include <objecttypes.h>
#include <errorcodes.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
#include "../../../main/signals.h"
#include "../../../main/scheduler.h"
#include "../../trigger.h"
#include "../../trigprocs.h"
#ifdef OBJ_VAR
/*#include "../../../objects/var/variables.h"*/
#endif
#include "../../../objects/pi/readout.h"
#include "../../../commu/commu.h"
#include <xdrstring.h>
#include <dev/pci/sis1100_var.h>

typedef enum {trigg_poll=0, trigg_signal, trigg_select} trigg_mode;

struct private {
    char* device;
    int p;
    trigg_mode mode;
    u_int32_t inmask;
    u_int32_t outmask;
    int signal;
    int got_sig;
    int irqs_got;
};

extern ems_u32* outptr;

#define ofs(what, elem) ((int)&(((what *)0)->elem))

RCS_REGISTER(cvsid, "trigger/vme/sis3100")

/*****************************************************************************/
#if 0
static char*
getbits(unsigned int v)
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
#endif
/*****************************************************************************/
static void
sighndlr(union SigHdlArg arg, int sig)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)arg.ptr;
    struct private* priv=(struct private*)tinfo->private;
    priv->got_sig=1;
}
/*****************************************************************************/
static plerrcode
done_trig_sis3100(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    struct sis1100_irq_ctl ctl;
    u_int32_t v;

printf("done_trig_sis3100\n");
    T(done_trig_sis3100)

    ctl.irq_mask=priv->inmask;
    ctl.signal=0;
    if (ioctl(priv->p, SIS1100_IRQ_CTL, &ctl)<0)
        printf("trig_sis3100: IRQ_CTL(0): %s\n", strerror(errno));

    if (priv->signal>0)
        remove_signalhandler(priv->signal);

    /* set used outputs to default state */
    v=priv->outmask<<16;
    ioctl(priv->p, SIS1100_FRONT_IO, &v);

    close(priv->p);
    free(priv->device);
    free(priv);
    return plOK;
}
/*****************************************************************************/
static int
get_trig_sis3100(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    struct sis1100_irq_get get;
    unsigned int trigger;
    u_int32_t v;

/*printf("get_trig_sis3100, mode=%d\n", priv->mode);*/
    switch (priv->mode) {
    case trigg_signal:
        printf("get: signal\n");
        if (!priv->got_sig) return 0;
        /* nobreak */
    case trigg_poll: {
        printf("get: poll\n");
        get.irq_mask=priv->inmask;
        ioctl(priv->p, SIS1100_IRQ_GET, &get);
        if (!(get.irqs&priv->inmask)) return 0;
        }
        break;
    case trigg_select: {
        int got;

        got=read(priv->p, &get, sizeof(struct sis1100_irq_get));
/*
 *         printf("read sis1100_irq: got=%d mask=0x%08x remote_status=%d "
 *                 "irqs=0x%08x\n",
 *                     got, get.irq_mask, get.remote_status, get.irqs);
 */
        if (got!=sizeof(struct sis1100_irq_get)) {
            printf("read trigstatus: got=%d: %s\n", got, strerror(errno));
            send_unsol_var(Unsol_RuntimeError, 4, rtErr_Trig, trinfo->eventcnt,
                    5, errno);
            fatal_readout_error();
            return 0;
        }
        }
        break;
    default:
        printf("get_trig_sis3100: illegal trigger mode %d\n", priv->mode);
        fatal_readout_error();
        return 0;
    }

    if (get.irqs&~(priv->inmask<<12)) {
        struct sis1100_irq_ack ack;
        printf("get_trig_sis3100: clearing stray interrupts 0x%x\n", get.irqs);
        ack.irq_mask=get.irqs;
        ioctl(priv->p, SIS1100_IRQ_ACK, &ack);
    }
    if (get.remote_status) {
        printf("Link is %s\n", get.remote_status<0?"down":"up");
    }

#if 0
printf("get.mask: 0x%x\n", get.irq_mask);
printf("get.irqs: 0x%x\n", get.irqs);
printf("get.remote: 0x%x\n", get.remote_status);
printf("get.opt: 0x%x\n", get.opt_status);
printf("get.mbx0: 0x%x\n", get.mbx0);
printf("get.level: 0x%x\n", get.level);
printf("get.vector: 0x%x\n", get.vector);
#endif

    /* light user LED */
    v=0x80;
    ioctl(priv->p, SIS1100_FRONT_IO, &v);

    /* recognice trigger bits */
    priv->irqs_got=(get.irqs>>12)&priv->inmask;
    trigger=priv->irqs_got;

    if (trigger) {
        trinfo->eventcnt++;
    }

    return trigger;
}
/*****************************************************************************/
static void
reset_trig_sis3100(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    struct sis1100_irq_ack ack;
    u_int32_t v;

/*printf("reset_trig_sis3100\n");*/
    switch (priv->mode) {
    case trigg_poll: /* nobreak */
    case trigg_signal: {
#if 0
/* unfinished */
        struct sis1100_irq_ack ack;
        ack.irq_mask=priv->irqs_got;
        if (ioctl(priv->p, SIS1100_IRQ_CTL, &ctrl)<0)
                printf("trig_sis3100: IRQ_CTL(0): %s\n", strerror(errno));
#endif
        }
        break;
    case trigg_select:
        /* do nothing */
        break;
    }

    /* extinguish user LED */
    v=0x800;
    ioctl(priv->p, SIS1100_FRONT_IO, &v);

    /* acknowledge irq */
    ack.irq_mask=priv->irqs_got<<12;
    ioctl(priv->p, SIS1100_IRQ_ACK, &ack);

    v=priv->outmask;
    ioctl(priv->p, SIS1100_FRONT_IO, &v);
    v=priv->outmask<<16;
    ioctl(priv->p, SIS1100_FRONT_IO, &v);
}
/*****************************************************************************/
/*
 * p[0] number of arguments
 * p[1] (string ) device
 * pp[0] (integer) 0: poll; 1: signal; 2: select
 * pp[1] inmask (bit 16..22 of sis3100 register OPT-IN-LATCH_IRQ (shifted))
 * pp[2] outmask (bit 0..6 of sis3100 register OPT-IN/OUT
        and bit 7 of OPT-VME-Master Status/Control (LED))
 */
/*
 * Out0 is set as soon as the trigger is recognized and reset at the end
 * of the deadtime (wrong! (why?))
 */
plerrcode
init_trig_sis3100(ems_u32* p, struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv;
    ems_u32* pp;
    int rest, res;
    struct sis1100_irq_ctl ctl;
    u_int32_t v;

printf("init_trig_sis3100\n");

    priv=calloc(1, sizeof(struct private));
    if (!priv) {
        printf("init_trig_sis3100: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    tinfo->private=priv;

    priv->device=0;
    if (p[0]<1) {
        printf("init_trig_sis3100: no arguments given\n");
        *outptr++=1;
        return plErr_ArgNum;
    }
    if (xdrstrlen(p+1)<1) {
        printf("Devicename ist %d Zeichen lang.\n", xdrstrlen(p+1));
        *outptr++=2;
        return plErr_ArgNum;
    }
    rest=p[0]-xdrstrlen(p+1);

    if (rest!=3) {
        printf("init_trig_sis3100: String braucht %d worte, habe %d worte\n",
          xdrstrlen(p+1), p[0]);
        *outptr++=3;
        return plErr_ArgNum;
    }
    pp=xdrstrcdup(&priv->device, p+1);
    if (priv->device==0)
        return plErr_NoMem;

    priv=calloc(1, sizeof(struct private));
    if (!priv) {
        printf("init_trig_sis3100: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    priv->p=-1;
    tinfo->private=priv;

    priv->mode=(trigg_mode)pp[0];
    priv->inmask=pp[1]&0x3f;
    priv->outmask=pp[2]&0xff;
    printf("trig_sis3100: mode=%d inmask=0x%x outmask=0x%x\n", priv->mode,
            priv->inmask, priv->outmask);

    trinfo->eventcnt=0;

    priv->p=open(priv->device, O_RDWR, 0);
    if (priv->p<0) {
        printf("trig_sis3100: open(%s) %s\n", priv->device, strerror(errno));
        *outptr++=errno;
        free(priv->device);
        free(priv);
        return plErr_System;
    }
    if (fcntl(priv->p, F_SETFD, FD_CLOEXEC)<0) {
        printf("trig_sis3100: fcntl(%s, FD_CLOEXEC): %s\n", priv->device,
                strerror(errno));
    }
#if 0
    if (fcntl(priv->p, F_SETFL, O_NDELAY)<0) {
        printf("trig_sis3100: fcntl(%s, F_SETFL): %s\n", priv->device,
                strerror(errno));
    }
#endif

    /* block trigger */
    v=priv->outmask;
    ioctl(priv->p, SIS1100_FRONT_IO, &v);

    priv->signal=-1;
    switch (priv->mode) {
    case trigg_poll:
        D(D_TRIG, printf("trig_sis3100: using poll mode\n");)
        printf("trig_sis3100: using poll mode\n");
        break;
    case trigg_signal: {
        union SigHdlArg sigarg;

        D(D_TRIG, printf("trig_sis3100: using signal mode\n");)
        printf("trig_sis3100: using signal mode\n");

        priv->got_sig=0;

        sigarg.ptr=tinfo;
        priv->signal=install_signalhandler(sighndlr, sigarg, "trigger");
        if (priv->signal==-1) {
            printf("trig_sis3100: can't install signal handler.\n");
            close(priv->p);
            free(priv->device);
            free(priv);
            return plErr_Other;
        } else {
            D(D_TRIG,
                printf("trig_sis3100: signal handler installed; signal=%d\n",
                    priv->signal);)
        }
        }
        break;
    case trigg_select:
        D(D_TRIG, printf("trig_sis3100: using select mode\n");)
        printf("trig_sis3100: using select mode\n");
        break;
    default:
        {
        printf("trig_sis3100: mode %d not known\n", priv->mode);
        close(priv->p);
        free(priv->device);
        free(priv);
        *outptr++=2;
        return plErr_ArgRange;
        }
    }

    ctl.irq_mask=priv->inmask<<8;
    ctl.signal=priv->signal;
    res=ioctl(priv->p, SIS1100_IRQ_CTL, &ctl);
    if (res) {
        *outptr++=errno;
        if (priv->signal>0) remove_signalhandler(priv->signal);
        close(priv->p);
        free(priv->device);
        free(priv);
        return plErr_System;
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

    tinfo->get_trigger=get_trig_sis3100;
    tinfo->reset_trigger=reset_trig_sis3100;
    tinfo->done_trigger=done_trig_sis3100;


    /* unblock trigger */
    v=priv->outmask<16;
    ioctl(priv->p, SIS1100_FRONT_IO, &v);

    return plOK;
}
/*****************************************************************************/
char name_trig_sis3100[]="sis3100";
int ver_trig_sis3100=1;
/*****************************************************************************/
/*****************************************************************************/
