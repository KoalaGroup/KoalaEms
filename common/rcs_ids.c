/*
 * rcs_ids.c
 * created 2011-03-31 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: rcs_ids.c,v 2.5 2011/04/13 21:24:26 wuestner Exp $";

#include <stdlib.h>
#include <unistd.h>
#include "rcs_ids.h"

RCS_REGISTER(cvsid, "common")

static struct rcs_id *rcs_ids;
static int num_ids, size_ids;

static void init_rcs(void) __attribute__((constructor(1000)));
static void init_rcs(void)
{
    rcs_ids=0;
    num_ids=size_ids=0;
}

void rcs_store(const char *id, const char *path)
{
    if (num_ids==size_ids) {
        size_ids+=16;
        rcs_ids=realloc(rcs_ids, sizeof(struct rcs_id)*size_ids);
        if (!rcs_ids) {
            printf("initialisation of rcs_ids failed after %d IDs\n", num_ids);
            exit(2);
        }
    }
    rcs_ids[num_ids].id=id;
    rcs_ids[num_ids].path=path;
    num_ids++;
}

int rcs_get(const struct rcs_id** ids)
{
    *ids=rcs_ids;
    return num_ids;
}

void rcs_dump(FILE* out)
{
    int i;

    fprintf(out, "RCS-IDs:\n");
    for (i=0; i<num_ids; i++) {
        fprintf(out, "%s\n", rcs_ids[i].path);
        fprintf(out, "  %s\n", rcs_ids[i].id);
    }
}
