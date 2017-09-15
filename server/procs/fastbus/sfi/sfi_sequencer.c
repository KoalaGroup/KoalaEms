/*
 * procs/fastbus/sfi/sfi_sequencer.c
 * 
 * created: 14.05.97
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sfi_sequencer.c,v 1.6 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errno.h>
#include <stdio.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../../lowlevel/fastbus/sfi/sfilib.h"
#include "../../../lowlevel/fastbus/sfi/sfi_lists.h"
#include "../../procs.h"
#include "../../procprops.h"
#include <dev/pci/zelsync.h>

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/fastbus/sfi")

/*****************************************************************************/
/*
loadFRDB
p[0] : No. of parameters (==4)
p[1] : address
p[2] : primary address
p[3] : secondary address
p[4] : count
*/

plerrcode proc_SFIloadFRDB(ems_u32* p)
{
    int res;
    res=sfi_loadlist_FRDB(p[1], p[2], p[3], p[4]);
    if (res) {
        *outptr++=res;
        return plErr_Other;
    } else
        return plOK;
}

plerrcode test_proc_SFIloadFRDB(ems_u32* p)
{
    if (p[0]!=4) return plErr_ArgNum;
    return plOK;
}

#ifdef PROCPROPS
static procprop SFIloadFRDB_prop={1, 1, "seq. addr., pa, sa, count", ""};

procprop* prop_proc_SFIloadFRDB()
{
    return &SFIloadFRDB_prop;
}
#endif
char name_proc_SFIloadFRDB[]="SFIloadFRDB";
int ver_proc_SFIloadFRDB=1;

/*****************************************************************************/
/*
loadFRDBp
p[0] : No. of parameters (==4)
p[1] : address
p[2] : primary address
p[3] : secondary address
p[4] : count
*/

plerrcode proc_SFIloadFRDBp(ems_u32* p)
{
    int res;
    res=sfi_loadlist_FRDBp(p[1], p[2], p[3], p[4]);
    if (res) {
        *outptr++=res;
        return plErr_Other;
    } else
        return plOK;
}

plerrcode test_proc_SFIloadFRDBp(ems_u32* p)
{
    if (p[0]!=4) return plErr_ArgNum;
    return plOK;
}

#ifdef PROCPROPS
static procprop SFIloadFRDBp_prop={1, 1, "seq. addr., pa, sa, count", ""};

procprop* prop_proc_SFIloadFRDBp()
{
    return &SFIloadFRDBp_prop;
}
#endif
char name_proc_SFIloadFRDBp[]="SFIloadFRDBp";
int ver_proc_SFIloadFRDBp=1;

/*****************************************************************************/
/*
SFIexec
p[0] : No. of parameters (==2)
p[1] : address
p[2] : count
*/

plerrcode proc_SFIexec(ems_u32* p)
{
    struct sfiexec exec;
    struct sfi_status err;

    exec.err=&err;
    exec.addr=p[1];
    exec.dma_dest=(caddr_t)(outptr+2)-sfi.backbase;
    exec.len=p[2];
    exec.fifo_dest=outptr+1;
    exec.fifo_size=1;
    if (ioctl(sfi.path, SFI_EXEC_LIST, &exec)<0) {
        printf("SFIexec: %s\n", strerror(errno));
        return plErr_Other;
    }
    outptr[0]=outptr[1]>>24;
    outptr[1]&=0xffffff;
    if (outptr[0]==8) {
        outptr[0]=0;
        outptr[1]--;
    }
/*sfi_reporterror(&err);*/
    outptr+=outptr[1]+2;
    return plOK;
}

plerrcode test_proc_SFIexec(ems_u32* p)
{
    if (p[0]!=2) return plErr_ArgNum;
    return plOK;
}

#ifdef PROCPROPS
static procprop SFIexec_prop={1, -1, "seq. addr., count", ""};

procprop* prop_proc_SFIexec()
{
    return &SFIexec_prop;
}
#endif
char name_proc_SFIexec[]="SFIexec";
int ver_proc_SFIexec=1;

/*****************************************************************************/
/*
SFIexecp
p[0] : No. of parameters (==3)
p[1] : address
p[2] : count
p[3] : timeout
*/

