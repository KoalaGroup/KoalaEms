/*
 * trigger/vme/sis3100/sis3100trigger.c
 * created 2006-Jul-09 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100trigger.c,v 1.14 2017/10/21 22:44:12 wuestner Exp $";

#include <sconf.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <glob.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
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
    int p_ctrl;
    int p_vme;
    trigg_mode mode;
    u_int32_t inmask;
    u_int32_t outmask;
    u_int32_t pulsemask;
    u_int32_t vetomask;
    int signal;
    int got_sig;
    int irqs_got;
};

extern ems_u32* outptr;

#define ofs(what, elem) ((int)&(((what *)0)->elem))

RCS_REGISTER(cvsid, "trigger/vme/sis3100")

/*****************************************************************************/
static void
sighndlr(union SigHdlArg arg, __attribute__((unused)) int sig)
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
    if (ioctl(priv->p_ctrl, SIS1100_IRQ_CTL, &ctl)<0)
        printf("trig_sis3100: IRQ_CTL(0): %s\n", strerror(errno));

    if (priv->signal>0)
        remove_signalhandler(priv->signal);

    /* deactivate used outputs */
    if (priv->outmask) {
        /* switch off 'normal' bits */
        v=(priv->outmask&0xff)<<16;
        /* switch on 'shifted' bits */
        v|=(priv->outmask&0xff0000)>>16;
        ioctl(priv->p_ctrl, SIS1100_FRONT_IO, &v);
    }

    close(priv->p_ctrl);
    if (priv->p_vme>=0)
        close(priv->p_vme);
    free(priv->device);
    free(priv);
    return plOK;
}
/*****************************************************************************/
static void
clear_trig_sis3100(struct private *priv)
{
    struct sis1100_irq_ack ack;

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

    /* acknowledge irq */
    ack.irq_mask=priv->irqs_got<<8;
    ioctl(priv->p_ctrl, SIS1100_IRQ_ACK, &ack);

#if 0 /* for debugging only */
    if (priv->p_vme>=0) {
        struct sis1100_ctrl_reg reg;
        reg.offset=0x84;
        if (ioctl(priv->p_vme, SIS1100_CTRL_READ, &reg)<0)
                printf("ERROR in ioctln\n");
        printf("res_trig_sis3100: 0x84: %08x\n", reg.val);
    }
#endif
}
/*****************************************************************************/
static void
reset_trig_sis3100(struct triggerinfo* trinfo)
{
    struct trigprocinfo* tinfo=(struct trigprocinfo*)trinfo->tinfo;
    struct private* priv=(struct private*)tinfo->private;
    u_int32_t v;

    clear_trig_sis3100(priv);

    /* manipulate outputs */
    if (priv->outmask) {
        /* switch off 'normal' bits */
        v=(priv->outmask&0xff)<<16;
        /* switch on 'shifted' bits */
        v|=(priv->outmask&0xff0000)>>16;
        ioctl(priv->p_ctrl, SIS1100_FRONT_IO, &v);
    }
    /* generate 'end' pulses */
    if (priv->pulsemask&0x7f0000) {
        v=priv->pulsemask>>16;
        ioctl(priv->p_ctrl, SIS1100_FRONT_PULSE, &v);
    }
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
        /* fallthrough */
    case trigg_poll: {
        printf("get: poll\n");
        get.irq_mask=priv->inmask;
        ioctl(priv->p_ctrl, SIS1100_IRQ_GET, &get);
        if (!(get.irqs&priv->inmask)) return 0;
        }
        break;
    case trigg_select: {
        int got;

        got=read(priv->p_ctrl, &get, sizeof(struct sis1100_irq_get));
#if 0
        printf("read sis1100_irq: got=%d\n"
               "                 mask=0x%08x\n"
               "        remote_status=%d\n"
               "           opt_status=0x%08x\n"
               "                 mbx0=0x%08x\n"
               "                 irqs=0x%08x\n"
               "                level=0x%08x\n"
               "               vector=0x%08x\n",
               got, get.irq_mask, get.remote_status,
                    get.opt_status, get.mbx0,
                    get.irqs, get.level, get.vector);
#endif
        if (got!=sizeof(struct sis1100_irq_get)) {
            printf("read trigstatus: got=%d: %s\n", got, strerror(errno));
            send_unsol_var(Unsol_RuntimeError, 4, rtErr_Trig,
                    global_evc.ev_count, 5, errno);
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

    if (get.irqs&~(priv->inmask<<8)) {
        struct sis1100_irq_ack ack;
        printf("get_trig_sis3100: clearing stray interrupts 0x%x\n", get.irqs);
#if 0
        printf("get_trig_sis3100: get.irqs    =%08x\n"
               "                  priv->inmask=%08x\n"
               "               priv->inmask<<8=%08x\n"
               "   get.irqs&~(priv->inmask<<8)=%08x\n",
               get.irqs, priv->inmask,
               priv->inmask<<8, get.irqs&~(priv->inmask<<8));
#endif
        ack.irq_mask=get.irqs;
        ioctl(priv->p_ctrl, SIS1100_IRQ_ACK, &ack);
    }
    if (get.remote_status) {
        printf("Link is %s\n", get.remote_status<0?"down":"up");
    }

#if 0 /* debugging only */
    printf("get.irqs: 0x%x\n", get.irqs);
    if (priv->p_vme>=0) {
        struct sis1100_ctrl_reg reg;
        reg.offset=0x84;
        if (ioctl(priv->p_vme, SIS1100_CTRL_READ, &reg)<0)
                printf("ERROR in ioctln\n");
        printf("get_trig_sis3100: 0x84: %08x\n", reg.val);
    }
#endif
#if 0 /* debugging only */
    printf("get.mask: 0x%x\n", get.irq_mask);
    printf("get.remote: 0x%x\n", get.remote_status);
    printf("get.opt: 0x%x\n", get.opt_status);
    printf("get.mbx0: 0x%x\n", get.mbx0);
    printf("get.level: 0x%x\n", get.level);
    printf("get.vector: 0x%x\n", get.vector);
#endif

    /* recognise trigger bits */
    priv->irqs_got=(get.irqs>>8)&priv->inmask;
    trigger=priv->irqs_got;

    /* check veto */
    if (priv->vetomask) {
        v=0; /* !!! */
        ioctl(priv->p_ctrl, SIS1100_FRONT_IO, &v);
        v=(~v&0x7f0000) | ((v>>16)&0x7f);
        if (v&priv->vetomask) {
            clear_trig_sis3100(priv);
            trigger=0;
            /* nevertheless generate 'end' pulses */
            if (priv->pulsemask&0x7f0000) {
                v=priv->pulsemask>>16;
                ioctl(priv->p_ctrl, SIS1100_FRONT_PULSE, &v);
            }
        }
    }

#if 0 /* debugging only */
    printf("get_trig_sis3100: return trigger %08x\n", trigger);
#endif

    if (trigger) {
        /* manipulate outputs */
        if (priv->outmask) {
            /* switch on 'normal' bits */
            v=priv->outmask&0xff;
            /* switch off 'shifted' bits */
            v|=priv->outmask&0xff0000;
            ioctl(priv->p_ctrl, SIS1100_FRONT_IO, &v);
        }
        /* generate 'start' pulses */
        if (priv->pulsemask&0x7f) {
            v=priv->pulsemask;
            ioctl(priv->p_ctrl, SIS1100_FRONT_PULSE, &v);
        }

        trinfo->count++;
    }

    return trigger;
}
/*****************************************************************************/
static plerrcode
open_device(struct private *priv)
{
    glob_t *pglob;
    unsigned int i;
    int res;

    priv->p_ctrl=-1;
    priv->p_vme=-1;

    pglob=calloc(1, sizeof(glob_t));
    if (!pglob) {
        printf("init_trig_sis3100: %s\n", strerror(errno));
        return plErr_NoMem;
    }

    res=glob(priv->device, GLOB_NOSORT, 0, pglob);
    if (res) {
        printf("init_trig_sis3100: glob returns %d\n", res);
        free(pglob);
        return plErr_System;
    }

    printf("init_trig_sis3100: glob: pathc=%llu\n",
            (unsigned long long)pglob->gl_pathc);

    for (i=0; i<pglob->gl_pathc; i++) {
        int p;
        enum sis1100_subdev subdev;

        printf("init_trig_sis3100: glob: pathv[%d]: %s\n",
                i, pglob->gl_pathv[i]);
        p=open(pglob->gl_pathv[i], O_RDWR, 0);
        if (p<0) {
            printf("init_trig_sis3100: open %s: %s\n",
                    pglob->gl_pathv[i], strerror(errno));
            continue;
        }
        if (fcntl(p, F_SETFD, FD_CLOEXEC)<0) {
            printf("trig_sis3100: fcntl(%s, FD_CLOEXEC): %s\n", 
                pglob->gl_pathv[i], strerror(errno));
        }
        if (ioctl(p, SIS1100_DEVTYPE, &subdev)<0){
            printf("trig_sis3100: iocntl(%s, DEVTYPE): %s\n", 
                pglob->gl_pathv[i], strerror(errno));
            close(p);
            continue;
        }
        switch (subdev) {
        case sis1100_subdev_ctrl:
            printf("trig_sis3100: dev %s used as ctrl\n", pglob->gl_pathv[i]);
            priv->p_ctrl=p;
            break;
        case sis1100_subdev_remote:
            printf("trig_sis3100: dev %s used as vme\n", pglob->gl_pathv[i]);
            priv->p_vme=p;
            break;
        case sis1100_subdev_ram: /* nobreak */
        case sis1100_subdev_dsp: /* nobreak */
        default:
            printf("trig_sis3100: dev %s not used\n", pglob->gl_pathv[i]);
            close(p);
            continue;
        }
    }

    if (priv->p_ctrl<0) {
        printf("trig_sis3100: no ctrl device found using %s\n", priv->device);
        if (priv->p_vme>=0)
            close(priv->p_vme);
        return plErr_ArgRange;
    }

    globfree(pglob);

    return plOK;
}
/*****************************************************************************/
/*
 * p[0] number of arguments
 * p[1] (string) device
 * pp[0] (integer) 0: poll; 1: signal; 2: select
 * pp[1] inmask
 * pp[2] outmask
 * pp[3] pulsemask
 * pp[4] vetomask
 * (pp[2..4] are optional, defaulting to 0)
 */
/*
 * p[1] (string) can be given in two variants:
 * - as /dev/sis1100_00ctrl (replase '00' as needed)
 *   this sfficient for daily use
 * - as /dev/sis1100_00*
 *   the code will expand the wildcard using glob(3) in order to find
 *   '*ctrl' and '*remote' automtically.
 *   '*remote' is (currently) used for debugging only and normally not needed
 *
 * all masks use the LEMO and FLAT IOs of the front panel
 * bit 0..3 denote the four FLAT IOs
 * bit 4..6  denote the thee LEMO IOs
 * bit 7 denote the user LED (outmask only)
 * in addition the bits 16..23 (original bit shifted by 16) can be used:
 * for outmask and vetomask:
 *   the polarity is inverted
 * for the pulsemask:
 *   the bits 0..6 generate a short pulse at begin of readout
 *   the bits 16..22 generate a short pulse at end of readout
 * if both a 'normal' and its shifted variant is set the behavior is undefined
 *
 * pp[1] inmask: defines the input ports (LEMO and FLAT) used as trigger
 * pp[2] outmask: defines the output port(s) whitch should be activated
 *       during readout (x80 denotes the user LED)
 * pp[3] pulsemask: efines the output port(s) for short pulses
 * pp[4] vetomask: defines the input port(s) used as veto
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
    plerrcode pres;

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

    if (rest<2) {
        printf("init_trig_sis3100: String braucht %d worte, habe %d worte\n",
          xdrstrlen(p+1), p[0]);
        *outptr++=3;
        return plErr_ArgNum;
    }


    priv=calloc(1, sizeof(struct private));
    if (!priv) {
        printf("init_trig_sis3100: %s\n", strerror(errno));
        return plErr_NoMem;
    }

    pp=xdrstrcdup(&priv->device, p+1);
    if (priv->device==0) {
        free(priv);
        return plErr_NoMem;
    }
    tinfo->private=priv;

    priv->mode=(trigg_mode)pp[0];
    priv->inmask=pp[1]&0x7f;
    priv->outmask=0;
    priv->pulsemask=0;
    priv->vetomask=0;
    if (rest>2)
        priv->outmask=pp[2]&0x00ff00ff;
    if (rest>3)
        priv->pulsemask=pp[3]&0x007f007f;
    if (rest>4)
        priv->vetomask=pp[4]&0x007f007f;

    printf("trig_sis3100: mode=%d inmask=0x%08x outmask=0x%08x "
            "pulsemask=0x%08x vetomask=0x%08x\n",
            priv->mode, priv->inmask, priv->outmask,
            priv->pulsemask, priv->vetomask);

    trinfo->count=0;

    if ((pres=open_device(priv))<0) {
        free(priv->device);
        free(priv);
        return pres;
    }

    /* bring outputs into there default state */
    if (priv->outmask) {
        /* switch off 'normal' bits */
        v=(priv->outmask&0xff)<<16;
        /* switch on 'shifted' bits */
        v|=(priv->outmask&0xff0000)>>16;
        ioctl(priv->p_ctrl, SIS1100_FRONT_IO, &v);
    }

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
            close(priv->p_ctrl);
            if (priv->p_vme>=0)
                close(priv->p_vme);
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
        close(priv->p_ctrl);
        if (priv->p_vme>=0)
            close(priv->p_vme);
        free(priv->device);
        free(priv);
        *outptr++=2;
        return plErr_ArgRange;
        }
    }

    ctl.irq_mask=priv->inmask<<8;
    ctl.signal=priv->signal;
    res=ioctl(priv->p_ctrl, SIS1100_IRQ_CTL, &ctl);
    if (res) {
        *outptr++=errno;
        if (priv->signal>0) remove_signalhandler(priv->signal);
        close(priv->p_ctrl);
        if (priv->p_vme>=0)
            close(priv->p_vme);
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
        tinfo->i.tp_select.path=priv->p_ctrl;
        tinfo->i.tp_select.seltype=select_read;
        break;
    }

    tinfo->get_trigger=get_trig_sis3100;
    tinfo->reset_trigger=reset_trig_sis3100;
    tinfo->done_trigger=done_trig_sis3100;

    return plOK;
}
/*****************************************************************************/
char name_trig_sis3100[]="sis3100";
int ver_trig_sis3100=1;
/*****************************************************************************/
/*****************************************************************************/
