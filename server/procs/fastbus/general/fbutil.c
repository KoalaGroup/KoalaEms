/*
 * procs/fastbus/general/fbutil.c
 * created: 13.10.94
 * 23.May.2002 PW: multi crate support
 * 24.Oct.2002 PW: FBpulse FBin FBout
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: fbutil.c,v 1.11 2011/04/06 20:30:32 wuestner Exp $";

#include <sconf.h>
#include <errno.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../../procs.h"
#include "../../procprops.h"
#include "../../../objects/domain/dom_ml.h"
#include "../../../lowlevel/fastbus/fastbus.h"
#include "../../../lowlevel/devices.h"
#include "../fastbus_verify.h"

extern ems_u32* outptr;
extern int wirbrauchen;
extern int *memberlist;

#define get_device(class, crate) \
    (struct fastbus_dev*)get_gendevice((class), (crate))

RCS_REGISTER(cvsid, "procs/fastbus/general")

/*****************************************************************************/
/*
FBArbLevel
p[0] : No. of parameters (==2)
p[1] : crate
p[2] : int arbitrationlevel
*/

plerrcode proc_FBArbLevel(ems_u32* p)
{
        struct fastbus_dev* dev=get_device(modul_fastbus, p[1]);
        if (p[0]>1)
            *outptr++=dev->setarbitrationlevel(dev, p[2]);
        else {
            dev->getarbitrationlevel(dev, outptr);
            outptr++;
        }
        return plOK;
}

plerrcode test_proc_FBArbLevel(ems_u32* p)
{
        plerrcode pres;

        if ((p[0]!=1) && (p[0]!=2)) return plErr_ArgNum;
        if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK)
            return pres;
        wirbrauchen=1;
        return plOK;
}
#ifdef PROCPROPS
static procprop FBArbLevel_prop={0, 1, 0, 0};

procprop* prop_proc_FBArbLevel()
{
        return(&FBArbLevel_prop);
}
#endif
char name_proc_FBArbLevel[]="FBArbLevel";
int ver_proc_FBArbLevel=3;

/*****************************************************************************/
/*
FModulID
p[0] : No. of parameters (==2)
p[1] : crate
p[2] : primary address
*/
plerrcode proc_FModulID(ems_u32* p)
{
        ems_u32 data, ss;
        int res;
        struct fastbus_dev* dev=get_device(modul_fastbus, p[1]);

        res=dev->FRC(dev, p[2], 0, &data, &ss);
        if (res) {
            *outptr++=res;
            return plErr_System;
        }
        *outptr++=ss?0:data>>16;
        return plOK;
}

plerrcode test_proc_FModulID(ems_u32* p)
{
        plerrcode pres;

        if (p[0]!=2) return plErr_ArgNum;
        if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK)
            return pres;
        wirbrauchen=1;
        return plOK;
}
#ifdef PROCPROPS
static procprop FModulID_prop={0, 1, 0, 0};

procprop* prop_proc_FModulID()
{
        return(&FModulID_prop);
}
#endif
char name_proc_FModulID[]="FModulID";
int ver_proc_FModulID=2;

/*****************************************************************************/
/*
FModulList
p[0] : No. of parameters (==1)
p[1] : crate
*/
plerrcode proc_FModulList(ems_u32* p)
{
        ems_u32 pa, ss, data;
        int j=0;
        ems_u32* help;
        int res;
        struct fastbus_dev* dev=get_device(modul_fastbus, p[1]);

        help=outptr++;
        for (pa=0; pa<32; pa++) {
                res=dev->FRC(dev, pa, 0, &data, &ss);
                if (res) {
                    printf("FModulList:FRC: res=%d; errno=%d\n", res, errno);
                    outptr=help;
                    *outptr++=errno;
                    return plErr_System;
                }
                if (!ss) {
                    *outptr++=pa;
                    *outptr++=data>>16;
                    j++;
                }
        }
        *help=j;
        return plOK;
}

