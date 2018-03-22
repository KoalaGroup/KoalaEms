/*
 * lowlevel/sync/pci_zel/syncmod.c
 * created 23.10.96 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: syncmod.c,v 1.11 2017/10/22 22:46:36 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <rcs_ids.h>

#include "../../../main/signals.h"
#include "synchrolib.h"
#include "sync_pci_zel.h"
#include <dev/pci/zelsync.h>

#ifdef DMALLOC
#include dmalloc.h
#endif

RCS_REGISTER(cvsid, "lowlevel/sync/pci_zel")

/*****************************************************************************/

static struct syncmod_info* infos=0;
static unsigned int numinfo=0;

/*****************************************************************************/
int sync_pci_zel_low_printuse(__attribute__((unused)) FILE* outfilepath)
{
    /* nothing to configure */
    return 0;
}
/*****************************************************************************/
errcode sync_pci_zel_low_init(__attribute__((unused)) char* arg)
{
    T(pci_syncmod_low_init)
    /*infos=0;*/
    /*numinfo=0;*/
    return OK;
}
/*****************************************************************************/
errcode sync_pci_zel_low_done()
{
    T(pci_syncmod_low_done)
    /*
    for (i=0; i<numinfo; i++) {
        while (infos[i].valid {
            syncmod_detach(i);
        }
    }
    free(infos);
    infos=0;
    numinfo=0;
    */
    return OK;
}
/*****************************************************************************/
plerrcode syncmod_attach(char* path, int* id)
{
    unsigned int i;

    T(syncmod_attach)
    for (i=0; i<numinfo; i++) {
        if (infos[i].valid && (strcmp(infos[i].pathname, path)==0)) {
            infos[i].count++;
            *id=i;
            return plOK;
        }
    }

    i=0;
    while ((i<numinfo) && infos[i].valid) i++;
    if (i==numinfo) {
        struct syncmod_info* help;
        help=(struct syncmod_info*)malloc((numinfo+1)*sizeof(struct syncmod_info));
        for (i=0; i<numinfo; i++) help[i]=infos[i];
        free(infos);
        infos=help;
        i=numinfo;
        numinfo++;
    }
    infos[i].valid=0;
    infos[i].count=0;
    infos[i].path=-1;
    infos[i].intctrl.signal=0;
    infos[i].base=0;
    infos[i].pathname=(char*)malloc(strlen(path)+1);
    strcpy(infos[i].pathname, path);

    if (syncmod_map(infos+i)<0) {
        free(infos[i].pathname);
        infos[i].valid=0;
        return plErr_System;
    } else {
        infos[i].valid=1;
        infos[i].count=1;
        *id=i;
        return plOK;
    }
}
/*****************************************************************************/
int syncmod_valid_id(int id)
{
    if (((unsigned int)id<numinfo) && infos[id].valid)
        return 0;
    else
        return -1;
}
/*****************************************************************************/
plerrcode syncmod_detach(int id)
{
    T(syncmod_detach)
    if ((unsigned int)id>numinfo) {
        printf("syncmod_detach: id %d invalid\n", id);
        return plOK;
    }
    if (!infos[id].valid) {
        printf("info[%d] not valid\n", id);
        return plOK;
    }
    infos[id].count--;
    if (infos[id].count==0) {
        if (infos[id].intctrl.signal) {
            remove_signalhandler(infos[id].intctrl.signal);
        }
        syncmod_unmap(infos+id);
        free(infos[id].pathname);
        infos[id].valid=0;
    }
    return plOK;
}
/*****************************************************************************/
struct syncmod_info* syncmod_getinfo(int id)
{
    return infos+id;
}
/*****************************************************************************/
void syncmod_init(int id, int master)
{
    infos[id].base[SYNC_RESET]=0;
    infos[id].base[SYNC_SSCR]=master?SYNC_SET_MASTER:SYNC_RES_MASTER;
}
/*****************************************************************************/
void syncmod_reset(int id)
{
    infos[id].base[SYNC_RESET]=0;
    infos[id].base[SYNC_SSCR]=SYNC_RES_MASTER;
}
/*****************************************************************************/
void syncmod_clear(int id)
{
    infos[id].base[SYNC_CSR]=SYNC_RES_DEADTIME|SYNC_RES_TRIGINT;
}
/*****************************************************************************/
void syncmod_write(int id, int reg, unsigned int val)
{
    /*
     * printf("syncmod_write: %08x --> [%p]\n", val, &infos[id].base[reg]);
     * printf("id=%d; reg=%d; &infos=%p; &infos[id]=%p\n",
     *     id, reg, &infos,  &infos[id]);
     * printf("infos[id].base=%p; &infos[id].base[reg]=%p\n",
     *     infos[id].base, &infos[id].base[reg]);
     */
    infos[id].base[reg]=val;
}
/*****************************************************************************/
unsigned int syncmod_read(int id, int reg)
{
    /*
     * printf("syncmod_read: [%p] -->\n", &infos[id].base[reg]);
     * printf("id=%d; reg=%d; &infos=%p; &infos[id]=%p\n",
     *     id, reg, &infos,  &infos[id]);
     * printf("infos[id].base=%p; &infos[id].base[reg]=%p\n",
     *     infos[id].base, &infos[id].base[reg]);
     */
    return infos[id].base[reg];
}
/*****************************************************************************/
int syncmod_status(int id)
{
    return infos[id].base[SYNC_CSR];
}
/*****************************************************************************/
int syncmod_ctime(int id)
{
    return infos[id].base[SYNC_CVTR];
}
/*****************************************************************************/
int syncmod_evcount(int id)
{
    return infos[id].base[SYNC_EVC];
}
/*****************************************************************************/
int syncmod_tmc(int id)
{
    return infos[id].base[SYNC_TMC];
}
/*****************************************************************************/
int syncmod_tdt(int id)
{
    return infos[id].base[SYNC_TDTR];
}
/*****************************************************************************/
int syncmod_reje(int id)
{
    return infos[id].base[SYNC_REJE];
}
/*****************************************************************************/
/*****************************************************************************/
