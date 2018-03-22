/*
 * server/procs/ip/ip_opaque.c
 * created 2016-04-27 PeWue
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: ip_opaque.c,v 1.9 2017/10/20 23:20:52 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errorcodes.h>
#include <modultypes.h>
#include <xdrstring.h>
#include <rcs_ids.h>
#include "../../commu/commu.h"
#include "../procs.h"
#include "../../objects/domain/dom_ml.h"
#include "../../lowlevel/oscompat/oscompat.h"
#include "../../lowlevel/devices.h"
#include "../../lowlevel/ipmod/ipmodules.h"

extern ems_u32* outptr;
extern unsigned int *memberlist;

RCS_REGISTER(cvsid, "procs/ip")

struct ip_opaque_priv_data {
    plerrcode (*read)(ml_entry*, ems_u8*, int);
    plerrcode (*read_nonblock)(ml_entry*, ems_u8*, int, int*);
    plerrcode (*read_timeout)(ml_entry*, ems_u8*, int, struct timeval*);
    plerrcode (*write)(ml_entry*, ems_u8*, int);
};

struct sendbuf {
    ems_u8 *buf;
    int size;
    int num;
};
static struct sendbuf sendbuf={0, 0, 0};

/*****************************************************************************/
static plerrcode
ip_opaque_init_socket(ml_entry* module)
{
    plerrcode pres;

    /* check values and apply defaults, if necessary */
    if (!module->address.ip.protocol) {
        module->address.ip.protocol=strdup("TCP");
    }
#if 0
    if (!module->address.ip.rserv) {
        complain("ip_opaque: no host address given");
        return plErr_ArgRange;
    }
    if (!module->address.ip.lserv) {
        complain("ip_opaque: no port given");
        return plErr_ArgRange;
    }
#endif
#if 0
    module->address.ip.recvtimeout=50; /* 50 ms */
#endif
    pres=ipmod_init(module);

    return pres;
}
/*****************************************************************************/
static plerrcode
ip_opaque_init_private(ml_entry* module)
{
    struct ip_opaque_priv_data *priv;
    plerrcode pres;

    if (module->private_data)
        return plOK;

    priv=calloc(1, sizeof(struct ip_opaque_priv_data));
    if (!priv) {
        complain("ip_opaque_init_private: %s", strerror(errno));
        return plErr_NoMem;
    }

    pres=ip_opaque_init_socket(module);
    if (pres!=plOK) {
        goto error;
    } else {
        priv->read=ipmod_read_stream;
        priv->read_nonblock=ipmod_read_stream_nonblock;
        priv->read_timeout=ipmod_read_stream_timeout;
        priv->write=ipmod_write_stream;
        module->private_data=priv;
    }

error:
    if (pres!=plOK) {
        free(priv);
        module->private_data=0;
    }
    return pres;
}
/*****************************************************************************/
static int
is_ip_opaque_module(ml_entry* module)
{
    return module->modulclass==modul_ip && module->modultype==IP_opaque;
}
/*****************************************************************************/
/**
 * 'for_each_member' calls 'proc' for each member of the IS modullist or
 * (if the IS modullist does not exist) for each member of the global
 * modullist.
 * p[1] has to be the module index (given as -1). It will be replaced by
 * the actual module idx.
 *
 * if write_count>0 the output will be preceded by the number of modules
 * if write_count==1 this is supressed if the procedures made no output
 * if write_count==2 this is not supressed
 *
 * Warning: because of the static variable (for_each_member_idx) this
 * function ist not reenrant. But ems server does not use threads...
 *
 * Warning 2: unlike check_ip_opaque this procedure does not check whether
 * the hardware is really a ip_opaque
 */

static int member_idx_=-1;
static int
__attribute__((unused))
member_idx(void)
{
    return member_idx_;
}

