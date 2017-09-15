/*
 * dataout/cluster/check_cluster.c
 * created: 21.Oct.2002 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: check_cluster.c,v 1.4 2011/04/06 20:30:21 wuestner Exp $";

#include <clusterformat.h> /* in ems/common */
#include <rcs_ids.h>
#include "cluster.h"
#include "check_cluster.h"

#define swap_int(a) ( ((a) << 24) | \
		      (((a) << 8) & 0x00ff0000) | \
		      (((a) >> 8) & 0x0000ff00) | \
	((unsigned int)(a) >>24) )

#define get_swapped(swapped, item) (swapped?swap_int(item):(item))

RCS_REGISTER(cvsid, "dataout/cluster")

/*****************************************************************************/
static int
check_events(Cluster* cl, ems_u32* p, int size)
{
    return 0;
}
/*****************************************************************************/
static int
check_ved_info1(Cluster* cl, ems_u32* p, int size)
{
    int id, num, swapped=cl->swapped;
    if (size<2) {
        printf("check_cluster: ved_info1: size=%d (<2)\n", size);
        return -1;
    }
    id=get_swapped(swapped, p[0]);
    num=get_swapped(swapped, p[1]);
    p+=2; size-=2;
    if (size!=num) {
        printf("check_cluster: ved_info1: size=%d (%d expected)\n", size, num);
        return -1;
    }
    return 0;
}
/*****************************************************************************/
static int
check_ved_info2(Cluster* cl, ems_u32* p, int size)
{
    printf("check_cluster: ved_info2\n");
    return -1;
}
/*****************************************************************************/
static int
check_ved_info3(Cluster* cl, ems_u32* p, int size)
{
    printf("check_cluster: ved_info3\n");
    return -1;
}
/*****************************************************************************/
static int
check_ved_info(Cluster* cl, ems_u32* p, int size)
{
    int type, res=0;

    if (!size) {
        printf("check_cluster: ved_info: size=0\n");
        return -1;
    }
    type=get_swapped(cl->swapped, p[0]);
    p++; size--;
    switch (type) {
        case 0x80000001:
            res=check_ved_info1(cl, p, size);
            break;
        case 0x80000002:
            res=check_ved_info2(cl, p, size);
            break;
        case 0x80000003:
            res=check_ved_info3(cl, p, size);
            break;
        default:
            printf("check_cluster: ved_info: type=%08x\n", type);
            return -1
    }
    return res;
}
/*****************************************************************************/
static int
check_text(Cluster* cl, ems_u32* p, int size)
{
    return 0;
}
/*****************************************************************************/
static int
check_wendy_setup(Cluster* cl, ems_u32* p, int size)
{
    printf("check_cluster: wendy_setup\n");
    return -1;
}
/*****************************************************************************/
static int
check_file(Cluster* cl, ems_u32* p, int size)
{
    return 0;
}
/*****************************************************************************/
static int
check_no_more_data(Cluster* cl, ems_u32* p, int size)
{
    if (size) {
        printf("check_cluster: no_more_data: size=%d\n", size);
        return -1;
    }
    return 0;
}
/*****************************************************************************/
int check_cluster(Cluster* cl)
{
    ems_u32* p=&cl->data;
    int size, type, swapped, optsize;
    ems_u32 endiantest;
    int res=0;

    if (cl->size<4) {
        printf("check_cluster: size=%d (<4)\n", cl->size);
        return -1;
    }
    endiantest=p[1];
    switch (endiantest) {
        case 0x12345678: swapped=0; break;
        case 0x78563412: swapped=1; break;
        default:
            printf("check_cluster: endian=%08x\n", endiantest);
            return -1;
    }
    if (swapped!=cl->swapped) {
        printf("check_cluster: 'swapped'=%d is wrong; endian=%08x\n",
                cl->swapped, endiantest);
        return -1;
    }
    size=get_swapped(swapped, p[0]);
    if (size!=cl->size) {
        printf("check_cluster: size=%d; cached size=%d\n", size, cl->size);
        return -1;
    }
    type=get_swapped(swapped, p[2]);
    if (type!=cl->type) {
        printf("check_cluster: type=%d; cached type=%d\n", type, cl->type);
        return -1;
    }
    optsize=get_swapped(swapped, p[3]);
    if (optsize!=cl->optsize) {
        printf("check_cluster: optsize=%d; cached optsize=%d\n",
                optsize, cl->optsize);
        return -1;
    }
    p+=4; size-=4;
    if (optsize) {
        ems_u32 sec, usec, optflags;
        if (size<optsize) {
            printf("check_cluster: optsize=%d; rem. size only%d\n",
                    optsize, size);
            return -1;
        }
        optflags=get_swapped(swapped, p[0]);

        if ((optsize!=3) || (optflags!=1)) {
            printf("check_cluster: optsize=%d; optflags=%d\n",
                    optsize, optflags);
            return -1;
        }
        sec=get_swapped(swapped, p[1]);
        usec=get_swapped(swapped, p[2]);
        /* Aug 22 09:06:40 CEST 2002  /  Dec 16 01:53:20 CET 2002 */
        if ((sec<1030000000) || (sec>1040000000)) {
            printf("check_cluster: option/sec out of range: %d\n", sec);
            return -1;
        }
        if (usec>=1000000) {
            printf("check_cluster: option/usec out of range: %d\n", usec);
            return -1;
        }
        p+=optsize; size-=optsize;
    }

    switch (type) {
        case clusterty_events:
            res=check_events(cl, p, size);
            break;
        case clusterty_ved_info:
            res=check_ved_info(cl, p, size);
            break;
        case clusterty_text:
            res=check_(textcl, p, size);
            break;
        case clusterty_wendy_setup
            res=check_wendy_setup(cl, p, size);
            break;
        case clusterty_file:
            res=check_file(cl, p, size);
            break;
        case clusterty_no_more_data:
            res=check_no_more_data(cl, p, size);
            break;
        default:
            printf("check_cluster: type==%08x\n", type);
            return -1;
    }
    return res;
}
/*****************************************************************************/
/*****************************************************************************/
