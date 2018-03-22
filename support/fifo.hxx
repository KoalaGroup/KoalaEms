/*
 * support/fifo.hxx
 * 
 * created 2015-10-12 PW
 *
 * $ZEL: fifo.hxx,v 1.1 2015/10/22 20:17:54 wuestner Exp $
 */

#ifndef _fifo_hxx_
#define _fifo_hxx_

#include <cstdint>


        class fblock {
            public:
                fblock(): bfirst(0), blast(-1), next(0) {};
            protected:
                int bfirst; // first valid word
                int blast;  // last valid word
                            // empty if last<first
                int64_t data[1024];
            public:
                class fblock *next;
                bool empty(void);
                bool full(void);
                int64_t num(void);
                int64_t get(void);
                int64_t getdrop(void);
                void drop(void);
                void put(int64_t);
                void clear(void);
        };

class fifo {
    public:
        fifo(int id):ident(id), first(0), last(0) {};
        virtual ~fifo();
    protected:
        int ident;
        class fblock *first;
        class fblock *last;

    public:
        bool empty(void);
        int64_t num(void);
        int64_t get(void);
        int64_t getdrop(void);
        void drop(void);
        void put(int64_t);
};

#endif