static plerrcode
for_each_ip_opaque_member(ems_u32 *p, plerrcode(*proc)(ems_u32*),
        int write_count)
{
    ml_entry* module;
    ems_u32 *help;
    ems_u32 *pc;
    plerrcode pres=plOK;
    unsigned int i, psize;

    if (!modullist)
        return plOK;

    /* create a copy of p */
    psize=4*(p[0]+1);
    pc=malloc(psize);
    if (!pc) {
        complain("ip_opaque:for_each: %s", strerror(errno));
        return plErr_NoMem;
    }
    memcpy(pc, p, psize);
    member_idx_=-1;
    if (write_count)
        help=outptr++;

    if (memberlist) { /* iterate over memberlist */
        for (i=1; i<=memberlist[0]; i++) {
            module=&modullist->entry[memberlist[i]];
            if (is_ip_opaque_module(module)) {
                pc[1]=i;
                member_idx_++;
                pres=proc(pc);
                if (pres!=plOK)
                    break;
            }
        }
    } else {          /* iterate over modullist */
        for (i=0; i<modullist->modnum; i++) {
            module=&modullist->entry[i];
            if (is_ip_opaque_module(module)) {
                pc[1]=i;
                member_idx_++;
                pres=proc(pc);
                if (pres!=plOK)
                    break;
            }
        }
    }

    if (write_count) {
        if (write_count==1 && outptr==help+1)
            outptr=help;
        else
            *help=member_idx_+1;
    }
    member_idx_=-1;
    free(pc);
    return pres;
}
/*****************************************************************************/
/**
 * 'for_each_module' calls 'proc' for each member of the IS modullist or
 * (if the IS modullist does not exist) for each member of the global
 * modullist.
 */
static plerrcode
for_each_ip_opaque_module(plerrcode(*proc)(ml_entry*))
{
    ml_entry* module;
    plerrcode pres=plOK;
    unsigned int i;

    if (memberlist) { /* iterate over memberlist */
        for (i=1; i<=memberlist[0]; i++) {
            module=&modullist->entry[memberlist[i]];
            if (is_ip_opaque_module(module)) {
                pres=proc(module);
                if (pres!=plOK)
                    break;
            }
        }
    } else {          /* iterate over modullist */
        for (i=0; i<modullist->modnum; i++) {
            module=&modullist->entry[i];
            if (is_ip_opaque_module(module)) {
                pres=proc(module);
                if (pres!=plOK)
                    break;
            }
        }
    }

    return pres;
}
/*****************************************************************************/
/*
 * init_ip_mod calls ip_mod_init_private to initiate the private
 * data structure of the module(s). Therefore each test_... procedure is
 * required to use init_ip_mod.
 *
 * In num it returns an upper limit of the number of modules used.
 */
static plerrcode
init_ip_mod(ems_i32 idx, int *num, const char *caller)
{
    ml_entry *module;
    plerrcode pres;

    if (!modullist)
        return Err_NoDomain;

    if (num) {
        if (idx>=0)
            *num=1;
        else
            *num=memberlist?memberlist[0]:modullist->modnum;
    }

    /* if idx<0 all IP_opaque modules are requested
     * 'for_each_ip_opaque_modul' will iterate over all IP_opaque modules
     * and initialise the private module data, if necessary
     */
    if (idx<0) {
        pres=for_each_ip_opaque_module(ip_opaque_init_private);
        if (pres!=plOK)
            complain("init_ip_mod(%s, idx=-1): communication init failed",
                    caller);
        return pres;
    }

    pres=get_modent(idx, &module);
    if (pres!=plOK) {
        complain("init_ip_mod(%s): get_modent(idx=%u) failed", caller, idx);
        return pres;
    }

    if (!is_ip_opaque_module(module)) {
        complain("init_ip_mod(%s, idx=%u): not IP_opaque", caller, idx);
        return plErr_BadModTyp;
    }

    pres=ip_opaque_init_private(module);
    if (pres!=plOK) {
        complain("init_ip_mod(%s, idx=%d]: communication init failed",
                caller, idx);
        return pres;
    }

    return plOK;
}
/*****************************************************************************/
/*****************************************************************************/
/**
 * ip_opaque_init explicitly initialises the read and write procedures for
 * one or all "IP_opaque" modules of the IS.
 * To call this procedure is optional, the initialisation is done on demand
 * if needed. (?)
 * It makes no harm if the procedure is called more than once.
 * The socket(s) will persist until free_modullist is executed.
 * 
 * p[0]: argcount(==1)
 * p[1]: index in memberlist (or -1 for all modules)
 */
plerrcode proc_ip_opaque_init(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    int num; /* unused here */
    plerrcode pres;

    pres=init_ip_mod(ip[1], &num, "ip_opaque_init");
    if (pres!=plOK)
        complain("communication init for ip_opaque[%d] failed", p[1]);

    return pres;
}

