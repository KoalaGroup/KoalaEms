/*
 * dataout/cluster/checksum.c
 * created 16.Aug.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: checksum.c,v 1.5 2011/04/06 20:30:21 wuestner Exp $";

#include <sconf.h>
#include "cluster.h"
#include "do_cluster.h"
#include <emsctypes.h>
#include <rcs_ids.h>
#ifdef HAVE_MD5INIT
#include <md5.h>
#else
#include "../../lowlevel/oscompat/md5.h"
#endif

RCS_REGISTER(cvsid, "dataout/cluster")

void
calculate_checksum(struct Cluster* cluster)
{
    ems_u32* ptr;
    ems_u32 type;
    ems_u32 optsize;
    ems_u32 optflags;
    ems_u32 size;

    ptr=cluster->data+3; /* optsize */
    optsize=*ptr++;
    if (optsize<1) {
        printf("calculate_checksum: optsize=%d\n", optsize);
        return;
    }
    optflags=*ptr++;
    if (!(optflags&ClOPTFL_CHECK)) {
        printf("calculate_checksum: optflags=%d\n", optflags);
        return;
    }
    if (optflags&ClOPTFL_TIME) ptr+=2; /* skip timestamp */
    type=*ptr++;
    size=*ptr++;
    switch (type) {
    case CHECKSUMTYPE_MD4:
        printf("calculate_checksum: MD4 not yet supported\n");
        break;
    case CHECKSUMTYPE_MD5: {
        MD5_CTX context;

        if (size!=4) {
            printf("calculate_checksum: size=%d\n", size);
        }
        MD5Init(&context);
        MD5Update(&context, (unsigned char*)cluster->data, cluster->size*4);
        MD5Final((unsigned char*)ptr, &context);
        }
        break;
    default:
        printf("calculate_checksum: 0x%08x not supported\n", type);
    }
}
