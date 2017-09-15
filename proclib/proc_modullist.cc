/*
 * proc_modullist.hxx
 * created 08.06.97
 */

#include <errno.h>
#include <stdlib.h>
#include <proc_modullist.hxx>
#include <proc_conf.hxx>
#include <errors.hxx>
#include <versions.hxx>

VERSION("2009-Feb-24", __FILE__, __DATE__, __TIME__,
"$ZEL: proc_modullist.cc,v 2.10 2010/06/20 22:43:21 wuestner Exp $")
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
        for (int i=0; i<max; i++) {
            entry[i].modulclass=modul_unspec;
            entry[i].address.unspec.addr=ib.getint();
            entry[i].modultype=ib.getint();
        }
    } else {
        for (int i=0; i<max; i++) {
            enum Modulclass modulclass;
            entry[i].modultype=ib.getint();
            modulclass=(enum Modulclass)ib.getint();
            entry[i].modulclass=modulclass;
            switch (modulclass) {
            case modul_none:
                break;
            case modul_unspec:
                entry[i].address.unspec.addr=ib.getint();
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
                {}
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
/*****************************************************************************/
void C_modullist::add(ml_entry& newentry)
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
    entry[size_++]=newentry;
}
/*****************************************************************************/
/*****************************************************************************/