plerrcode test_proc_FModulList(ems_u32* p)
{
        plerrcode pres;

        if (p[0]!=1) return plErr_ArgNum;
        if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK)
            return pres;
        wirbrauchen=65;
        return plOK;
}
#ifdef PROCPROPS
static procprop FModulList_prop={1, 65, 0, 0};

procprop* prop_proc_FModulList()
{
        return(&FModulList_prop);
}
#endif
char name_proc_FModulList[]="FModulList";
int ver_proc_FModulList=3;

/*****************************************************************************/
/*
FBout
p[0] : No. of parameters (==1)
p[1] : crate
p[2] : option (sis3100/sfi: 0: sfi 1: sis3100)
p[3] : set
p[4] : res
*/
plerrcode proc_FBout(ems_u32* p)
{
    struct fastbus_dev* dev=get_device(modul_fastbus, p[1]);

    dev->LEVELOUT(dev, (int)p[2], p[3], p[4]);
    return plOK;
}

plerrcode test_proc_FBout(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=4) return plErr_ArgNum;
    if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}
char name_proc_FBout[]="FBout";
int ver_proc_FBout=1;

/*****************************************************************************/
/*
FBin
p[0] : No. of parameters (==1)
p[1] : crate
p[2] : option (sis3100/sfi: 0: sfi 1: sis3100)
*/
plerrcode proc_FBin(ems_u32* p)
{
    struct fastbus_dev* dev=get_device(modul_fastbus, p[1]);
    dev->LEVELIN(dev, (int)p[2], outptr++);
    return plOK;
}

plerrcode test_proc_FBin(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=2) return plErr_ArgNum;
    if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=1;
    return plOK;
}
char name_proc_FBin[]="FBin";
int ver_proc_FBin=1;

/*****************************************************************************/
/*
FBpulse
p[0] : No. of parameters (==1)
p[1] : crate
p[2] : option (sis3100/sfi: 0: sfi 1: sis3100)
p[3] : mask
*/
plerrcode proc_FBpulse(ems_u32* p)
{
    struct fastbus_dev* dev=get_device(modul_fastbus, p[1]);

    dev->PULSEOUT(dev, (int)p[2], p[3]);
    return plOK;
}

plerrcode test_proc_FBpulse(ems_u32* p)
{
    plerrcode pres;

    if (p[0]!=3) return plErr_ArgNum;
    if ((pres=find_odevice(modul_fastbus, p[1], 0))!=plOK)
        return pres;
    wirbrauchen=0;
    return plOK;
}
char name_proc_FBpulse[]="FBpulse";
int ver_proc_FBpulse=1;

/*****************************************************************************/
/*
 * p[0]: argcount==0
 */
plerrcode proc_FBverifylist(ems_u32* p)
{
    int errors=0, i;

    if (memberlist) {
        for (i=memberlist[0]; i>0; i--) {
            ml_entry* module=ModulEnt(i);
            if (verify_fastbus_module(module)) {
                *outptr++=i;
                errors++;
            }
        }
    } else {
        for (i=0; i<modullist->modnum; i++) {
            ml_entry* module=modullist->entry+i;
            if (module->modulclass==modul_fastbus) {
                if (verify_fastbus_module(module)) {
                    *outptr++=i;
                    errors++;
                }
            }
        }
    }
    if (errors) return plErr_HWTestFailed;
    return plOK;
}

plerrcode test_proc_FBverifylist(ems_u32* p)
{
    if (p[0]) return plErr_ArgNum;
    return plOK;
}

char name_proc_FBverifylist[] = "FBverifylist";
int ver_proc_FBverifylist = 1;
/*****************************************************************************/
/*
 * p[0]: argcount==1
 * p[1]: module
 */
plerrcode proc_FBverify(ems_u32* p)
{
    return verify_fastbus_module(ModulEnt(p[1]));
}

plerrcode test_proc_FBverify(ems_u32* p)
{
    if (p[0]!=1) return plErr_ArgNum;
    return plOK;
}

char name_proc_FBverify[] = "FBverify";
int ver_proc_FBverify = 1;
/*****************************************************************************/
/*****************************************************************************/
