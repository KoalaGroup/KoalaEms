/******************************************************************************
*                                                                             *
* objects/notifystatus.c                                                      *
*                                                                             *
* created     : 05.04.97                                                      *
* last changed: 05.04.97                                                      *
*                                                                             *
* PW                                                                          *
*                                                                             *
*******************************************************************************/
static const char* cvsid __attribute__((unused))=
    "$ZEL: notifystatus.c,v 1.7 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdlib.h>
#include <objecttypes.h>
#include <actiontypes.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
#include "notifystatus.h"
#include "../commu/commu.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

RCS_REGISTER(cvsid, "objects")

void notifystatus(status_actions action, Object object, unsigned int size,
    ems_u32* id)
{
    ems_u32* data;
    data=(ems_u32*)malloc((size+2)*sizeof(ems_u32));
    if (data!=0) {
        unsigned int i;
        data[0]=action;
        data[1]=object;
        for (i=0; i<size; i++) data[i+2]=id[i];
        send_unsolicited(Unsol_StatusChanged, data, size+2);
        free(data);
    } else {
        printf("no memory for unsolicited message 'StatusChanged'.\n");
    }
}
/*****************************************************************************/
/*****************************************************************************/
