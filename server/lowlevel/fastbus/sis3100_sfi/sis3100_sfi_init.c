/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_init.c
 * created Oct.2002
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_init.c,v 1.20 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "../../../commu/commu.h"
#include "sis3100_sfi.h"
#include "sis3100_sfi_open_dev.h"

#ifdef SIS3100_SFI_DEADBEEF
ems_u8 sis3100_sfi_deadbeef_delayed[SIS3100_SFI_BEEF_SIZE];
ems_u8 sis3100_sfi_deadbeef_prompt[SIS3100_SFI_BEEF_SIZE];
#endif

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

/*****************************************************************************/
static int
sis3100sfi_check_ident(struct fastbus_dev* dev)
{
    struct fastbus_sis3100_sfi_info* info=(struct fastbus_sis3100_sfi_info*)dev->info;
    struct sis1100_ident ident;

    if (ioctl(info->p_ctrl, SIS1100_IDENT, &ident)<0) {
        printf("sis3100sfi_check_ident: fcntl(p_ctrl, IDENT): %s\n",
                strerror(errno));
        dev->generic.online=0;
        return -1;
    }
    printf("fastbus_init_sis3100_sfi local ident:\n");
    printf("  hw_type   =%d\n", ident.local.hw_type);
    printf("  hw_version=%d\n", ident.local.hw_version);
    printf("  fw_type   =%d\n", ident.local.fw_type);
    printf("  fw_version=%d\n", ident.local.fw_version);
    if (ident.remote_ok) {
        printf("fastbus_init_sis3100_sfi remote ident:\n");
        printf("  hw_type   =%d\n", ident.remote.hw_type);
        printf("  hw_version=%d\n", ident.remote.hw_version);
        printf("  fw_type   =%d\n", ident.remote.fw_type);
        printf("  fw_version=%d\n", ident.remote.fw_version);
    }

    if (ident.remote_online) {
        if (ident.remote.hw_type==sis1100_hw_vme) {
            dev->generic.online=1;
        } else {
            printf("fastbus_init_sis3100_sfi: Wrong hardware connected!\n");
            dev->generic.online=0;
        }
    } else {
        printf("fastbus_init_sis3100_sfi: NO remote Interface!\n");
        dev->generic.online=0;
    }
    return 0;
}
/*****************************************************************************/
static int
sis3100_set_user_led(struct fastbus_sis3100_sfi_info* info, int on)
{
    struct sis1100_ctrl_reg req;
    int res;

    req.offset=0x100;
    req.val=0x80;
    if (!on) req.val<<=16;

    res=ioctl(info->p_vme, SIS1100_CTRL_WRITE, &req);
    if (res) {
        printf("switch user LED: %s\n", strerror(errno));
        return -1;
    }
    if (req.error) {
        printf("switch user LED: error=0x%x\n", req.error);
        return -1;
    }
    return 0;
}
/*****************************************************************************/
static int
sis3100_get_user_led(struct fastbus_sis3100_sfi_info* info, int* on)
{
    struct sis1100_ctrl_reg req;
    int res;

    req.offset=0x100;

    res=ioctl(info->p_vme, SIS1100_CTRL_READ, &req);
    *on=!!(req.val&0x80);
    if (res) {
        printf("read user LED: %s\n", strerror(errno));
        return -1;
    }
    if (req.error) {
        printf("read user LED: error=0x%x\n", req.error);
        return -1;
    }
    return 0;
}
/*****************************************************************************/
static void
sis3100sfi_link_up_down(int p, enum select_types selected, union callbackdata data)
{
    struct generic_dev* gdev=(struct generic_dev*)data.p;
    struct fastbus_dev* dev=(struct fastbus_dev*)gdev;
    struct fastbus_sis3100_sfi_info* info=(struct fastbus_sis3100_sfi_info*)dev->info;
    struct sis1100_irq_get get;
    int res;

    res=read(p, &get, sizeof(struct sis1100_irq_get));
    if (res!=sizeof(struct sis1100_irq_get)) {
        printf("sis3100sfi_link_up_down: res=%d errno=%s\n", res,
            strerror(errno));
        return;
    }
    if (get.remote_status) {
        int crate=dev->generic.dev_idx;
        int initialised;
        int up;

        up=get.remote_status>0;
        printf("sis3100: FASTBUS crate %d is %s\n", crate, up?"up":"down");
        initialised=0;
        if (up) {
            sis3100sfi_check_ident(dev);
            if (gdev->generic.online) {
                int on, res;
                res=sis3100_get_user_led(info, &on);
                if (!res && on)
                    initialised=1;
                    printf("sis3100sfi: it is %s initialised\n",
                        initialised?"still":"not any more");
            }
        } else {
            gdev->generic.online=0;
        }
        send_unsol_var(Unsol_DeviceDisconnect, 4, modul_fastbus,
            crate, get.remote_status, initialised);
    } else {
        printf("sis3100sfi_link_up_down: status not changed\n");
    }
}
/*****************************************************************************/
#ifdef XXX
static
int sis3100_sfi_dsp_present (struct fastbus_dev* dev)
{
    struct sis1100_ctrl_reg reg;
    int res;

    reg.offset=ofs(struct sis3100_reg, dsp_sc);
    res=ioctl(dev->info.sis3100.p_dsp, SIS1100_CTRL_READ, &reg);
    if (res<0) {
        printf("sis3100_dsp_present: %s\n", strerror(errno));
        return -1;
    }
    if (reg.error) {
        printf("sis3100_dsp_present: error=0x%x\n", reg.error);
        return -1;
    }
    return !!(reg.val&sis3100_dsp_available);
}
#endif
#ifdef XXX
static
int sis3100_sfi_dsp_reset (struct fastbus_dev* dev)
{
    struct sis1100_ctrl_reg reg;
    int res;

    reg.offset=ofs(struct sis3100_reg, dsp_sc);
    reg.val=0x800;
    res=ioctl(dev->info.sis3100.p_dsp, SIS1100_CTRL_WRITE, &reg);
    if (res<0) {
        printf("sis3100_dsp_reset: %s\n", strerror(errno));
        return -1;
    }
    if (reg.error) {
        printf("sis3100_dsp_reset: error=0x%x\n", reg.error);
        return -1;
    }
    reg.offset=ofs(struct sis3100_reg, dsp_sc);
    reg.val=0x1000000;
    res=ioctl(dev->info.sis3100.p_dsp, SIS1100_CTRL_WRITE, &reg);
    if (res<0) {
        printf("sis3100_dsp_reset: %s\n", strerror(errno));
        return -1;
    }
    if (reg.error) {
        printf("sis3100_dsp_reset: error=0x%x\n", reg.error);
        return -1;
    }
    return 0;
}
#endif
#ifdef XXX
static
int sis3100_sfi_dsp_start (struct fastbus_dev* dev)
{
    struct sis1100_ctrl_reg reg;
    int res;

    reg.offset=ofs(struct sis3100_reg, dsp_sc);
    reg.val=0x100;
    res=ioctl(dev->info.sis3100.p_dsp, SIS1100_CTRL_WRITE, &reg);
    if (res<0) {
        printf("sis3100_dsp_start: %s\n", strerror(errno));
        return -1;
    }
    if (reg.error) {
        printf("sis3100_dsp_start: error=0x%x\n", reg.error);
        return -1;
    }
    return 0;
}
#endif
#ifdef XXX
static
int sis3100_sfi_dsp_load (struct fastbus_dev* dev, const char* codepath)
{
    return load_dsp_file(dev->info.sis3100.p_dsp, codepath);
}
#endif
/*****************************************************************************/
#ifdef XXX
sstatic struct dsp_procs dsp_procs={
    sis3100_dsp_present,
    sis3100_dsp_reset,
    sis3100_dsp_start,
    sis3100_dsp_load,
};

