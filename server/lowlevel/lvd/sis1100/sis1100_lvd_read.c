/*
 * lowlevel/lvd/sis1100/sis1100_lvd_read.c
 * created 2009-Jan-29 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: sis1100_lvd_read.c,v 1.16 2011/04/06 20:30:26 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <rcs_ids.h>
#include "sis1100_lvd.h"
#include "sis1100_lvd_read.h"
#include "../lvd_sync_statist.h"
#include "../../../commu/commu.h"
#include "../../../trigger/trigger.h"

RCS_REGISTER(cvsid, "lowlevel/lvd/sis1100")

/*****************************************************************************/
/*
 * sis1100_lvd_parse_time gets the second header word as input (h1)
 * and calculates the eventtime in ns
 */
static void
sis1100_lvd_parse_time(struct lvd_dev* dev, ems_u32 h1)
{
    double etime, fh1l, fh1h;

    fh1h=((h1>>16)&0xffff)*6400.;
    fh1l=(h1&0xffff)*dev->tdc_range;
    etime=fh1h+fh1l; /* absolute time in ns, but overflow is possible */
    while (etime<dev->ccard.last_ev_time[0])
        etime+=0x10000*6400.;
    dev->ccard.last_ev_time[1]=dev->ccard.last_ev_time[0];
    dev->ccard.last_ev_time[0]=etime;
}
/*****************************************************************************/
static int
sis1100_lvd_parse_header(struct lvd_dev* dev, ems_u32* event, int size)
{
    const int max=80;
    char ss[max];
#if 0
    ems_u32 eventsize, eventtime, eventcnt;
#endif

    if (size<3) {
        snprintf(ss, max, "lvd_parse_header, ev=%u: event too short: %d words",
                trigger.eventcnt, size);
        printf("%s\n", ss);
        send_unsol_text(ss, 0);
        return -1;
    }
#if 0
    eventsize=event[0];
    eventtime=event[1];
    eventcnt =event[2];
#endif

    sis1100_lvd_parse_time(dev, event[1]);

    return 0;
}
/*****************************************************************************/
int
sis1100_lvd_parse_event(struct lvd_dev* dev, ems_u32* event, int size)
{
    int res=0;

    if (event[0]&0x40000000) {
#if 0
        printf("parse_event: fragmented event, not parsed\n");
#endif
        return 0;
    }

    if (dev->parseflags&lvd_parse_head) {
        /* parse header (trigger time and errors) */
        res=sis1100_lvd_parse_header(dev, event, size);
        if (res) {
            printf("sis1100_lvd_parse_header returned %d\n", res);
            goto error;
        }
    }
    if (dev->parseflags&lvd_parse_sync) {
        /* parse event data and collect statistics */
        res=lvd_parse_sync_event(dev, event, size);
        if (res) {
            printf("lvd_parse_sync_event returned %d\n", res);
            goto error;
        }
    }

error:
    return res;
}
/*****************************************************************************/
void
sis1100_lvd_print_statist(struct lvd_dev* dev, struct timeval tv1)
{
    struct lvd_sis1100_info* info=(struct lvd_sis1100_info*)dev->info;
    struct ra_statist *new=&info->ra.statist;
    struct ra_statist *old=&info->ra.old_statist;
    double tdiff;

    tdiff=tv1.tv_sec-info->ra.tv0.tv_sec+(tv1.tv_usec-info->ra.tv0.tv_usec)
            /1000000.;

    printf("lvd_statist(%s):\n",dev->pathname);

    printf("  %.4g events/s %.4g words/s %.4g blocks/s; %.4g s/block\n",
            (new->events-old->events)/tdiff,
            (new->words-old->words)/tdiff,
            (new->blocks-old->blocks)/tdiff,
            tdiff/(new->blocks-old->blocks));

    printf("  %llu eeeeeeee_events, %.4g/s\n",
            (unsigned long long)new->eeeeeeee_events,
            (new->eeeeeeee_events-old->eeeeeeee_events)/tdiff);

    printf("  %llu splitted_events, %.4g/s\n",
            (unsigned long long)new->splitted_events,
            (new->splitted_events-old->splitted_events)/tdiff);

    //printf("  av. event size: %.4g\n",
    //        (double)(new->sum_evsize-old->sum_evsize)/
    //        (double)(new->num_ev-old->num_ev));
    printf("  av. event size: %.4g\n",
            (double)(new->words-old->words)/
            (double)(new->events-old->events));

    printf("  max. event size: %u\n",
            new->max_evsize);

    printf("  %.4g events/block\n",
            (double)(new->events-old->events)/
            (double)(new->blocks-old->blocks));
}
/*****************************************************************************/
#define HI(x) (((x)>>32)&0xffffffffUL)
#define LO(x) ((x)&0xffffffffUL)

int
sis1100_lvd_statist(struct lvd_dev* dev, ems_u32 flags, ems_u32 *ptr)
{
    struct lvd_sis1100_info* info=(struct lvd_sis1100_info*)dev->info;
    struct ra_statist *st=&info->ra.statist;
    ems_u32 *help;

    help=ptr;

    *ptr++=HI(st->blocks);
    *ptr++=LO(st->blocks);

    *ptr++=HI(st->events);
    *ptr++=LO(st->events);

    *ptr++=HI(st->eeeeeeee_events);
    *ptr++=LO(st->eeeeeeee_events);

    *ptr++=HI(st->splitted_events);
    *ptr++=LO(st->splitted_events);

    *ptr++=HI(st->words);
    *ptr++=LO(st->words);

    //*ptr++=HI(st->sum_evsize);
    //*ptr++=LO(st->sum_evsize);

    //*ptr+=HI(st->num_ev);
    //*ptr++=LO(st->num_ev);

    *ptr++=st->max_evsize;

    return ptr-help;
}
/*****************************************************************************/
/*****************************************************************************/
