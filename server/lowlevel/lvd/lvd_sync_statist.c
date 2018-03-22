/*
 * lowlevel/lvd/sync/lvd_sync_statist.c
 * created 2007-02-06
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: lvd_sync_statist.c,v 1.17 2016/05/12 20:37:40 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <emsctypes.h>
#include <modultypes.h>
#include <modulnames.h>
#include <rcs_ids.h>
#include "../../commu/commu.h"
#include "../../trigger/trigger.h"
#include <lvdbus.h>
#include "lvd_sync_statist.h"

struct lvd_i_distribution {
    ems_u64 num;
    ems_u64 sum;
    ems_u64 qsum;
    ems_u64 lastval;
};
struct lvd_f_distribution {
    double num;
    double sum;
    double qsum;
    double lastval;
};

struct lvd_trigger_stats {
    ems_u64 accepted;
    ems_u64 rejected;
    ems_u64 delayed;
};

struct lvd_sync_statistic {
    ems_u32 flags; /* 0x1: ignore errors */
    ems_u32 mtype[16];
    struct lvd_trigger_stats num_all;
    struct lvd_trigger_stats num[0x10000];

    struct lvd_i_distribution tdt_all; /* in units of 100 ns */
    //struct lvd_i_distribution tdt[0x10000];

    struct lvd_i_distribution ldt[16][8]; /* in units of 100 ns */

    struct lvd_f_distribution cross[16][16];
    struct lvd_f_distribution accepted[16];
    struct lvd_f_distribution rejected[16];
    struct lvd_f_distribution delayed[16];
    struct lvd_f_distribution all[16];
    struct lvd_f_distribution really_all;
};

#define UUL(x) ((unsigned long long)(x))

int parseerrcount=0;

RCS_REGISTER(cvsid, "lowlevel/lvd")

