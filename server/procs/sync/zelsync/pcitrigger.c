/*
 * procs/pci/trigger/pcitrigger.c
 * created 17.10.96 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: pcitrigger.c,v 1.14 2017/10/21 21:59:53 wuestner Exp $";

#include <debug.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../../lowlevel/sync/pci_zel/sync_pci_zel.h"
#include "../../../trigger/trigger.h"
#include "../../../trigger/pci/zelsync/zelsynctrigger.h"
#include "../../procs.h"
#include "../../procprops.h"
#include <xdrstring.h>
#include <dev/pci/zelsync.h>
#ifdef NEVER
#include "/zelnfs/r3/NetBSD/users/drochner/lkmx-current/sys/dev/pci/zelsync.h"
#endif

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "procs/sync/zelsync")

/*****************************************************************************/
static char*
getbits(unsigned int v)
{
    static char s[42];
    char st[32];
    int i, j, k, l, m;

    T(getbits)
    for (i=0; i<41; i++)
        s[i]=' ';
    s[41]=0;
    for (i=0; i<32; i++)
        st[i]='0';

    for (i=31; v; i--, v>>=1) {
        if (v&1)
            st[i]='1';
    }

    l=m=0;
    for (k=0; k<4; k++) {
        for (j=0; j<2; j++) {
            for (i=0; i<4; i++)
                s[l++]=st[m++];
            l++;
        }
        l++;
    }
    return s;
}
/*****************************************************************************/
/*
 * [0]: length
 * [1]: path
 * [2]: 0: slave 1: master
 */
plerrcode
proc_InitPCITrigger(ems_u32* p)
{
    ems_u32 *pp;
    int id;
    plerrcode res;
    char* path;

    T(proc_InitPCITrigger)
    pp=xdrstrcdup(&path, p+1);
    if (path==0)
        return plErr_NoMem;
    if ((res=syncmod_attach(path, &id))!=plOK) {
        free(path);
        *outptr++=errno;
        return res;
    }
    syncmod_init(id, *pp);
    syncmod_detach(id);
    free(path);
    return plOK;
}

plerrcode
test_proc_InitPCITrigger(ems_u32* p)
{
    if (p[0]<1)
        return plErr_ArgNum;
    if (p[0]!=xdrstrlen(p+1)+1)
        return plErr_ArgNum;

    return plOK;
}
#ifdef PROCPROPS
static procprop InitPCITrigger_prop={0, 0, "path master|slave",
    "initialisiert den ZEL-PCI-Trigger"};

procprop*
prop_proc_InitPCITrigger()
{
    return &InitPCITrigger_prop;
}
#endif
char name_proc_InitPCITrigger[]="InitPCITrigger";
int ver_proc_InitPCITrigger=1;

/*****************************************************************************/
/*
 * [0]: length
 * [1]: path
 */
plerrcode
proc_ClearPCITrigger(ems_u32* p)
{
    int id;
    plerrcode res;
    char* path;

    T(proc_ClearPCITrigger)
    xdrstrcdup(&path, p+1);
    if (path==0)
        return plErr_NoMem;
    if ((res=syncmod_attach(path, &id))!=plOK) {
        free(path);
        *outptr++=errno;
         return res;
    }
    syncmod_clear(id);
    syncmod_detach(id);
    free(path);
    return plOK;
}

plerrcode
test_proc_ClearPCITrigger(ems_u32* p)
{
    if (p[0]<1)
        return(plErr_ArgNum);
    if (p[0]!=xdrstrlen(p+1))
        return(plErr_ArgNum);

    return plOK;
}
#ifdef PROCPROPS
static procprop ClearPCITrigger_prop={0, 0, "path",
    "setzt den ZEL-PCI-Trigger zurueck"};

procprop*
prop_proc_ClearPCITrigger()
{
    return &ClearPCITrigger_prop;
}
#endif
char name_proc_ClearPCITrigger[]="ClearPCITrigger";
int ver_proc_ClearPCITrigger=1;

/*****************************************************************************/
static char*
get8bits(int v)
{
    static char s[9];
    int i;
    for (i=0; i<8; i++) {
        s[7-i]=v&1;
        v>>=1;
    }
    s[8]=0;
    return s;
}
/*
 * static void PUWR_wait(volatile int *base, int time)
 * {
 * while(base[SYNC_TMC]<=time);
 * }
 */
static void
PUWR_write(volatile ems_u32 *base, int val, unsigned int time)
{
    printf("PUWR: %s %d\n", get8bits(val), time);
    base[SYNC_PUWR]=val;
    while (base[SYNC_TMC]<=time);
}

/*
 * [0]: length
 * [1]: path
 * [2]: selection (1: trigg 1; 2: trigg 2 4: trigg 3 ...)
 * [3]: value
 * [4]: save 
 */
