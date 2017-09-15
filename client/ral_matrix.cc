/*
 * ral_matrix.cc
 * created 14.May.2004 PW
 */

#include "config.h"
#include "cxxcompat.hxx"

#include "ral_matrix.hxx"

#include <versions.hxx>

VERSION("May 14 2004", __FILE__, __DATE__, __TIME__,
"$ZEL: ral_matrix.cc,v 2.2 2004/11/26 22:20:47 wuestner Exp $")

/*****************************************************************************/
ostream& operator <<(ostream& os, const ral_matrix& rhs)
{
    return(rhs.print(os));
}
/*****************************************************************************/
ostream&
ral_matrix::print(ostream& os) const
{
    int card, chan;

    for (card=0; card<cards_; card++) {
        for (chan=0; chan<16; chan++) {
            os<<matrix[card][chan];
        }
        os<<endl;
    }
    os<<endl;
    return os;
}
/*****************************************************************************/
int
ral_matrix::hits(void) const
{
    int card, chan, s=0;

    for (card=0; card<cards_; card++) {
        for (chan=0; chan<16; chan++) {
            s+=matrix[card][chan];
        }
    }
    return s;
}
/*****************************************************************************/
int
ral_matrix::operator()(unsigned int card, unsigned int chan) const
{
    if ((card>=16) || (card>=cards_) || (chan>=16)) {
        cerr<<"ral_matrix::operator()(card="<<card<<", channel="<<chan<<"): "
            <<"invalid subscript"<<endl;
        exit(0);
    }
    return matrix[card][chan];
}
/*****************************************************************************/
int&
ral_matrix::operator()(unsigned int card, unsigned int chan)
{
    if ((card>=16) || (card>=cards_) || (chan>=16)) {
        cerr<<"ral_matrix::operator()(card="<<card<<", channel="<<chan<<")const: "
            <<"invalid subscript"<<endl;
        exit(0);
    }
    return matrix[card][chan];
}
/*****************************************************************************/
/*****************************************************************************/
