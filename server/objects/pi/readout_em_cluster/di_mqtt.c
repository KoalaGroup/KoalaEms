/*
 * readout_em_cluster/di_mqtt.c
 * created 2014-09-05 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: di_mqtt.c,v 1.6 2017/10/27 21:10:48 wuestner Exp $";

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sconf.h>
#include <debug.h>
#include <unsoltypes.h>
#include <rcs_ids.h>
#include <xdrstring.h>
#include "di_mqtt.h"
#include "../../../main/scheduler.h"
#include "datains.h"
#include "../../domain/dom_datain.h"
#include "../../../dataout/cluster/do_cluster.h"
#include "../readout.h"
#include "../../../commu/commu.h"

#include <mosquitto.h>

#ifdef DMALLOC
#include dmalloc.h
#endif

/* MQTT_IDNAME is used to build an identifier for the mqtt broker */
#define MQTT_IDNAME "em"
/* ID_PREFIX will precede the topic in the data cluster */
#define ID_PREFIX "mqtt://"

#define swap_int(a) ( ((a) << 24) | \
		      (((a) << 8) & 0x00ff0000) | \
		      (((a) >> 8) & 0x0000ff00) | \
	((unsigned int)(a) >>24) )

#define set_max(a, b) ((a)=(a)<(b)?(b):(a))

struct topic {
    struct topic *next;
    char *name;
};

struct di_mqtt_data {
    int idx;
    int qos;
    struct topic *topics;
    struct mosquitto *mosq;
    char mqtt_id[MOSQ_MQTT_ID_MAX_LENGTH+1];
    int mosqsock;
    struct seltaskdescr* selecttask;
    struct taskdescr* periodictask;

#if 0
    /* data information */
    struct Cluster* cluster;
    int head[2];
    int idx;
    int bytes;
#endif
};
RCS_REGISTER(cvsid, "objects/pi/readout_em_cluster")

extern ems_u32* outptr;

/******************************************************************************/
void
init_mqtt(void)
{
    mosquitto_lib_init();
}
/******************************************************************************/
void
cleanup_mqtt(void)
{
    mosquitto_lib_cleanup();
}
/******************************************************************************/
static void
mqtt_fatal_error(struct di_mqtt_data *private)
{
    int idx=private->idx;

    printf("mqtt_fatal_error called for idx %d\n", idx);

    datain_cl[idx].status=Invoc_error;
    if (private->selecttask) {
        sched_delete_select_task(private->selecttask);
        private->selecttask=0;
    }
    if (private->periodictask) {
        sched_remove_task(private->periodictask);
        private->periodictask=0;
    }
    /*
     * Because we do not communicate with the broker anymore it is possible
     * that the broker will close the connection.
     * We should be aware of this.
     */

    fatal_readout_error();
}
/******************************************************************************/
/*
 * we return -1 in case of error, but we are called from mqtt_on_message which
 * can not handle any errors
 */