plerrcode
proc_WidthPCITrigger(ems_u32* p)
{
    static volatile ems_u32 *base;
    struct syncmod_info* info;
    ems_u32 *pp;
    int id, i;
    plerrcode res;
    char* path;
    int selection, value, save, reg;

    T(proc_WidthPCITrigger)
    pp=xdrstrcdup(&path, p+1);
    if (path==0)
        return plErr_NoMem;
    selection=pp[0]&0x3f;
    value=pp[1];
    save=pp[2];

    printf("selected output(s):\n");
    if (selection& 0x1) printf("  Trigger 1\n");
    if (selection& 0x2) printf("  Trigger 2\n");
    if (selection& 0x4) printf("  Trigger 3\n");
    if (selection& 0x8) printf("  Trigger 4\n");
    if (selection&0x10) printf("  Main Trigger\n");
    if (selection&0x20) printf("  Trigger Acceptance Time\n");
    printf("value=%d; %sto be saved\n", value, save?"":"not ");
    if (value>32) {
        printf("value too large\n");
        return plErr_ArgRange;
    }

    res=syncmod_attach(path, &id);
    free(path);
    if (res!=plOK) {
        *outptr++=errno;
        return res;
    }
    info=syncmod_getinfo(id);
    base=info->base;

    /* selektieren */
    reg=selection;
    PUWR_write(base, reg, 30);
    /* counter auf null bringen */
    for (i=0; i<32; i++) {
        reg|=SYNC_PUWR_CNTC;
        PUWR_write(base, reg, 30);
        reg&=~SYNC_PUWR_CNTC;
        PUWR_write(base, reg, 30);
    }
    /* richtung aendern */
    reg|=SYNC_PUWR_UP;
    PUWR_write(base, reg, 30);
    /* counter hochzaehlen */
    for (i=0; i<value-1; i++) {
        reg|=SYNC_PUWR_CNTC;
        PUWR_write(base, reg, 30);
        reg&=~SYNC_PUWR_CNTC;
        PUWR_write(base, reg, 30);
    }
    reg|=SYNC_PUWR_CNTC;
    PUWR_write(base, reg, 30);
    if (save) {
        reg&=~SYNC_PUWR_CNTC;
        PUWR_write(base, reg, 30);
    }
    reg&=~selection;
    PUWR_write(base, reg, 30);
    PUWR_write(base, 0, 200000);

    syncmod_detach(id);
    return plOK;
}

plerrcode test_proc_WidthPCITrigger(ems_u32* p)
{
    if (p[0]<1) return(plErr_ArgNum);
    if (p[0]!=xdrstrlen(p+1)+3) return(plErr_ArgNum);

    return(plOK);
}
#ifdef PROCPROPS
static procprop WidthPCITrigger_prop={0, 0, "", ""};

procprop* prop_proc_WidthPCITrigger()
{
    return(&WidthPCITrigger_prop);
}
#endif
char name_proc_WidthPCITrigger[]="WidthPCITrigger";
int ver_proc_WidthPCITrigger=1;
/*****************************************************************************/
plerrcode
proc_SyncBit(ems_u32* p)
{
    int id;
    plerrcode res;
    char* path;
    ems_u32 reg, val, *pp;

    T(proc_SyncBit)
    pp=xdrstrcdup(&path, p+1);
    if (path==0)
        return plErr_NoMem;

    if ((res=syncmod_attach(path, &id))!=plOK) {
        free(path);
        *outptr++=errno;
        return res;
    }
    reg=pp[0];
    val=1<<pp[1];
    printf("write %08x to %d\n", val, reg);
    syncmod_write(id, reg, val);
    val=syncmod_status(id);
    *outptr++=val;
    printf("status: (%s)\n", getbits(val));
    syncmod_detach(id);
    free(path);
    return plOK;
}

plerrcode test_proc_SyncBit(__attribute__((unused)) ems_u32* p)
{
    return(plOK);
}
#ifdef PROCPROPS
static procprop SyncBit_prop={0, 0, 0, 0};

procprop* prop_proc_SyncBit()
{
    return(&SyncBit_prop);
}
#endif
char name_proc_SyncBit[]="SyncBit";
int ver_proc_SyncBit=1;
/*****************************************************************************/
/*
 * SyncOpen
 * p[0]  : No. of parameters
 * p[1..]: path
 */
plerrcode proc_SyncOpen(ems_u32* p)
{
int id;
plerrcode res;
char* path;

T(proc_SyncOpen)
xdrstrcdup(&path, p+1);
if (path==0)  return plErr_NoMem;

if ((res=syncmod_attach(path, &id))!=plOK)
  {
  free(path);
  *outptr++=errno;
  return res;
  }
*outptr++=id;
return plOK;
}

