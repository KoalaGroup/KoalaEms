/*
 * ral_matrix.hxx
 * created 14.May.2004 PW
 *
 * $ZEL: ral_matrix.hxx,v 2.2 2004/11/26 14:36:35 wuestner Exp $
 */

#ifndef _ral_matrix_hxx_
#define _ral_matrix_hxx_

class ral_matrix {
    public:
        ral_matrix() :cards_(0) {}
    private:
        int matrix[16][16];
        int cards_;
    public:
        void clear(void) {memset(matrix, 0, sizeof(matrix));}
        int hits(void) const;
        int get(int card, int chan) const {return matrix[card][chan];}
        //int operator()(int card, int chan) const {return matrix[card][chan];}
        //int& operator()(int card, int chan) {return matrix[card][chan];}
        int operator()(unsigned int card, unsigned int chan) const;
        int& operator()(unsigned int card, unsigned int chan);
        int cards() const {return cards_;}
        void cards(int _cards) {cards_=_cards;}

        ostream& print(ostream&) const;
};

ostream& operator <<(ostream&, const ral_matrix&);

#endif