static int
write_to_cluster(struct di_mqtt_data *private,
        const struct mosquitto_message *message, struct timeval *now)
{
    struct Cluster *cl;
    size_t clustersize;
    int nidx, xtlen, xplen, topiclen;
    char *cptr;

    /* calculate size of cluster */
      /* length of the identifier */
    topiclen=strlen(message->topic)+strlen(ID_PREFIX);
    xtlen=(topiclen+3)/4;
      /* length of the payload */
    xplen=(message->payloadlen+3)/4;
      /*  the cluster header needs 10 words:
       *    size, endiantest, type
       *    optionsize, optionflags, timestamp (2 words)
       *    flags, fragment_id, version
       *  and we need two words for the sizes of identifier and payload
       */
    clustersize=12+xtlen+xplen;

    if (clustersize>cluster_max) {
        complain("di_mqtt: data size (%d words) too large", xplen);
        return -1;
    }


    /* if there are no clusters available we just throw the messages away
       we do not stop reading the mqtt messages */
    cl=clusters_create(clustersize, private->idx, "mqtt data");
    if (!cl) {
        complain("MQTT reader: no space available, message lost (%s)",
                message->topic);
        return -1;
    }

    /* complete the internal header */
    cl->size=clustersize;
    cl->type=clusterty_async_data2;
    cl->flags=message->retain?0x10:0;
    cl->optsize=3;
    cl->ved_id=0;

    /* fill the external header */
    nidx=0;
    cl->data[nidx++]=htonl(cl->size-1);
    cl->data[nidx++]=htonl(0x12345678);
    cl->data[nidx++]=htonl(cl->type);
    cl->data[nidx++]=htonl(cl->optsize);
    cl->data[nidx++]=htonl(ClOPTFL_TIME);
    cl->data[nidx++]=htonl(now->tv_sec);
    cl->data[nidx++]=htonl(now->tv_usec);
    cl->data[nidx++]=htonl(cl->flags);
    cl->data[nidx++]=htonl(0); /* fragment_id */
    cl->data[nidx++]=htonl(0); /* version */

    /* copy the identifier */
    cl->data[nidx+xtlen]=0; /* the unused bytes at the end must be 0 */
    cl->data[nidx++]=htonl(topiclen);
    cptr=(char*)(cl->data+nidx);
    cptr=stpcpy(cptr, ID_PREFIX);
    strcpy(cptr, message->topic);
    nidx+=xtlen;

    /* copy the payload */
    cl->data[nidx+xplen]=0; /* the unused bytes at the end must be 0 */
    cl->data[nidx++]=htonl(message->payloadlen);
    cptr=(char*)(cl->data+nidx);
    if (message->payloadlen)
        memcpy(cptr, message->payload, message->payloadlen);

    cl->swapped=cl->data[1]==0x78563412;

    clusters_deposit(cl);

    return 0;
}
/******************************************************************************/
#if 0
static void
dump_message(const struct mosquitto_message *message)
{
    u_int8_t *m=(u_int8_t*)message->payload;
    int len=message->payloadlen, i;

    printf("mid        = %d\n", message->mid);
    printf("topic      : %s\n", message->topic);
    printf("payloadlen = %d\n", message->payloadlen);
    printf("qos        = %d\n", message->qos);
    printf("retain     = %d\n", message->retain);

    for (i=0; i<len; i++) {
        u_int8_t c=m[i];
        if ((i%16)==0)
            fprintf(stderr, "\n");
        fprintf(stderr, "%02x%c ", c, (c>=0x20 && c<127)?m[i]:'.');
    }
    fprintf(stderr, "\n");
    for (i=0; i<len; i++) {
        u_int8_t c=m[i];
        fprintf(stderr, "%c", (c>=0x20 && c<127)?m[i]:'.');
    }
    fprintf(stderr, "\n");
}
#endif
/******************************************************************************/
static int
mqtt_subscribe(struct di_mqtt_data *private)
{
    struct topic *topic;
    int res;

    topic=private->topics;
    while (topic) {
        printf("will subscribe to %s\n", topic->name);
        res=mosquitto_subscribe(private->mosq, NULL, topic->name, private->qos);
        if (res!=MOSQ_ERR_SUCCESS) {
            complain("mosquitto_subscribe: %s\n", mosquitto_strerror(res));
            return -1;
        }
        topic=topic->next;
    }
    return 0;
}
/******************************************************************************/
static void
mqtt_on_connect(__attribute__((unused)) struct mosquitto *mosq, void *obj,
        int rc)
{
    struct di_mqtt_data *private=(struct di_mqtt_data*)obj;

    if (rc==0) {
        mqtt_subscribe(private);
    } else {
        complain("MQTT: Connect failed: rc=%d", rc);
        mqtt_fatal_error(obj);
    }
}
/******************************************************************************/
static void
mqtt_on_disconnect(__attribute__((unused)) struct mosquitto *mosq, void *obj,
        int rc)
{
    struct di_mqtt_data *private=(struct di_mqtt_data*)obj;

    if (rc!=0)
        complain("MQTT: on_disconnect: rc=%d\n", rc);

    if (private->selecttask) {
        sched_delete_select_task(private->selecttask);
        private->selecttask=0;
    }
    if (private->periodictask) {
        sched_remove_task(private->periodictask);
        private->periodictask=0;
    }
    private->mosqsock=-1;
}
/******************************************************************************/
#if 0
static void
mqtt_on_publish(struct mosquitto *mosq, void *obj, int mid)
{
    printf("MQTT: on_publish: message ID=%d\n", mid);
}
#endif
/******************************************************************************/
static void
mqtt_on_message(__attribute__((unused)) struct mosquitto *mosq, void *obj,
        const struct mosquitto_message *message)
{
    struct di_mqtt_data *private=(struct di_mqtt_data*)obj;
    struct timeval now;

    gettimeofday(&now, 0);
#if 0
    dump_message(message);
#endif
    write_to_cluster(private, message, &now);
}
/******************************************************************************/
#if 0
static void
mqtt_on_subscribe(struct mosquitto *mosq, void *obj, int mid,
        int qos_count, const int *granted_qos)
{
    //struct di_mqtt_data *private=(struct di_mqtt_data*)obj;

    printf("MQTT: on_subscribe: message ID=%d\n", mid);
}
#endif
/******************************************************************************/
#if 0
static void
mqtt_on_unsubscribe(struct mosquitto *mosq, void *obj, int mid)
{
    //struct di_mqtt_data *private=(struct di_mqtt_data*)obj;

    printf("MQTT: on_unsubscribe: message ID=%d\n", mid);
}
#endif
/******************************************************************************/
static void
mqtt_on_log(__attribute__((unused)) struct mosquitto *mosq, void *obj,
        int level, const char *str)
{
    struct di_mqtt_data *private=(struct di_mqtt_data*)obj;
    int idx=private->idx;

    if (level>=MOSQ_LOG_DEBUG)
        return;
    
    complain("MQTT log[%d](%d): %s\n", idx, level, str);
}
/*****************************************************************************/
static void
complain_mosq_error(const char *txt, int rc, int error)
{
    if (rc==MOSQ_ERR_ERRNO) {
        complain("%s: %s", txt, strerror(error));
    } else {
        complain("%s: %s", txt, mosquitto_strerror(rc));
    }
}
/*****************************************************************************/
static void
mqtt_periodic_callback(union callbackdata data)
{
    struct di_mqtt_data *private=(struct di_mqtt_data*)data.p;
    int res;

#if 0
    printf("mqtt_periodic\n");
#endif

    res=mosquitto_loop_misc(private->mosq);
    if (res!=MOSQ_ERR_SUCCESS) {
        complain("mosquitto_misc: %s", mosquitto_strerror(res));
        mqtt_fatal_error(private);
        return;
    }

    if (mosquitto_want_write(private->mosq)) {
        printf("will write\n");
        res=mosquitto_loop_write(private->mosq, 1);
        if (res!=MOSQ_ERR_SUCCESS) {
            complain("mosquitto_write: %s", mosquitto_strerror(res));
            mqtt_fatal_error(private);
            return;
        }
    }
}
/*****************************************************************************/
static void
mqtt_select_callback(__attribute__((unused)) int path,
        enum select_types selected, union callbackdata data)
{
    struct di_mqtt_data *private=(struct di_mqtt_data*)data.p;
    int res;

#if 0
    printf("mqtt_select: selected=%d\n", selected);
#endif

