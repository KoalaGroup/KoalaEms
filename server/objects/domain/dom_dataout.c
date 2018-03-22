/*
 * dom_dataout.c
 * created before 30.05.94
 */
static const char* cvsid __attribute__((unused))=
    "$ZEL: dom_dataout.c,v 1.37 2017/10/20 23:21:31 wuestner Exp $";

#include <sconf.h>
#include <debug.h>
#include <stdlib.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <string.h>
#ifdef OSK
#include <types.h>
#include <in.h>
#endif

#include <errorcodes.h>
#include <xdrstring.h>
#include <rcs_ids.h>

#include "../../dataout/dataout.h"

#include "dom_dataout.h"
#include "domdoobj.h"

#ifdef DMALLOC
#include <dmalloc.h>
#endif

extern ems_u32* outptr;

RCS_REGISTER(cvsid, "objects/domain")

/*****************************************************************************/

#ifdef DATAOUT_MULTI
struct dataout_info dataout[MAX_DATAOUT];

static void free_dataout_res(unsigned int i)
{
    switch(dataout[i].addrtyp) {
        case Addr_Modul:
            if(dataout[i].addr.modulname)
                free(dataout[i].addr.modulname);
            break;
        case Addr_LocalSocket:
        case Addr_File:
        case Addr_Tape:
            if(dataout[i].addr.filename)
                free(dataout[i].addr.filename);
            break;
        case Addr_V6Socket:
            free(dataout[i].addr.inetV6sock.node);
            free(dataout[i].addr.inetV6sock.service);
            free(dataout[i].addr.inetV6sock.addr);
            break;
        default:
            /* there is nothing to be freed */
            break;
    }
    if (dataout[i].inout_args) free(dataout[i].inout_args);
    if (dataout[i].address_args) free(dataout[i].address_args);
    if (dataout[i].logfilename) free(dataout[i].logfilename);
    if (dataout[i].loginfo) free(dataout[i].loginfo);
    if (dataout[i].runlinkdir) free(dataout[i].runlinkdir);
}

#endif

/*****************************************************************************/
errcode dom_dataout_init()
{
    int i;
    T(dom_dataout_init)
#ifdef DATAOUT_MULTI
    for (i=0; i<MAX_DATAOUT; i++)
        dataout[i].bufftyp= -1;
#endif
    return OK;
}
/*****************************************************************************/
errcode dom_dataout_done()
{
    unsigned int i;

    T(dom_dataout_done)
#ifdef DATAOUT_MULTI
    for (i=0; i<MAX_DATAOUT; i++) {
        if (dataout[i].bufftyp!=InOut_Invalid) {
            remove_dataout(i);  /*Fehler abfangen!!!*/
            free_dataout_res(i);
            dataout[i].bufftyp=InOut_Invalid;
        }
    }
#endif
    return OK;
}
/*****************************************************************************/
static void
downloaddataout_dump(ems_u32* p, unsigned int len)
{
    ems_u32 *q;
    ems_i32 *ip=(ems_i32*)p;
    unsigned int i;

    printf("downloaddataout_dump: p=%p len=%u\n", p, len);
    for (i=0; i<len; i++) {
        printf("%08x  ", p[i]);
    }
    printf("\n");

    if (ip[2]==-1) {
        printf("new version\n");
        printf("index=%d\n", p[0]);
        printf("io type=%d\n", p[1]);
        printf("selector=%d\n", p[2]);
        printf("# io args=%d\n", p[3]);
        for (i=0; i<p[3]; i++) {
            printf("%08x  ", p[4+i]);
        }
        printf("\n");

        q=p+4+p[3];
        printf("addr type=%d\n", q[0]);
        printf("# addr args=%d\n", q[1]);
        for (i=0; i<q[1]; i++) {
            printf("%08x  ", q[2+i]);
        }
        printf("\n");
    } else {
        printf("old version\n");
    }
}
/*****************************************************************************/
/* downloaddataout (bisher)
 *  <Index> <InOutTyp> <Buffersize> <Prioritaet> <Addresstype> <Address>
 *
 * downloaddataout (neu)
 *  <Index> <InOutTyp> -1 <InOutArgs> <Adresstype> <AddressArgs>
 *  InOutArgs: Num {arg}
 *  AdressArgs: Num <weiter wie gehabt>
 *
 * (is the following for InOutArgs really correct?)
 * InOutArgs[0]: 0: don't write filemarks to tape if -o:nofilemark is also given
 *               1: write filemarks
 * InOutArgs[1]: triggermask; ignore cluster unless InOutArgs[1]&cluster.triggers!=0
 *
 * AddressArgs for Addr_Tape:
 * AddressArgs[0]: density
 *
 * AddressArgs for Addr_File:
 * AddressArgs[0]: reopen a new file every X seconds
 *
 */

