/*
 * lowlevel/lvd/datafilter/datafilter.c
 * created 2010-Feb-03 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: datafilter.c,v 1.4 2016/05/12 20:39:21 wuestner Exp $";

#include <sconf.h>
#include <stdlib.h>
#include <string.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "datafilter.h"

struct filterdefinition {
    const char *name;
    plerrcode (*init)(struct datafilter*);
    lvd_filterproc *filter;
};

static struct filterdefinition definitions[];
static int num_filterdefinitions;

RCS_REGISTER(cvsid, "lowlevel/lvd/datafilter")

/*****************************************************************************/
int
lvd_list_datafilter(ems_u32 *dest)
{
    ems_u32 *help=dest;
    int i;

    *dest++=num_filterdefinitions;
    for (i=0; i<num_filterdefinitions; i++) {
        dest=outstring(dest, definitions[i].name);
    }
    return dest-help;
}
/*****************************************************************************/
void
lvd_free_datafilter(struct datafilter *filter)
{
    if (filter->args!=filter->givenargs)
        free(filter->args);
    free(filter->givenargs);
    free(filter);
}
/*****************************************************************************/
plerrcode
lvd_add_datafilter(struct lvd_dev* dev, const char *name, ems_u32 *args,
    int num, int front)
{
    struct datafilter *filter;
    int idx, i;
    plerrcode pres;

    /* find filter definition */
    idx=0;
    while (idx<num_filterdefinitions && strcmp(name, definitions[idx].name))
        idx++;
    if (idx>=num_filterdefinitions) {
        printf("lvd_add_datafilter: filter \"%s\" not found\n", name);
        return plErr_NoSuchProc;
    }

    /* init filter struct */
    filter=malloc(sizeof(struct datafilter));
    if (!filter)
        return plErr_NoMem;
    filter->next=0;
    filter->definition=definitions+idx;
    filter->filter=definitions[idx].filter;
    filter->dev=dev;
    filter->mask=0;
    filter->args=0; /* to be filled from init procedure */
    filter->num_args=0;
    filter->givenargs=malloc(num*sizeof(ems_u32));
    for (i=0; i<num; i++)
        filter->givenargs[i]=args[i];
    filter->num_givenargs=num;

    /* init filter */
    pres=definitions[idx].init(filter);
    if (pres!=plOK) {
        lvd_free_datafilter(filter);
        goto error;
    }

    /* put filter in filterchain */
    if (front) {
        filter->next=dev->datafilter;
        dev->datafilter=filter;
    } else {
        if (!dev->datafilter) {
            dev->datafilter=filter;
        } else {
            struct datafilter *help=dev->datafilter;
            while (help->next)
                help=help->next;
            help->next=filter;
        }
    }

error:
    return pres;
}
/*****************************************************************************/
int
lvd_get_datafilter(struct lvd_dev* dev, ems_u32 *dest)
{
    ems_u32 *help=dest++;
    struct datafilter *filter=dev->datafilter;
    int n=0;

    while (filter) {
        struct filterdefinition *def=
                (struct filterdefinition*)filter->definition;
        int i;
        n++;
        dest=outstring(dest, def->name);
        *dest++=filter->num_args;
        for (i=0; i<filter->num_args; i++)
            *dest++=filter->args[i];
        filter=filter->next;
    }
    *help=n;
    return dest-help;
}
/*****************************************************************************/
void
lvd_clear_datafilter(struct lvd_dev* dev)
{
    while (dev->datafilter) {
        struct datafilter *help=dev->datafilter;
        dev->datafilter=dev->datafilter->next;
        lvd_free_datafilter(help);
    }
}
/*****************************************************************************/

static struct filterdefinition definitions[]={
#if 0
    {"sync_statist", filter_sync_statist_init, filter_sync_statist_filter},
    {"qdc_sparse", filter_qdc_sparse_init, filter_qdc_sparse_filter},
#endif
    {"add_header", filter_add_header_init, filter_add_header_filter},
};
static int num_filterdefinitions=
        sizeof(definitions)/sizeof(struct filterdefinition);

/*****************************************************************************/
/*****************************************************************************/