/*****************************************************************************/
static void
clear_i_distribution(struct lvd_i_distribution *dist)
{
    dist->num=dist->sum=dist->qsum=0;
    dist->lastval=0;
}
static void
clear_f_distribution(struct lvd_f_distribution *dist)
{
    dist->num=dist->sum=dist->qsum=0;
    dist->lastval=0;
}
static void
clear_trigger_stats(struct lvd_trigger_stats *stats)
{
    stats->accepted=stats->rejected=stats->delayed=0;
}
static void
clear_lvd_sync_statistic(struct lvd_sync_statistic *statist)
{
    int i, j;

    clear_trigger_stats(&statist->num_all);
    for (i=0; i<0x10000; i++)
        clear_trigger_stats(&statist->num[i]);

    clear_i_distribution(&statist->tdt_all);
    //for (i=0; i<0x10000; i++)
    //    clear_i_distribution(&statist->tdt[i]);

    for (j=0; j<16; j++)
        for (i=0; i<8; i++)
            clear_i_distribution(&statist->ldt[j][i]);

    for (j=0; j<16; j++)
        for (i=0; i<16; i++)
            clear_f_distribution(&statist->cross[j][i]);

    for (i=0; i<16; i++)
        clear_f_distribution(&statist->accepted[i]);
    for (i=0; i<16; i++)
        clear_f_distribution(&statist->rejected[i]);
    for (i=0; i<16; i++)
        clear_f_distribution(&statist->delayed[i]);
    for (i=0; i<16; i++)
        clear_f_distribution(&statist->all[i]);
    clear_f_distribution(&statist->really_all);
}
/*****************************************************************************/
void
lvd_sync_statist_clear(struct lvd_dev *dev)
{
    struct lvd_sync_statistic *statist=
            (struct lvd_sync_statistic*)dev->sync_statist;

    if (!statist)
        return;

    clear_lvd_sync_statistic(statist);
}
/*****************************************************************************/
static void
distr_i_add(struct lvd_i_distribution *distr, ems_u64 val)
{
    distr->num++;
    distr->sum+=val;
    distr->qsum+=val*val;
}
/*****************************************************************************/
static void
distr_f_add(struct lvd_f_distribution *distr, double val)
{
    distr->num++;
    distr->sum+=val;
    distr->qsum+=val*val;
}
/*****************************************************************************/
static void
distr_f_add_rel(struct lvd_f_distribution *distr, double val)
{
    double relval;

    if (distr->lastval>0) {
        relval=val-distr->lastval;
        if (relval>0) {
            distr->num++;
            distr->sum+=relval;
            distr->qsum+=relval*relval;
        }
    }
    distr->lastval=val;
}
/*****************************************************************************/
static int
get_i_distribution(struct lvd_i_distribution *distr,
        double *mean, double *sigma)
{
    double dnum, dsum, dqsum, average;

    if (distr->num) {
        dnum=distr->num;
        dsum=distr->sum;
        dqsum=distr->qsum;
        average=dsum/dnum;
        *sigma=sqrt(dqsum/dnum-average*average);
        *mean=average;
    }
    return distr->num;
}
/*****************************************************************************/
#if 0
static int
get_f_distribution(struct lvd_f_distribution *distr,
        double *mean, double *sigma)
{
    double dnum, dsum, dqsum, average;

    if (distr->num) {
        dnum=distr->num;
        dsum=distr->sum;
        dqsum=distr->qsum;
        average=dsum/dnum;
        *sigma=sqrt(dqsum/dnum-average*average);
        *mean=average;
    }
    return distr->num;
}
#endif
/*****************************************************************************/
static void
dump_i_distribution(struct lvd_i_distribution *distr)
{
    double average=-1, sigma=-1;
    int num;

    if (!(num=get_i_distribution(distr, &average, &sigma))) {
        printf("empty");
        return;
    }
    printf("num=%lld", UUL(distr->num));
    printf(" mean=%f us", average/10.);
    printf(" sigma=%f us", sigma/10.);
}
/*****************************************************************************/
#if 0
static void
dump_f_distribution(struct lvd_f_distribution *distr)
{
    double average=-1, sigma=-1;
    int num;

    if (!(num=get_f_distribution(distr, &average, &sigma))) {
        printf("empty");
        return;
    }
    printf("num=%lld", UUL(distr->num));
    printf(" mean=%f us", average/10.);
    printf(" sigma=%f us", sigma/10.);
}
#endif
/*****************************************************************************/
void
lvd_sync_statist_dump(struct lvd_dev* dev)
{
    struct lvd_sync_statistic *statist=
            (struct lvd_sync_statistic*)dev->sync_statist;
    int i, j, k;

    if (!statist) {
        printf("dump_sync_statist: dev does not collect sync statistics\n");
        return;
    }
 
    printf("sync_statist:\n");
    printf("        accepted : %lld\n", UUL(statist->num_all.accepted));
    printf("        rejected : %lld\n", UUL(statist->num_all.rejected));
    for (i=0, k=0; i<0x10000; i++) {
        if (statist->num[i].accepted||statist->num[i].rejected) {
            printf("  [%04x]accepted : %lld\n", i, UUL(statist->num[i].accepted));
            printf("  [%04x]rejected : %lld\n", i, UUL(statist->num[i].rejected));
            if (++k>=100) {
                printf("  List truncated!\n");
                break;
            }
        }
    }
    printf("  tdt: ");
    dump_i_distribution(&statist->tdt_all);
    printf("\n");
    for (j=0; j<16; j++) {
        for (i=0; i<8; i++) {
            if (statist->ldt[j][i].num) {
                printf("  ldt[%x][%x]: ", j, i);
                dump_i_distribution(&statist->ldt[j][i]);
                printf("\n");
            }
        }
    }
}
/*****************************************************************************/
static int
put_64(ems_u32 *p, ems_u64 v)
{
    *p++=(v>>32)&0xffffffff;
    *p++=v&0xffffffff;
    return 2; /* number of words stored */
}
static int
put_i_distribution(struct lvd_i_distribution *distr, ems_u32 *p)
{
    int r, res=0;
    r=put_64(p, distr->num);
    res+=r; p+=r;
    r=put_64(p, distr->sum);
    res+=r; p+=r;
    r=put_64(p, distr->qsum);
    res+=r;
    return res; /* number of words stored */
}
static int
put_f_distribution(struct lvd_f_distribution *distr, ems_u32 *p)
{
    int r, res=0;
    ems_u64 val;
    r=put_64(p, distr->num);
    res+=r; p+=r;
    val=(ems_u32)(distr->sum+.5);
    r=put_64(p, val);
    res+=r; p+=r;
    val=(ems_u32)(distr->qsum+.5);
    r=put_64(p, val);
    res+=r;
    return res; /* number of words stored */
}
/*****************************************************************************/
/*
 * output:
 * flags (copy of input, for easier decoding)
 * event_nr
 * timestamp (sec/usec)
 * all accepted
 * all rejected
 * tdt: triple of: num(64bit), sum(64bit), qsum(64bit)
 * if (flags&1)
 *   number of following values (==)
 *   16*accepted: triples of ... (see tdt)
 *   16*rejected: triples of ...
 * if (flags&2)
 *   number of following deadtimes per crate (max 128)
 *     triples of addr, port and triple of ...
 * if (flags&4)
 *   number of following deadtimes per trigger bits (max 16)
 *     pairs of trigger id and triple of ...
 * if (flags&8)
 *   number of following deadtimes per trigger pattern (max 65535)
 *     triples of trigger id, accepted(64bit) and rejected(64bit)
 */