    if (selected&select_read) {
#if 0
        printf("will read\n");
#endif
        res=mosquitto_loop_read(private->mosq, 1);
        if (res!=MOSQ_ERR_SUCCESS) {
            complain("mosquitto_read: %s", mosquitto_strerror(res));
            mqtt_fatal_error(private);
            return;
        }
    }

    /* call periodic_callback for write and misc */
    mqtt_periodic_callback(data);
}
/*****************************************************************************/
static int
generate_mqtt_id(char *id, int idx)
{
    char hostname[256];
    pid_t pid;

    gethostname(hostname, 256);
    hostname[255]=0;
    pid=getpid();

    /* the next check is not correct because we do not take the length of the
       pid into account */
    if (strlen(MQTT_IDNAME)+strlen(hostname)+16>=MOSQ_MQTT_ID_MAX_LENGTH) {
        int hostcut=MOSQ_MQTT_ID_MAX_LENGTH-9-strlen(MQTT_IDNAME);
        if (hostcut<0) {
            complain("generate_mqtt_id: hostname needs negative length\n");
            return -1;
        }
        hostname[hostcut]=0;
    }

    snprintf(id, MOSQ_MQTT_ID_MAX_LENGTH, "%s_%s:%d_%d",
        MQTT_IDNAME, hostname, pid, idx);
    return 0;
}
/*****************************************************************************/
static errcode
di_mqtt_start(int idx)
{
    struct di_mqtt_data *private=datain_cl[idx].private;
    u_int32_t brokeraddr;
    char str[32];
    union callbackdata cbdata;
    int rc;

    printf("di_mqtt_start(%d)\n", idx);

    if (generate_mqtt_id(private->mqtt_id, idx)<0)
        return Err_System;

    private->mosq=mosquitto_new(private->mqtt_id, true, private);
    if (!private->mosq) {
        complain("mosquitto_new: %s", strerror(errno));
        mosquitto_lib_cleanup();
        return Err_System;
    }

    mosquitto_connect_callback_set(private->mosq, mqtt_on_connect);
    mosquitto_disconnect_callback_set(private->mosq, mqtt_on_disconnect);
    mosquitto_message_callback_set(private->mosq, mqtt_on_message);
    mosquitto_log_callback_set(private->mosq, mqtt_on_log);
#if 0
    mosquitto_publish_callback_set(private->mosq, mqtt_on_publish);
    mosquitto_subscribe_callback_set(private->mosq, mqtt_on_subscribe);
    mosquitto_unsubscribe_callback_set(private->mosq, mqtt_on_unsubscribe);
#endif

    /*
     * we have the broker address as a number, but unfortunately
     * mosquitto_connect expect it as a string
     */
    brokeraddr=datain[idx].addr.inetsock.host;
    snprintf(str, 32, "%d.%d.%d.%d",
        (brokeraddr>>24)&0xff,
        (brokeraddr>>16)&0xff,
        (brokeraddr>>8)&0xff,
        brokeraddr&0xff);
    rc=mosquitto_connect(private->mosq, str,
            datain[idx].addr.inetsock.port, 0);
    if (rc) {
        complain_mosq_error("mosquitto_connect", rc, errno);
        return Err_System;
    }
 
    complain("mqtt connected to %s:%d as %s", str,
        datain[idx].addr.inetsock.port, private->mqtt_id);
    private->mosqsock=mosquitto_socket(private->mosq);

    snprintf(str, 32, "di_%02d_mqtt_s", idx);
    cbdata.p=private;
    private->selecttask=sched_insert_select_task(mqtt_select_callback, cbdata,
            str, private->mosqsock, select_read, 1);
    if (private->selecttask==0) {
        complain("mqtt_prepare: could not install select handler");
        private->mosqsock=-1;
        mosquitto_disconnect(private->mosq);
        mosquitto_destroy(private->mosq);
        return Err_System;
    }

    snprintf(str, 32, "di_%02d_mqtt_p", idx);
    private->periodictask=sched_exec_periodic(mqtt_periodic_callback, cbdata,
            100 /* 1s */, str);
    if (private->periodictask==0) {
        complain("mqtt_prepare: could not install periodic handler");
        sched_delete_select_task(private->selecttask);
        private->mosqsock=-1;
        mosquitto_disconnect(private->mosq);
        mosquitto_destroy(private->mosq);
        return Err_System;
    }

    datain_cl[idx].status=Invoc_active;

    return OK;
}
/*****************************************************************************/
/* we will not really suspend or resume mqtt */
static errcode
di_mqtt_stop(int idx)
{
    T(readout_em_cluster/di_cluster.c:di_mqtt_stop)
    printf("di_mqtt_stop(%d)\n", idx);

    if (datain_cl[idx].status!=Invoc_active) {
        printf("di_mqtt_stop(%d): status=%d\n", idx, datain_cl[idx].status);
    }

    datain_cl[idx].status=Invoc_stopped;
    return OK;
}
/*****************************************************************************/
/* we will not really suspend or resume mqtt */
static errcode
di_mqtt_resume(int idx)
{
    T(readout_em_cluster/di_cluster.c:di_mqtt_resume)

    if (datain_cl[idx].status!=Invoc_stopped) {
        printf("di_mqtt_resume(%d): status=%d\n", idx, datain_cl[idx].status);
        return Err_Program;
    }

    datain_cl[idx].status=Invoc_active;
    return OK;
}
/*****************************************************************************/
static errcode
di_mqtt_reset(int idx)
{
    struct di_mqtt_data *private=(struct di_mqtt_data*)datain_cl[idx].private;

    T(readout_em_cluster/di_cluster.c:di_mqtt_reset)
    printf("di_mqtt_reset(%d)\n", idx);

    if (datain_cl[idx].status==Invoc_notactive) {
        printf("di_mqtt_reset(%d): not active\n", idx);
        return Err_Program;
    }

    if (private->selecttask) {
        sched_delete_select_task(private->selecttask);
        private->selecttask=0;
    }
    if (private->periodictask) {
        sched_remove_task(private->periodictask);
        private->periodictask=0;
    }

    private->mosqsock=-1;
    mosquitto_disconnect(private->mosq);
    mosquitto_destroy(private->mosq);

    datain_cl[idx].status=Invoc_notactive;

    return OK;
}
/*****************************************************************************/
static void
di_mqtt_reactivate(int idx)
{
    T(readout_em_cluster/di_cluster.c:di_mqtt_reactivate)
    printf("di_mqtt_reactivate(%d)\n", idx);
}
/*****************************************************************************/
static void
di_mqtt_request(int di_idx, int ved_idx, int evnum)
{
    printf("di_mqtt_request(di_idx=%d, ved_idx=%d, evnum=%d)\n",
            di_idx, ved_idx, evnum);
}
/*****************************************************************************/
static void
di_mqtt_clean_topics(struct di_mqtt_data *private)
{
    struct topic *topic;

    while (private->topics) {
        topic=private->topics;
        private->topics=topic->next;
        free(topic->name);
        free(topic);
    }
}
/*****************************************************************************/
static void
di_mqtt_cleanup(int idx)
{
    struct di_mqtt_data *private=(struct di_mqtt_data*)datain_cl[idx].private;

    T(readout_em_cluster/di_cluster.c:di_mqtt_cleanup)
    printf("di_mqtt_cleanup(%d)\n", idx);
 
    di_mqtt_clean_topics(private);
    free(datain_cl[idx].private);
    datain_cl[idx].private=0;
}
/*****************************************************************************/
static int
add_topic(struct di_mqtt_data *private, const char *topicname)
{
    struct topic *topic;

    topic=malloc(sizeof(struct topic));
    if (!topic) {
        complain("di_mqtt.c:add_topic: %s", strerror(errno));
        return -1;
    }
    topic->name=strdup(topicname);
    if (!topic->name) {
        complain("di_mqtt.c:add_topic(%s): %s", topicname, strerror(errno));
        free(topic);
        return -1;
    }
    topic->next=private->topics;
    private->topics=topic;
    return 0;
}
/*****************************************************************************/
static ems_u32*
add_topic_xdr(struct di_mqtt_data *private, ems_u32 *xdr)
{
    struct topic *topic;
    topic=malloc(sizeof(struct topic));
    if (!topic) {
        complain("di_mqtt.c:add_topic_xdr: %s", strerror(errno));
        return 0;
    }
    xdr=xdrstrcdup(&topic->name, xdr);
    if (!xdr) {
        complain("di_mqtt.c:add_topic_xdr: %s", strerror(errno));
        free(topic);
        return 0;
    }
    topic->next=private->topics;
    private->topics=topic;
    return xdr;
}
/*****************************************************************************/
static di_procs mqtt_procs={
    di_mqtt_start,
    di_mqtt_stop,
    di_mqtt_resume,
    di_mqtt_reset,
    di_mqtt_cleanup,
    di_mqtt_reactivate,
    di_mqtt_request
};
/*****************************************************************************/
errcode
di_mqtt_init(int idx, int qlen, ems_u32* q)
{
    struct di_mqtt_data *private;

    T(readout_em_cluster/di_mqtt.c:di_mqtt_init)
#if 0
    printf("di_mqtt_init(idx=%d, qlen=%d, q=%p)\n", idx, qlen, q);
#endif

    private=calloc(1, sizeof(struct di_mqtt_data));
    if (!private) {
        printf("di_cluster_sock_init: %s\n", strerror(errno));
        return plErr_NoMem;
    }
    private->idx=idx;
    private->mosqsock=-1;

    datain_cl[idx].private=private;
    datain_cl[idx].procs=mqtt_procs;
/* these fields are inherited from InOut_Cluster and are mostly useless here */
    datain_cl[idx].seltask=0;
    datain_cl[idx].suspended=0;
    datain_cl[idx].status=Invoc_notactive;
    datain_cl[idx].error=0;
    datain_cl[idx].max_displacement=0;

/* we need the information in q: the topics */
    while (qlen) {
        int xdrlen=xdrstrlen(q);
        /* sanity check */
        if (xdrlen>qlen) {
            printf("di_mqtt_init: q too short for string\n");
            di_mqtt_clean_topics(private);
            free(private);
            datain_cl[idx].private=0;
            return plErr_ArgNum;
        }
        if (!add_topic_xdr(private, q)) {
            printf("di_mqtt_init: cannot allocate memory for topic\n");
            di_mqtt_clean_topics(private);
            free(private);
            datain_cl[idx].private=0;
            return plErr_NoMem;
        }
        q+=xdrlen;
        qlen-=xdrlen;
    }

    if (!private->topics) {
        if (add_topic(private, "#")<0) {
            printf("di_mqtt_init: cannot allocate memory for topic '#'\n");
            free(private);
            datain_cl[idx].private=0;
            return plErr_NoMem;
        }
    }

    return OK;
}
/*****************************************************************************/
/*****************************************************************************/