static errcode downloaddataout(ems_u32* p, unsigned int len)
{
    int res;
    ems_u32 *q, *r, *s=0;
    ems_i32 *ip=(ems_i32*)p;
    unsigned int qlen, rlen, slen;
    unsigned int index;

    T(downloaddataout)
    D(D_REQ, {
        unsigned int i;
        printf("DownloadDomain Dataout(");
        for (i=0; i<len; i++) printf("%d%s", p[i], i+1<len?", ":"");
        printf(")\n");}
    )
    downloaddataout_dump(p, len);

#ifdef DATAOUT_SIMPLE
    return Err_ObjDef;
#else
#ifdef DATAOUT_MULTI
    if ((unsigned int)len<5)
        return Err_ArgNum;
    if (ip[2]==-1) { /* newer protocol version */
        q=p+4;
        qlen=p[3];
        if (len<qlen+6) {
            *outptr++=1;
            return Err_ArgNum;
        }
        r=p+qlen+6;
        rlen=p[qlen+5];
        if (len<qlen+rlen+6) {
            *outptr++=2;
            return Err_ArgNum;
        }
        if (len>qlen+rlen+6) {
            s=p+qlen+rlen+7;
            slen=p[qlen+rlen+6];
        } else {
            slen=0;
        }
    } else { /* older protocol version */
        q=p+2;
        qlen=2;
        r=p+5;
        rlen=len-5;
        slen=0;
    }
    index=p[0];
    if (index>=MAX_DATAOUT)
        return Err_IllDomain;
    if (dataout[index].bufftyp!=-1)
        return Err_DomainDef;

    dataout[index].inout_arg_num=0;
    dataout[index].inout_args=0;
    dataout[index].address_arg_num=0;
    dataout[index].address_args=0;
    if (ip[2]==-1) {
        /* Buffersize und Prioritaet muessen da sein */
        if (qlen<2)
            return Err_ArgNum;
        dataout[index].buffsize=q[0];
        dataout[index].wieviel=q[1];
        dataout[index].addrtyp=p[qlen+4];
        if (qlen>2) {
            unsigned int i;
            dataout[index].inout_args=(int*)malloc((qlen-2)*sizeof(int));
            if (dataout[index].inout_args==0)
                return Err_NoMem;
            for (i=0; i<qlen-2; i++)
                dataout[index].inout_args[i]=q[i+2];
            dataout[index].inout_arg_num=qlen-2;
        }
    } else {
        dataout[index].buffsize=p[2];
        dataout[index].wieviel=p[3];
        dataout[index].addrtyp=p[4];
    }

    switch (dataout[index].addrtyp) {
#ifdef OSK
    case Addr_Raw:
        if (rlen!=1) {
            *outptr++=3;
            return Err_ArgNum;
        }
        dataout[index].addr.addr=(int*)r[0];
        break;
#endif
    case Addr_Modul:
        if((rlen<1)||(rlen!=xdrstrlen(r))) {
            *outptr++=4;
            return Err_ArgNum;
        }
        xdrstrcdup(&dataout[index].addr.modulname, r);
        if (!(dataout[index].addr.modulname))
            return Err_NoMem;
        break;
    case Addr_Socket:
        if (rlen==1) {       /* passive */
            dataout[index].addr.inetsock.port=r[0];
            dataout[index].addr.inetsock.host=INADDR_ANY;
        } else if (rlen==2) { /* active */
            dataout[index].addr.inetsock.host=r[0];
            dataout[index].addr.inetsock.port=r[1];
        } else {
            *outptr++=5;
            return Err_ArgNum;
        }
        break;
    case Addr_V6Socket: {
        struct addrinfo hints;
        int res;

        if (rlen<1) { /* we need the flags */
            *outptr++=6;
            return Err_ArgNum;
        }
        dataout[index].addr.inetV6sock.flags=r[0];
        r++; rlen--;
        if (!(dataout[index].addr.inetV6sock.flags&ip_passive)) {
            if ((rlen<1)||(rlen<xdrstrlen(r))) { /* we need a node string */
                *outptr++=7;
                return Err_ArgNum;
            }
            xdrstrcdup(&dataout[index].addr.inetV6sock.node, r);
            rlen-=xdrstrlen(r);
            r+=xdrstrlen(r);
            if (!dataout[index].addr.inetV6sock.node)
                return Err_NoMem;
        } else {
            dataout[index].addr.inetV6sock.node=0;
        }
        if ((rlen<1)||(rlen<xdrstrlen(r))) { /* we need a service string */
            *outptr++=8;
            free(dataout[index].addr.inetV6sock.node);
            return Err_ArgNum;
        }
        xdrstrcdup(&dataout[index].addr.inetV6sock.service, r);
        if (!dataout[index].addr.inetV6sock.service) {
            free(dataout[index].addr.inetV6sock.node);
            return Err_NoMem;
        }

        memset(&hints, 0, sizeof(struct addrinfo));
        if (dataout[index].addr.inetV6sock.flags&ip_passive)
            hints.ai_flags=AI_PASSIVE;
        if (dataout[index].addr.inetV6sock.flags&ip_v6only)
            hints.ai_family=AF_INET6;
        else if (dataout[index].addr.inetV6sock.flags&ip_v4only)
            hints.ai_family=AF_INET; /* to be corrected later */
        else
            hints.ai_family=AF_INET;
        hints.ai_socktype=SOCK_STREAM; /* ??? */
        hints.ai_protocol=IPPROTO_TCP; /* ??? */
        res=getaddrinfo(
                dataout[index].addr.inetV6sock.node,
                dataout[index].addr.inetV6sock.service,
                &hints,
                &dataout[index].addr.inetV6sock.addr);
        if (res) {
            printf("dom_dataout(%d):getaddrinfo: %s\n",
                    index, gai_strerror(res));
            free(dataout[index].addr.inetV6sock.node);
            free(dataout[index].addr.inetV6sock.service);
            return Err_ArgRange;
        }
        }
        break;
    case Addr_File:
    case Addr_LocalSocket:
    case Addr_Tape:
        if ((rlen<1)||(rlen!=xdrstrlen(r))) {
            *outptr++=6;
            return Err_ArgNum;
        }
        xdrstrcdup(&dataout[index].addr.filename, r);
        if (!(dataout[index].addr.filename))
            return Err_NoMem;
        break;
    case Addr_Null:
        break;
    default:
        return Err_AddrTypNotImpl;
    }

    if (slen) {
        unsigned int i;
        dataout[index].address_args=(int*)malloc(slen*sizeof(int));
        if (dataout[index].address_args==0)
            return Err_NoMem;
        for (i=0; i<slen; i++)
            dataout[index].address_args[i]=s[i];
        dataout[index].address_arg_num=slen;
        printf("address_args:\n");
        for (i=0; i<dataout[index].address_arg_num; i++)
            printf("[%d] %d\n", i, dataout[index].address_args[i]);
    }
    dataout[index].logfilename=0;
    dataout[index].loginfo=0;
    dataout[index].runlinkdir=0;
    dataout[index].runlinkabsolute=0;
    dataout[index].fadvise_flag=0;
    dataout[index].fadvise_data=0;

    dataout[index].bufftyp=p[1];
    res=insert_dataout(index);
    if (res) {
        free_dataout_res(index);
        dataout[index].bufftyp= -1;
        return res;
    } else {
        return OK;
    }
#else
    return Err_NotImpl;
#endif /* MULTI */
#endif
}