static struct mem_procs mem_procs={
    sis3100_mem_size,
    sis3100_mem_write,
    sis3100_mem_read,
    sis3100_mem_read_delayed,
    sis3100_mem_set_bufferstart,
};

static struct dsp_procs*
get_dsp_procs(struct fastbus_dev* dev)
{
    return &dsp_procs;
}

static struct mem_procs*
get_mem_procs(struct fastbus_dev* dev)
{
    return &mem_procs;
}
#endif
/*****************************************************************************/
#ifdef XXX
#include <sys/types.h>
#include <dirent.h>

static void
sis3100_sfi_open_devices(const char* stub)
{
    char *bname, *dname;
    DIR *dir;

    bname=basename(stub);
    dname=dirnmame(stub);


    dir=opendir(dname);
    readdir()


}
#endif
/*****************************************************************************/
static int
sis3100_sfi_dmalen(struct fastbus_dev* dev, int subsys, int* rlen, int* wlen)
{
    struct fastbus_sis3100_sfi_info* info=(struct fastbus_sis3100_sfi_info*)dev->info;
    int dmalen[2];
    int res, p=-1;

    switch (subsys) {
    case 0: /* subdev_remote */
        p=info->p_vme;
        break;
    case 1: /* subdev_ram */
        p=info->p_mem;
        break;
    case 2: /* subdev_ctrl */
    case 3: /* subdev_dsp */
    default:
        printf("cannot set dmalen for subsystem %d\n", subsys);
        return -1;
    }

    dmalen[0]=*rlen;
    dmalen[1]=*wlen;
    res=ioctl(p, SIS1100_MINDMALEN, dmalen);
    if (res<0)
        printf("sis3100_sfi_dmalen: %s\n", strerror(errno));
    *rlen=dmalen[0];
    *wlen=dmalen[1];
    return res;
}
/*****************************************************************************/
errcode
fastbus_init_sis3100_sfi(struct fastbus_dev* dev)
{
    struct fastbus_sis3100_sfi_info *info;
    ems_u32 status;
    struct sis1100_irq_ctl ctl;
    errcode res;

    printf("fastbus_init_sis3100_sfi(%s)\n", dev->pathname);

    info=(struct fastbus_sis3100_sfi_info*)malloc(sizeof(struct fastbus_sis3100_sfi_info));
    if (!info) {
        printf("fastbus_init_sis3100_sfi: calloc(info): %s\n", strerror(errno));
        return Err_NoMem;
    }
    dev->info=info;

    info->p_ctrl=-1;
    info->p_vme=-1;
    info->p_mem=-1;
    info->p_dsp=-1;

    if ((info->p_ctrl=
            sis3100_sfi_open_dev(dev->pathname, sis1100_subdev_ctrl))<0) {
        res=Err_System;
        goto error;
    }

    if ((info->p_vme=
            sis3100_sfi_open_dev(dev->pathname, sis1100_subdev_remote))<0) {
        res=Err_System;
        goto error;
    }

    if ((info->p_mem=
            sis3100_sfi_open_dev(dev->pathname, sis1100_subdev_ram))<0) {
        res=Err_System;
        goto error;
    }

    if ((info->p_dsp=
            sis3100_sfi_open_dev(dev->pathname, sis1100_subdev_dsp))<0) {
        res=Err_System;
        goto error;
    }

    if (fcntl(info->p_ctrl, F_SETFL, O_NDELAY)<0) {
        printf("vme_init_sis3100: fcntl(p_ctrl, F_SETFL): %s\n", strerror(errno));
    }

    if (ioctl(info->p_mem, SIS1100_MAPSIZE, &info->ram_size)<0) {
        printf("ioctl(MAPSIZE(ram): %s\n", strerror(errno));
        res=Err_System;
        goto error;
    }
    printf("Size of sdram: %Ld MByte\n", (long long int)(info->ram_size)/(1<<20));
    info->ram_free=0;

#ifdef SIS3100_SFI_MMAPPED
    if ((res=sis3100_sfi_init_mapped(dev->pathname, dev))!=OK) {
        goto error;
    }
#endif

    info->st=0;

    ctl.irq_mask=0;
    ctl.signal=-1;
    if (ioctl(info->p_ctrl, SIS1100_IRQ_CTL, &ctl)) {
        printf("fastbus_init_sis3100_sfi: SIS1100_IRQ_CTL: %s\n",
            strerror(errno));
    } else {
        union callbackdata cbdata;
        cbdata.p=dev;
        info->st=sched_insert_select_task(sis3100sfi_link_up_down, cbdata,
            "sis3100_link", info->p_ctrl, select_read
    #ifdef SELECT_STATIST
            , 1
    #endif
            );
        if (!info->st) {
            printf("fastbus_init_sis3100_sfi: cannot install callback for link_up_down\n");
            res=Err_System;
            goto error;
        }
    }

    dev->generic.done                =sis3100_sfi_done;
#ifdef DELAYED_READ
    dev->generic.reset_delayed_read  =sis3100_sfi_reset_delayed_read;
    dev->generic.read_delayed        =sis3100_sfi_read_delayed;
    dev->generic.enable_delayed_read =sis3100_sfi_enable_delayed_read;
#endif
    dev->setarbitrationlevel=sis3100_sfi_setarbitrationlevel;
    dev->getarbitrationlevel=sis3100_sfi_getarbitrationlevel;
    dev->FRC                =sis3100_sfi_FRC;
    dev->FRCa               =sis3100_sfi_FRCa;
    dev->FRD                =sis3100_sfi_FRD;
    dev->FRDa               =sis3100_sfi_FRDa;
    dev->multiFRC           =sis3100_sfi_multiFRC;
    dev->multiFRD           =sis3100_sfi_multiFRD;
    dev->FWC                =sis3100_sfi_FWC;
    dev->FWCa               =sis3100_sfi_FWCa;
    dev->FWD                =sis3100_sfi_FWD;
    dev->FWDa               =sis3100_sfi_FWDa;
    dev->FRCB               =sis3100_sfi_FRCB;
    dev->FRDB               =sis3100_sfi_FRDB;
    dev->FRDB_perfspect_needs   =sis3100_sfi_FRDB_perfspect_needs;
    dev->FRDB_S             =sis3100_sfi_FRDB_S;
    dev->FRDB_S_perfspect_needs =sis3100_sfi_FRDB_S_perfspect_needs;
/*
    dev->FWCB  =0;
    dev->FWDB  =0;
*/
    dev->FRCM               =sis3100_sfi_FRCM;
    dev->FRDM               =sis3100_sfi_FRDM;
    dev->FWCM               =sis3100_sfi_FWCM;
    dev->FWDM               =sis3100_sfi_FWDM;
    dev->FCPC               =sis3100_sfi_FCPC;
    dev->FCPD               =sis3100_sfi_FCPD;
    dev->FCDISC             =sis3100_sfi_FCDISC;
    dev->FCRW               =sis3100_sfi_FCRW;
    dev->FCRWS              =sis3100_sfi_FCRWS;
    dev->FCWSA              =sis3100_sfi_FCWSA;
    dev->FCWWS              =sis3100_sfi_FCWWS;
    dev->FCWW               =sis3100_sfi_FCWW;
    dev->LEVELIN            =sis3100_sfi_LEVELIN;
    dev->LEVELOUT           =sis3100_sfi_LEVELOUT;
    dev->PULSEOUT           =sis3100_sfi_PULSEOUT;
    dev->FRDB_multi_init    =sis3100_sfi_FRDB_multi_init;
    dev->FRDB_multi_read    =sis3100_sfi_FRDB_multi_read;
    dev->reset              =sis3100_sfi_reset;
    dev->restart_sequencer  =sis3100_restart_sequencer;
    dev->status             =sis3100_sfi_status;
    dev->dmalen             =sis3100_sfi_dmalen;

    dev->setarbitrationlevel(dev, 0x81);

    info->load_list=sis3100_sfi_load_list;
#ifdef XXX
    info->get_dsp_procs=get_dsp_procs;
    info->get_mem_procs=get_mem_procs;
#endif

#ifdef SIS3100_SFI_DEADBEEF
    memset(sis3100_sfi_deadbeef_delayed, SIS3100_SFI_BEEF_MAGIC_DELAYED,
            SIS3100_SFI_BEEF_SIZE);
    memset(sis3100_sfi_deadbeef_prompt, SIS3100_SFI_BEEF_MAGIC_PROMPT,
            SIS3100_SFI_BEEF_SIZE);
#endif

#ifdef DELAYED_READ
    info->hunk_list=0;
    info->hunk_list_size=0;
    info->delay_buffer=0;
    info->delay_buffer_size=0;
    dev->generic.delayed_read_enabled=1;
    sis3100_sfi_reset_delayed_read((struct generic_dev*)dev);
#endif

    {
        int rlen, wlen;
        rlen=10; wlen=10;
        sis3100_sfi_dmalen(dev, 0, &rlen, &wlen);
        rlen=10; wlen=10;
        sis3100_sfi_dmalen(dev, 1, &rlen, &wlen);
    }

    dev->generic.online=0;
    sis3100sfi_check_ident(dev);

    if (dev->generic.online) {
#if 0
        if (ioctl(info->p_vme, SIS1100_RESET)<0) {
            printf("ioctl(SIS1100_RESET(vme): %s\n", strerror(errno));
            res=Err_System;
            goto error;
        }
        sleep(2);
#endif

        sis3100_sfi_reset(dev);

        if (SFI_R_ioctl(dev, sequencer_status, &status)<0) {
            printf("sis3100_sfi: read status: %s\n", strerror(errno));
        }
        printf("SFI status (ioctl): 0x%08x\n", status);

        if (SFI_R(dev, sequencer_status, &status)<0) {
            printf("sis3100_sfi: read status: %s\n", strerror(errno));
            res=Err_System;
            goto error;
        }

        printf("SFI status: 0x%08x\n", status);

        if ((status&0xffff)!=0xa001) {
            /*
            sfi_sequencer_status(dev, 1);
            */
            printf("sis3100_sfi: status invalid: 0x%04x\n", status&0xffff);
            res=Err_HW;
            goto error;
        }

        dev->setarbitrationlevel(dev, 0x81);
        sis3100_set_user_led(info, 1);
    }

    return OK;

error:
    if (info->p_ctrl>-1) close(info->p_ctrl);
    if (info->p_vme>-1) close(info->p_vme);
    if (info->p_mem>-1) close(info->p_mem);
    if (info->p_dsp>-1) close(info->p_dsp);
    free(info);
    dev->info=0;
    return res;
}
/*****************************************************************************/
/*****************************************************************************/
