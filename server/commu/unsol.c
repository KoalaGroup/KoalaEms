/*
 * commu/unsol.c
 * created: 2009-Aug-23 PW
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: unsol.c,v 1.7 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <msg.h>
#include <requesttypes.h>
#include <objecttypes.h>
#include <unsoltypes.h>
#include <actiontypes.h>
#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "commu.h"

RCS_REGISTER(cvsid, "commu")

/*****************************************************************************/
int
send_unsolicited(UnsolMsg type, ems_u32* body, unsigned int size)
{
    msgheader header;

    T(commu.c::send_unsolicited)

    if (connstate!=conn_ok) {
        return -1;
    }
    header.client= EMS_commu;
    header.ved= EMS_unknown;
    header.type.unsoltype=type;
    header.flags=Flag_Unsolicited | Flag_Confirmation;
    header.size=size;
    header.transid=0;
    if (send_data((ems_u32*)&header,sizeof(msgheader)/4)<0) {
        conn_abort("send error");
        return -1;
    }
    if (size && send_data(body, size)<0) {
        conn_abort("send error");
        return -1;
    }
    //tsleep(50);
    return 0;
}
/****************************************************************************/
static ems_u32*
get_msg_buf(unsigned int num)
{
    static ems_u32* msg=0;
    static unsigned int msglen=0;

    if (num>msglen) {
        if (msg) free(msg);
        msg=malloc(num*sizeof(ems_u32));
        msglen=msg?num:0;
    }
    if (num && !msg) printf("send_unsol...: cannot allocate %d words\n", num);
    return msg;
}
/****************************************************************************/
int
send_unsol_var(UnsolMsg type, unsigned int num, ...)
{
    ems_u32* msg=0;
    va_list ap;
    unsigned int i;

    if (num) {
        if (!(msg=get_msg_buf(num)))
            return -1;

        va_start(ap, num);
        for (i=0; i<num; i++) {
            msg[i]=va_arg(ap, ems_u32);
        }
        va_end(ap);
    }
    return send_unsolicited(type, msg, num);
}
/****************************************************************************/
int
send_unsol_warning(int code, unsigned int num, ...)
{
    ems_u32* msg;
    va_list ap;
    unsigned int i;

    if (!(msg=get_msg_buf(num+2)))
        return -1;

    msg[0]=code;
    msg[1]=num;
    va_start(ap, num);
    for (i=0; i<num; i++) {
        msg[i+2]=va_arg(ap, ems_u32);
    }
    va_end(ap);
    return send_unsolicited(Unsol_Warning, msg, num+2);
}
/****************************************************************************/
int
send_unsol_patience(int code, unsigned int num, ...)
{
    ems_u32* msg;
    va_list ap;
    unsigned int i;

    if (!(msg=get_msg_buf(num+2)))
        return -1;

    msg[0]=code;
    msg[1]=num;
    va_start(ap, num);
    for (i=0; i<num; i++) {
        msg[i+2]=va_arg(ap, ems_u32);
    }
    va_end(ap);
    return send_unsolicited(Unsol_Patience, msg, num+2);
}
/****************************************************************************/
/*
 * each string is sent as a separate XDR string
 * a trailing '\n' is removed from each string
 */
int
send_unsol_text(const char* text, ...)
{
    ems_u32 *msg, *p;
    va_list ap;
    const char* s;
    unsigned int n, l;

    if (!text)
        return -1;

/* count parameters and string sizes */
    n=0; l=0;
    s=text;
    va_start(ap, text);
    do {
        n++;
        l+=strxdrlen(s);
    } while((s=va_arg(ap, char*)));
    va_end(ap);

/* request buffer */
    if (!(msg=get_msg_buf(l+1)))
        return -1;

/* fill buffer */
    p=msg;
    *p++=n;
    s=text;
    va_start(ap, text);
    do {
        l=strlen(s);
        p=outnstring(p, s, l-(l && s[l-1]=='\n'));
    } while ((s=va_arg(ap, char*)));
    va_end(ap);

/* send buffer */
    return send_unsolicited(Unsol_Text, msg, (unsigned int)(p-msg));
}
/****************************************************************************/
/*
 * strings which do not end with a '\n' are concatenated 
 */
int
send_unsol_text2(const char* text, ...)
{
    ems_u32 *msg, *help, *start, *next;
    va_list ap;
    const char* s;
    unsigned int l, nl0, nl1;

    if (!text)
        return -1;

/* count string sizes */
    l=0;
    s=text;
    va_start(ap, text);
    do {
        l+=strxdrlen(s);
    } while ((s=va_arg(ap, char*)));
    va_end(ap);

/* request buffer */
    if (!(msg=get_msg_buf(l+1)))
        return -1;

/* fill buffer */
    help=msg;
    start=msg+1;
    l=strlen(text);
    nl0=l && text[l-1]=='\n';
    next=outnstring(start, text, l-nl0);
    (*help)++;

    va_start(ap, text);
    while ((s=va_arg(ap, char*))) {
        l=strlen(s);
        nl1=l && s[l-1]=='\n';
        if (nl0) { /* new string*/
            start=next;
            next=outnstring(start, s, l-nl1);
        } else {   /* concatenate */
            next=append_xdrstring(start, s, l-nl1);
        }
        nl0=nl1;
        (*help)++;
    }
    va_end(ap);

/* send buffer */
    return send_unsolicited(Unsol_Text, msg, (unsigned int)(next-msg));
}
/****************************************************************************/
int
send_unsol_do_filename(int do_idx, char *name)
{
/*
 *     structure will be:
 *         action==11 action_filename
 *         object==6 dataout
 *         idx
 *         file name
 */
    ems_u32 *msg;
    int xlen;

    xlen=strxdrlen(name);
    if (!(msg=get_msg_buf(3+xlen)))
        return -1;
    msg[0]=status_action_filename;
    msg[1]=Object_do;
    msg[2]=do_idx;
    outstring(msg+3, name);

    return send_unsolicited(Unsol_StatusChanged , msg, 3+xlen);
}
/*****************************************************************************/
#if 0
int send_unsol_alarm_0(int severity, struct timeval tv,
        int i_counter, ems_u32 *i_vector,
        int s_counter, const char **s_vector)
{
    /* to be implemented later */
    return -1;
}
#endif
/*****************************************************************************/
/*****************************************************************************/
