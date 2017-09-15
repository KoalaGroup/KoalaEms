/*
 * ral_testmatrix.hxx
 * created 14.May.2004 PW
 *
 * $ZEL: ral_testmatrix.hxx,v 2.3 2004/11/26 22:20:48 wuestner Exp $
 */

#ifndef _ral_testmatrix_hxx_
#define _ral_testmatrix_hxx_

#include "ral_matrix.hxx"

class ral_testmatrix {
    public:
        ral_testmatrix(int* cards);
    private:
        int _multiples[17];
    public:
        int cardmask;
        int channelmask;
        enum generatortype {none, irregular, gen_and, gen_or, gen_xor};
        static const char* gentypenames[];
        generatortype gentype;
        ral_matrix matrix[16];
        ral_matrix ematrix[16];
        int errors[16];
        int sum_of_multiples[16];
        int hits[16], xhits[16];
        int multiples(unsigned int) const;
        int& multiples(unsigned int);

        void generate(void);
        void generate(int cardmask, int channelmask, generatortype gentype);
        int diff(int crate);
        int diff(ral_matrix&, int crate, int errormatrix);
        void print_generator(ostream&) const;
        void print_statistics(ostream&) const;
};

#endif