plerrcode test_proc_SyncOpen(ems_u32* p)
{
if (p[0]<1) return(plErr_ArgNum);
if (p[0]!=xdrstrlen(p+1)) return(plErr_ArgNum);

return(plOK);
}
#ifdef PROCPROPS
static procprop SyncOpen_prop={0, 0, "path",
    "xxx"};

procprop* prop_proc_SyncOpen()
{
return(&SyncOpen_prop);
}
#endif
char name_proc_SyncOpen[]="SyncOpen";
int ver_proc_SyncOpen=1;

/*****************************************************************************/

plerrcode proc_SyncClose(ems_u32* p)
{
T(proc_SyncClose)
return syncmod_detach(p[1]);
}

plerrcode test_proc_SyncClose(ems_u32* p)
{
if (p[0]<1) return(plErr_ArgNum);
if (p[0]!=xdrstrlen(p+1)) return(plErr_ArgNum);

return(plOK);
}
#ifdef PROCPROPS
static procprop SyncClose_prop={0, 0, "path",
    "xxx"};

procprop* prop_proc_SyncClose()
{
return(&SyncClose_prop);
}
#endif
char name_proc_SyncClose[]="SyncClose";
int ver_proc_SyncClose=1;

/*****************************************************************************/
/*
 * p[0]: num (==2)
 * p[1]: id
 * p[2]: reg (addr/4)
 */
plerrcode proc_SyncRead(ems_u32* p)
{
T(proc_SyncRead)
*outptr++=syncmod_read(p[1], p[2]);
return plOK;
}

plerrcode test_proc_SyncRead(ems_u32* p)
{
if (p[0]!=2) return plErr_ArgNum;
if (syncmod_valid_id(p[1])<0) return plErr_ArgRange;
return(plOK);
}
#ifdef PROCPROPS
static procprop SyncRead_prop={0, 0, 0, 0};

procprop* prop_proc_SyncRead()
{
return(&SyncRead_prop);
}
#endif
char name_proc_SyncRead[]="SyncRead";
int ver_proc_SyncRead=1;

/*****************************************************************************/
/*
 * p[0]: num (==3)
 * p[1]: id
 * p[2]: reg (addr/4)
 * p[3]: value
 */
plerrcode proc_SyncWrite(ems_u32* p)
{
T(proc_SyncWrite)
syncmod_write(p[1], p[2], p[3]);
return plOK;
}

plerrcode test_proc_SyncWrite(ems_u32* p)
{
if (p[0]!=3) return plErr_ArgNum;
if (syncmod_valid_id(p[1])<0) return plErr_ArgRange;
return(plOK);
}
#ifdef PROCPROPS
static procprop SyncWrite_prop={0, 0, 0, 0};

procprop* prop_proc_SyncWrite()
{
return(&SyncWrite_prop);
}
#endif
char name_proc_SyncWrite[]="SyncWrite";
int ver_proc_SyncWrite=1;

/*****************************************************************************/
/*
 * p[0]: num
 * p[1]: id
 * p[2]: int set   bits
 * p[3]: int clear bits
 */
plerrcode proc_SyncAuxOut(ems_u32* p)
{
struct syncmod_info* info;
T(proc_SyncAuxOut)
info=syncmod_getinfo(p[1]);
info->base[SYNC_SSCR]=((p[2]&7)<<4)|((p[3]&7)<<20);
return plOK;
}

plerrcode test_proc_SyncAuxOut(ems_u32* p)
{
if (p[0]!=3) return plErr_ArgNum;
if (syncmod_valid_id(p[1])<0) return plErr_ArgRange;
return(plOK);
}
#ifdef PROCPROPS
static procprop SyncAuxOut_prop={0, 0, 0, 0};

procprop* prop_proc_SyncAuxOut()
{
return(&SyncAuxOut_prop);
}
#endif
char name_proc_SyncAuxOut[]="SyncAuxOut";
int ver_proc_SyncAuxOut=1;

/*****************************************************************************/
/*
 * p[0]: num
 * p[1]: id
 */
plerrcode proc_SyncAuxIn(ems_u32* p)
{
struct syncmod_info* info;
T(proc_SyncAuxIn)
info=syncmod_getinfo(p[1]);
*outptr++=(info->base[SYNC_CSR]>>24)&7;
return plOK;
}

plerrcode test_proc_SyncAuxIn(ems_u32* p)
{
if (p[0]!=1) return plErr_ArgNum;
if (syncmod_valid_id(p[1])<0) return plErr_ArgRange;
return(plOK);
}
#ifdef PROCPROPS
static procprop SyncAuxIn_prop={0, 0, 0, 0};