plerrcode proc_SFIexecp(ems_u32* p)
{
    struct sfiexec exec;
    struct sfi_status err;
    int *op, res;

/* NIM 1 setzen */
/* SFI_W(&sfi, write_vme_out_signal)=0x100; */
    op=outptr;
    exec.err=&err;
    exec.addr=p[1];
    exec.dma_dest=(caddr_t)(outptr+2)-sfi.backbase;
/*
    printf("proc_SFIexecp: dma_dest=0x%08x\n", exec.dma_dest);
    printf("proc_SFIexecp: outptr=0x%08x, sfi.backbase=0x%08x\n", outptr,
            sfi.backbase);
*/
    exec.len=p[2];
    exec.fifo_dest=outptr+1;
    exec.fifo_size=1;
    exec.timeout=p[3];
    if ((res=ioctl(sfi.path, SFI_EXEC_LISTP, &exec))<0) {
        return plErr_Other;
    }
    outptr[0]=outptr[1]>>24;
    outptr[1]&=0xffffff;
    if (outptr[0]==8) {
        outptr[0]=0;
        outptr[1]--;
    }
    outptr+=outptr[1]+2;
/* NIM 1 zuruecknehmen */
    SFI_W(&sfi, write_vme_out_signal)=0x1000000;
    return plOK;
}

plerrcode test_proc_SFIexecp(ems_u32* p)
{
    if (p[0]!=3) return plErr_ArgNum;
    return plOK;
}

#ifdef PROCPROPS
static procprop SFIexecp_prop={1, -1, "seq. addr., count, timeout", ""};

procprop* prop_proc_SFIexecp()
{
    return &SFIexecp_prop;
}
#endif
char name_proc_SFIexecp[]="SFIexecp";
int ver_proc_SFIexecp=1;

/*****************************************************************************/
/*
 * Blockreadout, ueber ECL-Eingang im SFI gestartet:
 *   einmal waehrend der Initialisierung: SFIloadFRDB
 *   vor jedem Run:                       SFIinitFRDB
 *   nach jedem Run:                      SFIseqrest
 *   waehrend des Readouts:
 *     nur eine Prozedur:                 SFIread_initFRDB
 *     auch andere FB-Prozeduren:           SFIreadFRDB
 *                                          ... (andere Prozeduren)
 *                                          SFIinitFRDB
 */
/*
 * p[0] : No. of parameters (==4)
 * p[1] : address
 * p[2] : primary address
 * p[3] : secondary address
 * p[4] : max. count
 */
plerrcode proc_SFI_FRDBload(ems_u32* p)
{
    int i;
    struct sfilist list;
    struct sficommand comm[20];

    list.list=comm;
    i=0;

    comm[i].cmd=seq_wait_go_flag;  comm[i++].data=0;
/* NIM 2 u. 3 setzen */
    comm[i].cmd=seq_out;           comm[i++].data=0x600;
    comm[i].cmd=seq_load_address;
    comm[i++].data=(u_int32_t)((caddr_t)lowbuf_extrabuffer()
                 -sfi.backbase+sfi.dma_vmebase);
    comm[i].cmd=seq_prim_dsr;      comm[i++].data=p[2];
    comm[i].cmd=seq_secad_w;       comm[i++].data=p[3];
    comm[i].cmd=seq_dma_r_clearwc; comm[i++].data=(p[4] & 0xffffff) | 0xa<<24;
    comm[i].cmd=seq_discon;        comm[i++].data=0;
    comm[i].cmd=seq_store_statwc;  comm[i++].data=0;
/* comm[i].cmd=seq_clear_go_flag; comm[i++].data=0; */
/* ECL sfi.outbit setzen *//* und NIM 3 zuruecknehmen */
    comm[i].cmd=seq_out;           comm[i++].data=(0x10<<sfi.outbit)+0x4000000;
    comm[i].cmd=seq_disable_ram;   comm[i++].data=0;
    list.size=i;
    list.addr=p[1];
    if (ioctl(sfi.path, SFI_LOAD_LIST, &list)<0) {
        printf("proc_SFI_FRDBload: %s\n", strerror(errno));
        return plErr_HW;
    } else
        return plOK;
}
/*---------------------------------------------------------------------------*/
/*
 * p[0] : No. of parameters (==1)
 * p[1] : address
 */
plerrcode proc_SFI_FRDBinit(ems_u32* p)
{
    SEQ_W(&sfi, seq_out)=0x100000<<sfi.outbit;
    SEQ_W(&sfi, seq_clear_go_flag)=0;
    SEQ_W(&sfi, seq_enable_ram+(p[1]>>2))=0;
    return plOK;
}
/*---------------------------------------------------------------------------*/
/*
 * p[0] : No. of parameters (==0)
 */
