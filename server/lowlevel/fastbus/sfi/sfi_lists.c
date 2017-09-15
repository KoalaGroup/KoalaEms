/*
 * $ZEL: sfi_lists.c,v 1.5 2011/04/06 20:30:23 wuestner Exp $
 * lowlevel/fastbus/sfi/sfi_lists.c
 * 
 * created:      23.05.97
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sfi_lists.c,v 1.5 2011/04/06 20:30:23 wuestner Exp $";

#include <errno.h>
#include <stdlib.h>
#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../fastbus.h"
#include "sfilib.h"
#include "sfi_lists.h"

#undef PULSE

RCS_REGISTER(cvsid, "lowlevel/fastbus/sfi")

/*****************************************************************************/

int sfi_loadlist_FRDB(int addr, int pa, int sa, int num)
{
    struct sfilist list;
    struct sficommand comm[20];
    int res=0, i;

    list.list=comm;
    i=0;
    #ifdef PULSE
    /* NIM 3 setzen */
    comm[i].cmd=seq_out;           comm[i++].data=0x404;
    #endif
    comm[i].cmd=seq_prim_dsr;      comm[i++].data=pa;
    comm[i].cmd=seq_secad_w;       comm[i++].data=sa;
    comm[i].cmd=seq_dma_r_clearwc; comm[i++].data=(num & 0xffffff) | 0xa<<24;
    comm[i].cmd=seq_discon;        comm[i++].data=0;
    comm[i].cmd=seq_store_statwc;  comm[i++].data=0;
    comm[i].cmd=seq_set_cmd_flag;  comm[i++].data=0;
    #ifdef PULSE
    /* NIM 1 setzen */
    comm[i].cmd=seq_out;           comm[i++].data=0x101;
    /* NIM 1 und 3 wieder zuruecknehmen */
    comm[i].cmd=seq_out;           comm[i++].data=0x5050000;
    #endif
    comm[i].cmd=seq_disable_ram;   comm[i++].data=0;
    list.size=i;
    list.addr=addr;
    if (ioctl(sfi.path, SFI_LOAD_LIST, &list)<0) {
          res=errno;
          printf("sfi_loadlist_FRDB: %s\n", strerror(errno));
    }
    return res;
}
/*****************************************************************************/

int sfi_loadlist_FRDBp(int addr, int pa, int sa, int num)
{
    struct sfilist list;
    struct sficommand comm[20];
    int res=0, i;

    list.list=comm;
    i=0;
    #ifdef PULSE
    /* NIM 3 setzen */
    comm[i].cmd=seq_out;           comm[i++].data=0x400;
    #endif
    comm[i].cmd=seq_prim_dsr;      comm[i++].data=pa;
    comm[i].cmd=seq_secad_w;       comm[i++].data=sa;
    comm[i].cmd=seq_dma_r_clearwc; comm[i++].data=(num & 0xffffff) | 0xa<<24;
    comm[i].cmd=seq_discon;        comm[i++].data=0;
    comm[i].cmd=seq_store_statwc;  comm[i++].data=0;
    #ifdef PULSE
    /* ECL 3 setzen */
    comm[i].cmd=seq_out;           comm[i++].data=0x20;
    /* ECL 3 und NIM 3 zuruecknehmen */
    comm[i].cmd=seq_out;           comm[i++].data=0x4200000;
    #endif
    comm[i].cmd=seq_disable_ram;   comm[i++].data=0;
    list.size=i;
    list.addr=addr;
    if (ioctl(sfi.path, SFI_LOAD_LIST, &list)<0) {
          res=errno;
          printf("sfi_loadlist_FRDB: %s\n", strerror(errno));
      }
    printf("sfi_loadlist_FRDB: fertig\n");
    return res;
}
/*****************************************************************************/
/*****************************************************************************/
