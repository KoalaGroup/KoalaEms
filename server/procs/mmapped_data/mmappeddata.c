/*
 * procs/mmapped_data/mmappeddata.c
 *
 * created 2011-08-04 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: mmappeddata.c,v 1.1 2011/08/13 19:28:49 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdlib.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../../lowlevel/mmapped_data/mmappeddata.h"
#include "../procs.h"
#include "../procprops.h"

extern ems_u32* outptr;
extern int wirbrauchen;

RCS_REGISTER(cvsid, "procs/mmapped_data")

/*****************************************************************************/
/*
 * mmap_data maps a file and attaches to a semaphore and returns a handle.
 * This handle is needed for the procedure 'mmapped_data'.
 * 
 * p[0]: number of arguments (>1)
 * p[1]: name of mapfile
 */
plerrcode proc_map_data(ems_u32* p)
{
    char *name;
    ems_u32 handle;
    plerrcode pres;

    xdrstrcdup(&name, p+1);
    pres=mmap_data(name, &handle);
    free(name);
    if (pres==plOK)
        *outptr++=handle;
    return pres;
}

plerrcode test_proc_map_data(ems_u32* p)
{
    if (p[0]<1)
        return plErr_ArgNum;
    if (xdrstrlen(p+1)>p[0])
        return plErr_ArgNum;
    wirbrauchen=1;
    return plOK;
}

char name_proc_map_data[]="map_data";
int ver_proc_map_data=1;
/*****************************************************************************/
/*
 * unmap_data releases all resources created by proc_map_data.
 * 
 * p[0]: number of arguments (==1)
 * p[1]: handle (retrieved via 'map_data')
 */
plerrcode proc_unmap_data(ems_u32* p)
{
    return munmap_data(p[1]);
}

plerrcode test_proc_unmap_data(ems_u32* p)
{
    if (p[0]!=1)
        return plErr_ArgNum;
    wirbrauchen=0;
    return plOK;
}

char name_proc_unmap_data[]="unmap_data";
int ver_proc_unmap_data=1;
/*****************************************************************************/
/*
 * p[0]: number of arguments (==3)
 * p[1]: handle (retrieved via 'map_data')
 * p[2]: don't suppress old data
 * p[3]: wait for semaphore
 */
plerrcode proc_read_mapped_data(ems_u32* p)
{
    return read_mmapped_data(&outptr, p[1], p[2], p[3]);
}

plerrcode test_proc_read_mapped_data(ems_u32* p)
{
    if (p[0]!=3)
        return plErr_ArgNum;
    wirbrauchen=-1;
    return plOK;
}

char name_proc_read_mapped_data[]="read_mapped_data";
int ver_proc_read_mapped_data=1;
/*****************************************************************************/
/*****************************************************************************/