plerrcode
lvd_get_sync_statist(struct lvd_dev *dev, ems_u32 *p, int *num,
        int flags, int max)
{
    struct lvd_sync_statistic *statist=
            (struct lvd_sync_statistic*)dev->sync_statist;
    struct timeval now;
    ems_u32 *help, *p0;
    int i, j, k;

    if (!statist) {
        printf("get_sync_statist: dev does not collect sync statistics\n");
        return plErr_BadModTyp;
    }
 
    gettimeofday(&now, 0);

    p0=p;
    *p++=flags;
#if 0 /* more work needed to make it compatible multiple triggers */
    *p++=trigger.eventcnt;
#endif
    *p++=now.tv_sec;
    *p++=now.tv_usec;
    p+=put_64(p, statist->num_all.accepted);
    p+=put_64(p, statist->num_all.rejected);
    p+=put_i_distribution(&statist->tdt_all, p);

    if (flags&1) {
        help=p++;
        for (i=0; i<16; i++)
            p+=put_f_distribution(&statist->accepted[i], p);
        for (i=0; i<16; i++)
            p+=put_f_distribution(&statist->rejected[i], p);
        *help=p-help-1;
    }

    if (flags&2) {
        help=p++;
        for (j=0; j<16; j++) {
            for (i=0; i<8; i++) {
                if (statist->ldt[j][i].num) {
                    *p++=j;
                    *p++=i;
                    p+=put_i_distribution(&statist->ldt[j][i], p);
                }
            }
        }
        *help=p-help-1;
    }

    if (flags&4) {
        help=p++;
        /* not implemented yet */
        *help=p-help-1;
    }


    if (flags&8) {
        help=p++;
        for (i=0, k=0; i<0x10000; i++) {
            if (statist->num[i].accepted||statist->num[i].rejected) {
                *p++=i;
                *p++=statist->num[i].accepted;
                *p++=statist->num[i].rejected;
                if (++k>=max) {
                    break;
                }
            }
        }
        *help=p-help-1;
    }

    *num=p-p0;

    return plOK;
}
/*****************************************************************************/
static void
dump_event(struct lvd_dev *dev, ems_u32* event, int size)
{
    struct lvd_sync_statistic *statist=
            (struct lvd_sync_statistic*)dev->sync_statist;
    ems_u32 d;
    int i;

    d=event[0];
    printf("  %4d %08x (header)\n", 0, d);

    d=event[1];
    printf("  %4d %08x (header)\n", 1, d);

    d=event[2];
    printf("  %4d %08x (header)\n", 2, d);

    for (i=3; i<size; i++) {
        d=event[i];
        int addr=(d>>28)&0xf;

        printf("  %4d %08x ", i, d);
        switch (statist->mtype[addr]) {
        case ZEL_LVD_MSYNCH: {
                if (d&0x04000000) { /* tdt */
                    ems_u32 tdt=d&0xfffff;
                    if (d&0x0bf00000)
                        printf(" tdt=%d (illegal bits set)\n", tdt);
                    else
                        printf(" tdt=%d\n", tdt);
                } else {
                    int trigger=(d>>22)&0xf;
                    int time=d&0xfffff;
                    if (d&0x10000)
                        printf(" rejected trigger 0x%04x at %d\n",
                                trigger, time);
                    else
                        printf(" accepted trigger 0x%04x at %d\n",
                                trigger, time);
                }
            }
            break;
        case ZEL_LVD_OSYNCH: {
                if (d&0x40000) { /* event counter */
                    printf(" event counter: %d\n", d&0xffff);
                } else {         /* ldt */
                    if (d&0x30000) { /* error or timeout */
                        printf(" errorflag\n");
                    } else {
                        int connector=(d>>22)&0x7;
                        int time=d&0xffff;
                        printf(" ldt of addr %x con %d: %d\n",
                                addr, connector, time);
                    }
                }
            }
            break;
        case 0: {
                printf("illegal address %x\n", addr);
            }
            break;
        }
    }
}
/*****************************************************************************/
plerrcode
lvd_sync_statist_autosetup(struct lvd_dev* dev)
{
    struct lvd_sync_statistic *statist;
    int card, addr, masters=0, slaves=0;
    plerrcode pres=plOK;

#if 1
    printf("lvd_sync_statist_autosetup(flags=0x%x)\n", dev->parseflags);
#endif

    parseerrcount=0;
    for (card=0; card<dev->num_acards; card++) {
        struct lvd_acard *acard=dev->acards+card;
        switch (acard->mtype) {
        case ZEL_LVD_MSYNCH:
            masters++;
            break;
        case ZEL_LVD_OSYNCH:
            slaves++;
            break;
        }
    }

    if (masters==0)
        return plOK;


    statist=calloc(1, sizeof(struct lvd_sync_statistic));
    if (!statist) {
        printf("lvd_sync_statist_autosetup: %s\n", strerror(errno));
        return plErr_NoMem;
    }

    dev->sync_statist=statist;

    if (masters!=1) {
        printf("lvd_sync_statist: %d masters found\n", masters);
        pres=plErr_BadModTyp;
    } else {
        printf("lvd_sync_statist: 1 master found\n");
    }
    if (!slaves) {
        printf("lvd_sync_statist: no slaves found\n");
        //pres=plErr_BadModTyp;
    } else {
        printf("lvd_sync_statist: %d slaves found\n", slaves);
    }

    if (pres) {
        free(statist);
        dev->sync_statist=0;
        return pres;
    }

    for (addr=0; addr<16; addr++)
        statist->mtype[addr]=0;
    for (card=0; card<dev->num_acards; card++) {
        struct lvd_acard *acard=dev->acards+card;
        if (acard->addr>0xf) {
            printf("lvd_sync_statist_autosetup: card at illegal address 0x%x\n",
                    acard->addr);
            continue;
        }

        printf("addr %x: %s\n", acard->addr, modulname(acard->mtype, 0));

        switch (acard->mtype) {
        case ZEL_LVD_MSYNCH:
        case ZEL_LVD_OSYNCH:
            statist->mtype[acard->addr]=acard->mtype;
        }
    }

    dev->parsecaps |= lvd_parse_sync;

    return pres;
}
/*****************************************************************************/
static int
check_integrity(struct lvd_dev* dev, ems_u32* event, int size)
{
    struct lvd_sync_statistic *statist=
            (struct lvd_sync_statistic*)dev->sync_statist;
//    const int max=80;
//    char ss[max];
    int i;
    int num_tdt=0;
    int num_rejected=0;
    int num_accepted=0;
    int num_delayed=0;
    int pat_accepted=0;
    int pat_rejected=0;
    int pat_delayed=0;

    for (i=3; i<size; i++) {
        ems_u32 d=event[i];
        int addr=(d>>28)&0xf;

        switch (statist->mtype[addr]) {
#if 0
        case 0:
            if (!(dev->parseflags&lvd_parse_ignore_errors_sync)) {
                snprintf(ss, max, "lvd synch, ev=%u: no sync module has address 0x%x",
                    trigger.eventcnt, addr);
                printf("%s\n", ss);
                send_unsol_text(ss, 0);
            }
            return -1;
#endif
        case ZEL_LVD_MSYNCH:
            if (d&0x04000000) {
                if (d&0x0bf00000) {
#if 0 /* more work needed to make it compatible multiple triggers */
                    if (!(dev->parseflags&lvd_parse_ignore_errors_sync)) {
                        snprintf(ss, max, "lvd synch, ev=%u: illegal tdt 0x%08x",
                            trigger.eventcnt, d);
                        printf("%s\n", ss);
                        send_unsol_text(ss, 0);
                    }
 #endif
                   return -1;
                }
                num_tdt++;
            } else {
                int trig=1<<((d>>22)&0xf);
                switch ((d>>20)&0x3) {
                case 0:
                    num_accepted++;
                    pat_accepted|=trig;
                    break;
                case 1:
                    num_rejected++;
                    pat_rejected|=trig;
                    break;
                case 2:
                    num_delayed++;
                    pat_delayed|=trig;
                    break;
                case 3:
#if 0 /* more work needed to make it compatible multiple triggers */
                    if (!(dev->parseflags&lvd_parse_ignore_errors_sync)) {
                        snprintf(ss, max, "lvd_sync: illegal del/rej in event %d: 0x%08x",
                            trigger.eventcnt, d);
                        printf("%s\n", ss);
                        send_unsol_text(ss, 0);
                    }
#endif
                   return -1;
                }
            }
            break;
        case ZEL_LVD_OSYNCH:
            if (d&0x40000) { /* event counter */
                /* ignored */
            } else {         /* ldt */
                if (d&0x30000) { /* error or timeout */
#if 0 /* more work needed to make it compatible multiple triggers */
                   if (!(dev->parseflags&lvd_parse_ignore_errors_sync)) {
                        snprintf(ss, max, "lvd_sync: errorflag in event %d: 0x%08x",
                            trigger.eventcnt, d);
                        printf("%s\n", ss);
                        send_unsol_text(ss, 0);
                    }
#endif
                    return -1;
                }
            }
            break;
#if 0
        default:
            snprintf(ss, max, "lvd synch, ev=%u: no sync module has address 0x%x",
                trigger.eventcnt, addr);
            printf("%s\n", ss);
            send_unsol_text(ss, 0);
            return -1;
#endif
        }
    }
#if 0
    if (num_tdt>1 || num_accepted!=1 || num_delayed>1) {
        printf("lvd synch, ev=%u: num_tdt=%d num_accepted=%d num_delayed=%d\n",
            trigger.eventcnt, num_tdt, num_accepted, num_delayed);
    }
#endif

    return 0;
}
/*****************************************************************************/
int
lvd_parse_sync_event(struct lvd_dev* dev, ems_u32* event, int size)
{
    struct lvd_sync_statistic *statist=
            (struct lvd_sync_statistic*)dev->sync_statist;
    int accepted_pattern=0;
    int rejected_pattern=0;
    int delayed_pattern=0;
    int rejected_time=-1;
    int i;

    /*eventsize=event[0];*/
    /*eventtime=event[1];*/
    /*eventcnt =event[2];*/

    if (size>=265) {
        /* XXX better parse_sync handling needed */
        //complain("lvd_parse_sync_event: %d or more triggers rejected", size);
        return 0;
    }

    if (check_integrity(dev, event, size)) {
        if (!(dev->parseflags&lvd_parse_ignore_errors_sync)) {
            parseerrcount++;
            if ((parseerrcount&(parseerrcount-1))==0) {
                printf("parseerrcount=%d\n", parseerrcount);
                dump_event(dev, event, size);
            }
            //return 1;
            return 0;
        }
        return 0;
    }

    for (i=3; i<size; i++) {
        ems_u32 d=event[i];
        int addr=(d>>28)&0xf;
        switch (statist->mtype[addr]) {
        case ZEL_LVD_MSYNCH:
            {
                int time=d&0xfffff;
                if (d&0x4000000) { /* tdt */
                    distr_i_add(&statist->tdt_all, time);
                } else {           /* trigger time */
                    int bit=(d>>22)&0xf;
                    int trigger=1<<bit;
                    double ttime;

                    //ttime=make_ttime(dev, time);
                    ttime=0;
                    distr_f_add_rel(&statist->all[bit], ttime);
                    distr_f_add_rel(&statist->really_all, ttime);
                    switch ((d>>20)&0x3) {
                    case 0: /* accepted trigger */
                        distr_f_add_rel(&statist->accepted[bit], ttime);
                        accepted_pattern|=trigger;
                        break;
                    case 1: /* rejected trigger */
                        distr_f_add_rel(&statist->rejected[bit], ttime);
                        rejected_pattern|=trigger;
                        break;
                    case 2: /* delayed trigger */
                        distr_f_add_rel(&statist->delayed[bit], ttime);
                        delayed_pattern|=trigger;
                        break;
                    }

                    if (d&0x100000) { /* rejected */
                        distr_f_add(&statist->rejected[bit], time);
                        if (rejected_time==time) {
                            rejected_pattern|=trigger;
                        } else {
                            if (rejected_time!=-1) {
                                statist->num_all.rejected++;
                                statist->num[rejected_pattern].rejected++;
                            }
                            rejected_pattern=trigger;
                            rejected_time=time;
                        }
                    } else {          /* accepted */
                        distr_f_add(&statist->accepted[bit], time);
                        accepted_pattern|=trigger;
                    }
                }
            }
            break;
        case ZEL_LVD_OSYNCH:
            if (d&0x40000) { /* event counter */
                /* ignored */
            } else {         /* ldt */
                int connector=(d>>22)&0x7;
                int time=d&0xffff;
                struct lvd_i_distribution *ldt=
                        &statist->ldt[addr][connector];
                distr_i_add(ldt, time);
            }
            break;
        }
    }
    statist->num_all.accepted++;
    statist->num[accepted_pattern].accepted++;
    if (rejected_time!=-1) {
        statist->num_all.rejected++;
        statist->num[rejected_pattern].rejected++;
    }

    return 0;
}
/*****************************************************************************/
/*****************************************************************************/