plerrcode test_proc_ip_opaque_init(ems_u32* p)
{
    if (p[0]!=1) {
        complain("ip_opaque_init: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    wirbrauchen=0;
    return plOK;
}

char name_proc_ip_opaque_init[] = "ip_opaque_init";
int ver_proc_ip_opaque_init = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(3)
 * p[1]: index in memberlist (or -1 for all IP_opaque modules)
 * p[2]: flags: 1: use XDR 2:add line end 4: ...
 * p[3..]: arbitrary text
 */
plerrcode proc_ip_opaque_send_text(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (member_idx()<0) {
        int c_len=xdrstrclen(p+3);  
        int xdr_len=((c_len+2+3)/4+1)*4; /* we may have to add a line end */
        ems_u8 *sendbuf_start;

        /* sanity check */
        if (c_len==0) {
            complain("ip_opaque_send_text: empty string");
            if (p[2]==0) /* empty string without XDR is really nothing */
                return plOK;
        }

        if (sendbuf.size<xdr_len) {
            sendbuf.buf=realloc(sendbuf.buf, xdr_len);
            if (sendbuf.buf==0) {
                complain("ip_opaque_send_text: realloc(%d): %s",
                        xdr_len, strerror(errno));
                return plErr_NoMem;
            }
            sendbuf.size=xdr_len;
        }
        memset(sendbuf.buf, 0, sendbuf.size);
        if (p[2]&1) /* use XDR */
            sendbuf_start=sendbuf.buf+4; /* reserve space for length counter */
        else
            sendbuf_start=sendbuf.buf;

        extractstring((char*)sendbuf_start, p+3);
        /* here we have a standard C string, beginning at 'sendbuf.start', with
           a length of 'c_len', padded with at least two zeros */
        if (p[2]&2 && sendbuf_start[c_len-1]!='\n') {
            sendbuf_start[c_len++]='\n';
        }
#if 0
        printf("ip_opaque_send_text: string is >%s<\n", sendbuf_start);
#endif
        if (p[2]&1) { /* use XDR */
            sendbuf.buf[0]=(c_len>>24)&0xff;
            sendbuf.buf[1]=(c_len>>16)&0xff;
            sendbuf.buf[2]=(c_len>>8)&0xff;
            sendbuf.buf[3]=c_len&0xff;
            sendbuf.num=((c_len+3)/4+1)*4;
        } else {
            sendbuf.num=c_len;
        }
    }

    if (ip[1]<0) {
        pres=for_each_ip_opaque_member(p, proc_ip_opaque_send_text, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct ip_opaque_priv_data *priv;
        priv=(struct ip_opaque_priv_data*)module->private_data;

        pres=priv->write(module, sendbuf.buf, sendbuf.num);
    }

    return pres;
}

plerrcode test_proc_ip_opaque_send_text(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]<3) {
        complain("ip_opaque_send_tex: only %d arguments given", p[0]);
        return plErr_ArgNum;
    }

    if (p[3]>(p[0]-2)*4) {
        complain("ip_opaque_send_tex: illegal illegal string length %d", ip[3]);
        return plErr_ArgRange;
    }

    if ((pres=init_ip_mod(ip[1], &num, "ip_opaque_send_text"))!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_ip_opaque_send_text[] = "ip_opaque_send_text";
int ver_proc_ip_opaque_send_text = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(3)
 * p[1]: index in memberlist (or -1 for all IP_opaque modules)
 * p[2]: 1: use XDR 0: send raw bytes
 * p[3..]: arbitrary bytes
 */
plerrcode proc_ip_opaque_send_bytes(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres=plOK;

    if (member_idx()<0) {
        int c_len=p[0]-2;
        int xdr_len=((c_len+3)/4+1)*4;
        ems_u8 *sendbuf_start;
        int i;

        /* zero bytes without without XDR are really nothing */
        if (c_len==0 && p[2]==0) {
            complain("ip_opaque_send_bytes: nothing to send");
            return plOK;
        }

        if (sendbuf.size<xdr_len) {
            sendbuf.buf=realloc(sendbuf.buf, xdr_len);
            if (sendbuf.buf==0) {
                complain("ip_opaque_send_bytes: realloc(%d): %s",
                        xdr_len, strerror(errno));
                return plErr_NoMem;
            }
            sendbuf.size=xdr_len;
        }
        memset(sendbuf.buf, 0, sendbuf.size);
        if (p[2]) /* use XDR */
            sendbuf_start=sendbuf.buf+4; /* reserve space for length counter */
        else
            sendbuf_start=sendbuf.buf;

        for (i=0; i<c_len; i++)
            sendbuf_start[i]=p[3+i];

        if (p[2]) { /* use XDR */
            sendbuf.buf[0]=(c_len>>24)&0xff;
            sendbuf.buf[1]=(c_len>>16)&0xff;
            sendbuf.buf[2]=(c_len>>8)&0xff;
            sendbuf.buf[3]=c_len&0xff;
            sendbuf.num=((c_len+3)/4+1)*4;
        } else {
            sendbuf.num=c_len;
        }
    }

    if (ip[1]<0) {
        pres=for_each_ip_opaque_member(p, proc_ip_opaque_send_bytes, 1);
    } else {
        ml_entry* module=ModulEnt(p[1]);
        struct ip_opaque_priv_data *priv;
        priv=(struct ip_opaque_priv_data*)module->private_data;

        pres=priv->write(module, sendbuf.buf, sendbuf.num);
    }

    return pres;
}

plerrcode test_proc_ip_opaque_send_bytes(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]<2) {
        complain("ip_opaque_send_bytes: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if ((pres=init_ip_mod(ip[1], &num, "ip_opaque_send_bytes"))!=plOK)
        return pres;

    wirbrauchen=0;
    return plOK;
}

char name_proc_ip_opaque_send_bytes[] = "ip_opaque_send_bytes";
int ver_proc_ip_opaque_send_bytes = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(3)
 * p[1]: index in memberlist
 * p[2]: 1: use XDR 2: ??? 4: nonblocking
 * [p[3]: number of bytes to read (raw bytes only)]
 *
 * WARNING:
 *      XDR + nonblocking may lead to garbage in the next call
 *      XDR without nonblocking can block forever
 */
plerrcode proc_ip_opaque_recv_bytes(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct ip_opaque_priv_data *priv;
    priv=(struct ip_opaque_priv_data*)module->private_data;
    plerrcode pres=plOK;
    int len, num, i;

    if (p[2]&1) { /* use XDR */
        ems_u8 head[4];
        pres=priv->read(module, head, 4);
        if (pres!=plOK)
            return pres;
        num=head[0]<<24 | head[1]<<16 | head[2]<<8 | head[3];
        len=((num+3)/4)*4;
    } else {
        num=len=p[3];
    }
printf("num=%d, len=%d\n", num, len);

    if (sendbuf.size<len) {
        sendbuf.buf=realloc(sendbuf.buf, len);
        if (sendbuf.buf==0) {
            complain("ip_opaque_recv_bytes: realloc(%d): %s",
                    len, strerror(errno));
            return plErr_NoMem;
        }
        sendbuf.size=len;
    }
    if (p[2]&4) {
        int received;
        pres=priv->read_nonblock(module, sendbuf.buf, len, &received);
        num=received>=0?received:0;
    } else {
        pres=priv->read(module, sendbuf.buf, len);
    }
    if (pres!=plOK)
        return pres;

    for (i=0; i<num; i++)
        *outptr++=sendbuf.buf[i];

    return pres;
}

plerrcode test_proc_ip_opaque_recv_bytes(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]<2 || p[0]>3) {
        complain("ip_opaque_recv_bytes: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (!(p[2]&1) && p[0]<3) {
        complain("ip_opaque_recv_bytes: number of raw bytes must be given");
        return plErr_ArgNum;
    }

    if (ip[1]<0) {
        complain("ip_opaque_recv_bytes: illegal module '-1' ");
        return plErr_ArgRange;
    }

    if ((pres=init_ip_mod(ip[1], &num, "ip_opaque_recv_bytes"))!=plOK)
        return pres;

    if (p[2]&1)
        wirbrauchen=-1;
    else
        wirbrauchen=p[3];

    return plOK;
}

char name_proc_ip_opaque_recv_bytes[] = "ip_opaque_recv_bytes";
int ver_proc_ip_opaque_recv_bytes = 1;
/*****************************************************************************/
/**
 * p[0]: argcount(3)
 * p[1]: index in memberlist
 * p[2]: flags: 1: use XDR 2: ???
 * [p[3]: timeout in 1/100 s
 * [p[4..]: OK-string (timeout or reading a different string is an error)]]
 *
 * if !(flags&1) (XDR not selected) the procedure will read until '\n' is found
 */
plerrcode proc_ip_opaque_recv_text(ems_u32* p)
{
    ml_entry* module=ModulEnt(p[1]);
    struct ip_opaque_priv_data *priv;
    priv=(struct ip_opaque_priv_data*)module->private_data;
    struct timeval to, t0, now;
    char *okstr;
    plerrcode pres=plOK;
    int loop;

    gettimeofday(&t0, 0);

    to.tv_sec=p[3]/100;
    to.tv_usec=(p[3]%100)*10000;

    if (p[0]>3) {
        xdrstrcdup(&okstr, p+4);
        if (!okstr) {
            pres=plErr_NoMem;
            goto error;
        }
    } else {
        okstr=0;
    }

    for (loop=0; ; loop++) {
        if (p[2]&1) { /* use XDR */
            ems_u8 head[4];
            int num, len;

            pres=priv->read_timeout(module, head, 4, &to);
            if (pres!=plOK) {
                if (pres==plErr_Timeout) {
                    //printf("Timeout reading length\n");
                    pres=plOK;
                }
                if (loop==0)
                    goto error;
                else
                    break;
            }
            num=head[0]<<24 | head[1]<<16 | head[2]<<8 | head[3];
//printf("len: %d\n", num);
            len=((num+3)/4)*4;
            if (sendbuf.size<len+1) {
                sendbuf.buf=realloc(sendbuf.buf, len+1);
                if (sendbuf.buf==0) {
                    complain("ip_opaque_recv_text: realloc(%d): %s",
                            len, strerror(errno));
                    pres=plErr_NoMem;
                    goto error;
                }
                sendbuf.size=len+1;
            }

            pres=priv->read_timeout(module, sendbuf.buf, len, &to);
            if (pres!=plOK) {
                if (pres==plErr_Timeout) {
                    printf("Timeout reading string\n");
                    pres=plOK;
                }
                goto error;
            }
            sendbuf.buf[num]='\0'; /* append string end */
        } else {
            /*pres=read_raw_string(priv, p[3]);*/
            complain("ip_opaque_recv_text(raw) not yet implemented");
            pres=plErr_NotImpl;
            goto error;
        }
    #if 1
        printf("ip_opaque_recv_text(%d): got >%s<\n", loop, sendbuf.buf);
    #endif
        outptr=outstring(outptr, (char*)sendbuf.buf);

        /* preparation for next loop */
        to.tv_sec=to.tv_usec=0;
    }

    if (okstr && strcmp((char*)sendbuf.buf, okstr)) {
        complain("ip_opaque_recv_text: >%s< does not match >%s<",
            sendbuf.buf, okstr);
        pres=plErr_Verify;
    }

error:
    gettimeofday(&now, 0);
    timersub(&now, &t0, &to);
    printf("return after %ld:%ld\n", to.tv_sec, to.tv_usec);

    if (okstr)
        free(okstr);
    return pres;
}

plerrcode test_proc_ip_opaque_recv_text(ems_u32* p)
{
    ems_i32 *ip=(ems_i32*)p;
    plerrcode pres;
    int num;

    if (p[0]<3) {
        complain("ip_opaque_recv_text: illegal # of arguments: %d", p[0]);
        return plErr_ArgNum;
    }

    if (p[0]>3 && p[0]!=4+(p[4]+3)/4) {
        complain("ip_opaque_recv_text: OK-string not a valid string");
        return plErr_ArgNum;
    }

    if ((pres=init_ip_mod(ip[1], &num, "ip_opaque_recv_text"))!=plOK)
        return pres;

    wirbrauchen=-1;
    return plOK;
}

char name_proc_ip_opaque_recv_text[] = "ip_opaque_recv_text";
int ver_proc_ip_opaque_recv_text = 1;
/*****************************************************************************/
/*****************************************************************************/