plerrcode proc_SFI_FRDBread(ems_u32* p)
{
    int flags, dma_status, num, ss;
    int mask=0x7000000<<sfi.inbit;
/* wait for end of transfer */
    if ((sfi.syncinf->base[SYNC_CSR]&mask)==0) {
        struct timeval tv0, tv1;
        gettimeofday(&tv0, 0);
        while ((sfi.syncinf->base[SYNC_CSR]&mask)==0) {
            gettimeofday(&tv1, 0);
            if (((tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec))
                    >1000000) {
                printf("proc_SFI_FRDBread: timeout\n");
                goto fehler;
            }
        }
    }
    if ((SFI_R(&sfi, sequencer_status)&0x8001)!=0x8001) goto fehler;
    SEQ_W(&sfi, seq_out)=0x100000<<sfi.outbit;
    SEQ_W(&sfi, seq_clear_go_flag)=0;

    if ((flags=SFI_R(&sfi, fifo_flags))&0x10) { /* FIFO empty */
        dma_status=SFI_R(&sfi, read_seq2vme_fifo); /* dummy read */
        flags=SFI_R(&sfi, fifo_flags);
        if (flags&0x10) { /* FIFO empty */
            printf("FIFO empty\n");
            goto fehler;
        }
    }
    dma_status=SFI_R(&sfi, read_seq2vme_fifo);
    num=dma_status&0xffffff;
    ss=(dma_status>>24)&0x3f;
    if (ss&8) num--; /* stopped by limit counter */
    if (ss&0x30) {
        if (ss&0x10) printf("FB timeout\n");
        if (ss&0x20) printf("VME timeout\n");
        goto fehler;
    }
    *outptr++=ss&7;
    if ((ss&7)!=0) {
        if (((ss&7)!=2) || (num==0))
        printf("num=%d; ss=%x\n", num, ss);
/*
    else if (num)
        num--;
*/
    }
    *outptr++=num;
    memcpy(outptr, lowbuf_extrabuffer(), num*sizeof(int));
    outptr+=num;
/* NIM 2 zuruecknehmen */
    SEQ_W(&sfi, seq_out)=0x2000000;
    return plOK;

fehler:
    sfi_sequencer_status(&sfi, 1);
    sfi_restart_sequencer(&sfi);
    SEQ_W(&sfi, seq_out)=0x100000<<sfi.outbit;
    SEQ_W(&sfi, seq_clear_go_flag)=0;
    return plErr_HW;
}
/*---------------------------------------------------------------------------*/
/*
 * p[0] : No. of parameters (==1)
 * p[1] : address
 */
plerrcode proc_SFI_FRDBreadinit(ems_u32* p)
{
    plerrcode res;
    res=proc_SFI_FRDBread(p);
    SEQ_W(&sfi, seq_enable_ram+(p[1]>>2))=0;
    return res;
}
/*---------------------------------------------------------------------------*/
plerrcode test_proc_SFI_FRDBload(ems_u32* p)
{
    if (p[0]!=4) return plErr_ArgNum;
    return plOK;
}
plerrcode test_proc_SFI_FRDBinit(ems_u32* p)
{
    if (p[0]!=1) return plErr_ArgNum;
    return plOK;
}
plerrcode test_proc_SFI_FRDBread(ems_u32* p)
{
    if (p[0]!=0) return plErr_ArgNum;
    return plOK;
}
plerrcode test_proc_SFI_FRDBreadinit(ems_u32* p)
{
    if (p[0]!=1) return plErr_ArgNum;
    return plOK;
}

#ifdef PROCPROPS
static procprop SFI_FRDBload_prop={0, 0, "seq. addr., pa, sa, count", ""};
static procprop SFI_FRDBinit_prop={0, 0, "seq. addr.", ""};
static procprop SFI_FRDBread_prop={1, -1, "void", ""};
static procprop SFI_FRDBreadinit_prop={1, -1, "seq. addr.", ""};

procprop* prop_proc_SFI_FRDBload()
{
    return(&SFI_FRDBload_prop);
}
procprop* prop_proc_SFI_FRDBinit()
{
    return(&SFI_FRDBinit_prop);
}
procprop* prop_proc_SFI_FRDBread()
{
    return(&SFI_FRDBread_prop);
}
procprop* prop_proc_SFI_FRDBreadinit()
{
    return(&SFI_FRDBreadinit_prop);
}
#endif
char name_proc_SFI_FRDBload[]="SFI_FRDBload";
int ver_proc_SFI_FRDBload=1;
char name_proc_SFI_FRDBinit[]="SFI_FRDBinit";
int ver_proc_SFI_FRDBinit=1;
char name_proc_SFI_FRDBread[]="SFI_FRDBread";
int ver_proc_SFI_FRDBread=1;
char name_proc_SFI_FRDBreadinit[]="SFI_FRDBreadinit";
int ver_proc_SFI_FRDBreadinit=1;

/*****************************************************************************/
/*****************************************************************************/
