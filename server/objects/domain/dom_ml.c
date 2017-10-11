/*
 * dom_ml.c
 * created 15.01.93
 * 22.Jan.2001 PW: multicrate support
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: dom_ml.c,v 1.37 2015/04/06 21:35:01 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <modultypes.h>
#include <errorcodes.h>
#include <rcs_ids.h>
#include <xdrstring.h>
#include "dom_ml.h"
#include "dommlobj.h"
#ifdef TEST_ML
#include "test_modul_id.h"
#endif
#ifdef READOUT_CC
#include "../pi/readout.h"
#endif /* PI_READOUT */
#include "../is/is.h"

#include "../../lowlevel/devices.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

RCS_REGISTER(cvsid, "objects/domain")

/*****************************************************************************/

Modlist *modullist; /* changed from modlist (for check) */
extern ems_u32* outptr;
extern int *memberlist;

/*****************************************************************************/

errcode dom_ml_init()
{
    modullist=0;
    return(OK);
}

/*****************************************************************************/
static void
free_modullist(void)
{
    int i;
/*
 *     if an error occures during creation of the modullist we have to delete
 *     an incomplete modullist. Thus we require require than all data fields
 *     used here are either valid or have an safe default value (==NULL
 *     for pointers)
 */
    if (!modullist)
        return;
    for (i=0; i<modullist->modnum; i++) {

        switch (modullist->entry[i].modulclass) {
        case modul_generic:
            if (modullist->entry[i].address.generic.data)
                free(modullist->entry[i].address.generic.data);
            break;
        case modul_ip:
            free(modullist->entry[i].address.ip.address);
            free(modullist->entry[i].address.ip.protocol);
            break;
        }

        if (modullist->entry[i].property_data)
            free(modullist->entry[i].property_data);
        if (modullist->entry[i].private_data)
            free(modullist->entry[i].private_data);
    }
    free(modullist);
    modullist=(Modlist*)0;
}
/*****************************************************************************/

errcode dom_ml_done()
{
    T(dom_ml_done)
    free_modullist();
    return(OK);
}