procprop* prop_proc_SyncAuxIn()
{
return(&SyncAuxIn_prop);
}
#endif
char name_proc_SyncAuxIn[]="SyncAuxIn";
int ver_proc_SyncAuxIn=1;

/*****************************************************************************/
#if 0 /* more work needed to make it compatible multiple triggers */
/*
 * [0]: 3
 * [1]: id
 * [2]: 0: auslesen 1: pci_trigdata benutzen
 * [3]: verbose
 */
plerrcode proc_StatusPCITrigger(ems_u32* p)
{
unsigned int status, evcount, tmc, tdt, rej;

T(proc_StatusPCITrigger)

if (p[2]==0)
  {
  status=syncmod_status(p[1]);
  evcount=syncmod_evcount(p[1]);
  tmc=syncmod_tmc(p[1]);
  tdt=syncmod_tdt(p[1]);
  rej=syncmod_reje(p[1]);
  //rej=pci_trigdata.reje;
  *outptr++=status;
  *outptr++=evcount;
  *outptr++=tmc;
  *outptr++=tdt;
  *outptr++=rej;
  if ((p[0]>2) && (p[3]>0))
    {
    printf("status: (%s)\n", getbits(status));
    if (status&1) printf("Master ");
    if (status&2) printf("GO ");
    if (status&4) printf("T4LATCH ");
    if (status&8) printf("TAW_ENA ");
    if (status&&0x70) printf("AUX_OUT(%d) ", (status&0x70)>>4);
    if (status&0x80) printf("AUX0_RES_TRIG ");
    if (status&0x700) printf("Int_Ena_AUX(%d) ", (status&0x700)>>8);
    if (status&0x800) printf("Int_Ena_EOC ");
    if (status&0x1000) printf("Int_Ena_SI ");
    if (status&0x2000) printf("Int_Ena_TRIG ");
    if (status&0x4000) printf("Int_Ena_GAP ");
    if (status&0x8000) printf("GSI ");
    if (status&0xf0000) printf("Trigger(%d) ", (status&0xf0000)>>16);
    if (status&0x100000) printf("GO_RING ");
    if (status&0x200000) printf("VETO ");
    if (status&0x400000) printf("SI ");
    if (status&0x800000) printf("INH ");
    if (status&0x7000000) printf("AUX_IN(%d) ", (status&0x7000000)>>24);
    if (status&0x8000000) printf("EOC ");
    if (status&0x10000000) printf("SI_RING ");
    if (status&0x40000000) printf("GAP ");
    if (status&0x80000000) printf("TDT_RING ");
    printf("\n");
    printf("eventcount=%d; tmc=%d; tdt=%d; rej=%d\n",
      evcount, tmc, tdt, rej);
    }
  }
else
  {
//   struct trigstatus pci_trigdata;
//   if (get_pci_trigdata(&trigger, &pci_trigdata, 0)<0)
//     return plErr_BadModTyp;
    if (print_zelsync_status(&trigger, &outptr, (p[0]>2) && (p[3]>0))<0)
        return plErr_BadModTyp;
//   *outptr++=pci_trigdata.mbx;
//   *outptr++=status;
//   *outptr++=pci_trigdata.evc;
//   *outptr++=pci_trigdata.tmc;
//   *outptr++=pci_trigdata.tdt;
//   *outptr++=pci_trigdata.reje;
//   *outptr++=pci_trigdata.tgap[0];
//   *outptr++=pci_trigdata.tgap[1];
//   *outptr++=pci_trigdata.tgap[2];
//   if ((p[0]>2) && (p[3]>0))
//     {
//     printf("mailbox: (%s)\n", getbits(pci_trigdata.mbx));
//     printf("status : (%s)\n", getbits(status));
//     printf("eventcount=%d; tmc=%d; tdt=%d; rej=%d\n"
//            "gap1=%d; gap2=%d; gap3=%d\n",
//       pci_trigdata.evc,
//       pci_trigdata.tmc,
//       pci_trigdata.tdt,
//       pci_trigdata.reje, 
//       pci_trigdata.tgap[0], pci_trigdata.tgap[1], pci_trigdata.tgap[2]);
//     }
  }
return(plOK);
}

plerrcode test_proc_StatusPCITrigger(ems_u32* p)
{
if ((p[0]<2) || (p[0]>3)) return(plErr_ArgNum);
if (syncmod_valid_id(p[1])) return(plErr_ArgRange);

return(plOK);
}
#ifdef PROCPROPS
static procprop StatusPCITrigger_prop={0, 0, "id code verbose",
    "xxx"};

procprop* prop_proc_StatusPCITrigger()
{
return(&StatusPCITrigger_prop);
}
#endif
char name_proc_StatusPCITrigger[]="StatusPCITrigger";
int ver_proc_StatusPCITrigger=1;

#endif
/*****************************************************************************/
/*****************************************************************************/
