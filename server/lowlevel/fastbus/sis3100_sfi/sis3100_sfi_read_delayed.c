/*
 * lowlevel/fastbus/sis3100_sfi/sis3100_sfi_read_delayed.c
 * created before Jul.2003
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis3100_sfi_read_delayed.c,v 1.10 2011/04/06 20:30:23 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "sis3100_sfi.h"

RCS_REGISTER(cvsid, "lowlevel/fastbus/sis3100_sfi")

extern ems_u8 sis3100_sfi_deadbeef_delayed[];

int
sis3100_sfi_enable_delayed_read(struct generic_dev* dev, int val)
{
    int old;
#ifdef DELAYED_READ
    old=dev->generic.delayed_read_enabled;
    dev->generic.delayed_read_enabled=val;
#else
    old=-1;
#endif
    return old;
}
/*****************************************************************************/
void
sis3100_sfi_reset_delayed_read(struct generic_dev* gdev)
{
#ifdef DELAYED_READ
    struct fastbus_dev* dev=(struct fastbus_dev*)gdev;
    struct fastbus_sis3100_sfi_info* info=
            (struct fastbus_sis3100_sfi_info*)dev->info;

    info->num_hunks=0;
    info->ram_free=0;

#ifdef SIS3100_SFI_DEADBEEF
    {
    int r;
    r=pwrite(info->p_mem, sis3100_sfi_deadbeef_delayed, SIS3100_SFI_BEEF_SIZE,
            info->ram_free);
    if (r!=SIS3100_SFI_BEEF_SIZE) {
        printf("sis3100_sfi_reset_delayed_read: pwrite deadbeef failed: %s\n", strerror(errno));
    }
    }
#endif
#endif
}
/*****************************************************************************/
int
sis3100_sfi_read_delayed(struct generic_dev* gdev)
{
#ifdef DELAYED_READ
    struct fastbus_dev* dev=(struct fastbus_dev*)gdev;
    struct fastbus_sis3100_sfi_info* info=
            (struct fastbus_sis3100_sfi_info*)dev->info;
    struct delayed_hunk* hunks=info->hunk_list;
    int i, res, hunksum;

    if (!info->num_hunks) return 0;
    hunksum=hunks[info->num_hunks-1].src+
         hunks[info->num_hunks-1].size-
         hunks[0].src;

    if (sis3100_sfi_delay_buffer_size<hunksum) {
        printf("buffer_size=%lld, hunksum=%d, buffer=%p\n",
                (unsigned long long)(sis3100_sfi_delay_buffer_size),
                hunksum,
                sis3100_sfi_delay_buffer);
        free(sis3100_sfi_delay_buffer);
        sis3100_sfi_delay_buffer=calloc(hunksum, 1);
        if (!sis3100_sfi_delay_buffer) {
            printf("sis3100_sfi_read_delayed: malloc(%d):%s\n",
                hunksum, strerror(errno));
            return -1;
        }
        sis3100_sfi_delay_buffer_size=hunksum;
    }
/*
printf("read_delayed: pread(%p, %d, %08llx)\n", sis3100_sfi_delay_buffer,
        info->hunksum, info->hunk_list[0].src);
*/
#ifdef SIS3100_SFI_DEADBEEF
    memset(sis3100_sfi_delay_buffer, SIS3100_SFI_BEEF_MAGIC_LOCAL, hunksum);
#endif
    res=pread(info->p_mem, sis3100_sfi_delay_buffer,
            hunksum, info->hunk_list[0].src);
    if (res!=hunksum) {
        printf("sis3100_sfi_read_delayed: pread: size=%d res=%d errno=%s\n",
                hunksum, res, strerror(errno));
        return -1;
    }

    for (i=0; i<info->num_hunks; i++) {
        struct delayed_hunk* hunk=info->hunk_list+i;
/*
printf("read_delayed: memcpy(%p, %p, %d)\n", hunk->dst,
    sis3100_sfi_delay_buffer+hunk->src, hunk->size);
*/
        memcpy(hunk->dst, sis3100_sfi_delay_buffer+hunk->src, hunk->size);
    }

    info->num_hunks=0;
    info->ram_free=0;

#ifdef SIS3100_SFI_DEADBEEF
    {
    int r;
    r=pwrite(info->p_mem, sis3100_sfi_deadbeef_delayed, SIS3100_SFI_BEEF_SIZE,
            info->ram_free);
    if (r!=SIS3100_SFI_BEEF_SIZE) {
        printf("sis3100_sfi_read_delayed: pwrite deadbeef failed: %s\n", strerror(errno));
        return -1;
    }
    }
#endif
#endif

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