/*****************************************************************************/

#if defined (DATAOUT_SIMPLE) || defined (DATAOUT_MULTI)
static errcode getdataout(int index)
{
    ems_u32* help=0;
    int i;
    int ionum=dataout[index].inout_arg_num;
    int adnum=dataout[index].address_arg_num;
    int usenew=(ionum>0) || (adnum>0);

    if (dataout[index].bufftyp==-1)
        return Err_NoDomain;
    *outptr++=dataout[index].bufftyp;
    if (usenew) {*outptr++=-1; *outptr++=ionum+2;}
    *outptr++=dataout[index].buffsize;
    *outptr++=dataout[index].wieviel;
    if (usenew) {for (i=0; i<ionum; i++)
            *outptr++=dataout[index].inout_args[i];}
    *outptr++=dataout[index].addrtyp;
    if (usenew) help=outptr++;

    switch(dataout[index].addrtyp) {
#ifdef OSK
    case Addr_Raw:
        *outptr++=(int)(dataout[index].addr.addr);
        break;
#endif
    case Addr_Modul:
        outptr=outstring(outptr, dataout[index].addr.modulname);
#ifdef OSK
        *outptr++=(int)get_dataout_addr(index);
#endif
        break;
    case Addr_Socket:
        if (dataout[index].addr.inetsock.host==INADDR_ANY) { /* passiv */
            *outptr++=dataout[index].addr.inetsock.port;
        } else { /* active */
            *outptr++=dataout[index].addr.inetsock.host;
            *outptr++=dataout[index].addr.inetsock.port;
        }
        break;
    case Addr_V6Socket:
        *outptr++=dataout[index].addr.inetV6sock.flags;
        if (!(dataout[index].addr.inetV6sock.flags&ip_passive))
            outptr=outstring(outptr, dataout[index].addr.inetV6sock.node);
        outptr=outstring(outptr, dataout[index].addr.inetV6sock.service);
        break;
    case Addr_File:
    case Addr_LocalSocket:
    case Addr_Tape:
        outptr=outstring(outptr, dataout[index].addr.filename);
        break;
    case Addr_Null:
        break;
    default:
        return Err_Program;
    }
    if (usenew) {
        *help=outptr-help-1;
        if (adnum) {
            *outptr++=adnum;
            for (i=0; i<adnum; i++)
                    *outptr++=dataout[index].address_args[i];
        }
    }
    return OK;
}
#endif
/*****************************************************************************/
static errcode uploaddataout(ems_u32* p, unsigned int len)
{
    int index;

    if (len!=1)
        return Err_ArgNum;

#ifdef DATAOUT_SIMPLE
    return getdataout(0);
#else
#ifdef DATAOUT_MULTI
    index=p[0];
    if ((unsigned int)index>=MAX_DATAOUT)
        return Err_IllDomain;
    return(getdataout(index));
#else
    return Err_NotImpl;
#endif /* MULTI */
#endif
}
/*****************************************************************************/
/*
p[0] : Domain-id
*/
static errcode deletedataout(ems_u32* p, unsigned int len)
{
#ifdef DATAOUT_SIMPLE
    T(deletedataout)
    return Err_ObjNonDel;
#else
#ifdef DATAOUT_MULTI
    unsigned int index;
    errcode res;

    T(objects/domain/dom_dataout.c:deletedataout)

    if (len!=1)
        return Err_ArgNum;
    index=p[0];
    D(D_REQ, printf("deletedataout(%d)\n", p[0]);)
    if (index>=MAX_DATAOUT)
        return Err_IllDomain;
    if (dataout[index].bufftyp==-1)
        return Err_NoDomain;
    if ((res=remove_dataout(index)))
        return res;
    free_dataout_res(index);
    dataout[index].bufftyp= -1;
    return OK;
#else
    return Err_NotImpl;
#endif
#endif
}

