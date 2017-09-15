/*
 * procs/unixvme/vme_simple.c
 * created: 25.Jan.2001 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: vme_simple.c,v 1.10 2011/04/06 20:30:35 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include "../procs.h"
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/unixvme/vme.h"
#include "../../lowlevel/unixvme/vme_address_modifiers.h"
#include "../../lowlevel/devices.h"

extern ems_u32 *outptr;
extern int* memberlist;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/unixvme")

/*****************************************************************************/
static plerrcode
test_vmeparm(ems_u32* p, int n)
{
    if (p[0]!=n) return(plErr_ArgNum);
    if (!valid_module(p[1], modul_vme, 0)) return plErr_ArgRange;
    wirbrauchen = 1;
    return plOK;
}
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: module
 * p[2]: offset
 */
plerrcode proc_VMEread1(ems_u32* p)
{
    int res;
    ems_u8 help;
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;

    res=dev->read_a32d8(dev, module->address.vme.addr+p[2], &help);
    if (res==1) {
	*outptr++ = (int)help;
	return(plOK);
    } else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_VMEread1(ems_u32* p)
{
    return test_vmeparm(p, 2);
}

char name_proc_VMEread1[]="VMEread1";
int ver_proc_VMEread1=1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: module
 * p[2]: offset
 */
plerrcode proc_VMEread2(ems_u32* p)
{
    int res;
    ems_u16 help;
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;

    res=dev->read_a32d16(dev, module->address.vme.addr+p[2], &help);
    if (res==2) {
	*outptr++ = (int)help;
	return(plOK);
    } else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_VMEread2(ems_u32* p)
{
    return test_vmeparm(p, 2);
}

char name_proc_VMEread2[]="VMEread2";
int ver_proc_VMEread2=1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: module
 * p[2]: offset
 */
plerrcode proc_VME24read1(ems_u32* p)
{
    int res;
    ems_u8 help;
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;

    res=dev->read_a24d8(dev, module->address.vme.addr+p[2], &help);
    if (res==1) {
	*outptr++ = (int)help;
	return(plOK);
    } else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_VME24read1(ems_u32* p)
{
    return test_vmeparm(p, 2);
}

char name_proc_VME24read1[]="VME24read1";
int ver_proc_VME24read1=1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: module
 * p[2]: offset
 */
plerrcode proc_VME24read2(ems_u32* p)
{
    int res;
    ems_u16 help;
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;

    res=dev->read_a24d16(dev, module->address.vme.addr+p[2], &help);
    if (res==2) {
	*outptr++ = (int)help;
	return(plOK);
    } else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_VME24read2(ems_u32* p)
{
    return test_vmeparm(p, 2);
}

char name_proc_VME24read2[]="VME24read2";
int ver_proc_VME24read2=1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module
 * p[2]: offset
 * p[3]: data
 */
plerrcode proc_VMEwrite1(ems_u32* p)
{
    int res;
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;

    res=dev->write_a32d8(dev, module->address.vme.addr+p[2], (ems_u8)p[3]);
    if (res==1)
	return(plOK);
    else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_VMEwrite1(ems_u32* p)
{
    return test_vmeparm(p, 3);
}

char name_proc_VMEwrite1[]="VMEwrite1";
int ver_proc_VMEwrite1=1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module
 * p[2]: offset
 * p[3]: data
 */
plerrcode proc_VMEwrite2(ems_u32* p)
{
    int res;
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;

    res=dev->write_a32d16(dev, module->address.vme.addr+p[2], (ems_u16)p[3]);
    if (res==2)
	return(plOK);
    else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_VMEwrite2(ems_u32* p)
{
    return test_vmeparm(p, 3);
}

char name_proc_VMEwrite2[]="VMEwrite2";
int ver_proc_VMEwrite2=1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module
 * p[2]: offset
 * p[3]: data
 */
plerrcode proc_VME24write1(ems_u32* p)
{
    int res;
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;

    res=dev->write_a24d16(dev, module->address.vme.addr+p[2], (ems_u8)p[3]);
    if (res==1)
	return(plOK);
    else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_VME24write1(ems_u32* p)
{
    return test_vmeparm(p, 3);
}

char name_proc_VME24write1[]="VME24write1";
int ver_proc_VME24write1=1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module
 * p[2]: offset
 * p[3]: data
 */
plerrcode proc_VME24write2(ems_u32* p)
{
    int res;
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;

    res=dev->write_a24d16(dev, module->address.vme.addr+p[2], (ems_u16)p[3]);
    if (res==2)
	return(plOK);
    else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_VME24write2(ems_u32* p)
{
    return test_vmeparm(p, 3);
}

char name_proc_VME24write2[]="VME24write2";
int ver_proc_VME24write2=1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: module
 * p[2]: offset
 */
plerrcode proc_VMEread4(ems_u32* p)
{
    int res;
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;

    res=dev->read_a32d32(dev, module->address.vme.addr+p[2], outptr);
    if (res==4) {
	outptr++;
	return(plOK);
    } else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_VMEread4(ems_u32* p)
{
    return test_vmeparm(p, 2);
}

char name_proc_VMEread4[]="VMEread4";
int ver_proc_VMEread4=1;
/*****************************************************************************/
/*
 * p[0]: argcount==2
 * p[1]: module
 * p[2]: offset
 */
plerrcode proc_VME24read4(ems_u32* p)
{
    int res;
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;

    res=dev->read_a24d32(dev, module->address.vme.addr+p[2], outptr);
    if (res==4) {
	outptr++;
	return(plOK);
    } else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_VME24read4(ems_u32* p)
{
    return test_vmeparm(p, 2);
}

char name_proc_VME24read4[]="VME24read4";
int ver_proc_VME24read4=1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module
 * p[2]: offset
 * p[3]: data
 */
plerrcode proc_VMEwrite4(ems_u32* p)
{
    int res;
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;

    res=dev->write_a32d32(dev, module->address.vme.addr+p[2], p[3]);
    if (res==4)
	return(plOK);
    else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_VMEwrite4(ems_u32* p)
{
    return test_vmeparm(p, 3);
}

char name_proc_VMEwrite4[]="VMEwrite4";
int ver_proc_VMEwrite4=1;
/*****************************************************************************/
/*
 * p[0]: argcount==3
 * p[1]: module
 * p[2]: offset
 * p[3]: data
 */
plerrcode proc_VME24write4(ems_u32* p)
{
    int res;
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;

    res=dev->write_a24d32(dev, module->address.vme.addr+p[2], p[3]);
    if (res==4)
	return(plOK);
    else {
        *outptr++ = res;
	return(plErr_System);
    }
}

plerrcode test_proc_VME24write4(ems_u32* p)
{
    return test_vmeparm(p, 3);
}

char name_proc_VME24write4[]="VME24write4";
int ver_proc_VME24write4=1;
/*****************************************************************************/
/*
 * p[0]: argcount==5
 * p[1]: module
 * p[2]: offset
 * p[3]: address modifier
 * p[4]: datasize
 * p[5]: number of words
 */
plerrcode proc_VMEread(ems_u32* p)
{
    int res;
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;

    res=dev->read(dev, module->address.vme.addr+p[2], p[3], outptr,
            p[4]*p[5], p[4], (ems_u32**)&outptr);
    if (res>=0) {
        return plOK;
    } else {
        *outptr++=-res;
        return plErr_System;
    }
}

plerrcode test_proc_VMEread(ems_u32* p)
{
    if (p[0]!=5)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0))
        return plErr_ArgRange;
    wirbrauchen=(p[4]*p[5]+3)/4;
    if (!wirbrauchen)
        wirbrauchen=1;
    return plOK;
}

char name_proc_VMEread[]="VMEread";
int ver_proc_VMEread=1;
/*****************************************************************************/
#if 0
/*
 * p[0]: argcount==5
 * p[1]: module
 * p[2]: offset
 * p[3]: address modifier
 * p[4]: datasize
 * p[5]: number of words
 */
plerrcode proc_VMEread_pipe(ems_u32* p)
{
    int res;
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;

    res=dev->read_pipe(dev, module->address.vme.addr+p[2], p[3], outptr,
            p[4]*p[5], p[4], (ems_u32**)&outptr);
    if (res>=0) {
        return plOK;
    } else {
        *outptr++=-res;
        return plErr_System;
    }
}

plerrcode test_proc_VMEread_pipe(ems_u32* p)
{
    if (p[0]!=5)
        return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0))
        return plErr_ArgRange;
    wirbrauchen=(p[4]*p[5]+3)/4;
    if (!wirbrauchen)
        wirbrauchen=1;
    return plOK;
}

char name_proc_VMEread_pipe[]="VMEread_pipe";
int ver_proc_VMEread_pipe=1;
#endif
/*****************************************************************************/
/*
 * p[0]: argcount==4
 * p[1]: module
 * p[2]: offset
 * p[3]: datasize
 * p[4]: num
 */
plerrcode proc_VME24read(ems_u32* p)
{
    int res;
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;

    res=dev->read_a24(dev, module->address.vme.addr+p[2], outptr,
            p[3]*p[4], p[3], (ems_u32**)&outptr);
    if (res>=0) {
        return plOK;
    } else {
        *outptr++=-res;
        return plErr_System;
    }
}

plerrcode test_proc_VME24read(ems_u32* p)
{
    if (p[0]!=4) return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0)) return plErr_ArgRange;
    wirbrauchen=(p[3]*p[4]+3)/4;
    if (!wirbrauchen) wirbrauchen=1;
    return plOK;
}

char name_proc_VME24read[]="VME24read";
int ver_proc_VME24read=1;
/*****************************************************************************/
/*
 * p[0]: argcount==5+num_data
 * p[1]: module
 * p[2]: offset
 * p[3]: address modifier
 * p[4]: datasize
 * p[5]: num
 * p[6...]: data
 */
plerrcode proc_VMEwrite(ems_u32* p)
{
    int res;
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;

    res=dev->write(dev, module->address.vme.addr+p[2], p[3], p+6,
            p[4]*p[5], p[4]);
    if (res>=0) {
        return plOK;
    } else {
        *outptr++=-res;
        return plErr_System;
    }
}

plerrcode test_proc_VMEwrite(ems_u32* p)
{
    if (p[0]<5) return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0)) return plErr_ArgRange;
    if (p[0]!=5+(p[4]*p[5]+3)/4) return plErr_ArgNum;
    wirbrauchen = 1;
    return plOK;
}

char name_proc_VMEwrite[]="VMEwrite";
int ver_proc_VMEwrite=1;
/*****************************************************************************/
/*
 * p[0]: argcount==4
 * p[1]: module
 * p[2]: offset
 * p[3]: datasize
 * p[4]: num
 * p[5]: data
 */
plerrcode proc_VME24write(ems_u32* p)
{
    int res;
    ml_entry* module=ModulEnt(p[1]);
    struct vme_dev* dev=module->address.vme.dev;

    res=dev->write_a24(dev, module->address.vme.addr+p[2], p+5, p[3]*p[4], p[3]);
    if (res>=0) {
        return plOK;
    } else {
        *outptr++=-res;
        return plErr_System;
    }
}

plerrcode test_proc_VME24write(ems_u32* p)
{
    if (p[0]<4) return plErr_ArgNum;
    if (!valid_module(p[1], modul_vme, 0)) return plErr_ArgRange;
    if (p[0]!=4+(p[3]*p[4]+3)/4) return plErr_ArgNum;
    wirbrauchen = 1;
    return plOK;
}

char name_proc_VME24write[]="VME24write";
int ver_proc_VME24write=1;
/*****************************************************************************/
/*****************************************************************************/