/*****************************************************************************/
static errcode
downloadmodullist2(ems_u32* p, unsigned int num)
{
    ems_u32* ptr;
    int i;
    errcode res;

    T(downloadmodullist2)
    D(D_REQ, printf("DownloadDomain: Modullist, new style\n");)
    D(D_REQ, printf("  %d Eintraege:\n", p[1]);)
    D(D_REQ, {int i; for (i=0; i<num; i++) printf("%d, ", p[i]); printf("\n");})

/*
 *  we do not know how the compiler will align the members of Modlist
 *  therefore we allocate space for p[1]+1 ml_entry structs
 *  the first entry has enough space for the modnum
 *  and we use calloc in order to initialize all elements with a save walue
 */
    if ((modullist=(Modlist*)calloc(p[1]+1, sizeof(ml_entry)))==0) {
        return(Err_NoMem);
    }
    modullist->modnum=p[1];

    ptr=p+2;
    for (i=0; i<p[1]; i++) {
        Modulclass modulclass;
        if (ptr+1>p+num) {
            res=Err_ArgNum;
            goto error;
        }
        modullist->entry[i].modultype=*ptr++;
        modulclass=*ptr++;
        modullist->entry[i].modulclass=modulclass;
        switch (modulclass) {
        case modul_none:
            break;
#if 0
        case modul_unspec:
            if (ptr+1>p+num) {res=Err_ArgNum; goto error;}
            modullist->entry[i].address.unspec.addr=*ptr++;
            break;
#endif
        case modul_generic:
            if (ptr+1>p+num) {res=Err_ArgNum; goto error;}
            {
                int size=*ptr++;
                if (ptr+size>p+num) {res=Err_ArgNum; goto error;}
                modullist->entry[i].address.generic.length=size;
                if (size) {
                    int j;
                    modullist->entry[i].address.generic.data=(int*)malloc(size*sizeof(int));
                    if (!modullist->entry[i].address.generic.data) {
                        res=Err_System;
                        goto error;
                    }
                    for (j=0; j<size; j++) {
                        modullist->entry[i].address.generic.data[j]=*ptr++;
                    }
                } else {
                    modullist->entry[i].address.generic.data=0;
                }
            }
            break;
        case modul_camac:
            if (ptr+2>p+num) {res=Err_ArgNum; goto error;}
            modullist->entry[i].address.camac.crate=*ptr++;
            modullist->entry[i].address.camac.slot=*ptr++;
            res=find_device(modul_camac,
                    modullist->entry[i].address.camac.crate,
                    (struct generic_dev**)&modullist->entry[i].address.camac.dev);
            if (res!=OK) {
                printf("downloadmodullist2: modul #%d: CAMAC Crate %d is not valid\n",
                        i, modullist->entry[i].address.camac.crate);
                goto error;
            }
            break;
        case modul_fastbus:
            if (ptr+2>p+num) {res=Err_ArgNum; goto error;}
            modullist->entry[i].address.fastbus.crate=*ptr++;
            modullist->entry[i].address.fastbus.pa=*ptr++;
            res=find_device(modul_fastbus,
                    modullist->entry[i].address.fastbus.crate,
                    (struct generic_dev**)&modullist->entry[i].address.fastbus.dev);
            if (res!=OK) {
                printf("downloadmodullist2: modul #%d: FASTBUS Crate %d is not valid\n",
                        i, modullist->entry[i].address.fastbus.crate);
                goto error;
            }
            break;
        case modul_vme:
            if (ptr+2>p+num) {res=Err_ArgNum; goto error;}
            modullist->entry[i].address.vme.crate=*ptr++;
            modullist->entry[i].address.vme.addr=*ptr++;
            res=find_device(modul_vme,
                    modullist->entry[i].address.vme.crate,
                    (struct generic_dev**)&modullist->entry[i].address.vme.dev);
            if (res!=OK) {
                printf("downloadmodullist2: modul #%d: VME Crate %d is not valid\n",
                        i, modullist->entry[i].address.vme.crate);
                goto error;
            }
            break;
        case modul_lvd:
            if (ptr+2>p+num) {res=Err_ArgNum; goto error;}
            modullist->entry[i].address.lvd.crate=*ptr++;
            modullist->entry[i].address.lvd.mod=*ptr++;
            res=find_device(modul_lvd,
                    modullist->entry[i].address.lvd.crate,
                    (struct generic_dev**)&modullist->entry[i].address.lvd.dev);
            if (res!=OK) {
                printf("downloadmodullist2: modul #%d: LVD Crate %d is not valid\n",
                        i, modullist->entry[i].address.lvd.crate);
                goto error;
            }
            printf("Modullist: LVD module 0x%x addr 0x%x crate %d idx %d\n",
                modullist->entry[i].modultype,
                modullist->entry[i].address.lvd.mod,
                modullist->entry[i].address.lvd.crate,
                ((struct generic_dev*)(modullist->entry[i].address.lvd.dev))->generic.dev_idx
                );
            break;
        case modul_can:
            if (ptr+2>p+num) {res=Err_ArgNum; goto error;}
            modullist->entry[i].address.can.bus=*ptr++;
            modullist->entry[i].address.can.id=*ptr++;
            res=find_device(modul_can,
                    modullist->entry[i].address.can.bus,
                    (struct generic_dev**)&modullist->entry[i].address.can.dev);
            if (res!=OK) {
                printf("downloadmodullist2: modul #%d: CANbus %d is not valid\n",
                        i, modullist->entry[i].address.can.bus);
                goto error;
            }
            break;
        case modul_ip:
            /*
             * module_ip is for modules like SIS3316 with direct IP (ethernet)
             * connection
             * arguments are:
             *      string address (addr:port)
             *      string protocol (udp or tcp)
            */
            if (ptr+1>p+num || ptr+xdrstrlen(ptr)>p+num) {
                res=Err_ArgNum;
                goto error;
            }
            ptr=xdrstrcdup(&modullist->entry[i].address.ip.address, ptr);
            if (!ptr) {
                res=Err_NoMem;
                goto error;
            }
            if (ptr+1>p+num || ptr+xdrstrlen(ptr)>p+num) {
                res=Err_ArgNum;
                goto error;
            }
            ptr=xdrstrcdup(&modullist->entry[i].address.ip.protocol, ptr);
            if (!ptr) {
                res=Err_NoMem;
                goto error;
            }
            break;
        default:
            printf("downloadmodullist2: modul #%d: modul_class=%d\n", i, modulclass);
#ifdef DEFAULT_MODULE_CLASS
            modullist->entry[i].modulclass=modul_none;
#else
            free(modullist);
            modullist=0;
            return Err_ArgRange;
#endif
        }
    }

    return OK;

error:
    free_modullist();
    return res;
}
/*****************************************************************************/
#ifdef DEFAULT_MODULE_CLASS
static errcode
downloadmodullist1(ems_u32* p, unsigned int num)
{
    int size, i;
#ifdef TEST_ML
    int ok;
#endif

    D(D_REQ, printf("DownloadDomain: Modullist, old style\n");)
    if ((num<2) || (num!=2*p[1]+2))
        return(Err_ArgNum);
    D(D_REQ, printf("  %d Eintraege:\n", p[1]);)
    D(D_REQ, for (i=0; i<p[1]; i++)
        printf("  Adresse %d, Modul 0x%08x\n", p[2*i+2], p[2*i+3]);)
#ifdef TEST_ML
    ok=1;
    for (i=0; i<p[1]; i++) {
        if (test_modul_id(p[2+2*i], p[3+2*i])!=0) {
            ok=0;
            *outptr++=p[2+2*i];
        }
    }
    if (!ok)
        return(Err_BadModTyp);
#endif
    size=p[1]*sizeof(ml_entry)+2*sizeof(int);
    if ((modullist=(Modlist*)malloc(size))==0) {
        modullist=0;
        return(Err_NoMem);
    }
    modullist->modnum=p[1];
    for (i=0; i<p[1]; i++) {
        modullist->entry[i].modulclass=modul_unspec;
        modullist->entry[i].address.unspec.addr=p[2*i+2];
        modullist->entry[i].modultype=p[2*i+3];
    }
    return OK;
}
#endif
/*****************************************************************************/
/*
downloadmodullist
p[0] : Domain-ID (==0; id=1 calls downloadmodullist2)
       (Domain-ID is missused as version number)
p[1] : Anzahl der Eintraege
p[2] : daten
...
*/
static errcode
downloadmodullist(ems_u32* p, unsigned int num)
{
    errcode res;

    T(downloadmodullist)
    if (modullist)
        return(Err_DomainDef);
#ifdef READOUT_CC
    if (readout_active)
        return Err_PIActive;
#endif
    if (num<2)
        return(Err_ArgNum);
    if ((p[1]==0) || (p[0]>0)) /* empty modullist or new type */
        res=downloadmodullist2(p, num);
    else
#ifdef DEFAULT_MODULE_CLASS
#error default_module_class no longer allowed
        res=downloadmodullist1(p, num);
#else
        res=Err_IllDomain;
#endif

#ifdef OBJ_IS
    if (res==OK) {
        /* create dummy memberlist for IS 0 */
        int i;
        if (!is_list[0]) {
            is_list[0]=calloc(1, sizeof(struct IS));
            if (!is_list[0]) {
                printf("cannot allocate IS 0: %s\n", strerror(errno));
                return Err_NoMem;
            }
        }
        is_list[0]->members=realloc(is_list[0]->members,
                (modullist->modnum+1)*sizeof(int));
        if (!is_list[0]->members) {
            printf("cannot allocate memberlist for IS 0: %s\n", strerror(errno));
            return Err_NoMem;
        }
        for (i=0; i<modullist->modnum; i++)
            is_list[0]->members[i+1]=i;
        is_list[0]->members[0]=modullist->modnum;
    } else {
        /* delete dummy memberlist for IS 0 */
        if (is_list[0] && is_list[0]->members) {
            free(is_list[0]->members);
            is_list[0]->members=0;
        }
    }
#endif /* OBJ_IS */

    return OK;
}
/*****************************************************************************/
/*
uploadmodullist
p[0] : Domain-ID (0||1; Domain-ID is missused as version number)
*/
/* response format similar to downloadmodullist2 */
static errcode
uploadmodullist(ems_u32* p, unsigned int num)
{
    int i;

    T(uploadmodullist)
    D(D_REQ, printf("UploadDomain: Modullist\n");)

    if (num!=1)
        return Err_ArgNum;
    if (modullist==(Modlist*)0)
        return Err_NoDomain;

    if (p[0]==0) {
#ifdef DEFAULT_MODULE_CLASS
        for (i=0; i<modullist->modnum; i++) {
            if (modullist->entry[i].modulclass!=modul_unspec)
                return IllDomain;
        }
        *outptr++=modullist->modnum;
        for (i=0; i<modullist->modnum; i++) {
            *outptr++=modullist->entry[i].modultype;
            *outptr++=modullist->entry[i].address.unspec.addr;
        }
        return OK;
#else
        return Err_IllDomain;
#endif
    }

    *outptr++=modullist->modnum;
    for (i=0; i<modullist->modnum; i++) {
        *outptr++=modullist->entry[i].modultype;
        *outptr++=modullist->entry[i].modulclass;
        switch (modullist->entry[i].modulclass) {
        case modul_none:
            break;
#ifdef DEFAULT_MODULE_CLASS
        case modul_unspec:
            *outptr++=modullist->entry[i].address.unspec.addr;
            break;
#endif
        case modul_generic: {
            int j;
            int size=modullist->entry[i].address.generic.length;
            *outptr++=size;
            for (j=0; j<size; j++)
                *outptr++=modullist->entry[i].address.generic.data[j];
            }
            break;
        case modul_camac:
            *outptr++=modullist->entry[i].address.camac.crate;
            *outptr++=modullist->entry[i].address.camac.slot;
            break;
        case modul_fastbus:
            *outptr++=modullist->entry[i].address.fastbus.crate;
            *outptr++=modullist->entry[i].address.fastbus.pa;
            break;
        case modul_vme:
            *outptr++=modullist->entry[i].address.vme.crate;
            *outptr++=modullist->entry[i].address.vme.addr;
            break;
        case modul_lvd:
            *outptr++=modullist->entry[i].address.lvd.crate;
            *outptr++=modullist->entry[i].address.lvd.mod;
            break;
        case modul_ip:
            outptr=outstring(outptr, modullist->entry[i].address.ip.address);
            outptr=outstring(outptr, modullist->entry[i].address.ip.protocol);
            break;
        }
    }
    return OK;
}
/*****************************************************************************/
/*
deletemodullist
p[0] : Domain-ID (belanglos)
*/
static errcode
    deletemodullist(ems_u32* p, unsigned int num)
{
    int is;

    T(deletemodullist)
    D(D_REQ, printf("DeleteDomain: Modullist\n");)

    if (num!=1)
        return Err_ArgNum;
    if (modullist==0)
        return Err_NoDomain;

#ifdef OBJ_IS
    /* check whether modullist is in use by ISs */
    for (is=1; is<MAX_IS; is++) {
        if (is_list[is] && is_list[is]->members && is_list[is]->members[0]) {
            return Err_ObjNonDel;
        }
    }
    /* delete dummy memberlist for IS 0 */
    if (is_list[0] && is_list[0]->members) {
        free(is_list[0]->members);
        is_list[0]->members=0;
    }
#endif /* OBJ_IS */

    free_modullist();
    return OK;
}
/*****************************************************************************/
plerrcode
dump_modent(ml_entry *entry, int verbose)
{
    printf("%7s %08x", Modulclass_names[entry->modulclass], entry->modultype);
    switch (entry->modulclass) {
    case modul_camac:
        printf(" [%d %d]",
            entry->address.camac.crate, entry->address.camac.slot);
        break;
    case modul_fastbus:
        printf(" [%d %d]",
            entry->address.fastbus.crate, entry->address.fastbus.pa);
        break;
    case modul_vme:
        printf(" [%d %08x]",
            entry->address.vme.crate, entry->address.vme.addr);
        break;
    case modul_lvd:
        printf(" [%d %d]",
            entry->address.lvd.crate, entry->address.lvd.mod);
        break;
    case modul_can:
        printf(" [%d %d]", entry->address.can.bus, entry->address.can.id);
        break;
    case modul_ip:
        printf(" [%s %s]",
            entry->address.ip.address, entry->address.ip.protocol);
        break;
    }

    if (verbose) {
        int i;
        printf(" priv. data: %p, size: %d bytes\n", entry->private_data,
                entry->private_length);
        printf(" %d word%s of properties", entry->property_length,
                entry->property_length==1?"":"s");
        if (entry->property_length && entry->property_data) {
            printf(":");
            for (i=0; i<entry->property_length; i++)
                printf(" %08x", entry->property_data[i]);
        }
    }
    printf("\n");

    return plOK;
}
/*****************************************************************************/
plerrcode
dump_modulelist(void)
{
    int i;

    if (!modullist)
        return plErr_NoDomain;

    printf("dump module list: %d entries\n", modullist->modnum);
    for (i=0; i<modullist->modnum; i++) {
        ml_entry *entry=modullist->entry+i;
        printf(" %2d: ", i);
        dump_modent(entry, 1);
    }

    return plOK;
}
/*****************************************************************************/
plerrcode
dump_memberlist(void)
{
    int i, num;

    if (!memberlist)
        return plErr_NoISModulList;
    if (!modullist)
        return plErr_NoDomain;

    num=memberlist[0];
    printf("dump member list: %d entries\n", num);
    for (i=1; i<=num; i++) {
        ml_entry *entry=modullist->entry+memberlist[i];
        printf(" %2d (%2d): ", i, memberlist[i]);
        dump_modent(entry, 0);
    }

    return plOK;
}
/*****************************************************************************/
int
valid_module(unsigned idx, Modulclass accepted_class)
{
    Modulclass class;

    if (!modullist)
        return 0;
    if (memberlist) {
        if (idx==0 || idx>memberlist[0])
            return 0;
        idx=memberlist[idx];
    }
    if (idx>=modullist->modnum)
        return 0;
    class=modullist->entry[idx].modulclass;
    if (class!=accepted_class)
            return 0;
    return 1;
}
/*****************************************************************************/
/*static domobj *lookup_dom_ml(int* id, int idlen, int* remlen)*/
static objectcommon*
lookup_dom_ml(ems_u32* id, unsigned int idlen,
    unsigned int* remlen)
{
    T(lookup_dom_ml)
    if (idlen>0) {
        if ((id[0]==0) || (id[0]==1)) {
            *remlen=idlen;
            return (objectcommon*)&dom_ml_obj;
    } else {
        return 0;
    }
  } else {
        *remlen=0;
        return (objectcommon*)&dom_ml_obj;
    }
}

static ems_u32*
dir_dom_ml(ems_u32* ptr)
{
    T(dir_dom_ml)
    if (modullist)
        *ptr++=0;
    return ptr;
}

domobj dom_ml_obj={
    {
        dom_ml_init,
        dom_ml_done,
        lookup_dom_ml,
        dir_dom_ml
    },
    downloadmodullist,
    uploadmodullist,
    deletemodullist
};
/*****************************************************************************/
/*****************************************************************************/
