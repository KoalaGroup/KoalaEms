/*
 * support/fifo.cc
 * 
 * created 2015-10-12 PW
 */

#include "config.h"
#include <fstream>
#include <cerrno>
#include "fifo.hxx"
#include "versions.hxx"

VERSION("2015-10-14", __FILE__, __DATE__, __TIME__,
"$ZEL: fifo.cc,v 1.1 2015/10/22 20:17:54 wuestner Exp $")
#define XVERSION

using namespace std;

/*****************************************************************************/
fifo::~fifo()
{
}
/*****************************************************************************/
bool fifo::empty(void)
{
    return !first || first->empty();
}
/*****************************************************************************/
int64_t fifo::num(void)
{
    class fblock *block=first;
    int64_t n=0;
    while (block) {
        n+=block->num();
    }
    return n;
}
/*****************************************************************************/
int64_t fifo::get(void)
{
    if (!empty()) {
        return first->get();
    }
    return 17; // or throw something
}
/*****************************************************************************/
void fifo::drop(void)
{
    if (!empty()) {
        first->drop();
        if (first->empty()) {
            if (first==last) {
                first->clear();
            } else {
                struct fblock *help;
                help=first;
                first=first->next;
                delete help;
                if (!first)
                    last=0;
            }
        }
    }
    // throw something
}
/*****************************************************************************/
int64_t fifo::getdrop(void)
{
    int64_t val;

    if (empty()) {
        cout<<"fifo::getdrop("<<this<<" empty"<<endl;
        exit(0);
    }

    val=first->getdrop();

    if (first->empty()) {
        if (first==last) {
            first->clear();
        } else {
            struct fblock *help;
            help=first;
            first=first->next;
            delete help;
            if (!first)
                last=0;
        }
    }

    return val;
}
/*****************************************************************************/
void fifo::put(int64_t val)
{
    if (!last || last->full()) {
        class fblock *bl=new fblock;
        if (!bl) {
            cout<<"cannot allocate new block for fifo"<<endl;
            return; // throw something!
        }
        if (last)
            last->next=bl;
        last=bl;
        if (!first)
            first=bl;
    }
    last->put(val);
}
/*****************************************************************************/
bool fblock::empty(void)
{
//    cout<<"block::empty: first="<<first<<" last="<<last<<endl;
    return blast<bfirst;
}
/*****************************************************************************/
bool fblock::full(void)
{
    return blast==1023;
}
/*****************************************************************************/
void fblock::clear(void)
{
    bfirst=0;
    blast=-1;
}
/*****************************************************************************/
int64_t fblock::num(void)
{
    if (blast>=bfirst)
        return blast-bfirst+1;
    else
        return 0;
}
/*****************************************************************************/
int64_t fblock::get(void)
{
    if (empty()) {
        cout<<"block::get("<<this<<" empty"<<endl;
        exit(0);
    }
    return data[bfirst];
}
/*****************************************************************************/
int64_t fblock::getdrop(void)
{
    if (empty()) {
        cout<<"block::getdrop("<<this<<" empty"<<endl;
        exit(0);
    }
    
    return data[bfirst++];
}
/*****************************************************************************/
void fblock::drop(void)
{
    if (empty()) {
        cout<<"block::drop("<<this<<"): empty"<<endl;
        exit(0);
    }
    bfirst++;
}
/*****************************************************************************/
void fblock::put(int64_t val)
{
    data[++blast]=val;
}
/*****************************************************************************/
/*****************************************************************************/