/*****************************************************************************/
/* static domobj *lookup_dom_do(ems_u32* id, unsigned int idlen, unsigned int* remlen) */
static objectcommon* lookup_dom_do(ems_u32* id, unsigned int idlen, unsigned int* remlen)
{
    T(lookup_dom_do)
    if (idlen>0) {
        *remlen=idlen;
#ifdef DATAOUT_SIMPLE
        if (id[0]!=0)
            return 0;
#else
#ifdef DATAOUT_MULTI
        if ((unsigned int)id[0]>=MAX_DATAOUT)
            return 0;
#else
        return 0;
#endif
#endif
        return (objectcommon*)&dom_do_obj;
    } else {
        *remlen=0;
        return (objectcommon*)&dom_do_obj;
    }
}

static ems_u32 *dir_dom_do(ems_u32* ptr)
{
    int i;
    T(dir_dom_do)
#ifdef DATAOUT_SIMPLE
    *ptr++=0;
#else
#ifdef DATAOUT_MULTI
   for (i=0; i<MAX_DATAOUT; i++)
        if (dataout[i].bufftyp!=-1)
            *ptr++=i;
#endif
#endif
    return ptr;
}

domobj dom_do_obj={
    {
        0, 0,
        /*(lookupfunc)*/lookup_dom_do,
        dir_dom_do,
        0
    },
    downloaddataout,
    uploaddataout,
    deletedataout
};
/*****************************************************************************/
/*****************************************************************************/
