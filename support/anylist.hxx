/*
 * support/anylist.hxx
 * 
 * created 2014-09-10 PW
 *
 * $ZEL: anylist.hxx,v 1.1 2014/09/12 13:06:20 wuestner Exp $
 */

#ifndef _anylist_hxx_
#define _anylist_hxx_


#include "config.h"
#include <typeinfo>
#include <iostream>
#include <stdint.h>
#include "outbuf.hxx"

using namespace std;

class C_anylist {
    public:
        C_anylist(int);
        ~C_anylist();

        void add(uint32_t);
        void add(const char*);
        void add(const char*, int);
        int num() const {return num_;}

        struct value {
            const std::type_info *type;
            void *val;
        };
        value *list;

    protected:
        ostream& print(ostream&) const;
        C_outbuf& put(C_outbuf&) const;

    private:
        void enlarge(void);
        int size_;
        int num_;
};

#endif

/*****************************************************************************/
/*****************************************************************************/
