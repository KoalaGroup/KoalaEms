/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_delay_read.c
 * created before Jul.2003
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_delay_read.c,v 1.7 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <rcs_ids.h>

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

#ifdef DELAYED_READ

#include <debug.h>
#include "sis3100_sfi.h"

ems_u8* sis3100_sfi_delay_buffer;
size_t sis3100_sfi_delay_buffer_size;

int
sis3100_sfi_delay_read(struct fastbus_dev* dev,
        off_t src, ems_u8* dst, size_t size)
{
    struct fastbus_sis3100_sfi_info* info=
            (struct fastbus_sis3100_sfi_info*)dev->info;
    struct delayed_hunk* hunk;
/*
printf("delay_read: src=0x%08llx dst=%p size=%d\n", src, dst, size);
*/
    if (info->hunk_list_size==info->num_hunks) {
        int i;
        struct delayed_hunk* help;
printf("delay_read: allocating hunk %d\n", info->num_hunks);
        help=malloc((info->num_hunks+100)*sizeof(struct delayed_hunk));
        if (!help) {
            printf("sis3100_sfi_delay_read: alloc %d hunks: %s\n",
                    info->num_hunks+100, strerror(errno));
            return -1;
        }
        for (i=info->num_hunks-1; i>=0; i--) help[i]=info->hunk_list[i];
        free(info->hunk_list);
        info->hunk_list=help;
        info->hunk_list_size+=100;
    }
    hunk=info->hunk_list+info->num_hunks;
    hunk->src=src;
    hunk->dst=dst;
    hunk->size=size;
    info->num_hunks++;
    return 0;
}
#endif
/*****************************************************************************/
/*****************************************************************************/
