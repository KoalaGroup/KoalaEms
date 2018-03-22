/*
 * proc_modullist.hxx
 * created 08.06.97
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <proc_modullist.hxx>
#include <proc_conf.hxx>
#include <errors.hxx>
#include <versions.hxx>

VERSION("2016-Jan-15", __FILE__, __DATE__, __TIME__,
"$ZEL: proc_modullist.cc,v 2.12 2016/05/02 15:27:43 wuestner Exp $")
#define XVERSION

/*****************************************************************************/
C_modullist::C_modullist(int n)
:max(n), size_(0)
{
    entry=new ml_entry[max];
    if (entry==0)
        throw new C_unix_error(errno, "alloc modullist");
}
/*****************************************************************************/
C_modullist::C_modullist(const C_confirmation* conf, int version)
{
    C_inbuf ib(conf->buffer(), conf->size());

    ib++;
    size_=max=ib.getint();
    entry=new ml_entry[max];
    if (entry==0) 
        throw new C_unix_error(errno, "alloc modullist");
    if (version==0) {
        throw new C_program_error(
            "C_modullist: old version no longer supported");
#if 0
        for (int i=0; i<max; i++) {
            entry[i].modulclass=modul_unspec;
            entry[i].address.unspec.addr=ib.getint();
            entry[i].modultype=ib.getint();
        }
#endif
    } else {
        for (int i=0; i<max; i++) {
            enum Modulclass modulclass;
            int n;

            entry[i].modultype=ib.getint();
            modulclass=static_cast<enum Modulclass>(ib.getint());
            entry[i].modulclass=modulclass;
            switch (modulclass) {
            case modul_none:
                break;
            case modul_unspec_:
                throw new C_program_error(
                    "C_modullist: modul_unspec not supported");
#if 0
                entry[i].address.unspec.addr=ib.getint();
#endif
                break;
            case modul_generic:
                break;
            case modul_camac:
            case modul_fastbus:
            case modul_vme:
            case modul_lvd:
            case modul_can:
            case modul_perf:
                entry[i].address.adr2.crate=ib.getint();
                entry[i].address.adr2.addr=ib.getint();
                break;
            case modul_caenet:
            case modul_sync:
            case modul_pcihl:
            case modul_invalid:
                break;
            case modul_ip:
                entry[i].address.ip.address=0;
                entry[i].address.ip.protocol=0;
                entry[i].address.ip.rport=0;
                entry[i].address.ip.lport=0;
                n=ib.getint();
                if (n>0)
                    ib>>entry[i].address.ip.address;
                if (n>1)
                    ib>>entry[i].address.ip.protocol;
                if (n>2)
                    ib>>entry[i].address.ip.rport;
                if (n>3)
                    ib>>entry[i].address.ip.lport;
                break;
            default:
                cerr<<"C_modullist::C_modullist(modulclass="<<modulclass
                    <<") not implemented yet"
                    <<endl;
            }
        }
    }
}
/*****************************************************************************/
C_modullist::~C_modullist(void)
{
    delete[] entry;
}
/*****************************************************************************/
#if 0 /* modul_unspec no longer exists (old protocol version) */
void C_modullist::add(int typ, int address)
{
    if (size_>=max) {
        max+=25;
        ml_entry* help=new ml_entry[max];
        if (!help)
            throw new C_unix_error(errno, "realloc modullist");
        for (int i=0; i<size_; i++)
            help[i]=entry[i];
        delete[] entry;
        entry=help;
    }
    entry[size_].modultype=typ;
    entry[size_].modulclass=modul_unspec;
    entry[size_].address.unspec.addr=address;
    size_++;
}
#endif
/*****************************************************************************/
void C_modullist::add(ml_entry& newentry)
{
    if (size_>=max) {
        max+=25;
        ml_entry* help=new ml_entry[max];
        if (!help)
            throw new C_unix_error(errno, "realloc modullist");
        memset(help, 0, max*sizeof(ml_entry));
        for (int i=0; i<size_; i++)
            help[i]=entry[i];
        delete[] entry;
        entry=help;
    }
    entry[size_++]=newentry;
}
/*****************************************************************************/
C_modullist::ml_entry::~ml_entry()
{
    switch (modulclass) {
    case modul_ip:
        free(address.ip.address);
        free(address.ip.protocol);
        free(address.ip.rport);
        free(address.ip.lport);
        break;
    default:
        {}
    }
}
/*****************************************************************************/
C_modullist::ml_entry&
C_modullist::ml_entry::operator=(C_modullist::ml_entry const& other)
{
    if (this != &other) {

        switch (modulclass) {
        case modul_ip:
            free(address.ip.address);
            free(address.ip.protocol);
            free(address.ip.rport);
            free(address.ip.lport);
            break;
        default:
            {}
        }

        modultype=other.modultype;
        modulclass=other.modulclass;

        switch (modulclass) {
        case modul_ip:
            address.ip.address=0;
            address.ip.protocol=0;
            address.ip.rport=0;
            address.ip.lport=0;
            if (other.address.ip.address)
                address.ip.address=strdup(other.address.ip.address);
            if (other.address.ip.protocol)
                address.ip.protocol=strdup(other.address.ip.protocol);
            if (other.address.ip.rport)
                address.ip.rport=strdup(other.address.ip.rport);
            if (other.address.ip.lport)
                address.ip.lport=strdup(other.address.ip.lport);
            break;
        default:
            address.adr2.crate=other.address.adr2.crate;
            address.adr2.addr=other.address.adr2.addr;
        }

    }
    return *this;
}
/*****************************************************************************/
/*****************************************************************************/
