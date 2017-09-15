/*
 * ral_testmatrix.cc
 * created 14.May.2004 PW
 */

#include "config.h"
#include "cxxcompat.hxx"
#include <errors.hxx>

#include "ral_testmatrix.hxx"

#include <versions.hxx>

VERSION("May 14 2004", __FILE__, __DATE__, __TIME__,
"$ZEL: ral_testmatrix.cc,v 2.3 2004/11/26 22:20:47 wuestner Exp $")

/*****************************************************************************/
const char* ral_testmatrix::gentypenames[]={
    "none",
    "irregular",
    "and",
    "or",
    "xor"
};
/*****************************************************************************/
ral_testmatrix::ral_testmatrix(int* cards)
{
    int i;
    for (i=0; i<16; i++) {
        matrix[i].cards(cards[i]);
        ematrix[i].cards(cards[i]);
    }
}
/*****************************************************************************/
void
ral_testmatrix::print_generator(ostream& os) const
{
    if (gentype!=irregular) {
        os<<hex<<"cardmask="<<(ems_u16)cardmask
          <<" channelmask="<<(ems_u16)channelmask
          <<dec
          <<" operator: ";
    }
    os<<gentypenames[gentype];
}
/*****************************************************************************/
int
ral_testmatrix::diff(int crate)
{
    int card, chan;
    int i, e=0;

    errors[crate]=0;
    for (card=0; card<matrix[crate].cards(); card++) {
        for (chan=0; chan<16; chan++) {
            if (matrix[crate](card, chan)!=ematrix[crate](card, chan)) {
                errors[crate]++;
                e++;
            }
        }
    }
    if (!e) return 0;

    for (i=0; i<17; i++) multiples(i)=0;
    sum_of_multiples[crate]=0;
    hits[crate]=0, xhits[crate]=0;

    for (card=0; card<matrix[crate].cards(); card++) {
        for (chan=0; chan<16; chan++) {
            int x2=ematrix[crate](card, chan);
            hits[crate]+=matrix[crate](card, chan);
            xhits[crate]+=x2;
            if (x2<=16) {
                multiples(x2)++;
                if (x2>1) sum_of_multiples[crate]++;
            } else {
                cout<<"ral_testmatrix::diff: x2="<<x2<<endl;
            }
        }
    }
    return e;
}
/*****************************************************************************/
int
ral_testmatrix::diff(ral_matrix& xmatrix, int crate, int errormatrix)
{
    int card, chan;
    int _errors=0;
    ral_matrix& dmatrix=errormatrix?ematrix[crate]:matrix[crate];

//cerr<<"xcards="<<xmatrix.cards<<" dcards="<<dmatrix.cards<<endl;
    if (xmatrix.cards()!=dmatrix.cards()) {
        C_error* e;
        cerr<<"vor new C_program_error"<<endl;;
        e=new C_program_error("ral_testmatrix::diff: "
                "number of cards do not match.");
        cerr<<"nach new C_program_error"<<endl;;
        throw e;
    }

    for (card=0; card<dmatrix.cards(); card++) {
        for (chan=0; chan<16; chan++) {
            if (dmatrix(card, chan)!=xmatrix(card, chan)) _errors++;
        }
    }
    return _errors;
}
/*****************************************************************************/
void
ral_testmatrix::generate(void)
{
    int card, chan, crate;

    for (crate=0; crate<16; crate++) {
        if (!matrix[crate].cards()) continue;

        matrix[crate].clear();
        switch (gentype) {
        case irregular:
            cout<<"ral_testmatrix::generate: cannot use 'irregular'."<<endl;
            break;
        case gen_and:
            for (card=0; card<matrix[crate].cards(); card++) {
                if (cardmask&(1<<card)) {
                    for (chan=0; chan<16; chan++) {
                        int b=!!(channelmask&(1<<chan));
                        matrix[crate](card, chan)=b;
                    }
                }
            }
            break;
        case gen_or:
            for (card=0; card<matrix[crate].cards(); card++) {
                int a=!!(cardmask&(1<<card));
                for (chan=0; chan<16; chan++) {
                    int b=!!(channelmask&(1<<chan));
                    matrix[crate](card, chan)=a||b;
                }
            }
            break;
        case gen_xor:
            for (card=0; card<matrix[crate].cards(); card++) {
                int a=!!(cardmask&(1<<card));
                for (chan=0; chan<16; chan++) {
                    int b=!!(channelmask&(1<<chan));
                    matrix[crate](card, chan)=a!=b;
                }
            }
            break;
        }
    }
}
/*****************************************************************************/
void
ral_testmatrix::generate(int _cardmask, int _channelmask,
        generatortype _gentype)
{
    cardmask=_cardmask;
    channelmask=_channelmask;
    gentype=_gentype;
    generate();
}
/*****************************************************************************/
void
ral_testmatrix::print_statistics(ostream& os) const
{
    int crate;
    for (crate=0; crate<16; crate++) {
        os << "errors["<<crate<<"]="<<errors[crate]<<endl;
        if (hits!=xhits)
            os << "hit difference: "<<(xhits-hits)<<endl;
        if (sum_of_multiples[crate]>2) {
            int i;
            os << "sum_of_multiples["<<crate<<"]="<<sum_of_multiples[crate]<<endl;
            for (i=0; i<17; i++)
                os << multiples(i)<<' ';
            os<<endl;
        }
    }
}
/*****************************************************************************/
int
ral_testmatrix::multiples(unsigned int idx) const
{
    if (idx>=17) {
        cerr<<"ral_testmatrix::multiples("<<idx<<"): "
            <<"invalid subscript"<<endl;
        exit(0);
    }
    return _multiples[idx];
}
/*****************************************************************************/
int&
ral_testmatrix::multiples(unsigned int idx)
{
    if (idx>=17) {
        cerr<<"ral_testmatrix::multiples("<<idx<<"): "
            <<"invalid subscript"<<endl;
        exit(0);
    }
    return _multiples[idx];
}
/*****************************************************************************/
/*****************************************************************************/
